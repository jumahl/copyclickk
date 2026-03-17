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

void InMemoryHistoryRepository::trimToLimit(std::size_t limit) {
  if (items_.size() <= limit) {
    return;
  }

  std::vector<ClipboardItem> sorted = items_;
  std::sort(sorted.begin(), sorted.end(), [](const ClipboardItem& left, const ClipboardItem& right) {
    return left.timestampMs > right.timestampMs;
  });

  if (sorted.size() > limit) {
    sorted.resize(limit);
  }

  items_ = std::move(sorted);
}

}  // namespace copyclickk
