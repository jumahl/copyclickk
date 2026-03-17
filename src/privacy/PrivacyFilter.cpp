#include "PrivacyFilter.h"

#include <algorithm>

namespace copyclickk {

PrivacyFilter::PrivacyFilter(PrivacyRuleSet rules) : rules_(std::move(rules)) {
  compiledPatterns_.reserve(rules_.sensitiveTextPatterns.size());
  for (const auto& pattern : rules_.sensitiveTextPatterns) {
    std::regex_constants::syntax_option_type options = std::regex::ECMAScript;
    std::string normalized = pattern;
    if (normalized.rfind("(?i)", 0) == 0) {
      normalized = normalized.substr(4);
      options |= std::regex::icase;
    }
    try {
      compiledPatterns_.emplace_back(normalized, options);
    } catch (const std::regex_error&) {
      // Ignore malformed regex rules to keep clipboard capture service available.
    }
  }
}

PrivacyDecision PrivacyFilter::evaluate(const ClipboardItem& item) const {
  if (isBlockedApp(item.sourceApp)) {
    return PrivacyDecision::RejectBlockedApp;
  }

  if (containsSensitiveText(item)) {
    return PrivacyDecision::RejectSensitivePattern;
  }

  return PrivacyDecision::Allow;
}

bool PrivacyFilter::isBlockedApp(const std::string& sourceApp) const {
  return std::find(rules_.blockedApps.begin(), rules_.blockedApps.end(), sourceApp) != rules_.blockedApps.end();
}

bool PrivacyFilter::containsSensitiveText(const ClipboardItem& item) const {
  if (!isTextMime(item.mimeType)) {
    return false;
  }

  const std::string text(item.payload.begin(), item.payload.end());
  for (const auto& regexPattern : compiledPatterns_) {
    if (std::regex_search(text, regexPattern)) {
      return true;
    }
  }

  return false;
}

bool PrivacyFilter::isTextMime(ClipboardMimeType mimeType) {
  return mimeType == ClipboardMimeType::TextPlain || mimeType == ClipboardMimeType::TextHtml ||
         mimeType == ClipboardMimeType::TextUriList;
}

}  // namespace copyclickk
