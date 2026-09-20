#include "canvas_item.h"
#include "editor_controller.h"
#include "scene_renderer.h"
#include <QMouseEvent>
#include <QPainter>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
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
    if (editor) {
        connect(editor, &EditorController::changed, this, [this] {
            cancelGesture();
            update();
        });
        connect(editor, &EditorController::frameChanged, this, [this] {
            cancelGesture();
            selectedStroke_ = 0;
            update();
        });
        connect(editor, &EditorController::toolChanged, this, [this] {
            cancelGesture();
            update();
        });
        connect(editor, &EditorController::selectionChanged, this, [this] {
            cancelGesture();
            selectedStroke_ = 0;
            update();
        });
    }
    emit editorChanged();
    update();
}
void CanvasItem::setZoom(double value) {
    zoom_ = std::clamp(value, 0.1, 10.0);
    emit viewChanged();
    update();
}
void CanvasItem::setMirrored(bool value) {
    mirrored_ = value;
    emit viewChanged();
    update();
}
void CanvasItem::setRotationAngle(double value) {
    angle_ = value;
    emit viewChanged();
    update();
}
void CanvasItem::fit() {
    zoom_ = 1;
    angle_ = 0;
    pan_ = {};
    emit viewChanged();
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
    return {result.x(), result.y(), std::clamp(pressure, 0.0, 1.0)};
}
void CanvasItem::paint(QPainter* p) {
    p->fillRect(boundingRect(), QColor("#151515"));
    if (!editor_)
        return;
    p->setRenderHint(QPainter::Antialiasing);
    auto view = viewTransform();
    p->save();
    p->setWorldTransform(view, true);
    p->fillRect(QRectF(-1, -1, editor_->sceneWidth() + 2, editor_->sceneHeight() + 2), QColor("#454545"));
    SceneRenderer::paint(*p, editor_->document(), editor_->frame(),
                         {true, editor_->onionSkin(), 1, 0, rasterBrush_ ? Id(editor_->selectedLayer()) : 0,
                          rasterBrush_ ? &rasterPreview_ : nullptr});
    if (editor_->selectedLayer()) {
        p->save();
        p->setWorldTransform(
            SceneRenderer::worldTransform(
                editor_->document(), editor_->document().layer(editor_->selectedLayer()), editor_->frame()),
            true);
        if (!rasterBrush_ && drawing_ && !samples_.empty() && editor_->tool() != "Eraser" &&
            editor_->tool() != "Select" && editor_->tool() != "Recolor" && editor_->tool() != "Edit points") {
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
        if (selectedStroke_) {
            if (auto* d = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame()))
                for (const auto& s : d->strokes)
                    if (s.id == selectedStroke_) {
                        QRectF rect;
                        for (auto point : s.points) {
                            QRectF r(point.x, point.y, 0.01, 0.01);
                            rect = rect.isNull() ? r : rect.united(r);
                        }
                        rect.translate(selectionDelta_);
                        QPen pen(QColor("#777777"));
                        pen.setCosmetic(true);
                        pen.setStyle(Qt::DashLine);
                        p->setPen(pen);
                        p->setBrush(Qt::NoBrush);
                        p->drawRect(rect.adjusted(-6, -6, 6, 6));
                        if (editor_->tool() == "Edit points") {
                            p->setBrush(Qt::white);
                            for (std::size_t i = 0; i < s.points.size(); ++i) {
                                auto point =
                                    (drawing_ && int(i) == selectedPoint_) ? pointPreview_ : s.points[i];
                                p->drawRect(QRectF(point.x - 3, point.y - 3, 6, 6));
                            }
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
        auto point = localPoint(position, pressure);
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
                surface, BrushSettings{std::clamp(editor_->brushSize(), 1.0, 1024.0), 1, color, preset});
            rasterBrush_->sample(point, 1.0 / 120, tiltX_, tiltY_);
            rasterPreview_.raster = rasterBrush_->snapshot();
            sampleClock_.start();
        }
        samples_ = {point};
        drawing_ = true;
        selectionDelta_ = {};
        last_ = position;
        if (editor_->tool() == "Select" || editor_->tool() == "Recolor" || editor_->tool() == "Edit points") {
            selectedStroke_ = 0;
            if (auto* d = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame()))
                selectedStroke_ = hitStroke(*d, point, 8).value_or(0);
            if (editor_->tool() == "Recolor" && selectedStroke_) {
                drawing_ = false;
                editor_->recolorStroke(selectedStroke_);
            }
        }
        selectedPoint_ = -1;
        if (editor_->tool() == "Edit points" && selectedStroke_) {
            if (const auto* d = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame()))
                for (const auto& s : d->strokes)
                    if (s.id == selectedStroke_) {
                        double closest = 24;
                        for (std::size_t i = 0; i < s.points.size(); ++i) {
                            double distance = std::hypot(point.x - s.points[i].x, point.y - s.points[i].y);
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
        auto point = localPoint(position, pressure);
        if (rasterBrush_) {
            const auto elapsed = std::max(0.001, sampleClock_.nsecsElapsed() / 1e9);
            sampleClock_.restart();
            rasterBrush_->sample(point, elapsed, tiltX_, tiltY_);
            rasterPreview_.raster = rasterBrush_->snapshot();
        } else if (editor_->tool() == "Edit points")
            pointPreview_ = point;
        else if (editor_->tool() == "Select")
            selectionDelta_ = {point.x - samples_.front().x, point.y - samples_.front().y};
        else if (editor_->tool() == "Ellipse" || editor_->tool() == "Rectangle") {
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
    auto points = std::move(samples_);
    samples_.clear();
    if (rasterBrush_) {
        auto raster = std::move(*rasterPreview_.raster);
        rasterBrush_.reset();
        editor_->commitRaster(std::move(raster));
    } else if (editor_->tool() == "Edit points") {
        if (selectedStroke_ && selectedPoint_ >= 0)
            editor_->movePoint(selectedStroke_, selectedPoint_, pointPreview_);
    } else if (editor_->tool() == "Eraser")
        editor_->eraseGesture(std::move(points));
    else if (editor_->tool() == "Select") {
        if (selectedStroke_)
            editor_->translateStroke(selectedStroke_, selectionDelta_.x(), selectionDelta_.y());
    } else if (editor_->tool() != "Recolor" && editor_->tool() != "Edit points")
        editor_->commitStroke(std::move(points));
    selectionDelta_ = {};
    update();
}
void CanvasItem::cancelGesture() {
    rasterBrush_.reset();
    rasterPreview_ = {};
    drawing_ = false;
    panning_ = false;
    samples_.clear();
    selectionDelta_ = {};
    update();
}
void CanvasItem::mousePressEvent(QMouseEvent* e) {
    if (tablet_) {
        e->accept();
        return;
    }
    if (e->button() == Qt::MiddleButton) {
        panning_ = true;
        last_ = e->position();
        panStart_ = pan_;
    } else {
        tiltX_ = tiltY_ = 0;
        begin(e->position(), 1);
    }
    e->accept();
}
void CanvasItem::mouseMoveEvent(QMouseEvent* e) {
    if (!tablet_)
        move(e->position(), 1);
    e->accept();
}
void CanvasItem::mouseReleaseEvent(QMouseEvent* e) {
    if (!tablet_)
        end();
    e->accept();
}
void CanvasItem::mouseUngrabEvent() {
    cancelGesture();
}
void CanvasItem::wheelEvent(QWheelEvent* e) {
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
    if (editor_ && selectedStroke_) {
        editor_->deleteStroke(selectedStroke_);
        selectedStroke_ = 0;
        update();
    }
}
