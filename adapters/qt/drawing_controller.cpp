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
                                         int dy, const std::vector<Id>* selectedIds,
                                         std::vector<Id>* resultingIds) {
    if (!layer_)
        return false;
    std::vector<Id> output;
    const bool success = edit("Edit drawing selection", [&](Document& d) {
        if (!d.drawingAt(layer_, frame_))
            return;
        auto& drawing = d.editableDrawing(layer_, frame_);
        editDrawingSelection(drawing, rect, media, action, dx, dy, d.nextId, selectedIds, &output);
    });
    if (success && resultingIds)
        *resultingIds = std::move(output);
    return success;
}

bool EditorController::transformDrawingRegion(PixelRect rect, SelectionMedia media,
                                              const std::vector<Id>& ids, SelectionTransform matrix) {
    if (!layer_)
        return false;
    return edit("Transform drawing selection", [&](Document& d) {
        if (!d.drawingAt(layer_, frame_))
            return;
        transformDrawingSelection(d.editableDrawing(layer_, frame_), rect, media, ids, matrix);
    });
}
bool EditorController::setStrokeProperty(Id id, QString name, double value) {
    if (!layer_ || !std::isfinite(value))
        return false;
    return edit("Set selected object property", [&](Document& d) {
        if (!d.drawingAt(layer_, frame_))
            return;
        for (auto& stroke : d.editableDrawing(layer_, frame_).strokes)
            if (stroke.id == id) {
                if (name == "strokeWidth")
                    stroke.width = value;
                else if (name == "filled")
                    stroke.filled = value != 0;
                else if (name == "artLayer") {
                    if (value < 0 || value > 3)
                        throw std::invalid_argument("Invalid art layer.");
                    stroke.artLayer = int(value);
                } else if (name == "swatch") {
                    if (value < 1 || value > 1000000000)
                        throw std::invalid_argument("Invalid swatch.");
                    stroke.swatch = Id(value);
                }
            }
    });
}

bool EditorController::insertPoint(opentoon::Id id, int segment, double fraction) {
    if (!layer_ || segment < 0)
        return false;
    return edit("Insert vector point", [&](opentoon::Document& d) {
        auto& drawing = d.editableDrawing(layer_, frame_);
        for (auto& stroke : drawing.strokes)
            if (stroke.id == id) {
                opentoon::insertStrokePoint(stroke, segment, fraction);
                return;
            }
        throw std::runtime_error("The selected stroke no longer exists.");
    });
}
bool EditorController::deletePoint(opentoon::Id id, int point) {
    if (!layer_ || point < 0)
        return false;
    return edit("Delete vector point", [&](opentoon::Document& d) {
        auto& drawing = d.editableDrawing(layer_, frame_);
        for (auto& stroke : drawing.strokes)
            if (stroke.id == id) {
                opentoon::removeStrokePoint(stroke, point);
                return;
            }
        throw std::runtime_error("The selected stroke no longer exists.");
    });
}

bool EditorController::editVectors(const std::vector<Id>& ids, QString operation, double value) {
    if (!layer_)
        return false;
    return edit("Edit vector group", [&](Document& d) {
        auto& drawing = d.editableDrawing(layer_, frame_);
        if (operation == "arrange") {
            if (!std::isfinite(value) || value != std::floor(value) || value < 0 || value > 7)
                throw std::invalid_argument("Invalid alignment operation.");
            arrangeVectors(drawing, ids, static_cast<VectorLayout>(int(value)));
        } else if (operation == "order") {
            if (!std::isfinite(value) || value != std::floor(value) || value < 0 || value > 3)
                throw std::invalid_argument("Invalid stacking operation.");
            orderVectors(drawing, ids, static_cast<VectorOrder>(int(value)));
        } else if (operation == "smooth")
            smoothVectors(drawing, ids, value);
        else if (operation == "simplify")
            simplifyVectors(drawing, ids, value);
        else
            styleVectors(drawing, ids, operation.toStdString(), value);
    });
}
bool EditorController::pasteVectorBlock(const VectorBlock& block, std::vector<Id>& result) {
    if (!layer_)
        return false;
    std::vector<Id> incoming;
    if (!edit("Paste vectors in local coordinates",
              [&](Document& d) { incoming = pasteVectors(d, layer_, frame_, block); }))
        return false;
    result = std::move(incoming);
    return true;
}
