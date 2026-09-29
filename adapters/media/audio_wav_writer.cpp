#include "audio_wav_writer.h"
#include "opentoon/audio.h"
#include <QByteArray>
#include <algorithm>
#include <limits>

namespace opentoon {
namespace {
void u16(QByteArray& bytes, std::uint16_t value) {
    bytes.append(char(value & 255));
    bytes.append(char(value >> 8));
}
void u32(QByteArray& bytes, std::uint32_t value) {
    u16(bytes, value & 65535);
    u16(bytes, value >> 16);
}
void exactWrite(QIODevice& output, const QByteArray& bytes) {
    if (output.write(bytes) != bytes.size())
        throw std::runtime_error("Could not write the PCM WAV output.");
}
} // namespace
AudioWavResult writeAudioWav(const Document& document, QIODevice& output,
                             std::function<bool()> cancelled,
                             std::function<void(double)> progress) {
    AudioMixPlan mix(document, 48000);
    const auto samples = mix.sceneSamples();
    if (samples < 0 || samples > (std::numeric_limits<std::uint32_t>::max() - 36) / 4)
        throw std::invalid_argument("Scene audio exceeds the PCM WAV size limit.");
    if (cancelled && cancelled())
        throw AudioExportCancelled();
    const auto dataBytes = std::uint32_t(samples * 4);
    QByteArray header;
    header.append("RIFF", 4); u32(header, dataBytes + 36);
    header.append("WAVEfmt ", 8); u32(header, 16);
    u16(header, 1); u16(header, 2); u32(header, 48000);
    u32(header, 48000 * 4); u16(header, 4); u16(header, 16);
    header.append("data", 4); u32(header, dataBytes);
    exactWrite(output, header);
    constexpr std::size_t blockFrames = 8192;
    for (std::int64_t first = 0; first < samples; first += blockFrames) {
        if (cancelled && cancelled())
            throw AudioExportCancelled();
        const auto count = std::size_t(std::min<std::int64_t>(blockFrames, samples - first));
        const auto pcm = mix.renderBlock(first, count);
        QByteArray bytes;
        bytes.reserve(qsizetype(pcm.size() * 2));
        for (const auto sample : pcm)
            u16(bytes, std::uint16_t(sample));
        exactWrite(output, bytes);
        if (progress)
            progress(double(first + count) / samples);
    }
    return {samples, 48000, 2};
}
} // namespace opentoon
