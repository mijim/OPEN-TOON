#include "audio_device.h"
#include "opentoon/audio.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
void append16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(value & 255);
    bytes.push_back(value >> 8);
}
void append32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    append16(bytes, value & 65535);
    append16(bytes, value >> 16);
}
std::vector<std::uint8_t> silentWav() {
    constexpr std::uint32_t frames = 48000 * 10;
    std::vector<std::uint8_t> bytes{'R', 'I', 'F', 'F'};
    append32(bytes, 36 + frames * 2);
    bytes.insert(bytes.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    append32(bytes, 16); append16(bytes, 1); append16(bytes, 1);
    append32(bytes, 48000); append32(bytes, 96000);
    append16(bytes, 2); append16(bytes, 16);
    bytes.insert(bytes.end(), {'d', 'a', 't', 'a'});
    append32(bytes, frames * 2);
    bytes.resize(bytes.size() + frames * 2);
    return bytes;
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc != 4)
            throw std::invalid_argument("Usage: opentoon_audio_sync_benchmark 24|24000/1001 host|null seconds");
        const std::string rateName = argv[1], backendName = argv[2];
        const bool nullBackend = backendName == "null";
        if (backendName != "host" && !nullBackend)
            throw std::invalid_argument("Audio backend must be host or null.");
        const int seconds = std::stoi(argv[3]);
        if (seconds < 1 || seconds > 600)
            throw std::invalid_argument("The measurement must last 1–600 seconds.");
        auto document = opentoon::makeDocument();
        document.rate = rateName == "24" ? opentoon::FrameRate{24, 1}
                      : rateName == "24000/1001" ? opentoon::FrameRate{24000, 1001}
                      : throw std::invalid_argument("Unsupported scene rate.");
        document.duration = 15000;
        const auto clip = opentoon::importPcm16Wav(document, "silent clock probe", silentWav(), 0);
        opentoon::setAudioClipRepeats(document, clip, 64);
        document.validate();
        auto snapshot = std::make_shared<const opentoon::Document>(std::move(document));
        opentoon::AudioDevice device(snapshot, nullBackend);
        device.start(0);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        const auto baselineTime = std::chrono::steady_clock::now();
        const auto baselineSample = device.currentSample();
        auto previousFrame = device.currentFrame();
        std::uint64_t skippedFrames = 0;
        std::int64_t maximumJitter = 0, finalDrift = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
            const auto elapsed = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - baselineTime).count();
            const auto current = device.currentSample();
            const auto frame = device.currentFrame();
            if (frame > previousFrame + 1)
                skippedFrames += std::uint64_t(frame - previousFrame - 1);
            previousFrame = frame;
            finalDrift = current - baselineSample - std::int64_t(std::llround(elapsed * 48000));
            maximumJitter = std::max(maximumJitter, std::abs(finalDrift));
            if (elapsed >= seconds)
                break;
        }
        device.stop();
        const auto stats = device.stats();
        std::cout << "rate=" << rateName << " backend=" << backendName
                  << " seconds=" << seconds << " callbacks=" << stats.callbacks
                  << " callback_overruns=" << stats.processingOverruns
                  << " max_callback_ms=" << stats.maximumCallbackNanoseconds / 1000000.0
                  << " final_drift_samples=" << finalDrift
                  << " max_jitter_samples=" << maximumJitter
                  << " skipped_polled_frames=" << skippedFrames
                  << " interrupted=" << device.interrupted() << '\n';
        return device.interrupted() ? 2 : 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
