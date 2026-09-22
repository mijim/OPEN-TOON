#include "opentoon/document.h"
#include "opentoon/rigging.h"
#include "project_store.h"
#include "scene_renderer.h"
#include <QFile>
#include <QColorSpace>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <map>
#include <vector>

namespace {
const QString fixture = QStringLiteral(OPENTOON_SOURCE_DIR "/tests/fixtures/harmony-moment/");

QJsonObject shot() {
    QFile file(fixture + "shot.json");
    REQUIRE(file.open(QIODevice::ReadOnly));
    auto document = QJsonDocument::fromJson(file.readAll());
    REQUIRE(document.isObject());
    return document.object();
}

opentoon::Document makeRigidReference(const QJsonObject& spec) {
    opentoon::Document document;
    document.name = "Clockwork Hello — rigid baseline";
    document.duration = 480;
    document.rate = {24, 1};
    opentoon::Layer root;
    root.id = document.allocateId();
    root.name = "Character root";
    for (const auto& pose : spec.value("reference_key_poses").toArray()) {
        opentoon::Transform transform;
        transform.x = pose.toObject().value("root_dx_px").toDouble();
        root.keys.push_back({pose.toObject().value("frame").toInt(), transform});
    }
    document.layers.push_back(root);
    const auto centers = spec.value("reference_centers_px").toObject();
    for (const auto& part : spec.value("reference_paint_order").toArray()) {
        const auto role = part.toString();
        const QString variant = role == "mouth" ? "front__rest" :
                                (role == "head" || role == "hair" || role == "eyes") ? "front" :
                                role.startsWith("hand_") ? "open" : "base";
        QImage image(fixture + "parts/" + role + "__" + variant + ".png");
        REQUIRE_FALSE(image.isNull());
        REQUIRE(image.size() == QSize(256, 256));
        REQUIRE(image.colorSpace().isValid());
        image = image.convertToFormat(QImage::Format_RGBA8888);
        std::vector<std::uint8_t> bytes;
        bytes.reserve(256 * 256 * 4);
        for (int y = 0; y < image.height(); ++y) {
            const auto* row = image.constScanLine(y);
            bytes.insert(bytes.end(), row, row + image.width() * 4);
        }
        opentoon::Drawing drawing;
        drawing.id = document.allocateId();
        drawing.name = role.toStdString();
        drawing.image = opentoon::ImageAsset{256, 256, std::move(bytes)};
        const auto drawingId = drawing.id;
        document.drawings.emplace(drawingId, std::move(drawing));
        opentoon::Layer layer;
        layer.id = document.allocateId();
        layer.name = role.toStdString();
        layer.parent = root.id;
        const auto center = centers.value(role).toArray();
        REQUIRE(center.size() == 2);
        layer.transform.x = center.at(0).toDouble() - 128;
        layer.transform.y = center.at(1).toDouble() - 128;
        layer.exposures.push_back({0, document.duration, drawingId});
        document.layers.push_back(std::move(layer));
    }
    document.validate();
    return document;
}
opentoon::ImageAsset partImage(const QString& name) {
    QImage image(fixture + "parts/" + name + ".png");
    REQUIRE_FALSE(image.isNull());
    image = image.convertToFormat(QImage::Format_RGBA8888);
    std::vector<std::uint8_t> bytes;
    bytes.reserve(std::size_t(image.width()) * image.height() * 4);
    for (int y = 0; y < image.height(); ++y) {
        const auto* row = image.constScanLine(y);
        bytes.insert(bytes.end(), row, row + image.width() * 4);
    }
    return {image.width(), image.height(), std::move(bytes)};
}
} // namespace

TEST_CASE("Original registered character parts survive current-format save and reopen") {
    const auto spec = shot();
    auto document = makeRigidReference(spec);
    REQUIRE(document.layers.size() == 20);
    REQUIRE(document.drawings.size() == 19);
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    const auto path = std::filesystem::path((temporary.path() + "/clockwork-hello.otoon").toStdString());
    REQUIRE(opentoon::ProjectStore::save(path, document) == 1);
    auto reopened = opentoon::ProjectStore::load(path).document;
    REQUIRE(reopened == document);
    REQUIRE(opentoon::evaluateTransform(reopened.layers.front(), 240).x == 80);
    const auto actual = opentoon::SceneRenderer::render(reopened, 0);
    const auto expected = QImage(fixture + "reference_0000.png").convertToFormat(QImage::Format_ARGB32_Premultiplied);
    REQUIRE_FALSE(expected.isNull());
    REQUIRE(actual.size() == expected.size());
    REQUIRE(actual.format() == expected.format());
    for (int y = 0; y < actual.height(); ++y)
        REQUIRE(std::memcmp(actual.constScanLine(y), expected.constScanLine(y), actual.width() * 4) == 0);

    reopened.rate = {24000, 1001};
    REQUIRE(reopened.rate.sampleAt(480, 48000) == 960960);
    REQUIRE(opentoon::ProjectStore::save(path, reopened, "Fractional timing", 1) == 2);
    REQUIRE(opentoon::ProjectStore::load(path).document == reopened);
}

