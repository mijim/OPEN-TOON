#include "opentoon/audio.h"
#include "opentoon/session.h"
#include "project_store.h"
#include "serialization.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <array>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace opentoon;
namespace {
void write16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(value & 255);
    out.push_back(value >> 8);
}
void write32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    write16(out, value & 65535);
    write16(out, value >> 16);
}
std::vector<std::uint8_t> wav(std::uint32_t samples, std::uint32_t cue,
                              std::uint32_t sampleRate = 48000) {
    std::vector<std::uint8_t> result{'R', 'I', 'F', 'F'};
    write32(result, 36 + samples * 2);
    result.insert(result.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    write32(result, 16);
    write16(result, 1);
    write16(result, 1);
    write32(result, sampleRate);
    write32(result, sampleRate * 2);
    write16(result, 2);
    write16(result, 16);
    result.insert(result.end(), {'d', 'a', 't', 'a'});
    write32(result, samples * 2);
    for (std::uint32_t i = 0; i < samples; ++i)
        write16(result, i == cue ? 32767 : 0);
    return result;
}
} // namespace

TEST_CASE("PCM16 import rejects broken media atomically and aligns waveform to the source sample") {
    Session session;
    auto source = wav(48000, 2002);
    auto malformed = source;
    malformed[20] = 3; // Unsupported IEEE float tag.
    const auto before = session.document();
    REQUIRE_THROWS(session.apply("Bad audio", [&](Document& d) {
        (void)importPcm16Wav(d, "bad", malformed, 0);
    }));
    REQUIRE(session.document() == before);
    REQUIRE(session.apply("Import audio", [&](Document& d) {
        (void)importPcm16Wav(d, "cue", source, 0);
    }));
    const auto& asset = session.document().audioAssets.front();
    REQUIRE(asset.wav.values() == source);
    REQUIRE(asset.sampleFrames == 48000);
    REQUIRE(audioPeak(asset, 0, 2002) == 0);
    REQUIRE(audioPeak(asset, 2002, 2003) > 0.99);
    REQUIRE(audioPeak(asset, 2003, 4000) == 0);
    REQUIRE(session.undo());
    REQUIRE(session.document() == before);
    REQUIRE(session.redo());
    REQUIRE(session.document().audioAssets.front().wav.values() == source);
}

