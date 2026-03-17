#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
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

}  // namespace

int main() {
  bool ok = true;
  ok = testSqliteDeduplicatesByMimeAndPayload() && ok;
  ok = testSqlitePinnedItemsFirst() && ok;
  ok = testSqlitePersistsAcrossInstances() && ok;
  ok = testSqliteTrimToLimitRemovesOldest() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
