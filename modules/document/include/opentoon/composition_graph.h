#pragma once
#include "opentoon/document.h"
#include <cstdint>

namespace opentoon {
using GraphNodeId = std::uint32_t;
enum class GraphPortType : std::uint8_t { Image, Transform, Matte };
enum class GraphNodeKind : std::uint8_t {
    Background, LayerImage, LayerTransform, Over, MatteFromImage,
    ApplyMatte, DisplayOutput, WriteOutput
};
struct GraphConnection {
    GraphNodeId source = 0;
    std::uint8_t slot = 0;
    auto operator<=>(const GraphConnection&) const = default;
};
struct GraphNode {
    GraphNodeId id = 0;
    GraphNodeKind kind = GraphNodeKind::Background;
    Id layer = 0;
    std::vector<GraphConnection> inputs;
    auto operator<=>(const GraphNode&) const = default;
};
enum class GraphTarget : std::uint8_t { Display, Write };
struct CompositionGraph {
    std::vector<GraphNode> nodes;
    GraphNodeId display = 0;
    GraphNodeId write = 0;

    static CompositionGraph orderedLayers(const Document&);
    void validate(const Document&) const;
    [[nodiscard]] std::vector<GraphNodeId> topologicalOrder() const;
    [[nodiscard]] std::vector<GraphNodeId> affectedByLayer(Id layer) const;
};
} // namespace opentoon
