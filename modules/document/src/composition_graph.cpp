#include "opentoon/composition_graph.h"
#include <algorithm>
#include <functional>
#include <map>
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
    for (const auto& node : nodes) {
        if (!node.id || !indexed.emplace(node.id, &node).second)
            throw std::invalid_argument("Duplicate or invalid compositor node ID.");
    }
    std::map<GraphNodeId, int> state;
    std::vector<GraphNodeId> ordered;
    ordered.reserve(nodes.size());
    std::function<void(GraphNodeId)> visit = [&](GraphNodeId id) {
        if (!indexed.contains(id))
            throw std::invalid_argument("Compositor references a missing node.");
        if (state[id] == 1)
            throw std::invalid_argument("Compositor graph contains a cycle.");
        if (state[id] == 2)
            return;
        state[id] = 1;
        for (const auto& input : indexed.at(id)->inputs)
            visit(input.source);
        state[id] = 2;
        ordered.push_back(id);
    };
    for (const auto& node : nodes)
        visit(node.id);
    return ordered;
}
std::vector<GraphNodeId> CompositionGraph::affectedByLayer(Id layer) const {
    std::set<GraphNodeId> affected;
    for (const auto& node : nodes)
        if (node.layer == layer)
            affected.insert(node.id);
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& node : nodes)
            if (!affected.contains(node.id) &&
                std::any_of(node.inputs.begin(), node.inputs.end(), [&](const auto& input) {
                    return affected.contains(input.source);
                }))
                changed = affected.insert(node.id).second || changed;
    }
    return {affected.begin(), affected.end()};
}
} // namespace opentoon
