#include "canvas_item.h"
#include "editor_controller.h"
#include "opentoon/animation.h"
#include "scene_renderer.h"
#include "vector_hit.h"
#include <QCursor>
#include <QMouseEvent>
#include <QPainter>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QScopedValueRollback>
#include <QTabletEvent>
#include <cmath>
#include <stdexcept>
using namespace opentoon;
CanvasItem::CanvasItem(QQuickItem* parent) : QQuickPaintedItem(parent) {
    setAcceptedMouseButtons(Qt::LeftButton | Qt::MiddleButton);
    setAcceptHoverEvents(true);
    setAntialiasing(true);
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);
    auto cancelPathOnResize = [this] {
        if (motionKey_ >= 0)
            cancelGesture();
    };
    connect(this, &QQuickItem::widthChanged, this, cancelPathOnResize);
    connect(this, &QQuickItem::heightChanged, this, cancelPathOnResize);
    connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow* window) {
        if (filteredWindow_)
            filteredWindow_->removeEventFilter(this);
        filteredWindow_ = window;
        if (window)
            window->installEventFilter(this);
    });
}
void CanvasItem::setEditor(EditorController* editor) {
    if (editor_)
        disconnect(editor_, nullptr, this, nullptr);
    editor_ = editor;
    previousTool_ = editor ? editor->tool() : QString{};
    if (editor) {
        connect(editor, &EditorController::changed, this, [this] {
            if (committing_)
                return;
            cancelGesture();
            clearRegion();
            selectedStroke_ = 0;
            selectAnimationBounds();
            updateCursor(hoverPosition_);
            update();
        });
        connect(editor, &EditorController::frameChanged, this, [this] {
            if (committing_)
                return;
            cancelGesture();
            selectedStroke_ = 0;
            clearRegion();
            selectedStroke_ = 0;
            selectAnimationBounds();
            updateCursor(hoverPosition_);
            update();
        });
        connect(editor, &EditorController::toolChanged, this, [this] {
            if (committing_)
                return;
            const auto tool = editor_->tool();
            const bool keepSelection =
                (previousTool_ == "Select" || previousTool_ == "Marquee" || previousTool_ == "Lasso") &&
                (tool == "Select" || tool == "Marquee" || tool == "Lasso") && vectorSelection_;
            previousTool_ = tool;
            if (tool != "Animate")
                setMotionPathEditing(false);
            cancelGesture();
            if (!keepSelection) {
                clearRegion();
                selectAnimationBounds();
            } else if (tool == "Marquee") {
                selectionMedia_ = SelectionMedia::Vectors;
                emit regionChanged();
            }
            updateCursor(hoverPosition_);
            update();
        });
        connect(editor, &EditorController::selectionChanged, this, [this] {
            if (committing_)
                return;
            cancelGesture();
            selectedStroke_ = 0;
            clearRegion();
            selectedStroke_ = 0;
            selectAnimationBounds();
            if (motionPathEditing_)
                resetMotionReference();
            updateCursor(hoverPosition_);
            update();
        });
    }
    emit editorChanged();
    update();
}
void CanvasItem::setZoom(double value) {
    if (motionKey_ >= 0)
        cancelGesture();
    zoom_ = std::clamp(value, 0.1, 10.0);
    emit viewChanged();
    updateCursor(hoverPosition_);
    update();
}
void CanvasItem::setMirrored(bool value) {
    if (motionKey_ >= 0)
        cancelGesture();
    mirrored_ = value;
    emit viewChanged();
    updateCursor(hoverPosition_);
    update();
}
void CanvasItem::setRotationAngle(double value) {
    if (motionKey_ >= 0)
        cancelGesture();
    angle_ = value;
    emit viewChanged();
    updateCursor(hoverPosition_);
    update();
}
void CanvasItem::fit() {
    if (motionKey_ >= 0)
        cancelGesture();
    zoom_ = 1;
    angle_ = 0;
    pan_ = {};
    emit viewChanged();
    updateCursor(hoverPosition_);
    update();
}
QTransform CanvasItem::viewTransform() const {
    QTransform transform;
    if (!editor_)
        return transform;
    double scale =
        std::min((width() - 64) / editor_->sceneWidth(), (height() - 64) / editor_->sceneHeight()) * zoom_;
    transform.translate(width() / 2 + pan_.x(), height() / 2 + pan_.y());
    transform.rotate(angle_);
    transform.scale(mirrored_ ? -scale : scale, scale);
    transform.translate(-editor_->sceneWidth() / 2.0, -editor_->sceneHeight() / 2.0);
    return transform;
}
Point CanvasItem::localPoint(QPointF p, double pressure) const {
    QTransform transform = viewTransform();
    if (editor_ && editor_->selectedLayer())
        transform = SceneRenderer::worldTransform(editor_->document(),
                                                  editor_->document().layer(editor_->selectedLayer()),
                                                  editor_->frame()) *
                    transform;
    bool ok = false;
    auto inverse = transform.inverted(&ok);
    if (!ok)
        throw std::runtime_error("Cannot draw through a zero-scale transform.");
    auto result = inverse.map(p);
    if (!std::isfinite(result.x()) || !std::isfinite(result.y()) || std::abs(result.x()) > 10000000 ||
        std::abs(result.y()) > 10000000)
        throw std::runtime_error("Drawing coordinates exceed the supported range.");
    return {result.x(), result.y(), std::clamp(pressure, 0.0, 1.0)};
}
void CanvasItem::paint(QPainter* p) {
    const auto itemTransform = p->worldTransform();
    p->fillRect(boundingRect(), QColor("#151515"));
    if (!editor_)
        return;
    p->setRenderHint(QPainter::Antialiasing);
    const auto& displayDocument = posePreview_ ? *posePreview_ : editor_->document();
    auto view = viewTransform();
    p->save();
    p->setWorldTransform(view, true);
    p->fillRect(QRectF(-1, -1, editor_->sceneWidth() + 2, editor_->sceneHeight() + 2), QColor("#454545"));
    SceneRenderer::paint(
        *p, displayDocument, editor_->frame(),
        {true, editor_->onionSkin(), 1, 0,
         (rasterBrush_ || (drawing_ && selectedPoint_ >= 0 && editor_->tool() == "Edit points") ||
          (transforming_ && previewValid_ && !posePreview_))
             ? Id(editor_->selectedLayer())
             : 0,
         rasterBrush_ ? &rasterPreview_
         : (drawing_ && selectedPoint_ >= 0 && editor_->tool() == "Edit points")
             ? &pointDrawingPreview_
             : (transforming_ && previewValid_ && !posePreview_ ? &transformPreview_ : nullptr)});
    if (editor_->selectedLayer()) {
        p->save();
        p->setWorldTransform(SceneRenderer::worldTransform(displayDocument,
                                                           displayDocument.layer(editor_->selectedLayer()),
                                                           editor_->frame()),
                             true);
        paintGrid(p);
        if (!rasterBrush_ && drawing_ && !samples_.empty() && editor_->tool() != "Eraser" &&
            editor_->tool() != "Select" && editor_->tool() != "Marquee" && editor_->tool() != "Lasso" &&
            editor_->tool() != "Recolor" && editor_->tool() != "Edit points" &&
            editor_->tool() != "Animate") {
            Stroke stroke{0,
                          Id(editor_->selectedSwatch()),
                          editor_->brushSize(),
                          Shape::Stroke,
                          editor_->filled(),
                          editor_->artLayer(),
                          samples_};
            if (editor_->tool() == "Rectangle")
                stroke.shape = Shape::Rectangle;
            if (editor_->tool() == "Ellipse")
                stroke.shape = Shape::Ellipse;
            SceneRenderer::paintStroke(*p, stroke, editor_->document().palette);
        }
        if (editor_->tool() == "Lasso" && drawing_ && !transforming_ && !samples_.empty()) {
            QPolygonF outline;
            for (auto point : samples_)
                outline << QPointF(point.x, point.y);
            QPen pen(Qt::gray, 1, Qt::DashLine);
            pen.setCosmetic(true);
            p->setPen(pen);
            p->setBrush(QColor(127, 127, 127, 20));
            p->drawPolygon(outline, Qt::OddEvenFill);
        }
        if (editor_->tool() == "Marquee" && drawing_ && !transforming_ && !samples_.empty()) {
            auto rect = QRectF(QPointF(samples_.front().x, samples_.front().y),
                               QPointF(samples_.back().x, samples_.back().y))
                            .normalized();
            QPen pen(Qt::white);
            pen.setCosmetic(true);
            pen.setStyle(Qt::DashLine);
            p->setPen(pen);
            p->setBrush(QColor(255, 255, 255, 15));
            p->drawRect(rect);
        }
        if (editor_->tool() == "Animate" && (hasRegion() || (motionPathEditing_ && motionReferenceValid_)))
            paintMotionPath(p, displayDocument, itemTransform);
        paintVectorSelection(p, itemTransform);
        if (hasRegion() && !motionPathEditing_ &&
            (editor_->tool() == "Select" || editor_->tool() == "Marquee" || editor_->tool() == "Lasso" ||
             editor_->tool() == "Animate")) {
            p->save();
            p->setWorldTransform(itemTransform);
            QPen pen(previewValid_ || !transforming_ ? QColor("#777777") : QColor(Qt::red));
            pen.setWidthF(1);
            p->setPen(pen);
            p->setBrush(Qt::NoBrush);
            QPolygonF outline;
            for (int i : {0, 2, 4, 6, 0})
                outline << handlePosition(i);
            p->drawPolyline(outline);
            p->drawLine(handlePosition(1), handlePosition(8));
            p->setBrush(QColor("#111111"));
            for (int i = 0; i < 8; ++i)
                if (handleVisible(i))
                    p->drawRect(QRectF(handlePosition(i) - QPointF(4, 4), QSizeF(8, 8)));
            p->drawEllipse(handlePosition(8), 5, 5);
            p->restore();
        }
        if (selectedStroke_ && editor_->tool() == "Edit points") {
            if (auto* d = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame()))
                for (const auto& stroke : d->strokes)
                    if (stroke.id == selectedStroke_) {
                        QPen pen(Qt::gray);
                        pen.setCosmetic(true);
                        p->setPen(pen);
                        p->setBrush(Qt::white);
                        for (std::size_t i = 0; i < stroke.points.size(); ++i) {
                            auto point =
                                drawing_ && int(i) == selectedPoint_ ? pointPreview_ : stroke.points[i];
                            const auto screenPoint = selectionWorld().map(QPointF(point.x, point.y));
                            p->save();
                            p->setWorldTransform(itemTransform);
                            double radius = int(i) == selectedPoint_ ? 4 : 3;
                            p->setBrush(int(i) == selectedPoint_ ? Qt::black : Qt::white);
                            p->drawRect(QRectF(screenPoint - QPointF(radius, radius),
                                               QSizeF(radius * 2, radius * 2)));
                            p->restore();
                        }
                    }
        }
        p->restore();
    }
    p->restore();
}
void CanvasItem::begin(QPointF position, double pressure) {
    if (!editor_ || !editor_->selectedLayer() || editor_->playing())
        return;
    forceActiveFocus();
    if (editor_->document().layer(editor_->selectedLayer()).locked) {
        editor_->report("Unlock the layer before drawing.");
        return;
    }
    try {
        if (editor_->tool() == "Animate" && motionPathEditing_) {
            beginMotionPath(position);
            return;
        }
        auto point = snapDrawingPoint(localPoint(position, pressure));
        if (editor_->tool() == "Marquee" || editor_->tool() == "Lasso" || editor_->tool() == "Select" ||
            editor_->tool() == "Animate") {
            const bool selectionTool = editor_->tool() != "Animate";
            if (editor_->tool() == "Lasso" && hasRegion() &&
                activeSelectionMedia() != SelectionMedia::Vectors)
                clearRegion();
            const int operation = subtract_ ? 2 : shift_ ? 1 : 0;
            if (selectionTool && operation) {
                if ((hasRegion() && activeSelectionMedia() != SelectionMedia::Vectors) ||
                    (editor_->tool() == "Marquee" && selectionMedia_ != SelectionMedia::Vectors)) {
                    editor_->report("Choose Vectors in Marquee before adding or subtracting strokes.");
                    return;
                }
                if (editor_->tool() == "Select") {
                    const auto hit = hitVector(position);
                    if (hit)
                        modifyVectorSelection({hit}, operation);
                } else {
                    marqueeOperation_ = operation;
                    samples_ = {point};
                    drawing_ = true;
                }
                update();
                return;
            }
            if (hasRegion()) {
                for (int handle = 8; handle >= 0; --handle)
                    if (handleVisible(handle) && QLineF(position, handlePosition(handle)).length() <= 10) {
                        samples_ = {point};
                        drawing_ = true;
                        startTransform(handle);
                        update();
                        return;
                    }
                if (QRectF(region_.x, region_.y, region_.width, region_.height)
                        .contains(QPointF(point.x, point.y))) {
                    if (editor_->tool() == "Select") {
                        const auto hit = hitVector(position);
                        if (hit && std::find(regionStrokes_.begin(), regionStrokes_.end(), hit) ==
                                       regionStrokes_.end())
                            selectStroke(hit);
                    }
                    samples_ = {point};
                    drawing_ = true;
                    startTransform(-1);
                    update();
                    return;
                }
            }
            if (editor_->tool() == "Animate")
                return;
            clearRegion();
            if (editor_->tool() == "Select") {
                if (auto* d = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame()))
                    selectStroke(hitVectorOnScreen(*d, selectionWorld(), position).value_or(0));
                if (hasRegion()) {
                    samples_ = {point};
                    drawing_ = true;
                    startTransform(-1);
                }
                update();
                return;
            }
        }
        if (editor_->tool().startsWith("Raster ")) {
            const auto* existing = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame());
            rasterPreview_ = existing ? *existing : Drawing{};
            RasterImage surface = rasterPreview_.raster.value_or(
                RasterImage{editor_->sceneWidth(), editor_->sceneHeight(), {}});
            Color color{0, 0, 0, 1};
            for (const auto& swatch : editor_->document().palette)
                if (swatch.id == Id(editor_->selectedSwatch()))
                    color = swatch.color;
            auto preset = BrushPreset::Ink;
            if (editor_->tool() == "Raster soft")
                preset = BrushPreset::Soft;
            if (editor_->tool() == "Raster dry")
                preset = BrushPreset::Dry;
            if (editor_->tool() == "Raster smudge")
                preset = BrushPreset::Smudge;
            if (editor_->tool() == "Raster eraser")
                preset = BrushPreset::Eraser;
            rasterBrush_ = std::make_unique<RasterBrush>(
                surface, BrushSettings{std::clamp(editor_->brushSize(), 1.0, 1024.0), editor_->brushOpacity(),
                                       color, preset});
            rasterBrush_->sample(point, 1.0 / 120, tiltX_, tiltY_);
            rasterPreview_.raster = rasterBrush_->snapshot();
            sampleClock_.start();
        }
        samples_ = {point};
        drawing_ = true;
        selectionDelta_ = {};
        last_ = position;
        if (editor_->tool() == "Select" || editor_->tool() == "Recolor" || editor_->tool() == "Edit points") {
            const auto previousStroke = selectedStroke_;
            selectedStroke_ = 0;
            if (auto* d = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame())) {
                selectedStroke_ = hitVectorOnScreen(*d, selectionWorld(), position).value_or(0);
                if (editor_->tool() == "Edit points")
                    for (const auto& stroke : d->strokes)
                        if (stroke.id == previousStroke)
                            for (auto p : stroke.points)
                                if (QLineF(position, selectionWorld().map(QPointF(p.x, p.y))).length() <= 10)
                                    selectedStroke_ = previousStroke;
            }
            if (editor_->tool() == "Recolor" && selectedStroke_) {
                drawing_ = false;
                editor_->recolorStroke(selectedStroke_);
            }
        }
        emit regionChanged();
        selectedPoint_ = -1;
        if (editor_->tool() == "Edit points" && selectedStroke_) {
            if (const auto* d = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame()))
                for (const auto& s : d->strokes)
                    if (s.id == selectedStroke_) {
                        pointDrawingPreview_ = *d;
                        double closest = 10;
                        for (std::size_t i = 0; i < s.points.size(); ++i) {
                            double distance =
                                QLineF(position, selectionWorld().map(QPointF(s.points[i].x, s.points[i].y)))
                                    .length();
                            if (distance < closest) {
                                closest = distance;
                                selectedPoint_ = int(i);
                                pointPreview_ = s.points[i];
                            }
                        }
                    }
        }
        update();
    } catch (const std::exception& e) {
        cancelGesture();
        editor_->report(e.what());
    }
}
void CanvasItem::move(QPointF position, double pressure) {
    if (panning_) {
        pan_ = panStart_ + position - last_;
        update();
        return;
    }
    if (!drawing_ || !editor_)
        return;
    try {
        if (motionKey_ >= 0) {
            previewMotionPath(position);
            return;
        }
        auto point = snapDrawingPoint(localPoint(position, pressure));
        if (transforming_) {
            QTransform matrix;
            QRectF box(region_.x, region_.y, region_.width, region_.height);
            auto first = samples_.front();
            if (transformHandle_ == -1)
                matrix.translate(std::round(point.x - first.x), std::round(point.y - first.y));
            else if (transformHandle_ == 8) {
                auto center = box.center();
                double angle = (std::atan2(point.y - center.y(), point.x - center.x()) -
                                std::atan2(first.y - center.y(), first.x - center.x())) *
                               180 / 3.14159265358979323846;
                if (shift_)
                    angle = std::round(angle / 15) * 15;
                matrix.translate(center.x(), center.y());
                matrix.rotate(angle);
                matrix.translate(-center.x(), -center.y());
            } else {
                int h = transformHandle_;
                bool left = h == 0 || h == 6 || h == 7, right = h == 2 || h == 3 || h == 4;
                bool top = h == 0 || h == 1 || h == 2, bottom = h == 4 || h == 5 || h == 6;
                QPointF anchor(left    ? box.right()
                               : right ? box.left()
                                       : box.center().x(),
                               top      ? box.bottom()
                               : bottom ? box.top()
                                        : box.center().y());
                double sx = left    ? (anchor.x() - point.x) / box.width()
                            : right ? (point.x - anchor.x()) / box.width()
                                    : 1;
                double sy = top      ? (anchor.y() - point.y) / box.height()
                            : bottom ? (point.y - anchor.y()) / box.height()
                                     : 1;
                if (shift_ && (left || right) && (top || bottom)) {
                    double scale = std::max(std::abs(sx), std::abs(sy));
                    sx = std::copysign(scale, sx);
                    sy = std::copysign(scale, sy);
                }
                if (std::abs(sx) < .01)
                    sx = std::copysign(.01, sx);
                if (std::abs(sy) < .01)
                    sy = std::copysign(.01, sy);
                matrix.translate(anchor.x(), anchor.y());
                matrix.scale(sx, sy);
                matrix.translate(-anchor.x(), -anchor.y());
            }
            previewTransform(matrix);
        } else if (rasterBrush_) {
            const auto elapsed = std::max(0.001, sampleClock_.nsecsElapsed() / 1e9);
            sampleClock_.restart();
            rasterBrush_->sample(point, elapsed, tiltX_, tiltY_);
            rasterPreview_.raster = rasterBrush_->snapshot();
        } else if (editor_->tool() == "Lasso") {
            if (samples_.size() < 4096 &&
                std::hypot(point.x - samples_.back().x, point.y - samples_.back().y) >= .5)
                samples_.push_back(point);
        } else if (editor_->tool() == "Marquee") {
            if (movingRegion_)
                selectionDelta_ = {std::round(point.x - samples_.front().x),
                                   std::round(point.y - samples_.front().y)};
            else if (samples_.size() == 1)
                samples_.push_back(point);
            else
                samples_.back() = point;
        } else if (editor_->tool() == "Edit points") {
            pointPreview_ = point;
            for (auto& stroke : pointDrawingPreview_.strokes)
                if (stroke.id == selectedStroke_ && selectedPoint_ >= 0)
                    stroke.points[selectedPoint_] = point;
        } else if (editor_->tool() == "Select")
            selectionDelta_ = {point.x - samples_.front().x, point.y - samples_.front().y};
        else if (editor_->tool() == "Ellipse" || editor_->tool() == "Rectangle" ||
                 editor_->tool() == "Line") {
            point = constrainedEndpoint(point);
            if (samples_.size() == 1)
                samples_.push_back(point);
            else
                samples_.back() = point;
        } else if (samples_.size() < 100000) {
            auto last = samples_.back();
            if (std::hypot(point.x - last.x, point.y - last.y) >= 0.3)
                samples_.push_back(point);
        }
        update();
    } catch (const std::exception& e) {
        cancelGesture();
        editor_->report(e.what());
    }
}
void CanvasItem::end() {
    if (panning_) {
        panning_ = false;
        return;
    }
    if (!drawing_ || !editor_)
        return;
    drawing_ = false;
    if (motionKey_ >= 0) {
        commitMotionPath();
        return;
    }
    auto points = std::move(samples_);
    samples_.clear();
    if (transforming_) {
        commitTransform();
    } else if (editor_->tool() == "Lasso") {
        if (const auto* d = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame())) {
            try {
                modifyVectorSelection(enclosedVectorsByLasso(*d, points), marqueeOperation_);
            } catch (const std::exception& e) {
                editor_->report(e.what());
            }
        }
        marqueeOperation_ = 0;
    } else if (editor_->tool() == "Marquee") {
        if (movingRegion_)
            transformRegion(int(SelectionAction::Move), int(selectionDelta_.x()), int(selectionDelta_.y()));
        else if (points.size() > 1) {
            double x = std::floor(std::min(points.front().x, points.back().x));
            double y = std::floor(std::min(points.front().y, points.back().y));
            double right = std::ceil(std::max(points.front().x, points.back().x));
            double bottom = std::ceil(std::max(points.front().y, points.back().y));
            if (std::abs(x) <= 10000000 && std::abs(y) <= 10000000 && right - x > 0 && bottom - y > 0 &&
                right - x <= 20000000 && bottom - y <= 20000000) {
                const PixelRect box{int(x), int(y), int(right - x), int(bottom - y)};
                std::vector<Id> ids;
                if (selectionMedia_ != SelectionMedia::Raster)
                    if (auto* d = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame()))
                        ids = enclosedStrokes(*d, box);
                if (selectionMedia_ == SelectionMedia::Vectors)
                    modifyVectorSelection(ids, marqueeOperation_);
                else {
                    region_ = box;
                    regionStrokes_ = std::move(ids);
                    vectorSelection_ = false;
                }
            }
            marqueeOperation_ = 0;
            emit regionChanged();
            editor_->report(regionInfo());
        }
        movingRegion_ = false;
    } else if (rasterBrush_) {
        auto raster = std::move(*rasterPreview_.raster);
        rasterBrush_.reset();
        editor_->commitRaster(std::move(raster));
    } else if (editor_->tool() == "Edit points") {
        if (selectedStroke_ && selectedPoint_ >= 0) {
            QScopedValueRollback<bool> guard(committing_, true);
            editor_->movePoint(selectedStroke_, selectedPoint_, pointPreview_);
            selectStroke(selectedStroke_);
        }
    } else if (editor_->tool() == "Eraser")
        editor_->eraseGesture(std::move(points));
    else if (editor_->tool() == "Select") {
        if (selectedStroke_)
            editor_->translateStroke(selectedStroke_, selectionDelta_.x(), selectionDelta_.y());
    } else if (editor_->tool() != "Recolor" && editor_->tool() != "Edit points" &&
               editor_->tool() != "Animate")
        editor_->commitStroke(std::move(points));
    selectionDelta_ = {};
    update();
}
void CanvasItem::cancelGesture() {
    motionKey_ = -1;
    marqueeOperation_ = 0;
    posePreview_.reset();
    pointDrawingPreview_ = {};
    selectedPoint_ = -1;
    transforming_ = false;
    previewValid_ = false;
    pendingTransform_ = {};
    transformPreview_ = {};
    transformSource_ = {};
    rasterBrush_.reset();
    rasterPreview_ = {};
    drawing_ = false;
    movingRegion_ = false;
    panning_ = false;
    samples_.clear();
    selectionDelta_ = {};
    updateCursor(hoverPosition_);
    update();
}
void CanvasItem::mousePressEvent(QMouseEvent* e) {
    shift_ = e->modifiers() & Qt::ShiftModifier;
    subtract_ = e->modifiers() & Qt::AltModifier;
    if (tablet_) {
        e->accept();
        return;
    }
    if (e->button() == Qt::MiddleButton) {
        if (motionKey_ >= 0)
            cancelGesture();
        panning_ = true;
        last_ = e->position();
        panStart_ = pan_;
    } else {
        tiltX_ = tiltY_ = 0;
        begin(e->position(), 1);
    }
    updateCursor(e->position());
    e->accept();
}
void CanvasItem::mouseMoveEvent(QMouseEvent* e) {
    shift_ = e->modifiers() & Qt::ShiftModifier;
    subtract_ = e->modifiers() & Qt::AltModifier;
    if (!tablet_)
        move(e->position(), 1);
    updateCursor(e->position());
    e->accept();
}
void CanvasItem::mouseReleaseEvent(QMouseEvent* e) {
    if (!tablet_)
        end();
    updateCursor(e->position());
    e->accept();
}
void CanvasItem::mouseUngrabEvent() {
    // Normal release also drops the grab; keep the completed point selection.
    if (drawing_ || panning_ || transforming_)
        cancelGesture();
}
void CanvasItem::wheelEvent(QWheelEvent* e) {
    if (motionKey_ >= 0)
        cancelGesture();
    if (e->modifiers() & Qt::ControlModifier)
        setZoom(zoom_ * std::pow(1.0015, e->angleDelta().y()));
    else {
        auto delta = e->pixelDelta();
        pan_ += delta.isNull() ? QPointF(e->angleDelta()) / 4 : QPointF(delta);
        update();
    }
    e->accept();
}
void CanvasItem::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape) {
        cancelGesture();
        e->accept();
    } else
        QQuickPaintedItem::keyPressEvent(e);
}
bool CanvasItem::eventFilter(QObject*, QEvent* event) {
    if (event->type() == QEvent::TabletPress || event->type() == QEvent::TabletMove ||
        event->type() == QEvent::TabletRelease) {
        auto* e = static_cast<QTabletEvent*>(event);
        auto point = mapFromScene(e->position());
        shift_ = e->modifiers() & Qt::ShiftModifier;
        subtract_ = e->modifiers() & Qt::AltModifier;
        tiltX_ = std::clamp(e->xTilt() / 90.0, -1.0, 1.0);
        tiltY_ = std::clamp(e->yTilt() / 90.0, -1.0, 1.0);
        if (event->type() == QEvent::TabletPress) {
            if (!contains(point))
                return false;
            tablet_ = true;
            begin(point, e->pressure());
        } else if (!tablet_)
            return false;
        else if (event->type() == QEvent::TabletMove)
            move(point, e->pressure());
        else {
            end();
            tablet_ = false;
        }
        event->accept();
        return true;
    }
    if (event->type() == QEvent::WindowDeactivate) {
        cancelGesture();
        tablet_ = false;
    }
    return false;
}
void CanvasItem::capture(QString path) {
    auto result = grabToImage();
    if (!result)
        return;
    connect(result.data(), &QQuickItemGrabResult::ready, this, [result, path] { result->saveToFile(path); });
}

