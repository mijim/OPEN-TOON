#include "canvas_item.h"
#include "opentoon/deformation.h"
#include "opentoon/deformer.h"
#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QScopedValueRollback>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>

using namespace opentoon;
namespace {
MeshPoint influenceHandle(const BoneChain& bone) {
    const auto& joints = bone.restJoints;
    const double dx = joints[1].x - joints[0].x;
    const double dy = joints[1].y - joints[0].y;
    const double length = std::hypot(dx, dy);
    return {joints[1].x - dy / length * bone.elbowTransition,
            joints[1].y + dx / length * bone.elbowTransition};
}
} // namespace

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
    const auto evaluated = (binding->bone || binding->curve) ? evaluateMeshBinding(*binding, editor_->frame())
                                                              : *binding;
    const auto point = meshRestEditing_ ? evaluated.vertices[index].rest : evaluated.vertices[index].pose;
    return selectionWorld().map(QPointF(point.x, point.y));
}

QPointF CanvasItem::meshControlPosition(int index) const {
    const auto* binding = editor_ ? selectedMesh(editor_->document()) : nullptr;
    if (!binding || index < 0)
        return {};
    MeshPoint point;
    if (binding->bone && index < 3)
        point = meshRestEditing_ ? binding->bone->restJoints[index]
                                 : sampleBoneJoints(*binding->bone, editor_->frame())[index];
    else if (binding->curve && index < 4)
        point = meshRestEditing_ ? binding->curve->restControls[index]
                                 : sampleCurveControls(*binding->curve, editor_->frame())[index];
    else
        return {};
    return selectionWorld().map(QPointF(point.x, point.y));
}

QPointF CanvasItem::meshInfluenceHandlePosition() const {
    const auto* binding = editor_ ? selectedMesh(editor_->document()) : nullptr;
    if (!binding || !binding->bone)
        return {};
    const auto point = influenceHandle(*binding->bone);
    return selectionWorld().map(QPointF(point.x, point.y));
}

int CanvasItem::meshControlAt(QPointF position) const {
    const auto* binding = editor_ ? selectedMesh(editor_->document()) : nullptr;
    if (!binding || (!binding->bone && !binding->curve))
        return -1;
    const int first = binding->bone && !meshRestEditing_ ? 1 : 0;
    const int end = binding->bone ? 3 : 4;
    for (int index = end - 1; index >= first; --index)
        if (QLineF(position, meshControlPosition(index)).length() <= 10)
            return index;
    if (binding->bone && meshRestEditing_ &&
        QLineF(position, meshInfluenceHandlePosition()).length() <= 10)
        return 3;
    return -1;
}

