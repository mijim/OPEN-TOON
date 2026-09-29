#include "opentoon/animation.h"
#include "opentoon/deformation.h"
#include "opentoon/deformer.h"
#include "opentoon/rigging.h"
#include "opentoon/session.h"
#include "opentoon/timeline.h"
#include "mesh_warp.h"
#include "graph_renderer.h"
#include "project_store.h"
#include "scene_renderer.h"
#include "serialization.h"
#include <catch2/catch_test_macros.hpp>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <numbers>

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
std::pair<int, int> connectedInk(const QImage& frame) {
    const int width = frame.width(), height = frame.height();
    std::vector<std::uint8_t> visited(std::size_t(width) * height);
    int total = 0, largest = 0;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            if (frame.pixel(x, y) == qRgb(255, 255, 255))
                continue;
            ++total;
            const int start = y * width + x;
            if (visited[std::size_t(start)])
                continue;
            std::vector<int> pending{start};
            visited[std::size_t(start)] = 1;
            int size = 0;
            while (!pending.empty()) {
                const int current = pending.back();
                pending.pop_back();
                ++size;
                const int cx = current % width, cy = current / width;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = cx + dx, ny = cy + dy;
                        if (nx < 0 || nx >= width || ny < 0 || ny >= height)
                            continue;
                        const int next = ny * width + nx;
                        if (!visited[std::size_t(next)] &&
                            frame.pixel(nx, ny) != qRgb(255, 255, 255)) {
                            visited[std::size_t(next)] = 1;
                            pending.push_back(next);
                        }
                    }
            }
            largest = std::max(largest, size);
        }
    return {largest, total};
}
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

TEST_CASE("Keyed bone and curve poses use the same saved pixels in preview and output") {
    MeshScene fixture;
    bindRegularImageMesh(fixture.document, fixture.part, fixture.drawing, 4, 4);
    const auto rest = SceneRenderer::render(fixture.document, 0);
    bindBoneChain(fixture.document, fixture.part, fixture.drawing,
                  {{{0, 8}, {8, 8}, {16, 8}}}, 3);
    recordBonePose(fixture.document, fixture.part, fixture.drawing, 12, 0, 30);
    REQUIRE(SceneRenderer::render(fixture.document, 0) == rest);
    const auto boneFrame = SceneRenderer::render(fixture.document, 12);
    REQUIRE(boneFrame != rest);
    const auto reopenedBone = deserializeDocument(serializeDocument(fixture.document));
    REQUIRE(SceneRenderer::render(reopenedBone, 12) == boneFrame);
    fixture.document.composition = CompositionProfile::LinearSrgb;
    const auto graph = CompositionGraph::orderedLayers(fixture.document);
    REQUIRE(GraphRenderer::render(graph, fixture.document, 12, {}, {}, GraphTarget::Display) ==
            GraphRenderer::render(graph, fixture.document, 12, {}, {}, GraphTarget::Write));
    removeMeshDeformer(fixture.document, fixture.part, fixture.drawing);
    const std::array<MeshPoint, 4> straight{{{0, 8}, {16.0 / 3, 8},
                                            {32.0 / 3, 8}, {16, 8}}};
    bindCurveDeformer(fixture.document, fixture.part, fixture.drawing, straight);
    auto bent = straight;
    bent[1].y = 4;
    bent[2].y = 11;
    recordCurvePose(fixture.document, fixture.part, fixture.drawing, 12, bent);
    REQUIRE(SceneRenderer::render(fixture.document, 0) == rest);
    const auto curveFrame = SceneRenderer::render(fixture.document, 12);
    REQUIRE(curveFrame != rest);
    REQUIRE(curveFrame != boneFrame);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(fixture.document)), 12) ==
            curveFrame);
}

TEST_CASE("Animated mesh controls stay with their substitution through switching and reopen") {
    MeshScene fixture;
    bindRegularImageMesh(fixture.document, fixture.part, fixture.drawing, 4, 2);
    bindBoneChain(fixture.document, fixture.part, fixture.drawing,
                  {{{0, 8}, {8, 8}, {16, 8}}}, 3);
    recordBonePose(fixture.document, fixture.part, fixture.drawing, 0, 0, 20);
    const auto front = SceneRenderer::render(fixture.document, 0);
    const Id side = createSubstitution(fixture.document, fixture.part, 0, true, "Side");
    bindRegularImageMesh(fixture.document, fixture.part, side, 4, 2);
    const std::array<MeshPoint, 4> straight{{{0, 8}, {16.0 / 3, 8},
                                            {32.0 / 3, 8}, {16, 8}}};
    bindCurveDeformer(fixture.document, fixture.part, side, straight);
    auto curved = straight;
    curved[1].y += 3;
    recordCurvePose(fixture.document, fixture.part, side, 0, curved);
    fixture.document.validate();
    const auto sideFrame = SceneRenderer::render(fixture.document, 0);
    REQUIRE(sideFrame != front);
    REQUIRE(meshBindingFor(fixture.document.layer(fixture.part), fixture.drawing)->bone.has_value());
    REQUIRE(meshBindingFor(fixture.document.layer(fixture.part), side)->curve.has_value());
    selectSubstitution(fixture.document, fixture.part, 0, fixture.drawing);
    REQUIRE(SceneRenderer::render(fixture.document, 0) == front);
    auto reopened = deserializeDocument(serializeDocument(fixture.document));
    REQUIRE(SceneRenderer::render(reopened, 0) == front);
    selectSubstitution(reopened, fixture.part, 0, side);
    REQUIRE(SceneRenderer::render(reopened, 0) == sideFrame);
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
    const auto roles = shot.value("reference_paint_order").toArray();
    const auto centers = shot.value("reference_centers_px").toObject();
    REQUIRE(roles.size() == 19);
    auto document = makeDocument();
    document.width = 1920;
    document.height = 1080;
    document.background = {1, 1, 1, 1};
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
        const auto center = centers.value(role).toArray();
        REQUIRE(center.size() == 2);
        layer->transform.x = center[0].toDouble() - 128;
        layer->transform.y = center[1].toDouble() - 128;
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
    const auto reference = QImage(QStringLiteral(OPENTOON_SOURCE_DIR
        "/tests/fixtures/harmony-moment/reference_0000.png"))
        .convertToFormat(QImage::Format_ARGB32_Premultiplied);
    REQUIRE(baseline.size() == reference.size());
    int edgePixels = 0, maxDifference = 0;
    for (int y = 0; y < baseline.height(); ++y)
        for (int x = 0; x < baseline.width(); ++x) {
            const auto rendered = baseline.pixel(x, y);
            const auto sampled = reference.pixel(x, y);
            const int difference = std::max({std::abs(qRed(rendered) - qRed(sampled)),
                                             std::abs(qGreen(rendered) - qGreen(sampled)),
                                             std::abs(qBlue(rendered) - qBlue(sampled)),
                                             std::abs(qAlpha(rendered) - qAlpha(sampled))});
            edgePixels += difference > 0;
            maxDifference = std::max(maxDifference, difference);
        }
    INFO("Independent antialiased reference differs at " << edgePixels
         << " pixels, maximum channel difference " << maxDifference);
    REQUIRE(maxDifference <= 2);
    REQUIRE(edgePixels < 20000);
    REQUIRE(SceneRenderer::render(document, 12) == baseline);
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

    for (const Id part : parts) {
        const Id drawing = document.layer(part).exposures.front().drawing;
        resetMeshPose(document, part, drawing);
    }
    auto partFor = [&](const QString& role) {
        for (int index = 0; index < roles.size(); ++index)
            if (roles[index].toString() == role)
                return parts[std::size_t(index)];
        throw std::runtime_error("Missing original character part");
    };
    const auto armPart = partFor("upper_arm_left"), torsoPart = partFor("torso");
    const auto armDrawing = document.layer(armPart).exposures.front().drawing;
    const auto torsoDrawing = document.layer(torsoPart).exposures.front().drawing;
    removeMeshBinding(document, armPart, armDrawing);
    bindRegularImageMesh(document, armPart, armDrawing, 4, 4);
    const auto& armMesh = *meshBindingFor(document.layer(armPart), armDrawing);
    const auto armFirst = armMesh.vertices.front().rest;
    const auto armLast = armMesh.vertices.back().rest;
    const bool horizontal = armLast.x - armFirst.x >= armLast.y - armFirst.y;
    const MeshPoint rootJoint = horizontal ? MeshPoint{armFirst.x, (armFirst.y + armLast.y) / 2}
                                           : MeshPoint{(armFirst.x + armLast.x) / 2, armFirst.y};
    const MeshPoint tipJoint = horizontal ? MeshPoint{armLast.x, rootJoint.y}
                                          : MeshPoint{rootJoint.x, armLast.y};
    const MeshPoint elbowJoint{(rootJoint.x + tipJoint.x) / 2,
                               (rootJoint.y + tipJoint.y) / 2};
    bindBoneChain(document, armPart, armDrawing, {rootJoint, elbowJoint, tipJoint},
                  std::max(1.0, (horizontal ? tipJoint.x - rootJoint.x
                                             : tipJoint.y - rootJoint.y) * 0.15));
    recordBonePose(document, armPart, armDrawing, 12, 0, 12);
    const auto& torsoMesh = *meshBindingFor(document.layer(torsoPart), torsoDrawing);
    const auto torsoFirst = torsoMesh.vertices.front().rest;
    const auto torsoLast = torsoMesh.vertices.back().rest;
    const double centerX = (torsoFirst.x + torsoLast.x) / 2;
    std::array<MeshPoint, 4> torsoControls{};
    for (int index = 0; index < 4; ++index)
        torsoControls[index] = {centerX, torsoFirst.y + (torsoLast.y - torsoFirst.y) * index / 3};
    bindCurveDeformer(document, torsoPart, torsoDrawing, torsoControls);
    auto curved = torsoControls;
    curved[1].x += 4;
    curved[2].x += 4;
    recordCurvePose(document, torsoPart, torsoDrawing, 12, curved);
    document.validate();
    REQUIRE(SceneRenderer::render(document, 0) == baseline);
    const auto animated = SceneRenderer::render(document, 12);
    REQUIRE(animated != baseline);
    REQUIRE(animated.save("hm06-bone-curve.png"));
    REQUIRE(animated.copy(700, 350, 520, 500).save("hm06-bone-curve-detail.png"));
    const auto reopenedAnimated = deserializeDocument(serializeDocument(document));
    REQUIRE(SceneRenderer::render(reopenedAnimated, 12) == animated);
    auto stronger = document;
    const std::array<MeshPoint, 3> anatomicalArm{{{145, 50}, {112, 185}, {86, 330}}};
    constexpr double anatomicalTransition = 60;
    removeMeshBinding(stronger, armPart, armDrawing);
    bindRegularImageMesh(stronger, armPart, armDrawing, 4, 4);
    REQUIRE_NOTHROW(bindBoneChain(stronger, armPart, armDrawing, anatomicalArm, anatomicalTransition));
    REQUIRE_NOTHROW(recordBonePose(stronger, armPart, armDrawing, 12, 0, 12));
    REQUIRE_NOTHROW(recordBonePose(stronger, armPart, armDrawing, 24, 0, 70));
    for (const auto& role : {"lower_arm_left", "hand_left"}) {
        const Id follower = partFor(role);
        const auto& armTransform = stronger.layer(armPart).transform;
        const auto& followerTransform = stronger.layer(follower).transform;
        setPivotPreservingArtwork(stronger, follower,
                                 anatomicalArm[1].x + armTransform.x - followerTransform.x,
                                 anatomicalArm[1].y + armTransform.y - followerTransform.y);
        auto followerPose = stronger.layer(follower).transform;
        followerPose.rotation = 12;
        recordPose(stronger.layer(follower), 12, followerPose);
        followerPose.rotation = 70;
        recordPose(stronger.layer(follower), 24, followerPose);
    }
    stronger.validate();
    REQUIRE(SceneRenderer::render(stronger, 0) == baseline);
    const auto strongerImage = SceneRenderer::render(stronger, 24);
    REQUIRE(strongerImage != animated);
    REQUIRE(strongerImage.save("hm06-bone-extreme.png"));
    REQUIRE(strongerImage.copy(760, 250, 400, 500).save("hm06-bone-extreme-detail.png"));
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(stronger)), 24) ==
            strongerImage);
    if (qEnvironmentVariableIsSet("OPENTOON_HM06_BENCH_PROJECT")) {
        const auto output = qEnvironmentVariable("OPENTOON_HM06_BENCH_PROJECT");
        REQUIRE(ProjectStore::save(std::filesystem::path(output.toStdString()), document) > 0);
    }
    const auto animatedStart = std::chrono::steady_clock::now();
    for (int run = 0; run < 3; ++run)
        REQUIRE(SceneRenderer::render(document, 12) == animated);
    const auto animatedElapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - animatedStart).count() / 3;
    std::fprintf(stderr, "HM-06 Harmony 19 parts with bone and curve 1920x1080: %.2f ms per frame (%s)\n",
                 animatedElapsed,
#ifdef NDEBUG
                 "release"
#else
                 "debug"
#endif
    );
}

