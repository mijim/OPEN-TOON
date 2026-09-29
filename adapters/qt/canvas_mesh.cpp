#include "canvas_item.h"
#include "opentoon/deformation.h"
#include <QLineF>
#include <QPainter>
#include <QScopedValueRollback>
#include <algorithm>

using namespace opentoon;

const MeshBinding* CanvasItem::selectedMesh(const Document& document) const {
    if (!editor_ || !editor_->selectedLayer())
        return nullptr;
    const auto& layer = document.layer(editor_->selectedLayer());
    if (layer.kind != LayerKind::Part)
        return nullptr;
    const auto* drawing = document.drawingAt(layer.id, editor_->frame());
    return drawing ? meshBindingFor(layer, drawing->id) : nullptr;
}

QPointF CanvasItem::meshVertexPosition(int index) const {
    const auto* binding = editor_ ? selectedMesh(editor_->document()) : nullptr;
    if (!binding || index < 0 || std::size_t(index) >= binding->vertices.size())
        return {};
    const auto point = meshRestEditing_ ? binding->vertices[index].rest : binding->vertices[index].pose;
    return selectionWorld().map(QPointF(point.x, point.y));
}

int CanvasItem::meshVertexAt(QPointF position) const {
    const auto* binding = editor_ ? selectedMesh(editor_->document()) : nullptr;
    if (!binding)
        return -1;
    const auto transform = selectionWorld();
    int nearest = -1;
    double distance = 9;
    for (std::size_t i = 0; i < binding->vertices.size(); ++i) {
        const auto point = meshRestEditing_ ? binding->vertices[i].rest : binding->vertices[i].pose;
        const double candidate = QLineF(position, transform.map(QPointF(point.x, point.y))).length();
        if (candidate < distance) {
            distance = candidate;
            nearest = int(i);
        }
    }
    return nearest;
}

void CanvasItem::paintMesh(QPainter* painter, const Document& document,
                           const QTransform& itemTransform) {
    if (!editor_ || editor_->tool() != "Mesh")
        return;
    const auto* binding = selectedMesh(document);
    if (!binding)
        return;
    const auto transform = selectionWorld();
    auto at = [&](int row, int column) {
        const auto& vertex = binding->vertices[std::size_t(row) * (binding->columns + 1) + column];
        const auto point = meshRestEditing_ ? vertex.rest : vertex.pose;
        return transform.map(QPointF(point.x, point.y));
    };
    painter->save();
    painter->setWorldTransform(itemTransform);
    painter->setPen(QPen(QColor("#eeeeee"), 1));
    painter->setBrush(Qt::NoBrush);
    for (int row = 0; row <= binding->rows; ++row)
        for (int column = 0; column < binding->columns; ++column)
            painter->drawLine(at(row, column), at(row, column + 1));
    for (int column = 0; column <= binding->columns; ++column)
        for (int row = 0; row < binding->rows; ++row)
            painter->drawLine(at(row, column), at(row + 1, column));
    for (int row = 0; row <= binding->rows; ++row)
        for (int column = 0; column <= binding->columns; ++column) {
            const int index = row * (binding->columns + 1) + column;
            const auto point = at(row, column);
            painter->setBrush(index == meshVertex_ ? QColor("#ffffff") : QColor("#151515"));
            painter->drawRect(QRectF(point - QPointF(4, 4), QSizeF(8, 8)));
        }
    painter->restore();
}

void CanvasItem::beginMesh(QPointF position) {
    meshVertex_ = meshVertexAt(position);
    if (meshVertex_ < 0)
        return;
    const auto& document = editor_->document();
    const auto* binding = selectedMesh(document);
    if (!binding)
        return;
    if (meshRestEditing_ && std::any_of(binding->vertices.begin(), binding->vertices.end(),
                                       [](const MeshVertex& vertex) {
                                           return vertex.pose != vertex.rest;
                                       })) {
        editor_->report("Reset the mesh pose before editing its rest shape.");
        meshVertex_ = -1;
        return;
    }
    meshPreviewPoint_ = meshRestEditing_ ? binding->vertices[meshVertex_].rest
                                         : binding->vertices[meshVertex_].pose;
    drawing_ = true;
    previewValid_ = true;
    posePreview_ = document;
    update();
}

void CanvasItem::previewMesh(QPointF position) {
    if (meshVertex_ < 0 || !editor_)
        return;
    const auto point = localPoint(position, 1);
    auto candidate = editor_->document();
    try {
        const Id drawing = editor_->selectedSubstitution();
        if (meshRestEditing_)
            moveMeshRestVertex(candidate, editor_->selectedLayer(), drawing, meshVertex_,
                               {point.x, point.y});
        else
            moveMeshPoseVertex(candidate, editor_->selectedLayer(), drawing, meshVertex_,
                               {point.x, point.y});
        meshPreviewPoint_ = {point.x, point.y};
        posePreview_ = std::move(candidate);
        previewValid_ = true;
    } catch (const std::exception&) {
        posePreview_.reset();
        previewValid_ = false;
    }
    update();
}

void CanvasItem::commitMesh() {
    const int vertex = meshVertex_;
    meshVertex_ = -1;
    posePreview_.reset();
    drawing_ = false;
    if (editor_ && vertex >= 0 && previewValid_) {
        QScopedValueRollback<bool> guard(committing_, true);
        editor_->moveSelectedMeshVertex(vertex, meshPreviewPoint_.x,
                                        meshPreviewPoint_.y, meshRestEditing_);
    }
    previewValid_ = false;
    update();
}
