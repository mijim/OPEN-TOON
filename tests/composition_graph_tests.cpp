#include "opentoon/composition_graph.h"
#include "opentoon/rigging.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
using namespace opentoon;

TEST_CASE("Ordered composition has typed outputs and invalidates layer descendants") {
    auto document = makeDocument();
    auto graph = CompositionGraph::orderedLayers(document);
    REQUIRE_NOTHROW(graph.validate(document));
    REQUIRE(graph.display != graph.write);
    REQUIRE(std::count_if(graph.nodes.begin(), graph.nodes.end(), [&](const auto& node) {
                return node.kind == GraphNodeKind::Over &&
                       node.layer == document.layers.front().id;
            }) == 1);
    const auto order = graph.topologicalOrder();
    REQUIRE(order.size() == graph.nodes.size());
    const auto affected = graph.affectedByLayer(document, document.layers.front().id);
    REQUIRE(std::find(affected.begin(), affected.end(), graph.display) != affected.end());
    REQUIRE(std::find(affected.begin(), affected.end(), graph.write) != affected.end());
    REQUIRE(std::find(affected.begin(), affected.end(), graph.nodes.front().id) == affected.end());
}

TEST_CASE("Bypassed composite disconnects layer ink without removing its source node") {
    auto document = makeDocument();
    const Id drawing = document.layers.front().id;
    document.layer(drawing).compositeBypassed = true;
    const auto graph = CompositionGraph::orderedLayers(document);
    REQUIRE_NOTHROW(graph.validate(document));
    REQUIRE(std::count_if(graph.nodes.begin(), graph.nodes.end(), [&](const auto& node) {
                return node.kind == GraphNodeKind::BypassComposite && node.layer == drawing &&
                       node.inputs.size() == 1;
            }) == 1);
    REQUIRE(std::count_if(graph.nodes.begin(), graph.nodes.end(), [&](const auto& node) {
                return node.kind == GraphNodeKind::LayerImage && node.layer == drawing;
            }) == 1);
    const auto affected = graph.affectedByLayer(document, drawing);
    REQUIRE(std::find(affected.begin(), affected.end(), graph.write) == affected.end());
    document.layer(drawing).compositeBypassed = false;
    const auto enabled = CompositionGraph::orderedLayers(document);
    const auto enabledAffected = enabled.affectedByLayer(document, drawing);
    REQUIRE(std::find(enabledAffected.begin(), enabledAffected.end(), enabled.write) !=
            enabledAffected.end());
    auto characterDocument = makeDocument();
    const auto root = makeCharacter(characterDocument, characterDocument.layers.front().id,
                                    "Character");
    Layer peg;
    peg.id = characterDocument.allocateId();
    peg.kind = LayerKind::Peg;
    peg.name = "Peg";
    peg.parent = root;
    characterDocument.layers.push_back(peg);
    REQUIRE_NOTHROW(characterDocument.validate());
    characterDocument.layer(peg.id).compositeBypassed = true;
    REQUIRE_THROWS(characterDocument.validate());
}

TEST_CASE("Composition rejects cycles, dangling edges, wrong ports and duplicate slots") {
    auto document = makeDocument();
    auto baseline = CompositionGraph::orderedLayers(document);
    auto graph = baseline;
    graph.nodes.back().inputs.front().source = 99999;
    REQUIRE_THROWS(graph.validate(document));
    graph = baseline;
    graph.nodes.back().inputs.front().source = graph.write;
    REQUIRE_THROWS(graph.validate(document));
    graph = baseline;
    graph.nodes.insert(graph.nodes.end() - 2,
                       {99, GraphNodeKind::LayerTransform, document.layers.front().id, {}});
    graph.nodes[2].inputs[0].source = 99;
    REQUIRE_THROWS(graph.validate(document));
    graph = baseline;
    graph.nodes[2].inputs[1].slot = 0;
    REQUIRE_THROWS(graph.validate(document));
    graph = baseline;
    graph.nodes[1].layer = 99999;
    REQUIRE_THROWS(graph.validate(document));
    graph = baseline;
    graph.nodes[2].layer = 99999;
    REQUIRE_THROWS(graph.validate(document));
}

