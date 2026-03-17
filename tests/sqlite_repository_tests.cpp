#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

#include "../src/core/ClipboardItem.h"
#include "../src/storage/SqliteHistoryRepository.h"

using copyclickk::ClipboardItem;
using copyclickk::ClipboardMimeType;
using copyclickk::SqliteHistoryRepository;

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

bool hasNoGroupOrOthersPermissions(std::filesystem::perms permissions) {
  const auto publicBits = std::filesystem::perms::group_all | std::filesystem::perms::others_all;
  return (permissions & publicBits) == std::filesystem::perms::none;
}

bool testSqliteDeduplicatesByMimeAndPayload() {
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "copyclickk-test-dedupe.db";
  std::filesystem::remove(path);
  SqliteHistoryRepository repo(path.string());

  auto first = repo.add(makeTextItem("hello", 1000));
  auto second = repo.add(makeTextItem("hello", 2000));
  auto listed = repo.list(10);

  bool ok = true;
  ok = expect(first.id == second.id, "sqlite duplicate add should return existing row id") && ok;
  ok = expect(listed.size() == 1, "sqlite duplicate add should keep one row") && ok;

  std::filesystem::remove(path);
  return ok;
}

bool testSqliteAllowsNonConsecutiveDuplicateEntries() {
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "copyclickk-test-non-consecutive.db";
  std::filesystem::remove(path);
  SqliteHistoryRepository repo(path.string());

  auto first = repo.add(makeTextItem("hello", 1000));
  (void)repo.add(makeTextItem("other", 1500));
  auto third = repo.add(makeTextItem("hello", 2000));
  auto listed = repo.list(10);

  bool ok = true;
  ok = expect(first.id != third.id, "sqlite should keep non-consecutive duplicates") && ok;
  ok = expect(listed.size() == 3, "sqlite should keep three rows for non-consecutive duplicate sequence") && ok;

  std::filesystem::remove(path);
  return ok;
}

bool testSqlitePinnedItemsFirst() {
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "copyclickk-test-pinned.db";
  std::filesystem::remove(path);
  SqliteHistoryRepository repo(path.string());

  auto older = repo.add(makeTextItem("older", 1000));
  auto newer = repo.add(makeTextItem("newer", 2000));
  (void)newer;

  bool ok = repo.setPinned(older.id, true);
  ok = expect(ok, "setPinned should succeed") && ok;
  auto listed = repo.list(10);
  ok = expect(!listed.empty(), "expected list to return items") && ok;
  ok = expect(listed.front().id == older.id, "pinned row should be returned first") && ok;

  std::filesystem::remove(path);
  return ok;
}

bool testSqlitePersistsAcrossInstances() {
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "copyclickk-test-persist.db";
  std::filesystem::remove(path);

  {
    SqliteHistoryRepository repo(path.string());
    auto created = repo.add(makeTextItem("persist me", 1000));
    bool ok = expect(created.id > 0, "new sqlite row should get positive id");
    if (!ok) {
      std::filesystem::remove(path);
      return false;
    }
  }

  {
    SqliteHistoryRepository repo(path.string());
    auto listed = repo.list(10);
    bool ok = true;
    ok = expect(listed.size() == 1, "row should persist after reopening repository") && ok;
    ok = expect(std::string(listed.front().payload.begin(), listed.front().payload.end()) == "persist me",
                "payload should persist correctly") && ok;
    std::filesystem::remove(path);
    return ok;
  }
}

bool testSqliteTrimToLimitRemovesOldest() {
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "copyclickk-test-trim.db";
  std::filesystem::remove(path);
  SqliteHistoryRepository repo(path.string());

  repo.add(makeTextItem("one", 1));
  repo.add(makeTextItem("two", 2));
  repo.add(makeTextItem("three", 3));
  repo.trimToLimit(2);

  auto listed = repo.list(10);
  bool ok = true;
  ok = expect(listed.size() == 2, "trim should keep two rows") && ok;
  ok = expect(std::string(listed[0].payload.begin(), listed[0].payload.end()) == "three",
              "newest row should remain") && ok;
  ok = expect(std::string(listed[1].payload.begin(), listed[1].payload.end()) == "two",
              "second newest row should remain") && ok;

  std::filesystem::remove(path);
  return ok;
}

