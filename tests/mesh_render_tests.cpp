#include "opentoon/deformation.h"
#include "opentoon/deformer.h"
#include "opentoon/rigging.h"
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
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>

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
    for (int y = 0; y < baseline.height(); ++y)
        if (std::memcmp(baseline.constScanLine(y), reference.constScanLine(y),
                        baseline.width() * 4) != 0) {
            for (int x = 0; x < baseline.width(); ++x)
                if (baseline.pixel(x, y) != reference.pixel(x, y)) {
                    std::fprintf(stderr, "First reference difference at %d,%d: %08x versus %08x\n",
                                 x, y, baseline.pixel(x, y), reference.pixel(x, y));
                    break;
                }
            FAIL("Original character pixels differ from reference");
        }
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
    REQUIRE(animated.copy(760, 250, 400, 500).save("hm06-bone-curve-detail.png"));
    const auto reopenedAnimated = deserializeDocument(serializeDocument(document));
    REQUIRE(SceneRenderer::render(reopenedAnimated, 12) == animated);
    auto stronger = document;
    recordBonePose(stronger, armPart, armDrawing, 24, 0, 70);
    for (const auto& role : {"lower_arm_left", "hand_left"}) {
        const Id follower = partFor(role);
        const Id followerDrawing = stronger.layer(follower).exposures.front().drawing;
        removeMeshBinding(stronger, follower, followerDrawing);
        bindRegularImageMesh(stronger, follower, followerDrawing, 4, 4);
        auto joints = meshBindingFor(stronger.layer(armPart), armDrawing)->bone->restJoints;
        const auto& armTransform = stronger.layer(armPart).transform;
        const auto& followerTransform = stronger.layer(follower).transform;
        for (auto& joint : joints) {
            joint.x += armTransform.x - followerTransform.x;
            joint.y += armTransform.y - followerTransform.y;
        }
        bindBoneChain(stronger, follower, followerDrawing, joints,
                      meshBindingFor(stronger.layer(armPart), armDrawing)->bone->elbowTransition);
        recordBonePose(stronger, follower, followerDrawing, 12, 0, 12);
        recordBonePose(stronger, follower, followerDrawing, 24, 0, 70);
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
