#include "opentoon/session.h"
#include "raster_brush.h"
#include "serialization.h"
#include <catch2/catch_test_macros.hpp>
#include <numeric>
using namespace opentoon;
namespace {
RasterImage stroke(double pressure, BrushPreset preset = BrushPreset::Ink,
                   const RasterImage& source = {256, 256, {}}) {
    RasterBrush brush(source, {32, 1, {0.2, 0.4, 0.8, 1}, preset});
    for (int x = 32; x < 224; x += 2)
        brush.sample({double(x), 128, pressure});
    return brush.snapshot();
}
std::uint64_t coverage(const RasterImage& image) {
    std::uint64_t value = 0;
    for (const auto& [key, tile] : image.tiles)
        for (std::size_t n = 3; n < tile.size(); n += 4)
            value += tile[n];
    return value;
}
} // namespace
TEST_CASE("MyPaint pressure changes coverage and repeated input produces identical immutable artwork") {
    const auto light = stroke(0.2), heavy = stroke(1);
    REQUIRE(coverage(light) > 0);
    REQUIRE(coverage(heavy) > coverage(light) * 2);
    REQUIRE(stroke(1) == heavy);
    const auto erased = stroke(1, BrushPreset::Eraser, heavy);
    REQUIRE(coverage(erased) < coverage(heavy));
    REQUIRE(heavy == stroke(1));
}
TEST_CASE("Raster gestures survive undo redo and serialization without modifying prior snapshots") {
    auto d = makeDocument();
    auto layer = d.layers.front().id;
    d.editableDrawing(layer, 0).raster = stroke(1);
    Session session;
    session.replace(d);
    const auto before = session.snapshot();
    session.apply("Erase raster", [&](Document& doc) {
        auto& raster = *doc.editableDrawing(layer, 0).raster;
        raster = stroke(1, BrushPreset::Eraser, raster);
    });
    REQUIRE(*before == d);
    REQUIRE(session.document() != d);
    REQUIRE(deserializeDocument(serializeDocument(session.document())) == session.document());
    session.undo();
    REQUIRE(session.document() == d);
    session.redo();
    REQUIRE(coverage(*session.document().drawingAt(layer, 0)->raster) <
            coverage(*d.drawingAt(layer, 0)->raster));
}

TEST_CASE("A raster tap paints and cancelled preview leaves no shared tile changes") {
    RasterImage blank{128, 128, {}};
    RasterBrush brush(blank, {32, 1, {1, 0, 0, 1}, BrushPreset::Ink});
    brush.sample({64, 64, 1});
    const auto first = brush.snapshot();
    REQUIRE(coverage(first) > 0);
    brush.sample({80, 64, 1});
    REQUIRE(coverage(brush.snapshot()) > coverage(first));
    REQUIRE(blank.tiles.empty());
    REQUIRE_THROWS(brush.sample({0, 0, -1}));
    REQUIRE_THROWS(brush.sample({0, 0, 1}, 0));
}

TEST_CASE("Brush opacity reduces coverage and eraser strength ignores selected color alpha") {
    auto paint = [](double opacity) {
        RasterBrush brush({256, 256, {}}, {32, opacity, {0.2, 0.4, 0.8, 1}, BrushPreset::Ink});
        for (int x = 32; x < 224; x += 2)
            brush.sample({double(x), 128, 1});
        return brush.snapshot();
    };
    auto full = paint(1), faint = paint(.15), empty = paint(0);
    REQUIRE(coverage(faint) > 0);
    REQUIRE(coverage(faint) < coverage(full));
    REQUIRE(coverage(empty) == 0);
    RasterBrush eraser(full, {32, .5, {0, 0, 0, 0}, BrushPreset::Eraser});
    for (int x = 32; x < 224; x += 2)
        eraser.sample({double(x), 128, 1});
    REQUIRE(coverage(eraser.snapshot()) < coverage(full));
}
