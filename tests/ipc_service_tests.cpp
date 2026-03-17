#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
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

bool testClearUnpinnedHistoryPreservesPinnedItems() {
  auto repo = std::make_shared<InMemoryHistoryRepository>();
  PrivacyRuleSet rules;

  ClipboardService service(repo, rules);
  bool ok = true;
  ok = expect(service.ingest(makeTextItem("keep", "org.kde.kate", 1)), "ingest pinned candidate should succeed") && ok;
  ok = expect(service.ingest(makeTextItem("drop", "org.kde.kate", 2)), "ingest unpinned candidate should succeed") && ok;

  auto listed = service.listRecent(10);
  ok = expect(listed.size() == 2, "expected two items before clearUnpinnedHistory") && ok;
  ok = expect(service.pinItem(listed.back().id, true), "pin operation should succeed") && ok;

  service.clearUnpinnedHistory();
  listed = service.listRecent(10);
  ok = expect(listed.size() == 1, "clearUnpinnedHistory should keep only pinned entries") && ok;
  ok = expect(listed.front().pinned, "remaining item should be pinned") && ok;
  return ok;
}

bool testConstructorRejectsNullRepository() {
  PrivacyRuleSet rules;
  try {
    ClipboardService service(nullptr, rules);
    (void)service;
  } catch (const std::invalid_argument&) {
    return true;
  } catch (...) {
    return expect(false, "constructor should throw std::invalid_argument for null repository");
  }

  return expect(false, "constructor should reject null repository");
}

bool testRemoveOlderThanPreservesPinnedEntries() {
  auto repo = std::make_shared<InMemoryHistoryRepository>();
  PrivacyRuleSet rules;

  ClipboardService service(repo, rules);
  bool ok = true;
  ok = expect(service.ingest(makeTextItem("keep pinned", "org.kde.kate", 1000)), "first ingest should succeed") && ok;
  ok = expect(service.ingest(makeTextItem("drop old", "org.kde.kate", 2000)), "second ingest should succeed") && ok;
  ok = expect(service.ingest(makeTextItem("keep new", "org.kde.kate", 4000)), "third ingest should succeed") && ok;

  auto listed = service.listRecent(10);
  ok = expect(listed.size() == 3, "setup should have 3 entries") && ok;
  const auto oldest = listed.back().id;
  ok = expect(service.pinItem(oldest, true), "pin oldest item should succeed") && ok;

  service.removeOlderThan(3000);
  listed = service.listRecent(10);
  ok = expect(listed.size() == 2, "retention cleanup should remove only old unpinned entries") && ok;
  ok = expect(listed.front().pinned || listed.back().pinned, "pinned old entry should be preserved") && ok;
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = testRejectsSensitivePayloadAtServiceLayer() && ok;
  ok = testStoresAndPinsItem() && ok;
  ok = testDeleteAndClearHistory() && ok;
  ok = testClearUnpinnedHistoryPreservesPinnedItems() && ok;
  ok = testConstructorRejectsNullRepository() && ok;
  ok = testRemoveOlderThanPreservesPinnedEntries() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
