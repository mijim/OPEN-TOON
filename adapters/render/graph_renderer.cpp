#include "graph_renderer.h"
#include <QColor>
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace opentoon {
namespace {
float linear(float value) {
    return value <= 0.04045f ? value / 12.92f : std::pow((value + 0.055f) / 1.055f, 2.4f);
}
float srgb(float value) {
    return value <= 0.0031308f ? value * 12.92f : 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
}
struct Pixel {
    float r = 0, g = 0, b = 0, a = 0;
};
Pixel decode(QRgb pixel) {
    const float alpha = qAlpha(pixel) / 255.0f;
    if (alpha <= 0)
        return {};
    auto channel = [alpha](int value) {
        return linear(std::clamp(value / (255.0f * alpha), 0.0f, 1.0f)) * alpha;
    };
    return {channel(qRed(pixel)), channel(qGreen(pixel)), channel(qBlue(pixel)), alpha};
}
QRgb encode(Pixel pixel) {
    const float alpha = std::clamp(pixel.a, 0.0f, 1.0f);
    if (alpha <= 0)
        return qRgba(0, 0, 0, 0);
    auto channel = [alpha](float value) {
        const float straight = std::clamp(value / alpha, 0.0f, 1.0f);
        return std::clamp(int(std::lround(srgb(straight) * alpha * 255)), 0, 255);
    };
    return qRgba(channel(pixel.r), channel(pixel.g), channel(pixel.b),
                 std::clamp(int(std::lround(alpha * 255)), 0, 255));
}
QImage over(const QImage& background, const QImage& foreground, CompositionProfile profile) {
    if (background.size() != foreground.size())
        throw std::invalid_argument("Compositor images have different sizes.");
    QImage result = background.copy();
    if (profile == CompositionProfile::LegacyQt) {
        QPainter painter(&result);
        painter.drawImage(0, 0, foreground);
        return result;
    }
    for (int y = 0; y < result.height(); ++y) {
        auto* target = reinterpret_cast<QRgb*>(result.scanLine(y));
        const auto* source = reinterpret_cast<const QRgb*>(foreground.constScanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            const auto front = decode(source[x]);
            const auto back = decode(target[x]);
            const float remaining = 1.0f - front.a;
            target[x] = encode({front.r + back.r * remaining, front.g + back.g * remaining,
                                front.b + back.b * remaining, front.a + back.a * remaining});
        }
    }
    return result;
}
QImage matteFromImage(const QImage& image) {
    QImage matte(image.size(), QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < matte.height(); ++y) {
        auto* target = reinterpret_cast<QRgb*>(matte.scanLine(y));
        const auto* source = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < matte.width(); ++x)
            target[x] = qRgba(0, 0, 0, qAlpha(source[x]));
    }
    return matte;
}
QImage applyMatte(const QImage& image, const QImage& matte) {
    if (image.size() != matte.size())
        throw std::invalid_argument("Matte and image sizes differ.");
    QImage result = image.copy();
    for (int y = 0; y < result.height(); ++y) {
        auto* target = reinterpret_cast<QRgb*>(result.scanLine(y));
        const auto* mask = reinterpret_cast<const QRgb*>(matte.constScanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            const int alpha = qAlpha(mask[x]);
            auto scaled = [alpha](int value) { return (value * alpha + 127) / 255; };
            target[x] = qRgba(scaled(qRed(target[x])), scaled(qGreen(target[x])),
                              scaled(qBlue(target[x])), scaled(qAlpha(target[x])));
        }
    }
    return result;
}
} // namespace
QImage GraphRenderer::render(const CompositionGraph& graph, const Document& document,
                             Frame frame, QSize size, RenderOptions options, GraphTarget target) {
    graph.validate(document);
    if (frame < 0 || frame >= document.duration)
        throw std::invalid_argument("Compositor frame is outside the scene.");
    if (size.isEmpty())
        size = {document.width, document.height};
    if (size.width() <= 0 || size.height() <= 0 || size.width() > 8192 || size.height() > 8192)
        throw std::invalid_argument("Invalid compositor output size.");
    const auto terminal = target == GraphTarget::Display ? graph.display : graph.write;
    const auto order = graph.topologicalOrder();
    std::map<GraphNodeId, const GraphNode*> nodes;
    for (const auto& node : graph.nodes) {
        nodes.emplace(node.id, &node);
    }
    std::set<GraphNodeId> needed;
    auto mark = [&](auto&& self, GraphNodeId id) -> void {
        if (!needed.insert(id).second)
            return;
        for (const auto& input : nodes.at(id)->inputs)
            self(self, input.source);
    };
    mark(mark, terminal);
    std::map<GraphNodeId, int> uses;
    for (const auto& node : graph.nodes) {
        if (!needed.contains(node.id))
            continue;
        for (const auto& input : node.inputs)
            ++uses[input.source];
    }
    std::map<GraphNodeId, QImage> images;
    Document legacy = document;
    legacy.composition = CompositionProfile::LegacyQt;
    for (const auto id : order) {
        if (!needed.contains(id))
            continue;
        const auto& node = *nodes.at(id);
        auto input = [&](int slot) -> const QImage& {
            for (const auto& connection : node.inputs)
                if (connection.slot == slot)
                    return images.at(connection.source);
            throw std::invalid_argument("Compositor input is missing.");
        };
        QImage image;
        switch (node.kind) {
        case GraphNodeKind::Background:
            image = QImage(size, QImage::Format_ARGB32_Premultiplied);
            image.fill(options.background ? QColor::fromRgbF(document.background.r,
                                                              document.background.g,
                                                              document.background.b,
                                                              document.background.a)
                                          : QColor(Qt::transparent));
            break;
        case GraphNodeKind::LayerImage:
            if (!options.isolatedLayer || options.isolatedLayer == node.layer) {
                auto isolated = options;
                isolated.background = false;
                isolated.isolatedLayer = node.layer;
                image = SceneRenderer::render(legacy, frame, size, isolated);
            } else {
                image = QImage(size, QImage::Format_ARGB32_Premultiplied);
                image.fill(Qt::transparent);
            }
            break;
        case GraphNodeKind::LayerTransform:
            (void)evaluatedTransform(node, document, frame);
            break;
        case GraphNodeKind::Over:
            image = over(input(0), input(1), document.composition);
            break;
        case GraphNodeKind::MatteFromImage:
            image = matteFromImage(input(0));
            break;
        case GraphNodeKind::ApplyMatte:
            image = applyMatte(input(0), input(1));
            break;
        case GraphNodeKind::DisplayOutput:
        case GraphNodeKind::WriteOutput:
            image = input(0);
            break;
        }
        if (!image.isNull())
            images.emplace(id, std::move(image));
        for (const auto& connection : node.inputs)
            if (--uses.at(connection.source) == 0 && connection.source != terminal)
                images.erase(connection.source);
    }
    return images.at(terminal);
}
Transform GraphRenderer::evaluatedTransform(const GraphNode& node, const Document& document,
                                            Frame frame) {
    if (node.kind != GraphNodeKind::LayerTransform || frame < 0 || frame >= document.duration)
        throw std::invalid_argument("Select an evaluated transform node and a valid frame.");
    return evaluateTransform(document.layer(node.layer), frame);
}
} // namespace opentoon
