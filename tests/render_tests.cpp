#include "opentoon/animation.h"
#include "opentoon/session.h"
#include "scene_renderer.h"
#include "graph_renderer.h"
#include "revision_render_cache.h"
#include "preview_render_queue.h"
#include "serialization.h"
#include "vector_hit.h"
#include <QPainter>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <future>
#include <thread>
using namespace opentoon;
TEST_CASE("Saved and reopened scenes produce the same forty-eight rendered frames") {
    auto original = makeBouncingBall();
    auto reopened = deserializeDocument(serializeDocument(original));
    for (Frame frame = 0; frame < 48; ++frame)
        REQUIRE(SceneRenderer::render(original, frame, QSize(240, 135)) ==
                SceneRenderer::render(reopened, frame, QSize(240, 135)));
    REQUIRE(SceneRenderer::render(original, 0) != SceneRenderer::render(original, 12));
}
TEST_CASE("Linear composition preserves transparent alpha and agrees across outputs") {
    auto document = makeDocument();
    document.width = 1;
    document.height = 1;
    document.background = {0, 0, 0, 0};
    auto& base = document.editableDrawing(document.layers.front().id, 0);
    base.image = ImageAsset{1, 1, {255, 0, 0, 128}};
    Layer upper = document.layers.front();
    upper.id = document.allocateId();
    upper.name = "Upper";
    auto drawing = base;
    drawing.id = document.allocateId();
    drawing.image = ImageAsset{1, 1, {0, 0, 255, 128}};
    document.drawings.emplace(drawing.id, drawing);
    for (auto& exposure : upper.exposures)
        exposure.drawing = drawing.id;
    document.layers.push_back(upper);
    document.validate();
    const auto graph = CompositionGraph::orderedLayers(document);
    const auto legacy = SceneRenderer::render(document, 0);
    document.composition = CompositionProfile::LinearSrgb;
    const auto display = GraphRenderer::render(graph, document, 0, {}, {}, GraphTarget::Display);
    const auto write = GraphRenderer::render(graph, document, 0, {}, {}, GraphTarget::Write);
    REQUIRE(display == write);
    REQUIRE(SceneRenderer::render(document, 0) == write);
    QImage canvas({1, 1}, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::transparent);
    {
        QPainter painter(&canvas);
        SceneRenderer::paint(painter, document, 0);
    }
    REQUIRE(canvas == display);
    GraphNode transform{99, GraphNodeKind::LayerTransform, document.layers.front().id, {}};
    REQUIRE(GraphRenderer::evaluatedTransform(transform, document, 0) ==
            evaluateTransform(document.layers.front(), 0));
    auto branched = graph;
    branched.nodes[5].inputs.front().source = 1;
    REQUIRE(qAlpha(GraphRenderer::render(branched, document, 0, {}, {},
                                         GraphTarget::Display).pixel(0, 0)) == 0);
    REQUIRE(GraphRenderer::render(branched, document, 0, {}, {}, GraphTarget::Write) == write);
    REQUIRE(qAlpha(write.pixel(0, 0)) >= 190);
    REQUIRE(qRed(write.pixel(0, 0)) > qRed(legacy.pixel(0, 0)));
    REQUIRE(std::abs(qRed(write.pixel(0, 0)) - 118) <= 2);
    REQUIRE(std::abs(qBlue(write.pixel(0, 0)) - 160) <= 2);
    const auto reopened = deserializeDocument(serializeDocument(document));
    REQUIRE(SceneRenderer::render(reopened, 0) == write);
    auto masked = graph;
    masked.nodes.push_back({8, GraphNodeKind::MatteFromImage, 0, {{2, 0}}});
    masked.nodes.push_back({9, GraphNodeKind::ApplyMatte, 0, {{4, 0}, {8, 1}}});
    masked.nodes[5].inputs.front().source = 9;
    masked.nodes[6].inputs.front().source = 9;
    masked.validate(document);
    auto maskOutput = GraphRenderer::render(masked, document, 0, {}, {}, GraphTarget::Write);
    REQUIRE(qAlpha(maskOutput.pixel(0, 0)) == 64);
    document.layers.front().visible = false;
    document.layers.back().visible = false;
    REQUIRE(qRgba(0, 0, 0, 0) == SceneRenderer::render(document, 0).pixel(0, 0));
}
TEST_CASE("Saved cutter matte clips a layer without painting its source") {
    auto document = makeDocument();
    document.width = document.height = 1;
    document.background = {0, 0, 0, 0};
    auto& targetDrawing = document.editableDrawing(document.layers.front().id, 0);
    targetDrawing.image = ImageAsset{1, 1, {255, 0, 0, 128}};
    Layer source = document.layers.front();
    source.id = document.allocateId();
    source.name = "Cutter";
    Drawing sourceDrawing = targetDrawing;
    sourceDrawing.id = document.allocateId();
    sourceDrawing.image = ImageAsset{1, 1, {0, 0, 255, 128}};
    document.drawings.emplace(sourceDrawing.id, sourceDrawing);
    for (auto& exposure : source.exposures)
        exposure.drawing = sourceDrawing.id;
    document.layers.push_back(source);
    document.layers.front().matte = source.id;
    document.validate();
    const auto graph = CompositionGraph::orderedLayers(document);
    REQUIRE_NOTHROW(graph.validate(document));
    const auto sourceNode = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&](const auto& node) {
        return node.kind == GraphNodeKind::LayerImage && node.layer == source.id;
    });
    const auto matteNode = std::find_if(graph.nodes.begin(), graph.nodes.end(), [](const auto& node) {
        return node.kind == GraphNodeKind::MatteFromImage;
    });
    const auto appliedNode = std::find_if(graph.nodes.begin(), graph.nodes.end(), [](const auto& node) {
        return node.kind == GraphNodeKind::ApplyMatte;
    });
    REQUIRE(sourceNode != graph.nodes.end());
    REQUIRE(matteNode != graph.nodes.end());
    REQUIRE(appliedNode != graph.nodes.end());
    const auto sourcePreview = GraphRenderer::renderNode(graph, document, 0, sourceNode->id,
                                                          QSize(1, 1));
    const auto mattePreview = GraphRenderer::renderNode(graph, document, 0, matteNode->id,
                                                         QSize(1, 1));
    const auto appliedPreview = GraphRenderer::renderNode(graph, document, 0, appliedNode->id,
                                                           QSize(1, 1));
    REQUIRE(qBlue(sourcePreview.pixel(0, 0)) == 128);
    REQUIRE(qAlpha(sourcePreview.pixel(0, 0)) == 128);
    REQUIRE(qAlpha(mattePreview.pixel(0, 0)) == 128);
    REQUIRE(qAlpha(appliedPreview.pixel(0, 0)) == 64);
    REQUIRE_THROWS(GraphRenderer::renderNode(graph, document, 0, 999999, QSize(1, 1)));
    const auto expected = SceneRenderer::render(document, 0);
    REQUIRE(qAlpha(expected.pixel(0, 0)) == 64);
    REQUIRE(qBlue(expected.pixel(0, 0)) == 0);
    REQUIRE(GraphRenderer::render(graph, document, 0, {}, {}, GraphTarget::Write) == expected);
    REQUIRE(deserializeDocument(serializeDocument(document)) == document);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(document)), 0) == expected);
    document.layers.back().paintMatteSource = true;
    const auto paintedSource = SceneRenderer::render(document, 0);
    REQUIRE(qAlpha(paintedSource.pixel(0, 0)) == 160);
    REQUIRE(qBlue(paintedSource.pixel(0, 0)) > 0);
    REQUIRE(GraphRenderer::render(CompositionGraph::orderedLayers(document), document, 0,
                                  {}, {}, GraphTarget::Write) == paintedSource);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(document)), 0) == paintedSource);
    document.layers.back().paintMatteSource = false;
    REQUIRE(SceneRenderer::render(document, 0) == expected);
    document.layers.front().matteBypassed = true;
    const auto uncut = SceneRenderer::render(document, 0);
    REQUIRE(qAlpha(uncut.pixel(0, 0)) == 128);
    REQUIRE(qBlue(uncut.pixel(0, 0)) == 0);
    REQUIRE(GraphRenderer::render(CompositionGraph::orderedLayers(document), document, 0,
                                  {}, {}, GraphTarget::Write) == uncut);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(document)), 0) == uncut);
    document.layers.front().matteBypassed = false;
    document.drawings.at(sourceDrawing.id).image = ImageAsset{1, 1, {0, 0, 255, 64}};
    document.layers.front().invertMatte = true;
    document.validate();
    const auto inverted = SceneRenderer::render(document, 0);
    REQUIRE(qAlpha(inverted.pixel(0, 0)) == 96);
    REQUIRE(qBlue(inverted.pixel(0, 0)) == 0);
    REQUIRE(GraphRenderer::render(CompositionGraph::orderedLayers(document), document, 0,
                                  {}, {}, GraphTarget::Write) == inverted);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(document)), 0) == inverted);
    document.layers.front().matte = 0;
    document.layers.front().invertMatte = false;
    REQUIRE(qAlpha(SceneRenderer::render(document, 0).pixel(0, 0)) > 128);
    document.layers.front().matte = source.id;
    document.layers.back().visible = false;
    REQUIRE_THROWS(document.validate());
}
TEST_CASE("Layer opacity is a typed image node and attenuates fractional cutter alpha") {
    auto document = makeDocument();
    document.width = document.height = 1;
    document.background = {0, 0, 0, 0};
    auto& target = document.editableDrawing(document.layers.front().id, 0);
    target.image = ImageAsset{1, 1, {255, 0, 0, 128}};
    auto& layer = document.layers.front();
    layer.transform.opacity = .5;
    Keyframe first, second;
    first.frame = 0;
    first.value = layer.transform;
    second.frame = 12;
    second.value = layer.transform;
    second.value.opacity = 1;
    layer.keys = {first, second};
    Layer cutter = layer;
    cutter.id = document.allocateId();
    cutter.name = "Cutter";
    cutter.keys.clear();
    Drawing cutterDrawing = target;
    cutterDrawing.id = document.allocateId();
    cutterDrawing.image = ImageAsset{1, 1, {0, 0, 255, 64}};
    document.drawings.emplace(cutterDrawing.id, cutterDrawing);
    for (auto& exposure : cutter.exposures)
        exposure.drawing = cutterDrawing.id;
    layer.matte = cutter.id;
    document.layers.push_back(cutter);
    expose(document.layers.front(), 0, document.duration, target.id);
    expose(document.layers.back(), 0, document.duration, cutterDrawing.id);
    document.composition = CompositionProfile::LinearSrgb;
    document.validate();
    const auto graph = CompositionGraph::orderedLayers(document);
    REQUIRE(std::count_if(graph.nodes.begin(), graph.nodes.end(), [](const GraphNode& node) {
                return node.kind == GraphNodeKind::Opacity;
            }) == 2);
    const auto inside = GraphRenderer::render(graph, document, 0, {});
    REQUIRE(qAlpha(inside.pixel(0, 0)) == 8);
    REQUIRE(inside == GraphRenderer::render(graph, document, 0, {}, {}, GraphTarget::Write));
    REQUIRE(inside == SceneRenderer::render(document, 0));
    REQUIRE(qAlpha(SceneRenderer::render(document, 12).pixel(0, 0)) == 16);
    const auto reopened = deserializeDocument(serializeDocument(document));
    REQUIRE(SceneRenderer::render(reopened, 0) == inside);
    document.layers.front().invertMatte = true;
    REQUIRE(qAlpha(SceneRenderer::render(document, 0).pixel(0, 0)) == 56);
}
TEST_CASE("Bypassed opacity retains keyed values and changes both matte and output alpha") {
    auto document = makeDocument();
    document.width = document.height = 1;
    document.background = {0, 0, 0, 0};
    const Id targetId = document.layers.front().id;
    auto& targetDrawing = document.editableDrawing(targetId, 0);
    targetDrawing.image = ImageAsset{1, 1, {255, 0, 0, 128}};
    document.layer(targetId).transform.opacity = .5;
    Keyframe rest;
    rest.frame = 0;
    rest.value = document.layer(targetId).transform;
    Keyframe key;
    key.frame = 12;
    key.value = document.layer(targetId).transform;
    key.value.opacity = .25;
    document.layer(targetId).keys = {rest, key};
    Layer cutter = document.layer(targetId);
    cutter.id = document.allocateId();
    cutter.name = "Cutter";
    cutter.keys.clear();
    Drawing source = targetDrawing;
    source.id = document.allocateId();
    source.image = ImageAsset{1, 1, {0, 0, 255, 64}};
    document.drawings.emplace(source.id, source);
    for (auto& exposure : cutter.exposures)
        exposure.drawing = source.id;
    document.layers.push_back(cutter);
    expose(document.layer(targetId), 0, document.duration, targetDrawing.id);
    expose(document.layer(cutter.id), 0, document.duration, source.id);
    document.layer(targetId).matte = cutter.id;
    document.composition = CompositionProfile::LinearSrgb;
    document.validate();
    const auto baseline = SceneRenderer::render(document, 0);
    REQUIRE(qAlpha(baseline.pixel(0, 0)) == 8);
    Session session;
    session.replace(document);
    REQUIRE(session.apply("Bypass target opacity", [&](Document& d) {
        d.layer(targetId).opacityBypassed = true;
    }));
    const auto targetBypassed = session.document();
    REQUIRE(qAlpha(SceneRenderer::render(targetBypassed, 0).pixel(0, 0)) == 16);
    REQUIRE(targetBypassed.layer(targetId).keys == document.layer(targetId).keys);
    REQUIRE(session.apply("Bypass cutter opacity", [&](Document& d) {
        d.layer(cutter.id).opacityBypassed = true;
    }));
    const auto bothBypassed = session.document();
    const auto graph = CompositionGraph::orderedLayers(bothBypassed);
    REQUIRE_NOTHROW(graph.validate(bothBypassed));
    REQUIRE(std::count_if(graph.nodes.begin(), graph.nodes.end(), [](const GraphNode& node) {
                return node.kind == GraphNodeKind::BypassOpacity;
            }) == 2);
    REQUIRE(std::count_if(graph.nodes.begin(), graph.nodes.end(), [](const GraphNode& node) {
                return node.kind == GraphNodeKind::Opacity;
            }) == 0);
    const auto displayed = SceneRenderer::render(bothBypassed, 0);
    REQUIRE(qAlpha(displayed.pixel(0, 0)) == 32);
    REQUIRE(displayed == GraphRenderer::render(graph, bothBypassed, 0, {}, {},
                                               GraphTarget::Write));
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(bothBypassed)), 0) ==
            displayed);
    REQUIRE(session.undo());
    REQUIRE(session.document() == targetBypassed);
    REQUIRE(session.undo());
    REQUIRE(session.document() == document);
    REQUIRE(session.redo());
    REQUIRE(session.redo());
    REQUIRE(session.document() == bothBypassed);
    REQUIRE(qAlpha(SceneRenderer::render(bothBypassed, 12).pixel(0, 0)) == 32);
}
TEST_CASE("Opacity bypass reaches legacy scenes without a matte") {
    auto document = makeDocument();
    document.width = document.height = 1;
    document.background = {0, 0, 0, 0};
    const Id layer = document.layers.front().id;
    document.editableDrawing(layer, 0).image = ImageAsset{1, 1, {255, 0, 0, 128}};
    document.layer(layer).transform.opacity = .5;
    document.validate();
    REQUIRE(std::abs(qAlpha(SceneRenderer::render(document, 0).pixel(0, 0)) - 64) <= 1);
    document.layer(layer).opacityBypassed = true;
    document.validate();
    const auto graph = CompositionGraph::orderedLayers(document);
    const auto display = SceneRenderer::render(document, 0);
    REQUIRE(qAlpha(display.pixel(0, 0)) == 128);
    REQUIRE(display == GraphRenderer::render(graph, document, 0, {}, {}, GraphTarget::Write));
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(document)), 0) == display);
}
TEST_CASE("Multiply Screen and Add blend fractional layers in both color profiles") {
    auto document = makeDocument();
    document.width = document.height = 1;
    document.background = {0, 0, 0, 0};
    auto& lower = document.editableDrawing(document.layers.front().id, 0);
    lower.image = ImageAsset{1, 1, {50, 150, 200, 128}};
    Layer upper = document.layers.front();
    upper.id = document.allocateId();
    upper.name = "Upper";
    auto upperDrawing = lower;
    upperDrawing.id = document.allocateId();
    upperDrawing.image = ImageAsset{1, 1, {200, 100, 50, 128}};
    document.drawings.emplace(upperDrawing.id, upperDrawing);
    for (auto& exposure : upper.exposures)
        exposure.drawing = upperDrawing.id;
    document.layers.push_back(upper);
    document.validate();
    const auto normal = SceneRenderer::render(document, 0);
    Session session;
    session.replace(document);
    REQUIRE(session.apply("Multiply", [&](Document& d) {
        d.layer(upper.id).blendMode = LayerBlendMode::Multiply;
    }));
    const auto multiply = SceneRenderer::render(session.document(), 0);
    REQUIRE(qAlpha(multiply.pixel(0, 0)) == qAlpha(normal.pixel(0, 0)));
    REQUIRE(qRed(multiply.pixel(0, 0)) < qRed(normal.pixel(0, 0)));
    REQUIRE(multiply == GraphRenderer::render(CompositionGraph::orderedLayers(session.document()),
                                              session.document(), 0, {}, {}, GraphTarget::Write));
    REQUIRE(session.apply("Screen", [&](Document& d) {
        d.layer(upper.id).blendMode = LayerBlendMode::Screen;
    }));
    const auto screen = SceneRenderer::render(session.document(), 0);
    REQUIRE(qAlpha(screen.pixel(0, 0)) == qAlpha(multiply.pixel(0, 0)));
    REQUIRE(qRed(screen.pixel(0, 0)) > qRed(normal.pixel(0, 0)));
    REQUIRE(screen == GraphRenderer::render(CompositionGraph::orderedLayers(session.document()),
                                            session.document(), 0, {}, {}, GraphTarget::Display));
    REQUIRE(session.apply("Add", [&](Document& d) {
        d.layer(upper.id).blendMode = LayerBlendMode::Add;
    }));
    const auto added = SceneRenderer::render(session.document(), 0);
    REQUIRE(qAlpha(added.pixel(0, 0)) == qAlpha(screen.pixel(0, 0)));
    REQUIRE(qRed(added.pixel(0, 0)) > qRed(screen.pixel(0, 0)));
    REQUIRE(added == GraphRenderer::render(CompositionGraph::orderedLayers(session.document()),
                                           session.document(), 0, {}, {}, GraphTarget::Write));
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(session.document())), 0) ==
            added);
    REQUIRE(session.apply("Bypass blend", [&](Document& d) {
        d.layer(upper.id).blendBypassed = true;
    }));
    const auto bypassed = SceneRenderer::render(session.document(), 0);
    REQUIRE(bypassed == normal);
    const auto bypassGraph = CompositionGraph::orderedLayers(session.document());
    REQUIRE(std::count_if(bypassGraph.nodes.begin(), bypassGraph.nodes.end(), [](const auto& node) {
                return node.kind == GraphNodeKind::BypassBlend;
            }) == 1);
    REQUIRE(bypassed == GraphRenderer::render(bypassGraph, session.document(), 0, {}, {},
                                              GraphTarget::Write));
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(session.document())), 0) ==
            bypassed);
    REQUIRE(session.undo());
    REQUIRE(SceneRenderer::render(session.document(), 0) == added);
    REQUIRE(session.redo());
    REQUIRE(SceneRenderer::render(session.document(), 0) == normal);
    REQUIRE(session.undo());
    REQUIRE_THROWS(session.apply("Invalid blend", [&](Document& d) {
        d.layer(upper.id).blendMode = static_cast<LayerBlendMode>(99);
    }));
    REQUIRE(SceneRenderer::render(session.document(), 0) == added);
    REQUIRE(session.undo());
    REQUIRE(SceneRenderer::render(session.document(), 0) == screen);
    REQUIRE(session.undo());
    REQUIRE(SceneRenderer::render(session.document(), 0) == multiply);
    REQUIRE(session.undo());
    REQUIRE(session.document() == document);
    REQUIRE(session.redo());
    REQUIRE(session.redo());
    REQUIRE(session.redo());
    REQUIRE(SceneRenderer::render(session.document(), 0) == added);
    for (const auto mode : {LayerBlendMode::Multiply, LayerBlendMode::Screen,
                            LayerBlendMode::Add}) {
        auto linear = session.document();
        linear.composition = CompositionProfile::LinearSrgb;
        linear.layer(upper.id).blendMode = mode;
        linear.validate();
        const auto graph = CompositionGraph::orderedLayers(linear);
        const auto display = GraphRenderer::render(graph, linear, 0, {}, {}, GraphTarget::Display);
        REQUIRE(SceneRenderer::render(linear, 0) == display);
        REQUIRE(GraphRenderer::render(graph, linear, 0, {}, {}, GraphTarget::Write) == display);
    }
}
TEST_CASE("Inverted cutter keeps target ink outside its source bounds") {
    auto document = makeDocument();
    document.width = 2;
    document.height = 1;
    document.background = {0, 0, 0, 0};
    auto& target = document.editableDrawing(document.layers.front().id, 0);
    target.image = ImageAsset{2, 1, {255, 0, 0, 128, 255, 0, 0, 128}};
    Layer cutter = document.layers.front();
    cutter.id = document.allocateId();
    cutter.name = "Cutter";
    Drawing source = target;
    source.id = document.allocateId();
    source.image = ImageAsset{2, 1, {0, 0, 255, 64, 0, 0, 0, 0}};
    document.drawings.emplace(source.id, source);
    for (auto& exposure : cutter.exposures)
        exposure.drawing = source.id;
    document.layers.push_back(cutter);
    document.layers.front().matte = cutter.id;
    document.layers.front().invertMatte = true;
    document.validate();
    const auto output = SceneRenderer::render(document, 0);
    REQUIRE(qAlpha(output.pixel(0, 0)) == 96);
    REQUIRE(qAlpha(output.pixel(1, 0)) == 128);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(document)), 0) == output);
}
TEST_CASE("Composite bypass keeps a painted cutter usable without painting its layer") {
    auto document = makeDocument();
    document.width = document.height = 1;
    document.background = {0, 0, 0, 0};
    const Id baseId = document.layers.front().id;
    document.editableDrawing(baseId, 0).image = ImageAsset{1, 1, {255, 0, 0, 255}};
    const auto addImage = [&](std::string name, std::vector<std::uint8_t> pixels) {
        Layer layer = document.layers.front();
        layer.id = document.allocateId();
        layer.name = std::move(name);
        Drawing drawing;
        drawing.id = document.allocateId();
        drawing.image = ImageAsset{1, 1, std::move(pixels)};
        document.drawings.emplace(drawing.id, drawing);
        for (auto& exposure : layer.exposures)
            exposure.drawing = drawing.id;
        document.layers.push_back(layer);
        return layer.id;
    };
    const Id sourceId = addImage("Blue cutter", {0, 0, 255, 128});
    const Id targetId = addImage("Green target", {0, 255, 0, 255});
    document.layer(sourceId).paintMatteSource = true;
    document.layer(targetId).matte = sourceId;
    document.validate();
    const auto painted = SceneRenderer::render(document, 0);
    REQUIRE(qBlue(painted.pixel(0, 0)) > 0);
    Session session;
    session.replace(document);
    REQUIRE(session.apply("Bypass source composite", [&](Document& candidate) {
        candidate.layer(sourceId).compositeBypassed = true;
    }));
    const auto& bypassed = session.document();
    const auto graph = CompositionGraph::orderedLayers(bypassed);
    REQUIRE_NOTHROW(graph.validate(bypassed));
    const auto sourceAffected = graph.affectedByLayer(bypassed, sourceId);
    REQUIRE(std::find(sourceAffected.begin(), sourceAffected.end(), graph.write) !=
            sourceAffected.end());
    const auto display = GraphRenderer::render(graph, bypassed, 0, {}, {}, GraphTarget::Display);
    const auto write = GraphRenderer::render(graph, bypassed, 0, {}, {}, GraphTarget::Write);
    REQUIRE(display == write);
    REQUIRE(SceneRenderer::render(bypassed, 0) == write);
    REQUIRE(qBlue(write.pixel(0, 0)) == 0);
    REQUIRE(qGreen(write.pixel(0, 0)) > 0);
    REQUIRE(qAlpha(write.pixel(0, 0)) == 255);
    const auto matte = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&](const auto& node) {
        return node.kind == GraphNodeKind::MatteFromImage && node.layer == sourceId;
    });
    REQUIRE(matte != graph.nodes.end());
    REQUIRE(qAlpha(GraphRenderer::renderNode(graph, bypassed, 0, matte->id, {1, 1}).pixel(0, 0)) == 128);
    const auto reopened = deserializeDocument(serializeDocument(bypassed));
    REQUIRE(reopened == bypassed);
    REQUIRE(SceneRenderer::render(reopened, 0) == write);
    REQUIRE(session.undo());
    REQUIRE(SceneRenderer::render(session.document(), 0) == painted);
    REQUIRE(session.redo());
    REQUIRE(SceneRenderer::render(session.document(), 0) == write);

    auto unmasked = makeDocument();
    unmasked.width = unmasked.height = 1;
    unmasked.background = {0, 0, 0, 0};
    unmasked.editableDrawing(unmasked.layers.front().id, 0).image =
        ImageAsset{1, 1, {255, 0, 0, 255}};
    unmasked.layers.front().compositeBypassed = true;
    unmasked.validate();
    REQUIRE(qAlpha(SceneRenderer::render(unmasked, 0).pixel(0, 0)) == 0);
}
TEST_CASE("Composite group ports preserve matte-only sources and exact rendered pixels") {
    auto document = makeDocument();
    document.width = document.height = 1;
    document.background = {0, 0, 0, 0};
    const Id baseId = document.layers.front().id;
    document.editableDrawing(baseId, 0).image = ImageAsset{1, 1, {255, 0, 0, 255}};
    const auto addImage = [&](std::string name, std::vector<std::uint8_t> pixels) {
        auto layer = document.layers.front();
        layer.id = document.allocateId();
        layer.name = std::move(name);
        auto drawing = document.drawings.at(layer.exposures.front().drawing);
        drawing.id = document.allocateId();
        drawing.image = ImageAsset{1, 1, std::move(pixels)};
        document.drawings.emplace(drawing.id, drawing);
        for (auto& exposure : layer.exposures)
            exposure.drawing = drawing.id;
        document.layers.push_back(layer);
        return layer.id;
    };
    const Id sourceId = addImage("Cutter", {0, 0, 255, 128});
    const Id targetId = addImage("Target", {0, 255, 0, 255});
    document.layer(targetId).matte = sourceId;
    const auto original = SceneRenderer::render(document, 0);
    REQUIRE(qBlue(original.pixel(0, 0)) == 0);
    const Id groupId = document.allocateId();
    document.compositeGroups.push_back({groupId, "Body and cutter",
                                        {baseId, sourceId, targetId}});
    document.validate();
    const auto graph = CompositionGraph::orderedLayers(document);
    REQUIRE_NOTHROW(graph.validate(document));
    const auto output = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&](const auto& node) {
        return node.kind == GraphNodeKind::GroupOutput && node.group == groupId;
    });
    REQUIRE(output != graph.nodes.end());
    REQUIRE(SceneRenderer::render(document, 0) == original);
    REQUIRE(GraphRenderer::render(graph, document, 0, {}, {}, GraphTarget::Display) == original);
    REQUIRE(GraphRenderer::render(graph, document, 0, {}, {}, GraphTarget::Write) == original);
    REQUIRE(GraphRenderer::renderNode(graph, document, 0, output->id, {1, 1}) == original);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(document)), 0) == original);
    document.compositeGroups.front().bypassed = true;
    document.validate();
    const auto bypassed = SceneRenderer::render(document, 0);
    REQUIRE(qAlpha(bypassed.pixel(0, 0)) == 0);
    REQUIRE(GraphRenderer::render(graph, document, 0, {}, {}, GraphTarget::Display) == bypassed);
    REQUIRE(GraphRenderer::render(graph, document, 0, {}, {}, GraphTarget::Write) == bypassed);
    REQUIRE(GraphRenderer::renderNode(graph, document, 0, output->id, {1, 1}) == bypassed);
    REQUIRE(SceneRenderer::render(deserializeDocument(serializeDocument(document)), 0) == bypassed);
    document.compositeGroups.clear();
    REQUIRE(SceneRenderer::render(document, 0) == original);
    document.compositeGroups.push_back({groupId, "Body and cutter",
                                        {baseId, sourceId}, true});
    document.validate();
    const auto externalCut = SceneRenderer::render(document, 0);
    REQUIRE(qAlpha(externalCut.pixel(0, 0)) == 128);
    REQUIRE(qBlue(externalCut.pixel(0, 0)) == 0);
    const auto externalGraph = CompositionGraph::orderedLayers(document);
    REQUIRE(GraphRenderer::render(externalGraph, document, 0, {}, {}, GraphTarget::Display) ==
            externalCut);
    REQUIRE(GraphRenderer::render(externalGraph, document, 0, {}, {}, GraphTarget::Write) ==
            externalCut);
}
TEST_CASE("Linear color chart keeps bounded alpha and premultiplied color across coverage levels") {
    auto document = makeDocument();
    document.width = 8;
    document.height = 8;
    document.background = {0, 0, 0, 0};
    auto& lower = document.editableDrawing(document.layers.front().id, 0);
    std::vector<std::uint8_t> red(8 * 8 * 4), blue(8 * 8 * 4);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x) {
            const auto i = std::size_t((y * 8 + x) * 4);
            red[i] = 255;
            red[i + 3] = std::uint8_t(x * 36);
            blue[i + 2] = 255;
            blue[i + 3] = std::uint8_t(y * 36);
        }
    lower.image = ImageAsset{8, 8, std::move(red)};
    Layer upper = document.layers.front();
    upper.id = document.allocateId();
    upper.name = "Blue coverage";
    Drawing drawing;
    drawing.id = document.allocateId();
    drawing.image = ImageAsset{8, 8, std::move(blue)};
    document.drawings.emplace(drawing.id, drawing);
    upper.exposures = {{0, 1, drawing.id}};
    document.layers.push_back(upper);
    document.validate();
    const auto legacy = SceneRenderer::render(document, 0);
    document.composition = CompositionProfile::LinearSrgb;
    const auto graph = CompositionGraph::orderedLayers(document);
    const auto display = GraphRenderer::render(graph, document, 0, {}, {}, GraphTarget::Display);
    const auto write = GraphRenderer::render(graph, document, 0, {}, {}, GraphTarget::Write);
    REQUIRE(display == write);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x) {
            const auto pixel = write.pixel(x, y);
            REQUIRE(std::abs(qAlpha(pixel) - qAlpha(legacy.pixel(x, y))) <= 1);
            REQUIRE(qRed(pixel) <= qAlpha(pixel));
            REQUIRE(qGreen(pixel) <= qAlpha(pixel));
            REQUIRE(qBlue(pixel) <= qAlpha(pixel));
            if (x == 0 && y == 0)
                REQUIRE(pixel == qRgba(0, 0, 0, 0));
        }
}
TEST_CASE("Orthographic output camera changes framing without changing drawing coordinates") {
    auto document = makeDocument();
    document.width = document.height = 64;
    document.background = {0, 0, 0, 0};
    auto& artwork = document.editableDrawing(document.layers.front().id, 0);
    artwork.image = ImageAsset{8, 8, std::vector<std::uint8_t>(8 * 8 * 4, 255)};
    document.layers.front().transform.x = 16;
    document.layers.front().transform.y = 16;
    document.validate();
    for (const auto profile : {CompositionProfile::LegacyQt, CompositionProfile::LinearSrgb}) {
        document.composition = profile;
        const auto original = SceneRenderer::render(document, 0);
        Layer camera;
        camera.id = document.allocateId();
        camera.kind = LayerKind::Camera;
        camera.name = "Output camera";
        camera.transform.x = camera.transform.y = 32;
        document.activeCamera = camera.id;
        document.layers.push_back(camera);
        document.validate();
        REQUIRE(SceneRenderer::render(document, 0) == original);
        document.layer(camera.id).transform.x = 40;
        document.validate();
        const auto moved = SceneRenderer::render(document, 0);
        REQUIRE(qAlpha(moved.pixel(12, 20)) == 255);
        REQUIRE(qAlpha(moved.pixel(20, 20)) == 0);
        REQUIRE(document.layers.front().transform.x == 16);
        auto zoomed = document.layer(camera.id).transform;
        zoomed.scaleX = zoomed.scaleY = 2;
        document.layer(camera.id).transform = zoomed;
        document.validate();
        const auto mapped = SceneRenderer::cameraTransform(document, 0).map(QPointF(20, 20));
        REQUIRE(mapped == QPointF(-8, 8));
        const auto graph = CompositionGraph::orderedLayers(document);
        if (profile == CompositionProfile::LinearSrgb)
            REQUIRE(GraphRenderer::render(graph, document, 0, {}, {}, GraphTarget::Display) ==
                    SceneRenderer::render(document, 0));
        document.layers.pop_back();
        document.activeCamera = 0;
    }
}
TEST_CASE("Camera keys are validated and survive output save and reopen") {
    auto document = makeBouncingBall();
    Layer camera;
    camera.id = document.allocateId();
    camera.kind = LayerKind::Camera;
    camera.name = "Output camera";
    camera.transform.x = document.width / 2;
    camera.transform.y = document.height / 2;
    document.activeCamera = camera.id;
    document.layers.push_back(camera);
    auto final = camera.transform;
    final.x += 100;
    final.rotation = 20;
    final.scaleX = final.scaleY = 1.5;
    recordPose(document.layer(camera.id), 0, camera.transform);
    recordPose(document.layer(camera.id), 24, final);
    document.validate();
    REQUIRE(evaluateTransform(document.layer(camera.id), 12).x == camera.transform.x + 50);
    const auto reopened = deserializeDocument(serializeDocument(document));
    REQUIRE(reopened == document);
    for (int frame : {0, 12, 24})
        REQUIRE(SceneRenderer::render(reopened, frame, QSize(240, 135)) ==
                SceneRenderer::render(document, frame, QSize(240, 135)));
    auto invalid = document;
    invalid.layer(camera.id).transform.scaleX = 0;
    REQUIRE_THROWS(invalid.validate());
    invalid = document;
    invalid.layer(camera.id).keys.front().easing["scaleX"] = {.25, -1, .75, 2};
    REQUIRE_THROWS(invalid.validate());
    invalid = document;
    invalid.activeCamera = 0;
    REQUIRE_THROWS(invalid.validate());
    invalid = document;
    invalid.layer(camera.id).parent = document.layers.front().id;
    REQUIRE_THROWS(invalid.validate());
    invalid = document;
    invalid.layers.front().parent = camera.id;
    REQUIRE_THROWS(invalid.validate());
    invalid = document;
    auto duplicate = camera;
    duplicate.id = invalid.allocateId();
    invalid.layers.push_back(duplicate);
    REQUIRE_THROWS(invalid.validate());
}
TEST_CASE("Revision render cache reuses frames and separates view options") {
    RevisionRenderCache cache(8);
    RenderCacheKey key{7, 11, 0, 1, 1, CompositionProfile::LinearSrgb,
                       GraphTarget::Display, true, false, 1};
    int evaluations = 0;
    auto render = [&] {
        ++evaluations;
        QImage image(1, 1, QImage::Format_ARGB32_Premultiplied);
        image.fill(QColor(Qt::red));
        return image;
    };
    REQUIRE(cache.resolve(key, render) == cache.resolve(key, render));
    REQUIRE(evaluations == 1);
    key.frame = 1;
    (void)cache.resolve(key, render);
    REQUIRE(evaluations == 2);
    key.frame = 0;
    (void)cache.resolve(key, render);
    REQUIRE(evaluations == 2);
    key.onionSkin = true;
    (void)cache.resolve(key, render);
    REQUIRE(evaluations == 3);
    REQUIRE(cache.retainedBytes() <= 8);
    key.frame = 1;
    (void)cache.resolve(key, render);
    REQUIRE(evaluations == 4); // Least recently used frame was evicted.
    key.revision = 12;
    (void)cache.resolve(key, render);
    REQUIRE(evaluations == 5);
    REQUIRE(cache.retainedBytes() == 4);
    key.scene = 8;
    (void)cache.resolve(key, render);
    REQUIRE(evaluations == 6);
}
TEST_CASE("Intermediate Display cache separates node outputs across edits") {
    RevisionRenderCache cache(16);
    RenderCacheKey key{2, 3, 0, 1, 1, CompositionProfile::LinearSrgb};
    int evaluations = 0;
    auto image = [&](QColor color) {
        ++evaluations;
        QImage result(1, 1, QImage::Format_ARGB32_Premultiplied);
        result.fill(color);
        return result;
    };
    key.node = 2;
    REQUIRE(cache.resolve(key, [&] { return image(Qt::red); }).pixelColor(0, 0) == Qt::red);
    key.node = 3;
    REQUIRE(cache.resolve(key, [&] { return image(Qt::blue); }).pixelColor(0, 0) == Qt::blue);
    key.node = 2;
    REQUIRE(cache.resolve(key, [&] { return image(Qt::green); }).pixelColor(0, 0) == Qt::red);
    REQUIRE(evaluations == 2);
    key.revision = 4;
    REQUIRE(cache.resolve(key, [&] { return image(Qt::green); }).pixelColor(0, 0) == Qt::green);
    REQUIRE(evaluations == 3);
}
TEST_CASE("Late render results cannot publish after a new request or document revision") {
    RevisionRenderCache cache(4);
    RenderCacheKey old{1, 1, 0, 1, 1, CompositionProfile::LinearSrgb};
    auto oldTicket = cache.begin(old);
    auto newKey = old;
    newKey.frame = 1;
    auto newTicket = cache.begin(newKey);
    QImage image(1, 1, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    REQUIRE_FALSE(cache.current(oldTicket));
    REQUIRE_FALSE(cache.publish(oldTicket, image));
    REQUIRE(cache.current(newTicket));
    REQUIRE(cache.publish(newTicket, image));
    REQUIRE(cache.lookup(newKey).has_value());
    auto stale = cache.begin(newKey);
    newKey.revision = 2;
    (void)cache.lookup(newKey);
    REQUIRE_FALSE(cache.publish(stale, image));
    REQUIRE_FALSE(cache.lookup(newKey).has_value());
    auto fresh = cache.begin(newKey);
    REQUIRE_FALSE(cache.lookup(old).has_value());
    REQUIRE(cache.current(fresh));
    REQUIRE_FALSE(cache.publish(fresh, QImage(2, 1, QImage::Format_ARGB32_Premultiplied)));
    REQUIRE(cache.publish(fresh, image));
    cache.clear();
    REQUIRE_FALSE(cache.publish(fresh, image));
    auto latest = newKey;
    latest.revision = 3;
    REQUIRE(cache.resolve(newKey, [&] {
                (void)cache.begin(latest);
                return image;
            }).isNull());
}
TEST_CASE("A worker finishing after a scene change cannot publish its pixels") {
    RevisionRenderCache cache(16);
    RenderCacheKey key{1, 1, 0, 1, 1, CompositionProfile::LinearSrgb};
    auto ticket = cache.begin(key);
    std::promise<void> release;
    const auto ready = release.get_future().share();
    bool published = true;
    std::thread worker([&] {
        ready.wait();
        QImage pixels(1, 1, QImage::Format_ARGB32_Premultiplied);
        pixels.fill(Qt::red);
        published = cache.publish(ticket, std::move(pixels));
    });
    key.scene = 2;
    (void)cache.begin(key);
    release.set_value();
    worker.join();
    REQUIRE_FALSE(published);
    REQUIRE_FALSE(cache.lookup(key).has_value());
}
TEST_CASE("Speculative preview renders an immutable next frame and keeps the latest scene") {
    auto document = makeDocument();
    document.width = 32;
    document.height = 32;
    document.composition = CompositionProfile::LinearSrgb;
    document.background = {0, 0, 0, 0};
    auto& drawing = document.editableDrawing(document.layers.front().id, 0);
    std::vector<std::uint8_t> redPixels(32 * 32 * 4, 0);
    for (std::size_t i = 0; i < redPixels.size(); i += 4) {
        redPixels[i] = 255;
        redPixels[i + 3] = 255;
    }
    drawing.image = ImageAsset{32, 32, std::move(redPixels)};
    expose(document.layers.front(), 0, 3, drawing.id);
    document.validate();
    RevisionRenderCache cache(32 * 32 * 4);
    PreviewRenderQueue queue(cache);
    RenderCacheKey key{1, 1, 1, 32, 32, CompositionProfile::LinearSrgb};
    auto snapshot = std::make_shared<const Document>(document);
    REQUIRE(queue.request(snapshot, key));
    REQUIRE_FALSE(queue.request(snapshot, key));
    auto changedPixels = drawing.image->rgba.values();
    changedPixels[0] = 0;
    drawing.image->rgba = std::move(changedPixels);
    auto waitFor = [&](const RenderCacheKey& wanted) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (std::chrono::steady_clock::now() < deadline) {
            if (auto ready = cache.lookup(wanted))
                return *ready;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        return QImage{};
    };
    auto first = waitFor(key);
    REQUIRE_FALSE(first.isNull());
    REQUIRE(qRed(first.pixel(0, 0)) == 255);
    REQUIRE_FALSE(queue.request(snapshot, key));
    key.revision = 2;
    snapshot = std::make_shared<const Document>(document);
    REQUIRE(queue.request(snapshot, key));
    auto second = waitFor(key);
    REQUIRE_FALSE(second.isNull());
    REQUIRE(qRed(second.pixel(0, 0)) == 0);
    auto old = key;
    old.revision = 1;
    REQUIRE_FALSE(cache.lookup(old).has_value());
    REQUIRE(cache.retainedBytes() <= 32 * 32 * 4);
    queue.cancel();
}
TEST_CASE("Speculative preview skips frames that cannot fit its cache budget") {
    auto source = makeDocument();
    source.width = 1;
    source.height = 1;
    source.composition = CompositionProfile::LinearSrgb;
    auto document = std::make_shared<const Document>(source);
    RevisionRenderCache cache(3);
    PreviewRenderQueue queue(cache);
    RenderCacheKey key{1, 1, 0, document->width, document->height,
                       CompositionProfile::LinearSrgb};
    REQUIRE_FALSE(queue.request(document, key));
    REQUIRE(cache.retainedBytes() == 0);
}
TEST_CASE("Linear compositor cancellation aborts inside the current frame") {
    auto document = makeDocument();
    document.width = 32;
    document.height = 64;
    document.composition = CompositionProfile::LinearSrgb;
    auto& drawing = document.editableDrawing(document.layers.front().id, 0);
    drawing.image = ImageAsset{32, 64, std::vector<std::uint8_t>(32 * 64 * 4, 160)};
    int checks = 0;
    RenderOptions options;
    options.cancelled = [&] { return ++checks > 12; };
    REQUIRE_THROWS_AS(SceneRenderer::render(document, 0, {}, options), RenderCancelled);
    REQUIRE(checks > 12);
}
TEST_CASE("Conservative ink regions preserve rotated vector and sparse raster coverage") {
    auto document = makeDocument();
    document.width = 320;
    document.height = 180;
    document.background = {0, 0, 0, 0};
    auto& layer = document.layers.front();
    layer.transform.x = 35;
    layer.transform.y = 12;
    layer.transform.rotation = 25;
    auto& drawing = document.editableDrawing(layer.id, 0);
    drawing.strokes.push_back({document.allocateId(), document.palette.front().id, 22,
                               Shape::Stroke, false, 2,
                               {{30, 30, 1}, {160, 90, 1}}});
    drawing.raster = RasterImage{320, 180, {}};
    std::vector<std::uint16_t> pixels(64 * 64 * 4);
    for (std::size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i] = 32768;
        pixels[i + 3] = 32768;
    }
    drawing.raster->tiles[{2, 1}] = pixels;
    document.validate();
    for (const auto size : {QSize{640, 360}, QSize{80, 45}, QSize{333, 187}}) {
        const auto legacy = SceneRenderer::render(document, 0, size);
        document.composition = CompositionProfile::LinearSrgb;
        const auto linear = SceneRenderer::render(document, 0, size);
        for (int y = 0; y < legacy.height(); ++y)
            for (int x = 0; x < legacy.width(); ++x)
                REQUIRE(qAlpha(legacy.pixel(x, y)) == qAlpha(linear.pixel(x, y)));
        document.composition = CompositionProfile::LegacyQt;
    }
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