TEST_CASE("Continuous Harmony limbs bend as four single meshes and reopen identically") {
    QFile specification(QStringLiteral(OPENTOON_SOURCE_DIR
        "/tests/fixtures/harmony-continuous-limbs/rig.json"));
    REQUIRE(specification.open(QIODevice::ReadOnly));
    const auto rig = QJsonDocument::fromJson(specification.readAll()).object();
    const auto limbs = rig.value("limbs").toObject();
    const auto waist = rig.value("joined_waist").toObject();
    QFile shotFile(QStringLiteral(OPENTOON_SOURCE_DIR
        "/tests/fixtures/harmony-moment/shot.json"));
    REQUIRE(shotFile.open(QIODevice::ReadOnly));
    const auto shot = QJsonDocument::fromJson(shotFile.readAll()).object();
    const auto centers = shot.value("reference_centers_px").toObject();
    const QStringList order{"leg_left", "foot_left", "leg_right", "foot_right",
                            "arm_left", "hand_left", "arm_right", "hand_right",
                            "pelvis", "torso", "neck", "head", "hair", "eyes", "mouth"};
    auto document = makeDocument();
    document.name = "Clockwork Hello — continuous rig study";
    document.width = 1920;
    document.height = 1080;
    document.background = {1, 1, 1, 1};
    QHash<QString, Id> partIds;
    for (const auto& role : order) {
        const bool limb = limbs.contains(role);
        const auto base = (limb || role == "pelvis") ? QStringLiteral(OPENTOON_SOURCE_DIR
            "/tests/fixtures/harmony-continuous-limbs/parts/") :
            QStringLiteral(OPENTOON_SOURCE_DIR "/tests/fixtures/harmony-moment/parts/");
        const QString variant = role == "mouth" ? "front__rest" :
                                (role == "head" || role == "hair" || role == "eyes") ? "front" :
                                role.startsWith("hand_") ? "open" : "base";
        const QImage source(base + role + "__" + variant + ".png");
        REQUIRE_FALSE(source.isNull());
        const auto image = source.convertToFormat(QImage::Format_RGBA8888);
        Layer* layer = nullptr;
        if (partIds.isEmpty())
            layer = &document.layers.front();
        else {
            Layer added;
            added.id = document.allocateId();
            document.layers.push_back(added);
            layer = &document.layers.back();
        }
        layer->name = role.toStdString();
        const Id drawing = document.allocateId();
        layer->exposures.push_back({0, document.duration, drawing});
        Drawing created;
        created.id = drawing;
        created.image = ImageAsset{image.width(), image.height(), {}};
        created.image->rgba.assign(image.constBits(), image.constBits() + image.sizeInBytes());
        document.drawings.emplace(drawing, std::move(created));
        const auto center = limb ? limbs.value(role).toObject().value("center_px").toArray() :
                            role == "pelvis" ? waist.value("center_px").toArray() :
                                               centers.value(role).toArray();
        REQUIRE(center.size() == 2);
        layer->transform.x = center[0].toDouble() - image.width() / 2;
        layer->transform.y = center[1].toDouble() - image.height() / 2;
        partIds.insert(role, layer->id);
    }
    const Id root = makeCharacter(document, partIds.value("leg_left"), "Clockwork Hello");
    setPartRole(document, partIds.value("leg_left"), "leg_left");
    for (const auto& role : order)
        if (role != "leg_left")
            attachDrawingAsPart(document, partIds.value(role), root, role.toStdString());
    document.validate();
    REQUIRE(document.layers.size() == 16); // Fifteen artwork Parts plus the character peg.
    const auto rest = SceneRenderer::render(document, 0);
    const auto [restConnected, restInk] = connectedInk(rest);
    REQUIRE(restConnected == restInk);
    struct LimbKey { QString role; double angle; QString follower; };
    const std::array<LimbKey, 4> keys{{{"arm_left", 65, "hand_left"},
                                      {"arm_right", -50, "hand_right"},
                                      {"leg_left", 25, "foot_left"},
                                      {"leg_right", -25, "foot_right"}}};
    for (const auto& key : keys) {
        INFO(key.role.toStdString());
        const auto config = limbs.value(key.role).toObject();
        const auto joints = config.value("joints_scene_px").toArray();
        REQUIRE(joints.size() == 3);
        const Id part = partIds.value(key.role);
        const Id drawing = document.layer(part).exposures.front().drawing;
        bindRegularImageMesh(document, part, drawing, 6, 16);
        const auto transform = document.layer(part).transform;
        std::array<MeshPoint, 3> local{};
        for (int index = 0; index < 3; ++index) {
            const auto point = joints[index].toArray();
            local[index] = {point[0].toDouble() - transform.x,
                            point[1].toDouble() - transform.y};
        }
        REQUIRE_NOTHROW(bindBoneChain(document, part, drawing, local,
                                     config.value("transition_px").toDouble()));
        REQUIRE_NOTHROW(recordBonePose(document, part, drawing, 24, 0, key.angle));
        const Id follower = partIds.value(key.follower);
        const auto followerTransform = document.layer(follower).transform;
        setPivotPreservingArtwork(document, follower,
                                 joints[2].toArray()[0].toDouble() - followerTransform.x,
                                 joints[2].toArray()[1].toDouble() - followerTransform.y);
        attachPartToBoneTip(document, follower, part);
    }
    document.validate();
    for (const auto& key : keys) {
        const Id part = partIds.value(key.role);
        const Id follower = partIds.value(key.follower);
        REQUIRE(document.layer(follower).keys.empty());
        const auto& source = document.layer(part);
        const auto& bone = *meshBindingFor(source, source.exposures.front().drawing)->bone;
        const auto restTip = bone.restJoints[2];
        const auto restScene = SceneRenderer::worldTransform(document, source, 0)
                                   .map(QPointF(restTip.x, restTip.y));
        const auto followerLocal = SceneRenderer::worldTransform(document,
                                                                 document.layer(follower), 0)
                                       .inverted().map(restScene);
        for (Frame frame = 0; frame <= 24; ++frame) {
            const auto tip = sampleBoneJoints(bone, frame)[2];
            const auto expected = SceneRenderer::worldTransform(document, source, frame)
                                      .map(QPointF(tip.x, tip.y));
            const auto actual = SceneRenderer::worldTransform(document,
                                                               document.layer(follower), frame)
                                    .map(followerLocal);
            REQUIRE(std::hypot(expected.x() - actual.x(), expected.y() - actual.y()) < 1e-8);
        }
    }
    REQUIRE(SceneRenderer::render(document, 0) == rest);
    const auto bent = SceneRenderer::render(document, 24);
    REQUIRE(bent != rest);
    const auto [bentConnected, bentInk] = connectedInk(bent);
    REQUIRE(bentConnected == bentInk);
    const Id linkedHand = partIds.value("hand_left");
    const Id linkedArm = partIds.value("arm_left");
    REQUIRE(document.layer(linkedHand).boneTipAnchor);
    REQUIRE(document.layer(linkedHand).parent == linkedArm);
    auto duplicated = document;
    const Id duplicateRoot = duplicateCharacter(duplicated, root, 300, 0);
    duplicated.validate();
    const auto duplicateHand = std::find_if(duplicated.layers.begin(), duplicated.layers.end(),
                                            [&](const Layer& layer) {
                                                return layer.kind == LayerKind::Part &&
                                                       layer.role == "hand_left" &&
                                                       characterFor(duplicated, layer.id) == duplicateRoot;
                                            });
    REQUIRE(duplicateHand != duplicated.layers.end());
    REQUIRE(duplicateHand->boneTipAnchor);
    REQUIRE(duplicateHand->parent != linkedArm);
    REQUIRE(duplicated.layer(duplicateHand->parent).role == "arm_left");
    auto unlinked = document;
    detachPartFromBoneTip(unlinked, linkedHand);
    Session attachmentSession;
    attachmentSession.replace(unlinked);
    const auto unlinkedPixels = SceneRenderer::render(attachmentSession.document(), 24);
    REQUIRE(unlinkedPixels != bent);
    REQUIRE(attachmentSession.apply("Follow arm tip", [&](Document& candidate) {
        attachPartToBoneTip(candidate, linkedHand, linkedArm);
    }));
    REQUIRE(SceneRenderer::render(attachmentSession.document(), 0) == rest);
    REQUIRE(SceneRenderer::render(attachmentSession.document(), 24) == bent);
    REQUIRE(attachmentSession.undo());
    REQUIRE(SceneRenderer::render(attachmentSession.document(), 24) == unlinkedPixels);
    REQUIRE(attachmentSession.redo());
    REQUIRE(SceneRenderer::render(attachmentSession.document(), 24) == bent);
    const auto attachedState = attachmentSession.document();
    REQUIRE_THROWS(attachmentSession.apply("Remove attached source bone", [&](Document& candidate) {
        removeMeshDeformer(candidate, linkedArm,
                           candidate.layer(linkedArm).exposures.front().drawing);
    }));
    REQUIRE(attachmentSession.document() == attachedState);
    REQUIRE_THROWS(attachmentSession.apply("Reparent attached hand", [&](Document& candidate) {
        reparentPreservingWorld(candidate, linkedHand, root);
    }));
    REQUIRE(attachmentSession.document() == attachedState);
    REQUIRE_THROWS(attachmentSession.apply("Expose unbound arm substitution", [&](Document& candidate) {
        createSubstitution(candidate, linkedArm, 30, true, "Unbound");
    }));
    REQUIRE(attachmentSession.document() == attachedState);
    auto tuned = document;
    const Id tunedArm = partIds.value("arm_left");
    const Id tunedDrawing = tuned.layer(tunedArm).exposures.front().drawing;
    setBoneElbowTransition(tuned, tunedArm, tunedDrawing, 55);
    REQUIRE(SceneRenderer::render(tuned, 0) == rest);
    const auto tunedFrame = SceneRenderer::render(tuned, 24);
    REQUIRE(tunedFrame != bent);
    const auto [tunedConnected, tunedInk] = connectedInk(tunedFrame);
    REQUIRE(tunedConnected == tunedInk);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(tuned)), 24) ==
            tunedFrame);
    REQUIRE(bent.save("hm06-continuous-limbs.png"));
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(document)), 24) == bent);
    QTemporaryDir savedProject;
    REQUIRE(savedProject.isValid());
    const auto savedPath = std::filesystem::path((savedProject.path() + "/continuous.otoon").toStdString());
    REQUIRE(ProjectStore::save(savedPath, document) > 0);
    const auto reopened = ProjectStore::load(savedPath).document;
    REQUIRE(reopened == document);
    REQUIRE(SceneRenderer::render(reopened, 24) == bent);
    auto switched = document;
    const Id sourceDrawing = switched.layer(linkedArm).exposures.front().drawing;
    const Id alternateDrawing = createSubstitution(switched, linkedArm, 30, true,
                                                   "Alternate sleeve");
    auto alternateBinding = *meshBindingFor(switched.layer(linkedArm), sourceDrawing);
    alternateBinding.drawing = alternateDrawing;
    switched.layer(linkedArm).bindings.push_back(std::move(alternateBinding));
    recordBonePose(switched, linkedArm, alternateDrawing, 36, 0, 30);
    switched.validate();
    const auto switchedFrame = SceneRenderer::render(switched, 36);
    REQUIRE(switchedFrame != SceneRenderer::render(document, 36));
    const auto [switchedConnected, switchedInk] = connectedInk(switchedFrame);
    REQUIRE(switchedConnected == switchedInk);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(switched)), 36) ==
            switchedFrame);
    auto shiftedWrist = switched;
    const auto& baselineBone = *meshBindingFor(shiftedWrist.layer(linkedArm),
        shiftedWrist.drawingAt(linkedArm, 0)->id)->bone;
    const auto baselineTip = baselineBone.restJoints[2];
    const auto baselineElbow = baselineBone.restJoints[1];
    const double baselineLength = std::hypot(baselineTip.x - baselineElbow.x,
                                             baselineTip.y - baselineElbow.y);
    const auto baselineAxis = QPointF((baselineTip.x - baselineElbow.x) / baselineLength,
                                       (baselineTip.y - baselineElbow.y) / baselineLength);
    const auto sourceRestWorld = SceneRenderer::worldTransform(shiftedWrist,
        shiftedWrist.layer(linkedArm), 0);
    const auto childRestWorld = SceneRenderer::worldTransform(shiftedWrist,
        shiftedWrist.layer(linkedHand), 0);
    const auto childAnchor = childRestWorld.inverted().map(
        sourceRestWorld.map(QPointF(baselineTip.x, baselineTip.y)));
    const auto childAxisPoint = childRestWorld.inverted().map(sourceRestWorld.map(
        QPointF(baselineTip.x + 10 * baselineAxis.x(),
                baselineTip.y + 10 * baselineAxis.y())));
    const auto activeRestTip = meshBindingFor(shiftedWrist.layer(linkedArm),
        alternateDrawing)->bone->restJoints[2];
    REQUIRE_NOTHROW(moveBoneRestJoint(shiftedWrist, linkedArm, alternateDrawing, 2,
                                      {activeRestTip.x + 8, activeRestTip.y - 12}));
    shiftedWrist.validate();
    Session retargetSession;
    retargetSession.replace(switched);
    REQUIRE(retargetSession.apply("Retarget alternate wrist", [&](Document& candidate) {
        moveBoneRestJoint(candidate, linkedArm, alternateDrawing, 2,
                          {activeRestTip.x + 8, activeRestTip.y - 12});
    }));
    REQUIRE(retargetSession.document() == shiftedWrist);
    REQUIRE(retargetSession.undo());
    REQUIRE(retargetSession.document() == switched);
    REQUIRE(retargetSession.redo());
    REQUIRE(retargetSession.document() == shiftedWrist);
    for (const Frame frame : {30, 36}) {
        const auto& source = shiftedWrist.layer(linkedArm);
        const auto& bone = *meshBindingFor(source,
            shiftedWrist.drawingAt(linkedArm, frame)->id)->bone;
        const auto joints = sampleBoneJoints(bone, frame);
        const auto expectedTip = SceneRenderer::worldTransform(shiftedWrist, source, frame)
            .map(QPointF(joints[2].x, joints[2].y));
        const auto axisLength = std::hypot(joints[2].x - joints[1].x,
                                           joints[2].y - joints[1].y);
        const auto expectedAxisPoint = SceneRenderer::worldTransform(shiftedWrist, source, frame)
            .map(QPointF(joints[2].x + 10 * (joints[2].x - joints[1].x) / axisLength,
                         joints[2].y + 10 * (joints[2].y - joints[1].y) / axisLength));
        const auto childWorld = SceneRenderer::worldTransform(shiftedWrist,
            shiftedWrist.layer(linkedHand), frame);
        const auto actualTip = childWorld.map(childAnchor);
        const auto actualAxisPoint = childWorld.map(childAxisPoint);
        REQUIRE(std::hypot(expectedTip.x() - actualTip.x(),
                           expectedTip.y() - actualTip.y()) < 1e-8);
        REQUIRE(std::hypot(expectedAxisPoint.x() - actualAxisPoint.x(),
                           expectedAxisPoint.y() - actualAxisPoint.y()) < 1e-8);
    }
    auto mirroredWrist = shiftedWrist;
    mirroredWrist.layer(root).transform.rotation = 25;
    mirroredWrist.layer(root).transform.scaleX = -1;
    mirroredWrist.validate();
    for (const Frame frame : {30, 36}) {
        const auto& source = mirroredWrist.layer(linkedArm);
        const auto& bone = *meshBindingFor(source,
            mirroredWrist.drawingAt(linkedArm, frame)->id)->bone;
        const auto tip = sampleBoneJoints(bone, frame)[2];
        const auto expected = SceneRenderer::worldTransform(mirroredWrist, source, frame)
            .map(QPointF(tip.x, tip.y));
        const auto actual = SceneRenderer::worldTransform(mirroredWrist,
            mirroredWrist.layer(linkedHand), frame).map(childAnchor);
        REQUIRE(std::hypot(expected.x() - actual.x(),
                           expected.y() - actual.y()) < 1e-8);
    }
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(shiftedWrist)), 36) ==
            SceneRenderer::render(shiftedWrist, 36));
    const auto retargetedPath = std::filesystem::path((savedProject.path() +
                                                       "/shifted-wrist.otoon").toStdString());
    REQUIRE(ProjectStore::save(retargetedPath, shiftedWrist) > 0);
    const auto reopenedRetargeted = ProjectStore::load(retargetedPath).document;
    REQUIRE(reopenedRetargeted == shiftedWrist);
    REQUIRE(SceneRenderer::render(reopenedRetargeted, 36) ==
            SceneRenderer::render(shiftedWrist, 36));
    auto initialVariant = shiftedWrist;
    expose(initialVariant.layer(linkedArm), 0, 30, alternateDrawing);
    initialVariant.validate();
    REQUIRE(initialVariant.layer(linkedHand).boneTipAnchor ==
            shiftedWrist.layer(linkedHand).boneTipAnchor);
    const auto& initialBone = *meshBindingFor(initialVariant.layer(linkedArm),
        initialVariant.drawingAt(linkedArm, 0)->id)->bone;
    const auto initialJoints = sampleBoneJoints(initialBone, 0);
    const auto initialExpected = SceneRenderer::worldTransform(initialVariant,
        initialVariant.layer(linkedArm), 0).map(
            QPointF(initialJoints[2].x, initialJoints[2].y));
    const auto initialActual = SceneRenderer::worldTransform(initialVariant,
        initialVariant.layer(linkedHand), 0).map(childAnchor);
    REQUIRE(std::hypot(initialExpected.x() - initialActual.x(),
                       initialExpected.y() - initialActual.y()) < 1e-8);
    const auto initialProject = std::filesystem::path((savedProject.path() +
                                                      "/initial-variant.otoon").toStdString());
    REQUIRE(ProjectStore::save(initialProject, initialVariant) > 0);
    REQUIRE(ProjectStore::load(initialProject).document == initialVariant);
    auto expressive = switched;
    const Id frontView = captureCharacterView(expressive, root, 0, "Front");
    const auto replaceWithPartArt = [&](QString role, QString name, QString imageName) {
        const Id drawing = createSubstitution(expressive, partIds.value(role), 30, false,
                                              name.toStdString());
        const QImage source(QStringLiteral(OPENTOON_SOURCE_DIR
            "/tests/fixtures/harmony-moment/parts/") + imageName + ".png");
        REQUIRE_FALSE(source.isNull());
        const auto image = source.convertToFormat(QImage::Format_RGBA8888);
        std::vector<std::uint8_t> pixels;
        pixels.reserve(std::size_t(image.width()) * image.height() * 4);
        for (int y = 0; y < image.height(); ++y) {
            const auto* row = image.constScanLine(y);
            pixels.insert(pixels.end(), row, row + image.width() * 4);
        }
        expressive.drawings.at(drawing).image =
            ImageAsset{image.width(), image.height(), std::move(pixels)};
        return drawing;
    };
    const Id fistHand = replaceWithPartArt("hand_left", "Fist", "hand_left__fist");
    replaceWithPartArt("head", "Three-quarter", "head__three_quarter");
    replaceWithPartArt("hair", "Three-quarter", "hair__three_quarter");
    replaceWithPartArt("eyes", "Three-quarter", "eyes__three_quarter");
    replaceWithPartArt("mouth", "Three-quarter Ah", "mouth__three_quarter__ah");
    const Id sideView = captureCharacterView(expressive, root, 36, "Three-quarter fist");
    REQUIRE(sideView != frontView);
    REQUIRE(expressive.drawingAt(linkedHand, 36)->id == fistHand);
    REQUIRE(expressive.layer(linkedHand).boneTipAnchor);
    expressive.validate();
    const auto& restSource = expressive.layer(linkedArm);
    const auto& restBone = *meshBindingFor(restSource,
        expressive.drawingAt(linkedArm, 0)->id)->bone;
    const auto restTipScene = SceneRenderer::worldTransform(expressive, restSource, 0)
        .map(QPointF(restBone.restJoints[2].x, restBone.restJoints[2].y));
    const auto handAnchor = SceneRenderer::worldTransform(expressive,
        expressive.layer(linkedHand), 0).inverted().map(restTipScene);
    const auto handTracksActiveTip = [&](const Document& scene, Frame frame) {
        const auto& source = scene.layer(linkedArm);
        const auto& bone = *meshBindingFor(source,
            scene.drawingAt(linkedArm, frame)->id)->bone;
        const auto tip = sampleBoneJoints(bone, frame)[2];
        const auto expected = SceneRenderer::worldTransform(scene, source, frame)
            .map(QPointF(tip.x, tip.y));
        const auto actual = SceneRenderer::worldTransform(scene,
            scene.layer(linkedHand), frame).map(handAnchor);
        REQUIRE(std::hypot(expected.x() - actual.x(),
                           expected.y() - actual.y()) < 1e-8);
    };
    for (const Frame frame : {0, 24, 30, 36})
        handTracksActiveTip(expressive, frame);
    const auto expressiveFrame = SceneRenderer::render(expressive, 36);
    REQUIRE(expressiveFrame != switchedFrame);
    const auto [expressiveConnected, expressiveInk] = connectedInk(expressiveFrame);
    REQUIRE(expressiveConnected == expressiveInk);
    REQUIRE(expressiveFrame.save("hm06-continuous-pose-switch.png"));
    Session viewSession;
    viewSession.replace(expressive);
    REQUIRE(viewSession.apply("Return to front", [&](Document& candidate) {
        applyCharacterViewRange(candidate, root, frontView, 40, candidate.duration);
    }));
    REQUIRE(viewSession.document().drawingAt(linkedHand, 36)->id == fistHand);
    REQUIRE(viewSession.document().drawingAt(linkedHand, 44)->id != fistHand);
    handTracksActiveTip(viewSession.document(), 44);
    const auto returnedFrame = SceneRenderer::render(viewSession.document(), 44);
    REQUIRE(returnedFrame != expressiveFrame);
    REQUIRE(viewSession.undo());
    REQUIRE(SceneRenderer::render(viewSession.document(), 36) == expressiveFrame);
    REQUIRE(viewSession.redo());
    const auto viewProject = std::filesystem::path((savedProject.path() +
                                                   "/expressive.otoon").toStdString());
    REQUIRE(ProjectStore::save(viewProject, viewSession.document()) > 0);
    const auto reopenedView = ProjectStore::load(viewProject).document;
    REQUIRE(reopenedView == viewSession.document());
    REQUIRE(SceneRenderer::render(reopenedView, 36) == expressiveFrame);
    REQUIRE(SceneRenderer::render(reopenedView, 44) == returnedFrame);
    auto polishedView = reopenedView;
    for (const auto& key : keys) {
        const Id part = partIds.value(key.role);
        const Id follower = partIds.value(key.follower);
        detachPartFromBoneTip(polishedView, follower);
        std::vector<std::pair<Id, BoneChain>> controls;
        for (const auto& binding : polishedView.layer(part).bindings)
            if (binding.bone)
                controls.emplace_back(binding.drawing, *binding.bone);
        REQUIRE_FALSE(controls.empty());
        for (const auto& [drawing, originalBone] : controls) {
            removeMeshDeformer(polishedView, part, drawing);
            bindContourImageMesh(polishedView, part, drawing, 6, 16);
            bindBoneChain(polishedView, part, drawing, originalBone.restJoints, 40);
            for (const auto& pose : originalBone.keys)
                recordBonePose(polishedView, part, drawing, pose.frame,
                               pose.shoulderAngle, pose.elbowAngle,
                               pose.interpolation);
        }
        attachPartToBoneTip(polishedView, follower, part);
    }
    polishedView.validate();
    REQUIRE(SceneRenderer::render(polishedView, 0) == rest);
    for (const Frame frame : {24, 36, 44}) {
        const auto [connected, ink] = connectedInk(SceneRenderer::render(polishedView, frame));
        REQUIRE(connected == ink);
    }
    if (qEnvironmentVariableIsSet("OPENTOON_HM06_VIEW_PROJECT")) {
        const auto output = qEnvironmentVariable("OPENTOON_HM06_VIEW_PROJECT");
        REQUIRE(ProjectStore::save(std::filesystem::path(output.toStdString()), polishedView) > 0);
    }
    const auto bundledExample = ProjectStore::load(std::filesystem::path(
        OPENTOON_SOURCE_DIR "/examples/clockwork-continuous.otoon")).document;
    REQUIRE(bundledExample == polishedView);
    auto folded = document;
    const Id foldedArm = partIds.value("arm_left");
    const Id foldedDrawing = folded.layer(foldedArm).exposures.front().drawing;
    setBoneElbowTransition(folded, foldedArm, foldedDrawing, 55);
    const auto beforeRejectedBend = serializeDocument(folded);
    REQUIRE_THROWS_AS(recordBonePose(folded, foldedArm, foldedDrawing, 36, 0, 90),
                      std::invalid_argument);
    REQUIRE(serializeDocument(folded) == beforeRejectedBend);
    auto contoured = document;
    for (const auto& key : keys) {
        INFO(key.role.toStdString());
        const Id part = partIds.value(key.role);
        const Id drawing = contoured.layer(part).exposures.front().drawing;
        const Id follower = partIds.value(key.follower);
        const auto originalBone = *meshBindingFor(contoured.layer(part), drawing)->bone;
        detachPartFromBoneTip(contoured, follower);
        removeMeshDeformer(contoured, part, drawing);
        bindContourImageMesh(contoured, part, drawing, 6, 16);
        const auto& contour = *meshBindingFor(contoured.layer(part), drawing);
        const auto& sourceImage = *contoured.drawings.at(drawing).image;
        const double top = contour.vertices.front().rest.y;
        const double bottom = contour.vertices.back().rest.y;
        bool coversSourceAlpha = true;
        for (int y = 0; y < sourceImage.height; ++y)
            for (int x = 0; x < sourceImage.width; ++x) {
                if (!sourceImage.rgba[(std::size_t(y) * sourceImage.width + x) * 4 + 3])
                    continue;
                const double rowPosition = (y + 0.5 - top) * contour.rows / (bottom - top);
                const int row = std::clamp(int(std::floor(rowPosition)), 0, contour.rows - 1);
                const double t = std::clamp(rowPosition - row, 0.0, 1.0);
                const auto index = std::size_t(row) * (contour.columns + 1);
                const double left = std::lerp(contour.vertices[index].rest.x,
                                              contour.vertices[index + contour.columns + 1].rest.x, t);
                const double right = std::lerp(contour.vertices[index + contour.columns].rest.x,
                                               contour.vertices[index + 2 * contour.columns + 1].rest.x, t);
                coversSourceAlpha &= x + 0.5 >= left && x + 0.5 <= right;
            }
        REQUIRE(coversSourceAlpha);
        if (part == foldedArm)
            REQUIRE(meshBindingFor(contoured.layer(part), drawing)->vertices[7 * 7].rest.x >
                    meshBindingFor(document.layer(part), drawing)->vertices[7 * 7].rest.x);
        bindBoneChain(contoured, part, drawing, originalBone.restJoints, 40);
        recordBonePose(contoured, part, drawing, 24, 0, key.angle);
        REQUIRE_NOTHROW(recordBonePose(contoured, part, drawing, 36, 0,
                                      key.angle < 0 ? -90 : 90));
        attachPartToBoneTip(contoured, follower, part);
    }
    contoured.validate();
    const auto beforeUnsafeRebind = serializeDocument(contoured);
    REQUIRE_THROWS_AS(bindContourImageMesh(contoured, foldedArm, foldedDrawing, 6, 16),
                      std::invalid_argument);
    REQUIRE(serializeDocument(contoured) == beforeUnsafeRebind);
    auto overlySharp = contoured;
    REQUIRE_THROWS_AS(setBoneElbowTransition(overlySharp, foldedArm, foldedDrawing, 35),
                      std::invalid_argument);
    REQUIRE(overlySharp == contoured);
    REQUIRE(SceneRenderer::render(contoured, 0) == rest);
    REQUIRE(SceneRenderer::render(contoured, 24).save("hm06-contour-bend-24.png"));
    const auto contourFrame = SceneRenderer::render(contoured, 36);
    const auto [contourConnected, contourInk] = connectedInk(contourFrame);
    REQUIRE(contourConnected == contourInk);
    REQUIRE(contourFrame != SceneRenderer::render(document, 36));
    REQUIRE(contourFrame.save("hm06-contour-bend-90.png"));
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(contoured)), 36) ==
            contourFrame);
    const auto contourPath = std::filesystem::path((savedProject.path() +
                                                   "/contour.otoon").toStdString());
    REQUIRE(ProjectStore::save(contourPath, contoured) > 0);
    REQUIRE(SceneRenderer::render(ProjectStore::load(contourPath).document, 36) == contourFrame);
    auto extreme = document;
    for (const auto& key : keys) {
        INFO(key.role.toStdString());
        const Id part = partIds.value(key.role);
        const Id drawing = extreme.layer(part).exposures.front().drawing;
        const double angle = key.angle < 0 ? -90 : 90;
        REQUIRE_NOTHROW(recordBonePose(extreme, part, drawing, 36, 0, angle));
    }
    extreme.validate();
    REQUIRE(SceneRenderer::render(extreme, 0) == rest);
    const auto extremeFrame = SceneRenderer::render(extreme, 36);
    REQUIRE(extremeFrame.save("hm06-continuous-limbs-90.png"));
    const auto [extremeConnected, extremeInk] = connectedInk(extremeFrame);
    REQUIRE(extremeConnected == extremeInk);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(extreme)), 36) ==
            extremeFrame);
    for (int frame = 1; frame < 36; ++frame) {
        INFO(frame);
        const auto sampled = SceneRenderer::render(extreme, frame);
        const auto [connected, ink] = connectedInk(sampled);
        REQUIRE(connected == ink);
    }
    if (qEnvironmentVariableIsSet("OPENTOON_HM06_CONTINUOUS_PROJECT")) {
        const auto output = qEnvironmentVariable("OPENTOON_HM06_CONTINUOUS_PROJECT");
        REQUIRE(ProjectStore::save(std::filesystem::path(output.toStdString()), document) > 0);
    }
}

