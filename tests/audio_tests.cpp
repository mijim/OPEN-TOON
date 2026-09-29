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

TEST_CASE("Audio clip duplication reuses its source and restores exact cues after reopen") {
    Session session;
    Id original = 0, copy = 0;
    REQUIRE(session.apply("Import cue", [&](Document& d) {
        original = importPcm16Wav(d, "shared cue", wav(48000, 2002), 0);
        trimAudioClip(d, original, 0, 24000);
        setAudioClipRepeats(d, original, 2);
        setAudioClipGain(d, original, .5);
        setAudioClipFades(d, original, 200, 200);
    }));
    const auto before = session.document();
    REQUIRE(session.apply("Duplicate cue", [&](Document& d) {
        copy = duplicateAudioClip(d, original, 24);
    }));
    const auto duplicated = session.document();
    REQUIRE(duplicated.audioAssets.size() == 1);
    REQUIRE(duplicated.audioClips.size() == 2);
    REQUIRE(duplicated.audioClips[1].asset == duplicated.audioClips[0].asset);
    REQUIRE(duplicated.audioClips[1].id == copy);
    REQUIRE(duplicated.audioClips[1].start == 24);
    REQUIRE(duplicated.audioClips[1].gain == .5);
    REQUIRE(duplicated.audioClips[1].repeats == 2);
    REQUIRE(duplicated.audioClips[1].fadeInSamples == 200);
    REQUIRE(duplicated.audioClips[1].fadeOutSamples == 200);
    const AudioMixPlan mix(duplicated, 48000);
    for (const auto cue : {2002, 26002, 50002, 74002}) {
        REQUIRE(mix.renderBlock(cue - 1, 1)[0] == 0);
        REQUIRE(mix.renderBlock(cue, 1)[0] == 16384);
    }
    REQUIRE(session.undo());
    REQUIRE(session.document() == before);
    REQUIRE(session.redo());
    REQUIRE(session.document() == duplicated);
    REQUIRE_THROWS(session.apply("Copy missing clip", [&](Document& d) {
        (void)duplicateAudioClip(d, copy + 100, 0);
    }));
    REQUIRE_THROWS(session.apply("Copy outside scene", [&](Document& d) {
        (void)duplicateAudioClip(d, original, d.duration);
    }));
    REQUIRE(session.document() == duplicated);
    const auto directory = std::filesystem::temp_directory_path() /
        ("opentoon-audio-copy-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    const auto path = directory / "shared-cue.otoon";
    REQUIRE(ProjectStore::save(path, duplicated) > 0);
    REQUIRE(ProjectStore::load(path).document == duplicated);
    std::filesystem::remove_all(directory);
}
TEST_CASE("Frame-aligned clip split preserves the exact 48 kHz mix and original WAV") {
    for (const auto rate : {FrameRate{24, 1}, FrameRate{24000, 1001}}) {
        Session session;
        Id first = 0, second = 0;
        const auto bytes = toneWav(48000, 48000, 440);
        REQUIRE(session.apply("Place source", [&](Document& d) {
            d.rate = rate;
            first = importPcm16Wav(d, "continuous tone", bytes, 0);
            setAudioClipGain(d, first, .7);
            setAudioClipFades(d, first, 1000, 1000);
        }));
        const auto before = session.document();
        const AudioMixPlan beforeMix(before, 48000);
        const auto beginning = beforeMix.renderBlock(0, 48000);
        const auto ending = beforeMix.renderBlock(48000,
            std::size_t(beforeMix.sceneSamples() - 48000));
        REQUIRE(session.apply("Split at frame 12", [&](Document& d) {
            second = splitAudioClipAtFrame(d, first, 12);
        }));
        const auto divided = session.document();
        REQUIRE(divided.audioAssets.size() == 1);
        REQUIRE(divided.audioAssets.front().wav.values() == bytes);
        REQUIRE(divided.audioClips.size() == 2);
        REQUIRE(divided.audioClips.back().id == second);
        REQUIRE(divided.audioClips.back().asset == divided.audioClips.front().asset);
        REQUIRE(divided.audioClips.back().start == 12);
        REQUIRE(divided.audioClips.front().outSample == divided.audioClips.back().inSample);
        REQUIRE(divided.audioClips.front().fadeInSamples == 1000);
        REQUIRE(divided.audioClips.front().fadeOutSamples == 0);
        REQUIRE(divided.audioClips.back().fadeInSamples == 0);
        REQUIRE(divided.audioClips.back().fadeOutSamples == 1000);
        const AudioMixPlan afterMix(divided, 48000);
        REQUIRE(afterMix.renderBlock(0, 48000) == beginning);
        REQUIRE(afterMix.renderBlock(48000,
            std::size_t(afterMix.sceneSamples() - 48000)) == ending);
        REQUIRE(session.undo());
        REQUIRE(session.document() == before);
        REQUIRE(session.redo());
        REQUIRE(session.document() == divided);
        REQUIRE_THROWS(session.apply("Split at start", [&](Document& d) {
            (void)splitAudioClipAtFrame(d, first, 0);
        }));
        REQUIRE(session.document() == divided);
        auto repeated = before;
        setAudioClipRepeats(repeated, first, 2);
        REQUIRE_THROWS(splitAudioClipAtFrame(repeated, first, 12));
        auto faded = before;
        setAudioClipFades(faded, first, 30000, 0);
        REQUIRE_THROWS(splitAudioClipAtFrame(faded, first, 12));
        const auto directory = std::filesystem::temp_directory_path() /
            ("opentoon-audio-split-" + std::to_string(rate.denominator) + "-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(directory);
        const auto path = directory / "split.otoon";
        REQUIRE(ProjectStore::save(path, divided) > 0);
        REQUIRE(ProjectStore::load(path).document == divided);
        std::filesystem::remove_all(directory);
    }
    auto differentRate = makeDocument();
    const Id clip = importPcm16Wav(differentRate, "44.1 kHz", toneWav(44100, 44100, 440), 0);
    REQUIRE_THROWS(splitAudioClipAtFrame(differentRate, clip, 12));
}

TEST_CASE("Muting one shared audio clip removes only its mixed samples and reopens") {
    Session session;
    Id first = 0, second = 0;
    const auto bytes = wav(48000, 2002);
    REQUIRE(session.apply("Place two cues", [&](Document& d) {
        first = importPcm16Wav(d, "original cue", bytes, 0);
        setAudioClipGain(d, first, .5);
        second = duplicateAudioClip(d, first, 0);
    }));
    const auto paired = session.document();
    REQUIRE(AudioMixPlan(paired, 48000).renderBlock(2002, 1)[0] == 32767);
    REQUIRE(session.apply("Mute first cue", [&](Document& d) {
        setAudioClipMuted(d, first, true);
    }));
    const auto oneMuted = session.document();
    REQUIRE(oneMuted.audioClips.front().muted);
    REQUIRE_FALSE(oneMuted.audioClips.back().muted);
    REQUIRE(oneMuted.audioAssets.front().wav.values() == bytes);
    REQUIRE(AudioMixPlan(oneMuted, 48000).renderBlock(2002, 1)[0] == 16384);
    REQUIRE(session.undo());
    REQUIRE(session.document() == paired);
    REQUIRE(session.redo());
    REQUIRE(session.document() == oneMuted);
    REQUIRE(session.apply("Mute second cue", [&](Document& d) {
        setAudioClipMuted(d, second, true);
    }));
    REQUIRE(AudioMixPlan(session.document(), 48000).renderBlock(2002, 1)[0] == 0);
    REQUIRE(session.apply("Unmute first cue", [&](Document& d) {
        setAudioClipMuted(d, first, false);
    }));
    REQUIRE(AudioMixPlan(session.document(), 48000).renderBlock(2002, 1)[0] == 16384);
    auto independentCopy = session.document();
    const Id third = duplicateAudioClip(independentCopy, second, 24);
    REQUIRE(independentCopy.audioClips.back().muted);
    setAudioClipMuted(independentCopy, third, false);
    REQUIRE_FALSE(independentCopy.audioClips.back().muted);
    REQUIRE(independentCopy.audioClips[1].muted);
    REQUIRE(deserializeDocument(serializeDocument(session.document())) == session.document());
}

TEST_CASE("Soloed audio clips isolate their mix while mute still takes precedence") {
    Session session;
    Id first = 0, second = 0;
    REQUIRE(session.apply("Place independent cues", [&](Document& d) {
        first = importPcm16Wav(d, "first cue", wav(48000, 2002), 0);
        second = importPcm16Wav(d, "second cue", wav(48000, 4004), 0);
    }));
    const auto original = session.document();
    REQUIRE(AudioMixPlan(original, 48000).renderBlock(2002, 1)[0] == 32767);
    REQUIRE(AudioMixPlan(original, 48000).renderBlock(4004, 1)[0] == 32767);
    REQUIRE(session.apply("Solo first", [&](Document& d) {
        setAudioClipSolo(d, first, true);
    }));
    const auto firstSolo = session.document();
    REQUIRE(AudioMixPlan(firstSolo, 48000).renderBlock(2002, 1)[0] == 32767);
    REQUIRE(AudioMixPlan(firstSolo, 48000).renderBlock(4004, 1)[0] == 0);
    REQUIRE(session.apply("Solo second", [&](Document& d) {
        setAudioClipSolo(d, second, true);
    }));
    REQUIRE(AudioMixPlan(session.document(), 48000).renderBlock(4004, 1)[0] == 32767);
    REQUIRE(session.apply("Mute first solo", [&](Document& d) {
        setAudioClipMuted(d, first, true);
    }));
    const auto mutedSolo = session.document();
    REQUIRE(AudioMixPlan(mutedSolo, 48000).renderBlock(2002, 1)[0] == 0);
    REQUIRE(AudioMixPlan(mutedSolo, 48000).renderBlock(4004, 1)[0] == 32767);
    REQUIRE(session.undo());
    REQUIRE(session.undo());
    REQUIRE(session.document() == firstSolo);
    REQUIRE(session.redo());
    REQUIRE(session.redo());
    REQUIRE(session.document() == mutedSolo);
    REQUIRE_THROWS(session.apply("Solo missing clip", [&](Document& d) {
        setAudioClipSolo(d, second + 100, true);
    }));
    REQUIRE(session.document() == mutedSolo);
    const auto directory = std::filesystem::temp_directory_path() /
        ("opentoon-audio-solo-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    const auto path = directory / "solo-cues.otoon";
    REQUIRE(ProjectStore::save(path, mutedSolo) > 0);
    const auto reopened = ProjectStore::load(path).document;
    REQUIRE(reopened == mutedSolo);
    REQUIRE(AudioMixPlan(reopened, 48000).renderBlock(2002, 1)[0] == 0);
    REQUIRE(AudioMixPlan(reopened, 48000).renderBlock(4004, 1)[0] == 32767);
    std::filesystem::remove_all(directory);
}

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
TEST_CASE("Source-sample fades shape repeated PCM without changing its bytes") {
    Session session;
    auto source = wav(8, 8, 8000);
    for (int sample = 0; sample < 8; ++sample) {
        source[44 + sample * 2] = 0;
        source[45 + sample * 2] = 64; // 16384, constant mono PCM.
    }
    Id id = 0;
    REQUIRE(session.apply("Import cue", [&](Document& d) {
        id = importPcm16Wav(d, "constant", source, 0);
    }));
    const auto baseline = session.document();
    REQUIRE(session.apply("Fade cue", [&](Document& d) {
        setAudioClipFades(d, id, 3, 3);
    }));
    const auto faded = session.document();
    REQUIRE(faded.audioAssets.front().wav.values() == source);
    const auto sample = [&](const Document& d, int at) {
        return AudioMixPlan(d, 8000).renderBlock(at, 1)[0];
    };
    REQUIRE(sample(faded, 0) == 0);
    REQUIRE(sample(faded, 1) == 8192);
    REQUIRE(sample(faded, 2) == 16384);
    REQUIRE(sample(faded, 5) == 16384);
    REQUIRE(sample(faded, 6) == 8192);
    REQUIRE(sample(faded, 7) == 0);
    REQUIRE(session.undo());
    REQUIRE(session.document() == baseline);
    REQUIRE(session.redo());
    REQUIRE(session.document() == faded);
    REQUIRE_THROWS(session.apply("Overlong fade", [&](Document& d) {
        setAudioClipFades(d, id, 5, 4);
    }));
    REQUIRE(session.document() == faded);
    REQUIRE(session.apply("Repeat cue", [&](Document& d) { setAudioClipRepeats(d, id, 2); }));
    REQUIRE(sample(session.document(), 7) == 16384);
    REQUIRE(sample(session.document(), 8) == 16384);
    REQUIRE(sample(session.document(), 15) == 0);
    REQUIRE(session.apply("Trim cue", [&](Document& d) { trimAudioClip(d, id, 2, 4); }));
    REQUIRE(session.document().audioClips.front().fadeInSamples == 3);
    REQUIRE(session.document().audioClips.front().fadeOutSamples == 1);
    REQUIRE(deserializeDocument(serializeDocument(session.document())) == session.document());
    auto previous = nlohmann::json::parse(serializeDocument(faded));
    previous["version"] = 21;
    for (auto& clip : previous["audioClips"]) {
        clip.erase("fadeInSamples");
        clip.erase("fadeOutSamples");
    }
    const auto loaded = deserializeDocument(previous.dump());
    REQUIRE(loaded.audioClips.front().fadeInSamples == 0);
    REQUIRE(loaded.audioClips.front().fadeOutSamples == 0);
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
