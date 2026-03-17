#pragma once

#include <vector>

#include "IHistoryRepository.h"

namespace copyclickk {

class InMemoryHistoryRepository final : public IHistoryRepository {
 public:
  ClipboardItem add(ClipboardItem item) override;
  std::vector<ClipboardItem> list(std::size_t limit) const override;
  std::optional<ClipboardItem> findById(std::int64_t id) const override;
  bool setPinned(std::int64_t id, bool pinned) override;
  bool remove(std::int64_t id) override;
  void clear() override;
  void clearUnpinned() override;
  void trimToLimit(std::size_t limit) override;
  void removeOlderThan(std::int64_t cutoffTimestampMs) override;

 private:
  std::int64_t nextId_{1};
  std::vector<ClipboardItem> items_;
};

}  // namespace copyclickk
