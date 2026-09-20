#include "canvas_item.h"
#include "vector_hit.h"
#include <QCursor>
#include <QHash>
#include <QHoverEvent>
#include <QPainter>
#include <cmath>
using namespace opentoon;
namespace {
QCursor toolCursor(const QString& tool) {
    static QHash<QString, QCursor> cursors;
    if (auto found = cursors.constFind(tool); found != cursors.cend())
        return *found;
    // Original monochrome cursor artwork. The cross at (6,6) is the precise drawing hotspot.
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(Qt::black, 3));
    p.drawLine(2, 6, 10, 6);
    p.drawLine(6, 2, 6, 10);
    p.setPen(QPen(Qt::white, 1));
    p.drawLine(2, 6, 10, 6);
    p.drawLine(6, 2, 6, 10);
    p.setPen(QPen(Qt::white, 1.5));
    p.setBrush(QColor("#151515"));
    if (tool.contains("eraser", Qt::CaseInsensitive) || tool == "Eraser") {
        p.drawPolygon(QPolygonF{QPointF(12, 22), QPointF(20, 12), QPointF(28, 18), QPointF(20, 28)});
        p.drawLine(16, 17, 24, 23);
    } else if (tool == "Rectangle" || tool == "Marquee") {
        if (tool == "Marquee")
            p.setPen(QPen(Qt::white, 1.5, Qt::DashLine));
        p.drawRect(13, 13, 14, 12);
    } else if (tool == "Ellipse")
        p.drawEllipse(13, 13, 14, 12);
    else if (tool == "Edit points") {
        p.drawLine(13, 25, 27, 13);
        p.drawRect(17, 17, 6, 6);
    } else if (tool == "Recolor") {
        p.drawPolygon(QPolygonF{QPointF(13, 20), QPointF(20, 12), QPointF(28, 20), QPointF(20, 27)});
        p.drawLine(12, 29, 28, 29);
    } else if (tool == "Rotate") {
        p.drawArc(QRectF(12, 12, 16, 16), 25 * 16, 280 * 16);
        p.drawLine(27, 12, 28, 19);
        p.drawLine(28, 19, 21, 18);
    } else if (tool == "Animate") {
        p.drawPolygon(QPolygonF{QPointF(13, 20), QPointF(20, 13), QPointF(27, 20), QPointF(20, 27)});
    } else if (tool.startsWith("Raster")) {
        p.drawEllipse(13, 13, 14, 14);
        if (tool == "Raster soft")
            p.drawEllipse(17, 17, 6, 6);
        else if (tool == "Raster smudge")
            p.drawLine(12, 28, 27, 16);
        else if (tool == "Raster dry") {
            p.drawPoint(18, 18);
            p.drawPoint(23, 22);
            p.drawPoint(17, 24);
        } else {
            p.setBrush(Qt::white);
            p.drawEllipse(18, 18, 4, 4);
        }
    } else {
        p.drawPolygon(
            QPolygonF{QPointF(12, 28), QPointF(16, 19), QPointF(25, 10), QPointF(29, 14), QPointF(20, 23)});
        p.drawLine(16, 19, 20, 23);
    }
    p.end();
    QCursor cursor(pixmap, 6, 6);
    cursors.insert(tool, cursor);
    return cursor;
}
} // namespace
Id CanvasItem::hitVector(QPointF point) const {
    if (editor_ && editor_->selectedLayer())
        if (const auto* d = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame()))
            return hitVectorOnScreen(*d, selectionWorld(), point).value_or(0);
    return 0;
}
void CanvasItem::updateCursor(QPointF point) {
    hoverPosition_ = point;
    if (!editor_) {
        setCursor(Qt::ArrowCursor);
        return;
    }
    if (panning_) {
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (!editor_->selectedLayer() || editor_->playing() ||
        editor_->document().layer(editor_->selectedLayer()).locked) {
        setCursor(Qt::ForbiddenCursor);
        return;
    }
    const auto tool = editor_->tool();
    if (tool == "Select" || tool == "Marquee" || tool == "Animate") {
        if (hasRegion()) {
            for (int h = 8; h >= 0; --h) {
                if (!handleVisible(h))
                    continue;
                if ((transforming_ && transformHandle_ == h) ||
                    QLineF(point, handlePosition(h)).length() <= 10) {
                    if (h == 8)
                        setCursor(toolCursor("Rotate"));
                    else {
                        auto direction = handlePosition(h) - handlePosition((h + 4) % 8);
                        double angle = std::atan2(direction.y(), direction.x()) * 180 / std::acos(-1);
                        int sector = (int(std::lround(angle / 45)) % 4 + 4) % 4;
                        const Qt::CursorShape shapes[] = {Qt::SizeHorCursor, Qt::SizeFDiagCursor,
                                                          Qt::SizeVerCursor, Qt::SizeBDiagCursor};
                        setCursor(shapes[sector]);
                    }
                    return;
                }
            }
            bool invertible;
            auto local = selectionWorld().inverted(&invertible).map(point);
            if (transforming_ ||
                (invertible && QRectF(region_.x, region_.y, region_.width, region_.height).contains(local))) {
                setCursor(transforming_ ? Qt::ClosedHandCursor : Qt::OpenHandCursor);
                return;
            }
        }
        if (tool == "Select") {
            setCursor(hitVector(point) ? Qt::OpenHandCursor : Qt::ArrowCursor);
            return;
        }
    }
    if (tool == "Edit points" && selectedStroke_) {
        if (const auto* d = editor_->document().drawingAt(editor_->selectedLayer(), editor_->frame()))
            for (const auto& s : d->strokes)
                if (s.id == selectedStroke_)
                    for (auto p : s.points)
                        if (QLineF(point, selectionWorld().map(QPointF(p.x, p.y))).length() <= 10) {
                            setCursor(drawing_ ? Qt::ClosedHandCursor : Qt::PointingHandCursor);
                            return;
                        }
    }
    setCursor(toolCursor(tool));
}
void CanvasItem::hoverMoveEvent(QHoverEvent* e) {
    updateCursor(e->position());
    e->accept();
}
void CanvasItem::hoverEnterEvent(QHoverEvent* e) {
    updateCursor(e->position());
    e->accept();
}
void CanvasItem::hoverLeaveEvent(QHoverEvent* e) {
    unsetCursor();
    e->accept();
}
