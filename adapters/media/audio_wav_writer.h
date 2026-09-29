#pragma once
#include "opentoon/document.h"
#include <QIODevice>
#include <functional>
#include <stdexcept>

namespace opentoon {
class AudioExportCancelled : public std::runtime_error {
  public:
    AudioExportCancelled() : std::runtime_error("Audio export cancelled.") {}
};
struct AudioWavResult {
    std::int64_t sampleFrames = 0;
    std::int32_t sampleRate = 48000;
    std::int32_t channels = 2;
};
[[nodiscard]] AudioWavResult writeAudioWav(const Document& document, QIODevice& output,
                                            std::function<bool()> cancelled = {},
                                            std::function<void(double)> progress = {});
} // namespace opentoon
