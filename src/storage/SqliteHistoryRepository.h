#pragma once

#include <string>

#include <sqlite3.h>

#include "IHistoryRepository.h"

namespace copyclickk {

class SqliteHistoryRepository final : public IHistoryRepository {
 public:
  explicit SqliteHistoryRepository(std::string dbPath);
  ~SqliteHistoryRepository() override;

  SqliteHistoryRepository(const SqliteHistoryRepository&) = delete;
  SqliteHistoryRepository& operator=(const SqliteHistoryRepository&) = delete;

  ClipboardItem add(ClipboardItem item) override;
  std::vector<ClipboardItem> list(std::size_t limit) const override;
  std::optional<ClipboardItem> findById(std::int64_t id) const override;
  bool setPinned(std::int64_t id, bool pinned) override;
  bool remove(std::int64_t id) override;
  void clear() override;
  void clearUnpinned() override;
  void trimToLimit(std::size_t limit) override;

 private:
  void initializeSchema();

  sqlite3* db_{nullptr};
};

}  // namespace copyclickk
