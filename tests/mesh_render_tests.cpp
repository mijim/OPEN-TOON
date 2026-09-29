#include "opentoon/deformation.h"
#include "opentoon/rigging.h"
#include "mesh_warp.h"
#include "graph_renderer.h"
#include "scene_renderer.h"
#include "serialization.h"
#include <catch2/catch_test_macros.hpp>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <chrono>
#include <cstdio>

using namespace opentoon;

namespace {
struct MeshScene {
    Document document = makeDocument();
    Id part = document.layers.front().id;
    Id root = makeCharacter(document, part, "Mesh test");
    Id drawing = createSubstitution(document, part, 0, false, "Front");
    MeshScene() {
        document.width = document.height = 32;
        document.background = {0, 0, 0, 0};
        std::vector<std::uint8_t> pixels(16 * 16 * 4);
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x) {
                const auto i = std::size_t(y * 16 + x) * 4;
                pixels[i] = x < 8 ? 255 : 0;
                pixels[i + 2] = x >= 8 ? 255 : 0;
                pixels[i + 3] = 255;
            }
        document.drawings.at(drawing).image = ImageAsset{16, 16, std::move(pixels)};
        document.validate();
    }
};
} // namespace

TEST_CASE("Rest mesh is pixel-identical; posed mesh renders through save, reopen and both outputs") {
    MeshScene fixture;
    const auto original = SceneRenderer::render(fixture.document, 0);
    bindRegularImageMesh(fixture.document, fixture.part, fixture.drawing, 2, 2);
    REQUIRE(SceneRenderer::render(fixture.document, 0) == original);
    moveMeshPoseVertex(fixture.document, fixture.part, fixture.drawing, 2, {20, 0});
    fixture.document.validate();
    const auto posed = SceneRenderer::render(fixture.document, 0);
    REQUIRE(posed != original);
    REQUIRE(qAlpha(original.pixel(17, 2)) == 0);
    REQUIRE(qAlpha(posed.pixel(17, 2)) == 255);
    REQUIRE(qBlue(posed.pixel(17, 2)) == 255);
    const auto reopened = deserializeDocument(serializeDocument(fixture.document));
    REQUIRE(SceneRenderer::render(reopened, 0) == posed);
    fixture.document.composition = CompositionProfile::LinearSrgb;
    const auto graph = CompositionGraph::orderedLayers(fixture.document);
    const auto display = GraphRenderer::render(graph, fixture.document, 0, {}, {},
                                               GraphTarget::Display);
    const auto write = GraphRenderer::render(graph, fixture.document, 0, {}, {},
                                             GraphTarget::Write);
    REQUIRE(display == write);
    REQUIRE(display.pixel(17, 2) == posed.pixel(17, 2));
    resetMeshPose(fixture.document, fixture.part, fixture.drawing);
    REQUIRE(SceneRenderer::render(fixture.document, 0) == original);
}

TEST_CASE("Edited rest shape renders its authored UV map and reset does not accumulate distortion") {
    MeshScene fixture;
    bindRegularImageMesh(fixture.document, fixture.part, fixture.drawing, 2, 2);
    const auto original = SceneRenderer::render(fixture.document, 0);
    moveMeshRestVertex(fixture.document, fixture.part, fixture.drawing, 2, {20, 0});
    const auto rest = SceneRenderer::render(fixture.document, 0);
    REQUIRE(rest != original);
    REQUIRE(qAlpha(rest.pixel(17, 2)) == 255);
    moveMeshPoseVertex(fixture.document, fixture.part, fixture.drawing, 2, {21, 0});
    REQUIRE(SceneRenderer::render(fixture.document, 0) != rest);
    resetMeshPose(fixture.document, fixture.part, fixture.drawing);
    REQUIRE(SceneRenderer::render(fixture.document, 0) == rest);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(fixture.document)), 0) == rest);
}

TEST_CASE("Regular binding crops registered transparent canvas without changing rest pixels") {
    MeshScene fixture;
    auto& image = *fixture.document.drawings.at(fixture.drawing).image;
    std::vector<std::uint8_t> sparse(16 * 16 * 4);
    for (int y = 4; y < 12; ++y)
        for (int x = 5; x < 10; ++x) {
            const auto i = std::size_t(y * 16 + x) * 4;
            sparse[i] = 255;
            sparse[i + 3] = 255;
        }
    image = ImageAsset{16, 16, std::move(sparse)};
    const auto original = SceneRenderer::render(fixture.document, 0);
    bindRegularImageMesh(fixture.document, fixture.part, fixture.drawing, 2, 2);
    const auto& binding = *meshBindingFor(fixture.document.layer(fixture.part), fixture.drawing);
    REQUIRE(binding.vertices.front().rest == MeshPoint{5, 4});
    REQUIRE(binding.vertices.back().rest == MeshPoint{10, 12});
    REQUIRE(SceneRenderer::render(fixture.document, 0) == original);
    moveMeshPoseVertex(fixture.document, fixture.part, fixture.drawing, 2, {12, 4});
    const auto warped = warpMeshImage(image, binding);
    REQUIRE(warped.pixels.width() == 7);
    REQUIRE(warped.pixels.height() == 8);
    REQUIRE(warped.origin == QPoint(5, 4));
    REQUIRE(SceneRenderer::render(fixture.document, 0) != original);
}

