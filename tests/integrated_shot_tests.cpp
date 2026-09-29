#include "opentoon/audio.h"
#include "opentoon/composition_graph.h"
#include "audio_wav_writer.h"
#include "project_store.h"
#include "scene_renderer.h"
#include <QBuffer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <vector>

using namespace opentoon;
namespace {
void put16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(std::uint8_t(value));
    bytes.push_back(std::uint8_t(value >> 8));
}
void put32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    put16(bytes, std::uint16_t(value));
    put16(bytes, std::uint16_t(value >> 16));
}
std::vector<std::uint8_t> syntheticCueTrack(const Document& scene,
                                             const QJsonArray& cueFrames) {
    constexpr std::uint32_t rate = 48000;
    const auto sampleCount = std::uint32_t(scene.rate.sampleAt(scene.duration, rate));
    std::vector<std::int16_t> samples(sampleCount);
    for (const auto& entry : cueFrames) {
        const auto first = std::size_t(scene.rate.sampleAt(entry.toInt(), rate));
        samples[first] = 12000; // Exact frame marker for deterministic alignment checks.
        for (std::size_t offset = 1; offset < 1920 && first + offset < samples.size(); ++offset) {
            const double t = double(offset) / rate;
            const double envelope = std::pow(std::sin(std::numbers::pi * offset / 1920), 2);
            const double tone = (std::sin(2 * std::numbers::pi * 440 * t) +
                                 .3 * std::sin(2 * std::numbers::pi * 660 * t)) / 1.3;
            samples[first + offset] = std::int16_t(std::lround(9000 * envelope * tone));
        }
    }
    std::vector<std::uint8_t> bytes{'R', 'I', 'F', 'F'};
    bytes.reserve(44 + samples.size() * 2);
    put32(bytes, 36 + sampleCount * 2);
    bytes.insert(bytes.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    put32(bytes, 16); put16(bytes, 1); put16(bytes, 1);
    put32(bytes, rate); put32(bytes, rate * 2);
    put16(bytes, 2); put16(bytes, 16);
    bytes.insert(bytes.end(), {'d', 'a', 't', 'a'});
    put32(bytes, sampleCount * 2);
    for (const auto sample : samples)
        put16(bytes, std::uint16_t(sample));
    return bytes;
}
Id role(const Document& scene, const char* name) {
    const auto found = std::find_if(scene.layers.begin(), scene.layers.end(),
                                    [name](const auto& layer) { return layer.role == name; });
    return found == scene.layers.end() ? 0 : found->id;
}
std::int16_t sampleAt(const QByteArray& wav, std::int64_t frame) {
    const auto at = 44 + frame * 4;
    const auto low = std::uint8_t(wav[int(at)]);
    const auto high = std::uint8_t(wav[int(at + 1)]);
    return std::int16_t(std::uint16_t(low | std::uint16_t(high) << 8));
}
} // namespace

TEST_CASE("Twenty-second toon joins mouth cues audio and painted eye cutter after reopen") {
    QFile briefFile(QStringLiteral(OPENTOON_SOURCE_DIR
        "/tests/fixtures/harmony-moment/shot.json"));
    REQUIRE(briefFile.open(QIODevice::ReadOnly));
    const auto brief = QJsonDocument::fromJson(briefFile.readAll()).object();
    const auto cues = brief.value("audio").toObject().value("cue_frames").toArray();
    REQUIRE(cues.size() == 9);
    auto scene = ProjectStore::load(std::filesystem::path(
        OPENTOON_SOURCE_DIR "/examples/clockwork-visual-shot.otoon")).document;
    REQUIRE(scene.duration == 480);
    REQUIRE(scene.rate.numerator == 24);
    const Id head = role(scene, "head"), eyes = role(scene, "eyes"), mouth = role(scene, "mouth");
    REQUIRE(head != 0);
    REQUIRE(eyes != 0);
    REQUIRE(mouth != 0);
    scene.name = "Clockwork Hello — integrated cue and cutter study";
    scene.layer(eyes).matte = head;
    scene.layer(head).paintMatteSource = true;
    const auto bytes = syntheticCueTrack(scene, cues);
    const Id cueClip = importPcm16Wav(scene, "Synthetic dialogue cues", bytes, 0);
    setAudioClipFades(scene, cueClip, 2000, 2000);
    scene.validate();
    const auto graph = CompositionGraph::orderedLayers(scene);
    REQUIRE_NOTHROW(graph.validate(scene));
    REQUIRE(std::count_if(graph.nodes.begin(), graph.nodes.end(), [](const auto& node) {
        return node.kind == GraphNodeKind::ApplyMatte;
    }) == 1);
    REQUIRE(scene.audioAssets.front().wav.values() == bytes);
    const AudioMixPlan mix(scene, 48000);
    REQUIRE(mix.sceneSamples() == 960000);
    for (const auto& entry : cues) {
        const Frame frame = entry.toInt();
        INFO(frame);
        const auto first = scene.rate.sampleAt(frame, 48000);
        REQUIRE(mix.renderBlock(first - 1, 1)[0] == 0);
        REQUIRE(mix.renderBlock(first, 1)[0] == 12000);
        REQUIRE(scene.drawingAt(mouth, frame) != nullptr);
        REQUIRE(scene.drawingAt(mouth, frame - 1) != scene.drawingAt(mouth, frame));
    }
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    const auto path = std::filesystem::path((temporary.path() + "/integrated.otoon").toStdString());
    REQUIRE(ProjectStore::save(path, scene) > 0);
    const auto reopened = ProjectStore::load(path).document;
    REQUIRE(reopened == scene);
    for (Frame frame : {0, 120, 240, 360, 479}) {
        const auto before = SceneRenderer::render(scene, frame, QSize(480, 270));
        REQUIRE(SceneRenderer::render(reopened, frame, QSize(480, 270)) == before);
    }
    QByteArray wav;
    QBuffer output(&wav);
    REQUIRE(output.open(QIODevice::WriteOnly));
    const auto result = writeAudioWavRange(reopened, output, 0, 480);
    REQUIRE(result.sampleFrames == 960000);
    REQUIRE(wav.size() == 44 + 960000 * 4);
    REQUIRE(sampleAt(wav, 0) == 0);
    for (const auto& entry : cues)
        REQUIRE(sampleAt(wav, reopened.rate.sampleAt(entry.toInt(), 48000)) == 12000);
    if (qEnvironmentVariableIsSet("OPENTOON_INTEGRATED_PROJECT")) {
        const auto outputPath = qEnvironmentVariable("OPENTOON_INTEGRATED_PROJECT");
        REQUIRE(ProjectStore::save(std::filesystem::path(outputPath.toStdString()), reopened) > 0);
    }
    const auto bundled = ProjectStore::load(std::filesystem::path(
        OPENTOON_SOURCE_DIR "/examples/clockwork-integrated-study.otoon")).document;
    REQUIRE(bundled == reopened);
}