void CanvasItem::smoothSelection() {
    if (editor_ && selectedStroke_)
        editor_->smoothStroke(selectedStroke_);
}
void CanvasItem::deleteSelection() {
    if (editor_ && editor_->tool() == "Edit points") {
        if (selectedStroke_ && selectedPoint_ >= 0) {
            QScopedValueRollback<bool> guard(committing_, true);
            if (editor_->deletePoint(selectedStroke_, selectedPoint_)) {
                selectedPoint_ = -1;
                selectStroke(selectedStroke_);
            }
        } else
            editor_->report("Select a control point to delete it.");
        return;
    }
    if (editor_ &&
        (editor_->tool() == "Marquee" || editor_->tool() == "Lasso" || editor_->tool() == "Select") &&
        hasRegion()) {
        transformRegion(int(SelectionAction::Delete));
    } else if (editor_ && selectedStroke_) {
        editor_->deleteStroke(selectedStroke_);
        selectedStroke_ = 0;
        update();
    }
}

void CanvasItem::setSelectionMedia(int value) {
    if (value < 0 || value > 2 || value == int(selectionMedia_))
        return;
    cancelGesture();
    clearRegion();
    selectionMedia_ = static_cast<SelectionMedia>(value);
    emit regionChanged();
    update();
}
QString CanvasItem::regionInfo() const {
    if (!hasRegion())
        return "Drag a rectangle. Vectors must fit completely; imported images are excluded.";
    const auto count = regionStrokes_.size();
    return QString("%1 × %2 px · %3 vector strokes · drag handles to scale, top circle to rotate")
        .arg(region_.width)
        .arg(region_.height)
        .arg(count);
}
void CanvasItem::clearRegion() {
    region_ = {};
    regionStrokes_.clear();
    vectorSelection_ = false;
    selectedStroke_ = 0;
    emit regionChanged();
    update();
}
void CanvasItem::transformRegion(int action, int dx, int dy) {
    if (!editor_ || !hasRegion() || action < 0 || action > 5)
        return;
    auto rect = region_;
    auto ids = regionStrokes_;
    QScopedValueRollback<bool> guard(committing_, true);
    if (editor_->editDrawingRegion(rect, activeSelectionMedia(), static_cast<SelectionAction>(action), dx, dy,
                                   &ids, &ids)) {
        if (action == int(SelectionAction::Delete))
            clearRegion();
        else {
            if (action == int(SelectionAction::Move) || action == int(SelectionAction::Duplicate)) {
                rect.x += dx;
                rect.y += dy;
            }
            if (action == int(SelectionAction::RotateClockwise))
                std::swap(rect.width, rect.height);
            region_ = rect;
            regionStrokes_ = ids;
            if (vectorSelection_)
                setVectorSelection(ids);
        }
    }
    emit regionChanged();
    update();
}

