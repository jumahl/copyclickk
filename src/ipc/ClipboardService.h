#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "../privacy/PrivacyFilter.h"
#include "../storage/IHistoryRepository.h"

namespace copyclickk {

class ClipboardService {
 public:
  ClipboardService(std::shared_ptr<IHistoryRepository> repository, PrivacyRuleSet rules);

  [[nodiscard]] bool ingest(const ClipboardItem& item);
  [[nodiscard]] bool ingestWithLimit(const ClipboardItem& item, std::size_t historyLimit);
  void trimToLimit(std::size_t historyLimit);
  [[nodiscard]] std::vector<ClipboardItem> listRecent(std::size_t limit) const;
  [[nodiscard]] std::optional<ClipboardItem> getItem(std::int64_t id) const;
  [[nodiscard]] bool pinItem(std::int64_t id, bool pinned);
  [[nodiscard]] bool deleteItem(std::int64_t id);
  void clearHistory();
  void clearUnpinnedHistory();

 private:
  std::shared_ptr<IHistoryRepository> repository_;
  PrivacyFilter privacyFilter_;
};

}  // namespace copyclickk
