#include "SettingsModel.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace copyclickk {

SettingsModel::SettingsModel() = default;

int SettingsModel::historyLimit() const {
  return historyLimit_;
}

void SettingsModel::setHistoryLimit(int limit) {
  historyLimit_ = std::clamp(limit, 1, 5000);
}

int SettingsModel::thumbnailSizePx() const {
  return thumbnailSizePx_;
}

void SettingsModel::setThumbnailSizePx(int sizePx) {
  thumbnailSizePx_ = std::clamp(sizePx, 16, 256);
}

bool SettingsModel::startInSystemTray() const {
  return startInSystemTray_;
}

void SettingsModel::setStartInSystemTray(bool enabled) {
  startInSystemTray_ = enabled;
}

const std::string& SettingsModel::language() const {
  return language_;
}

void SettingsModel::setLanguage(std::string languageCode) {
  if (languageCode.empty()) {
    return;
  }
  language_ = std::move(languageCode);
}

const std::string& SettingsModel::openHistoryShortcut() const {
  return openHistoryShortcut_;
}

void SettingsModel::setOpenHistoryShortcut(std::string shortcut) {
  if (shortcut.empty()) {
    return;
  }
  openHistoryShortcut_ = std::move(shortcut);
}

int SettingsModel::retentionDays() const {
  return retentionDays_;
}

void SettingsModel::setRetentionDays(int days) {
  retentionDays_ = std::clamp(days, 0, 3650);
}

bool SettingsModel::skipConsecutiveDuplicates() const {
  return skipConsecutiveDuplicates_;
}

void SettingsModel::setSkipConsecutiveDuplicates(bool enabled) {
  skipConsecutiveDuplicates_ = enabled;
}

bool SettingsModel::pasteAsPlainText() const {
  return pasteAsPlainText_;
}

void SettingsModel::setPasteAsPlainText(bool enabled) {
  pasteAsPlainText_ = enabled;
}

bool SettingsModel::saveImages() const {
  return saveImages_;
}

void SettingsModel::setSaveImages(bool enabled) {
  saveImages_ = enabled;
}

bool SettingsModel::compressImages() const {
  return compressImages_;
}

void SettingsModel::setCompressImages(bool enabled) {
  compressImages_ = enabled;
}

int SettingsModel::imageCompressionQuality() const {
  return imageCompressionQuality_;
}

void SettingsModel::setImageCompressionQuality(int quality) {
  imageCompressionQuality_ = std::clamp(quality, 30, 100);
}

bool SettingsModel::startAtLogin() const {
  return startAtLogin_;
}

void SettingsModel::setStartAtLogin(bool enabled) {
  startAtLogin_ = enabled;
}

bool SettingsModel::confirmBeforeClearAll() const {
  return confirmBeforeClearAll_;
}

void SettingsModel::setConfirmBeforeClearAll(bool enabled) {
  confirmBeforeClearAll_ = enabled;
}

const std::string& SettingsModel::theme() const {
  return theme_;
}

void SettingsModel::setTheme(std::string themeName) {
  if (themeName == "system" || themeName == "light" || themeName == "dark") {
    theme_ = std::move(themeName);
  }
}

bool SettingsModel::loadFromFile(const std::string& filePath) {
  std::ifstream input(filePath);
  if (!input.is_open()) {
    return false;
  }

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }

    const auto separator = line.find('=');
    if (separator == std::string::npos) {
      continue;
    }

    const std::string key = line.substr(0, separator);
    const std::string value = line.substr(separator + 1);

    if (key == "history_limit") {
      try {
        setHistoryLimit(std::stoi(value));
      } catch (...) {
      }
    } else if (key == "thumbnail_size_px") {
      try {
        setThumbnailSizePx(std::stoi(value));
      } catch (...) {
      }
    } else if (key == "start_in_system_tray") {
      setStartInSystemTray(value == "1" || value == "true");
    } else if (key == "language") {
      setLanguage(value);
    } else if (key == "open_history_shortcut") {
      setOpenHistoryShortcut(value);
    } else if (key == "retention_days") {
      try {
        setRetentionDays(std::stoi(value));
      } catch (...) {
      }
    } else if (key == "skip_consecutive_duplicates") {
      setSkipConsecutiveDuplicates(value == "1" || value == "true");
    } else if (key == "paste_as_plain_text") {
      setPasteAsPlainText(value == "1" || value == "true");
    } else if (key == "save_images") {
      setSaveImages(value == "1" || value == "true");
    } else if (key == "compress_images") {
      setCompressImages(value == "1" || value == "true");
    } else if (key == "image_compression_quality") {
      try {
        setImageCompressionQuality(std::stoi(value));
      } catch (...) {
      }
    } else if (key == "start_at_login") {
      setStartAtLogin(value == "1" || value == "true");
    } else if (key == "confirm_before_clear_all") {
      setConfirmBeforeClearAll(value == "1" || value == "true");
    } else if (key == "theme") {
      setTheme(value);
    }
  }

  return true;
}

bool SettingsModel::saveToFile(const std::string& filePath) const {
  std::ofstream output(filePath, std::ios::trunc);
  if (!output.is_open()) {
    return false;
  }

  output << "history_limit=" << historyLimit_ << "\n";
  output << "thumbnail_size_px=" << thumbnailSizePx_ << "\n";
  output << "start_in_system_tray=" << (startInSystemTray_ ? "1" : "0") << "\n";
  output << "language=" << language_ << "\n";
  output << "open_history_shortcut=" << openHistoryShortcut_ << "\n";
  output << "retention_days=" << retentionDays_ << "\n";
  output << "skip_consecutive_duplicates=" << (skipConsecutiveDuplicates_ ? "1" : "0") << "\n";
  output << "paste_as_plain_text=" << (pasteAsPlainText_ ? "1" : "0") << "\n";
  output << "save_images=" << (saveImages_ ? "1" : "0") << "\n";
  output << "compress_images=" << (compressImages_ ? "1" : "0") << "\n";
  output << "image_compression_quality=" << imageCompressionQuality_ << "\n";
  output << "start_at_login=" << (startAtLogin_ ? "1" : "0") << "\n";
  output << "confirm_before_clear_all=" << (confirmBeforeClearAll_ ? "1" : "0") << "\n";
  output << "theme=" << theme_ << "\n";

  output.flush();
  if (!output.good()) {
    return false;
  }

  std::error_code ec;
  std::filesystem::permissions(filePath,
                               std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                               std::filesystem::perm_options::replace,
                               ec);
  return !ec;
}

}  // namespace copyclickk
