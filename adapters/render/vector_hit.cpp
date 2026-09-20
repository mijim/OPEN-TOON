#include "vector_hit.h"
#include <QPainterPath>
#include <QPainterPathStroker>
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
