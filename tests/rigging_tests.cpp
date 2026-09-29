#include "opentoon/property_address.h"
#include "opentoon/character_pose.h"
#include "opentoon/rigging.h"
#include "opentoon/session.h"
#include "serialization.h"
#include "scene_renderer.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>

using namespace opentoon;

TEST_CASE("Named character poses apply only masked channels and substitutions in one undo step") {
    Session session;
    const Id body = session.document().layers.front().id;
    Id root = 0, hand = 0, alternate = 0, pose = 0;
    REQUIRE(session.apply("Assemble pose fixture", [&](Document& d) {
        root = makeCharacter(d, body, "Hero");
        Layer part;
        part.id = d.allocateId();
        hand = part.id;
        part.name = "Hand";
        d.layers.push_back(part);
        attachDrawingAsPart(d, hand, root, "Hand");
        (void)createSubstitution(d, hand, 0, false, "Closed hand");
        alternate = createSubstitution(d, hand, 0, true, "Open hand");
        d.layer(hand).transform.x = 40;
        d.layer(hand).transform.rotation = 20;
        d.layer(hand).transform.opacity = .8;
        pose = captureCharacterPose(d, root, 0,
            std::vector<PoseCaptureTarget>{{hand, PoseChannels::PositionX | PoseChannels::Drawing},
                                           {body, PoseChannels::Rotation}}, "Wave");
    }));
    REQUIRE(session.apply("Change pose", [&](Document& d) {
        d.layer(hand).transform.x = 110;
        d.layer(hand).transform.rotation = 70;
        d.layer(hand).transform.opacity = .4;
        d.layer(body).transform.rotation = 32;
        selectSubstitution(d, hand, 0, d.layer(hand).variants.front().drawing);
    }));
    const auto before = session.document();
    REQUIRE(session.apply("Apply pose", [&](Document& d) { applyCharacterPose(d, root, pose, 8); }));
    const auto result = session.document();
    REQUIRE(evaluateTransform(result.layer(hand), 8).x == 40);
    REQUIRE(evaluateTransform(result.layer(hand), 8).rotation == 70);
    REQUIRE(evaluateTransform(result.layer(hand), 8).opacity == .4);
    REQUIRE(evaluateTransform(result.layer(body), 8).rotation == 0);
    REQUIRE(result.drawingAt(hand, 8)->id == alternate);
    REQUIRE(result.drawingAt(hand, 0)->id != alternate);
    REQUIRE(session.undo());
    REQUIRE(session.document() == before);
    REQUIRE(session.redo());
    REQUIRE(session.document() == result);
}

