#include "opentoon/deformer.h"
#include "opentoon/deformation.h"
#include "opentoon/rigging.h"
#include "opentoon/session.h"
#include "opentoon/timeline.h"
#include "serialization.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <numbers>

using namespace opentoon;

namespace {
struct PartFixture {
    Document document = makeDocument();
    Id part = document.layers.front().id;
    Id root = makeCharacter(document, part, "Chain");
    Id drawing = createSubstitution(document, part, 0, false, "Front");
    PartFixture() {
        document.drawings.at(drawing).image =
            ImageAsset{16, 16, std::vector<std::uint8_t>(16 * 16 * 4, 255)};
        bindRegularImageMesh(document, part, drawing, 4, 2);
        document.validate();
    }
};
} // namespace

TEST_CASE("Two-segment bone keys retain elbow connection, rest and undo") {
    PartFixture fixture;
    Session session;
    session.replace(fixture.document);
    REQUIRE(session.apply("Bind bone", [&](Document& document) {
        bindBoneChain(document, fixture.part, fixture.drawing,
                      {{{0, 8}, {8, 8}, {16, 8}}}, 3);
    }));
    REQUIRE(session.apply("Bend elbow", [&](Document& document) {
        recordBonePose(document, fixture.part, fixture.drawing, 12, 0, 30);
    }));
    const auto& binding = *meshBindingFor(session.document().layer(fixture.part), fixture.drawing);
    REQUIRE(binding.bone->keys.size() == 2);
    REQUIRE(binding.bone->keys.front().frame == 0);
    REQUIRE(binding.bone->keys.back().frame == 12);
    REQUIRE(evaluateMeshBinding(binding, 0).vertices == binding.vertices);
    const auto posed = evaluateMeshBinding(binding, 12);
    REQUIRE(posed.vertices[7].pose == MeshPoint{8, 8});
    REQUIRE(std::abs(posed.vertices[9].pose.x - (8 + 8 * std::cos(std::numbers::pi / 6))) < 1e-9);
    REQUIRE(std::abs(posed.vertices[9].pose.y - 12) < 1e-9);
    const auto middle = evaluateMeshBinding(binding, 6);
    REQUIRE(middle.vertices[9].pose.y > 8);
    REQUIRE(middle.vertices[9].pose.y < posed.vertices[9].pose.y);
    REQUIRE(deserializeDocument(serializeDocument(session.document())) == session.document());
    REQUIRE(session.undo());
    REQUIRE(meshBindingFor(session.document().layer(fixture.part), fixture.drawing)->bone->keys.empty());
    REQUIRE(session.redo());
    REQUIRE(meshBindingFor(session.document().layer(fixture.part), fixture.drawing)->bone->keys.size() == 2);
}

TEST_CASE("Cubic curve tangent keys move a field continuously without altering rest") {
    PartFixture fixture;
    const std::array<MeshPoint, 4> straight{{{0, 8}, {16.0 / 3, 8},
                                            {32.0 / 3, 8}, {16, 8}}};
    bindCurveDeformer(fixture.document, fixture.part, fixture.drawing, straight);
    auto lifted = straight;
    lifted[1].y = 3;
    lifted[2].y = 10;
    recordCurvePose(fixture.document, fixture.part, fixture.drawing, 12, lifted);
    const auto& binding = *meshBindingFor(fixture.document.layer(fixture.part), fixture.drawing);
    REQUIRE(binding.curve->keys.size() == 2);
    REQUIRE(evaluateMeshBinding(binding, 0).vertices == binding.vertices);
    const auto atSix = evaluateMeshBinding(binding, 6);
    const auto atTwelve = evaluateMeshBinding(binding, 12);
    REQUIRE(atTwelve.vertices[6].pose.y < atSix.vertices[6].pose.y);
    REQUIRE(atSix.vertices[6].pose.y < binding.vertices[6].rest.y);
    REQUIRE(atTwelve.vertices[6].rest == binding.vertices[6].rest);
    REQUIRE(deserializeDocument(serializeDocument(fixture.document)) == fixture.document);
}

