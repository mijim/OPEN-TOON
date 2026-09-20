#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>
namespace opentoon {
[[nodiscard]] std::string resourceHash(std::span<const std::uint8_t>);
[[nodiscard]] std::vector<std::uint8_t> compressResource(std::span<const std::uint8_t>);
[[nodiscard]] std::vector<std::uint8_t> decompressResource(std::span<const std::uint8_t>,
                                                           std::size_t expected);
} // namespace opentoon
