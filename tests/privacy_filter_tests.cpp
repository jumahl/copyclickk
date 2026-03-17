#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "../src/core/ClipboardItem.h"
#include "../src/privacy/PrivacyFilter.h"

using copyclickk::ClipboardItem;
using copyclickk::ClipboardMimeType;
using copyclickk::PrivacyDecision;
using copyclickk::PrivacyFilter;
using copyclickk::PrivacyRuleSet;

namespace {

bool expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

ClipboardItem makeTextItem(const std::string& text, const std::string& sourceApp) {
  ClipboardItem item;
  item.mimeType = ClipboardMimeType::TextPlain;
  item.sourceApp = sourceApp;
  item.payload = std::vector<std::uint8_t>(text.begin(), text.end());
  return item;
}

bool testRejectsBlockedApp() {
  PrivacyRuleSet rules;
  rules.blockedApps = {"org.keepassxc.KeePassXC"};

  PrivacyFilter filter(rules);
  auto decision = filter.evaluate(makeTextItem("hello", "org.keepassxc.KeePassXC"));

  return expect(decision == PrivacyDecision::RejectBlockedApp, "should reject blocked source app");
}

bool testRejectsSensitivePattern() {
  PrivacyRuleSet rules;
  rules.sensitiveTextPatterns = {"(?i)api[_-]?key\\s*[:=]\\s*[a-z0-9]{8,}"};

  PrivacyFilter filter(rules);
  auto decision = filter.evaluate(makeTextItem("api_key=abcd1234", "org.kde.kate"));

  return expect(decision == PrivacyDecision::RejectSensitivePattern, "should reject text that matches sensitive pattern");
}

bool testAllowsSafeText() {
  PrivacyRuleSet rules;
  rules.blockedApps = {"org.keepassxc.KeePassXC"};
  rules.sensitiveTextPatterns = {"(?i)password\\s*[:=]"};

  PrivacyFilter filter(rules);
  auto decision = filter.evaluate(makeTextItem("nota publica", "org.kde.kate"));

  return expect(decision == PrivacyDecision::Allow, "should allow text that is not sensitive and not blocked");
}

bool testImageBypassesTextPatterns() {
  PrivacyRuleSet rules;
  rules.sensitiveTextPatterns = {"secret"};

  PrivacyFilter filter(rules);
  ClipboardItem imageItem;
  imageItem.mimeType = ClipboardMimeType::ImagePng;
  imageItem.sourceApp = "org.kde.gwenview";
  imageItem.payload = {0x01, 0x02, 0x03};

  auto decision = filter.evaluate(imageItem);
  return expect(decision == PrivacyDecision::Allow, "non-text mime types should bypass text regex checks");
}

bool testIgnoresInvalidRegexRules() {
  PrivacyRuleSet rules;
  rules.sensitiveTextPatterns = {"("};

  PrivacyFilter filter(rules);
  auto decision = filter.evaluate(makeTextItem("safe text", "org.kde.kate"));
  return expect(decision == PrivacyDecision::Allow, "invalid regex rules should be ignored safely");
}

}  // namespace

int main() {
  bool ok = true;
  ok = testRejectsBlockedApp() && ok;
  ok = testRejectsSensitivePattern() && ok;
  ok = testAllowsSafeText() && ok;
  ok = testImageBypassesTextPatterns() && ok;
  ok = testIgnoresInvalidRegexRules() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
