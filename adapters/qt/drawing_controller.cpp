#include "editor_controller.h"
#include <cmath>
using namespace opentoon;
void EditorController::setBrushOpacity(double value) {
    if (!std::isfinite(value))
        return;
    brushOpacity_ = std::clamp(value, 0.0, 1.0);
    emit toolChanged();
}
bool EditorController::editDrawingRegion(PixelRect rect, SelectionMedia media, SelectionAction action, int dx,
                                         int dy) {
    if (!layer_)
        return false;
    return edit("Edit drawing selection", [&](Document& d) {
        if (!d.drawingAt(layer_, frame_))
            return;
        auto& drawing = d.editableDrawing(layer_, frame_);
        editDrawingSelection(drawing, rect, media, action, dx, dy, d.nextId);
    });
}
