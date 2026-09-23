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
    const auto order = graph.topologicalOrder();
    REQUIRE(order.size() == graph.nodes.size());
    const auto affected = graph.affectedByLayer(document, document.layers.front().id);
    REQUIRE(std::find(affected.begin(), affected.end(), graph.display) != affected.end());
    REQUIRE(std::find(affected.begin(), affected.end(), graph.write) != affected.end());
    REQUIRE(std::find(affected.begin(), affected.end(), graph.nodes.front().id) == affected.end());
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
}

TEST_CASE("Composition profile is a validated document property") {
    auto document = makeDocument();
    document.composition = CompositionProfile::LinearSrgb;
    REQUIRE_NOTHROW(document.validate());
    document.composition = static_cast<CompositionProfile>(99);
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
