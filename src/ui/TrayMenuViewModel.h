#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../core/ClipboardItem.h"
#include "../ipc/ClipboardService.h"
#include "SettingsModel.h"

namespace copyclickk {

class TrayMenuViewModel {
 public:
  TrayMenuViewModel(ClipboardService& service, SettingsModel& settings);

  [[nodiscard]] std::string appNameLabel() const;
  [[nodiscard]] std::string showHistoryLabel() const;
  [[nodiscard]] std::string clearClipboardLabel() const;
  [[nodiscard]] std::string settingsLabel() const;
  [[nodiscard]] std::string openHistoryShortcutLabel() const;

  void onTrayIconClicked();
  void onShowHistoryClicked();
  void onClearClipboardClicked();
  void onSettingsClicked();
  void onHistoryClosed();
  void onSettingsClosed();
  void updateHistoryLimit(int limit);
  void updateOpenHistoryShortcut(const std::string& shortcut);

  [[nodiscard]] bool isHistoryVisible() const;
  [[nodiscard]] bool isSettingsVisible() const;
  [[nodiscard]] std::size_t historySize() const;
  [[nodiscard]] std::vector<ClipboardItem> recentItems() const;
  [[nodiscard]] std::string openHistoryShortcut() const;

  void debugSeedText(const std::string& text, std::int64_t timestampMs);

 private:
  ClipboardService& service_;
  SettingsModel& settings_;
  bool historyVisible_{false};
  bool settingsVisible_{false};
};

}  // namespace copyclickk