bool testSqliteTrimToLimitKeepsPinnedItems() {
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "copyclickk-test-trim-pinned.db";
  std::filesystem::remove(path);
  SqliteHistoryRepository repo(path.string());

  auto pinned = repo.add(makeTextItem("pinned", 1));
  repo.add(makeTextItem("two", 2));
  repo.add(makeTextItem("three", 3));
  (void)repo.setPinned(pinned.id, true);
  repo.trimToLimit(1);

  auto listed = repo.list(10);
  bool ok = true;
  ok = expect(listed.size() == 1, "trim should keep pinned rows when limit is small") && ok;
  ok = expect(listed.front().id == pinned.id, "pinned row should survive trim") && ok;

  std::filesystem::remove(path);
  return ok;
}

bool testSqliteClearUnpinnedPreservesPinnedRows() {
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "copyclickk-test-clear-pinned.db";
  std::filesystem::remove(path);
  SqliteHistoryRepository repo(path.string());

  auto pinned = repo.add(makeTextItem("keep", 1));
  repo.add(makeTextItem("drop", 2));
  (void)repo.setPinned(pinned.id, true);
  repo.clearUnpinned();

  auto listed = repo.list(10);
  bool ok = true;
  ok = expect(listed.size() == 1, "clearUnpinned should remove only unpinned rows") && ok;
  ok = expect(listed.front().id == pinned.id, "pinned row should remain after clearUnpinned") && ok;

  std::filesystem::remove(path);
  return ok;
}

bool testSqliteDatabaseFileIsPrivate() {
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "copyclickk-test-perms.db";
  std::filesystem::remove(path);

  SqliteHistoryRepository repo(path.string());
  (void)repo.add(makeTextItem("hello", 1));

  std::error_code ec;
  const auto status = std::filesystem::status(path, ec);
  bool ok = true;
  ok = expect(!ec, "status for sqlite file should be readable") && ok;
  if (!ec) {
    ok = expect(hasNoGroupOrOthersPermissions(status.permissions()),
                "sqlite history file should not be readable by group/others") && ok;
  }

  std::filesystem::remove(path);
  return ok;
}

bool testSqliteRemoveOlderThanRemovesOldUnpinned() {
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "copyclickk-test-retention.db";
  std::filesystem::remove(path);
  SqliteHistoryRepository repo(path.string());

  auto pinned = repo.add(makeTextItem("keep pinned", 1000));
  (void)repo.setPinned(pinned.id, true);
  (void)repo.add(makeTextItem("drop old", 2000));
  (void)repo.add(makeTextItem("keep new", 4000));

  repo.removeOlderThan(3000);
  const auto listed = repo.list(10);

  bool ok = true;
  ok = expect(listed.size() == 2, "removeOlderThan should drop only old unpinned rows") && ok;

  std::filesystem::remove(path);
  return ok;
}

}  // namespace

int main() {
  bool ok = true;
  ok = testSqliteDeduplicatesByMimeAndPayload() && ok;
  ok = testSqliteAllowsNonConsecutiveDuplicateEntries() && ok;
  ok = testSqlitePinnedItemsFirst() && ok;
  ok = testSqlitePersistsAcrossInstances() && ok;
  ok = testSqliteTrimToLimitRemovesOldest() && ok;
  ok = testSqliteTrimToLimitKeepsPinnedItems() && ok;
  ok = testSqliteClearUnpinnedPreservesPinnedRows() && ok;
  ok = testSqliteDatabaseFileIsPrivate() && ok;
  ok = testSqliteRemoveOlderThanRemovesOldUnpinned() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
