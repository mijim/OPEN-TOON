#include "scene_renderer.h"
#include "serialization.h"
#include <catch2/catch_test_macros.hpp>
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
