#include "opentoon/composition_graph.h"
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
    const auto affected = graph.affectedByLayer(document.layers.front().id);
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
