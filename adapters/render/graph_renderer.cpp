#include "graph_renderer.h"
#include <QColor>
#include <QPainter>
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

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
const std::array<float, 256>& decodeTable() {
    static const auto table = [] {
        std::array<float, 256> values{};
        for (int i = 0; i < 256; ++i)
            values[i] = linear(i / 255.0f);
        return values;
    }();
    return table;
}
const std::array<float, 65536>& encodeTable() {
    static const auto table = [] {
        std::array<float, 65536> values{};
        for (int i = 0; i < 65536; ++i)
            values[i] = srgb(i / 65535.0f);
        return values;
    }();
    return table;
}
Pixel decode(QRgb pixel) {
    const float alpha = qAlpha(pixel) / 255.0f;
    if (alpha <= 0)
        return {};
    auto channel = [alpha](int value) {
        const int straight = std::clamp(int(std::lround(value / alpha)), 0, 255);
        return decodeTable()[straight] * alpha;
    };
    return {channel(qRed(pixel)), channel(qGreen(pixel)), channel(qBlue(pixel)), alpha};
}
QRgb encode(Pixel pixel) {
    const float alpha = std::clamp(pixel.a, 0.0f, 1.0f);
    if (alpha <= 0)
        return qRgba(0, 0, 0, 0);
    auto channel = [alpha](float value) {
        const float straight = std::clamp(value / alpha, 0.0f, 1.0f);
        const int index = std::clamp(int(std::lround(straight * 65535)), 0, 65535);
        return std::clamp(int(std::lround(encodeTable()[index] * alpha * 255)), 0, 255);
    };
    return qRgba(channel(pixel.r), channel(pixel.g), channel(pixel.b),
                 std::clamp(int(std::lround(alpha * 255)), 0, 255));
}
void checkCancelled(const RenderOptions& options) {
    if (options.cancelled && options.cancelled())
        throw RenderCancelled();
}
QImage over(const QImage& background, const QImage& foreground, CompositionProfile profile,
            const RenderOptions& options, QRect foregroundBounds) {
    if (background.size() != foreground.size())
        throw std::invalid_argument("Compositor images have different sizes.");
    if (profile == CompositionProfile::LegacyQt) {
        QImage result = background.copy();
        QPainter painter(&result);
        painter.drawImage(0, 0, foreground);
        return result;
    }
    foregroundBounds = foregroundBounds.intersected(QRect(QPoint(0, 0), foreground.size()));
    if (foregroundBounds.isEmpty())
        return background;
    QImage result = background.copy();
    for (int y = foregroundBounds.top(); y <= foregroundBounds.bottom(); ++y) {
        checkCancelled(options);
        auto* target = reinterpret_cast<QRgb*>(result.scanLine(y));
        const auto* source = reinterpret_cast<const QRgb*>(foreground.constScanLine(y));
        for (int x = foregroundBounds.left(); x <= foregroundBounds.right(); ++x) {
            if (qAlpha(source[x]) == 0) {
                if (qAlpha(target[x]) == 0)
                    target[x] = 0;
                continue;
            }
            if (qAlpha(source[x]) == 255 || qAlpha(target[x]) == 0) {
                target[x] = source[x];
                continue;
            }
            const auto front = decode(source[x]);
            const auto back = decode(target[x]);
            const float remaining = 1.0f - front.a;
            target[x] = encode({front.r + back.r * remaining, front.g + back.g * remaining,
                                front.b + back.b * remaining, front.a + back.a * remaining});
        }
    }
    return result;
}
QImage blendImages(const QImage& background, const QImage& foreground,
                   CompositionProfile profile, GraphNodeKind mode,
                   const RenderOptions& options, QRect foregroundBounds) {
    if (background.size() != foreground.size())
        throw std::invalid_argument("Compositor images have different sizes.");
    foregroundBounds = foregroundBounds.intersected(QRect(QPoint(0, 0), foreground.size()));
    if (foregroundBounds.isEmpty())
        return background;
    QImage result = background.copy();
    const auto unpack = [profile](QRgb value) {
        if (profile == CompositionProfile::LinearSrgb)
            return decode(value);
        constexpr float unit = 1.0f / 255.0f;
        return Pixel{qRed(value) * unit, qGreen(value) * unit,
                     qBlue(value) * unit, qAlpha(value) * unit};
    };
    const auto pack = [profile](Pixel value) {
        if (profile == CompositionProfile::LinearSrgb)
            return encode(value);
        const auto channel = [](float component) {
            return std::clamp(int(std::lround(component * 255.0f)), 0, 255);
        };
        return qRgba(channel(value.r), channel(value.g), channel(value.b), channel(value.a));
    };
    const auto blended = [mode](float front, float back) {
        if (mode == GraphNodeKind::Multiply)
            return front * back;
        if (mode == GraphNodeKind::Add)
            return std::min(1.0f, front + back);
        return front + back - front * back;
    };
    for (int y = foregroundBounds.top(); y <= foregroundBounds.bottom(); ++y) {
        checkCancelled(options);
        auto* target = reinterpret_cast<QRgb*>(result.scanLine(y));
        const auto* source = reinterpret_cast<const QRgb*>(foreground.constScanLine(y));
        for (int x = foregroundBounds.left(); x <= foregroundBounds.right(); ++x) {
            if (qAlpha(source[x]) == 0)
                continue;
            if (qAlpha(target[x]) == 0) {
                target[x] = source[x];
                continue;
            }
            const Pixel back = unpack(target[x]);
            const Pixel front = unpack(source[x]);
            const float overlap = front.a * back.a;
            const auto color = [&](float frontPremul, float backPremul) {
                return frontPremul * (1 - back.a) + backPremul * (1 - front.a) +
                       overlap * blended(frontPremul / front.a, backPremul / back.a);
            };
            target[x] = pack({color(front.r, back.r), color(front.g, back.g),
                              color(front.b, back.b),
                              front.a + back.a - overlap});
        }
    }
    return result;
}
QImage matteFromImage(const QImage& image, const RenderOptions& options) {
    QImage matte(image.size(), QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < matte.height(); ++y) {
        checkCancelled(options);
        auto* target = reinterpret_cast<QRgb*>(matte.scanLine(y));
        const auto* source = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < matte.width(); ++x)
            target[x] = qRgba(0, 0, 0, qAlpha(source[x]));
    }
    return matte;
}
QImage invertMatte(const QImage& matte, const RenderOptions& options) {
    QImage result(matte.size(), QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < result.height(); ++y) {
        checkCancelled(options);
        auto* target = reinterpret_cast<QRgb*>(result.scanLine(y));
        const auto* source = reinterpret_cast<const QRgb*>(matte.constScanLine(y));
        for (int x = 0; x < result.width(); ++x)
            target[x] = qRgba(0, 0, 0, 255 - qAlpha(source[x]));
    }
    return result;
}
QImage applyMatte(const QImage& image, const QImage& matte, const RenderOptions& options) {
    if (image.size() != matte.size())
        throw std::invalid_argument("Matte and image sizes differ.");
    QImage result = image.copy();
    for (int y = 0; y < result.height(); ++y) {
        checkCancelled(options);
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
QImage applyOpacity(const QImage& image, double opacity, QRect bounds,
                    const RenderOptions& options) {
    if (opacity >= 1)
        return image;
    if (opacity <= 0) {
        QImage empty(image.size(), QImage::Format_ARGB32_Premultiplied);
        empty.fill(Qt::transparent);
        return empty;
    }
    bounds = bounds.intersected(QRect(QPoint(0, 0), image.size()));
    QImage result = image.copy();
    for (int y = bounds.top(); y <= bounds.bottom(); ++y) {
        checkCancelled(options);
        auto* row = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = bounds.left(); x <= bounds.right(); ++x) {
            const auto scale = [opacity](int channel) {
                return std::clamp(int(std::lround(channel * opacity)), 0, 255);
            };
            row[x] = qRgba(scale(qRed(row[x])), scale(qGreen(row[x])),
                            scale(qBlue(row[x])), scale(qAlpha(row[x])));
        }
    }
    return result;
}
QImage renderGraphTerminal(const CompositionGraph& graph, const Document& document,
                           Frame frame, QSize size, RenderOptions options,
                           GraphNodeId terminal) {
    graph.validate(document);
    checkCancelled(options);
    if (frame < 0 || frame >= document.duration)
        throw std::invalid_argument("Compositor frame is outside the scene.");
    if (size.isEmpty())
        size = {document.width, document.height};
    if (size.width() <= 0 || size.height() <= 0 || size.width() > 8192 || size.height() > 8192)
        throw std::invalid_argument("Invalid compositor output size.");
    const auto order = graph.topologicalOrder();
    std::map<GraphNodeId, const GraphNode*> nodes;
    for (const auto& node : graph.nodes) {
        nodes.emplace(node.id, &node);
    }
    if (!nodes.contains(terminal) || nodes.at(terminal)->kind == GraphNodeKind::LayerTransform)
        throw std::invalid_argument("Select an image or matte compositor node.");
    std::set<GraphNodeId> needed;
    std::vector<GraphNodeId> pending{terminal};
    while (!pending.empty()) {
        const auto id = pending.back();
        pending.pop_back();
        if (!needed.insert(id).second)
            continue;
        for (const auto& input : nodes.at(id)->inputs)
            pending.push_back(input.source);
    }
    std::map<GraphNodeId, int> uses;
    for (const auto& node : graph.nodes) {
        if (!needed.contains(node.id))
            continue;
        for (const auto& input : node.inputs)
            ++uses[input.source];
    }
    std::map<GraphNodeId, QImage> images;
    std::map<GraphNodeId, QRect> bounds;
    Document legacy = document;
    legacy.composition = CompositionProfile::LegacyQt;
    for (auto& layer : legacy.layers) {
        layer.matte = 0; // Isolated source render must not re-enter the graph.
        layer.opacityBypassed = false;
        layer.blendMode = LayerBlendMode::Normal;
        layer.blendBypassed = false;
    }
    for (const auto& node : graph.nodes)
        if (node.kind == GraphNodeKind::Opacity ||
            node.kind == GraphNodeKind::BypassOpacity) {
            auto& layer = legacy.layer(node.layer);
            layer.transform.opacity = 1;
            for (auto& key : layer.keys)
                key.value.opacity = 1;
        }
    for (const auto id : order) {
        checkCancelled(options);
        if (!needed.contains(id))
            continue;
        const auto& node = *nodes.at(id);
        auto input = [&](int slot) -> const QImage& {
            for (const auto& connection : node.inputs)
                if (connection.slot == slot)
                    return images.at(connection.source);
            throw std::invalid_argument("Compositor input is missing.");
        };
        auto inputBounds = [&](int slot) -> QRect {
            for (const auto& connection : node.inputs)
                if (connection.slot == slot)
                    return bounds.at(connection.source);
            throw std::invalid_argument("Compositor input is missing.");
        };
        QImage image;
        QRect bound;
        switch (node.kind) {
        case GraphNodeKind::Background:
            image = QImage(size, QImage::Format_ARGB32_Premultiplied);
            image.fill(options.background ? QColor::fromRgbF(document.background.r,
                                                              document.background.g,
                                                              document.background.b,
                                                              document.background.a)
                                          : QColor(Qt::transparent));
            if (options.background && document.background.a > 0)
                bound = QRect(QPoint(0, 0), size);
            break;
        case GraphNodeKind::LayerImage:
            if (!options.isolatedLayer || options.isolatedLayer == node.layer ||
                document.layer(options.isolatedLayer).matte == node.layer) {
                auto isolated = options;
                isolated.background = false;
                isolated.isolatedLayer = node.layer;
                image = SceneRenderer::render(legacy, frame, size, isolated);
                bound = SceneRenderer::layerInkBounds(document, document.layer(node.layer),
                                                     frame, size, isolated);
            } else {
                image = QImage(size, QImage::Format_ARGB32_Premultiplied);
                image.fill(Qt::transparent);
            }
            break;
        case GraphNodeKind::LayerTransform:
            (void)GraphRenderer::evaluatedTransform(node, document, frame);
            break;
        case GraphNodeKind::Opacity: {
            const double opacity = evaluateTransform(document.layer(node.layer), frame).opacity;
            image = applyOpacity(input(0), opacity, inputBounds(0), options);
            bound = opacity > 0 ? inputBounds(0) : QRect{};
            break;
        }
        case GraphNodeKind::BypassOpacity:
            image = input(0);
            bound = inputBounds(0);
            break;
        case GraphNodeKind::Over:
        case GraphNodeKind::BypassBlend:
            image = over(input(0), input(1), document.composition, options, inputBounds(1));
            bound = inputBounds(0).united(inputBounds(1));
            break;
        case GraphNodeKind::Multiply:
        case GraphNodeKind::Screen:
        case GraphNodeKind::Add:
            image = blendImages(input(0), input(1), document.composition, node.kind,
                                options, inputBounds(1));
            bound = inputBounds(0).united(inputBounds(1));
            break;
        case GraphNodeKind::MatteFromImage:
            image = matteFromImage(input(0), options);
            bound = inputBounds(0);
            break;
        case GraphNodeKind::InvertMatte:
            image = invertMatte(input(0), options);
            bound = QRect(QPoint(0, 0), size);
            break;
        case GraphNodeKind::ApplyMatte:
            image = applyMatte(input(0), input(1), options);
            bound = inputBounds(0).intersected(inputBounds(1));
            break;
        case GraphNodeKind::BypassMatte:
            image = input(0);
            bound = inputBounds(0);
            break;
        case GraphNodeKind::DisplayOutput:
        case GraphNodeKind::WriteOutput:
            image = input(0);
            bound = inputBounds(0);
            break;
        }
        if (!image.isNull()) {
            images.emplace(id, std::move(image));
            bounds.emplace(id, bound);
        }
        for (const auto& connection : node.inputs)
            if (--uses.at(connection.source) == 0 && connection.source != terminal) {
                images.erase(connection.source);
                bounds.erase(connection.source);
            }
    }
    checkCancelled(options);
    return images.at(terminal);
}
} // namespace
QImage GraphRenderer::render(const CompositionGraph& graph, const Document& document,
                             Frame frame, QSize size, RenderOptions options, GraphTarget target) {
    return renderGraphTerminal(graph, document, frame, size, std::move(options),
                               target == GraphTarget::Display ? graph.display : graph.write);
}
QImage GraphRenderer::renderNode(const CompositionGraph& graph, const Document& document,
                                 Frame frame, GraphNodeId node, QSize size,
                                 RenderOptions options) {
    return renderGraphTerminal(graph, document, frame, size, std::move(options), node);
}
Transform GraphRenderer::evaluatedTransform(const GraphNode& node, const Document& document,
                                            Frame frame) {
    if (node.kind != GraphNodeKind::LayerTransform || frame < 0 || frame >= document.duration)
        throw std::invalid_argument("Select an evaluated transform node and a valid frame.");
    return evaluateTransform(document.layer(node.layer), frame);
}
} // namespace opentoon
