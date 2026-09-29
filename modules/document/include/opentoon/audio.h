#pragma once
#include "opentoon/document.h"
#include <span>

namespace opentoon {
struct PcmWavInfo {
    std::int32_t sampleRate = 0;
    std::int32_t channels = 0;
    std::uint64_t sampleFrames = 0;
    std::size_t dataOffset = 0;
};
[[nodiscard]] PcmWavInfo inspectPcm16Wav(std::span<const std::uint8_t> bytes);
[[nodiscard]] Id importPcm16Wav(Document& document, std::string name,
                                std::vector<std::uint8_t> bytes, Frame start);
void moveAudioClip(Document& document, Id clip, Frame start);
void trimAudioClip(Document& document, Id clip, std::uint64_t inSample,
                   std::uint64_t outSample);
void setAudioClipGain(Document& document, Id clip, double gain);
void removeAudioClip(Document& document, Id clip);
// Maximum absolute PCM amplitude within a source-sample interval. This is
// independent of the current timeline zoom and leaves the source unchanged.
[[nodiscard]] double audioPeak(const AudioAsset& asset, std::uint64_t begin,
                               std::uint64_t end);
} // namespace opentoon