TEST_CASE("Named poses survive duplication and remove stale part references") {
    auto document = makeDocument();
    const Id part = document.layers.front().id;
    const Id root = makeCharacter(document, part, "Hero");
    const Id pose = captureCharacterPose(document, root, 0,
        std::vector<PoseCaptureTarget>{{part, PoseChannels::AllTransforms}}, "Stand");
    publishCharacterPose(document, root, pose, true);
    const Id copy = duplicateCharacter(document, root);
    REQUIRE(document.layer(copy).poses.size() == 1);
    REQUIRE(document.layer(copy).poses.front().id != pose);
    REQUIRE(document.layer(copy).poses.front().published);
    REQUIRE(document.layer(copy).poses.front().parts.front().part != part);
    document.validate();
    removeRigBranch(document, part);
    REQUIRE(document.layer(root).poses.empty());
    REQUIRE(document.layer(copy).poses.size() == 1);
    document.validate();
}
TEST_CASE("Pose transfer maps unique Part roles and drawing names without touching source") {
    Session session;
    const Id part = session.document().layers.front().id;
    Id source = 0, target = 0, pose = 0, copiedPart = 0, alternate = 0;
    REQUIRE(session.apply("Build compatible characters", [&](Document& d) {
        source = makeCharacter(d, part, "Source");
        (void)createSubstitution(d, part, 0, false, "Closed");
        alternate = createSubstitution(d, part, 0, true, "Open");
        d.layer(part).transform.x = 80;
        pose = captureCharacterPose(d, source, 0,
            std::vector<PoseCaptureTarget>{{part, PoseChannels::PositionX | PoseChannels::Drawing}},
            "Reach");
        publishCharacterPose(d, source, pose, true);
        target = duplicateCharacter(d, source);
        copiedPart = d.layer(target).poses.front().parts.front().part;
        removeCharacterPose(d, target, d.layer(target).poses.front().id);
        editTransform(d.layer(copiedPart), 8, "x", 20, AnimationEditMode::Animate, true);
    }));
    const auto baseline = session.document();
    Id transferred = 0;
    REQUIRE(session.apply("Transfer pose", [&](Document& d) {
        transferred = transferCharacterPose(d, source, pose, target);
    }));
    REQUIRE(session.document().layer(source) == baseline.layer(source));
    const auto& copy = session.document().layer(target).poses.front();
    REQUIRE(copy.id == transferred);
    REQUIRE(copy.parts.front().part == copiedPart);
    REQUIRE(copy.parts.front().drawing != alternate);
    REQUIRE(!copy.published);
    const Id copiedDrawing = copy.parts.front().drawing;
    REQUIRE(session.apply("Apply transferred pose", [&](Document& d) {
        applyCharacterPose(d, target, transferred, 8);
    }));
    REQUIRE(evaluateTransform(session.document().layer(copiedPart), 8).x == 80);
    REQUIRE(session.document().drawingAt(copiedPart, 8)->id == copiedDrawing);
    REQUIRE(session.document().layer(source) == baseline.layer(source));
    REQUIRE(session.undo());
    REQUIRE(session.undo());
    REQUIRE(session.document() == baseline);
    REQUIRE(session.redo());
    REQUIRE(session.document().layer(target).poses.front().id == transferred);

    auto mismatched = baseline;
    mismatched.layer(copiedPart).role = "Other";
    const auto beforeFailure = mismatched;
    REQUIRE_THROWS(transferCharacterPose(mismatched, source, pose, target));
    REQUIRE(mismatched == beforeFailure);
    mismatched = baseline;
    auto variant = std::find_if(mismatched.layer(copiedPart).variants.begin(),
                                mismatched.layer(copiedPart).variants.end(),
                                [copiedDrawing](const auto& item) { return item.drawing == copiedDrawing; });
    REQUIRE(variant != mismatched.layer(copiedPart).variants.end());
    variant->name = "Different";
    const auto beforeDrawingFailure = mismatched;
    REQUIRE_THROWS(transferCharacterPose(mismatched, source, pose, target));
    REQUIRE(mismatched == beforeDrawingFailure);
    mismatched = baseline;
    mismatched.layer(copiedPart).transform.x += 1;
    const auto beforeRestFailure = mismatched;
    REQUIRE_THROWS(transferCharacterPose(mismatched, source, pose, target));
    REQUIRE(mismatched == beforeRestFailure);
}
TEST_CASE("Mirrored pose swaps paired roles and reflects masked rest deltas") {
    Session session;
    const Id left = session.document().layers.front().id;
    Id root = 0, right = 0, sourcePose = 0, rightOpen = 0;
    REQUIRE(session.apply("Build paired character", [&](Document& d) {
        root = makeCharacter(d, left, "Hero");
        d.layer(left).role = "arm_left";
        Layer part;
        part.id = d.allocateId();
        right = part.id;
        part.name = "Right arm";
        d.layers.push_back(part);
        attachDrawingAsPart(d, right, root, "arm_right");
        (void)createSubstitution(d, left, 0, false, "Closed");
        (void)createSubstitution(d, right, 0, false, "Closed");
        (void)createSubstitution(d, left, 0, true, "Open");
        rightOpen = createSubstitution(d, right, 0, false, "Open");
        d.layer(left).transform.x = -40;
        d.layer(left).transform.rotation = -10;
        d.layer(left).transform.opacity = .3;
        d.layer(right).transform.x = 40;
        d.layer(right).transform.rotation = 10;
        d.layer(right).transform.opacity = .9;
        d.layer(left).transform.x = -60;
        d.layer(left).transform.y = 8;
        d.layer(left).transform.rotation = -30;
        d.layer(left).transform.opacity = .8;
        sourcePose = captureCharacterPose(d, root, 0,
            std::vector<PoseCaptureTarget>{{left, PoseChannels::PositionX |
                                                 PoseChannels::PositionY |
                                                 PoseChannels::Rotation |
                                                 PoseChannels::Drawing}}, "Reach left");
        d.layer(left).transform.x = -40;
        d.layer(left).transform.y = 0;
        d.layer(left).transform.rotation = -10;
        d.layer(left).transform.opacity = .3;
    }));
    const auto baseline = session.document();
    Id mirrored = 0;
    REQUIRE(session.apply("Mirror pose", [&](Document& d) {
        mirrored = mirrorCharacterPose(d, root, sourcePose);
    }));
    REQUIRE(session.document().layer(left) == baseline.layer(left));
    REQUIRE(session.document().layer(right) == baseline.layer(right));
    const auto& entry = session.document().layer(root).poses.back().parts.front();
    REQUIRE(entry.part == right);
    REQUIRE(entry.channels == (PoseChannels::PositionX | PoseChannels::PositionY |
                               PoseChannels::Rotation | PoseChannels::Drawing));
    REQUIRE(entry.transform.x == 60);
    REQUIRE(entry.transform.y == 8);
    REQUIRE(entry.transform.rotation == 30);
    REQUIRE(entry.transform.opacity == .9);
    REQUIRE(entry.drawing == rightOpen);
    REQUIRE(!session.document().layer(root).poses.back().published);
    REQUIRE(session.apply("Apply mirrored pose", [&](Document& d) {
        applyCharacterPose(d, root, mirrored, 8);
    }));
    REQUIRE(evaluateTransform(session.document().layer(right), 8).x == 60);
    REQUIRE(evaluateTransform(session.document().layer(right), 8).rotation == 30);
    REQUIRE(session.document().drawingAt(right, 8)->id == rightOpen);
    REQUIRE(session.document().layer(left) == baseline.layer(left));
    REQUIRE(session.undo());
    REQUIRE(session.undo());
    REQUIRE(session.document() == baseline);

    auto invalid = baseline;
    invalid.layer(right).role = "other";
    const auto beforeMissing = invalid;
    REQUIRE_THROWS(mirrorCharacterPose(invalid, root, sourcePose));
    REQUIRE(invalid == beforeMissing);
    invalid = baseline;
    invalid.layer(right).variants.front().name = "Open";
    const auto beforeAmbiguous = invalid;
    REQUIRE_THROWS(mirrorCharacterPose(invalid, root, sourcePose));
    REQUIRE(invalid == beforeAmbiguous);
}
TEST_CASE("Published view and pose bindings stay local and outside rendered output") {
    auto document = makeDocument();
    const Id part = document.layers.front().id;
    const Id root = makeCharacter(document, part, "Hero");
    const Id mouth = createSubstitution(document, part, 0, false, "Mouth A");
    const Id view = captureCharacterView(document, root, 0, "Talk");
    const Id pose = captureCharacterPose(document, root, 0,
        std::vector<PoseCaptureTarget>{{part, PoseChannels::Rotation}}, "Turn");
    const auto before = SceneRenderer::render(document, 0, {320, 180});
    publishCharacterView(document, root, view, true);
    publishCharacterPose(document, root, pose, true);
    publishSubstitution(document, part, mouth, true);
    REQUIRE(SceneRenderer::render(document, 0, {320, 180}) == before);
    const Id copy = duplicateCharacter(document, root);
    REQUIRE(document.layer(copy).views.front().published);
    REQUIRE(document.layer(copy).poses.front().published);
    const auto copiedPart = document.layer(copy).views.front().choices.front().part;
    REQUIRE(document.layer(copiedPart).variants.front().published);
    REQUIRE(document.layer(copiedPart).variants.front().drawing != mouth);
    REQUIRE(document.layer(copy).views.front().id != view);
    REQUIRE(document.layer(copy).poses.front().id != pose);
    const auto original = document.layer(root).views.front();
    publishCharacterView(document, copy, document.layer(copy).views.front().id, false);
    publishSubstitution(document, copiedPart, document.layer(copiedPart).variants.front().drawing, false);
    REQUIRE(document.layer(root).views.front() == original);
    REQUIRE(document.layer(part).variants.front().published);
    document.validate();
}
TEST_CASE("Published control groups survive independent copies without rendering") {
    Session session;
    const Id part = session.document().layers.front().id;
    Id root = 0, view = 0, pose = 0, mouth = 0;
    REQUIRE(session.apply("Create published controls", [&](Document& d) {
        root = makeCharacter(d, part, "Hero");
        mouth = createSubstitution(d, part, 0, false, "Smile");
        view = captureCharacterView(d, root, 0, "Front");
        pose = captureCharacterPose(d, root, 0,
            std::vector<PoseCaptureTarget>{{part, PoseChannels::PositionX}}, "Reach");
        publishSubstitution(d, part, mouth, true);
        publishCharacterView(d, root, view, true);
        publishCharacterPose(d, root, pose, true);
    }));
    const auto pixels = SceneRenderer::render(session.document(), 0, {320, 180});
    const auto baseline = session.document();
    REQUIRE(session.apply("Assign control groups", [&](Document& d) {
        setSubstitutionControlGroup(d, part, mouth, "Face");
        setCharacterViewControlGroup(d, root, view, "Stage");
        setCharacterPoseControlGroup(d, root, pose, "Body");
    }));
    REQUIRE(SceneRenderer::render(session.document(), 0, {320, 180}) == pixels);
    REQUIRE(session.document().layer(part).variants.back().controlGroup == "Face");
    REQUIRE(session.document().layer(root).views.front().controlGroup == "Stage");
    REQUIRE(session.document().layer(root).poses.front().controlGroup == "Body");
    REQUIRE(session.undo());
    REQUIRE(session.document() == baseline);
    REQUIRE(session.redo());
    auto copy = session.document();
    const Id copiedRoot = duplicateCharacter(copy, root);
    const auto copiedPart = copy.layer(copiedRoot).views.front().choices.front().part;
    REQUIRE(copy.layer(copiedPart).variants.back().controlGroup == "Face");
    REQUIRE(copy.layer(copiedRoot).views.front().controlGroup == "Stage");
    REQUIRE(copy.layer(copiedRoot).poses.front().controlGroup == "Body");
    const auto beforeInvalid = copy;
    REQUIRE_THROWS(setCharacterPoseControlGroup(copy, copiedRoot,
                   copy.layer(copiedRoot).poses.front().id, ""));
    REQUIRE_THROWS(setCharacterViewControlGroup(copy, copiedRoot,
                   copy.layer(copiedRoot).views.front().id, std::string(65, 'x')));
    REQUIRE_THROWS(setSubstitutionControlGroup(copy, copiedPart,
                   copy.layer(copiedPart).variants.back().drawing, ""));
    REQUIRE(copy == beforeInvalid);
}
TEST_CASE("Pose entries can be refined per Part without broadening other masks") {
    Session session;
    const Id body = session.document().layers.front().id;
    Id root = 0, hand = 0, pose = 0;
    REQUIRE(session.apply("Build character", [&](Document& d) {
        root = makeCharacter(d, body, "Hero");
        Layer layer;
        layer.id = d.allocateId();
        hand = layer.id;
        layer.name = "Hand";
        d.layers.push_back(layer);
        attachDrawingAsPart(d, hand, root, "Hand");
        d.layer(body).transform.x = 12;
        d.layer(hand).transform.rotation = 30;
        pose = captureCharacterPose(d, root, 0,
            std::vector<PoseCaptureTarget>{{body, PoseChannels::PositionX}}, "Reach");
    }));
    REQUIRE(session.apply("Add hand rotation", [&](Document& d) {
        setCharacterPosePart(d, root, pose, hand, 0, PoseChannels::Rotation);
    }));
    REQUIRE(session.document().layer(root).poses.front().parts.size() == 2);
    const auto before = session.document();
    REQUIRE_THROWS(session.apply("Missing hand drawing", [&](Document& d) {
        setCharacterPosePart(d, root, pose, hand, 0, PoseChannels::Drawing);
    }));
    REQUIRE(session.document() == before);
    REQUIRE(session.apply("Change hand rotation", [&](Document& d) {
        d.layer(hand).transform.rotation = 70;
        d.layer(hand).transform.x = 25;
        d.layer(body).transform.x = 42;
    }));
    REQUIRE(session.apply("Apply refined pose", [&](Document& d) {
        applyCharacterPose(d, root, pose, 8);
    }));
    REQUIRE(evaluateTransform(session.document().layer(hand), 8).rotation == 30);
    REQUIRE(evaluateTransform(session.document().layer(hand), 8).x == 25);
    REQUIRE(evaluateTransform(session.document().layer(body), 8).x == 12);
    REQUIRE(session.apply("Remove hand entry", [&](Document& d) {
        removeCharacterPosePart(d, root, pose, hand);
    }));
    REQUIRE(session.document().layer(root).poses.front().parts.size() == 1);
    REQUIRE_THROWS(session.apply("Remove final entry", [&](Document& d) {
        removeCharacterPosePart(d, root, pose, body);
    }));
    REQUIRE(session.document().layer(root).poses.front().parts.size() == 1);
}
TEST_CASE("Pose blend has exact endpoints, a half-way drawing threshold and one drag undo") {
    Session session;
    const Id part = session.document().layers.front().id;
    Id root = 0, pose = 0, open = 0, closed = 0;
    REQUIRE(session.apply("Make blend fixture", [&](Document& d) {
        root = makeCharacter(d, part, "Hero");
        closed = createSubstitution(d, part, 0, false, "Closed");
        open = createSubstitution(d, part, 0, true, "Open");
        d.layer(part).transform.x = 100;
        d.layer(part).transform.opacity = .7;
        pose = captureCharacterPose(d, root, 0,
            std::vector<PoseCaptureTarget>{{part, PoseChannels::PositionX | PoseChannels::Drawing}},
            "Reach");
        selectSubstitution(d, part, 0, closed);
        d.layer(part).transform.x = 20;
        d.layer(part).transform.opacity = .3;
    }));
    const auto baseline = session.document();
    auto sample = [&](double amount) {
        auto d = baseline;
        blendCharacterPose(d, root, pose, 8, amount);
        return d;
    };
    REQUIRE(sample(0) == baseline);
    REQUIRE(evaluateTransform(sample(.25).layer(part), 8).x == 40);
    REQUIRE(sample(.49).drawingAt(part, 8)->id == closed);
    REQUIRE(sample(.5).drawingAt(part, 8)->id == open);
    REQUIRE(evaluateTransform(sample(1).layer(part), 8).x == 100);
    REQUIRE(evaluateTransform(sample(1).layer(part), 8).opacity == .3);
    REQUIRE(sample(1).drawingAt(part, 8)->id == open);
    auto invalid = baseline;
    REQUIRE_THROWS(blendCharacterPose(invalid, root, pose, 8, 1.1));
    REQUIRE(invalid == baseline);
    REQUIRE(session.applyCoalesced("Blend pose", 17, [&](Document& d) {
        blendCharacterPose(d, root, pose, 8, .25);
    }));
    REQUIRE(session.applyCoalesced("Blend pose", 17, [&](Document& d) {
        blendCharacterPose(d, root, pose, 8, .75);
    }));
    REQUIRE(session.applyCoalesced("Blend pose", 17, [&](Document& d) {
        blendCharacterPose(d, root, pose, 8, 1);
    }));
    session.endCoalesced(17);
    REQUIRE(session.document() == sample(1));
    REQUIRE(session.undo());
    REQUIRE(session.document() == baseline);
    REQUIRE(session.redo());
    REQUIRE(session.document() == sample(1));
}

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

