#pragma once
#include "opentoon/document.h"
namespace opentoon {
void insertStrokePoint(Stroke&, std::size_t segment, double fraction);
void removeStrokePoint(Stroke&, std::size_t point);
struct PixelRect {
    int x = 0, y = 0, width = 0, height = 0;
    auto operator<=>(const PixelRect&) const = default;
};
enum class SelectionMedia { Vectors, Raster, Both };
enum class SelectionAction { Move, Duplicate, Delete, FlipHorizontal, FlipVertical, RotateClockwise };
struct SelectionTransform {
    double a = 1, b = 0, c = 0, d = 1, tx = 0, ty = 0;
};
// Explicit vector IDs freeze selection membership during interactive transforms.
void transformDrawingSelection(Drawing&, PixelRect, SelectionMedia, const std::vector<Id>&,
                               SelectionTransform);
[[nodiscard]] std::optional<PixelRect> strokeBounds(const Stroke&);
[[nodiscard]] std::optional<PixelRect> strokeSelectionBounds(const Drawing&, const std::vector<Id>&);
[[nodiscard]] std::vector<Id> enclosedStrokes(const Drawing&, PixelRect);
// Half-open pixel bounds. Vector strokes must be wholly enclosed including their width.
// Imported ImageAsset is excluded. Apply inside Session::apply for document-wide atomicity.
void editDrawingSelection(Drawing&, PixelRect, SelectionMedia, SelectionAction, int dx, int dy, Id& nextId,
                          const std::vector<Id>* selectedIds = nullptr,
                          std::vector<Id>* resultingIds = nullptr);
} // namespace opentoon
