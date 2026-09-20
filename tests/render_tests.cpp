#include "opentoon/animation.h"
#include "scene_renderer.h"
#include "serialization.h"
#include "vector_hit.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
using namespace opentoon;
TEST_CASE("Saved and reopened scenes produce the same forty-eight rendered frames") {
    auto original = makeBouncingBall();
    auto reopened = deserializeDocument(serializeDocument(original));
    for (Frame frame = 0; frame < 48; ++frame)
        REQUIRE(SceneRenderer::render(original, frame, QSize(240, 135)) ==
                SceneRenderer::render(reopened, frame, QSize(240, 135)));
    REQUIRE(SceneRenderer::render(original, 0) != SceneRenderer::render(original, 12));
}
TEST_CASE("Palette identity recolors only referenced strokes and opacity preserves alpha") {
    auto d = makeBouncingBall();
    d.background.a = 0;
    auto before = SceneRenderer::render(d, 0);
    d.palette.front().color = {1, 0, 0, 1};
    REQUIRE(SceneRenderer::render(d, 0) != before);
    d.layers.front().transform.opacity = 0;
    auto transparent = SceneRenderer::render(d, 0);
    for (int y = 0; y < transparent.height(); ++y)
        for (int x = 0; x < transparent.width(); ++x)
            REQUIRE(qAlpha(transparent.pixel(x, y)) == 0);
}
TEST_CASE("Child transforms compose with parent transforms") {
    auto d = makeDocument();
    auto child = d.layers.front().id;
    Layer parent;
    parent.id = d.allocateId();
    parent.name = "Peg";
    parent.transform.x = 100;
    parent.transform.scaleX = 2;
    d.layers.push_back(parent);
    d.layer(child).parent = parent.id;
    d.layer(child).transform.x = 10;
    auto point = SceneRenderer::worldTransform(d, d.layer(child), 0).map(QPointF(0, 0));
    REQUIRE(point.x() == 120);
}
TEST_CASE("Soloing a parent renders its visible descendants and hides unrelated layers") {
    auto d = makeBouncingBall();
    Layer parent;
    parent.id = d.allocateId();
    parent.name = "Parent";
    d.layers.front().parent = parent.id;
    d.layers.push_back(parent);
    auto normal = SceneRenderer::render(d, 0, QSize(240, 135));
    d.layers.back().solo = true;
    REQUIRE(SceneRenderer::render(d, 0, QSize(240, 135)) == normal);
    d.layers.back().visible = false;
    REQUIRE(SceneRenderer::render(d, 0, QSize(240, 135)) != normal);
}

TEST_CASE("Opaque raster tiles have no internal seams at fractional viewport scales") {
    auto document = makeDocument();
    document.width = 128;
    document.height = 64;
    auto& drawing = document.editableDrawing(document.layers.front().id, 0);
    drawing.raster = RasterImage{128, 64, {}};
    std::vector<std::uint16_t> pixels(64 * 64 * 4);
    for (std::size_t i = 0; i < pixels.size(); i += 4)
        pixels[i + 3] = 32768;
    drawing.raster->tiles[{0, 0}] = pixels;
    drawing.raster->tiles[{1, 0}] = pixels;
    for (auto size : {QSize{81, 41}, QSize{199, 99}, QSize{47, 23}}) {
        auto image = SceneRenderer::render(document, 0, size);
        for (int y = 1; y < size.height() - 1; ++y)
            for (int x = 1; x < size.width() - 1; ++x)
                REQUIRE(image.pixelColor(x, y) == QColor(Qt::black));
    }
}

TEST_CASE("Thin vector picking has a screen-space margin at any zoom and respects art order") {
    Drawing drawing;
    drawing.strokes.push_back({1, 1, .5, Shape::Stroke, false, 0, {{0, 0, 1}, {100, 0, 1}}});
    for (double zoom : {.1, .4, 1.0, 5.0, 10.0}) {
        QTransform view;
        view.translate(100, 100);
        view.rotate(30);
        view.scale(zoom, zoom * 2);
        auto center = view.map(QPointF(50, 0));
        auto normal = QPointF(-.5, std::sqrt(3.0) / 2);
        REQUIRE(hitVectorOnScreen(drawing, view, center + normal * 7) == 1);
        REQUIRE_FALSE(hitVectorOnScreen(drawing, view, center + normal * (12 + zoom)));
    }
    drawing.strokes.push_back({2, 1, .5, Shape::Stroke, false, 2, {{0, 0, 1}, {100, 0, 1}}});
    drawing.strokes.push_back({3, 1, .5, Shape::Stroke, false, 1, {{0, 0, 1}, {100, 0, 1}}});
    REQUIRE(hitVectorOnScreen(drawing, {}, QPointF(50, 6)) == 2);
    drawing.strokes = {{4, 1, 1, Shape::Ellipse, false, 0, {{0, 0, 1}, {100, 100, 1}}}};
    REQUIRE_FALSE(hitVectorOnScreen(drawing, {}, QPointF(50, 50)));
    REQUIRE(hitVectorOnScreen(drawing, {}, QPointF(50, -7)) == 4);
    drawing.strokes.front().filled = true;
    REQUIRE(hitVectorOnScreen(drawing, {}, QPointF(50, 50)) == 4);
}
TEST_CASE("Bezier motion uses identical preview and reopened rendering") {
    auto d = makeBouncingBall();
    auto& layer = d.layers.back();
    layer.keys.clear();
    Transform target;
    target.x = 200;
    recordPose(layer, 24, target);
    setKeyEase(layer, 0, "x", {.25, 0, .65, 1.8});
    const auto reopened = deserializeDocument(serializeDocument(d));
    for (int f = 0; f <= 24; ++f)
        REQUIRE(SceneRenderer::render(d, f, QSize(240, 135)) ==
                SceneRenderer::render(reopened, f, QSize(240, 135)));
}

TEST_CASE("Whole-vector lasso respects concavity and stroke width instead of just sample points") {
    Drawing d;
    d.strokes = {{1, 1, 2, Shape::Stroke, false, 2, {{10, 10, 1}, {20, 10, 1}}},
                 {2, 1, 2, Shape::Stroke, false, 2, {{10, 20, 1}, {80, 20, 1}}},
                 {3, 1, 8, Shape::Ellipse, false, 2, {{5, 5, 1}, {25, 25, 1}}},
                 {4, 1, 2, Shape::Rectangle, true, 2, {{70, 70, 1}, {80, 80, 1}}}};
    std::vector<Point> concave{{0, 0},   {100, 0}, {100, 100}, {60, 100},
                               {60, 15}, {40, 15}, {40, 100},  {0, 100}};
    REQUIRE(enclosedVectorsByLasso(d, concave) == std::vector<Id>{1, 3, 4});
    std::vector<Point> small{{4, 4}, {26, 4}, {26, 26}, {4, 26}};
    REQUIRE(enclosedVectorsByLasso(d, small) == std::vector<Id>{1});
    REQUIRE(enclosedVectorsByLasso(d, {{0, 0}, {1, 1}}).empty());
}
