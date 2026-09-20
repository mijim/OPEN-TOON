#include "scene_renderer.h"
#include <QPainterPath>
#include <cmath>
namespace opentoon {
namespace {
QColor qtColor(Color c) {
    return QColor::fromRgbF(c.r, c.g, c.b, c.a);
}
QTransform localTransform(Transform t) {
    QTransform m;
    m.translate(t.x + t.pivotX, t.y + t.pivotY);
    m.rotate(t.rotation);
    m.scale(t.scaleX, t.scaleY);
    m.translate(-t.pivotX, -t.pivotY);
    return m;
}
void drawing(QPainter& painter, const Drawing& d, const std::vector<Swatch>& palette) {
    if (d.image) {
        const auto& im = *d.image;
        QImage image(im.rgba.data(), im.width, im.height, im.width * 4, QImage::Format_RGBA8888);
        painter.drawImage(QPointF(0, 0), image);
    }
    for (int art = 0; art < 4; ++art)
        for (const auto& s : d.strokes)
            if (s.artLayer == art)
                SceneRenderer::paintStroke(painter, s, palette);
}
} // namespace
QTransform SceneRenderer::worldTransform(const Document& d, const Layer& layer, Frame frame) {
    QTransform result = localTransform(evaluateTransform(layer, frame));
    Id parent = layer.parent;
    std::size_t depth = 0;
    while (parent && depth++ < d.layers.size()) {
        const auto& p = d.layer(parent);
        result = result * localTransform(evaluateTransform(p, frame));
        parent = p.parent;
    }
    return result;
}
void SceneRenderer::paintStroke(QPainter& painter, const Stroke& s, const std::vector<Swatch>& palette,
                                double opacity) {
    if (s.points.empty())
        return;
    QColor color = Qt::black;
    for (const auto& swatch : palette)
        if (swatch.id == s.swatch) {
            color = qtColor(swatch.color);
            break;
        }
    color.setAlphaF(color.alphaF() * opacity);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(s.filled ? QBrush(color) : Qt::NoBrush);
    painter.setPen(QPen(color, s.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (s.shape == Shape::Ellipse || s.shape == Shape::Rectangle) {
        if (s.points.size() > 1) {
            QRectF bounds(QPointF(s.points.front().x, s.points.front().y),
                          QPointF(s.points.back().x, s.points.back().y));
            if (s.shape == Shape::Ellipse)
                painter.drawEllipse(bounds.normalized());
            else
                painter.drawRect(bounds.normalized());
        }
    } else if (s.shape == Shape::Polygon) {
        QPainterPath path;
        path.moveTo(s.points.front().x, s.points.front().y);
        for (auto p : s.points)
            path.lineTo(p.x, p.y);
        path.closeSubpath();
        painter.drawPath(path);
    } else {
        if (s.points.size() == 1) {
            const auto p = s.points.front();
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            double r = s.width * std::max(0.05, p.pressure) / 2;
            painter.drawEllipse(QPointF(p.x, p.y), r, r);
        }
        for (std::size_t i = 1; i < s.points.size(); ++i) {
            auto a = s.points[i - 1], b = s.points[i];
            const auto width = s.width * std::max(0.05, (a.pressure + b.pressure) / 2);
            painter.setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawLine(QPointF(a.x, a.y), QPointF(b.x, b.y));
        }
    }
    painter.restore();
}
void SceneRenderer::paint(QPainter& painter, const Document& d, Frame frame, RenderOptions options) {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setClipRect(QRectF(0, 0, d.width, d.height));
    if (options.background)
        painter.fillRect(QRectF(0, 0, d.width, d.height), qtColor(d.background));
    bool solo = std::any_of(d.layers.begin(), d.layers.end(), [](const Layer& l) { return l.solo; });
    for (const auto& l : d.layers) {
        if (!l.visible || (options.isolatedLayer && l.id != options.isolatedLayer))
            continue;
        double opacity = evaluateTransform(l, frame).opacity;
        bool visible = true, inSoloBranch = l.solo;
        Id parent = l.parent;
        std::size_t depth = 0;
        while (parent && depth++ < d.layers.size()) {
            const auto& p = d.layer(parent);
            visible = visible && p.visible;
            inSoloBranch = inSoloBranch || p.solo;
            opacity *= evaluateTransform(p, frame).opacity;
            parent = p.parent;
        }
        if (!visible || (solo && !inSoloBranch))
            continue;
        painter.save();
        painter.setWorldTransform(worldTransform(d, l, frame), true);
        painter.setOpacity(opacity);
        if (options.onionSkin) {
            const Drawing* current = d.drawingAt(l.id, frame);
            std::vector<Id> shown;
            if (current)
                shown.push_back(current->id);
            for (int dir : {-1, 1}) {
                int count = 0;
                for (Frame f = frame + dir; f >= 0 && f < d.duration && count < options.onionRange;
                     f += dir) {
                    const auto* ghost = d.drawingAt(l.id, f);
                    if (!ghost || std::find(shown.begin(), shown.end(), ghost->id) != shown.end())
                        continue;
                    shown.push_back(ghost->id);
                    ++count;
                    painter.setOpacity(opacity * 0.15 / (count));
                    drawing(painter, *ghost, d.palette);
                }
            }
            painter.setOpacity(opacity);
        }
        if (const auto* current = d.drawingAt(l.id, frame))
            drawing(painter, *current, d.palette);
        painter.restore();
    }
    painter.restore();
}
QImage SceneRenderer::render(const Document& d, Frame frame, QSize size, RenderOptions options) {
    if (size.isEmpty())
        size = QSize(d.width, d.height);
    if (size.width() > 8192 || size.height() > 8192 || size.width() <= 0 || size.height() <= 0)
        throw std::invalid_argument("Invalid render dimensions.");
    QImage result(size, QImage::Format_ARGB32_Premultiplied);
    if (result.isNull())
        throw std::runtime_error("Unable to allocate render surface.");
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.scale(double(size.width()) / d.width, double(size.height()) / d.height);
    paint(painter, d, frame, options);
    return result;
}
} // namespace opentoon
