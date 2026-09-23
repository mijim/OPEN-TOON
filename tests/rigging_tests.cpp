#include "opentoon/property_address.h"
#include "opentoon/rigging.h"
#include "opentoon/session.h"
#include "serialization.h"
#include "scene_renderer.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace opentoon;

TEST_CASE("Character assembly preserves registered artwork through peg and pivot edits") {
    Session session;
    const Id body = session.document().layers.front().id;
    REQUIRE(session.apply("Draw body", [&](Document& d) {
        auto& drawing = d.editableDrawing(body, 0);
        drawing.strokes.push_back({d.allocateId(), d.palette.front().id, 8, Shape::Rectangle, true, 2,
                                   {{30, 40}, {90, 110}}});
        d.layer(body).transform.x = 120;
        d.layer(body).transform.rotation = 16;
    }));
    const auto before = SceneRenderer::render(session.document(), 0, {320, 180});
    Id root = 0, peg = 0;
    REQUIRE(session.apply("Build character", [&](Document& d) {
        root = makeCharacter(d, body, "Hero");
        peg = addPeg(d, body, "Torso peg");
        setPartRole(d, body, "Torso");
        setPivotPreservingArtwork(d, body, 60, 80);
    }));
    REQUIRE(session.document().layer(body).kind == LayerKind::Part);
    REQUIRE(session.document().layer(body).parent == peg);
    REQUIRE(session.document().layer(peg).parent == root);
    REQUIRE(session.document().layer(body).role == "Torso");
    REQUIRE(SceneRenderer::render(session.document(), 0, {320, 180}) == before);
    auto typed = PropertyAddress{body, PropertyKind::Rotation};
    typed.entity = PropertyEntityKind::Part;
    REQUIRE(propertyValue(session.document(), typed, 0, PropertySource::Rest) == 16);
    typed.entity = PropertyEntityKind::Peg;
    REQUIRE_THROWS(propertyValue(session.document(), typed, 0, PropertySource::Rest));
    REQUIRE(session.apply("Move peg", [&](Document& d) {
        d.layer(peg).transform.x = 25;
        d.layer(peg).transform.y = -14;
    }));
    const auto beforeReparent = SceneRenderer::render(session.document(), 0, {320, 180});
    REQUIRE(session.apply("Reparent part", [&](Document& d) {
        reparentPreservingWorld(d, body, root);
    }));
    REQUIRE(SceneRenderer::render(session.document(), 0, {320, 180}) == beforeReparent);
    REQUIRE(session.undo());
    REQUIRE(session.document().layer(body).parent == peg);
    REQUIRE(session.redo());
    REQUIRE(session.document().layer(body).parent == root);
}

TEST_CASE("Registered parts attach without jumps and reject singular or sheared reparenting") {
    auto d = makeDocument();
    const Id body = d.layers.front().id;
    const Id root = makeCharacter(d, body, "Hero");
    Layer hand;
    hand.id = d.allocateId();
    const Id handId = hand.id;
    hand.name = "Hand";
    hand.transform.x = 52;
    hand.transform.y = 21;
    d.layers.push_back(hand);
    Layer peg;
    peg.id = d.allocateId();
    const Id pegId = peg.id;
    peg.name = "Arm";
    peg.kind = LayerKind::Peg;
    peg.parent = root;
    peg.transform.x = 10;
    peg.transform.y = -3;
    d.layers.push_back(peg);
    attachDrawingAsPart(d, handId, pegId, "Hand");
    auto local = d.layer(handId).transform;
    REQUIRE(std::abs(local.x - 42) < 1e-8);
    REQUIRE(std::abs(local.y - 24) < 1e-8);
    REQUIRE(d.layer(handId).role == "Hand");
    d.layer(pegId).transform.scaleX = 0;
    const auto before = d;
    REQUIRE_THROWS(reparentPreservingWorld(d, handId, root));
    REQUIRE(d == before);
    d.layer(pegId).transform.scaleX = 2;
    d.layer(pegId).transform.rotation = 30;
    d.layer(handId).transform.rotation = 17;
    d.layer(handId).transform.scaleX = 1.5;
    REQUIRE_THROWS(reparentPreservingWorld(d, handId, root));
    d.layer(pegId).transform = {};
    d.layer(handId).transform = {};
    d.layer(handId).keys.push_back({0, {}, Interpolation::Linear});
    const auto animated = d;
    REQUIRE_THROWS(reparentPreservingWorld(d, handId, root));
    REQUIRE_THROWS(setPivotPreservingArtwork(d, handId, 12, 15));
    REQUIRE(d == animated);
}

