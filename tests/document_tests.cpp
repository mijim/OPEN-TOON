#include "opentoon/document.h"
#include "opentoon/session.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
using namespace opentoon;
TEST_CASE("Rational time preserves NTSC audio alignment without accumulating rounded frame durations") {
    FrameRate rate{24000, 1001};
    REQUIRE(rate.sampleAt(24000, 48000) == 48048000);
    REQUIRE(rate.sampleAt(1, 48000) == 2002);
    REQUIRE(rate.seconds(24000) == Catch::Approx(1001));
    REQUIRE_THROWS((FrameRate{0, 1}.seconds(1)));
}
TEST_CASE("Exposure overwrite splits holds without cloning drawings") {
    auto d = makeDocument();
    auto layer = d.layers.front().id;
    auto id = d.editableDrawing(layer, 0).id;
    expose(d.layer(layer), 0, 10, id);
    expose(d.layer(layer), 3, 5, 0);
    REQUIRE(d.drawings.size() == 1);
    REQUIRE(d.drawingAt(layer, 2)->id == id);
    REQUIRE(d.drawingAt(layer, 3) == nullptr);
    REQUIRE(d.drawingAt(layer, 5)->id == id);
    d.validate();
}
TEST_CASE("Commands are atomic and undo and redo return the saved state") {
    Session session;
    const auto before = session.document();
    REQUIRE_THROWS(session.apply("Invalid dimensions", [](auto& d) { d.width = -1; }));
    REQUIRE(session.document() == before);
    REQUIRE_FALSE(session.modified());
    REQUIRE(session.apply("Rename", [](auto& d) { d.name = "A new name"; }));
    REQUIRE(session.modified());
    REQUIRE(session.undo());
    REQUIRE(session.document() == before);
    REQUIRE_FALSE(session.modified());
    REQUIRE(session.redo());
    REQUIRE(session.document().name == "A new name");
}
TEST_CASE("Shared exposure edits affect only the referenced drawing") {
    auto d = makeDocument();
    Id l = d.layers.front().id;
    Id first = d.editableDrawing(l, 0).id;
    expose(d.layer(l), 0, 5, first);
    Id second = d.editableDrawing(l, 5).id;
    d.editableDrawing(l, 2).name = "Shared";
    REQUIRE(d.drawings.at(first).name == "Shared");
    REQUIRE(d.drawings.at(second).name != "Shared");
}
TEST_CASE("Frame insertion and removal retime exposures keys and markers coherently") {
    auto d = makeBouncingBall();
    auto original = d;
    insertFrames(d, 10, 3);
    REQUIRE(d.duration == 51);
    REQUIRE(d.markers[1].frame == 27);
    removeFrames(d, 10, 3);
    REQUIRE(d == original);
}
TEST_CASE("Hierarchy cycles and missing swatches are rejected") {
    auto d = makeBouncingBall();
    d.layers.front().parent = d.layers.front().id;
    REQUIRE_THROWS(d.validate());
    d.layers.front().parent = 0;
    d.palette.clear();
    REQUIRE_THROWS(d.validate());
}
TEST_CASE("Linear smooth and held keys preserve endpoints and finite negative scales") {
    Layer l;
    Transform a, b;
    b.x = 100;
    b.scaleX = -1;
    l.keys = {{0, a, Interpolation::Linear}, {10, b, Interpolation::Linear}};
    REQUIRE(evaluateTransform(l, 5).x == 50);
    REQUIRE(evaluateTransform(l, 10) == b);
    l.keys.front().interpolation = Interpolation::Step;
    REQUIRE(evaluateTransform(l, 9) == a);
}
TEST_CASE("Vector eraser splits a sparse segment and undo restores original geometry") {
    Session s;
    Id l = s.document().layers.front().id;
    s.apply("Stroke", [=](auto& d) {
        auto& drawing = d.editableDrawing(l, 0);
        drawing.strokes.push_back(
            {d.allocateId(), d.palette.front().id, 2, Shape::Stroke, false, 2, {{0, 0, 1}, {100, 0, 1}}});
    });
    auto before = s.document();
    s.apply("Erase", [=](auto& d) { eraseAt(d.editableDrawing(l, 0), {50, 0, 1}, 10, d.nextId); });
    const auto* drawing = s.document().drawingAt(l, 0);
    REQUIRE(drawing->strokes.size() == 2);
    REQUIRE(drawing->strokes.front().points.back().x < 40);
    REQUIRE(drawing->strokes.back().points.front().x > 60);
    s.undo();
    REQUIRE(s.document() == before);
}