TEST_CASE("Deformer binding rejects invalid controls and protected static edits atomically") {
    PartFixture fixture;
    const auto before = fixture.document;
    REQUIRE_THROWS(bindBoneChain(fixture.document, fixture.part, fixture.drawing,
                                  {{{0, 0}, {0, 0}, {16, 8}}}, 3));
    REQUIRE(fixture.document == before);
    bindBoneChain(fixture.document, fixture.part, fixture.drawing,
                  {{{0, 8}, {8, 8}, {16, 8}}}, 3);
    const auto bound = fixture.document;
    REQUIRE_THROWS(moveMeshRestVertex(fixture.document, fixture.part, fixture.drawing, 0, {1, 1}));
    REQUIRE_THROWS(moveMeshPoseVertex(fixture.document, fixture.part, fixture.drawing, 0, {1, 1}));
    REQUIRE_THROWS(bindRegularImageMesh(fixture.document, fixture.part, fixture.drawing, 2, 2));
    REQUIRE_THROWS(recordBonePose(fixture.document, fixture.part, fixture.drawing, 12,
                                   std::numeric_limits<double>::quiet_NaN(), 0));
    REQUIRE_THROWS(recordBonePose(fixture.document, fixture.part, fixture.drawing, 12,
                                   0, 180));
    REQUIRE(fixture.document == bound);
    auto damaged = bound;
    damaged.layer(fixture.part).bindings.front().bone->distalWeights[0] = 1.5;
    REQUIRE_THROWS(damaged.validate());
    removeMeshDeformer(fixture.document, fixture.part, fixture.drawing);
    REQUIRE_FALSE(meshBindingFor(fixture.document.layer(fixture.part), fixture.drawing)->bone);
    fixture.document.validate();
}

TEST_CASE("Frame insertion removal and Clear keep deformer keys synchronized") {
    PartFixture fixture;
    bindBoneChain(fixture.document, fixture.part, fixture.drawing,
                  {{{0, 8}, {8, 8}, {16, 8}}}, 3);
    recordBonePose(fixture.document, fixture.part, fixture.drawing, 12, 0, 20);
    const Id side = createSubstitution(fixture.document, fixture.part, 0, true, "Side");
    bindRegularImageMesh(fixture.document, fixture.part, side, 4, 2);
    const std::array<MeshPoint, 4> straight{{{0, 8}, {16.0 / 3, 8},
                                            {32.0 / 3, 8}, {16, 8}}};
    bindCurveDeformer(fixture.document, fixture.part, side, straight);
    auto curved = straight;
    curved[1].y += 2;
    recordCurvePose(fixture.document, fixture.part, side, 16, curved);
    const auto before = fixture.document;
    auto clip = copyRange(fixture.document, {fixture.part}, 10, 17);
    REQUIRE(clip.tracks.front().containsDeformerKeys);
    REQUIRE_THROWS(pasteRange(fixture.document, {fixture.part}, 20, clip,
                              PasteContent::All, true));
    REQUIRE_THROWS(retimeRange(fixture.document, {fixture.part}, 10, 17, 10));
    REQUIRE(fixture.document == before);
    insertFrames(fixture.document, 10, 3);
    const auto& inserted = fixture.document.layer(fixture.part);
    REQUIRE(meshBindingFor(inserted, fixture.drawing)->bone->keys.back().frame == 15);
    REQUIRE(meshBindingFor(inserted, side)->curve->keys.back().frame == 19);
    removeFrames(fixture.document, 14, 2);
    const auto& removed = fixture.document.layer(fixture.part);
    REQUIRE(meshBindingFor(removed, fixture.drawing)->bone->keys.size() == 1);
    REQUIRE(meshBindingFor(removed, side)->curve->keys.back().frame == 17);
    fixture.document.validate();
    Session session;
    session.replace(fixture.document);
    REQUIRE(session.apply("Clear deformation", [&](Document& document) {
        clearRange(document, {fixture.part}, 16, 18, true);
    }));
    REQUIRE(meshBindingFor(session.document().layer(fixture.part), side)->curve->keys.size() == 1);
    REQUIRE(session.undo());
    REQUIRE(session.document() == fixture.document);
}
