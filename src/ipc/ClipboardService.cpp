#include "ClipboardService.h"

#include <stdexcept>

namespace copyclickk {

ClipboardService::ClipboardService(std::shared_ptr<IHistoryRepository> repository, PrivacyRuleSet rules)
    : repository_(std::move(repository)), privacyFilter_(std::move(rules)) {
  if (!repository_) {
    throw std::invalid_argument("ClipboardService requires a non-null repository");
  }
}

bool ClipboardService::ingest(const ClipboardItem& item) {
  if (privacyFilter_.evaluate(item) != PrivacyDecision::Allow) {
    return false;
  }

  repository_->add(item);
  return true;
}

bool ClipboardService::ingestWithLimit(const ClipboardItem& item, std::size_t historyLimit) {
  if (!ingest(item)) {
    return false;
  }

  trimToLimit(historyLimit);
  return true;
}

void ClipboardService::trimToLimit(std::size_t historyLimit) {
  repository_->trimToLimit(historyLimit);
}

std::vector<ClipboardItem> ClipboardService::listRecent(std::size_t limit) const {
  return repository_->list(limit);
}

std::optional<ClipboardItem> ClipboardService::getItem(std::int64_t id) const {
  return repository_->findById(id);
}

bool ClipboardService::pinItem(std::int64_t id, bool pinned) {
  return repository_->setPinned(id, pinned);
}

bool ClipboardService::deleteItem(std::int64_t id) {
  return repository_->remove(id);
}

void ClipboardService::clearHistory() {
  repository_->clear();
}

void ClipboardService::clearUnpinnedHistory() {
  repository_->clearUnpinned();
}

}  // namespace copyclickk