TEST_CASE("Composition profile is a validated document property") {
    auto document = makeDocument();
    document.composition = CompositionProfile::LinearSrgb;
    REQUIRE_NOTHROW(document.validate());
    document.composition = static_cast<CompositionProfile>(99);
    REQUIRE_THROWS(document.validate());
}
TEST_CASE("Cutter matte requires an independent visible drawing source") {
    auto document = makeDocument();
    const Id target = document.layers.front().id;
    Layer source = document.layers.front();
    source.id = document.allocateId();
    source.name = "Cutter";
    document.layers.push_back(source);
    document.layer(target).matte = source.id;
    REQUIRE_NOTHROW(document.validate());
    const auto graph = CompositionGraph::orderedLayers(document);
    REQUIRE_NOTHROW(graph.validate(document));
    REQUIRE(std::count_if(graph.nodes.begin(), graph.nodes.end(), [&](const auto& node) {
                return node.kind == GraphNodeKind::MatteFromImage && node.layer == source.id;
            }) == 1);
    REQUIRE(std::count_if(graph.nodes.begin(), graph.nodes.end(), [&](const auto& node) {
                return node.kind == GraphNodeKind::ApplyMatte && node.layer == target;
            }) == 1);
    REQUIRE(std::count_if(graph.nodes.begin(), graph.nodes.end(), [](const auto& node) {
                return node.kind == GraphNodeKind::ApplyMatte;
            }) == 1);
    document.layer(target).invertMatte = true;
    REQUIRE_NOTHROW(document.validate());
    const auto outside = CompositionGraph::orderedLayers(document);
    REQUIRE(std::count_if(outside.nodes.begin(), outside.nodes.end(), [&](const auto& node) {
                return node.kind == GraphNodeKind::InvertMatte && node.layer == target;
            }) == 1);
    REQUIRE(std::count_if(outside.nodes.begin(), outside.nodes.end(), [](const auto& node) {
                return node.kind == GraphNodeKind::InvertMatte;
            }) == 1);
    REQUIRE_NOTHROW(outside.validate(document));
    document.layer(target).matteBypassed = true;
    const auto bypassed = CompositionGraph::orderedLayers(document);
    REQUIRE(std::count_if(bypassed.nodes.begin(), bypassed.nodes.end(), [](const auto& node) {
                return node.kind == GraphNodeKind::BypassMatte;
            }) == 1);
    REQUIRE(std::count_if(bypassed.nodes.begin(), bypassed.nodes.end(), [](const auto& node) {
                return node.kind == GraphNodeKind::ApplyMatte;
            }) == 0);
    const auto sourceAffected = bypassed.affectedByLayer(document, source.id);
    REQUIRE(std::find(sourceAffected.begin(), sourceAffected.end(), bypassed.write) ==
            sourceAffected.end());
    document.layer(source.id).paintMatteSource = true;
    const auto painted = CompositionGraph::orderedLayers(document);
    REQUIRE_NOTHROW(painted.validate(document));
    REQUIRE(std::count_if(painted.nodes.begin(), painted.nodes.end(), [](const auto& node) {
                return node.kind == GraphNodeKind::Over;
            }) == 2);
    const auto paintedAffected = painted.affectedByLayer(document, source.id);
    REQUIRE(std::find(paintedAffected.begin(), paintedAffected.end(), painted.write) !=
            paintedAffected.end());
    document.layer(source.id).paintMatteSource = false;
    document.layer(target).matteBypassed = false;
    document.layer(target).invertMatte = false;
    document.layer(target).matte = target;
    REQUIRE_THROWS(document.validate());
    document.layer(target).matte = 999999;
    REQUIRE_THROWS(document.validate());
    document.layer(target).matte = source.id;
    document.layer(source.id).visible = false;
    REQUIRE_THROWS(document.validate());
    document.layer(source.id).visible = true;
    document.layer(source.id).matte = target;
    REQUIRE_THROWS(document.validate());
    document.layer(source.id).matte = 0;
    document.layer(target).matte = 0;
    document.layer(target).invertMatte = true;
    REQUIRE_THROWS(document.validate());
    document.layer(target).invertMatte = false;
    document.layer(target).matteBypassed = true;
    REQUIRE_THROWS(document.validate());
}
TEST_CASE("Deep composition chains order and invalidate without recursive traversal") {
    auto document = makeDocument();
    CompositionGraph graph;
    graph.nodes.push_back({1, GraphNodeKind::Background, 0, {}});
    graph.nodes.push_back({2, GraphNodeKind::LayerImage, document.layers.front().id, {}});
    GraphNodeId last = 1;
    for (GraphNodeId id = 3; id < 8003; ++id) {
        graph.nodes.push_back({id, GraphNodeKind::Over, 0, {{last, 0}, {2, 1}}});
        last = id;
    }
    graph.display = 8003;
    graph.write = 8004;
    graph.nodes.push_back({graph.display, GraphNodeKind::DisplayOutput, 0, {{last, 0}}});
    graph.nodes.push_back({graph.write, GraphNodeKind::WriteOutput, 0, {{last, 0}}});
    REQUIRE_NOTHROW(graph.validate(document));
    REQUIRE(graph.topologicalOrder().size() == graph.nodes.size());
    const auto affected = graph.affectedByLayer(document, document.layers.front().id);
    REQUIRE(affected.size() == graph.nodes.size() - 1);
}
TEST_CASE("Changing a peg invalidates images of its descendant parts") {
    auto document = makeDocument();
    const auto part = document.layers.front().id;
    const auto root = makeCharacter(document, part, "Character");
    Layer peg;
    peg.id = document.allocateId();
    peg.name = "Arm peg";
    peg.kind = LayerKind::Peg;
    peg.parent = root;
    document.layers.push_back(peg);
    document.layer(part).parent = peg.id;
    document.validate();
    const auto graph = CompositionGraph::orderedLayers(document);
    const auto affected = graph.affectedByLayer(document, peg.id);
    REQUIRE(affected.size() == 4); // Source, Over, Display, Write.
    REQUIRE_THROWS(graph.affectedByLayer(document, 999999));
}