TEST_CASE("Reparenting below a shared animated character preserves every rendered frame") {
    Session session;
    const Id body = session.document().layers.front().id;
    Id root = 0, oldPeg = 0, newPeg = 0;
    REQUIRE(session.apply("Assemble animated character", [&](Document& d) {
        root = makeCharacter(d, body, "Hero");
        auto& artwork = d.editableDrawing(body, 0);
        artwork.strokes.push_back({d.allocateId(), d.palette.front().id, 8,
                                   Shape::Rectangle, true, 2, {{25, 25}, {75, 75}}});
        expose(d.layer(body), 0, d.duration, artwork.id);
        oldPeg = addPeg(d, body, "Old peg");
        d.layer(oldPeg).transform.x = 17;
        d.layer(oldPeg).transform.y = 8;
        Layer target;
        target.id = d.allocateId();
        newPeg = target.id;
        target.name = "New peg";
        target.kind = LayerKind::Peg;
        target.parent = root;
        target.transform.x = -12;
        target.transform.y = 19;
        d.layers.push_back(target);
        d.layer(root).keys.push_back({0, {.x = 10, .y = 5, .rotation = -12},
                                      Interpolation::Linear});
        d.layer(root).keys.push_back({24, {.x = 65, .y = 12, .rotation = 24},
                                      Interpolation::Linear});
    }));
    std::vector<QImage> before;
    for (Frame frame = 0; frame < session.document().duration; ++frame)
        before.push_back(SceneRenderer::render(session.document(), frame, {320, 180}));
    auto differentlyAnimated = session.document();
    differentlyAnimated.layer(newPeg).keys.push_back({0, {}, Interpolation::Linear});
    const auto unchanged = differentlyAnimated;
    REQUIRE_THROWS(reparentPreservingWorld(differentlyAnimated, body, newPeg));
    REQUIRE(differentlyAnimated == unchanged);
    REQUIRE(session.apply("Reparent inside animated character", [&](Document& d) {
        reparentPreservingWorld(d, body, newPeg);
    }));
    REQUIRE(session.document().layer(body).parent == newPeg);
    std::size_t index = 0;
    for (Frame frame = 0; frame < session.document().duration; ++frame)
        REQUIRE(SceneRenderer::render(session.document(), frame, {320, 180}) == before[index++]);
    REQUIRE(session.undo());
    REQUIRE(session.document().layer(body).parent == oldPeg);
    REQUIRE(session.redo());
    REQUIRE(deserializeDocument(serializeDocument(session.document())) == session.document());

    auto incompatible = session.document();
    incompatible.layer(newPeg).keys.push_back({0, {}, Interpolation::Linear});
    const auto rejectedState = incompatible;
    REQUIRE_THROWS(reparentPreservingWorld(incompatible, body, oldPeg));
    REQUIRE(incompatible == rejectedState);
}

