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
    case GraphNodeKind::Opacity:
    case GraphNodeKind::BypassOpacity:
    case GraphNodeKind::Over:
    case GraphNodeKind::Multiply:
    case GraphNodeKind::Screen:
    case GraphNodeKind::Add:
    case GraphNodeKind::BypassBlend:
    case GraphNodeKind::BypassComposite:
    case GraphNodeKind::ApplyMatte:
    case GraphNodeKind::BypassMatte:
    case GraphNodeKind::DisplayOutput:
    case GraphNodeKind::WriteOutput:
        return GraphPortType::Image;
    case GraphNodeKind::LayerTransform:
        return GraphPortType::Transform;
    case GraphNodeKind::MatteFromImage:
    case GraphNodeKind::InvertMatte:
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
    case GraphNodeKind::Opacity:
    case GraphNodeKind::BypassOpacity:
        return {GraphPortType::Image};
    case GraphNodeKind::Over:
    case GraphNodeKind::Multiply:
    case GraphNodeKind::Screen:
    case GraphNodeKind::Add:
    case GraphNodeKind::BypassBlend:
        return {GraphPortType::Image, GraphPortType::Image};
    case GraphNodeKind::BypassComposite:
        return {GraphPortType::Image};
    case GraphNodeKind::MatteFromImage:
        return {GraphPortType::Image};
    case GraphNodeKind::InvertMatte:
        return {GraphPortType::Matte};
    case GraphNodeKind::ApplyMatte:
        return {GraphPortType::Image, GraphPortType::Matte};
    case GraphNodeKind::BypassMatte:
        return {GraphPortType::Image};
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
    std::map<Id, GraphNodeId> sourceIds;
    std::set<Id> matteSources;
    for (const auto& layer : document.layers) {
        if (layer.kind != LayerKind::Drawing && layer.kind != LayerKind::Part)
            continue;
        const GraphNodeId imageNode = next++;
        graph.nodes.push_back({imageNode, GraphNodeKind::LayerImage, layer.id, {}});
        sourceIds[layer.id] = imageNode;
        const bool hasOpacity = layer.transform.opacity != 1 ||
            std::any_of(layer.keys.begin(), layer.keys.end(), [](const Keyframe& key) {
                return key.value.opacity != 1;
            });
        if (hasOpacity) {
            sourceIds[layer.id] = next++;
            graph.nodes.push_back({sourceIds[layer.id], layer.opacityBypassed
                                                        ? GraphNodeKind::BypassOpacity
                                                        : GraphNodeKind::Opacity, layer.id,
                                   {{imageNode, 0}}});
        }
        if (layer.matte)
            matteSources.insert(layer.matte);
    }
    for (const auto& layer : document.layers) {
        if (layer.kind != LayerKind::Drawing && layer.kind != LayerKind::Part)
            continue;
        if (matteSources.contains(layer.id) && !layer.paintMatteSource)
            continue;
        GraphNodeId source = sourceIds.at(layer.id);
        if (layer.matte) {
            if (layer.matteBypassed) {
                const GraphNodeId bypassed = next++;
                graph.nodes.push_back({bypassed, GraphNodeKind::BypassMatte, layer.id,
                                       {{source, 0}}});
                source = bypassed;
            } else {
                GraphNodeId matte = next++;
                graph.nodes.push_back({matte, GraphNodeKind::MatteFromImage, layer.matte,
                                       {{sourceIds.at(layer.matte), 0}}});
                if (layer.invertMatte) {
                    const GraphNodeId inverted = next++;
                    graph.nodes.push_back({inverted, GraphNodeKind::InvertMatte, layer.id, {{matte, 0}}});
                    matte = inverted;
                }
                const GraphNodeId masked = next++;
                graph.nodes.push_back({masked, GraphNodeKind::ApplyMatte, layer.id,
                                       {{source, 0}, {matte, 1}}});
                source = masked;
            }
        }
        if (layer.compositeBypassed) {
            const GraphNodeId bypassed = next++;
            graph.nodes.push_back({bypassed, GraphNodeKind::BypassComposite, layer.id,
                                   {{image, 0}}});
            image = bypassed;
            continue;
        }
        const GraphNodeId composite = next++;
        const auto kind = layer.blendBypassed && layer.blendMode != LayerBlendMode::Normal
                              ? GraphNodeKind::BypassBlend
                              : layer.blendMode == LayerBlendMode::Multiply
                              ? GraphNodeKind::Multiply
                              : layer.blendMode == LayerBlendMode::Screen
                                    ? GraphNodeKind::Screen
                                    : layer.blendMode == LayerBlendMode::Add
                                          ? GraphNodeKind::Add
                                          : GraphNodeKind::Over;
        graph.nodes.push_back({composite, kind, layer.id, {{image, 0}, {source, 1}}});
        image = composite;
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
        if (node.kind == GraphNodeKind::LayerImage || node.kind == GraphNodeKind::LayerTransform ||
            node.kind == GraphNodeKind::Opacity || node.kind == GraphNodeKind::BypassOpacity ||
            node.kind == GraphNodeKind::BypassMatte) {
            const auto& layer = document.layer(node.layer);
            if ((node.kind == GraphNodeKind::Opacity ||
                 node.kind == GraphNodeKind::BypassOpacity ||
                 node.kind == GraphNodeKind::BypassMatte) &&
                layer.kind != LayerKind::Drawing && layer.kind != LayerKind::Part)
                throw std::invalid_argument("This image node requires a drawing source.");
        }
        else if (node.layer) {
            if (node.kind != GraphNodeKind::MatteFromImage &&
                node.kind != GraphNodeKind::InvertMatte &&
                node.kind != GraphNodeKind::ApplyMatte &&
                node.kind != GraphNodeKind::Over && node.kind != GraphNodeKind::Multiply &&
                node.kind != GraphNodeKind::Screen && node.kind != GraphNodeKind::Add &&
                node.kind != GraphNodeKind::BypassBlend &&
                node.kind != GraphNodeKind::BypassComposite)
                throw std::invalid_argument("This compositor node cannot reference a layer.");
            const auto& layer = document.layer(node.layer);
            if (layer.kind != LayerKind::Drawing && layer.kind != LayerKind::Part)
                throw std::invalid_argument("A layer composite needs a drawing or Part owner.");
        }
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
        if (node.layer && node.kind != GraphNodeKind::BypassComposite &&
            layerBranch.contains(node.layer) && affected.insert(node.id).second)
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
