#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>

#include "../src/ipc/ClipboardService.h"
#include "../src/storage/InMemoryHistoryRepository.h"
#include "../src/ui/SettingsModel.h"
#include "../src/ui/TrayMenuViewModel.h"

using copyclickk::ClipboardService;
using copyclickk::InMemoryHistoryRepository;
using copyclickk::PrivacyRuleSet;
using copyclickk::SettingsModel;
using copyclickk::TrayMenuViewModel;

namespace {

bool expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

bool hasNoGroupOrOthersPermissions(std::filesystem::perms permissions) {
  const auto publicBits = std::filesystem::perms::group_all | std::filesystem::perms::others_all;
  return (permissions & publicBits) == std::filesystem::perms::none;
}

std::string permissionsToOctal(std::filesystem::perms perms) {
  auto bit = [perms](std::filesystem::perms flag) {
    return (perms & flag) != std::filesystem::perms::none ? 1 : 0;
  };

  const int owner = bit(std::filesystem::perms::owner_read) * 4 +
                    bit(std::filesystem::perms::owner_write) * 2 +
                    bit(std::filesystem::perms::owner_exec);
  const int group = bit(std::filesystem::perms::group_read) * 4 +
                    bit(std::filesystem::perms::group_write) * 2 +
                    bit(std::filesystem::perms::group_exec);
  const int others = bit(std::filesystem::perms::others_read) * 4 +
                     bit(std::filesystem::perms::others_write) * 2 +
                     bit(std::filesystem::perms::others_exec);

  return std::to_string(owner) + std::to_string(group) + std::to_string(others);
}

bool testDefaultEnglishLabels() {
  auto repo = std::make_shared<InMemoryHistoryRepository>();
  ClipboardService service(repo, PrivacyRuleSet{});
  SettingsModel settings;
  TrayMenuViewModel tray(service, settings);

  bool ok = true;
  ok = expect(tray.appNameLabel() == "CopyClickk", "app label should be English") && ok;
  ok = expect(tray.showHistoryLabel() == "Show History", "history label should be English") && ok;
  ok = expect(tray.clearClipboardLabel() == "Clear Clipboard", "clear label should be English") && ok;
  ok = expect(tray.settingsLabel() == "Settings", "settings label should be English") && ok;
  ok = expect(tray.openHistoryShortcutLabel() == "Open History Shortcut", "shortcut label should be English") && ok;
  ok = expect(tray.openHistoryShortcut() == "Ctrl+Alt+V", "default shortcut should be Ctrl+Alt+V") && ok;
  return ok;
}

bool testTrayClickOpensHistory() {
  auto repo = std::make_shared<InMemoryHistoryRepository>();
  ClipboardService service(repo, PrivacyRuleSet{});
  SettingsModel settings;
  TrayMenuViewModel tray(service, settings);

  tray.onTrayIconClicked();
  return expect(tray.isHistoryVisible(), "clicking tray icon should show history");
}

bool testClearClipboardActionClearsHistory() {
  auto repo = std::make_shared<InMemoryHistoryRepository>();
  ClipboardService service(repo, PrivacyRuleSet{});
  SettingsModel settings;
  TrayMenuViewModel tray(service, settings);

  tray.debugSeedText("one", 1);
  tray.debugSeedText("two", 2);
  bool ok = expect(tray.historySize() == 2, "test setup should seed two history items");

  tray.onClearClipboardClicked();
  ok = expect(tray.historySize() == 0, "clear action should remove all history entries") && ok;
  return ok;
}

bool testClearClipboardPreservesPinnedItems() {
  auto repo = std::make_shared<InMemoryHistoryRepository>();
  ClipboardService service(repo, PrivacyRuleSet{});
  SettingsModel settings;
  TrayMenuViewModel tray(service, settings);

  tray.debugSeedText("keep", 1);
  tray.debugSeedText("drop", 2);
  auto items = tray.recentItems();

  bool ok = true;
  ok = expect(items.size() == 2, "test setup should seed two items") && ok;
  ok = expect(tray.onPinItemClicked(items.back().id, true), "pin action should succeed") && ok;

  tray.onClearClipboardClicked();
  items = tray.recentItems();
  ok = expect(items.size() == 1, "clear action should preserve pinned items") && ok;
  ok = expect(items.front().pinned, "remaining item should be pinned") && ok;
  return ok;
}

bool testPerItemPinAndDeleteActions() {
  auto repo = std::make_shared<InMemoryHistoryRepository>();
  ClipboardService service(repo, PrivacyRuleSet{});
  SettingsModel settings;
  TrayMenuViewModel tray(service, settings);

  tray.debugSeedText("first", 1);
  tray.debugSeedText("second", 2);
  auto items = tray.recentItems();
  bool ok = true;
  ok = expect(items.size() == 2, "test setup should create two items") && ok;

  const auto firstId = items.front().id;
  ok = expect(tray.onPinItemClicked(firstId, true), "pin action should update item") && ok;
  items = tray.recentItems();
  ok = expect(items.front().id == firstId && items.front().pinned, "pinned item should be listed first") && ok;

  ok = expect(tray.onDeleteItemClicked(firstId), "delete action should remove selected item") && ok;
  items = tray.recentItems();
  ok = expect(items.size() == 1, "one item should remain after delete") && ok;
  return ok;
}

bool testSettingsAdjustHistoryLimit() {
  auto repo = std::make_shared<InMemoryHistoryRepository>();
  ClipboardService service(repo, PrivacyRuleSet{});
  SettingsModel settings;
  TrayMenuViewModel tray(service, settings);

  tray.onSettingsClicked();
  bool ok = expect(tray.isSettingsVisible(), "settings action should open settings view") && ok;

  tray.updateHistoryLimit(250);
  ok = expect(settings.historyLimit() == 250, "settings should update clipboard history limit") && ok;

  tray.updateOpenHistoryShortcut("Ctrl+Shift+V");
  ok = expect(settings.openHistoryShortcut() == "Ctrl+Shift+V", "settings should update open history shortcut") && ok;
  return ok;
}

bool testSettingsPersistenceRoundTrip() {
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "copyclickk-settings-test.ini";
  std::filesystem::remove(path);

  SettingsModel settings;
  settings.setHistoryLimit(30);
  settings.setThumbnailSizePx(64);
  settings.setOpenHistoryShortcut("Ctrl+Shift+H");
  settings.setRetentionDays(30);
  settings.setSkipConsecutiveDuplicates(true);
  settings.setPasteAsPlainText(true);
  settings.setSaveImages(false);
  settings.setCompressImages(true);
  settings.setImageCompressionQuality(70);
  settings.setStartAtLogin(true);
  settings.setConfirmBeforeClearAll(false);
  settings.setTheme("dark");

  bool ok = expect(settings.saveToFile(path.string()), "saveToFile should succeed") && true;

  SettingsModel loaded;
  ok = expect(loaded.loadFromFile(path.string()), "loadFromFile should succeed") && ok;
  ok = expect(loaded.historyLimit() == 30, "history limit should persist") && ok;
  ok = expect(loaded.thumbnailSizePx() == 64, "thumbnail size should persist") && ok;
  ok = expect(loaded.openHistoryShortcut() == "Ctrl+Shift+H", "shortcut should persist") && ok;
  ok = expect(loaded.retentionDays() == 30, "retention days should persist") && ok;
  ok = expect(loaded.skipConsecutiveDuplicates(), "skip duplicates should persist") && ok;
  ok = expect(loaded.pasteAsPlainText(), "plain text paste setting should persist") && ok;
  ok = expect(!loaded.saveImages(), "save images setting should persist") && ok;
  ok = expect(loaded.compressImages(), "compress images setting should persist") && ok;
  ok = expect(loaded.imageCompressionQuality() == 70, "image compression quality should persist") && ok;
  ok = expect(loaded.startAtLogin(), "start at login setting should persist") && ok;
  ok = expect(!loaded.confirmBeforeClearAll(), "clear confirmation setting should persist") && ok;
  ok = expect(loaded.theme() == "dark", "theme setting should persist") && ok;

  std::error_code ec;
  const auto status = std::filesystem::status(path, ec);
  ok = expect(!ec, "status for settings file should be readable") && ok;
  if (!ec && std::filesystem::exists(path)) {
    const auto perms = status.permissions();
#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    if (!hasNoGroupOrOthersPermissions(perms)) {
      std::cerr << "INFO: settings permissions are " << permissionsToOctal(perms)
                << " (expected owner-only like 600)\n";
    }
    ok = expect(hasNoGroupOrOthersPermissions(perms),
                "settings file should not be readable by group/others") && ok;
#else
    if (!hasNoGroupOrOthersPermissions(perms)) {
      std::cerr << "INFO: skipping strict POSIX permission assertion on non-POSIX platform; observed perms="
                << permissionsToOctal(perms) << '\n';
    }
#endif
  }

  std::filesystem::remove(path);
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = testDefaultEnglishLabels() && ok;
  ok = testTrayClickOpensHistory() && ok;
  ok = testClearClipboardActionClearsHistory() && ok;
  ok = testClearClipboardPreservesPinnedItems() && ok;
  ok = testPerItemPinAndDeleteActions() && ok;
  ok = testSettingsAdjustHistoryLimit() && ok;
  ok = testSettingsPersistenceRoundTrip() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
