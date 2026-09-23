#include "canvas_item.h"
#include "opentoon/animation.h"
#include "scene_renderer.h"
#include <QPainter>
#include <QScopedValueRollback>
#include <cmath>
#include <set>
using namespace opentoon;
void CanvasItem::resetMotionReference() {
    if (motionReferenceValid_ && motionReferenceLayer_ == opentoon::Id(editor_->selectedLayer()) &&
        motionReferenceScene_ == editor_->sceneGeneration())
        return;
    motionReferenceLayer_ = editor_->selectedLayer();
    motionReferenceScene_ = editor_->sceneGeneration();
    motionReferenceValid_ = hasRegion();
    if (motionReferenceValid_)
        motionReference_ = {region_.x + region_.width / 2.0, region_.y + region_.height / 2.0};
    else if (motionPathEditing_)
        setMotionPathEditing(false);
}
void CanvasItem::setMotionPathEditing(bool enabled) {
    if (motionPathEditing_ == enabled)
        return;
    if (enabled && (!editor_ || editor_->tool() != "Animate" || !hasRegion())) {
        if (editor_)
            editor_->report("Expose a drawing and choose Animate before editing its motion path.");
        return;
    }
    cancelGesture();
    motionPathEditing_ = enabled;
    if (enabled)
        resetMotionReference();
    else
        motionReferenceValid_ = false;
    updateCursor(hoverPosition_);
    emit viewChanged();
    update();
}
QPointF CanvasItem::motionPathPosition(int frame) const {
    if (!editor_ || !editor_->selectedLayer() || !motionReferenceValid_)
        return {};
    const auto& doc = posePreview_ ? *posePreview_ : editor_->document();
    return (SceneRenderer::worldTransform(doc, doc.layer(editor_->selectedLayer()), frame) * contentTransform())
        .map(motionReference_);
}
std::vector<CanvasItem::MotionSample> CanvasItem::motionSamples(const Document& doc,
                                                                QPointF reference) const {
    const auto& layer = doc.layer(editor_->selectedLayer());
    if (layer.keys.empty())
        return {};
    const auto start = layer.keys.front().frame, end = layer.keys.back().frame;
    std::set<Frame> frames;
    for (Frame frame = start; frame < end; frame += std::max(1, (end - start) / 240))
        frames.insert(frame);
    for (const auto& key : layer.keys) {
        frames.insert(key.frame);
        if (key.frame > start)
            frames.insert(key.frame - 1);
    }
    const auto view = contentTransform();
    std::vector<MotionSample> samples;
    samples.reserve(frames.size());
    for (auto frame : frames)
        samples.push_back({frame, (SceneRenderer::worldTransform(doc, layer, frame) * view).map(reference)});
    return samples;
}
int CanvasItem::motionPathKeyAt(QPointF point) const {
    if (!motionPathEditing_ || !motionReferenceValid_ || !editor_ || !editor_->selectedLayer())
        return -1;
    int nearest = -1;
    double distance = 11;
    for (const auto& key : editor_->document().layer(editor_->selectedLayer()).keys) {
        const auto d = QLineF(point, motionPathPosition(key.frame)).length();
        if (d < distance - 1e-6 || (d <= distance && key.frame == editor_->frame())) {
            distance = d;
            nearest = key.frame;
        }
    }
    return nearest;
}
int CanvasItem::motionPathFrameAt(QPointF point) const {
    if (!motionPathEditing_ || !motionReferenceValid_ || !editor_ || !editor_->selectedLayer())
        return -1;
    if (auto key = motionPathKeyAt(point); key >= 0)
        return key;
    const auto samples = motionSamples(editor_->document(), motionReference_);
    int nearest = -1;
    double distance = 10;
    for (std::size_t i = 1; i < samples.size(); ++i) {
        const auto& a = samples[i - 1];
        const auto& b = samples[i];
        const auto delta = b.position - a.position;
        const double length = QPointF::dotProduct(delta, delta);
        if (length < 1e-12)
            continue;
        const double t = std::clamp(QPointF::dotProduct(point - a.position, delta) / length, 0.0, 1.0);
        if (QLineF(point, a.position + delta * t).length() > distance)
            continue;
        const auto frame = Frame(std::lround(a.frame + t * (b.frame - a.frame)));
        // A held jump is drawn as a guide, but must not invent intermediate poses.
        const double actualDistance = QLineF(point, motionPathPosition(frame)).length();
        if (actualDistance < distance) {
            distance = actualDistance;
            nearest = frame;
        }
    }
    return nearest;
}
void CanvasItem::beginMotionPath(QPointF point) {
    const int frame = motionPathKeyAt(point);
    if (frame < 0) {
        const int sample = motionPathFrameAt(point);
        if (sample >= 0)
            editor_->setFrame(sample);
        return;
    }
    editor_->setFrame(frame); // Frame changes cancel old gestures before we start this one.
    editor_->clearPoseSelection();
    editor_->selectPoseKey(frame);
    const auto& doc = editor_->document();
    const auto& layer = doc.layer(editor_->selectedLayer());
    QTransform parent;
    if (layer.parent)
        parent = SceneRenderer::worldTransform(doc, doc.layer(layer.parent), frame);
    bool invertible = false;
    motionParentInverse_ = (parent * contentTransform()).inverted(&invertible);
    if (!invertible) {
        editor_->report("Cannot move a path key through a zero-scale parent transform.");
        return;
    }
    sourcePose_ = evaluateTransform(layer, frame);
    previewPose_ = sourcePose_;
    motionPress_ = point;
    motionKey_ = frame;
    drawing_ = true;
    previewValid_ = true;
    update();
}
void CanvasItem::previewMotionPath(QPointF point) {
    QPointF delta = point - motionPress_;
    if (shift_) {
        if (std::abs(delta.x()) >= std::abs(delta.y()))
            delta.setY(0);
        else
            delta.setX(0);
    }
    const auto localDelta =
        motionParentInverse_.map(motionPress_ + delta) - motionParentInverse_.map(motionPress_);
    previewPose_ = sourcePose_;
    previewPose_.x += localDelta.x();
    previewPose_.y += localDelta.y();
    try {
        posePreview_ = editor_->document();
        setKeyPosition(posePreview_->layer(editor_->selectedLayer()), motionKey_, previewPose_.x,
                       previewPose_.y);
        posePreview_->validate();
        previewValid_ = true;
    } catch (const std::exception& e) {
        posePreview_.reset();
        previewValid_ = false;
        editor_->report(QString::fromUtf8(e.what()));
    }
    update();
}
void CanvasItem::commitMotionPath() {
    const bool commit = posePreview_ && previewValid_ && previewPose_ != sourcePose_;
    const auto pose = previewPose_;
    const auto frame = motionKey_;
    cancelGesture();
    if (commit) {
        QScopedValueRollback<bool> guard(committing_, true);
        editor_->setPoseKeyPosition(frame, pose.x, pose.y);
    }
    selectAnimationBounds();
    update();
}
void CanvasItem::paintMotionPath(QPainter* painter, const Document& doc, const QTransform& itemTransform) {
    const auto& layer = doc.layer(editor_->selectedLayer());
    if (layer.keys.empty())
        return;
    const bool editing = motionPathEditing_ && motionReferenceValid_;
    const QPointF reference =
        editing ? motionReference_
                : QPointF(region_.x + region_.width / 2.0, region_.y + region_.height / 2.0);
    const auto samples = motionSamples(doc, reference);
    const auto view = contentTransform();
    painter->save();
    painter->setWorldTransform(itemTransform);
    QPolygonF path;
    for (const auto& sample : samples)
        path << sample.position;
    painter->setPen(QPen(QColor("#999999"), editing ? 1.5 : 1, Qt::DashLine));
    painter->drawPolyline(path);
    const int hovered = editing ? motionPathKeyAt(hoverPosition_) : -1;
    for (const auto& key : layer.keys) {
        const auto point = (SceneRenderer::worldTransform(doc, layer, key.frame) * view).map(reference);
        const bool active = key.frame == editor_->frame();
        painter->setBrush(editing && active ? Qt::white : QColor("#111111"));
        painter->setPen(QPen(editing && active ? QColor("#111111") : QColor("#999999"), 1));
        const double radius = editing ? (active || hovered == key.frame ? 6 : 5) : 3;
        painter->drawEllipse(point, radius, radius);
        if (editing && active) {
            const QString text = QString("Frame %1").arg(key.frame + 1);
            auto box = painter->fontMetrics().boundingRect(text).adjusted(-4, -2, 4, 2);
            box.moveTopLeft((point + QPointF(10, -24)).toPoint());
            painter->fillRect(box, QColor("#151515"));
            painter->setPen(Qt::white);
            painter->drawText(box, Qt::AlignCenter, text);
        }
    }
    painter->restore();
}
