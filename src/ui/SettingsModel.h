#pragma once

#include <string>

namespace copyclickk {

class SettingsModel {
 public:
  SettingsModel();

  [[nodiscard]] int historyLimit() const;
  void setHistoryLimit(int limit);

  [[nodiscard]] int thumbnailSizePx() const;
  void setThumbnailSizePx(int sizePx);

  [[nodiscard]] bool startInSystemTray() const;
  void setStartInSystemTray(bool enabled);

  [[nodiscard]] const std::string& language() const;
  void setLanguage(std::string languageCode);

  [[nodiscard]] const std::string& openHistoryShortcut() const;
  void setOpenHistoryShortcut(std::string shortcut);

  [[nodiscard]] bool loadFromFile(const std::string& filePath);
  [[nodiscard]] bool saveToFile(const std::string& filePath) const;

 private:
  int historyLimit_{30};
  int thumbnailSizePx_{48};
  bool startInSystemTray_{true};
  std::string language_{"en"};
  std::string openHistoryShortcut_{"Ctrl+Alt+V"};
};

}  // namespace copyclickk