int CanvasItem::meshVertexAt(QPointF position) const {
    const auto* binding = editor_ ? selectedMesh(editor_->document()) : nullptr;
    if (!binding || binding->bone || binding->curve)
        return -1;
    const auto evaluated = (binding->bone || binding->curve) ? evaluateMeshBinding(*binding, editor_->frame())
                                                              : *binding;
    binding = &evaluated;
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
    const auto evaluated = (binding->bone || binding->curve)
        ? std::optional<MeshBinding>(evaluateMeshBinding(*binding, editor_->frame()))
        : std::nullopt;
    if (evaluated)
        binding = &*evaluated;
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
    if (!binding->bone && !binding->curve)
        for (int row = 0; row <= binding->rows; ++row)
            for (int column = 0; column <= binding->columns; ++column) {
                const int index = row * (binding->columns + 1) + column;
                const auto point = at(row, column);
                painter->setBrush(index == meshVertex_ ? QColor("#ffffff") : QColor("#151515"));
                painter->drawRect(QRectF(point - QPointF(4, 4), QSizeF(8, 8)));
            }
    if (binding->bone) {
        const auto joints = meshRestEditing_ ? binding->bone->restJoints
                                             : sampleBoneJoints(*binding->bone, editor_->frame());
        if (meshRestEditing_) {
            const auto& bone = *binding->bone;
            const QPointF elbow = transform.map(QPointF(joints[1].x, joints[1].y));
            const auto handle = influenceHandle(bone);
            const QPointF radiusHandle = transform.map(QPointF(handle.x, handle.y));
            QPainterPath influence;
            for (int step = 0; step <= 48; ++step) {
                const double angle = 2 * std::numbers::pi * step / 48;
                const QPointF point = transform.map(QPointF(
                    joints[1].x + std::cos(angle) * bone.elbowTransition,
                    joints[1].y + std::sin(angle) * bone.elbowTransition));
                if (step == 0) influence.moveTo(point); else influence.lineTo(point);
            }
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(QColor("#999999"), 1, Qt::DashLine));
            painter->drawPath(influence);
            painter->drawLine(elbow, radiusHandle);
            painter->setBrush(meshControl_ == 3 ? QColor("#ffffff") : QColor("#151515"));
            painter->setPen(QPen(QColor("#ffffff"), 1));
            painter->drawRect(QRectF(radiusHandle - QPointF(5, 5), QSizeF(10, 10)));
        }
        painter->setPen(QPen(QColor("#ffffff"), 2));
        for (int index = 0; index < 2; ++index)
            painter->drawLine(transform.map(QPointF(joints[index].x, joints[index].y)),
                              transform.map(QPointF(joints[index + 1].x, joints[index + 1].y)));
        for (int index = 0; index < 3; ++index) {
            const auto point = transform.map(QPointF(joints[index].x, joints[index].y));
            painter->setBrush(index == meshControl_ ? QColor("#ffffff") : QColor("#151515"));
            painter->drawEllipse(point, 5, 5);
        }
    } else if (binding->curve) {
        const auto controls = meshRestEditing_ ? binding->curve->restControls
                                               : sampleCurveControls(*binding->curve, editor_->frame());
        auto point = [&](int index) {
            return transform.map(QPointF(controls[index].x, controls[index].y));
        };
        painter->setPen(QPen(QColor("#aaaaaa"), 1, Qt::DashLine));
        painter->drawLine(point(0), point(1));
        painter->drawLine(point(2), point(3));
        QPainterPath path(point(0));
        path.cubicTo(point(1), point(2), point(3));
        painter->setPen(QPen(QColor("#ffffff"), 2));
        painter->drawPath(path);
        for (int index = 0; index < 4; ++index) {
            painter->setBrush(index == meshControl_ ? QColor("#ffffff") : QColor("#151515"));
            painter->drawEllipse(point(index), 5, 5);
        }
    }
    painter->restore();
}

