#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

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

  bool ok = expect(settings.saveToFile(path.string()), "saveToFile should succeed") && true;

  SettingsModel loaded;
  ok = expect(loaded.loadFromFile(path.string()), "loadFromFile should succeed") && ok;
  ok = expect(loaded.historyLimit() == 30, "history limit should persist") && ok;
  ok = expect(loaded.thumbnailSizePx() == 64, "thumbnail size should persist") && ok;
  ok = expect(loaded.openHistoryShortcut() == "Ctrl+Shift+H", "shortcut should persist") && ok;

  std::filesystem::remove(path);
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = testDefaultEnglishLabels() && ok;
  ok = testTrayClickOpensHistory() && ok;
  ok = testClearClipboardActionClearsHistory() && ok;
  ok = testSettingsAdjustHistoryLimit() && ok;
  ok = testSettingsPersistenceRoundTrip() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
