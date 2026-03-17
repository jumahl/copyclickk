#include "InMemoryHistoryRepository.h"

#include <algorithm>

namespace copyclickk {

ClipboardItem InMemoryHistoryRepository::add(ClipboardItem item) {
  const std::string newHash = item.contentHash();
  auto existing = std::find_if(items_.begin(), items_.end(), [&](const ClipboardItem& current) {
    return current.mimeType == item.mimeType && current.contentHash() == newHash;
  });

  if (existing != items_.end()) {
    existing->timestampMs = item.timestampMs;
    if (existing->sourceApp.empty()) {
      existing->sourceApp = item.sourceApp;
    }
    return *existing;
  }

  item.id = nextId_++;
  items_.push_back(item);
  return item;
}

std::vector<ClipboardItem> InMemoryHistoryRepository::list(std::size_t limit) const {
  std::vector<ClipboardItem> result = items_;
  std::sort(result.begin(), result.end(), [](const ClipboardItem& left, const ClipboardItem& right) {
    if (left.pinned != right.pinned) {
      return left.pinned && !right.pinned;
    }
    return left.timestampMs > right.timestampMs;
  });

  if (result.size() > limit) {
    result.resize(limit);
  }

  return result;
}

std::optional<ClipboardItem> InMemoryHistoryRepository::findById(std::int64_t id) const {
  auto it = std::find_if(items_.begin(), items_.end(), [id](const ClipboardItem& item) {
    return item.id == id;
  });

  if (it == items_.end()) {
    return std::nullopt;
  }

  return *it;
}

bool InMemoryHistoryRepository::setPinned(std::int64_t id, bool pinned) {
  auto it = std::find_if(items_.begin(), items_.end(), [id](const ClipboardItem& item) {
    return item.id == id;
  });
  if (it == items_.end()) {
    return false;
  }

  it->pinned = pinned;
  return true;
}

bool InMemoryHistoryRepository::remove(std::int64_t id) {
  auto before = items_.size();
  std::erase_if(items_, [id](const ClipboardItem& item) {
    return item.id == id;
  });
  return items_.size() != before;
}

void InMemoryHistoryRepository::clear() {
  items_.clear();
}

void InMemoryHistoryRepository::clearUnpinned() {
  std::erase_if(items_, [](const ClipboardItem& item) {
    return !item.pinned;
  });
}

void InMemoryHistoryRepository::trimToLimit(std::size_t limit) {
  if (items_.empty()) {
    return;
  }

  std::vector<ClipboardItem> pinned;
  std::vector<ClipboardItem> unpinned;
  pinned.reserve(items_.size());
  unpinned.reserve(items_.size());

  for (const auto& item : items_) {
    if (item.pinned) {
      pinned.push_back(item);
    } else {
      unpinned.push_back(item);
    }
  }

  const std::size_t pinnedCount = pinned.size();
  if (pinnedCount >= limit) {
    items_ = std::move(pinned);
    return;
  }

  std::sort(unpinned.begin(), unpinned.end(), [](const ClipboardItem& left, const ClipboardItem& right) {
    return left.timestampMs > right.timestampMs;
  });

  const std::size_t allowedUnpinned = limit - pinnedCount;
  if (unpinned.size() > allowedUnpinned) {
    unpinned.resize(allowedUnpinned);
  }

  pinned.insert(pinned.end(), unpinned.begin(), unpinned.end());
  items_ = std::move(pinned);
}

}  // namespace copyclickk
