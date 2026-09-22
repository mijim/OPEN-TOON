#include "opentoon/document.h"
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
#include <cstring>
#include <filesystem>
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
} // namespace

TEST_CASE("Original registered character parts survive format-3 save and reopen") {
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