void CanvasItem::beginMesh(QPointF position) {
    meshControl_ = meshControlAt(position);
    if (meshControl_ >= 0) {
        const auto* binding = selectedMesh(editor_->document());
        meshPreviewAngles_ = binding->bone ? sampleBoneAngles(*binding->bone, editor_->frame())
                                           : std::array<double, 2>{};
        if (binding->curve)
            meshPreviewPoint_ = meshRestEditing_
                ? binding->curve->restControls[meshControl_]
                : sampleCurveControls(*binding->curve, editor_->frame())[meshControl_];
        else if (binding->bone && meshControl_ == 3)
            meshPreviewRadius_ = binding->bone->elbowTransition;
        else
            meshPreviewPoint_ = meshRestEditing_ && binding->bone
                ? binding->bone->restJoints[meshControl_] : MeshPoint{};
        meshControlMoved_ = false;
        drawing_ = true;
        previewValid_ = true;
        update();
        return;
    }
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

void CanvasItem::previewMeshControl(QPointF position) {
    if (!editor_ || meshControl_ < 0)
        return;
    const auto point = localPoint(position, 1);
    const auto* source = selectedMesh(editor_->document());
    if (!source)
        return;
    auto candidate = editor_->document();
    try {
        const Id drawing = editor_->selectedSubstitution();
        if (source->bone) {
            if (meshRestEditing_ && meshControl_ == 3) {
                const auto& joints = source->bone->restJoints;
                const double dx = joints[1].x - joints[0].x;
                const double dy = joints[1].y - joints[0].y;
                const double length = std::hypot(dx, dy);
                const double radius = std::max(0.01,
                    ((point.x - joints[1].x) * -dy +
                     (point.y - joints[1].y) * dx) / length);
                setBoneElbowTransition(candidate, editor_->selectedLayer(), drawing, radius);
                meshPreviewRadius_ = radius;
            } else if (meshRestEditing_) {
                moveBoneRestJoint(candidate, editor_->selectedLayer(), drawing,
                                  meshControl_, {point.x, point.y});
                meshPreviewPoint_ = {point.x, point.y};
            } else {
                const auto& rest = source->bone->restJoints;
                const auto joints = sampleBoneJoints(*source->bone, editor_->frame());
                const auto origin = meshControl_ == 1 ? rest[0] : joints[1];
                const auto start = meshControl_ == 1 ? rest[1] : rest[2];
                const auto prior = meshControl_ == 1 ? rest[0] : rest[1];
                const double direction = std::atan2(point.y - origin.y, point.x - origin.x);
                const double restDirection = std::atan2(start.y - prior.y, start.x - prior.x);
                auto angles = sampleBoneAngles(*source->bone, editor_->frame());
                if (meshControl_ == 1)
                    angles[0] = (direction - restDirection) * 180 / std::numbers::pi;
                else
                    angles[1] = (direction - restDirection) * 180 / std::numbers::pi - angles[0];
                recordBonePose(candidate, editor_->selectedLayer(), drawing, editor_->frame(),
                               angles[0], angles[1]);
                meshPreviewAngles_ = angles;
            }
        } else if (source->curve) {
            if (meshRestEditing_)
                moveCurveRestControl(candidate, editor_->selectedLayer(), drawing,
                                     meshControl_, {point.x, point.y});
            else {
                auto controls = sampleCurveControls(*source->curve, editor_->frame());
                controls[meshControl_] = {point.x, point.y};
                recordCurvePose(candidate, editor_->selectedLayer(), drawing,
                                editor_->frame(), controls);
            }
            meshPreviewPoint_ = {point.x, point.y};
        }
        posePreview_ = std::move(candidate);
        previewValid_ = true;
        meshControlMoved_ = true;
    } catch (const std::exception& error) {
        posePreview_.reset();
        previewValid_ = false;
        editor_->report(error.what());
    }
    update();
}

void CanvasItem::commitMeshControl() {
    const int control = meshControl_;
    const auto* binding = editor_ ? selectedMesh(editor_->document()) : nullptr;
    const bool bone = binding && binding->bone.has_value();
    meshControl_ = -1;
    posePreview_.reset();
    drawing_ = false;
    if (editor_ && control >= 0 && meshControlMoved_ && previewValid_) {
        QScopedValueRollback<bool> guard(committing_, true);
        if (bone && meshRestEditing_ && control == 3)
            editor_->setSelectedBoneTransition(meshPreviewRadius_);
        else if (bone && meshRestEditing_)
            editor_->moveSelectedBoneRestJoint(control, meshPreviewPoint_.x, meshPreviewPoint_.y);
        else if (bone)
            editor_->recordSelectedBonePose(meshPreviewAngles_[0], meshPreviewAngles_[1]);
        else if (meshRestEditing_)
            editor_->moveSelectedCurveRestControl(control, meshPreviewPoint_.x,
                                                   meshPreviewPoint_.y);
        else
            editor_->moveSelectedCurveControl(control, meshPreviewPoint_.x, meshPreviewPoint_.y);
    }
    meshControlMoved_ = false;
    previewValid_ = false;
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
