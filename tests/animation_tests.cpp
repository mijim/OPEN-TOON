#include "opentoon/animation.h"
#include "opentoon/key_block.h"
#include "opentoon/session.h"
#include "serialization.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
using namespace opentoon;
TEST_CASE("Setup never creates keys and animate requires explicit autokey") {
    Session session;
    auto id = session.document().layers.front().id;
    session.apply(
        "Rest", [&](Document& d) { editTransform(d.layer(id), 0, "x", 40, AnimationEditMode::Setup, true); });
    REQUIRE(session.document().layer(id).keys.empty());
    REQUIRE(evaluateTransform(session.document().layer(id), 0).x == 40);
    auto before = session.document();
    REQUIRE_THROWS(session.apply("Unkeyed", [&](Document& d) {
        editTransform(d.layer(id), 4, "x", 90, AnimationEditMode::Animate, false);
    }));
    REQUIRE(session.document() == before);
    session.apply("Auto key", [&](Document& d) {
        editTransform(d.layer(id), 4, "x", 90, AnimationEditMode::Animate, true);
    });
    session.apply("Rest only", [&](Document& d) {
        editTransform(d.layer(id), 4, "x", 12, AnimationEditMode::Setup, false);
    });
    REQUIRE(session.document().layer(id).transform.x == 12);
    REQUIRE(evaluateTransform(session.document().layer(id), 4).x == 90);
    session.apply("Existing key", [&](Document& d) {
        editTransform(d.layer(id), 4, "y", 70, AnimationEditMode::Animate, false);
    });
    REQUIRE(session.document().layer(id).keys.size() == 1);
    REQUIRE(evaluateTransform(session.document().layer(id), 4).y == 70);
    session.undo();
    REQUIRE(evaluateTransform(session.document().layer(id), 4).y == 0);
}
TEST_CASE("Key-only retiming preserves artwork and rejects collisions across layers atomically") {
    auto d = makeBouncingBall();
    const auto id = d.layers.front().id;
    d.layer(id).keys = {{0, {}, Interpolation::Smooth},
                        {5, {}, Interpolation::Step},
                        {10, {}, Interpolation::Linear},
                        {40, {}, Interpolation::Linear}};
    const auto artwork = d.drawings;
    const auto exposures = d.layer(id).exposures;
    retimeKeys(d, {id}, 0, 11, 12, 21);
    REQUIRE(d.layer(id).keys[0].frame == 12);
    REQUIRE(d.layer(id).keys[1].frame == 22);
    REQUIRE(d.layer(id).keys[2].frame == 32);
    REQUIRE(d.layer(id).keys[3].frame == 40);
    REQUIRE(d.drawings == artwork);
    REQUIRE(d.layer(id).exposures == exposures);
    auto before = d;
    REQUIRE_THROWS(retimeKeys(d, {id}, 12, 33, 20, 21));
    REQUIRE(d == before);
    REQUIRE_THROWS(retimeKeys(d, {id}, 12, 33, 0, 2));
    REQUIRE(d == before);
    auto second = d.layer(id);
    second.id = d.allocateId();
    second.locked = true;
    d.layers.push_back(second);
    before = d;
    REQUIRE_THROWS(retimeKeys(d, {id, second.id}, 12, 33, 0, 11));
    REQUIRE(d == before);
}
TEST_CASE("Curve edits retain other channel values and round trip held and smooth segments") {
    auto d = makeDocument();
    auto& l = d.layers.front();
    Transform first;
    first.x = -20;
    first.y = 30;
    first.scaleX = -1;
    Transform last = first;
    last.x = 80;
    last.scaleX = 1;
    l.keys = {{0, first, Interpolation::Linear}, {20, last, Interpolation::Linear}};
    editKey(l, 20, 24, "x", 100, Interpolation::Step);
    REQUIRE(l.keys.back().value.y == 30);
    editKey(l, 0, 0, "x", -20, Interpolation::Smooth);
    REQUIRE(evaluateTransform(l, 12).x == Catch::Approx(40));
    REQUIRE(evaluateTransform(l, 6).x == Catch::Approx(-1.25));
    REQUIRE(evaluateTransform(l, 12).scaleX == 0);
    auto reopened = deserializeDocument(serializeDocument(d));
    for (Frame f = 0; f < d.duration; ++f)
        REQUIRE(evaluateTransform(reopened.layers.front(), f) == evaluateTransform(l, f));
    REQUIRE_THROWS(editKey(l, 24, 0, "x", 50, Interpolation::Linear));
    l.locked = true;
    REQUIRE_THROWS(editKey(l, 24, 25, "x", 50, Interpolation::Linear));
}

