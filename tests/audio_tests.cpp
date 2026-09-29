#include "opentoon/audio.h"
#include "opentoon/session.h"
#include "project_store.h"
#include "serialization.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <array>
#include <iostream>
#include <numbers>
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
std::vector<std::uint8_t> toneWav(std::uint32_t samples, std::uint32_t sampleRate,
                                  double frequency) {
    auto result = wav(samples, samples, sampleRate);
    for (std::uint32_t i = 0; i < samples; ++i) {
        const auto value = std::int16_t(std::lround(26000 *
            std::sin(2 * std::numbers::pi * frequency * i / sampleRate)));
        result[44 + i * 2] = std::uint8_t(value);
        result[45 + i * 2] = std::uint8_t(std::uint16_t(value) >> 8);
    }
    return result;
}
} // namespace

TEST_CASE("Downsampling suppresses aliased treble and keeps audible passband") {
    auto scene = makeDocument();
    (void)importPcm16Wav(scene, "30 kHz", toneWav(9600, 96000, 30000), 0);
    auto mix = AudioMixPlan(scene, 48000);
    const auto rejected = mix.renderBlock(480, 3840);
    auto rms = [](const std::vector<std::int16_t>& values) {
        double energy = 0;
        for (std::size_t i = 0; i < values.size(); i += 2) {
            const double sample = values[i] / 32768.0;
            energy += sample * sample;
        }
        return std::sqrt(energy / (values.size() / 2));
    };
    const auto rejectedRms = rms(rejected);
    REQUIRE(rejectedRms < .02);
    scene = makeDocument();
    (void)importPcm16Wav(scene, "1 kHz", toneWav(9600, 96000, 1000), 0);
    mix = AudioMixPlan(scene, 48000);
    const auto retained = mix.renderBlock(480, 3840);
    const auto retainedRms = rms(retained);
    std::cout << "96-to-48 kHz RMS: rejected 30 kHz=" << rejectedRms
              << ", retained 1 kHz=" << retainedRms << '\n';
    REQUIRE(retainedRms > .5);
    REQUIRE(retainedRms < .6);
    const auto whole = mix.renderBlock(0, 4800);
    const auto first = mix.renderBlock(0, 2400);
    const auto second = mix.renderBlock(2400, 2400);
    REQUIRE(std::equal(first.begin(), first.end(), whole.begin()));
    REQUIRE(std::equal(second.begin(), second.end(), whole.begin() + first.size()));
}
TEST_CASE("Downsampled repeated cue keeps its source-sample seam") {
    auto scene = makeDocument();
    const auto clip = importPcm16Wav(scene, "96 kHz cue", wav(96000, 2002, 96000), 0);
    setAudioClipRepeats(scene, clip, 2);
    AudioMixPlan mix(scene, 48000);
    const auto first = mix.renderBlock(1001, 1)[0];
    const auto second = mix.renderBlock(49001, 1)[0];
    REQUIRE(first > 10000);
    REQUIRE(second == first);
    REQUIRE(mix.renderBlock(49000, 1)[0] < second);
}

