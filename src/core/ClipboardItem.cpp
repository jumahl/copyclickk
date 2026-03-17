#include "ClipboardItem.h"

#include <functional>
#include <sstream>

namespace copyclickk {

std::string ClipboardItem::contentHash() const {
  std::uint64_t seed = static_cast<std::uint64_t>(mimeType);
  for (std::uint8_t byte : payload) {
    seed ^= static_cast<std::uint64_t>(byte) + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
  }

  std::ostringstream stream;
  stream << std::hex << seed;
  return stream.str();
}

}  // namespace copyclickk
