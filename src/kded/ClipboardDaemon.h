#pragma once

#include <memory>

#include "../privacy/PrivacyFilter.h"
#include "../storage/IHistoryRepository.h"

namespace copyclickk {

class ClipboardDaemon {
 public:
  explicit ClipboardDaemon(std::shared_ptr<IHistoryRepository> repository);
  ClipboardDaemon(std::shared_ptr<IHistoryRepository> repository, PrivacyFilter privacyFilter);

  void start();
  void stop();
  [[nodiscard]] bool isRunning() const;
  [[nodiscard]] bool ingest(const ClipboardItem& item);

 private:
  std::shared_ptr<IHistoryRepository> repository_;
  PrivacyFilter privacyFilter_;
  bool running_{false};
};

}  // namespace copyclickk