TEST_CASE("Upsampling retains upper source passband and stays block independent") {
    auto scene = makeDocument();
    (void)importPcm16Wav(scene, "8 kHz 3 kHz tone", toneWav(8000, 8000, 3000), 0);
    AudioMixPlan mix(scene, 48000);
    const auto interior = mix.renderBlock(480, 3840);
    double energy = 0;
    for (std::size_t i = 0; i < interior.size(); i += 2) {
        const double sample = interior[i] / 32768.0;
        energy += sample * sample;
    }
    const double rms = std::sqrt(energy / (interior.size() / 2));
    std::cout << "8-to-48 kHz retained 3 kHz RMS: " << rms << '\n';
    REQUIRE(rms > .43);
    REQUIRE(rms < .6);
    const auto whole = mix.renderBlock(0, 4800);
    const auto first = mix.renderBlock(0, 1024);
    const auto second = mix.renderBlock(1024, 3776);
    REQUIRE(std::equal(first.begin(), first.end(), whole.begin()));
    REQUIRE(std::equal(second.begin(), second.end(), whole.begin() + first.size()));
}

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
TEST_CASE("Audio clip visible end uses exact rational sample bounds") {
    auto scene = makeDocument();
    const auto id = importPcm16Wav(scene, "one second", wav(48000, 2002), 4);
    auto& clip = scene.audioClips.front();
    const auto& asset = scene.audioAssets.front();
    REQUIRE(clip.id == id);
    REQUIRE(audioClipEndFrame(scene, clip, asset) == 28);
    trimAudioClip(scene, id, 2002, 4000);
    REQUIRE(audioClipEndFrame(scene, clip, asset) == 5);
    scene.rate = {24000, 1001};
    trimAudioClip(scene, id, 0, 48000);
    REQUIRE(audioClipEndFrame(scene, clip, asset) == 28);
    trimAudioClip(scene, id, 0, 2003);
    REQUIRE(audioClipEndFrame(scene, clip, asset) == 6);
    moveAudioClip(scene, id, 47);
    REQUIRE(audioClipEndFrame(scene, clip, asset) == 48);
}
TEST_CASE("Repeated trimmed audio stays sample-contiguous through mix and format migration") {
    auto scene = makeDocument();
    const auto id = importPcm16Wav(scene, "repeat cue", wav(48000, 2002), 0);
    trimAudioClip(scene, id, 0, 4000);
    setAudioClipRepeats(scene, id, 3);
    REQUIRE(scene.audioClips.front().repeats == 3);
    REQUIRE(audioClipEndFrame(scene, scene.audioClips.front(), scene.audioAssets.front()) == 6);
    scene.validate();
    AudioMixPlan mix(scene, 48000);
    for (auto cue : {2002, 6002, 10002}) {
        REQUIRE(mix.renderBlock(cue - 1, 1)[0] == 0);
        REQUIRE(mix.renderBlock(cue, 1)[0] == 32767);
    }
    AudioPeakIndex peaks(scene.audioAssets.front());
    REQUIRE(audioClipFramePeak(scene, scene.audioClips.front(), scene.audioAssets.front(), peaks, 1) > .99);
    REQUIRE(audioClipFramePeak(scene, scene.audioClips.front(), scene.audioAssets.front(), peaks, 3) > .99);
    REQUIRE(audioClipFramePeak(scene, scene.audioClips.front(), scene.audioAssets.front(), peaks, 5) > .99);
    REQUIRE(deserializeDocument(serializeDocument(scene)) == scene);
    auto previous = nlohmann::json::parse(serializeDocument(scene));
    previous["version"] = 16;
    for (auto& clip : previous["audioClips"])
        clip.erase("repeats");
    REQUIRE(deserializeDocument(previous.dump()).audioClips.front().repeats == 1);
    Session session;
    session.replace(scene);
    REQUIRE(session.apply("Reduce repeats", [&](Document& d) { setAudioClipRepeats(d, id, 1); }));
    REQUIRE(session.undo());
    REQUIRE(session.document() == scene);
    REQUIRE_THROWS(session.apply("Invalid repeats", [&](Document& d) { setAudioClipRepeats(d, id, 65); }));
    REQUIRE(session.document() == scene);
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
TEST_CASE("Waveform peak pyramid matches exact PCM edges at every queried zoom interval") {
    auto d = makeDocument();
    auto source = wav(8192, 257);
    const auto clipId = importPcm16Wav(d, "boundary cue", std::move(source), 0);
    const auto& asset = d.audioAssets.front();
    REQUIRE(d.audioClips.front().id == clipId);
    AudioPeakIndex index(asset);
    REQUIRE(index.matches(asset));
    REQUIRE(index.peak(256, 257) == 0);
    REQUIRE(index.peak(257, 258) > 0.99);
    REQUIRE(index.peak(258, 512) == 0);
    for (std::uint64_t query = 0; query < 300; ++query) {
        const auto begin = (query * 131) % 8192;
        const auto end = std::min<std::uint64_t>(8192, begin + (query * 197) % 2048);
        REQUIRE(index.peak(begin, end) == audioPeak(asset, begin, end));
    }
    REQUIRE_THROWS(index.peak(8000, 8200));
    std::vector<std::uint8_t> stereo{'R', 'I', 'F', 'F'};
    write32(stereo, 36 + 1024 * 4);
    stereo.insert(stereo.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    write32(stereo, 16); write16(stereo, 1); write16(stereo, 2);
    write32(stereo, 48000); write32(stereo, 48000 * 4);
    write16(stereo, 4); write16(stereo, 16);
    stereo.insert(stereo.end(), {'d', 'a', 't', 'a'});
    write32(stereo, 1024 * 4);
    for (int sample = 0; sample < 1024; ++sample) {
        write16(stereo, 0);
        write16(stereo, sample == 256 ? 32768 : 0);
    }
    (void)importPcm16Wav(d, "right-only", std::move(stereo), 0);
    AudioPeakIndex stereoIndex(d.audioAssets.back());
    REQUIRE(stereoIndex.peak(256, 257) == 1);
    REQUIRE(stereoIndex.peak(257, 1024) == 0);
}
TEST_CASE("Waveform index reuses a long source across repeated timeline pans") {
    auto scene = makeDocument();
    (void)importPcm16Wav(scene, "long source", wav(48000 * 30, 48000 * 20), 0);
    const auto& source = scene.audioAssets.front();
    const auto startBuild = std::chrono::steady_clock::now();
    AudioPeakIndex index(source);
    const auto endBuild = std::chrono::steady_clock::now();
    double indexedSum = 0, directSum = 0;
    const auto indexedStart = std::chrono::steady_clock::now();
    for (int query = 0; query < 1000; ++query) {
        const auto begin = std::uint64_t((query * 1289) % (48000 * 29));
        indexedSum += index.peak(begin, begin + 2000);
    }
    const auto indexedEnd = std::chrono::steady_clock::now();
    for (int query = 0; query < 1000; ++query) {
        const auto begin = std::uint64_t((query * 1289) % (48000 * 29));
        directSum += audioPeak(source, begin, begin + 2000);
    }
    const auto directEnd = std::chrono::steady_clock::now();
    const auto ms = [](auto duration) {
        return std::chrono::duration<double, std::milli>(duration).count();
    };
    std::cout << "Waveform index build: " << ms(endBuild - startBuild)
              << " ms; 1000 indexed queries: " << ms(indexedEnd - indexedStart)
              << " ms; direct queries: " << ms(directEnd - indexedEnd) << " ms\n";
    REQUIRE(indexedSum == directSum);
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
    REQUIRE(block[4] > 30000); // Reconstructed 44.1 kHz impulse peaks at the rational cue.
    REQUIRE(block[4] == block[5]);
    REQUIRE(block[6] > 0);
    REQUIRE(block[6] < block[4]);
    REQUIRE(block[6] == block[7]);
    REQUIRE_THROWS(mix.renderBlock(95999, 2));
    auto fractional = d;
    fractional.rate = {24000, 1001};
    fractional.validate();
    AudioMixPlan fractionalMix(fractional, 48000);
    REQUIRE(fractionalMix.sceneSamples() == 96096);
    const auto fractionalCue = fractionalMix.renderBlock(2002, 1);
    REQUIRE(fractionalCue[0] > 14000);
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
TEST_CASE("Two downsampled tracks fit the realtime callback period") {
    auto scene = makeDocument();
    const auto source = toneWav(96000, 96000, 1000);
    const auto first = importPcm16Wav(scene, "96 kHz A", source, 0);
    const auto second = importPcm16Wav(scene, "96 kHz B", source, 0);
    setAudioClipRepeats(scene, first, 2);
    setAudioClipRepeats(scene, second, 2);
    AudioMixPlan mix(scene, 48000);
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
    std::cout << "Two 96-to-48 kHz downsampled tracks p95: " << p95
              << " ms; callback period: " << (1000.0 * 1024 / 48000) << " ms\n";
    REQUIRE(p95 < 1000.0 * 1024 / 48000);
}
