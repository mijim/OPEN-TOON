#include "opentoon/deformation.h"
#include "opentoon/rigging.h"
#include "opentoon/session.h"
#include "serialization.h"
#include <catch2/catch_test_macros.hpp>
#include <limits>

using namespace opentoon;

namespace {
struct ImagePart {
    Document document = makeDocument();
    Id part = document.layers.front().id;
    Id root = makeCharacter(document, part, "Hero");
    Id drawing = createSubstitution(document, part, 0, false, "Front");
    ImagePart() {
        document.drawings.at(drawing).image = ImageAsset{16, 16, std::vector<std::uint8_t>(16 * 16 * 4, 255)};
        document.validate();
    }
};
} // namespace

TEST_CASE("Regular substitution mesh keeps rest UV and pose separate through undo and reopen") {
    ImagePart fixture;
    Session session;
    session.replace(fixture.document);
    REQUIRE(session.apply("Bind image", [&](Document& d) {
        bindRegularImageMesh(d, fixture.part, fixture.drawing, 2, 2);
    }));
    const auto& initial = *meshBindingFor(session.document().layer(fixture.part), fixture.drawing);
    REQUIRE(initial.vertices.size() == 9);
    REQUIRE(initial.vertices[4].rest == MeshPoint{8, 8});
    REQUIRE(initial.vertices[4].uv == MeshPoint{0.5, 0.5});
    REQUIRE(initial.vertices[4].pose == initial.vertices[4].rest);
    const auto restDocument = session.document();
    REQUIRE(session.apply("Pose center", [&](Document& d) {
        moveMeshPoseVertex(d, fixture.part, fixture.drawing, 4, {9, 7});
    }));
    REQUIRE(meshBindingFor(session.document().layer(fixture.part), fixture.drawing)->vertices[4].rest ==
            MeshPoint{8, 8});
    REQUIRE(meshBindingFor(session.document().layer(fixture.part), fixture.drawing)->vertices[4].pose ==
            MeshPoint{9, 7});
    REQUIRE_THROWS(session.apply("Unsafe rebind", [&](Document& d) {
        bindRegularImageMesh(d, fixture.part, fixture.drawing, 4, 4);
    }));
    REQUIRE(session.undo());
    REQUIRE(session.document() == restDocument);
    REQUIRE(session.redo());
    REQUIRE(session.apply("Reset mesh pose", [&](Document& d) {
        resetMeshPose(d, fixture.part, fixture.drawing);
    }));
    REQUIRE(meshBindingFor(session.document().layer(fixture.part), fixture.drawing)->vertices[4].pose ==
            MeshPoint{8, 8});
    REQUIRE(session.apply("Edit rest center", [&](Document& d) {
        moveMeshRestVertex(d, fixture.part, fixture.drawing, 4, {8.5, 8});
    }));
    REQUIRE(meshBindingFor(session.document().layer(fixture.part), fixture.drawing)->vertices[4].rest ==
            MeshPoint{8.5, 8});
    REQUIRE(deserializeDocument(serializeDocument(session.document())) == session.document());
}

TEST_CASE("Mesh binding rejects folded geometry changed artwork and unsafe removal atomically") {
    ImagePart fixture;
    bindRegularImageMesh(fixture.document, fixture.part, fixture.drawing, 2, 2);
    const auto before = fixture.document;
    REQUIRE_THROWS(moveMeshPoseVertex(fixture.document, fixture.part, fixture.drawing, 4,
                                      {-100, 8}));
    REQUIRE_THROWS(moveMeshPoseVertex(fixture.document, fixture.part, fixture.drawing, 4,
                                      {std::numeric_limits<double>::quiet_NaN(), 8}));
    REQUIRE_THROWS(moveMeshPoseVertex(fixture.document, fixture.part, fixture.drawing, 99,
                                      {8, 8}));
    REQUIRE_THROWS(removeSubstitution(fixture.document, fixture.part, fixture.drawing));
    REQUIRE_THROWS(detachPart(fixture.document, fixture.part));
    REQUIRE(fixture.document == before);
    auto changedArtwork = before;
    changedArtwork.drawings.at(fixture.drawing).image->width = 8;
    REQUIRE_THROWS(changedArtwork.validate());
    auto badUv = before;
    badUv.layer(fixture.part).bindings.front().vertices[4].uv.x = 2;
    REQUIRE_THROWS(badUv.validate());
    removeMeshBinding(fixture.document, fixture.part, fixture.drawing);
    REQUIRE(meshBindingFor(fixture.document.layer(fixture.part), fixture.drawing) == nullptr);
    removeSubstitution(fixture.document, fixture.part, fixture.drawing);
    fixture.document.validate();
}

TEST_CASE("Character and branch copies keep independent substitution mesh identities") {
    ImagePart fixture;
    bindRegularImageMesh(fixture.document, fixture.part, fixture.drawing, 2, 2);
    const Id duplicate = duplicateCharacter(fixture.document, fixture.root);
    const auto& copied = fixture.document.layer(duplicate).views;
    (void)copied;
    Id copiedPart = 0;
    for (const auto& layer : fixture.document.layers)
        if (layer.kind == LayerKind::Part && layer.id != fixture.part)
            copiedPart = layer.id;
    REQUIRE(copiedPart != 0);
    const auto copiedDrawing = fixture.document.layer(copiedPart).variants.front().drawing;
    REQUIRE(copiedDrawing != fixture.drawing);
    REQUIRE(meshBindingFor(fixture.document.layer(copiedPart), copiedDrawing));
    const Id linked = duplicateRigBranch(fixture.document, fixture.part, true);
    REQUIRE(meshBindingFor(fixture.document.layer(linked), fixture.drawing));
    const Id independent = duplicateRigBranch(fixture.document, fixture.part, false);
    const Id independentDrawing = fixture.document.layer(independent).variants.front().drawing;
    REQUIRE(independentDrawing != fixture.drawing);
    REQUIRE(meshBindingFor(fixture.document.layer(independent), independentDrawing));
    fixture.document.validate();
    REQUIRE(deserializeDocument(serializeDocument(fixture.document)) == fixture.document);
}
