#include "opentoon/audio.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace opentoon {
namespace {
std::uint16_t u16(std::span<const std::uint8_t> data, std::size_t at) {
    return std::uint16_t(data[at] | (std::uint16_t(data[at + 1]) << 8));
}
std::uint32_t u32(std::span<const std::uint8_t> data, std::size_t at) {
    return std::uint32_t(data[at]) | (std::uint32_t(data[at + 1]) << 8) |
           (std::uint32_t(data[at + 2]) << 16) | (std::uint32_t(data[at + 3]) << 24);
}
bool tag(std::span<const std::uint8_t> data, std::size_t at, const char* value) {
    return std::equal(data.begin() + at, data.begin() + at + 4, value);
}
AudioClip& clip(Document& document, Id id) {
    auto found = std::find_if(document.audioClips.begin(), document.audioClips.end(),
                              [=](const auto& item) { return item.id == id; });
    if (found == document.audioClips.end())
        throw std::invalid_argument("Audio clip does not exist.");
    return *found;
}
} // namespace
PcmWavInfo inspectPcm16Wav(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 44 || bytes.size() > 128 * 1024 * 1024 ||
        !tag(bytes, 0, "RIFF") || !tag(bytes, 8, "WAVE") ||
        std::uint64_t(u32(bytes, 4)) + 8 != bytes.size())
        throw std::invalid_argument("Expected a bounded RIFF/WAVE file.");
    bool hasFormat = false, hasData = false;
    PcmWavInfo info;
    std::size_t dataBytes = 0;
    for (std::size_t at = 12; at < bytes.size();) {
        if (bytes.size() - at < 8)
            throw std::invalid_argument("Truncated WAV chunk header.");
        const std::size_t size = u32(bytes, at + 4);
        const std::size_t payload = at + 8;
        if (size > bytes.size() - payload || (size & 1 && size == bytes.size() - payload))
            throw std::invalid_argument("Truncated WAV chunk.");
        if (tag(bytes, at, "fmt ")) {
            if (hasFormat || size < 16)
                throw std::invalid_argument("Invalid WAV format chunk.");
            hasFormat = true;
            const auto format = u16(bytes, payload);
            info.channels = u16(bytes, payload + 2);
            info.sampleRate = std::int32_t(u32(bytes, payload + 4));
            const auto byteRate = u32(bytes, payload + 8);
            const auto blockAlign = u16(bytes, payload + 12);
            const auto bits = u16(bytes, payload + 14);
            if (format != 1 || (info.channels != 1 && info.channels != 2) ||
                info.sampleRate < 8000 || info.sampleRate > 192000 || bits != 16 ||
                blockAlign != info.channels * 2 ||
                byteRate != std::uint64_t(info.sampleRate) * blockAlign)
                throw std::invalid_argument("Only mono/stereo PCM16 WAV is supported.");
        } else if (tag(bytes, at, "data")) {
            if (hasData)
                throw std::invalid_argument("Multiple WAV data chunks are unsupported.");
            hasData = true;
            info.dataOffset = payload;
            dataBytes = size;
        }
        at = payload + size + (size & 1);
    }
    if (!hasFormat || !hasData || dataBytes == 0 ||
        dataBytes % std::size_t(info.channels * 2) != 0)
        throw std::invalid_argument("Invalid or empty PCM16 WAV data.");
    info.sampleFrames = dataBytes / std::size_t(info.channels * 2);
    return info;
}
Id importPcm16Wav(Document& document, std::string name,
                  std::vector<std::uint8_t> bytes, Frame start) {
    const auto info = inspectPcm16Wav(bytes);
    if (name.empty() || name.size() > 4096 || start < 0 || start >= document.duration ||
        document.audioAssets.size() >= 64 || document.audioClips.size() >= 1000)
        throw std::invalid_argument("Invalid audio import target or resource limit.");
    std::size_t existingBytes = 0;
    for (const auto& asset : document.audioAssets)
        existingBytes += asset.wav.size();
    if (bytes.size() > 512 * 1024 * 1024 - existingBytes)
        throw std::invalid_argument("Document audio budget exceeded.");
    const Id assetId = document.allocateId(), clipId = document.allocateId();
    document.audioAssets.push_back({assetId, std::move(name), info.sampleRate,
                                    info.channels, info.sampleFrames, std::move(bytes)});
    document.audioClips.push_back({clipId, assetId, start, 0, info.sampleFrames, 1});
    return clipId;
}
void moveAudioClip(Document& document, Id id, Frame start) {
    if (start < 0 || start >= document.duration)
        throw std::invalid_argument("Audio clip start is outside the scene.");
    clip(document, id).start = start;
}
void trimAudioClip(Document& document, Id id, std::uint64_t inSample,
                   std::uint64_t outSample) {
    auto& target = clip(document, id);
    const auto asset = std::find_if(document.audioAssets.begin(), document.audioAssets.end(),
                                    [&](const auto& item) { return item.id == target.asset; });
    if (asset == document.audioAssets.end() || inSample >= outSample ||
        outSample > asset->sampleFrames)
        throw std::invalid_argument("Invalid audio trim range.");
    target.inSample = inSample;
    target.outSample = outSample;
}
void setAudioClipGain(Document& document, Id id, double gain) {
    if (!std::isfinite(gain) || gain < 0 || gain > 4)
        throw std::invalid_argument("Audio gain must be between zero and four.");
    clip(document, id).gain = gain;
}
void removeAudioClip(Document& document, Id id) {
    const auto oldSize = document.audioClips.size();
    std::erase_if(document.audioClips, [=](const auto& item) { return item.id == id; });
    if (oldSize == document.audioClips.size())
        throw std::invalid_argument("Audio clip does not exist.");
}
double audioPeak(const AudioAsset& asset, std::uint64_t begin, std::uint64_t end) {
    const auto info = inspectPcm16Wav({asset.wav.data(), asset.wav.size()});
    if (begin > end || end > info.sampleFrames)
        throw std::invalid_argument("Waveform interval is outside the audio asset.");
    std::uint32_t peak = 0;
    for (auto frame = begin; frame < end; ++frame)
        for (int channel = 0; channel < info.channels; ++channel) {
            const auto at = info.dataOffset + (frame * info.channels + channel) * 2;
            const auto value = std::int16_t(u16({asset.wav.data(), asset.wav.size()}, at));
            peak = std::max(peak, std::uint32_t(value == std::numeric_limits<std::int16_t>::min()
                                                     ? 32768 : std::abs(int(value))));
        }
    return double(peak) / 32768.0;
}
} // namespace opentoon
