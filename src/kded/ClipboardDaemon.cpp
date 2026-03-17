#include "ClipboardDaemon.h"

namespace copyclickk {

ClipboardDaemon::ClipboardDaemon(std::shared_ptr<IHistoryRepository> repository)
  : repository_(std::move(repository)), privacyFilter_({}) {}

ClipboardDaemon::ClipboardDaemon(std::shared_ptr<IHistoryRepository> repository, PrivacyFilter privacyFilter)
  : repository_(std::move(repository)), privacyFilter_(std::move(privacyFilter)) {}

void ClipboardDaemon::start() {
  running_ = true;
}

void ClipboardDaemon::stop() {
  running_ = false;
}

bool ClipboardDaemon::isRunning() const {
  return running_;
}

bool ClipboardDaemon::ingest(const ClipboardItem& item) {
  const PrivacyDecision decision = privacyFilter_.evaluate(item);
  if (decision != PrivacyDecision::Allow) {
    return false;
  }

  repository_->add(item);
  return true;
}

}  // namespace copyclickk
