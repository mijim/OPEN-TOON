#pragma once
#include "opentoon/composition_graph.h"
#include "scene_renderer.h"

namespace opentoon {
class GraphRenderer {
  public:
    static QImage render(const CompositionGraph&, const Document&, Frame, QSize,
                         RenderOptions = {}, GraphTarget = GraphTarget::Write);
    static Transform evaluatedTransform(const GraphNode&, const Document&, Frame);
};
} // namespace opentoon
