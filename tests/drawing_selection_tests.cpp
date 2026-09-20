#include "opentoon/drawing_selection.h"
#include "opentoon/session.h"
#include "serialization.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
using namespace opentoon;
namespace {
using Pixel = std::array<std::uint16_t, 4>;
void put(RasterImage& image, int x, int y, Pixel p) {
    auto key = std::pair{x / 64, y / 64};
    std::vector<std::uint16_t> tile(64 * 64 * 4, 0);
    if (image.tiles.contains(key))
        tile = image.tiles.at(key).values();
    std::copy(p.begin(), p.end(), tile.begin() + ((y % 64) * 64 + x % 64) * 4);
    image.tiles.insert_or_assign(key, SharedBuffer<std::uint16_t>(std::move(tile)));
}
Pixel get(const RasterImage& image, int x, int y) {
    auto it = image.tiles.find({x / 64, y / 64});
    if (it == image.tiles.end())
        return {};
    int n = ((y % 64) * 64 + x % 64) * 4;
    return {it->second[n], it->second[n + 1], it->second[n + 2], it->second[n + 3]};
}
Drawing rasterDrawing() {
    Drawing drawing;
    drawing.raster = RasterImage{256, 256, {}};
    return drawing;
}
} // namespace
TEST_CASE("Overlapping raster moves read original pixels and retain untouched tile buffers") {
    auto d = rasterDrawing();
    put(*d.raster, 63, 10, {16384, 0, 0, 16384});
    put(*d.raster, 64, 10, {0, 32768, 0, 32768});
    put(*d.raster, 190, 190, {0, 0, 32768, 32768});
    auto before = *d.raster;
    auto untouched = d.raster->tiles.at({2, 2}).data();
    Id next = 1;
    editDrawingSelection(d, {63, 10, 2, 1}, SelectionMedia::Raster, SelectionAction::Move, 1, 0, next);
    REQUIRE(get(*d.raster, 63, 10) == Pixel{});
    REQUIRE(get(*d.raster, 64, 10) == get(before, 63, 10));
    REQUIRE(get(*d.raster, 65, 10) == get(before, 64, 10));
    REQUIRE(d.raster->tiles.at({2, 2}).data() == untouched);
    REQUIRE(get(before, 64, 10) == Pixel{0, 32768, 0, 32768});
    REQUIRE_FALSE(d.raster->tiles.contains({0, 0}));
}
TEST_CASE("Selection compositing preserves premultiplication and transparent source leaves destination") {
    auto d = rasterDrawing();
    Id next = 1;
    put(*d.raster, 10, 10, {16384, 0, 0, 16384});
    put(*d.raster, 20, 10, {0, 0, 32768, 32768});
    put(*d.raster, 21, 10, {0, 32768, 0, 32768});
    editDrawingSelection(d, {10, 10, 2, 1}, SelectionMedia::Raster, SelectionAction::Duplicate, 10, 0, next);
    REQUIRE(get(*d.raster, 20, 10) == Pixel{16384, 0, 16384, 32768});
    REQUIRE(get(*d.raster, 21, 10) == Pixel{0, 32768, 0, 32768});
    REQUIRE(get(*d.raster, 10, 10) == Pixel{16384, 0, 0, 16384});
}
TEST_CASE("Raster flips and quarter turns preserve sparse alpha without resampling") {
    auto d = rasterDrawing();
    Id next = 1;
    put(*d.raster, 10, 20, {1000, 2000, 3000, 4000});
    put(*d.raster, 12, 21, {0, 10000, 0, 12000});
    const auto original = d;
    editDrawingSelection(d, {10, 20, 3, 2}, SelectionMedia::Raster, SelectionAction::FlipHorizontal, 0, 0,
                         next);
    REQUIRE(get(*d.raster, 12, 20) == Pixel{1000, 2000, 3000, 4000});
    editDrawingSelection(d, {10, 20, 3, 2}, SelectionMedia::Raster, SelectionAction::FlipHorizontal, 0, 0,
                         next);
    REQUIRE(d == original);
    PixelRect rect{10, 20, 3, 2};
    for (int i = 0; i < 4; ++i) {
        editDrawingSelection(d, rect, SelectionMedia::Raster, SelectionAction::RotateClockwise, 0, 0, next);
        std::swap(rect.width, rect.height);
        if (i == 0) {
            REQUIRE(get(*d.raster, 11, 20) == Pixel{1000, 2000, 3000, 4000});
            REQUIRE(get(*d.raster, 10, 20) == Pixel{});
        }
    }
    REQUIRE(d == original);
}
TEST_CASE("Mixed selection operations are atomic and retain vector IDs palette art and image data") {
    auto doc = makeDocument();
    auto id = doc.layers.front().id;
    auto& drawing = doc.editableDrawing(id, 0);
    drawing.strokes.push_back(
        {doc.allocateId(), doc.palette.front().id, 2, Shape::Rectangle, true, 1, {{10, 10, 1}, {20, 20, 1}}});
    drawing.strokes.push_back(
        {doc.allocateId(), doc.palette.front().id, 2, Shape::Stroke, false, 2, {{5, 5, 1}, {50, 5, 1}}});
    drawing.raster = RasterImage{256, 256, {}};
    put(*drawing.raster, 10, 10, {16384, 0, 0, 16384});
    drawing.image = ImageAsset{1, 1, {255, 0, 0, 255}};
    const auto drawingId = drawing.id;
    const auto before = doc;
    const auto rect = PixelRect{0, 0, 32, 32};
    REQUIRE(enclosedStrokes(drawing, rect).size() == 1);
    Session session;
    session.replace(doc);
    session.apply("Duplicate selection", [&](Document& d) {
        editDrawingSelection(d.drawings.at(drawingId), rect, SelectionMedia::Both, SelectionAction::Duplicate,
                             40, 0, d.nextId);
    });
    const auto& duplicate = session.document().drawings.at(drawingId);
    REQUIRE(duplicate.strokes.size() == 3);
    REQUIRE(duplicate.strokes[2].id != duplicate.strokes[0].id);
    REQUIRE(duplicate.strokes[2].swatch == duplicate.strokes[0].swatch);
    REQUIRE(duplicate.strokes[2].artLayer == 1);
    REQUIRE(duplicate.strokes[1] == drawing.strokes[1]);
    REQUIRE(duplicate.image == drawing.image);
    REQUIRE(deserializeDocument(serializeDocument(session.document())) == session.document());
    session.undo();
    REQUIRE(session.document() == before);
    REQUIRE_THROWS(session.apply("Out of bounds", [&](Document& d) {
        editDrawingSelection(d.drawings.at(drawingId), rect, SelectionMedia::Both, SelectionAction::Move, -20,
                             0, d.nextId);
    }));
    REQUIRE(session.document() == before);
    session.apply("Delete selection", [&](Document& d) {
        editDrawingSelection(d.drawings.at(drawingId), rect, SelectionMedia::Both, SelectionAction::Delete, 0,
                             0, d.nextId);
    });
    REQUIRE(session.document().drawings.at(drawingId).strokes.size() == 1);
    REQUIRE(session.document().drawings.at(drawingId).raster->tiles.empty());
    session.undo();
    REQUIRE(session.document() == before);
}