TEST_CASE("Full-length continuous toon retimes linked limbs and coordinated views") {
    const auto shortScene = ProjectStore::load(std::filesystem::path(
        OPENTOON_SOURCE_DIR "/examples/clockwork-continuous.otoon")).document;
    REQUIRE(shortScene.duration == 48);
    auto fullScene = shortScene;
    fullScene.name = "Clockwork Hello — 20-second deformation study";
    std::vector<Id> tracks;
    tracks.reserve(fullScene.layers.size());
    for (const auto& layer : fullScene.layers)
        tracks.push_back(layer.id);
    REQUIRE_NOTHROW(retimeRange(fullScene, tracks, 0, 48, 480));
    fullScene.validate();
    REQUIRE(fullScene.duration == 480);
    REQUIRE(std::count_if(fullScene.layers.begin(), fullScene.layers.end(),
                          [](const Layer& layer) { return layer.boneTipAnchor.has_value(); }) == 4);
    for (const Frame frame : {0, 120, 240, 300, 360, 400, 440}) {
        INFO(frame);
        REQUIRE(SceneRenderer::render(fullScene, frame) ==
                SceneRenderer::render(shortScene, frame / 10));
    }
    const bool fullResolution = qEnvironmentVariableIsSet("OPENTOON_HM06_FULL_RENDER");
    const QSize renderSize = fullResolution ? QSize{} : QSize(480, 270);
    const QSize expectedSize = fullResolution ? QSize(1920, 1080) : renderSize;
    const auto renderStart = std::chrono::steady_clock::now();
    for (Frame frame = 0; frame < fullScene.duration; ++frame) {
        const auto preview = SceneRenderer::render(fullScene, frame, renderSize);
        REQUIRE(preview.size() == expectedSize);
    }
    if (fullResolution) {
        const auto elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - renderStart).count();
        std::fprintf(stderr, "HM-06 480-frame 1920x1080 deformation study: %.2f ms/frame\n",
                     elapsed / fullScene.duration);
    }
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    const auto path = std::filesystem::path((temporary.path() + "/long-study.otoon").toStdString());
    REQUIRE(ProjectStore::save(path, fullScene) > 0);
    const auto reopened = ProjectStore::load(path).document;
    REQUIRE(reopened == fullScene);
    for (const Frame frame : {0, 120, 240, 300, 360, 400, 440, 479})
        REQUIRE(SceneRenderer::render(reopened, frame) ==
                SceneRenderer::render(fullScene, frame));
    if (qEnvironmentVariableIsSet("OPENTOON_HM06_LONG_PROJECT")) {
        const auto output = qEnvironmentVariable("OPENTOON_HM06_LONG_PROJECT");
        REQUIRE(ProjectStore::save(std::filesystem::path(output.toStdString()), reopened) > 0);
    }
}