TEST_CASE("Visual poses anchor the start and preserve artwork, easing and undo") {
    Session session;
    const auto id = session.document().layers.front().id;
    const auto before = session.document();
    Transform pose;
    pose.x = 180;
    pose.rotation = 25;
    pose.scaleX = 1.5;
    session.apply("Pose", [&](Document& d) { recordPose(d.layer(id), 24, pose); });
    const auto& layer = session.document().layer(id);
    REQUIRE(layer.keys.size() == 2);
    REQUIRE(evaluateTransform(layer, 0) == before.layer(id).transform);
    REQUIRE(evaluateTransform(layer, 12).x == Catch::Approx(90));
    REQUIRE(session.document().drawings == before.drawings);
    session.undo();
    REQUIRE(session.document() == before);
    auto locked = before.layer(id);
    locked.locked = true;
    REQUIRE_THROWS(recordPose(locked, 24, pose));
}
TEST_CASE("Per-channel Bezier curves overshoot deterministically and survive saving and retiming") {
    auto d = makeDocument();
    auto& layer = d.layers.front();
    Transform target;
    target.x = 100;
    target.y = 200;
    target.opacity = .5;
    recordPose(layer, 20, target);
    const BezierEase ease{.25, 0, .65, 1.8};
    setKeyEase(layer, 0, "x", ease);
    REQUIRE(evaluateTransform(layer, 15).x > 100);
    REQUIRE(evaluateTransform(layer, 10).y == Catch::Approx(100));
    REQUIRE(evaluateTransform(layer, 0).x == 0);
    REQUIRE(evaluateTransform(layer, 20).x == 100);
    setKeyEase(layer, 0, "opacity", {.1, -4, .9, 4});
    for (int f = 0; f <= 20; ++f) {
        auto opacity = evaluateTransform(layer, f).opacity;
        REQUIRE(opacity >= 0);
        REQUIRE(opacity <= 1);
    }
    auto reopened = deserializeDocument(serializeDocument(d));
    REQUIRE(reopened == d);
    auto original = layer;
    retimeKeys(d, {layer.id}, 0, 21, 0, 41);
    for (int f = 0; f <= 20; ++f)
        REQUIRE(evaluateTransform(d.layers.front(), f * 2) == evaluateTransform(original, f));
    recordPose(d.layers.front(), 0, {});
    REQUIRE(d.layers.front().keys.front().easing.at("x") == ease);
    REQUIRE_THROWS(setKeyEase(d.layers.front(), 0, "x", {.8, 0, .2, 1}));
    REQUIRE_THROWS(setKeyEase(d.layers.front(), 0, "unknown", ease));
    REQUIRE_THROWS(setKeyEase(d.layers.front(), 40, "x", ease));
    d.layers.front().keys.front().easing["x"].y1 = 5;
    REQUIRE_THROWS(d.validate());
}

TEST_CASE("Combined pose timing and easing preserve artwork and edit every channel atomically") {
    Session session;
    auto id = session.document().layers.front().id;
    Transform pose;
    pose.x = 90;
    pose.y = -40;
    pose.rotation = 70;
    pose.scaleX = 2;
    pose.opacity = .3;
    session.apply("Pose", [&](Document& d) { recordPose(d.layer(id), 20, pose); });
    session.apply("Ease all", [&](Document& d) { setPoseEase(d.layer(id), 0, {.25, 0, .65, 1.8}); });
    auto before = session.document();
    REQUIRE(before.layer(id).keys.front().easing.size() == 8);
    session.apply("Retime pose", [&](Document& d) { movePoseKey(d.layer(id), 20, 30); });
    REQUIRE(session.document().layer(id).keys.back().value == pose);
    REQUIRE(session.document().layer(id).keys.front().easing == before.layer(id).keys.front().easing);
    REQUIRE(session.document().drawings == before.drawings);
    REQUIRE_THROWS(session.apply("Collision", [&](Document& d) { movePoseKey(d.layer(id), 30, 0); }));
    session.undo();
    REQUIRE(session.document() == before);
    REQUIRE_THROWS(
        session.apply("Invalid ease", [&](Document& d) { setPoseEase(d.layer(id), 0, {.9, 0, .1, 1}); }));
    REQUIRE(session.document() == before);
}