TEST_CASE("Free raster transforms preserve transparent surroundings and roll back rejected bounds") {
    auto d = rasterDrawing();
    put(*d.raster, 10, 20, {1000, 2000, 3000, 4000});
    const auto original = d;
    transformDrawingSelection(d, {10, 20, 2, 2}, SelectionMedia::Raster, {}, {2, 0, 0, 2, -10, -20});
    REQUIRE(get(*d.raster, 10, 20) == Pixel{1000, 2000, 3000, 4000});
    REQUIRE(get(*d.raster, 11, 21) == Pixel{1000, 2000, 3000, 4000});
    REQUIRE(get(*d.raster, 12, 20) == Pixel{});
    d = original;
    transformDrawingSelection(d, {10, 20, 2, 2}, SelectionMedia::Raster, {}, {0, 1, -1, 0, 32, 10});
    REQUIRE(get(*d.raster, 11, 20) == Pixel{1000, 2000, 3000, 4000});
    REQUIRE(get(*d.raster, 10, 20) == Pixel{});
    auto before = d;
    REQUIRE_THROWS(
        transformDrawingSelection(d, {0, 0, 32, 32}, SelectionMedia::Raster, {}, {1, 0, 0, 1, -100, 0}));
    REQUIRE(d == before);
    REQUIRE_THROWS(
        transformDrawingSelection(d, {0, 0, 32, 32}, SelectionMedia::Raster, {}, {0, 0, 0, 1, 0, 0}));
    REQUIRE(d == before);
}
TEST_CASE("Affine vector edits freeze object membership and keep rotated primitives editable") {
    Drawing d;
    d.strokes = {{1, 5, 2, Shape::Rectangle, true, 1, {{10, 20, 1}, {20, 30, 1}}},
                 {2, 5, 2, Shape::Stroke, false, 2, {{10, 20, 1}, {40, 40, 1}}}};
    auto unrelated = d.strokes.back();
    transformDrawingSelection(d, {0, 0, 50, 50}, SelectionMedia::Vectors, {1}, {0, 1, -1, 0, 50, 0});
    REQUIRE(d.strokes.front().shape == Shape::Polygon);
    REQUIRE(d.strokes.front().points.size() == 4);
    REQUIRE(d.strokes.front().points.front() == Point{30, 10, 1});
    REQUIRE(d.strokes.front().id == 1);
    REQUIRE(d.strokes.front().swatch == 5);
    REQUIRE(d.strokes.back() == unrelated);
}