TEST_CASE("Twenty-second visual shot combines deformers views mouth hands and camera") {
    QFile specification(QStringLiteral(OPENTOON_SOURCE_DIR
        "/tests/fixtures/harmony-moment/shot.json"));
    REQUIRE(specification.open(QIODevice::ReadOnly));
    const auto brief = QJsonDocument::fromJson(specification.readAll()).object();
    const auto cues = brief.value("audio").toObject().value("cue_frames").toArray();
    REQUIRE(cues.size() == 9);
    const auto shortScene = ProjectStore::load(std::filesystem::path(
        OPENTOON_SOURCE_DIR "/examples/clockwork-continuous.otoon")).document;
    REQUIRE(shortScene.duration == 48);
    auto scene = shortScene;
    scene.name = "Clockwork Hello — visual shot study";
    std::vector<Id> tracks;
    for (const auto& layer : scene.layers)
        tracks.push_back(layer.id);
    REQUIRE_NOTHROW(retimeRange(scene, tracks, 0, 48, 480));
    const auto findRole = [&](const char* role) {
        const auto found = std::find_if(scene.layers.begin(), scene.layers.end(),
                                        [&](const Layer& layer) { return layer.role == role; });
        return found == scene.layers.end() ? Id{0} : found->id;
    };
    const auto rootLayer = std::find_if(scene.layers.begin(), scene.layers.end(),
                                        [](const Layer& layer) {
                                            return layer.kind == LayerKind::Character;
                                        });
    REQUIRE(rootLayer != scene.layers.end());
    const Id root = rootLayer->id;
    const Id arm = findRole("arm_left"), hand = findRole("hand_left");
    const Id mouth = findRole("mouth"), torso = findRole("torso");
    REQUIRE(arm != 0);
    REQUIRE(hand != 0);
    REQUIRE(mouth != 0);
    REQUIRE(torso != 0);
    const auto views = scene.layer(root).views;
    REQUIRE(views.size() == 2);
    const Id front = views.front().id, side = views.back().id;
    applyCharacterViewRange(scene, root, front, 0, scene.duration);
    applyCharacterViewRange(scene, root, side, 120, 432);
    const Id baseSleeve = scene.layer(arm).variants.front().drawing;
    const Id alternateSleeve = scene.layer(arm).variants.back().drawing;
    expose(scene.layer(arm), 120, 300, baseSleeve);
    REQUIRE(scene.drawingAt(arm, 299)->id == baseSleeve);
    REQUIRE(scene.drawingAt(arm, 300)->id == alternateSleeve);

    const auto addArt = [&](Id part, Frame frame, std::string name, QString file) {
        const Id drawing = createSubstitution(scene, part, frame, false, std::move(name));
        const QImage source(QStringLiteral(OPENTOON_SOURCE_DIR
            "/tests/fixtures/harmony-moment/parts/") + file + ".png");
        REQUIRE_FALSE(source.isNull());
        const auto image = source.convertToFormat(QImage::Format_RGBA8888);
        std::vector<std::uint8_t> pixels;
        pixels.reserve(std::size_t(image.width()) * image.height() * 4);
        for (int y = 0; y < image.height(); ++y) {
            const auto* row = image.constScanLine(y);
            pixels.insert(pixels.end(), row, row + image.width() * 4);
        }
        scene.drawings.at(drawing).image =
            ImageAsset{image.width(), image.height(), std::move(pixels)};
        return drawing;
    };
    const Id openHand = scene.layer(hand).variants.front().drawing;
    const Id fistHand = scene.layer(hand).variants.back().drawing;
    const Id pointHand = addArt(hand, 240, "Point", "hand_left__point");
    expose(scene.layer(hand), 120, 240, openHand);
    expose(scene.layer(hand), 240, 336, pointHand);
    expose(scene.layer(hand), 336, 360, fistHand);
    expose(scene.layer(hand), 360, 432, pointHand);
    const Id sideAh = scene.drawingAt(mouth, 120)->id;
    std::map<std::string, Id> sideMouth{{"ah", sideAh}};
    for (const auto* shape : {"rest", "mbp", "fv", "ee", "oh", "l", "wide"})
        sideMouth.emplace(shape, addArt(mouth, 120, std::string("Side ") + shape,
                             QStringLiteral("mouth__three_quarter__") + shape));
    REQUIRE(sideMouth.size() == 8);
    expose(scene.layer(mouth), 120, 432, sideMouth.at("rest"));
    const std::array<std::pair<Frame, const char*>, 10> mouthKeys{{
        {120, "ah"}, {126, "mbp"}, {144, "ee"}, {168, "ah"}, {190, "fv"},
        {214, "rest"}, {345, "oh"}, {365, "l"}, {388, "wide"}, {410, "rest"}}};
    for (std::size_t index = 0; index < mouthKeys.size(); ++index) {
        const Frame end = index + 1 < mouthKeys.size() ? mouthKeys[index + 1].first : 432;
        expose(scene.layer(mouth), mouthKeys[index].first, end,
               sideMouth.at(mouthKeys[index].second));
    }
    for (int index = 0; index < cues.size(); ++index) {
        REQUIRE(cues[index].toInt() == mouthKeys[std::size_t(index + 1)].first);
        scene.markers.push_back({cues[index].toInt(),
                                 "Dialogue cue " + std::to_string(index + 1)});
    }

    const auto rootRest = scene.layer(root).transform;
    for (const auto [frame, offset] :
         {std::pair{Frame{0}, 0.0}, {Frame{120}, 60.0}, {Frame{240}, 80.0},
          {Frame{360}, 80.0}, {Frame{479}, 0.0}}) {
        auto pose = rootRest;
        pose.x += offset;
        recordPose(scene.layer(root), frame, pose);
    }
    std::vector<std::pair<Id, Id>> boneBindings;
    for (const auto& layer : scene.layers)
        for (const auto& binding : layer.bindings)
            if (binding.bone)
                boneBindings.emplace_back(layer.id, binding.drawing);
    REQUIRE(boneBindings.size() == 5);
    for (const auto [part, drawing] : boneBindings)
        recordBonePose(scene, part, drawing, 432, 0, 0);
    REQUIRE(canMatchPreviousDeformerPose(scene, arm, 300));
    matchPreviousDeformerPose(scene, arm, 300);
    const Id torsoDrawing = scene.drawingAt(torso, 0)->id;
    bindRegularImageMesh(scene, torso, torsoDrawing, 4, 8);
    const std::array<MeshPoint, 4> restCurve{{{128, 35}, {126, 95},
                                               {130, 155}, {128, 215}}};
    bindCurveDeformer(scene, torso, torsoDrawing, restCurve);
    auto sway = restCurve;
    sway[1].x += 3;
    sway[2].x += 6;
    recordCurvePose(scene, torso, torsoDrawing, 240, sway);
    sway[1].x += 3;
    sway[2].x += 4;
    recordCurvePose(scene, torso, torsoDrawing, 360, sway);
    recordCurvePose(scene, torso, torsoDrawing, 432, restCurve);

    Layer camera;
    camera.id = scene.allocateId();
    camera.kind = LayerKind::Camera;
    camera.name = "Output camera";
    camera.transform.x = scene.width / 2;
    camera.transform.y = scene.height / 2;
    scene.activeCamera = camera.id;
    scene.layers.push_back(camera);
    for (const auto [frame, zoom] :
         {std::pair{Frame{0}, 1.0}, {Frame{336}, 1.0},
          {Frame{360}, 1.12}, {Frame{432}, 1.12}, {Frame{479}, 1.0}}) {
        auto pose = camera.transform;
        pose.scaleX = pose.scaleY = zoom;
        recordPose(scene.layer(camera.id), frame, pose);
    }
    scene.validate();
    REQUIRE(scene.duration == 480);
    REQUIRE(scene.layer(hand).boneTipAnchor.has_value());
    REQUIRE(scene.drawingAt(hand, 239)->id == openHand);
    REQUIRE(scene.drawingAt(hand, 240)->id == pointHand);
    REQUIRE(scene.drawingAt(hand, 336)->id == fistHand);
    REQUIRE(scene.drawingAt(hand, 360)->id == pointHand);
    REQUIRE(scene.drawingAt(hand, 432)->id == openHand);
    REQUIRE(scene.drawingAt(mouth, 120)->id == sideAh);
    REQUIRE(scene.drawingAt(mouth, 345)->id == sideMouth.at("oh"));
    REQUIRE(scene.drawingAt(mouth, 410)->id == sideMouth.at("rest"));
    REQUIRE(scene.drawingAt(mouth, 432)->id == scene.drawingAt(mouth, 0)->id);
    REQUIRE(evaluateTransform(scene.layer(camera.id), 360).scaleX == 1.12);
    for (const auto [sourceRole, followerRole] :
         {std::pair{"arm_left", "hand_left"}, {"arm_right", "hand_right"},
         {"leg_left", "foot_left"}, {"leg_right", "foot_right"}}) {
        INFO(sourceRole);
        const Id sourceId = findRole(sourceRole), followerId = findRole(followerRole);
        REQUIRE(sourceId != 0);
        REQUIRE(followerId != 0);
        const auto& restSource = scene.layer(sourceId);
        const auto& restBone = *meshBindingFor(restSource,
            scene.drawingAt(sourceId, 0)->id)->bone;
        const auto restTip = restBone.restJoints[2];
        const auto restWorld = SceneRenderer::worldTransform(scene, restSource, 0)
            .map(QPointF(restTip.x, restTip.y));
        const auto followerLocal = SceneRenderer::worldTransform(
            scene, scene.layer(followerId), 0).inverted().map(restWorld);
        QPointF previousTip;
        for (Frame frame = 0; frame < scene.duration; ++frame) {
            INFO(sourceRole << " at frame " << frame);
            const auto& source = scene.layer(sourceId);
            const auto& bone = *meshBindingFor(source,
                scene.drawingAt(sourceId, frame)->id)->bone;
            const auto tip = sampleBoneJoints(bone, frame)[2];
            const auto expected = SceneRenderer::worldTransform(scene, source, frame)
                .map(QPointF(tip.x, tip.y));
            if (frame > 0) {
                const double movement = std::hypot(expected.x() - previousTip.x(),
                                                   expected.y() - previousTip.y());
                REQUIRE(movement < 4);
                if (sourceId == arm && (frame == 300 || frame == 432))
                    REQUIRE(movement < 2);
            }
            previousTip = expected;
            const auto actual = SceneRenderer::worldTransform(
                scene, scene.layer(followerId), frame).map(followerLocal);
            REQUIRE(std::hypot(expected.x() - actual.x(), expected.y() - actual.y()) < 1e-8);
        }
    }

    const std::array<Frame, 7> stills{0, 120, 240, 299, 300, 360, 479};
    std::map<Frame, QImage> frames;
    const bool fullResolution = qEnvironmentVariableIsSet("OPENTOON_HM06_VISUAL_FULL_RENDER");
    const QSize previewSize = fullResolution ? QSize{} : QSize(480, 270);
    const QSize expectedSize = fullResolution ? QSize(1920, 1080) : previewSize;
    const auto renderStart = std::chrono::steady_clock::now();
    for (Frame frame = 0; frame < scene.duration; ++frame) {
        INFO("Visual-shot preview frame " << frame);
        const auto preview = SceneRenderer::render(scene, frame, previewSize);
        REQUIRE(preview.size() == expectedSize);
        if (!fullResolution) {
            const auto [largest, ink] = connectedInk(preview);
            REQUIRE(ink > 2000);
            REQUIRE(largest == ink);
        }
    }
    if (fullResolution) {
        const auto elapsed = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - renderStart).count();
        std::fprintf(stderr, "HM-06 480-frame visual shot 1920x1080: %.2f ms/frame\n",
                     elapsed / scene.duration);
    }
    for (Frame frame : stills) {
        const auto image = SceneRenderer::render(scene, frame);
        REQUIRE(image.size() == QSize(1920, 1080));
        REQUIRE(image != QImage{});
        const auto [largest, ink] = connectedInk(image);
        REQUIRE(ink > 10000);
        REQUIRE(largest == ink);
        frames.emplace(frame, image);
        REQUIRE(image.save(QString("hm06-visual-shot-%1.png").arg(frame, 4, 10, QLatin1Char('0'))));
    }
    REQUIRE(frames.at(120) != frames.at(0));
    REQUIRE(frames.at(0) == SceneRenderer::render(shortScene, 0));
    REQUIRE(frames.at(360) != frames.at(240));
    REQUIRE(frames.at(479) != frames.at(360));
    REQUIRE(frames.at(479) == frames.at(0));
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    const auto path = std::filesystem::path((temporary.path() + "/visual-shot.otoon").toStdString());
    REQUIRE(ProjectStore::save(path, scene) > 0);
    const auto reopened = ProjectStore::load(path).document;
    REQUIRE(reopened == scene);
    for (Frame frame : stills)
        REQUIRE(SceneRenderer::render(reopened, frame) == frames.at(frame));
    if (qEnvironmentVariableIsSet("OPENTOON_HM06_VISUAL_PROJECT")) {
        const auto output = qEnvironmentVariable("OPENTOON_HM06_VISUAL_PROJECT");
        REQUIRE(ProjectStore::save(std::filesystem::path(output.toStdString()), reopened) > 0);
    }
    const auto bundled = ProjectStore::load(std::filesystem::path(
        OPENTOON_SOURCE_DIR "/examples/clockwork-visual-shot.otoon")).document;
    REQUIRE(bundled == reopened);
}