TEST_CASE("Vector source remains editable while a bounded raster proxy deforms it") {
    auto document = makeDocument();
    document.width = document.height = 64;
    document.background = {0, 0, 0, 0};
    const Id part = document.layers.front().id;
    auto& drawing = document.editableDrawing(part, 0);
    const Id drawingId = drawing.id;
    drawing.strokes.push_back({document.allocateId(), document.palette.front().id, 2,
                               Shape::Rectangle, true, 1,
                               {{16, 16, 1}, {40, 40, 1}}});
    makeCharacter(document, part, "Vector test");
    document.validate();
    const auto original = SceneRenderer::render(document, 0);
    bindRegularVectorMesh(document, part, drawingId, 2, 2);
    REQUIRE(document.drawings.at(drawingId).strokes.size() == 1);
    REQUIRE(SceneRenderer::render(document, 0) == original);
    moveMeshPoseVertex(document, part, drawingId, 2, {44, 15});
    REQUIRE(SceneRenderer::layerInkBounds(document, document.layer(part), 0, {64, 64}).right() >= 44);
    const auto posed = SceneRenderer::render(document, 0);
    REQUIRE(posed != original);
    const auto strokeId = document.drawings.at(drawingId).strokes.front().id;
    document.drawings.at(drawingId).strokes.front().points.back().x = 36;
    document.validate();
    REQUIRE(document.drawings.at(drawingId).strokes.front().id == strokeId);
    REQUIRE(SceneRenderer::render(document, 0) != posed);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(document)), 0) ==
            SceneRenderer::render(document, 0));
    auto outOfProxy = document;
    outOfProxy.drawings.at(drawingId).strokes.front().points.front().x = -100;
    REQUIRE_THROWS(outOfProxy.validate());
}

TEST_CASE("Switching substitutions selects only that drawing's mesh binding") {
    MeshScene fixture;
    bindRegularImageMesh(fixture.document, fixture.part, fixture.drawing, 2, 2);
    moveMeshPoseVertex(fixture.document, fixture.part, fixture.drawing, 2, {20, 0});
    const auto front = SceneRenderer::render(fixture.document, 0);
    const Id side = createSubstitution(fixture.document, fixture.part, 0, true, "Side");
    fixture.document.drawings.at(side).image =
        ImageAsset{16, 16, std::vector<std::uint8_t>(16 * 16 * 4, 255)};
    fixture.document.validate();
    REQUIRE(meshBindingFor(fixture.document.layer(fixture.part), side) == nullptr);
    const auto unbound = SceneRenderer::render(fixture.document, 0);
    REQUIRE(unbound != front);
    bindRegularImageMesh(fixture.document, fixture.part, side, 2, 2);
    moveMeshPoseVertex(fixture.document, fixture.part, side, 2, {18, 0});
    const auto boundSide = SceneRenderer::render(fixture.document, 0);
    REQUIRE(boundSide != unbound);
    selectSubstitution(fixture.document, fixture.part, 0, fixture.drawing);
    REQUIRE(SceneRenderer::render(fixture.document, 0) == front);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(fixture.document)), 0) ==
            front);
}

TEST_CASE("Mesh edges have contiguous coverage and transparent texels do not leak color") {
    MeshScene fixture;
    bindRegularImageMesh(fixture.document, fixture.part, fixture.drawing, 2, 2);
    moveMeshPoseVertex(fixture.document, fixture.part, fixture.drawing, 4, {10, 8});
    const auto& binding = *meshBindingFor(fixture.document.layer(fixture.part), fixture.drawing);
    auto& image = *fixture.document.drawings.at(fixture.drawing).image;
    const auto opaque = warpMeshImage(image, binding);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
            REQUIRE(qAlpha(opaque.pixels.pixel(x, y)) == 255);
    std::vector<std::uint8_t> transparentPixels(16 * 16 * 4);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            const auto i = std::size_t(y * 16 + x) * 4;
            transparentPixels[i] = 255;
            if (x >= 8) {
                transparentPixels[i] = 0;
                transparentPixels[i + 2] = 255;
                transparentPixels[i + 3] = 255;
            }
        }
    image = ImageAsset{16, 16, std::move(transparentPixels)};
    const auto warped = warpMeshImage(image, binding);
    REQUIRE(warped.pixels.width() == 16);
    REQUIRE(warped.pixels.height() == 16);
    bool hasPartialCoverage = false;
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            const auto pixel = warped.pixels.pixel(x, y);
            REQUIRE(qRed(pixel) == 0);
            REQUIRE(qGreen(pixel) == 0);
            REQUIRE(qBlue(pixel) == qAlpha(pixel));
            hasPartialCoverage |= qAlpha(pixel) > 0 && qAlpha(pixel) < 255;
        }
    REQUIRE(hasPartialCoverage);
    REQUIRE_THROWS_AS(warpMeshImage(image, binding, [] { return true; }), RenderCancelled);
}