TEST_CASE("Named substitutions switch held drawings without changing pose and reopen identically") {
    Session session;
    const Id partId = session.document().layers.front().id;
    Id mouthA = 0, mouthB = 0;
    REQUIRE(session.apply("Create mouth rig", [&](Document& d) {
        makeCharacter(d, partId, "Speaker");
        setPartRole(d, partId, "Mouth");
        mouthA = createSubstitution(d, partId, 0, false, "Closed");
        mouthB = createSubstitution(d, partId, 12, true, "Open");
        d.layer(partId).transform.x = 25;
    }));
    REQUIRE(session.document().drawingAt(partId, 0)->id == mouthA);
    REQUIRE(session.document().drawingAt(partId, 11)->id == mouthA);
    REQUIRE(session.document().drawingAt(partId, 12)->id == mouthB);
    REQUIRE(session.apply("Retiming and naming", [&](Document& d) {
        renameSubstitution(d, partId, mouthB, "Wide open");
        selectSubstitution(d, partId, 4, mouthB);
    }));
    REQUIRE(session.document().drawingAt(partId, 4)->id == mouthB);
    REQUIRE(session.document().drawingAt(partId, 3)->id == mouthA);
    REQUIRE(session.document().layer(partId).transform.x == 25);
    REQUIRE(session.document().layer(partId).variants.back().name == "Wide open");
    REQUIRE(session.apply("Remove closed", [&](Document& d) { removeSubstitution(d, partId, mouthA); }));
    REQUIRE(session.document().drawingAt(partId, 0)->id == mouthB);
    REQUIRE(session.undo());
    REQUIRE(session.document().drawingAt(partId, 0)->id == mouthA);
    REQUIRE(session.redo());
    const auto serialized = serializeDocument(session.document());
    REQUIRE(deserializeDocument(serialized) == session.document());
    auto invalid = session.document();
    REQUIRE_THROWS(selectSubstitution(invalid, partId, 0, 999999));
}

TEST_CASE("Character views coordinate held part choices and reject incomplete changes atomically") {
    Session session;
    const Id mouth = session.document().layers.front().id;
    Id root = 0, hand = 0, mouthClosed = 0, mouthOpen = 0, handDown = 0, handUp = 0;
    REQUIRE(session.apply("Assemble", [&](Document& d) {
        root = makeCharacter(d, mouth, "Hero");
        setPartRole(d, mouth, "Mouth");
        mouthClosed = createSubstitution(d, mouth, 0, false, "Closed");
        mouthOpen = createSubstitution(d, mouth, 10, true, "Open");
        Layer source;
        source.id = d.allocateId();
        hand = source.id;
        source.name = "Hand";
        d.layers.push_back(source);
        attachDrawingAsPart(d, hand, root, "Hand");
        handDown = createSubstitution(d, hand, 0, false, "Down");
        handUp = createSubstitution(d, hand, 10, true, "Up");
    }));
    Id front = 0, raised = 0;
    REQUIRE(session.apply("Capture views", [&](Document& d) {
        front = captureCharacterView(d, root, 0, "Front");
        raised = captureCharacterView(d, root, 10, "Raised");
    }));
    REQUIRE(session.document().layer(root).views.size() == 2);
    REQUIRE(session.apply("Switch both parts", [&](Document& d) {
        applyCharacterView(d, root, raised, 4);
    }));
    REQUIRE(session.document().drawingAt(mouth, 4)->id == mouthOpen);
    REQUIRE(session.document().drawingAt(hand, 4)->id == handUp);
    REQUIRE(session.document().drawingAt(mouth, 3)->id == mouthClosed);
    REQUIRE(session.document().drawingAt(hand, 3)->id == handDown);
    REQUIRE(session.undo());
    REQUIRE(session.document().drawingAt(mouth, 4)->id == mouthClosed);
    REQUIRE(session.redo());
    auto locked = session.document();
    locked.layer(hand).locked = true;
    REQUIRE_THROWS(applyCharacterView(locked, root, front, 4));
    REQUIRE(locked.drawingAt(mouth, 4)->id == mouthOpen);
    auto incomplete = session.document();
    incomplete.layer(root).views.front().choices.pop_back();
    REQUIRE_THROWS(applyCharacterView(incomplete, root, front, 4));
    REQUIRE(incomplete.drawingAt(mouth, 4)->id == mouthOpen);
    REQUIRE_THROWS(removeSubstitution(incomplete, mouth, mouthClosed));
    REQUIRE(session.apply("Manage named views", [&](Document& d) {
        renameCharacterView(d, root, front, "Neutral");
        const Id copy = duplicateCharacterView(d, root, raised);
        updateCharacterView(d, root, copy, 4);
        removeCharacterView(d, root, copy);
        reorderSubstitution(d, mouth, mouthOpen, -1);
        REQUIRE(stepSubstitution(d, mouth, 8, 1) == mouthClosed);
    }));
    REQUIRE(session.document().layer(root).views.front().name == "Neutral");
    REQUIRE(session.document().drawingAt(mouth, 8)->id == mouthClosed);
    REQUIRE(deserializeDocument(serializeDocument(session.document())) == session.document());
}

