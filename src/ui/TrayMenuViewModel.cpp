#include "TrayMenuViewModel.h"

#include <vector>

namespace copyclickk {

TrayMenuViewModel::TrayMenuViewModel(ClipboardService& service, SettingsModel& settings)
    : service_(service), settings_(settings) {}

std::string TrayMenuViewModel::appNameLabel() const {
  return "CopyClickk";
}

std::string TrayMenuViewModel::showHistoryLabel() const {
  return "Show History";
}

std::string TrayMenuViewModel::clearClipboardLabel() const {
  return "Clear Clipboard";
}

std::string TrayMenuViewModel::settingsLabel() const {
  return "Settings";
}

std::string TrayMenuViewModel::openHistoryShortcutLabel() const {
  return "Open History Shortcut";
}

void TrayMenuViewModel::onTrayIconClicked() {
  historyVisible_ = true;
}

void TrayMenuViewModel::onShowHistoryClicked() {
  historyVisible_ = true;
}

void TrayMenuViewModel::onClearClipboardClicked() {
  service_.clearUnpinnedHistory();
}

void TrayMenuViewModel::onSettingsClicked() {
  settingsVisible_ = true;
}

bool TrayMenuViewModel::onPinItemClicked(std::int64_t id, bool pinned) {
  return service_.pinItem(id, pinned);
}

bool TrayMenuViewModel::onDeleteItemClicked(std::int64_t id) {
  return service_.deleteItem(id);
}

void TrayMenuViewModel::onHistoryClosed() {
  historyVisible_ = false;
}

void TrayMenuViewModel::onSettingsClosed() {
  settingsVisible_ = false;
}

void TrayMenuViewModel::updateHistoryLimit(int limit) {
  settings_.setHistoryLimit(limit);
  service_.trimToLimit(static_cast<std::size_t>(settings_.historyLimit()));
}

void TrayMenuViewModel::updateOpenHistoryShortcut(const std::string& shortcut) {
  settings_.setOpenHistoryShortcut(shortcut);
}

bool TrayMenuViewModel::isHistoryVisible() const {
  return historyVisible_;
}

bool TrayMenuViewModel::isSettingsVisible() const {
  return settingsVisible_;
}

std::size_t TrayMenuViewModel::historySize() const {
  return service_.listRecent(static_cast<std::size_t>(settings_.historyLimit())).size();
}

std::vector<ClipboardItem> TrayMenuViewModel::recentItems() const {
  return service_.listRecent(static_cast<std::size_t>(settings_.historyLimit()));
}

std::string TrayMenuViewModel::openHistoryShortcut() const {
  return settings_.openHistoryShortcut();
}

void TrayMenuViewModel::debugSeedText(const std::string& text, std::int64_t timestampMs) {
  ClipboardItem item;
  item.timestampMs = timestampMs;
  item.mimeType = ClipboardMimeType::TextPlain;
  item.sourceApp = "debug";
  item.payload = std::vector<std::uint8_t>(text.begin(), text.end());
  (void)service_.ingest(item);
}

}  // namespace copyclickk
