#include "resources.h"
#include <array>
#include <openssl/evp.h>
#include <stdexcept>
#include <zstd.h>
namespace opentoon {
std::string resourceHash(std::span<const std::uint8_t> data) {
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int length = 0;
    if (EVP_Digest(data.data(), data.size(), digest.data(), &length, EVP_sha256(), nullptr) != 1 ||
        length != 32)
        throw std::runtime_error("Cannot calculate resource SHA-256.");
    const char digits[] = "0123456789abcdef";
    std::string hash;
    hash.reserve(64);
    for (unsigned int i = 0; i < length; ++i) {
        hash += digits[digest[i] >> 4];
        hash += digits[digest[i] & 15];
    }
    return hash;
}
std::vector<std::uint8_t> compressResource(std::span<const std::uint8_t> data) {
    std::vector<std::uint8_t> result(ZSTD_compressBound(data.size()));
    auto count = ZSTD_compress(result.data(), result.size(), data.data(), data.size(), 3);
    if (ZSTD_isError(count))
        throw std::runtime_error("Resource compression failed.");
    result.resize(count);
    return result;
}
std::vector<std::uint8_t> decompressResource(std::span<const std::uint8_t> data, std::size_t expected) {
    if (expected > 256 * 1024 * 1024)
        throw std::runtime_error("Resource decompression budget exceeded.");
    auto declared = ZSTD_getFrameContentSize(data.data(), data.size());
    if (declared != expected)
        throw std::runtime_error("Invalid compressed resource size.");
    std::vector<std::uint8_t> result(expected);
    auto count = ZSTD_decompress(result.data(), result.size(), data.data(), data.size());
    if (ZSTD_isError(count) || count != expected)
        throw std::runtime_error("Resource decompression failed.");
    return result;
}
} // namespace opentoon
