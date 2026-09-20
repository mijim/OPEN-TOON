#include "canvas_item.h"
#include "opentoon/animation.h"
#include <cmath>
using namespace opentoon;
namespace {
QTransform localMatrix(const Transform& t) {
    QTransform m;
    m.translate(t.x + t.pivotX, t.y + t.pivotY);
    m.rotate(t.rotation);
    m.scale(t.scaleX, t.scaleY);
    m.translate(-t.pivotX, -t.pivotY);
    return m;
}
} // namespace
void CanvasItem::selectAnimationBounds() {
    if (!editor_ || editor_->tool() != "Animate" || !editor_->selectedLayer())
        return;
    region_ = {};
    const auto* drawing = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame());
    if (!drawing)
        return;
    QRectF bounds;
    auto include = [&](QRectF r) { bounds = bounds.isNull() ? r : bounds.united(r); };
    for (const auto& stroke : drawing->strokes) {
        if (stroke.points.empty())
            continue;
        double left = stroke.points.front().x, right = left, top = stroke.points.front().y, bottom = top;
        for (auto p : stroke.points) {
            left = std::min(left, p.x);
            right = std::max(right, p.x);
            top = std::min(top, p.y);
            bottom = std::max(bottom, p.y);
        }
        double pad = std::max(.5, stroke.width / 2);
        include(QRectF(left - pad, top - pad, right - left + 2 * pad, bottom - top + 2 * pad));
    }
    if (drawing->image)
        include(QRectF(0, 0, drawing->image->width, drawing->image->height));
    if (drawing->raster)
        for (const auto& [tile, pixels] : drawing->raster->tiles) {
            (void)pixels;
            include(QRectF(tile.first * 64, tile.second * 64, 64, 64));
        }
    if (!bounds.isEmpty()) {
        const auto r = bounds.toAlignedRect();
        region_ = {r.x(), r.y(), r.width(), r.height()};
    }
    emit regionChanged();
    update();
}
void CanvasItem::previewPose(QTransform delta) {
    previewPose_ = sourcePose_;
    const auto original = localMatrix(sourcePose_);
    const QRectF box(region_.x, region_.y, region_.width, region_.height);
    if (transformHandle_ == -1) {
        const auto displacement = original.map(delta.map(QPointF{})) - original.map(QPointF{});
        previewPose_.x += displacement.x();
        previewPose_.y += displacement.y();
    } else {
        QPointF anchor = box.center();
        if (transformHandle_ == 8) {
            // Rotate in parent coordinates, preserving the visual center even with nonuniform scale.
            const auto first = original.map(QPointF(samples_.front().x, samples_.front().y));
            const auto current = original.map(delta.map(QPointF(samples_.front().x, samples_.front().y)));
            const auto center = original.map(anchor);
            double angle = (std::atan2(current.y() - center.y(), current.x() - center.x()) -
                            std::atan2(first.y() - center.y(), first.x() - center.x())) *
                           180 / std::acos(-1);
            if (shift_)
                angle = std::round(angle / 15) * 15;
            previewPose_.rotation += angle;
        } else {
            const int h = transformHandle_;
            bool left = h == 0 || h == 6 || h == 7, right = h == 2 || h == 3 || h == 4;
            bool top = h == 0 || h == 1 || h == 2, bottom = h == 4 || h == 5 || h == 6;
            anchor = {left    ? box.right()
                      : right ? box.left()
                              : box.center().x(),
                      top      ? box.bottom()
                      : bottom ? box.top()
                               : box.center().y()};
            previewPose_.scaleX *= delta.m11();
            previewPose_.scaleY *= delta.m22();
        }
        auto offset = original.map(anchor) - localMatrix(previewPose_).map(anchor);
        previewPose_.x += offset.x();
        previewPose_.y += offset.y();
    }
    try {
        posePreview_ = editor_->document();
        recordPose(posePreview_->layer(editor_->selectedLayer()), editor_->frame(), previewPose_);
        posePreview_->validate();
        previewValid_ = true;
    } catch (const std::exception& e) {
        posePreview_.reset();
        previewValid_ = false;
        editor_->report(e.what());
    }
    update();
}
