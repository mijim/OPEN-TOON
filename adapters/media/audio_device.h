#pragma once
#include "opentoon/document.h"
#include <cstdint>
#include <memory>

namespace opentoon {
struct AudioDeviceStats {
    std::uint64_t callbacks = 0;
    std::uint64_t processingOverruns = 0;
    std::uint64_t maximumCallbackNanoseconds = 0;
};
// One playback device owns one immutable scene snapshot. The callback reads
// only that snapshot and publishes a sample cursor through atomics.
class AudioDevice {
  public:
    AudioDevice(std::shared_ptr<const Document> snapshot, bool nullBackend = false);
    ~AudioDevice();
    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;
    void start(Frame frame);
    void scrub(Frame frame);
    void stop();
    void seek(Frame frame);
    [[nodiscard]] Frame currentFrame() const;
    [[nodiscard]] std::int64_t currentSample() const;
    [[nodiscard]] bool running() const;
    [[nodiscard]] bool interrupted() const;
    [[nodiscard]] AudioDeviceStats stats() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace opentoon
