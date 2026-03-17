#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../core/ClipboardItem.h"

namespace copyclickk {

class IHistoryRepository {
 public:
  virtual ~IHistoryRepository() = default;

  virtual ClipboardItem add(ClipboardItem item) = 0;
  virtual std::vector<ClipboardItem> list(std::size_t limit) const = 0;
  virtual std::optional<ClipboardItem> findById(std::int64_t id) const = 0;
  virtual bool setPinned(std::int64_t id, bool pinned) = 0;
  virtual bool remove(std::int64_t id) = 0;
  virtual void clear() = 0;
  virtual void trimToLimit(std::size_t limit) = 0;
};

}  // namespace copyclickk
