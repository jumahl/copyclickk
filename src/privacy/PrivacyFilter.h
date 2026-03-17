#pragma once

#include <regex>
#include <string>
#include <vector>

#include "../core/ClipboardItem.h"

namespace copyclickk {

enum class PrivacyDecision {
  Allow,
  RejectBlockedApp,
  RejectSensitivePattern,
};

struct PrivacyRuleSet {
  std::vector<std::string> blockedApps;
  std::vector<std::string> sensitiveTextPatterns;
};

class PrivacyFilter {
 public:
  explicit PrivacyFilter(PrivacyRuleSet rules);

  [[nodiscard]] PrivacyDecision evaluate(const ClipboardItem& item) const;

 private:
  bool isBlockedApp(const std::string& sourceApp) const;
  bool containsSensitiveText(const ClipboardItem& item) const;
  static bool isTextMime(ClipboardMimeType mimeType);

  PrivacyRuleSet rules_;
  std::vector<std::regex> compiledPatterns_;
};

}  // namespace copyclickk
