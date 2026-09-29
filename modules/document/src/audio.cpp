#include "opentoon/audio.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
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
constexpr int resamplerTaps = 32;
constexpr int resamplerPhases = 1024;
std::vector<float> lowpassKernel(std::int32_t sourceRate, std::int32_t outputRate) {
    std::vector<float> weights(resamplerTaps * resamplerPhases);
    const double cutoff = .9 * std::min(1.0, double(outputRate) / sourceRate);
    for (int phase = 0; phase < resamplerPhases; ++phase) {
        const double fraction = double(phase) / resamplerPhases;
        double total = 0;
        for (int tap = 0; tap < resamplerTaps; ++tap) {
            const double distance = tap - 15 - fraction;
            double value = 0;
            if (std::abs(distance) < 16) {
                const double argument = std::numbers::pi * distance;
                const double sinc = std::abs(distance) < 1e-12
                                        ? cutoff
                                        : std::sin(argument * cutoff) / argument;
                const double window = .42 + .5 * std::cos(argument / 16) +
                                      .08 * std::cos(argument / 8);
                value = sinc * window;
            }
            weights[phase * resamplerTaps + tap] = float(value);
            total += value;
        }
        for (int tap = 0; tap < resamplerTaps; ++tap)
            weights[phase * resamplerTaps + tap] =
                float(weights[phase * resamplerTaps + tap] / total);
    }
    return weights;
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
    document.audioClips.push_back({clipId, assetId, start, 0, info.sampleFrames, 1, 1});
    return clipId;
}
void moveAudioClip(Document& document, Id id, Frame start) {
    if (start < 0 || start >= document.duration)
        throw std::invalid_argument("Audio clip start is outside the scene.");
    clip(document, id).start = start;
}
Id duplicateAudioClip(Document& document, Id id, Frame start) {
    if (start < 0 || start >= document.duration || document.audioClips.size() >= 1000)
        throw std::invalid_argument("Audio clip copy is outside the scene or clip limit.");
    const auto original = clip(document, id);
    if (std::none_of(document.audioAssets.begin(), document.audioAssets.end(),
                     [&](const auto& asset) { return asset.id == original.asset; }))
        throw std::invalid_argument("Audio clip source does not exist.");
    auto copy = original;
    copy.id = document.allocateId();
    copy.start = start;
    document.audioClips.push_back(copy);
    return copy.id;
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
    const auto total = (outSample - inSample) * std::uint64_t(target.repeats);
    target.fadeInSamples = std::min(target.fadeInSamples, total);
    target.fadeOutSamples = std::min(target.fadeOutSamples, total - target.fadeInSamples);
}
void setAudioClipGain(Document& document, Id id, double gain) {
    if (!std::isfinite(gain) || gain < 0 || gain > 4)
        throw std::invalid_argument("Audio gain must be between zero and four.");
    clip(document, id).gain = gain;
}
void setAudioClipRepeats(Document& document, Id id, int repeats) {
    if (repeats < 1 || repeats > 64)
        throw std::invalid_argument("Audio repeat count must be between one and 64.");
    auto& target = clip(document, id);
    target.repeats = repeats;
    const auto total = (target.outSample - target.inSample) * std::uint64_t(repeats);
    target.fadeInSamples = std::min(target.fadeInSamples, total);
    target.fadeOutSamples = std::min(target.fadeOutSamples, total - target.fadeInSamples);
}
void setAudioClipFades(Document& document, Id id, std::uint64_t fadeInSamples,
                       std::uint64_t fadeOutSamples) {
    auto& target = clip(document, id);
    const auto total = (target.outSample - target.inSample) * std::uint64_t(target.repeats);
    if (fadeInSamples > total || fadeOutSamples > total - fadeInSamples)
        throw std::invalid_argument("Audio fades exceed the repeated clip length.");
    target.fadeInSamples = fadeInSamples;
    target.fadeOutSamples = fadeOutSamples;
}
void removeAudioClip(Document& document, Id id) {
    const auto oldSize = document.audioClips.size();
    std::erase_if(document.audioClips, [=](const auto& item) { return item.id == id; });
    if (oldSize == document.audioClips.size())
        throw std::invalid_argument("Audio clip does not exist.");
}
Frame audioClipEndFrame(const Document& document, const AudioClip& clip,
                        const AudioAsset& asset) {
    if (clip.start < 0 || clip.start >= document.duration ||
        clip.inSample >= clip.outSample || clip.outSample > asset.sampleFrames ||
        clip.repeats < 1 || clip.repeats > 64)
        throw std::invalid_argument("Invalid audio clip interval.");
    const auto samples = (clip.outSample - clip.inSample) * std::uint64_t(clip.repeats);
    Frame low = 0, high = document.duration - clip.start;
    while (low < high) {
        const Frame mid = low + (high - low) / 2;
        if (std::uint64_t(document.rate.sampleAt(mid, asset.sampleRate)) >= samples)
            high = mid;
        else
            low = mid + 1;
    }
    return clip.start + low;
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
AudioPeakIndex::AudioPeakIndex(const AudioAsset& asset)
    : wav_(asset.wav), sampleFrames_(asset.sampleFrames), channels_(asset.channels),
      sampleRate_(asset.sampleRate) {
    const auto info = inspectPcm16Wav({wav_.data(), wav_.size()});
    if (info.sampleFrames != sampleFrames_ || info.channels != channels_ ||
        info.sampleRate != sampleRate_)
        throw std::invalid_argument("Audio peak source metadata does not match its WAV.");
    dataOffset_ = info.dataOffset;
    const auto binCount = std::size_t((sampleFrames_ + 255) / 256);
    while (treeBase_ < binCount)
        treeBase_ *= 2;
    tree_.resize(treeBase_ * 2);
    for (std::uint64_t frame = 0; frame < sampleFrames_; ++frame)
        tree_[treeBase_ + std::size_t(frame / 256)] =
            std::max(tree_[treeBase_ + std::size_t(frame / 256)], samplePeak(frame));
    for (std::size_t node = treeBase_ - 1; node > 0; --node)
        tree_[node] = std::max(tree_[node * 2], tree_[node * 2 + 1]);
}
bool AudioPeakIndex::matches(const AudioAsset& asset) const {
    return wav_.data() == asset.wav.data() && sampleFrames_ == asset.sampleFrames &&
           channels_ == asset.channels && sampleRate_ == asset.sampleRate;
}
std::uint16_t AudioPeakIndex::samplePeak(std::uint64_t frame) const {
    std::uint16_t peak = 0;
    for (int channel = 0; channel < channels_; ++channel) {
        const auto at = dataOffset_ + (frame * channels_ + channel) * 2;
        const auto encoded = std::uint16_t(wav_[at] | (std::uint16_t(wav_[at + 1]) << 8));
        const int value = encoded <= 32767 ? encoded : int(encoded) - 65536;
        peak = std::max(peak, std::uint16_t(value == -32768 ? 32768 : std::abs(value)));
    }
    return peak;
}
std::uint16_t AudioPeakIndex::edgePeak(std::uint64_t begin, std::uint64_t end) const {
    std::uint16_t peak = 0;
    for (auto frame = begin; frame < end; ++frame)
        peak = std::max(peak, samplePeak(frame));
    return peak;
}
double AudioPeakIndex::peak(std::uint64_t begin, std::uint64_t end) const {
    if (begin > end || end > sampleFrames_)
        throw std::invalid_argument("Waveform interval is outside the audio asset.");
    if (begin == end)
        return 0;
    const auto firstFull = std::min(end, ((begin + 255) / 256) * 256);
    const auto lastFull = (end / 256) * 256;
    if (firstFull >= lastFull)
        return double(edgePeak(begin, end)) / 32768.0;
    std::uint16_t maximum = std::max(edgePeak(begin, firstFull), edgePeak(lastFull, end));
    auto left = treeBase_ + std::size_t(firstFull / 256);
    auto right = treeBase_ + std::size_t(lastFull / 256);
    while (left < right) {
        if (left & 1)
            maximum = std::max(maximum, tree_[left++]);
        if (right & 1)
            maximum = std::max(maximum, tree_[--right]);
        left /= 2;
        right /= 2;
    }
    return double(maximum) / 32768.0;
}
double audioClipFramePeak(const Document& document, const AudioClip& clip,
                          const AudioAsset& asset, const AudioPeakIndex& index,
                          Frame frame) {
    if (frame < clip.start || frame >= document.duration)
        return 0;
    const auto length = clip.outSample - clip.inSample;
    if (!length || clip.repeats < 1 || clip.repeats > 64)
        throw std::invalid_argument("Invalid repeated audio clip.");
    const auto total = length * std::uint64_t(clip.repeats);
    const auto begin = std::min(total,
        std::uint64_t(document.rate.sampleAt(frame - clip.start, asset.sampleRate)));
    const auto end = std::min(total,
        std::uint64_t(document.rate.sampleAt(frame - clip.start + 1, asset.sampleRate)));
    if (begin >= end)
        return 0;
    if (end - begin >= length)
        return index.peak(clip.inSample, clip.outSample);
    const auto beginLoop = begin / length, endLoop = (end - 1) / length;
    if (beginLoop == endLoop)
        return index.peak(clip.inSample + begin % length,
                          clip.inSample + (end - 1) % length + 1);
    return std::max(index.peak(clip.inSample + begin % length, clip.outSample),
                    index.peak(clip.inSample, clip.inSample + (end - 1) % length + 1));
}
AudioMixPlan::AudioMixPlan(const Document& document, std::int32_t outputRate)
    : frameRate_(document.rate), duration_(document.duration), outputRate_(outputRate) {
    frameRate_.validate();
    if (outputRate < 8000 || outputRate > 192000)
        throw std::invalid_argument("Unsupported audio output sample rate.");
    for (const auto& clip : document.audioClips) {
        const auto asset = std::find_if(document.audioAssets.begin(), document.audioAssets.end(),
                                        [&](const auto& value) { return value.id == clip.asset; });
        if (asset == document.audioAssets.end())
            throw std::invalid_argument("Audio clip references a missing source.");
        const auto info = inspectPcm16Wav({asset->wav.data(), asset->wav.size()});
        if (clip.inSample >= clip.outSample || clip.outSample > info.sampleFrames ||
            clip.start < 0 || clip.start >= duration_ || !std::isfinite(clip.gain) ||
            clip.gain < 0 || clip.gain > 4 || clip.repeats < 1 || clip.repeats > 64 ||
            clip.fadeInSamples > (clip.outSample - clip.inSample) * std::uint64_t(clip.repeats) ||
            clip.fadeOutSamples > (clip.outSample - clip.inSample) *
                                      std::uint64_t(clip.repeats) - clip.fadeInSamples)
            throw std::invalid_argument("Invalid audio clip in mix plan.");
        sources_.push_back({&*asset, clip, info.dataOffset,
                            frameRate_.sampleAt(clip.start, outputRate_)});
        if (asset->sampleRate != outputRate_ &&
            !rateKernels_.contains(asset->sampleRate))
            rateKernels_.emplace(asset->sampleRate,
                                 lowpassKernel(asset->sampleRate, outputRate_));
    }
}
std::int64_t AudioMixPlan::sceneSamples() const {
    return frameRate_.sampleAt(duration_, outputRate_);
}
void AudioMixPlan::renderInto(std::int64_t firstSample, std::span<std::int16_t> output,
                              std::span<double> scratch) const {
    if (output.size() % 2 || scratch.size() < output.size() || firstSample < 0 ||
        std::uint64_t(firstSample) + output.size() / 2 > std::uint64_t(sceneSamples()))
        throw std::invalid_argument("Audio render block is outside the scene.");
    const auto frameCount = output.size() / 2;
    std::fill_n(scratch.begin(), output.size(), 0.0);
    for (const auto& source : sources_) {
        const auto& asset = *source.asset;
        const std::span bytes{asset.wav.data(), asset.wav.size()};
        const auto kernel = rateKernels_.find(asset.sampleRate);
        const bool rateConversion = kernel != rateKernels_.end();
        for (std::size_t index = 0; index < frameCount; ++index) {
            const auto sceneSample = firstSample + std::int64_t(index);
            if (sceneSample < source.startSample)
                continue;
            const auto relative = std::uint64_t(sceneSample - source.startSample);
            const auto wholeSeconds = relative / outputRate_;
            const auto remainder = relative % outputRate_;
            const auto subsecond = remainder * std::uint64_t(asset.sampleRate);
            const auto sourceOffset = wholeSeconds * std::uint64_t(asset.sampleRate) +
                                      subsecond / outputRate_;
            const auto length = source.clip.outSample - source.clip.inSample;
            if (sourceOffset >= length * std::uint64_t(source.clip.repeats))
                continue;
            const auto cycle = sourceOffset / length;
            const auto sourceFrame = source.clip.inSample + sourceOffset % length;
            const auto nextFrame = sourceFrame + 1 < source.clip.outSample
                                       ? sourceFrame + 1
                                       : cycle + 1 < std::uint64_t(source.clip.repeats)
                                             ? source.clip.inSample : sourceFrame;
            const double fraction = double(subsecond % outputRate_) / outputRate_;
            const double sourcePosition = double(sourceOffset) + fraction;
            const double totalSource = double(length * std::uint64_t(source.clip.repeats));
            const auto ramp = [](double position, std::uint64_t samples) {
                if (!samples)
                    return 1.0;
                if (samples == 1)
                    return position > 0 ? 1.0 : 0.0;
                return std::clamp(position / double(samples - 1), 0.0, 1.0);
            };
            const double envelope = ramp(sourcePosition, source.clip.fadeInSamples) *
                                    ramp(totalSource - 1 - sourcePosition,
                                         source.clip.fadeOutSamples);
            auto read = [&](std::uint64_t frame, int channel) {
                const auto at = source.dataOffset + (frame * asset.channels + channel) * 2;
                const auto encoded = u16(bytes, at);
                return double(encoded <= 32767 ? int(encoded) : int(encoded) - 65536) / 32768.0;
            };
            for (int channel = 0; channel < 2; ++channel) {
                const int inputChannel = std::min(channel, asset.channels - 1);
                double value = 0;
                if (rateConversion) {
                    const auto phase = std::size_t((subsecond % outputRate_) * resamplerPhases /
                                                   outputRate_);
                    const auto* weights = kernel->second.data() + phase * resamplerTaps;
                    const auto total = std::int64_t(length * std::uint64_t(source.clip.repeats));
                    for (int tap = 0; tap < resamplerTaps; ++tap) {
                        const auto offset = std::clamp(std::int64_t(sourceOffset) + tap - 15,
                                                       std::int64_t(0), total - 1);
                        const auto frame = source.clip.inSample + std::uint64_t(offset) % length;
                        value += read(frame, inputChannel) * weights[tap];
                    }
                } else {
                    const double a = read(sourceFrame, inputChannel);
                    const double b = read(nextFrame, inputChannel);
                    value = a + (b - a) * fraction;
                }
                scratch[index * 2 + channel] += value * source.clip.gain * envelope;
            }
        }
    }
    for (std::size_t i = 0; i < output.size(); ++i)
        output[i] = std::int16_t(std::lround(std::clamp(scratch[i], -1.0,
                                                         32767.0 / 32768.0) * 32768.0));
}
std::vector<std::int16_t> AudioMixPlan::renderBlock(std::int64_t firstSample,
                                                    std::size_t frameCount) const {
    if (frameCount > 65536)
        throw std::invalid_argument("Audio render block exceeds its supported size.");
    std::vector<std::int16_t> output(frameCount * 2);
    std::vector<double> scratch(output.size());
    renderInto(firstSample, output, scratch);
    return output;
}
} // namespace opentoon
