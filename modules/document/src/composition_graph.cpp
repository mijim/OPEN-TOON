#include "opentoon/composition_graph.h"
#include <algorithm>
#include <functional>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>

namespace opentoon {
namespace {
GraphPortType outputType(GraphNodeKind kind) {
    switch (kind) {
    case GraphNodeKind::Background:
    case GraphNodeKind::LayerImage:
    case GraphNodeKind::Over:
    case GraphNodeKind::ApplyMatte:
    case GraphNodeKind::DisplayOutput:
    case GraphNodeKind::WriteOutput:
        return GraphPortType::Image;
    case GraphNodeKind::LayerTransform:
        return GraphPortType::Transform;
    case GraphNodeKind::MatteFromImage:
        return GraphPortType::Matte;
    }
    throw std::invalid_argument("Unknown compositor node kind.");
}
std::vector<GraphPortType> inputTypes(GraphNodeKind kind) {
    switch (kind) {
    case GraphNodeKind::Background:
    case GraphNodeKind::LayerImage:
    case GraphNodeKind::LayerTransform:
        return {};
    case GraphNodeKind::Over:
        return {GraphPortType::Image, GraphPortType::Image};
    case GraphNodeKind::MatteFromImage:
        return {GraphPortType::Image};
    case GraphNodeKind::ApplyMatte:
        return {GraphPortType::Image, GraphPortType::Matte};
    case GraphNodeKind::DisplayOutput:
    case GraphNodeKind::WriteOutput:
        return {GraphPortType::Image};
    }
    throw std::invalid_argument("Unknown compositor node kind.");
}
} // namespace
CompositionGraph CompositionGraph::orderedLayers(const Document& document) {
    CompositionGraph graph;
    GraphNodeId next = 1;
    graph.nodes.push_back({next++, GraphNodeKind::Background, 0, {}});
    GraphNodeId image = graph.nodes.front().id;
    for (const auto& layer : document.layers) {
        if (layer.kind != LayerKind::Drawing && layer.kind != LayerKind::Part)
            continue;
        const GraphNodeId source = next++;
        graph.nodes.push_back({source, GraphNodeKind::LayerImage, layer.id, {}});
        const GraphNodeId over = next++;
        graph.nodes.push_back({over, GraphNodeKind::Over, 0, {{image, 0}, {source, 1}}});
        image = over;
    }
    graph.display = next++;
    graph.nodes.push_back({graph.display, GraphNodeKind::DisplayOutput, 0, {{image, 0}}});
    graph.write = next++;
    graph.nodes.push_back({graph.write, GraphNodeKind::WriteOutput, 0, {{image, 0}}});
    return graph;
}
void CompositionGraph::validate(const Document& document) const {
    if (nodes.empty() || nodes.size() > 10000)
        throw std::invalid_argument("Compositor graph exceeds its node limit.");
    std::map<GraphNodeId, const GraphNode*> indexed;
    for (const auto& node : nodes) {
        if (!node.id || !indexed.emplace(node.id, &node).second)
            throw std::invalid_argument("Duplicate or invalid compositor node ID.");
        (void)outputType(node.kind);
        if (node.kind == GraphNodeKind::LayerImage || node.kind == GraphNodeKind::LayerTransform)
            (void)document.layer(node.layer);
        else if (node.layer)
            throw std::invalid_argument("This compositor node cannot reference a layer.");
    }
    if (!indexed.contains(display) || !indexed.contains(write) || display == write ||
        indexed.at(display)->kind != GraphNodeKind::DisplayOutput ||
        indexed.at(write)->kind != GraphNodeKind::WriteOutput)
        throw std::invalid_argument("Compositor outputs are missing or invalid.");
    for (const auto& node : nodes) {
        const auto ports = inputTypes(node.kind);
        if (node.inputs.size() != ports.size())
            throw std::invalid_argument("Compositor input count is invalid.");
        std::set<std::uint8_t> connected;
        for (const auto& input : node.inputs) {
            if (input.slot >= ports.size() || !connected.insert(input.slot).second ||
                !indexed.contains(input.source))
                throw std::invalid_argument("Compositor input is missing or duplicated.");
            if (outputType(indexed.at(input.source)->kind) != ports[input.slot])
                throw std::invalid_argument("Compositor port types are incompatible.");
        }
    }
    (void)topologicalOrder();
}
std::vector<GraphNodeId> CompositionGraph::topologicalOrder() const {
    std::map<GraphNodeId, const GraphNode*> indexed;
    std::map<GraphNodeId, std::size_t> pending;
    std::map<GraphNodeId, std::vector<GraphNodeId>> consumers;
    for (const auto& node : nodes) {
        if (!node.id || !indexed.emplace(node.id, &node).second)
            throw std::invalid_argument("Duplicate or invalid compositor node ID.");
        pending.emplace(node.id, node.inputs.size());
    }
    for (const auto& node : nodes)
        for (const auto& input : node.inputs) {
            if (!indexed.contains(input.source))
                throw std::invalid_argument("Compositor references a missing node.");
            consumers[input.source].push_back(node.id);
        }
    std::priority_queue<GraphNodeId, std::vector<GraphNodeId>, std::greater<>> ready;
    for (const auto& [id, count] : pending)
        if (count == 0)
            ready.push(id);
    std::vector<GraphNodeId> ordered;
    ordered.reserve(nodes.size());
    while (!ready.empty()) {
        const auto id = ready.top();
        ready.pop();
        ordered.push_back(id);
        for (const auto consumer : consumers[id])
            if (--pending.at(consumer) == 0)
                ready.push(consumer);
    }
    if (ordered.size() != nodes.size())
        throw std::invalid_argument("Compositor graph contains a cycle.");
    return ordered;
}
std::vector<GraphNodeId> CompositionGraph::affectedByLayer(const Document& document,
                                                           Id layer) const {
    (void)document.layer(layer);
    std::map<Id, std::vector<Id>> children;
    for (const auto& item : document.layers)
        if (item.parent)
            children[item.parent].push_back(item.id);
    std::set<Id> layerBranch;
    std::queue<Id> layers;
    layers.push(layer);
    while (!layers.empty()) {
        const auto id = layers.front();
        layers.pop();
        if (!layerBranch.insert(id).second)
            continue;
        for (const auto child : children[id])
            layers.push(child);
    }
    std::set<GraphNodeId> affected;
    std::map<GraphNodeId, std::vector<GraphNodeId>> consumers;
    for (const auto& node : nodes)
        for (const auto& input : node.inputs)
            consumers[input.source].push_back(node.id);
    std::queue<GraphNodeId> queue;
    for (const auto& node : nodes)
        if (node.layer && layerBranch.contains(node.layer) && affected.insert(node.id).second)
            queue.push(node.id);
    while (!queue.empty()) {
        const auto id = queue.front();
        queue.pop();
        for (const auto consumer : consumers[id])
            if (affected.insert(consumer).second)
                queue.push(consumer);
    }
    return {affected.begin(), affected.end()};
}
} // namespace opentoon
