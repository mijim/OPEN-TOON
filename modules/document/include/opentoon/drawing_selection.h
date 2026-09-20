#pragma once
#include "opentoon/document.h"
namespace opentoon {
struct PixelRect {
    int x = 0, y = 0, width = 0, height = 0;
    auto operator<=>(const PixelRect&) const = default;
};
enum class SelectionMedia { Vectors, Raster, Both };
enum class SelectionAction { Move, Duplicate, Delete, FlipHorizontal, FlipVertical, RotateClockwise };
[[nodiscard]] std::vector<Id> enclosedStrokes(const Drawing&, PixelRect);
// Half-open pixel bounds. Vector strokes must be wholly enclosed including their width.
// Imported ImageAsset is excluded. Apply inside Session::apply for document-wide atomicity.
void editDrawingSelection(Drawing&, PixelRect, SelectionMedia, SelectionAction, int dx, int dy, Id& nextId);
} // namespace opentoon