TEST_CASE("Milo uses four continuous limb images with joined bent and reopened output") {
    const QString sourceRoot = QStringLiteral(OPENTOON_SOURCE_DIR "/assets/characters/milo/");
    QFile manifestFile(sourceRoot + "continuous-parts/manifest.json");
    QFile rigFile(sourceRoot + "continuous-rig.json");
    REQUIRE(manifestFile.open(QIODevice::ReadOnly));
    REQUIRE(rigFile.open(QIODevice::ReadOnly));
    const auto manifest = QJsonDocument::fromJson(manifestFile.readAll()).object();
    const auto rig = QJsonDocument::fromJson(rigFile.readAll()).object();
    const auto entries = manifest.value("parts").toArray();
    const auto limbs = rig.value("limbs").toObject();
    REQUIRE(entries.size() == 17);
    REQUIRE(limbs.size() == 4);
    auto document = makeDocument();
    document.name = "Milo — continuous rig study";
    document.width = 1920;
    document.height = 1080;
    document.duration = 48;
    document.background = {1, 1, 1, 1};
    QHash<QString, Id> ids;
    for (const auto& entry : entries) {
        const auto part = entry.toObject();
        const auto role = part.value("id").toString();
        const QImage source(sourceRoot + "continuous-parts/" + part.value("file").toString());
        REQUIRE_FALSE(source.isNull());
        const auto image = source.convertToFormat(QImage::Format_RGBA8888);
        REQUIRE(image.size() == QSize(800, 1080));
        Layer* layer = nullptr;
        if (ids.isEmpty())
            layer = &document.layers.front();
        else {
            Layer added;
            added.id = document.allocateId();
            document.layers.push_back(added);
            layer = &document.layers.back();
        }
        layer->name = role.toStdString();
        layer->transform.x = 560;
        const Id drawing = document.allocateId();
        layer->exposures.push_back({0, document.duration, drawing});
        Drawing created;
        created.id = drawing;
        created.image = ImageAsset{800, 1080, {}};
        created.image->rgba.assign(image.constBits(), image.constBits() + image.sizeInBytes());
        document.drawings.emplace(drawing, std::move(created));
        ids.insert(role, layer->id);
    }
    const auto firstRole = entries.at(0).toObject().value("id").toString();
    const Id root = makeCharacter(document, ids.value(firstRole), "Milo");
    setPartRole(document, ids.value(firstRole), firstRole.toStdString());
    for (const auto& entry : entries) {
        const auto role = entry.toObject().value("id").toString();
        if (role != firstRole)
            attachDrawingAsPart(document, ids.value(role), root, role.toStdString());
    }
    for (const auto& entry : entries) {
        const auto part = entry.toObject();
        const auto parent = part.value("parent").toString();
        if (!parent.isEmpty())
            reparentPreservingWorld(document, ids.value(part.value("id").toString()),
                                    ids.value(parent));
    }
    document.validate();
    const auto rest = SceneRenderer::render(document, 0);
    REQUIRE(rest.size() == QSize(1920, 1080));
    const QImage master(sourceRoot + "milo.png");
    REQUIRE(master.size() == QSize(800, 1080));
    int maxChannelDifference = 0;
    for (int y = 0; y < master.height(); ++y)
        for (int x = 0; x < master.width(); ++x) {
            const QRgb reference = master.pixel(x, y);
            const QRgb actual = rest.pixel(x + 560, y);
            const int alpha = qAlpha(reference);
            const auto overWhite = [alpha](int channel) {
                return (channel * alpha + 255 * (255 - alpha) + 127) / 255;
            };
            maxChannelDifference = std::max({maxChannelDifference,
                std::abs(qRed(actual) - overWhite(qRed(reference))),
                std::abs(qGreen(actual) - overWhite(qGreen(reference))),
                std::abs(qBlue(actual) - overWhite(qBlue(reference)))});
        }
    REQUIRE(maxChannelDifference <= 4);
    const auto [restConnected, restInk] = connectedInk(rest);
    REQUIRE(restInk > 100000);
    REQUIRE(restConnected == restInk);
    for (auto iterator = limbs.begin(); iterator != limbs.end(); ++iterator) {
        const auto role = iterator.key();
        const auto config = iterator.value().toObject();
        const auto joints = config.value("joints_canvas_px").toArray();
        const auto angles = rig.value("pose_angles_deg").toObject().value(role).toArray();
        REQUIRE(joints.size() == 3);
        REQUIRE(angles.size() == 2);
        const Id part = ids.value(role);
        const Id drawing = document.layer(part).exposures.front().drawing;
        REQUIRE_NOTHROW(bindContourImageMesh(document, part, drawing, 6, 16));
        std::array<MeshPoint, 3> local{};
        for (int index = 0; index < 3; ++index) {
            const auto point = joints[index].toArray();
            local[index] = {point[0].toDouble(), point[1].toDouble()};
        }
        REQUIRE_NOTHROW(bindBoneChain(document, part, drawing, local,
                                     config.value("elbow_transition_px").toDouble()));
        const Id follower = ids.value(config.value("follower").toString());
        const auto tip = joints[2].toArray();
        setPivotPreservingArtwork(document, follower, tip[0].toDouble(), tip[1].toDouble());
        attachPartToBoneTip(document, follower, part);
        REQUIRE_NOTHROW(recordBonePose(document, part, drawing,
                                      rig.value("pose_frame").toInt(),
                                      angles[0].toDouble(), angles[1].toDouble()));
        recordBonePose(document, part, drawing, 47, 0, 0);
        REQUIRE(document.layer(follower).parent == part);
        REQUIRE(document.layer(follower).boneTipAnchor.has_value());
    }
    document.validate();
    REQUIRE(SceneRenderer::render(document, 0) == rest);
    const auto bent = SceneRenderer::render(document, 24);
    REQUIRE(bent != rest);
    const auto [bentConnected, bentInk] = connectedInk(bent);
    REQUIRE(bentConnected == bentInk);
    REQUIRE(SceneRenderer::render(document, 47) == rest);
    for (int frame = 1; frame < 47; ++frame) {
        INFO("Milo frame " << frame);
        const auto preview = SceneRenderer::render(document, frame, {480, 270});
        const auto [largest, ink] = connectedInk(preview);
        REQUIRE(ink > 5000);
        REQUIRE(largest == ink);
    }
    Session poseSession;
    poseSession.replace(document);
    const Id nearArm = ids.value("arm-near");
    const Id nearArmDrawing = document.layer(nearArm).exposures.front().drawing;
    REQUIRE(poseSession.apply("Retune Milo elbow", [&](Document& candidate) {
        recordBonePose(candidate, nearArm, nearArmDrawing, 24, 0, 38);
    }));
    const auto retuned = SceneRenderer::render(poseSession.document(), 24);
    REQUIRE(retuned != bent);
    REQUIRE(poseSession.undo());
    REQUIRE(SceneRenderer::render(poseSession.document(), 24) == bent);
    REQUIRE(poseSession.redo());
    REQUIRE(SceneRenderer::render(poseSession.document(), 24) == retuned);
    REQUIRE(rest.save("milo-rest.png"));
    REQUIRE(bent.save("milo-bent.png"));
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    const auto path = std::filesystem::path((temporary.path() + "/milo.otoon").toStdString());
    REQUIRE(ProjectStore::save(path, document) > 0);
    const auto reopened = ProjectStore::load(path).document;
    REQUIRE(reopened == document);
    REQUIRE(SceneRenderer::render(reopened, 24) == bent);
    if (qEnvironmentVariableIsSet("OPENTOON_MILO_PROJECT")) {
        const auto output = qEnvironmentVariable("OPENTOON_MILO_PROJECT");
        REQUIRE(ProjectStore::save(std::filesystem::path(output.toStdString()), document) > 0);
    } else {
        const auto bundled = ProjectStore::load(std::filesystem::path(
            OPENTOON_SOURCE_DIR "/examples/milo-continuous.otoon")).document;
        REQUIRE(bundled == document);
    }
}
