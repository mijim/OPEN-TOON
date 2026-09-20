#include "canvas_item.h"
#include <QPainter>
#include <QScopedValueRollback>
#include <set>
using namespace opentoon;
SelectionMedia CanvasItem::activeSelectionMedia() const {
    return vectorSelection_ ? SelectionMedia::Vectors : selectionMedia_;
}
void CanvasItem::selectStroke(Id id) {
    setVectorSelection(id ? std::vector<Id>{id} : std::vector<Id>{});
}
void CanvasItem::setVectorSelection(std::vector<Id> ids) {
    if (!editor_)
        return;
    const auto* drawing = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame());
    const std::set<Id> requested(ids.begin(), ids.end());
    ids.clear();
    if (drawing)
        for (const auto& stroke : drawing->strokes)
            if (requested.contains(stroke.id))
                ids.push_back(stroke.id);
    try {
        const auto bounds = drawing ? strokeSelectionBounds(*drawing, ids) : std::nullopt;
        region_ = bounds.value_or(PixelRect{});
        regionStrokes_ = std::move(ids);
        vectorSelection_ = true;
        selectedStroke_ = regionStrokes_.size() == 1 ? regionStrokes_.front() : 0;
        emit regionChanged();
        update();
    } catch (const std::exception& error) {
        editor_->report(error.what());
    }
}
void CanvasItem::modifyVectorSelection(const std::vector<Id>& hits, int operation) {
    std::set<Id> selected;
    if (operation)
        selected.insert(regionStrokes_.begin(), regionStrokes_.end());
    for (auto id : hits) {
        if (operation == 2)
            selected.erase(id);
        else
            selected.insert(id);
    }
    setVectorSelection({selected.begin(), selected.end()});
    editor_->report(regionInfo());
}
void CanvasItem::paintVectorSelection(QPainter* painter, const QTransform& itemTransform) {
    if (!editor_ || !vectorSelection_ || regionStrokes_.size() < 2 ||
        (editor_->tool() != "Select" && editor_->tool() != "Marquee" && editor_->tool() != "Lasso"))
        return;
    const auto* drawing = transforming_
                              ? &transformPreview_
                              : editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame());
    if (!drawing)
        return;
    painter->save();
    painter->setWorldTransform(itemTransform);
    painter->setPen(QPen(QColor("#aaaaaa"), 1, Qt::DotLine));
    painter->setBrush(Qt::NoBrush);
    const auto world = selectionWorld();
    // Per-object bounds expose sparse membership, including holes in the group box.
    const std::set<Id> selected(regionStrokes_.begin(), regionStrokes_.end());
    for (const auto& stroke : drawing->strokes) {
        if (!selected.contains(stroke.id))
            continue;
        if (const auto bounds = strokeBounds(stroke)) {
            const QRectF r(bounds->x, bounds->y, bounds->width, bounds->height);
            painter->drawPolygon(world.map(QPolygonF(r)));
        }
    }
    painter->restore();
}

void CanvasItem::selectAllVectors(bool invert) {
    if (!editor_ || !editor_->selectedLayer())
        return;
    cancelGesture();
    const auto* drawing = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame());
    const std::set<Id> current(regionStrokes_.begin(), regionStrokes_.end());
    std::vector<Id> ids;
    if (drawing)
        for (const auto& s : drawing->strokes)
            if (!invert || !current.contains(s.id))
                ids.push_back(s.id);
    setVectorSelection(std::move(ids));
}
bool CanvasItem::copyVectorSelection(bool cut) {
    if (!editor_ || !vectorSelection_ || regionStrokes_.empty())
        return false;
    try {
        auto block = opentoon::copyVectors(editor_->document(), editor_->selectedLayer(), editor_->frame(),
                                           regionStrokes_);
        if (cut) {
            auto ids = regionStrokes_;
            QScopedValueRollback<bool> guard(committing_, true);
            if (!editor_->editDrawingRegion(region_, SelectionMedia::Vectors, SelectionAction::Delete, 0, 0,
                                            &ids))
                return false;
            clearRegion();
        }
        vectorClipboard_ = std::move(block);
        emit regionChanged();
        editor_->report(QString("%1 %2 vectors in local drawing coordinates")
                            .arg(cut ? "Cut" : "Copied")
                            .arg(vectorClipboard_.strokes.size()));
        return true;
    } catch (const std::exception& e) {
        editor_->report(e.what());
        return false;
    }
}
bool CanvasItem::pasteVectorSelection() {
    if (!editor_ || !hasVectorClipboard())
        return false;
    setMotionPathEditing(false);
    cancelGesture();
    std::vector<Id> ids;
    QScopedValueRollback<bool> guard(committing_, true);
    if (!editor_->pasteVectorBlock(vectorClipboard_, ids))
        return false;
    editor_->setTool("Select");
    previousTool_ = "Select";
    setVectorSelection(std::move(ids));
    forceActiveFocus();
    return true;
}
bool CanvasItem::editVectorSelection(QString operation, double value) {
    if (!editor_ || !vectorSelection_ || regionStrokes_.empty())
        return false;
    cancelGesture();
    const auto ids = regionStrokes_;
    QScopedValueRollback<bool> guard(committing_, true);
    const bool success = editor_->editVectors(ids, operation, value);
    if (success)
        setVectorSelection(ids);
    return success;
}
bool CanvasItem::nudgeVectorSelection(int dx, int dy) {
    if (!editor_ || !vectorSelection_ || regionStrokes_.empty())
        return false;
    cancelGesture();
    const auto ids = regionStrokes_;
    QScopedValueRollback<bool> guard(committing_, true);
    if (!editor_->editDrawingRegion(region_, SelectionMedia::Vectors, SelectionAction::Move, dx, dy, &ids))
        return false;
    setVectorSelection(ids);
    return true;
}
