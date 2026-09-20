#pragma once
#include "opentoon/document.h"
#include <functional>
#include <span>
#include <string>
namespace opentoon {
using ResourceWriter = std::function<std::string(std::span<const std::uint8_t>)>;
using ResourceReader = std::function<std::vector<std::uint8_t>(const std::string&)>;
[[nodiscard]] std::string serializeDocument(const Document&, ResourceWriter = {});
[[nodiscard]] Document deserializeDocument(const std::string&, ResourceReader = {});
} // namespace opentoon