TEST_CASE("Sparse key blocks move together and collisions leave the domain untouched") {
    auto d = makeBouncingBall();
    auto& layer = d.layers.front();
    layer.keys = {{0, {}, Interpolation::Linear},
                  {4, {10, 20}, Interpolation::Smooth},
                  {8, {30, 40}, Interpolation::Step},
                  {12, {50, 60}, Interpolation::Linear},
                  {24, {80, 90}, Interpolation::Linear}};
    setKeyEase(layer, 4, "x", {.2, 0, .8, 1.6});
    auto original = layer;
    auto frames = retimeKeyBlock(layer, {4, 12}, 6, 14);
    REQUIRE(frames == std::vector<Frame>{6, 14});
    REQUIRE(layer.keys[1].value == original.keys[1].value);
    REQUIRE(layer.keys[1].easing == original.keys[1].easing);
    REQUIRE(layer.keys[2] == original.keys[2]);
    auto before = layer;
    REQUIRE_THROWS(retimeKeyBlock(layer, {6, 14}, 8, 16));
    REQUIRE(layer == before);
    REQUIRE_THROWS(retimeKeyBlock(layer, {6, 14}, -1, 7));
    REQUIRE(layer == before);
    REQUIRE_THROWS(retimeKeyBlock(layer, {6, 7}, 10, 11));
    REQUIRE(layer == before);
    REQUIRE_THROWS(retimeKeyBlock(layer, {6, 6}, 10, 11));
    REQUIRE(layer == before);
}
TEST_CASE("Stretching a selected key block preserves evaluated motion and rejects rounded collisions") {
    Session session;
    const auto id = session.document().layers.front().id;
    session.apply("Fixture", [&](Document& d) {
        auto& layer = d.layer(id);
        layer.keys = {{4, {}, Interpolation::Linear},
                      {8, {40, 10}, Interpolation::Smooth},
                      {12, {80, 30}, Interpolation::Linear}};
        setKeyEase(layer, 4, "x", {.2, 0, .7, 1.6});
    });
    const auto before = session.document();
    session.apply("Stretch", [&](Document& d) { retimeKeyBlock(d.layer(id), {4, 8, 12}, 4, 20); });
    for (int f = 4; f <= 12; ++f)
        REQUIRE(evaluateTransform(session.document().layer(id), 4 + (f - 4) * 2) ==
                evaluateTransform(before.layer(id), f));
    REQUIRE_THROWS(
        session.apply("Collapse", [&](Document& d) { retimeKeyBlock(d.layer(id), {4, 12, 20}, 4, 5); }));
    session.undo();
    REQUIRE(session.document() == before);
    session.redo();
    REQUIRE(session.document().layer(id).keys.back().frame == 20);
}
TEST_CASE("Motion clipboard transfers local poses and pivots without drawings or hierarchy changes") {
    auto d = makeBouncingBall();
    auto& source = d.layers.front();
    Transform pose;
    pose.x = 30;
    pose.y = -20;
    pose.pivotX = 17;
    pose.rotation = 350;
    pose.scaleX = -2;
    source.keys = {{4, pose, Interpolation::Smooth}, {10, {70, 20}, Interpolation::Linear}};
    setKeyEase(source, 4, "rotation", {.3, -.2, .8, 1.4});
    const auto block = copyKeyBlock(source, {4, 10});
    const auto drawings = d.drawings;
    Layer target;
    target.id = d.allocateId();
    target.name = "Target";
    target.parent = source.id;
    target.transform.y = 150;
    auto rest = target.transform;
    auto frames = pasteKeyBlock(target, block, 20);
    REQUIRE(frames == std::vector<Frame>{20, 26});
    REQUIRE(target.keys.front().value == pose);
    REQUIRE(target.keys.front().easing == source.keys.front().easing);
    REQUIRE(target.parent == source.id);
    REQUIRE(target.transform == rest);
    REQUIRE(target.exposures.empty());
    d.layers.push_back(target);
    REQUIRE(d.drawings == drawings);
    auto reopened = deserializeDocument(serializeDocument(d));
    for (int f = 0; f < d.duration; ++f)
        REQUIRE(evaluateTransform(reopened.layers.back(), f) == evaluateTransform(target, f));
    auto before = target;
    REQUIRE_THROWS(pasteKeyBlock(target, block, 20));
    REQUIRE(target == before);
    REQUIRE_THROWS(pasteKeyBlock(target, block, 999999));
    REQUIRE(target == before);
}
TEST_CASE("Duplicate and delete selected pose keys preserve originals and reject locked edits") {
    auto layer = makeDocument().layers.front();
    layer.keys = {{0, {}, Interpolation::Linear},
                  {4, {10, 20}, Interpolation::Smooth},
                  {8, {20, 30}, Interpolation::Step}};
    auto original = layer;
    retimeKeyBlock(layer, {0, 4}, 12, 16, true);
    REQUIRE(layer.keys.size() == 5);
    for (int i = 0; i < 3; ++i)
        REQUIRE(layer.keys[i] == original.keys[i]);
    REQUIRE(layer.keys[3].value == original.keys[0].value);
    auto before = layer;
    REQUIRE_THROWS(retimeKeyBlock(layer, {0, 4}, 4, 8, true));
    REQUIRE(layer == before);
    REQUIRE_THROWS(deleteKeyBlock(layer, {12, 17}));
    REQUIRE(layer == before);
    deleteKeyBlock(layer, {12, 16});
    REQUIRE(layer == original);
    layer.locked = true;
    before = layer;
    REQUIRE_THROWS(deleteKeyBlock(layer, {0}));
    REQUIRE_THROWS(retimeKeyBlock(layer, {0}, 12, 12));
    REQUIRE_THROWS(pasteKeyBlock(layer, copyKeyBlock(layer, {0, 4}), 12));
    REQUIRE(layer == before);
}
