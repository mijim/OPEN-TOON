#include "scene_renderer.h"
#include "graph_renderer.h"
#include <QPainterPath>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
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
QImage rasterTile(const SharedBuffer<std::uint16_t>& tile) {
    struct Entry {
        SharedBuffer<std::uint16_t> source;
        QImage image;
    };
    // Each render worker owns its cache. Retaining the immutable source prevents
    // allocator address reuse from aliasing a different tile.
    thread_local std::unordered_map<const std::uint16_t*, Entry> cache;
    if (auto found = cache.find(tile.data()); found != cache.end())
        return found->second.image;
    if (cache.size() >= 2048)
        cache.clear(); // <=96 MiB including retained source data.
    QImage image(64, 64, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < 64; ++y) {
        auto* row = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < 64; ++x) {
            auto i = std::size_t((y * 64 + x) * 4);
            auto channel = [&](int n) { return (std::uint32_t(tile[i + n]) * 255 + 16384) / 32768; };
            row[x] = qRgba(channel(0), channel(1), channel(2), channel(3));
        }
    }
    cache.emplace(tile.data(), Entry{tile, image});
    return image;
}
void drawing(QPainter& painter, const Drawing& d, const std::vector<Swatch>& palette) {
    if (d.image) {
        const auto& im = *d.image;
        QImage image(im.rgba.data(), im.width, im.height, im.width * 4, QImage::Format_RGBA8888);
        painter.drawImage(QPointF(0, 0), image);
    }
    if (d.raster) {
        painter.save();
        // Antialiasing tile rectangles creates translucent seams at fractional zoom.
        // The brush pixels already contain their own antialiased coverage.
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setClipRect(QRect(0, 0, d.raster->width, d.raster->height), Qt::IntersectClip);
        for (const auto& [key, tile] : d.raster->tiles)
            painter.drawImage(QPoint(key.first * 64, key.second * 64), rasterTile(tile));
        painter.restore();
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
    if (d.composition == CompositionProfile::LinearSrgb) {
        painter.save();
        painter.setClipRect(QRectF(0, 0, d.width, d.height));
        painter.drawImage(QPointF(0, 0),
                          GraphRenderer::render(CompositionGraph::orderedLayers(d), d, frame,
                                                {d.width, d.height}, options, GraphTarget::Display));
        painter.restore();
        return;
    }
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
        const auto* current = options.previewDrawing && options.previewLayer == l.id
                                  ? options.previewDrawing
                                  : d.drawingAt(l.id, frame);
        if (current)
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
    if (d.composition == CompositionProfile::LinearSrgb)
        return GraphRenderer::render(CompositionGraph::orderedLayers(d), d, frame, size,
                                     options, GraphTarget::Write);
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