TEST_CASE("Failed opacity preserving reparent leaves the candidate unchanged") {
    auto d = makeDocument();
    const Id body = d.layers.front().id;
    const Id root = makeCharacter(d, body, "Hero");
    const Id peg = addPeg(d, body, "Opaque peg");
    Layer target;
    target.id = d.allocateId();
    target.name = "Dim peg";
    target.kind = LayerKind::Peg;
    target.parent = root;
    target.transform.opacity = 0.5;
    const Id targetId = target.id;
    d.layers.push_back(target);
    d.layer(body).transform.opacity = 0.8;
    d.layer(peg).transform.opacity = 0.8;
    const auto before = d;
    REQUIRE_THROWS(reparentPreservingWorld(d, body, targetId));
    REQUIRE(d == before);
}

TEST_CASE("Rigid reparent preserves mirrored nonuniform artwork and rejects shear") {
    auto d = makeDocument();
    const Id body = d.layers.front().id;
    const Id root = makeCharacter(d, body, "Hero");
    auto& drawing = d.editableDrawing(body, 0);
    drawing.strokes.push_back({d.allocateId(), d.palette.front().id, 6,
                               Shape::Rectangle, true, 2, {{15, 20}, {45, 65}}});
    expose(d.layer(body), 0, d.duration, drawing.id);
    const Id mirror = addPeg(d, body, "Mirror");
    d.layer(mirror).transform.x = 145;
    d.layer(mirror).transform.y = 75;
    d.layer(mirror).transform.scaleX = -1.5;
    d.layer(mirror).transform.scaleY = 0.75;
    d.layer(body).transform.x = 20;
    d.layer(body).transform.y = 10;
    const auto before = SceneRenderer::render(d, 0, {320, 180});
    reparentPreservingWorld(d, body, root);
    d.validate();
    REQUIRE(SceneRenderer::render(d, 0, {320, 180}) == before);
    REQUIRE(d.layer(body).transform.scaleX > 0);
    REQUIRE(d.layer(body).transform.scaleY < 0);
    reparentPreservingWorld(d, body, mirror);
    REQUIRE(SceneRenderer::render(d, 0, {320, 180}) == before);
    const auto stable = d;
    d.layer(body).transform.rotation = 25;
    const auto rotated = d;
    REQUIRE_THROWS(reparentPreservingWorld(d, body, root));
    REQUIRE(d == rotated);
    REQUIRE(stable.layer(body).parent == mirror);
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