TEST_CASE("Duplicating a character remaps every part drawing and view without changing the source") {
    auto d = makeDocument();
    const Id body = d.layers.front().id;
    const Id root = makeCharacter(d, body, "Hero");
    const Id drawingA = createSubstitution(d, body, 0, false, "A");
    const Id drawingB = createSubstitution(d, body, 10, true, "B");
    const Id view = captureCharacterView(d, root, 0, "Front");
    d.drawings.at(drawingA).strokes.push_back({d.allocateId(), d.palette.front().id, 8,
                                               Shape::Rectangle, true, 2, {{10, 10}, {30, 30}}});
    d.validate();
    const auto originalFrame = SceneRenderer::render(d, 0, {320, 180});
    const Id clone = duplicateCharacter(d, root);
    d.validate();
    REQUIRE(d.layer(clone).name == "Hero copy");
    REQUIRE(d.layer(clone).transform.x == d.layer(root).transform.x + 64);
    REQUIRE(d.layer(clone).views.size() == 1);
    REQUIRE(d.layer(clone).views.front().id != view);
    const Id clonedPart = d.layer(clone).views.front().choices.front().part;
    const Id clonedDrawing = d.layer(clone).views.front().choices.front().drawing;
    REQUIRE(clonedPart != body);
    REQUIRE(clonedDrawing != drawingA);
    REQUIRE(d.drawingAt(clonedPart, 0)->id == clonedDrawing);
    REQUIRE(d.layer(clonedPart).variants.size() == 2);
    REQUIRE(d.layer(clonedPart).variants.back().drawing != drawingB);
    d.drawings.at(clonedDrawing).strokes.front().points.front().x += 15;
    REQUIRE(d.drawings.at(drawingA).strokes.front().points.front().x == 10);
    applyCharacterView(d, clone, d.layer(clone).views.front().id, 12);
    REQUIRE(d.drawingAt(body, 12)->id == drawingB);
    REQUIRE(d.drawingAt(clonedPart, 12)->id == clonedDrawing);
    REQUIRE(SceneRenderer::render(d, 0, {320, 180}) != originalFrame);
    REQUIRE(deserializeDocument(serializeDocument(d)) == d);
}

TEST_CASE("Batch assembly registers new parts in saved views without moving artwork") {
    Session session;
    const Id body = session.document().layers.front().id;
    Id root = 0, hand = 0, empty = 0, view = 0;
    REQUIRE(session.apply("Prepare imported parts", [&](Document& d) {
        root = makeCharacter(d, body, "Hero");
        createSubstitution(d, body, 0, false, "Body");
        view = captureCharacterView(d, root, 0, "Front");
        Layer source;
        source.id = d.allocateId();
        hand = source.id;
        source.name = "Hand";
        source.transform.x = 40;
        d.layers.push_back(source);
        d.editableDrawing(hand, 0).strokes.push_back(
            {d.allocateId(), d.palette.front().id, 4, Shape::Rectangle, true, 2,
             {{5, 5}, {25, 25}}});
        Layer blank;
        blank.id = d.allocateId();
        empty = blank.id;
        blank.name = "Unexposed guide";
        d.layers.push_back(blank);
    }));
    const auto before = SceneRenderer::render(session.document(), 0, {320, 180});
    REQUIRE(session.apply("Assemble exposed drawings", [&](Document& d) {
        REQUIRE(attachUnparentedDrawings(d, root, 0) == 1);
    }));
    REQUIRE(session.document().layer(hand).kind == LayerKind::Part);
    REQUIRE(session.document().layer(hand).role == "Hand");
    REQUIRE(session.document().layer(empty).kind == LayerKind::Drawing);
    REQUIRE(session.document().layer(root).views.front().choices.size() == 2);
    REQUIRE(SceneRenderer::render(session.document(), 0, {320, 180}) == before);
    REQUIRE(session.undo());
    REQUIRE(session.document().layer(hand).kind == LayerKind::Drawing);
    REQUIRE(session.redo());
    REQUIRE(session.document().layer(root).views.front().id == view);
    REQUIRE(deserializeDocument(serializeDocument(session.document())) == session.document());
}

