#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "../src/core/ClipboardItem.h"
#include "../src/storage/InMemoryHistoryRepository.h"

using copyclickk::ClipboardItem;
using copyclickk::ClipboardMimeType;
using copyclickk::InMemoryHistoryRepository;

namespace {

ClipboardItem makeTextItem(std::string text, std::int64_t timestampMs) {
  ClipboardItem item;
  item.timestampMs = timestampMs;
  item.mimeType = ClipboardMimeType::TextPlain;
  item.payload = std::vector<std::uint8_t>(text.begin(), text.end());
  item.sourceApp = "test";
  return item;
}

bool expect(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    return false;
  }
  return true;
}

bool testDeduplicatesIdenticalPayloadAndMime() {
  InMemoryHistoryRepository repo;

  auto first = repo.add(makeTextItem("hello", 1000));
  auto second = repo.add(makeTextItem("hello", 2000));

  auto listed = repo.list(10);
  bool ok = true;
  ok = expect(first.id == second.id, "duplicate add should return the existing item id") && ok;
  ok = expect(listed.size() == 1, "duplicate add should keep a single entry in history") && ok;
  return ok;
}

bool testAllowsNonConsecutiveDuplicateEntries() {
  InMemoryHistoryRepository repo;

  auto first = repo.add(makeTextItem("hello", 1000));
  (void)repo.add(makeTextItem("other", 1500));
  auto third = repo.add(makeTextItem("hello", 2000));

  auto listed = repo.list(10);
  bool ok = true;
  ok = expect(first.id != third.id, "non-consecutive duplicates should create a new entry") && ok;
  ok = expect(listed.size() == 3, "history should keep non-consecutive duplicates") && ok;
  return ok;
}

bool testPinnedItemsAppearBeforeNonPinned() {
  InMemoryHistoryRepository repo;

  auto older = repo.add(makeTextItem("older", 1000));
  auto newer = repo.add(makeTextItem("newer", 2000));
  (void)newer;

  bool ok = repo.setPinned(older.id, true);
  ok = expect(ok, "setPinned should succeed for existing item") && ok;

  auto listed = repo.list(10);
  ok = expect(listed.size() == 2, "expected exactly 2 history items") && ok;
  ok = expect(listed.front().id == older.id, "pinned item must be listed first") && ok;
  return ok;
}

bool testTrimToLimitRemovesOldestEntries() {
  InMemoryHistoryRepository repo;
  repo.add(makeTextItem("first", 1000));
  repo.add(makeTextItem("second", 2000));
  repo.add(makeTextItem("third", 3000));

  repo.trimToLimit(2);
  auto listed = repo.list(10);

  bool ok = true;
  ok = expect(listed.size() == 2, "trim should keep only two entries") && ok;
  ok = expect(std::string(listed[0].payload.begin(), listed[0].payload.end()) == "third",
              "newest entry should be preserved") && ok;
  ok = expect(std::string(listed[1].payload.begin(), listed[1].payload.end()) == "second",
              "second newest entry should be preserved") && ok;
  return ok;
}

bool testTrimToLimitNeverDeletesPinnedItems() {
  InMemoryHistoryRepository repo;
  auto pinned = repo.add(makeTextItem("pinned", 1000));
  repo.add(makeTextItem("second", 2000));
  repo.add(makeTextItem("third", 3000));
  (void)repo.setPinned(pinned.id, true);

  repo.trimToLimit(1);
  const auto listed = repo.list(10);

  bool ok = true;
  ok = expect(listed.size() == 1, "trim should keep pinned item even at low limits") && ok;
  ok = expect(listed.front().pinned, "remaining item should be pinned") && ok;
  return ok;
}

bool testClearUnpinnedPreservesPinnedItems() {
  InMemoryHistoryRepository repo;
  auto pinned = repo.add(makeTextItem("keep", 1000));
  auto regular = repo.add(makeTextItem("remove", 2000));
  (void)regular;
  (void)repo.setPinned(pinned.id, true);

  repo.clearUnpinned();
  const auto listed = repo.list(10);

  bool ok = true;
  ok = expect(listed.size() == 1, "clearUnpinned should remove only non-pinned entries") && ok;
  ok = expect(listed.front().id == pinned.id, "pinned entry should remain after clearUnpinned") && ok;
  return ok;
}

bool testRemoveOlderThanRemovesOnlyOldUnpinned() {
  InMemoryHistoryRepository repo;
  auto oldPinned = repo.add(makeTextItem("keep pinned", 1000));
  (void)repo.setPinned(oldPinned.id, true);
  (void)repo.add(makeTextItem("drop old", 2000));
  (void)repo.add(makeTextItem("keep new", 4000));

  repo.removeOlderThan(3000);
  const auto listed = repo.list(10);

  bool ok = true;
  ok = expect(listed.size() == 2, "removeOlderThan should remove old unpinned entries") && ok;
  ok = expect(std::string(listed[0].payload.begin(), listed[0].payload.end()) == "keep pinned" ||
                  std::string(listed[1].payload.begin(), listed[1].payload.end()) == "keep pinned",
              "old pinned entry should remain") && ok;
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = testDeduplicatesIdenticalPayloadAndMime() && ok;
  ok = testAllowsNonConsecutiveDuplicateEntries() && ok;
  ok = testPinnedItemsAppearBeforeNonPinned() && ok;
  ok = testTrimToLimitRemovesOldestEntries() && ok;
  ok = testTrimToLimitNeverDeletesPinnedItems() && ok;
  ok = testClearUnpinnedPreservesPinnedItems() && ok;
  ok = testRemoveOlderThanRemovesOnlyOldUnpinned() && ok;

  if (!ok) {
    return EXIT_FAILURE;
  }

  std::cout << "All tests passed\n";
  return EXIT_SUCCESS;
}