QTransform CanvasItem::selectionWorld() const {
    if (!editor_ || !editor_->selectedLayer())
        return viewTransform();
    const auto& doc = posePreview_ ? *posePreview_ : editor_->document();
    return SceneRenderer::worldTransform(doc, doc.layer(editor_->selectedLayer()), editor_->frame()) *
           viewTransform();
}
QPointF CanvasItem::handlePosition(int handle) const {
    QRectF r(region_.x, region_.y, region_.width, region_.height);
    const QPointF points[] = {r.topLeft(),     QPointF(r.center().x(), r.top()),
                              r.topRight(),    QPointF(r.right(), r.center().y()),
                              r.bottomRight(), QPointF(r.center().x(), r.bottom()),
                              r.bottomLeft(),  QPointF(r.left(), r.center().y())};
    auto world =
        (editor_ && editor_->tool() == "Animate" ? QTransform{} : pendingTransform_) * selectionWorld();
    if (handle == 8) {
        auto top = world.map(points[1]), center = world.map(r.center());
        auto delta = top - center;
        double length = std::hypot(delta.x(), delta.y());
        return top + (length > 0 ? delta / length * 28 : QPointF(0, -28));
    }
    return world.map(points[std::clamp(handle, 0, 7)]);
}
void CanvasItem::startTransform(int handle) {
    if (!editor_ || !hasRegion())
        return;
    if (auto* drawing = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame())) {
        sourcePose_ =
            evaluateTransform(editor_->document().layer(editor_->selectedLayer()), editor_->frame());
        transformSource_ = *drawing;
        transformPreview_ = *drawing;
        transformHandle_ = handle;
        transforming_ = true;
        previewValid_ = true;
        pendingTransform_ = {};
    }
}
void CanvasItem::previewTransform(QTransform matrix) {
    if (editor_->tool() == "Animate") {
        previewPose(matrix);
        return;
    }
    pendingTransform_ = matrix;
    try {
        transformPreview_ = transformSource_;
        transformDrawingSelection(
            transformPreview_, region_, activeSelectionMedia(), regionStrokes_,
            {matrix.m11(), matrix.m12(), matrix.m21(), matrix.m22(), matrix.dx(), matrix.dy()});
        previewValid_ = true;
    } catch (const std::exception& error) {
        previewValid_ = false;
        editor_->report(error.what());
    }
    emit regionChanged();
    update();
}
void CanvasItem::commitTransform() {
    if (editor_->tool() == "Animate") {
        bool commit = posePreview_ && previewValid_ && previewPose_ != sourcePose_;
        auto pose = previewPose_;
        cancelGesture();
        if (commit) {
            QScopedValueRollback<bool> guard(committing_, true);
            editor_->commitPose(pose);
        }
        selectAnimationBounds();
        return;
    }
    auto matrix = pendingTransform_;
    auto original = region_;
    auto ids = regionStrokes_;
    bool valid = previewValid_;
    transforming_ = false;
    pendingTransform_ = {};
    transformPreview_ = {};
    transformSource_ = {};
    if (valid && !matrix.isIdentity()) {
        QScopedValueRollback<bool> guard(committing_, true);
        if (editor_->transformDrawingRegion(
                original, activeSelectionMedia(), ids,
                {matrix.m11(), matrix.m12(), matrix.m21(), matrix.m22(), matrix.dx(), matrix.dy()})) {
            auto r = matrix.mapRect(QRectF(original.x, original.y, original.width, original.height))
                         .toAlignedRect();
            region_ = {r.x(), r.y(), r.width(), r.height()};
            regionStrokes_ = ids;
            if (vectorSelection_)
                setVectorSelection(ids);
        }
    }
    emit regionChanged();
    update();
}
QVariantMap CanvasItem::objectProperties() const {
    QVariantMap result{{"kind", "none"}};
    if (!hasRegion() || !editor_ || editor_->tool() == "Animate")
        return result;
    auto bounds = pendingTransform_.mapRect(QRectF(region_.x, region_.y, region_.width, region_.height));
    result = {{"kind", selectedStroke_                                    ? "vector"
                       : activeSelectionMedia() == SelectionMedia::Raster ? "raster"
                                                                          : "selection"},
              {"x", bounds.x()},
              {"y", bounds.y()},
              {"width", bounds.width()},
              {"height", bounds.height()},
              {"rotation", 0.0},
              {"count", int(regionStrokes_.size())},
              {"vectorOnly", vectorSelection_},
              {"locked", editor_->document().layer(editor_->selectedLayer()).locked}};
    if (vectorSelection_)
        if (const auto* d = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame())) {
            std::optional<double> width;
            bool mixed = false;
            for (const auto& s : d->strokes)
                if (std::find(regionStrokes_.begin(), regionStrokes_.end(), s.id) != regionStrokes_.end()) {
                    if (width && *width != s.width)
                        mixed = true;
                    width = s.width;
                }
            result.insert("commonStrokeWidth", !mixed && width ? QVariant(*width) : QVariant{});
        }
    if (selectedStroke_)
        if (auto* d = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame()))
            for (const auto& stroke : d->strokes)
                if (stroke.id == selectedStroke_) {
                    result.insert("strokeWidth", stroke.width);
                    result.insert("filled", stroke.filled);
                    result.insert("artLayer", stroke.artLayer);
                    result.insert("swatch", int(stroke.swatch));
                }
    return result;
}
void CanvasItem::setObjectProperty(QString name, double value) {
    if (!editor_ || !hasRegion() || !std::isfinite(value))
        return;
    if (name == "strokeWidth" || name == "filled" || name == "artLayer" || name == "swatch") {
        if (selectedStroke_) {
            auto id = selectedStroke_;
            QScopedValueRollback<bool> guard(committing_, true);
            editor_->setStrokeProperty(id, name, value);
            selectStroke(id);
        }
        return;
    }
    QTransform matrix;
    QRectF r(region_.x, region_.y, region_.width, region_.height);
    if (name == "x")
        matrix.translate(value - r.x(), 0);
    else if (name == "y")
        matrix.translate(0, value - r.y());
    else if (name == "width" || name == "height") {
        if (value < 1)
            return;
        matrix.translate(r.x(), r.y());
        matrix.scale(name == "width" ? value / r.width() : 1, name == "height" ? value / r.height() : 1);
        matrix.translate(-r.x(), -r.y());
    } else if (name == "rotation") {
        matrix.translate(r.center().x(), r.center().y());
        matrix.rotate(value);
        matrix.translate(-r.center().x(), -r.center().y());
    } else
        return;
    startTransform(-1);
    previewTransform(matrix);
    commitTransform();
}

