#include <QApplication>
#include <QFileInfo>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QString>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>

#include "../storage/InMemoryHistoryRepository.h"
#ifdef COPYCLICKK_HAS_SQLITE
#include "../storage/SqliteHistoryRepository.h"
#endif
#include "TrayAppController.h"

namespace {

constexpr auto kShowHistoryCommand = "show-history";

QString singleInstanceServerName() {
  const QString user = qEnvironmentVariable("USER");
  if (!user.isEmpty()) {
    return QStringLiteral("copyclickk-tray-%1").arg(user);
  }
  return QStringLiteral("copyclickk-tray-default");
}

bool notifyRunningInstance(const QString& serverName) {
  QLocalSocket socket;
  socket.connectToServer(serverName, QIODevice::WriteOnly);
  if (!socket.waitForConnected(200)) {
    return false;
  }

  socket.write(kShowHistoryCommand);
  socket.flush();
  (void)socket.waitForBytesWritten(200);
  socket.disconnectFromServer();
  return true;
}

}  // namespace

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("CopyClickk");
  app.setApplicationDisplayName("CopyClickk");

#ifdef COPYCLICKK_ASSETS_DIR
  const QString appIconSvgPath = QStringLiteral(COPYCLICKK_ASSETS_DIR) + "/icons/app/app.svg";
  const QString appIconPngPath = QStringLiteral(COPYCLICKK_ASSETS_DIR) + "/icons/app/app.png";
  const QString trayIconPath = QStringLiteral(COPYCLICKK_ASSETS_DIR) + "/icons/tray/clipboard.svg";

  for (const QString& path : {appIconSvgPath, appIconPngPath, trayIconPath}) {
    if (QFileInfo::exists(path)) {
      const QIcon icon(path);
      if (!icon.isNull()) {
        app.setWindowIcon(icon);
        break;
      }
    }
  }
#endif

  const QString serverName = singleInstanceServerName();
  if (notifyRunningInstance(serverName)) {
    return EXIT_SUCCESS;
  }

  QLocalServer instanceServer;
  if (!instanceServer.listen(serverName)) {
    QLocalServer::removeServer(serverName);
    if (!instanceServer.listen(serverName)) {
      return EXIT_FAILURE;
    }
  }

  std::shared_ptr<copyclickk::IHistoryRepository> repository;

#ifdef COPYCLICKK_HAS_SQLITE
  const char* home = std::getenv("HOME");
  const std::string baseDir = home != nullptr ? std::string(home) + "/.local/share/copyclickk" : ".";
  const std::string dbPath = baseDir + "/history.db";
  std::error_code ec;
  std::filesystem::create_directories(baseDir, ec);
  std::filesystem::permissions(baseDir,
                               std::filesystem::perms::owner_all,
                               std::filesystem::perm_options::replace,
                               ec);

  repository = std::make_shared<copyclickk::SqliteHistoryRepository>(dbPath);
  std::filesystem::permissions(dbPath,
                               std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                               std::filesystem::perm_options::replace,
                               ec);
#else
  repository = std::make_shared<copyclickk::InMemoryHistoryRepository>();
#endif

  copyclickk::TrayAppController controller(repository);
  controller.start();

  QObject::connect(&instanceServer, &QLocalServer::newConnection, &app, [&instanceServer, &controller]() {
    while (instanceServer.hasPendingConnections()) {
      QLocalSocket* socket = instanceServer.nextPendingConnection();
      if (socket == nullptr) {
        continue;
      }

      QObject::connect(socket, &QLocalSocket::readyRead, socket, [&controller, socket]() {
        const QByteArray command = socket->readAll();
        if (command.contains(kShowHistoryCommand)) {
          controller.showHistoryFromExternalTrigger();
        }
        socket->disconnectFromServer();
      });
      QObject::connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
    }
  });

  return app.exec();
}
