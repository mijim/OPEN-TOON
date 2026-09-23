#include "canvas_item.h"
#include "opentoon/animation.h"
#include "scene_renderer.h"
#include <QPainter>
#include <QScopedValueRollback>
#include <cmath>

using namespace opentoon;

namespace {
QPointF centerOf(const QPolygonF& frame) {
    return (frame[0] + frame[2]) / 2;
}
QPointF rotationHandle(const QPolygonF& frame) {
    const auto top = (frame[0] + frame[1]) / 2;
    const auto outward = top - centerOf(frame);
    const double length = std::hypot(outward.x(), outward.y());
    return top + (length > 1e-6 ? outward / length * 28 : QPointF(0, -28));
}
} // namespace

QPolygonF CanvasItem::cameraFrame(const Document& document) const {
    if (!editor_ || !document.activeCamera)
        return {};
    bool invertible = false;
    const auto inverse = SceneRenderer::cameraTransform(document, editor_->frame()).inverted(&invertible);
    if (!invertible)
        return {};
    const QPolygonF output{QPointF(0, 0), QPointF(document.width, 0),
                           QPointF(document.width, document.height), QPointF(0, document.height)};
    return viewTransform().map(inverse.map(output));
}
QPointF CanvasItem::cameraHandlePosition(int index) const {
    if (!editor_ || !editor_->document().activeCamera)
        return {};
    const auto frame = cameraFrame(posePreview_ ? *posePreview_ : editor_->document());
    if (frame.size() != 4)
        return {};
    if (index >= 0 && index < 4)
        return frame[index];
    return index == 4 ? rotationHandle(frame) : centerOf(frame);
}
int CanvasItem::cameraHandleAt(QPointF point) const {
    if (!editor_ || editor_->tool() != "Camera" || !editor_->document().activeCamera)
        return -1;
    const auto frame = cameraFrame(posePreview_ ? *posePreview_ : editor_->document());
    if (frame.size() != 4)
        return -1;
    for (int index = 0; index < 4; ++index)
        if (QLineF(point, frame[index]).length() <= 12)
            return index;
    if (QLineF(point, rotationHandle(frame)).length() <= 12)
        return 4;
    return frame.containsPoint(point, Qt::OddEvenFill) ? 5 : -1;
}
void CanvasItem::paintCameraGuide(QPainter* painter, const QTransform& itemTransform) {
    if (!editor_ || !editor_->document().activeCamera ||
        (editor_->tool() != "Camera" && !cameraGuidesVisible_))
        return;
    painter->save();
    painter->setWorldTransform(itemTransform);
    painter->setRenderHint(QPainter::Antialiasing);
    QPolygonF frame;
    if (editor_->tool() == "Camera")
        frame = cameraFrame(posePreview_ ? *posePreview_ : editor_->document());
    else {
        const auto& document = editor_->document();
        frame = viewTransform().map(QPolygonF{QPointF(0, 0), QPointF(document.width, 0),
                                              QPointF(document.width, document.height),
                                              QPointF(0, document.height)});
    }
    if (frame.size() == 4) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(QColor("#111111"), 3));
        painter->drawPolygon(frame);
        painter->setPen(QPen(QColor("#f0f0f0"), 1));
        painter->drawPolygon(frame);
        QPolygonF safe;
        const auto center = centerOf(frame);
        for (const auto& corner : frame)
            safe << center + (corner - center) * .9;
        painter->setPen(QPen(QColor("#b0b0b0"), 1, Qt::DashLine));
        painter->drawPolygon(safe);
        if (editor_->tool() == "Camera") {
            painter->setPen(QPen(QColor("#e8e8e8"), 1));
            painter->drawLine(center + QPointF(-8, 0), center + QPointF(8, 0));
            painter->drawLine(center + QPointF(0, -8), center + QPointF(0, 8));
            const auto top = (frame[0] + frame[1]) / 2;
            const auto rotate = rotationHandle(frame);
            painter->drawLine(top, rotate);
            painter->setBrush(QColor("#111111"));
            for (const auto& corner : frame)
                painter->drawRect(QRectF(corner - QPointF(4, 4), QSizeF(8, 8)));
            painter->drawEllipse(rotate, 5, 5);
        }
    }
    painter->restore();
}
void CanvasItem::beginCamera(QPointF point) {
    const auto hit = cameraHandleAt(point);
    if (hit < 0)
        return;
    const auto& document = editor_->document();
    if (document.layer(document.activeCamera).locked) {
        editor_->report("Unlock the camera before editing.");
        return;
    }
    cameraHandle_ = hit;
    cameraPress_ = point;
    cameraSourcePose_ = evaluateTransform(document.layer(document.activeCamera), editor_->frame());
    previewPose_ = cameraSourcePose_;
    drawing_ = true;
    previewValid_ = true;
    updateCursor(point);
}
void CanvasItem::previewCamera(QPointF point) {
    bool invertible = false;
    const auto inverse = viewTransform().inverted(&invertible);
    if (!invertible)
        return;
    const auto start = inverse.map(cameraPress_);
    const auto current = inverse.map(point);
    auto pose = cameraSourcePose_;
    const QPointF center(pose.x, pose.y);
    if (cameraHandle_ == 5) {
        auto displacement = current - start;
        if (shift_) {
            if (std::abs(displacement.x()) >= std::abs(displacement.y()))
                displacement.setY(0);
            else
                displacement.setX(0);
        }
        pose.x += displacement.x();
        pose.y += displacement.y();
    } else if (cameraHandle_ == 4) {
        const auto a = start - center, b = current - center;
        double degrees = (std::atan2(b.y(), b.x()) - std::atan2(a.y(), a.x())) *
                         180 / std::acos(-1);
        if (shift_)
            degrees = std::round(degrees / 15) * 15;
        pose.rotation += degrees;
    } else {
        const double first = QLineF(start, center).length();
        const double now = QLineF(current, center).length();
        if (now < 1)
            return;
        const double ratio = first / now;
        pose.scaleX = std::clamp(cameraSourcePose_.scaleX * ratio, .05, 20.0);
        pose.scaleY = std::clamp(cameraSourcePose_.scaleY * ratio, .05, 20.0);
    }
    try {
        posePreview_ = editor_->document();
        recordPose(posePreview_->layer(posePreview_->activeCamera), editor_->frame(), pose);
        posePreview_->validate();
        previewPose_ = pose;
        previewValid_ = true;
    } catch (const std::exception& error) {
        posePreview_.reset();
        previewValid_ = false;
        editor_->report(error.what());
    }
    update();
}
void CanvasItem::commitCamera() {
    const bool commit = previewValid_ && posePreview_ && previewPose_ != cameraSourcePose_;
    const auto pose = previewPose_;
    cancelGesture();
    if (commit) {
        QScopedValueRollback<bool> guard(committing_, true);
        editor_->commitCameraPose(pose);
    }
}