TEST_CASE("Sampled point insertion preserves pressure and point deletion preserves shape identity") {
    Stroke s{10, 7, 4, Shape::Stroke, false, 2, {{0, 0, .2}, {100, 40, .8}}};
    const auto original = s;
    insertStrokePoint(s, 0, .25);
    REQUIRE(s.points.size() == 3);
    REQUIRE(s.points[1].x == 25);
    REQUIRE(s.points[1].y == 10);
    REQUIRE(s.points[1].pressure == Catch::Approx(.35));
    REQUIRE(s.id == 10);
    REQUIRE(s.swatch == 7);
    REQUIRE(s.artLayer == 2);
    removeStrokePoint(s, 1);
    REQUIRE(s == original);
    removeStrokePoint(s, 0);
    REQUIRE_THROWS(removeStrokePoint(s, 0));
    s = original;
    s.shape = Shape::Rectangle;
    REQUIRE_THROWS(insertStrokePoint(s, 0, .5));
    REQUIRE_THROWS(removeStrokePoint(s, 0));
    s = original;
    s.shape = Shape::Polygon;
    s.points.push_back({50, 80, .5});
    insertStrokePoint(s, 2, .5);
    REQUIRE(s.points.back().x == 25);
    REQUIRE(s.points.back().y == 40);
    removeStrokePoint(s, 3);
    REQUIRE_THROWS(removeStrokePoint(s, 0));
}

TEST_CASE("Sparse vector selection bounds and duplicate identities exclude enclosed bystanders") {
    auto d = rasterDrawing();
    d.strokes = {{1, 5, 2, Shape::Rectangle, true, 1, {{10, 20, 1}, {20, 30, 1}}},
                 {2, 5, 2, Shape::Stroke, false, 2, {{25, 25, .4}, {30, 30, .8}}},
                 {3, 5, 2, Shape::Ellipse, true, 3, {{40, 20, 1}, {50, 30, 1}}}};
    put(*d.raster, 25, 25, {1000, 2000, 3000, 4000});
    const auto before = d;
    std::vector<Id> ids{3, 1, 3};
    REQUIRE(strokeSelectionBounds(d, ids) == PixelRect{9, 19, 42, 12});
    REQUIRE_FALSE(strokeSelectionBounds(d, {999}));
    REQUIRE(strokeSelectionBounds(d, {2}) == PixelRect{24, 24, 7, 7});
    const auto bounds = *strokeSelectionBounds(d, ids);
    Id next = 4;
    editDrawingSelection(d, bounds, SelectionMedia::Vectors, SelectionAction::Duplicate, 0, 0, next, &ids,
                         &ids);
    REQUIRE(ids == std::vector<Id>{4, 5});
    REQUIRE(d.strokes.size() == 5);
    REQUIRE(d.strokes[0] == before.strokes[0]);
    REQUIRE(d.strokes[1] == before.strokes[1]);
    REQUIRE(d.strokes[2] == before.strokes[2]);
    REQUIRE(d.strokes[3].artLayer == 1);
    REQUIRE(d.strokes[4].artLayer == 3);
    transformDrawingSelection(d, bounds, SelectionMedia::Vectors, ids, {1, 0, 0, 1, 20, -10});
    REQUIRE(d.strokes[0] == before.strokes[0]);
    REQUIRE(d.strokes[1] == before.strokes[1]);
    REQUIRE(d.strokes[2] == before.strokes[2]);
    REQUIRE(d.raster == before.raster);
    editDrawingSelection(d, bounds, SelectionMedia::Vectors, SelectionAction::Delete, 0, 0, next, &ids, &ids);
    REQUIRE(ids.empty());
    REQUIRE(d == before);
    ids = {1, 3};
    const auto originalIds = ids;
    REQUIRE_THROWS(editDrawingSelection(d, bounds, SelectionMedia::Both, SelectionAction::Duplicate, -100, 0,
                                        next, &ids, &ids));
    REQUIRE(d == before);
    REQUIRE(ids == originalIds);
    REQUIRE(next == 6);
}
