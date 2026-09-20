#pragma once
#include "opentoon/drawing_selection.h"
#include <string_view>
namespace opentoon {
struct VectorBlock {
    std::vector<Stroke> strokes;
    std::vector<Swatch> palette;
};
// Clipboard positions stay in drawing-local units; destination receives independent IDs.
VectorBlock copyVectors(const Document&, Id layer, Frame, const std::vector<Id>&);
std::vector<Id> pasteVectors(Document&, Id layer, Frame, const VectorBlock&, double dx = 0, double dy = 0);
enum class VectorLayout { Left, CenterX, Right, Top, CenterY, Bottom, DistributeX, DistributeY };
enum class VectorOrder { Back, Backward, Forward, Front };
void arrangeVectors(Drawing&, const std::vector<Id>&, VectorLayout);
void orderVectors(Drawing&, const std::vector<Id>&, VectorOrder);
void styleVectors(Drawing&, const std::vector<Id>&, std::string_view property, double value);
void smoothVectors(Drawing&, const std::vector<Id>&, double strength);
// Open sampled strokes only. Centerline tolerance in local pixels; pressure error <= .02.
void simplifyVectors(Drawing&, const std::vector<Id>&, double tolerance);
} // namespace opentoon
