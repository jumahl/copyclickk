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

  [[nodiscard]] int retentionDays() const;
  void setRetentionDays(int days);

  [[nodiscard]] bool skipConsecutiveDuplicates() const;
  void setSkipConsecutiveDuplicates(bool enabled);

  [[nodiscard]] bool pasteAsPlainText() const;
  void setPasteAsPlainText(bool enabled);

  [[nodiscard]] bool saveImages() const;
  void setSaveImages(bool enabled);

  [[nodiscard]] bool compressImages() const;
  void setCompressImages(bool enabled);

  [[nodiscard]] int imageCompressionQuality() const;
  void setImageCompressionQuality(int quality);

  [[nodiscard]] bool startAtLogin() const;
  void setStartAtLogin(bool enabled);

  [[nodiscard]] bool confirmBeforeClearAll() const;
  void setConfirmBeforeClearAll(bool enabled);

  [[nodiscard]] const std::string& theme() const;
  void setTheme(std::string themeName);

  [[nodiscard]] bool loadFromFile(const std::string& filePath);
  [[nodiscard]] bool saveToFile(const std::string& filePath) const;

 private:
  int historyLimit_{30};
  int thumbnailSizePx_{48};
  bool startInSystemTray_{true};
  std::string language_{"en"};
  std::string openHistoryShortcut_{"Ctrl+Alt+V"};
  int retentionDays_{0};
  bool skipConsecutiveDuplicates_{true};
  bool pasteAsPlainText_{false};
  bool saveImages_{true};
  bool compressImages_{false};
  int imageCompressionQuality_{85};
  bool startAtLogin_{false};
  bool confirmBeforeClearAll_{true};
  std::string theme_{"system"};
};

}  // namespace copyclickk
