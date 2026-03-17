#include <QApplication>
#include <QFileInfo>
#include <QIcon>
#include <QString>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

#include "../storage/InMemoryHistoryRepository.h"
#ifdef COPYCLICKK_HAS_SQLITE
#include "../storage/SqliteHistoryRepository.h"
#endif
#include "TrayAppController.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

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

  std::shared_ptr<copyclickk::IHistoryRepository> repository;

#ifdef COPYCLICKK_HAS_SQLITE
  const char* home = std::getenv("HOME");
  const std::string baseDir = home != nullptr ? std::string(home) + "/.local/share/copyclickk" : ".";
  std::filesystem::create_directories(baseDir);
  repository = std::make_shared<copyclickk::SqliteHistoryRepository>(baseDir + "/history.db");
#else
  repository = std::make_shared<copyclickk::InMemoryHistoryRepository>();
#endif

  copyclickk::TrayAppController controller(repository);
  controller.start();

  return app.exec();
}
