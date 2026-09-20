#include "vector_hit.h"
#include <QPainterPath>
#include <QPainterPathStroker>
#include <cmath>
#include <stdexcept>
namespace opentoon {
std::optional<Id> hitVectorOnScreen(const Drawing& d, const QTransform& transform, QPointF point,
                                    double radius) {
    auto contains = [&](const QPainterPath& path, double width, bool fill) {
        QPainterPathStroker stroke;
        stroke.setWidth(width);
        stroke.setCapStyle(Qt::RoundCap);
        stroke.setJoinStyle(Qt::RoundJoin);
        auto ink = transform.map(stroke.createStroke(path));
        if (fill)
            ink = ink.united(transform.map(path));
        QPainterPathStroker margin;
        margin.setWidth(radius * 2);
        margin.setCapStyle(Qt::RoundCap);
        margin.setJoinStyle(Qt::RoundJoin);
        return ink.contains(point) || margin.createStroke(ink).contains(point);
    };
    for (int art = 3; art >= 0; --art)
        for (auto it = d.strokes.rbegin(); it != d.strokes.rend(); ++it) {
            const auto& s = *it;
            if (s.artLayer != art || s.points.empty())
                continue;
            QPainterPath path;
            const auto first = s.points.front(), last = s.points.back();
            if (s.shape == Shape::Ellipse || s.shape == Shape::Rectangle) {
                QRectF bounds(QPointF(first.x, first.y), QPointF(last.x, last.y));
                if (s.shape == Shape::Ellipse)
                    path.addEllipse(bounds.normalized());
                else
                    path.addRect(bounds.normalized());
                if (contains(path, s.width, s.filled))
                    return s.id;
            } else if (s.shape == Shape::Stroke) {
                if (s.points.size() == 1) {
                    double r = s.width * std::max(.05, first.pressure) / 2;
                    path.addEllipse(QPointF(first.x, first.y), r, r);
                    if (contains(path, 0.01, true))
                        return s.id;
                }
                for (std::size_t i = 1; i < s.points.size(); ++i) {
                    auto a = s.points[i - 1], b = s.points[i];
                    QPainterPath segment(QPointF(a.x, a.y));
                    segment.lineTo(b.x, b.y);
                    if (contains(segment, s.width * std::max(.05, (a.pressure + b.pressure) / 2), false))
                        return s.id;
                }
            } else {
                path.moveTo(first.x, first.y);
                for (auto p : s.points)
                    path.lineTo(p.x, p.y);
                path.closeSubpath();
                if (contains(path, s.width, s.filled))
                    return s.id;
            }
        }
    return {};
}
} // namespace opentoon

namespace opentoon {
std::vector<Id> enclosedVectorsByLasso(const Drawing& drawing, const std::vector<Point>& points) {
    if (points.size() < 3)
        return {};
    if (points.size() > 4096)
        throw std::invalid_argument("Lasso exceeds the 4096-point input budget.");
    QPainterPath lasso;
    lasso.moveTo(points.front().x, points.front().y);
    for (auto p : points) {
        if (!std::isfinite(p.x) || !std::isfinite(p.y))
            throw std::invalid_argument("Invalid lasso coordinate.");
        lasso.lineTo(p.x, p.y);
    }
    lasso.closeSubpath();
    lasso.setFillRule(Qt::OddEvenFill);
    std::vector<Id> result;
    for (const auto& s : drawing.strokes) {
        if (s.points.empty())
            continue;
        QPainterPath ink;
        ink.setFillRule(Qt::WindingFill);
        auto add = [&](QPainterPath path, double width, bool filled) {
            QPainterPathStroker stroker;
            stroker.setWidth(width);
            stroker.setCapStyle(Qt::RoundCap);
            stroker.setJoinStyle(Qt::RoundJoin);
            ink.addPath(stroker.createStroke(path));
            if (filled)
                ink.addPath(path);
        };
        const auto first = s.points.front(), last = s.points.back();
        if (s.shape == Shape::Stroke) {
            if (s.points.size() == 1) {
                const double r = s.width * std::max(.05, first.pressure) / 2;
                ink.addEllipse(QPointF(first.x, first.y), r, r);
            }
            for (std::size_t i = 1; i < s.points.size(); ++i) {
                const auto a = s.points[i - 1], b = s.points[i];
                QPainterPath path(QPointF(a.x, a.y));
                path.lineTo(b.x, b.y);
                // A repeated sample still produces a round dot in the renderer.
                if (a.x == b.x && a.y == b.y) {
                    const double r = s.width * std::max(.05, (a.pressure + b.pressure) / 2) / 2;
                    ink.addEllipse(QPointF(a.x, a.y), r, r);
                } else
                    add(path, s.width * std::max(.05, (a.pressure + b.pressure) / 2), false);
            }
        } else {
            QPainterPath path;
            if (s.shape == Shape::Rectangle || s.shape == Shape::Ellipse) {
                const QRectF r = QRectF(QPointF(first.x, first.y), QPointF(last.x, last.y)).normalized();
                if (s.shape == Shape::Rectangle)
                    path.addRect(r);
                else
                    path.addEllipse(r);
            } else {
                path.moveTo(first.x, first.y);
                for (auto p : s.points)
                    path.lineTo(p.x, p.y);
                path.closeSubpath();
            }
            add(path, s.width, s.filled);
        }
        if (!ink.isEmpty() && lasso.contains(ink))
            result.push_back(s.id);
    }
    return result;
}
} // namespace opentoon
