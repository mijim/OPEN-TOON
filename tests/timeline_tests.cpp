#include "opentoon/session.h"
#include "opentoon/timeline.h"
#include "serialization.h"
#include <catch2/catch_test_macros.hpp>
using namespace opentoon;
TEST_CASE("Exposure-only pasting preserves keys and independent drawing pasting breaks sharing") {
    auto d = makeBouncingBall();
    const auto layer = d.layers.front().id;
    d.layer(layer).keys = {{0, {}, Interpolation::Linear}, {12, {}, Interpolation::Step}};
    auto originalKeys = d.layer(layer).keys;
    auto clip = copyRange(d, {layer}, 0, 4);
    pasteRange(d, {layer}, 12, clip, PasteContent::Exposures);
    REQUIRE(d.layer(layer).keys == originalKeys);
    REQUIRE(d.drawingAt(layer, 12)->id == d.drawingAt(layer, 0)->id);
    pasteRange(d, {layer}, 20, clip, PasteContent::IndependentDrawings);
    REQUIRE(d.drawingAt(layer, 20)->id != d.drawingAt(layer, 0)->id);
    REQUIRE(d.drawingAt(layer, 20)->strokes.front().points == d.drawingAt(layer, 0)->strokes.front().points);
    REQUIRE(deserializeDocument(serializeDocument(d)) == d);
}
TEST_CASE("Timeline clipboard imports missing drawings and colors across scenes without ID collisions") {
    auto source = makeBouncingBall();
    source.palette.front().color = {1, .2, .3, 1};
    auto clip = copyRange(source, {source.layers.front().id}, 0, 4);
    auto destination = makeDocument();
    pasteRange(destination, {destination.layers.front().id}, 0, clip, PasteContent::All, false, false);
    destination.validate();
    const auto* drawing = destination.drawingAt(destination.layers.front().id, 0);
    REQUIRE(drawing != nullptr);
    auto color = std::find_if(destination.palette.begin(), destination.palette.end(),
                              [&](const auto& s) { return s.id == drawing->strokes.front().swatch; });
    REQUIRE(color != destination.palette.end());
    REQUIRE(color->color == source.palette.front().color);
}
TEST_CASE("Cycles extend scene boundaries and retiming refuses silent drawing loss atomically") {
    Session session;
    session.replace(makeBouncingBall());
    auto id = session.document().layers.front().id;
    const auto original = session.document();
    REQUIRE(session.apply("Cycle", [&](Document& d) { repeatRange(d, {id}, 44, 48, 3); }));
    REQUIRE(session.document().duration == 60);
    REQUIRE(session.document().drawingAt(id, 44)->id == session.document().drawingAt(id, 56)->id);
    REQUIRE(session.undo());
    REQUIRE(session.document() == original);
    REQUIRE_THROWS(session.apply("Impossible retime", [&](Document& d) { retimeRange(d, {id}, 0, 48, 1); }));
    REQUIRE(session.document() == original);
    REQUIRE(session.apply("Retime", [&](Document& d) { retimeRange(d, {id}, 0, 48, 96); }));
    REQUIRE(session.document().duration == 96);
    REQUIRE(session.document().drawingAt(id, 3)->id == original.drawingAt(id, 0)->id);
    REQUIRE(session.document().drawingAt(id, 4)->id == original.drawingAt(id, 2)->id);
}
TEST_CASE("Locked multi-layer range fails atomically and sparse 10000-frame edits stay interval-based") {
    Session session;
    auto d = makeBouncingBall();
    d.duration = 10000;
    Layer locked;
    locked.id = d.allocateId();
    locked.locked = true;
    d.layers.push_back(locked);
    session.replace(d);
    auto first = d.layers.front().id;
    REQUIRE_THROWS(
        session.apply("Clear", [&](Document& next) { clearRange(next, {first, locked.id}, 0, 10000); }));
    REQUIRE(session.document() == d);
    auto clip = copyRange(d, {first}, 0, 48);
    pasteRange(d, {first}, 9952, clip, PasteContent::Exposures);
    REQUIRE(d.layer(first).exposures.size() == 48);
    REQUIRE(d.duration == 10000);
    REQUIRE(deserializeDocument(serializeDocument(d)) == d);
}
TEST_CASE("Keys-only paste leaves exposures intact and markers survive retiming operations") {
    auto d = makeBouncingBall();
    auto id = d.layers.front().id;
    d.layer(id).keys.push_back({1, {}, Interpolation::Smooth});
    auto clip = copyRange(d, {id}, 0, 4);
    auto exposures = d.layer(id).exposures;
    pasteRange(d, {id}, 8, clip, PasteContent::Keys);
    REQUIRE(d.layer(id).exposures == exposures);
    REQUIRE(d.layer(id).keys.back().frame == 9);
    setMarker(d, 8, "Contact");
    insertFrames(d, 8, 2);
    REQUIRE(std::any_of(d.markers.begin(), d.markers.end(),
                        [](auto& m) { return m.frame == 10 && m.name == "Contact"; }));
    setMarker(d, 10, "");
    REQUIRE(std::none_of(d.markers.begin(), d.markers.end(), [](auto& m) { return m.frame == 10; }));
}