TEST_CASE("Rig branch copies keep view membership and choose independent or linked artwork") {
    Session session;
    const Id body = session.document().layers.front().id;
    Id root = 0, peg = 0, drawing = 0, view = 0;
    REQUIRE(session.apply("Build source", [&](Document& d) {
        root = makeCharacter(d, body, "Hero");
        drawing = createSubstitution(d, body, 0, false, "Body");
        peg = addPeg(d, body, "Torso peg");
        view = captureCharacterView(d, root, 0, "Front");
    }));
    Id independent = 0, linked = 0, copiedPeg = 0;
    REQUIRE(session.apply("Copy branches", [&](Document& d) {
        independent = duplicateRigBranch(d, body, false);
        linked = duplicateRigBranch(d, body, true);
        copiedPeg = duplicateRigBranch(d, peg, false);
        applyCharacterView(d, root, view, 5);
    }));
    REQUIRE(session.document().drawingAt(independent, 0)->id != drawing);
    REQUIRE(session.document().drawingAt(linked, 0)->id == drawing);
    REQUIRE(session.document().layer(copiedPeg).kind == LayerKind::Peg);
    REQUIRE(session.document().layer(root).views.front().choices.size() == 6);
    REQUIRE(session.document().layer(independent).parent == peg);
    REQUIRE(session.apply("Delete copied limb", [&](Document& d) {
        removeRigBranch(d, copiedPeg);
    }));
    REQUIRE(session.document().layer(root).views.front().choices.size() == 3);
    REQUIRE_THROWS(session.document().layer(copiedPeg));
    REQUIRE(session.undo());
    REQUIRE(session.document().layer(copiedPeg).kind == LayerKind::Peg);
    REQUIRE(deserializeDocument(serializeDocument(session.document())) == session.document());
}

TEST_CASE("Detaching a part and dissolving its peg preserve registration and view validity") {
    auto d = makeDocument();
    const Id body = d.layers.front().id;
    const Id root = makeCharacter(d, body, "Hero");
    const Id artwork = createSubstitution(d, body, 0, false, "Body");
    d.drawings.at(artwork).strokes.push_back(
        {d.allocateId(), d.palette.front().id, 6, Shape::Rectangle, true, 2,
         {{15, 15}, {50, 50}}});
    const Id peg = addPeg(d, body, "Body peg");
    d.layer(peg).transform.x = 35;
    d.layer(peg).transform.y = 14;
    const Id view = captureCharacterView(d, root, 0, "Front");
    const auto before = SceneRenderer::render(d, 0, {320, 180});
    dissolvePeg(d, peg);
    REQUIRE(d.layer(body).parent == root);
    REQUIRE(SceneRenderer::render(d, 0, {320, 180}) == before);
    detachPart(d, body);
    REQUIRE(d.layer(body).kind == LayerKind::Drawing);
    REQUIRE(d.layer(root).views.front().id == view);
    REQUIRE(d.layer(root).views.front().choices.empty());
    REQUIRE(SceneRenderer::render(d, 0, {320, 180}) == before);
    REQUIRE(deserializeDocument(serializeDocument(d)) == d);
}

TEST_CASE("View ranges and single-part updates preserve outside frames and order") {
    auto d = makeDocument();
    const Id body = d.layers.front().id;
    const Id root = makeCharacter(d, body, "Hero");
    const Id frontDrawing = createSubstitution(d, body, 0, false, "Front");
    const Id sideDrawing = createSubstitution(d, body, 10, true, "Side");
    const Id front = captureCharacterView(d, root, 0, "Front");
    const Id side = captureCharacterView(d, root, 10, "Side");
    applyCharacterViewRange(d, root, front, 20, 30);
    REQUIRE(d.drawingAt(body, 19)->id == sideDrawing);
    REQUIRE(d.drawingAt(body, 20)->id == frontDrawing);
    REQUIRE(d.drawingAt(body, 29)->id == frontDrawing);
    REQUIRE(d.drawingAt(body, 30)->id == sideDrawing);
    auto locked = d;
    locked.layer(body).locked = true;
    REQUIRE_THROWS(applyCharacterViewRange(locked, root, side, 20, 30));
    REQUIRE(locked.drawingAt(body, 20)->id == frontDrawing);
    updateCharacterViewPart(d, root, front, body, 10);
    REQUIRE(d.layer(root).views.front().choices.front().drawing == sideDrawing);
    reorderCharacterView(d, root, side, -1);
    REQUIRE(d.layer(root).views.front().id == side);
    REQUIRE_THROWS(applyCharacterViewRange(d, root, front, 30, 20));
    REQUIRE(deserializeDocument(serializeDocument(d)) == d);
}