bool CanvasItem::handleVisible(int handle) const {
    // Opposing middle handles must not cover the move target of a thin line.
    if (handle == 1 || handle == 5)
        return QLineF(handlePosition(1), handlePosition(5)).length() >= 24;
    if (handle == 3 || handle == 7)
        return QLineF(handlePosition(3), handlePosition(7)).length() >= 24;
    return true;
}

void CanvasItem::mouseDoubleClickEvent(QMouseEvent* e) {
    if (editor_ && editor_->tool() == "Animate" && motionPathEditing_ && !editor_->playing() &&
        editor_->selectedLayer()) {
        cancelGesture();
        const int frame = motionPathFrameAt(e->position());
        if (frame >= 0) {
            editor_->setFrame(frame);
            editor_->addKey();
            editor_->clearPoseSelection();
            editor_->selectPoseKey(frame);
        } else
            editor_->report("Double-click near the motion path to add a pose at a sampled frame.");
        e->accept();
        return;
    }
    if (!editor_ || editor_->tool() != "Edit points" || editor_->playing() || !editor_->selectedLayer()) {
        QQuickPaintedItem::mouseDoubleClickEvent(e);
        return;
    }
    cancelGesture();
    const auto* drawing = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame());
    if (!drawing)
        return;
    Id id = hitVector(e->position());
    double closest = 8, fraction = 0;
    int segment = -1;
    for (const auto& stroke : drawing->strokes)
        if (stroke.id == id && (stroke.shape == Shape::Stroke || stroke.shape == Shape::Polygon)) {
            const auto count = stroke.points.size();
            const auto segments = stroke.shape == Shape::Polygon ? count : count ? count - 1 : 0;
            for (std::size_t i = 0; i < segments; ++i) {
                auto a = selectionWorld().map(QPointF(stroke.points[i].x, stroke.points[i].y));
                auto b = selectionWorld().map(
                    QPointF(stroke.points[(i + 1) % count].x, stroke.points[(i + 1) % count].y));
                auto v = b - a, delta = e->position() - a;
                const double length = v.x() * v.x() + v.y() * v.y();
                if (length < 1e-12)
                    continue;
                double t = std::clamp(QPointF::dotProduct(delta, v) / length, 0.0, 1.0);
                double distance = QLineF(e->position(), a + v * t).length();
                if (t > 1e-6 && t < 1 - 1e-6 && distance < closest) {
                    closest = distance;
                    fraction = t;
                    segment = int(i);
                }
            }
        }
    if (segment >= 0) {
        QScopedValueRollback<bool> guard(committing_, true);
        if (editor_->insertPoint(id, segment, fraction)) {
            selectStroke(id);
            selectedPoint_ = segment + 1;
            update();
        }
    } else
        editor_->report("Double-click inside a pencil or polygon segment to insert a point.");
    e->accept();
}