TEST_CASE("Mesh warp cost is measured on a representative opaque image") {
    MeshScene fixture;
    auto& image = *fixture.document.drawings.at(fixture.drawing).image;
    image = ImageAsset{512, 512, std::vector<std::uint8_t>(512 * 512 * 4, 255)};
    bindRegularImageMesh(fixture.document, fixture.part, fixture.drawing, 8, 8);
    moveMeshPoseVertex(fixture.document, fixture.part, fixture.drawing, 40, {256, 250});
    const auto& binding = *meshBindingFor(fixture.document.layer(fixture.part), fixture.drawing);
    const auto start = std::chrono::steady_clock::now();
    for (int run = 0; run < 3; ++run)
        REQUIRE_FALSE(warpMeshImage(image, binding).pixels.isNull());
    const auto elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count() / 3;
    std::fprintf(stderr, "HM-05 mesh warp 512x512/8x8: %.2f ms per render (%s)\n",
                 elapsed,
#ifdef NDEBUG
                 "release"
#else
                 "debug"
#endif
    );
}

TEST_CASE("Nineteen Harmony parts keep rest pixels and bounded posed render cost") {
    QFile specification(QStringLiteral(OPENTOON_SOURCE_DIR "/tests/fixtures/harmony-moment/shot.json"));
    REQUIRE(specification.open(QIODevice::ReadOnly));
    const auto shot = QJsonDocument::fromJson(specification.readAll()).object();
    const auto roles = shot.value("part_roles").toArray();
    REQUIRE(roles.size() == 19);
    auto document = makeDocument();
    document.width = 1920;
    document.height = 1080;
    document.background = {0, 0, 0, 0};
    std::vector<Id> parts;
    for (int index = 0; index < roles.size(); ++index) {
        const QString role = roles[index].toString();
        const QString variant = role == "mouth" ? "front__rest" :
                                (role == "head" || role == "hair" || role == "eyes") ? "front" :
                                role.startsWith("hand_") ? "open" : "base";
        const QImage source(QStringLiteral(OPENTOON_SOURCE_DIR "/tests/fixtures/harmony-moment/parts/") +
                            role + "__" + variant + ".png");
        REQUIRE_FALSE(source.isNull());
        const auto image = source.convertToFormat(QImage::Format_RGBA8888);
        Layer* layer = nullptr;
        if (index == 0)
            layer = &document.layers.front();
        else {
            Layer added;
            added.id = document.allocateId();
            added.name = role.toStdString();
            document.layers.push_back(added);
            layer = &document.layers.back();
        }
        const Id drawing = document.allocateId();
        layer->exposures.push_back({0, document.duration, drawing});
        Drawing created;
        created.id = drawing;
        document.drawings.emplace(drawing, std::move(created));
        layer->name = role.toStdString();
        auto& artwork = document.drawings.at(drawing);
        artwork.image = ImageAsset{image.width(), image.height(), {}};
        artwork.image->rgba.assign(image.constBits(), image.constBits() + image.sizeInBytes());
        parts.push_back(layer->id);
    }
    const Id root = makeCharacter(document, parts.front(), "Clockwork Hello");
    for (std::size_t index = 1; index < parts.size(); ++index)
        attachDrawingAsPart(document, parts[index], root, roles[int(index)].toString().toStdString());
    document.validate();
    const auto baseline = SceneRenderer::render(document, 0);
    for (const Id part : parts) {
        const Id drawing = document.layer(part).exposures.front().drawing;
        bindRegularImageMesh(document, part, drawing, 2, 2);
    }
    REQUIRE(SceneRenderer::render(document, 0) == baseline);
    for (const Id part : parts) {
        const Id drawing = document.layer(part).exposures.front().drawing;
        const auto& binding = *meshBindingFor(document.layer(part), drawing);
        const auto center = binding.vertices[4].pose;
        moveMeshPoseVertex(document, part, drawing, 4, {center.x + 1, center.y});
    }
    document.validate();
    const auto start = std::chrono::steady_clock::now();
    for (int run = 0; run < 3; ++run)
        REQUIRE_FALSE(SceneRenderer::render(document, 0).isNull());
    const auto elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count() / 3;
    std::fprintf(stderr, "HM-05 Harmony 19 posed parts 1920x1080: %.2f ms per frame (%s)\n",
                 elapsed,
#ifdef NDEBUG
                 "release"
#else
                 "debug"
#endif
    );
    const auto reopened = deserializeDocument(serializeDocument(document));
    REQUIRE(SceneRenderer::render(reopened, 0) == SceneRenderer::render(document, 0));
}
