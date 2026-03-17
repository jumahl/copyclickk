#pragma once

#include <functional>
#include <memory>
#include <string>

#include <QDialog>
#include <QKeySequenceEdit>
#include <QListWidget>
#include <QMenu>
#include <QObject>
#include <QPointer>
#include <QShortcut>
#include <QSpinBox>
#include <QSystemTrayIcon>
#include <QWidget>

#include "../ipc/ClipboardService.h"
#include "../ui/SettingsModel.h"
#include "../ui/TrayMenuViewModel.h"

class QMimeData;
class QListWidgetItem;

namespace copyclickk {

class TrayAppController : public QObject {
 public:
  TrayAppController(std::shared_ptr<IHistoryRepository> repository, QObject* parent = nullptr);
  void start();
  void showHistoryFromExternalTrigger();

 private:
  void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
  void onShowHistoryTriggered();
  void onClearClipboardTriggered();
  void onSettingsTriggered();
  void onToggleHistoryShortcutTriggered();
  void onClipboardChanged();
  void onHistoryItemActivated(QListWidgetItem* item);

  void createTray();
  void ensureHistoryWindow();
  void ensureSettingsDialog();
  void applyItemToClipboard(std::int64_t itemId);
  [[nodiscard]] QMimeData* mimeDataFromItem(const ClipboardItem& item) const;
  void populateHistoryList(QListWidget* listWidget);
  void refreshHistoryList();
  void registerOpenHistoryShortcut();
  void loadSettings();
  void saveSettings() const;
  bool buildClipboardItemFromMimeData(const QMimeData* mimeData, ClipboardItem* item) const;

  std::shared_ptr<IHistoryRepository> repository_;
  SettingsModel settings_;
  ClipboardService service_;
  TrayMenuViewModel trayViewModel_;

  QSystemTrayIcon trayIcon_;
  QMenu trayMenu_;
  QAction* showHistoryAction_{nullptr};
  QAction* clearClipboardAction_{nullptr};
  QAction* settingsAction_{nullptr};
  QAction* quitAction_{nullptr};
  QAction* globalShowHistoryAction_{nullptr};

  QPointer<QWidget> historyWindow_;
  QPointer<QListWidget> historyList_;
  QPointer<QDialog> settingsDialog_;
  QPointer<QSpinBox> historyLimitSpinBox_;
  QPointer<QSpinBox> thumbnailSizeSpinBox_;
  QPointer<QKeySequenceEdit> openHistoryShortcutEdit_;
  QPointer<QWidget> shortcutHost_;
  QPointer<QShortcut> openHistoryShortcut_;
  bool suppressClipboardCapture_{false};
  std::string settingsFilePath_;
};

}  // namespace copyclickk
