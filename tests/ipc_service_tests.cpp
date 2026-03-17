#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../src/ipc/ClipboardService.h"
#include "../src/storage/InMemoryHistoryRepository.h"

using copyclickk::ClipboardItem;
using copyclickk::ClipboardMimeType;
using copyclickk::ClipboardService;
using copyclickk::InMemoryHistoryRepository;
using copyclickk::PrivacyRuleSet;

namespace {

bool expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

ClipboardItem makeTextItem(const std::string& text, const std::string& sourceApp, std::int64_t ts) {
  ClipboardItem item;
  item.mimeType = ClipboardMimeType::TextPlain;
  item.sourceApp = sourceApp;
  item.timestampMs = ts;
  item.payload = std::vector<std::uint8_t>(text.begin(), text.end());
  return item;
}

bool testRejectsSensitivePayloadAtServiceLayer() {
  auto repo = std::make_shared<InMemoryHistoryRepository>();
  PrivacyRuleSet rules;
  rules.sensitiveTextPatterns = {"token\\s*[:=]"};

  ClipboardService service(repo, rules);
  bool stored = service.ingest(makeTextItem("token=abc", "org.kde.kate", 1000));

  bool ok = true;
  ok = expect(!stored, "service should reject sensitive clipboard item") && ok;
  ok = expect(service.listRecent(10).empty(), "rejected item should not be persisted") && ok;
  return ok;
}

bool testStoresAndPinsItem() {
  auto repo = std::make_shared<InMemoryHistoryRepository>();
  PrivacyRuleSet rules;

  ClipboardService service(repo, rules);
  bool stored = service.ingest(makeTextItem("hola", "org.kde.kate", 1000));
  bool ok = expect(stored, "safe item should be stored");

  auto listed = service.listRecent(10);
  ok = expect(listed.size() == 1, "expected one stored item") && ok;
  ok = expect(service.pinItem(listed.front().id, true), "pin operation should succeed") && ok;
  auto fetched = service.getItem(listed.front().id);
  ok = expect(fetched.has_value() && fetched->pinned, "pinned flag should persist") && ok;
  return ok;
}

bool testDeleteAndClearHistory() {
  auto repo = std::make_shared<InMemoryHistoryRepository>();
  PrivacyRuleSet rules;

  ClipboardService service(repo, rules);
  bool ok = true;
  ok = expect(service.ingest(makeTextItem("a", "org.kde.kate", 1)), "first ingest should succeed") && ok;
  ok = expect(service.ingest(makeTextItem("b", "org.kde.kate", 2)), "second ingest should succeed") && ok;

  auto listed = service.listRecent(10);
  ok = expect(listed.size() == 2, "expected two items before delete") && ok;
  ok = expect(service.deleteItem(listed.front().id), "delete should remove selected item") && ok;
  ok = expect(service.listRecent(10).size() == 1, "expected one item after delete") && ok;

  service.clearHistory();
  ok = expect(service.listRecent(10).empty(), "clear should remove all items") && ok;
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = testRejectsSensitivePayloadAtServiceLayer() && ok;
  ok = testStoresAndPinsItem() && ok;
  ok = testDeleteAndClearHistory() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