TEST_CASE("Nineteen-part character switches coordinated views and reopens with independent copy") {
    auto document = makeRigidReference(shot());
    const auto reference = QImage(fixture + "reference_0000.png")
                               .convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const auto original = opentoon::SceneRenderer::render(document, 0);
    for (int y = 0; y < original.height(); ++y)
        REQUIRE(std::memcmp(original.constScanLine(y), reference.constScanLine(y),
                            original.width() * 4) == 0);
    const auto root = document.layers.front().id;
    document.layer(root).kind = opentoon::LayerKind::Character;
    std::map<std::string, opentoon::Id> parts;
    for (std::size_t index = 1; index < document.layers.size(); ++index) {
        auto& layer = document.layers[index];
        layer.kind = opentoon::LayerKind::Part;
        layer.role = layer.name;
        layer.variants.push_back({layer.exposures.front().drawing, "Front"});
        parts.emplace(layer.role, layer.id);
    }
    const auto front = opentoon::captureCharacterView(document, root, 0, "Front");
    for (const auto& [role, file] : std::vector<std::pair<std::string, QString>>{
             {"head", "head__three_quarter"}, {"hair", "hair__three_quarter"},
             {"eyes", "eyes__three_quarter"}, {"mouth", "mouth__three_quarter__ah"}}) {
        const auto partId = parts.at(role);
        const auto drawing = opentoon::createSubstitution(document, partId, 120, false,
                                                           "Three-quarter " + role);
        document.drawings.at(drawing).image = partImage(file);
    }
    const auto side = opentoon::captureCharacterView(document, root, 120, "Three-quarter");
    REQUIRE(document.layer(root).views.front().choices.size() == 19);
    REQUIRE(document.layer(root).views.back().choices.size() == 19);
    opentoon::applyCharacterViewRange(document, root, front, 220, 240);
    REQUIRE(document.drawingAt(parts.at("head"), 219)->id !=
            document.drawingAt(parts.at("head"), 220)->id);
    REQUIRE(document.drawingAt(parts.at("head"), 240)->id !=
            document.drawingAt(parts.at("head"), 239)->id);
    const auto copy = opentoon::duplicateCharacter(document, root);
    const auto copiedHeadLayer = std::find_if(document.layers.begin(), document.layers.end(),
                                              [&](const opentoon::Layer& layer) {
                                                  return layer.role == "head" &&
                                                         layer.id != parts.at("head") &&
                                                         opentoon::characterFor(document, layer.id) == copy;
                                              });
    REQUIRE(copiedHeadLayer != document.layers.end());
    const auto copiedHead = copiedHeadLayer->id;
    REQUIRE(document.layer(copiedHead).kind == opentoon::LayerKind::Part);
    opentoon::applyCharacterView(document, copy, document.layer(copy).views.front().id, 250);
    const auto& sourceChoices = document.layer(root).views.back().choices;
    const auto sourceChoice = std::find_if(sourceChoices.begin(), sourceChoices.end(),
                                            [&](const opentoon::ViewChoice& choice) {
                                                return choice.part == parts.at("head");
                                            });
    const auto& copyChoices = document.layer(copy).views.front().choices;
    const auto copyChoice = std::find_if(copyChoices.begin(), copyChoices.end(),
                                          [&](const opentoon::ViewChoice& choice) {
                                              return choice.part == copiedHead;
                                          });
    REQUIRE(sourceChoice != sourceChoices.end());
    REQUIRE(copyChoice != copyChoices.end());
    const auto sideHead = sourceChoice->drawing;
    const auto copyFrontHead = copyChoice->drawing;
    REQUIRE(document.drawingAt(parts.at("head"), 250)->id == sideHead);
    REQUIRE(document.drawingAt(copiedHead, 250)->id == copyFrontHead);
    REQUIRE(document.layer(root).views.back().id == side);
    document.validate();
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    const auto path = std::filesystem::path((temporary.path() + "/rigged.otoon").toStdString());
    REQUIRE(opentoon::ProjectStore::save(path, document) == 1);
    const auto reopened = opentoon::ProjectStore::load(path).document;
    REQUIRE(reopened == document);
    REQUIRE(opentoon::SceneRenderer::render(reopened, 250) ==
            opentoon::SceneRenderer::render(document, 250));
}
