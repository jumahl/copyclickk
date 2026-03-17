#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace copyclickk {

enum class ClipboardMimeType {
  TextPlain,
  TextHtml,
  ImagePng,
  ImageJpeg,
  ImageWebp,
  ImageBmp,
  ImageIco,
  TextUriList,
  Unknown,
};

struct ClipboardItem {
  std::int64_t id{0};
  std::int64_t timestampMs{0};
  ClipboardMimeType mimeType{ClipboardMimeType::Unknown};
  std::vector<std::uint8_t> payload;
  std::string sourceApp;
  bool pinned{false};

  [[nodiscard]] std::string contentHash() const;
};

}  // namespace copyclickk