TEST_CASE("Audio edits retain original PCM through undo and format-16 project reopen") {
    Session session;
    const auto source = wav(96000, 48000);
    Id clipId = 0;
    REQUIRE(session.apply("Import audio", [&](Document& d) {
        clipId = importPcm16Wav(d, "dialogue", source, 4);
    }));
    const auto baseline = session.document();
    REQUIRE(session.apply("Move audio", [&](Document& d) { moveAudioClip(d, clipId, 8); }));
    REQUIRE(session.apply("Trim audio", [&](Document& d) { trimAudioClip(d, clipId, 2002, 80000); }));
    REQUIRE(session.apply("Gain", [&](Document& d) { setAudioClipGain(d, clipId, 0.5); }));
    REQUIRE_THROWS(session.apply("Bad trim", [&](Document& d) { trimAudioClip(d, clipId, 80000, 80000); }));
    const auto edited = session.document();
    REQUIRE(edited.audioAssets.front().wav.values() == source);
    REQUIRE(session.undo());
    REQUIRE(session.undo());
    REQUIRE(session.undo());
    REQUIRE(session.document() == baseline);
    REQUIRE(session.redo());
    REQUIRE(session.redo());
    REQUIRE(session.redo());
    REQUIRE(session.document() == edited);
    const auto folder = std::filesystem::temp_directory_path() /
        ("opentoon-audio-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);
    const auto path = folder / "sound.otoon";
    try {
        (void)ProjectStore::save(path, edited);
        REQUIRE(ProjectStore::load(path).document == edited);
        auto older = nlohmann::json::parse(serializeDocument(edited));
        older["version"] = 15;
        older.erase("audioAssets");
        older.erase("audioClips");
        const auto migrated = deserializeDocument(older.dump());
        REQUIRE(migrated.audioAssets.empty());
        REQUIRE(migrated.audioClips.empty());
        std::filesystem::remove_all(folder);
    } catch (...) {
        std::filesystem::remove_all(folder);
        throw;
    }
}

TEST_CASE("Rational audio cue positions remain exact across integer and fractional frame rates") {
    for (auto rate : {FrameRate{24, 1}, FrameRate{24000, 1001}}) {
        const auto first = rate.sampleAt(1, 48000);
        REQUIRE(first == (rate.numerator == 24 ? 2000 : 2002));
        const auto last = rate.sampleAt(14386, 48000);
        const auto direct = std::int64_t(14386) * rate.denominator * 48000 / rate.numerator;
        REQUIRE(last == direct);
        REQUIRE(last - rate.sampleAt(14385, 48000) <= 2002);
    }
}
TEST_CASE("Two placed clips mix at the same rational sample with rate conversion and stereo output") {
    auto d = makeDocument();
    const auto first = importPcm16Wav(d, "cue-48k", wav(48000, 2000), 0);
    const auto second = importPcm16Wav(d, "cue-44k", wav(44100, 0, 44100), 1);
    setAudioClipGain(d, first, 0.5);
    setAudioClipGain(d, second, 0.5);
    d.validate();
    AudioMixPlan mix(d, 48000);
    REQUIRE(mix.sceneSamples() == 96000);
    const auto block = mix.renderBlock(1998, 5);
    REQUIRE(block.size() == 10);
    REQUIRE(block[2] == 0);
    REQUIRE(block[3] == 0);
    REQUIRE(block[4] == 32767);
    REQUIRE(block[5] == 32767);
    REQUIRE(block[6] > 0); // Interpolated 44.1 kHz source after its first sample.
    REQUIRE(block[6] == block[7]);
    REQUIRE_THROWS(mix.renderBlock(95999, 2));
    auto fractional = d;
    fractional.rate = {24000, 1001};
    fractional.validate();
    AudioMixPlan fractionalMix(fractional, 48000);
    REQUIRE(fractionalMix.sceneSamples() == 96096);
    const auto fractionalCue = fractionalMix.renderBlock(2002, 1);
    REQUIRE(fractionalCue[0] > 16000);
}
TEST_CASE("Audio callback-sized mix stays below its playback period on a ten-minute scene") {
    auto scene = makeDocument();
    scene.duration = 14386; // Slightly more than ten minutes at 24000/1001.
    scene.rate = {24000, 1001};
    (void)importPcm16Wav(scene, "48k", wav(96000, 2002), 0);
    (void)importPcm16Wav(scene, "44.1k", wav(88200, 1839, 44100), 0);
    scene.validate();
    AudioMixPlan mix(scene, 48000);
    REQUIRE(mix.sceneSamples() == scene.rate.sampleAt(scene.duration, 48000));
    std::array<std::int16_t, 1024 * 2> output{};
    std::array<double, 1024 * 2> scratch{};
    std::vector<double> durations;
    durations.reserve(80);
    for (int block = 0; block < 80; ++block) {
        const auto before = std::chrono::steady_clock::now();
        mix.renderInto(std::int64_t(block) * 1024, output, scratch);
        const auto after = std::chrono::steady_clock::now();
        durations.push_back(std::chrono::duration<double, std::milli>(after - before).count());
    }
    std::sort(durations.begin(), durations.end());
    const auto p95 = durations[75];
    std::cout << "Two-track 1024-frame audio mix p95: " << p95 << " ms; device period: "
              << (1000.0 * 1024 / 48000) << " ms\n";
    REQUIRE(p95 < 1000.0 * 1024 / 48000);
    REQUIRE(mix.renderBlock(2002, 1)[0] != 0);
}
