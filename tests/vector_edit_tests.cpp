#include "opentoon/session.h"
#include "opentoon/vector_edit.h"
#include "serialization.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
using namespace opentoon;
namespace {
Document fixture() {
    auto doc = makeDocument();
    auto& d = doc.editableDrawing(doc.layers.front().id, 0);
    for (auto p : {Point{10, 20}, Point{70, 70}, Point{200, 50}, Point{90, 40}})
        d.strokes.push_back({doc.allocateId(),
                             doc.palette.front().id,
                             2,
                             Shape::Rectangle,
                             true,
                             2,
                             {p, {p.x + 20, p.y + 30, .7}}});
    return doc;
}
} // namespace
TEST_CASE("Vector alignment and distribution preserve sparse bystanders and editable identities") {
    auto doc = fixture();
    auto& d = doc.drawings.begin()->second;
    const auto before = d;
    const std::vector<Id> ids{d.strokes[0].id, d.strokes[1].id, d.strokes[2].id};
    arrangeVectors(d, ids, VectorLayout::Top);
    for (int i = 0; i < 3; ++i) {
        REQUIRE(d.strokes[i].points[0].y == 20);
        REQUIRE(d.strokes[i].id == before.strokes[i].id);
        REQUIRE(d.strokes[i].points[1].pressure == .7);
    }
    REQUIRE(d.strokes[3] == before.strokes[3]);
    arrangeVectors(d, ids, VectorLayout::DistributeX);
    REQUIRE(d.strokes[0].points[0].x == 10);
    REQUIRE(d.strokes[1].points[0].x == 105);
    REQUIRE(d.strokes[2].points[0].x == 200);
    const auto aligned = d;
    REQUIRE_THROWS(arrangeVectors(d, {ids.front()}, VectorLayout::Left));
    REQUIRE(d == aligned);
    REQUIRE_THROWS(arrangeVectors(d, {ids.front(), 9999}, VectorLayout::Top));
    REQUIRE(d == aligned);
    for (auto mode : {VectorLayout::Left, VectorLayout::Right, VectorLayout::CenterX, VectorLayout::CenterY,
                      VectorLayout::Bottom, VectorLayout::DistributeY}) {
        auto copy = before;
        arrangeVectors(copy, ids, mode);
        REQUIRE(copy.strokes[3] == before.strokes[3]);
    }
}
TEST_CASE("Vector ordering is stable within art layers and preserves selected relative order") {
    auto doc = fixture();
    auto& d = doc.drawings.begin()->second;
    d.strokes[1].artLayer = 1;
    const auto before = d;
    const auto a = d.strokes[0].id, c = d.strokes[2].id, last = d.strokes[3].id;
    orderVectors(d, {a, c}, VectorOrder::Front);
    REQUIRE(d.strokes[0].id == last);
    REQUIRE(d.strokes[1] == before.strokes[1]);
    REQUIRE(d.strokes[2].id == a);
    REQUIRE(d.strokes[3].id == c);
    orderVectors(d, {a, c}, VectorOrder::Back);
    REQUIRE(d == before);
    orderVectors(d, {a}, VectorOrder::Forward);
    REQUIRE(d.strokes[2].id == a);
    orderVectors(d, {a}, VectorOrder::Backward);
    REQUIRE(d == before);
}
TEST_CASE("Vector clipboard transfers local geometry and resolves colors across scenes atomically") {
    auto source = fixture();
    source.palette.front().name = "Accent";
    source.palette.front().color = {1, .2, .4, .8};
    const auto sourceDrawing = source.drawings.begin()->second;
    const auto block = copyVectors(source, source.layers.front().id, 0,
                                   {sourceDrawing.strokes[0].id, sourceDrawing.strokes[2].id});
    auto destination = makeDocument();
    const auto layer = destination.layers.front().id;
    auto ids = pasteVectors(destination, layer, 4, block, 12, -5);
    REQUIRE(ids.size() == 2);
    REQUIRE(destination.palette.back().color == source.palette.front().color);
    auto drawing = *destination.drawingAt(layer, 4);
    REQUIRE(drawing.strokes[0].points[0] == Point{22, 15, 1});
    REQUIRE(drawing.strokes[1].points[1].pressure == .7);
    REQUIRE(destination.drawingAt(layer, 0) == nullptr);
    const auto colorCount = destination.palette.size();
    const auto second = pasteVectors(destination, layer, 5, block);
    REQUIRE(destination.palette.size() == colorCount);
    REQUIRE(second[0] != ids[0]);
    REQUIRE(deserializeDocument(serializeDocument(destination)) == destination);
    const auto before = destination;
    auto invalid = block;
    invalid.palette.clear();
    REQUIRE_THROWS(pasteVectors(destination, layer, 10, invalid));
    REQUIRE(destination == before);
    destination.layer(layer).locked = true;
    const auto locked = destination;
    REQUIRE_THROWS(pasteVectors(destination, layer, 0, block));
    REQUIRE(destination == locked);
}
TEST_CASE("Batch vector styles preserve nonselected geometry and validate before mutation") {
    auto doc = fixture();
    auto& d = doc.drawings.begin()->second;
    const auto before = d;
    const std::vector<Id> ids{d.strokes[0].id, d.strokes[2].id};
    styleVectors(d, ids, "strokeWidth", 7);
    styleVectors(d, ids, "artLayer", 3);
    styleVectors(d, ids, "filled", 0);
    REQUIRE(d.strokes[0].width == 7);
    REQUIRE(d.strokes[2].artLayer == 3);
    REQUIRE_FALSE(d.strokes[2].filled);
    REQUIRE(d.strokes[0].points == before.strokes[0].points);
    REQUIRE(d.strokes[1] == before.strokes[1]);
    const auto edited = d;
    REQUIRE_THROWS(styleVectors(d, ids, "strokeWidth", -1));
    REQUIRE_THROWS(styleVectors(d, ids, "artLayer", 1.5));
    REQUIRE(d == edited);
}
TEST_CASE("Smoothing preserves endpoints pressure corners and non-pencil primitives") {
    Drawing d;
    d.strokes = {{1, 5, 4, Shape::Stroke, false, 2, {{0, 0, .1}, {10, 1, .5}, {20, 0, .8}, {20, 20, 1}}},
                 {2, 5, 3, Shape::Rectangle, true, 0, {{0, 0, 1}, {40, 40, 1}}}};
    const auto before = d;
    smoothVectors(d, {1, 2}, .5);
    REQUIRE(d.strokes[0].points[1].y == .5);
    REQUIRE(d.strokes[0].points[1].pressure == .5);
    REQUIRE(d.strokes[0].points[0] == before.strokes[0].points[0]);
    REQUIRE(d.strokes[0].points[2] == before.strokes[0].points[2]);
    REQUIRE(d.strokes[0].points.back() == before.strokes[0].points.back());
    REQUIRE(d.strokes[1] == before.strokes[1]);
}
TEST_CASE("Simplification bounds centerline error preserves pressure peaks and undoes exactly") {
    auto doc = makeDocument();
    auto& d = doc.editableDrawing(doc.layers.front().id, 0);
    Stroke s{doc.allocateId(), doc.palette.front().id, 4, Shape::Stroke};
    for (int x = 0; x <= 100; ++x)
        s.points.push_back({double(x), .1 * std::sin(x), x == 50 ? 1. : .3});
    d.strokes.push_back(s);
    Session session;
    session.replace(doc);
    session.apply("Simplify",
                  [&](Document& edit) { simplifyVectors(edit.drawings.begin()->second, {s.id}, .25); });
    const auto& reduced = session.document().drawings.begin()->second.strokes.front();
    REQUIRE(reduced.points.size() < 20);
    REQUIRE(reduced.points.front() == s.points.front());
    REQUIRE(reduced.points.back() == s.points.back());
    REQUIRE(std::any_of(reduced.points.begin(), reduced.points.end(),
                        [](auto p) { return p.x == 50 && p.pressure == 1; }));
    for (const auto p : s.points) {
        double distance = 1e10;
        for (std::size_t i = 1; i < reduced.points.size(); ++i) {
            auto a = reduced.points[i - 1], b = reduced.points[i];
            const auto dx = b.x - a.x, dy = b.y - a.y;
            const auto t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / (dx * dx + dy * dy), 0., 1.);
            distance = std::min(distance, std::hypot(p.x - a.x - t * dx, p.y - a.y - t * dy));
        }
        REQUIRE(distance <= .25);
    }
    session.undo();
    REQUIRE(session.document() == doc);
}
