#include "SettingsModel.h"

#include <algorithm>
#include <fstream>
#include <sstream>

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
  return true;
}

}  // namespace copyclickk
