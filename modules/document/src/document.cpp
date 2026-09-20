#include "opentoon/document.h"
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>

namespace opentoon {
namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::invalid_argument(message);
}
bool bounded(double v, double maximum = 1e7) {
    return std::isfinite(v) && std::abs(v) <= maximum;
}
double distance(Point a, Point b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}
double segmentDistance(Point p, Point a, Point b) {
    const double dx = b.x - a.x, dy = b.y - a.y, size = dx * dx + dy * dy;
    const double t = size > 0 ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / size, 0.0, 1.0) : 0;
    return distance(p, {a.x + t * dx, a.y + t * dy, 1});
}
void validateTransform(const Transform& t) {
    for (double v : {t.x, t.y, t.rotation, t.scaleX, t.scaleY, t.pivotX, t.pivotY})
        require(bounded(v), "Invalid transform value.");
    require(bounded(t.opacity, 1) && t.opacity >= 0, "Opacity must be between zero and one.");
}
} // namespace
void FrameRate::validate() const {
    require(numerator > 0 && numerator <= 240000 && denominator > 0 && denominator <= 10000,
            "Invalid rational frame rate.");
}
double FrameRate::seconds(Frame frame) const {
    validate();
    return static_cast<double>(frame) * denominator / numerator;
}
std::int64_t FrameRate::sampleAt(Frame frame, std::int32_t sampleRate) const {
    validate();
    require(frame >= 0 && frame <= 1000000 && sampleRate > 0 && sampleRate <= 384000,
            "Invalid sample conversion range.");
    return static_cast<std::int64_t>(frame) * denominator * sampleRate / numerator;
}
Layer& Document::layer(Id id) {
    for (auto& l : layers)
        if (l.id == id)
            return l;
    throw std::invalid_argument("Layer does not exist.");
}
const Layer& Document::layer(Id id) const {
    for (const auto& l : layers)
        if (l.id == id)
            return l;
    throw std::invalid_argument("Layer does not exist.");
}
const Drawing* Document::drawingAt(Id layerId, Frame frame) const {
    const auto& spans = layer(layerId).exposures;
    auto it = std::upper_bound(spans.begin(), spans.end(), frame,
                               [](Frame f, const Exposure& e) { return f < e.start; });
    if (it == spans.begin())
        return nullptr;
    --it;
    if (frame >= it->end || it->drawing == 0)
        return nullptr;
    auto drawing = drawings.find(it->drawing);
    return drawing == drawings.end() ? nullptr : &drawing->second;
}
Drawing& Document::editableDrawing(Id layerId, Frame frame) {
    auto& l = layer(layerId);
    require(!l.locked, "Unlock the layer before editing.");
    require(frame >= 0 && frame < duration, "Frame is outside the scene.");
    if (const auto* existing = drawingAt(layerId, frame))
        return drawings.at(existing->id);
    const Id id = allocateId();
    drawings.emplace(id, Drawing{id, "Drawing " + std::to_string(id), {}, std::nullopt});
    expose(l, frame, frame + 1, id);
    return drawings.at(id);
}
void expose(Layer& layer, Frame start, Frame end, Id drawing) {
    require(start >= 0 && end > start, "Exposure must have a positive range.");
    std::vector<Exposure> result;
    for (const auto& e : layer.exposures) {
        if (e.end <= start || e.start >= end)
            result.push_back(e);
        else {
            if (e.start < start)
                result.push_back({e.start, start, e.drawing});
            if (e.end > end)
                result.push_back({end, e.end, e.drawing});
        }
    }
    if (drawing)
        result.push_back({start, end, drawing});
    std::sort(result.begin(), result.end(), [](auto a, auto b) { return a.start < b.start; });
    layer.exposures.clear();
    for (auto e : result) {
        if (!layer.exposures.empty() && layer.exposures.back().end == e.start &&
            layer.exposures.back().drawing == e.drawing)
            layer.exposures.back().end = e.end;
        else
            layer.exposures.push_back(e);
    }
}
void Document::validate() const {
    rate.validate();
    require(width > 0 && width <= 8192 && height > 0 && height <= 8192,
            "Scene dimensions must be between 1 and 8192.");
    require(duration > 0 && duration <= 1000000, "Scene duration is outside supported limits.");
    require(name.size() <= 4096 && layers.size() <= 2000 && drawings.size() <= 50000 &&
                palette.size() <= 65536,
            "Document exceeds resource limits.");
    std::set<Id> ids, swatches;
    auto id = [&](Id value) {
        require(value != 0 && value < nextId && ids.insert(value).second,
                "Duplicate or invalid resource identity.");
    };
    auto color = [](Color c) {
        for (double v : {c.r, c.g, c.b, c.a})
            require(std::isfinite(v) && v >= 0 && v <= 1, "Invalid color component.");
    };
    color(background);
    for (const auto& s : palette) {
        id(s.id);
        swatches.insert(s.id);
        color(s.color);
        require(s.name.size() <= 4096, "Swatch name is too long.");
    }
    std::size_t totalPoints = 0, totalPixels = 0;
    for (const auto& [key, d] : drawings) {
        require(key == d.id, "Drawing map identity mismatch.");
        id(d.id);
        require(d.name.size() <= 4096, "Drawing name is too long.");
        for (const auto& s : d.strokes) {
            id(s.id);
            require(swatches.contains(s.swatch), "Stroke references a missing swatch.");
            require(s.width > 0 && s.width <= 4096 && bounded(s.width), "Invalid stroke width.");
            require(s.artLayer >= 0 && s.artLayer <= 3, "Invalid art layer.");
            require(static_cast<int>(s.shape) >= 0 && static_cast<int>(s.shape) <= 3,
                    "Unsupported stroke shape.");
            require(!s.points.empty(), "A stroke must contain at least one point.");
            totalPoints += s.points.size();
            require(totalPoints <= 2000000, "Document contains too many points.");
            for (auto p : s.points)
                require(bounded(p.x) && bounded(p.y) && std::isfinite(p.pressure) && p.pressure >= 0 &&
                            p.pressure <= 1,
                        "Invalid stroke sample.");
        }
        if (d.image) {
            const auto& image = *d.image;
            require(image.width > 0 && image.width <= 8192 && image.height > 0 && image.height <= 8192,
                    "Invalid image size.");
            const auto size = static_cast<std::size_t>(image.width) * image.height * 4;
            totalPixels += size;
            require(image.rgba.size() == size && totalPixels <= 256 * 1024 * 1024,
                    "Invalid image buffer or image memory limit exceeded.");
        }
    }
    for (const auto& l : layers) {
        id(l.id);
        require(l.name.size() <= 4096, "Layer name is too long.");
        validateTransform(l.transform);
        Frame last = 0;
        for (auto e : l.exposures) {
            require(e.start >= last && e.end > e.start && e.end <= duration,
                    "Invalid or overlapping exposure interval.");
            require(drawings.contains(e.drawing), "Exposure references a missing drawing.");
            last = e.end;
        }
        Frame previous = -1;
        for (auto k : l.keys) {
            require(k.frame > previous && k.frame < duration, "Invalid keyframe order or range.");
            validateTransform(k.value);
            require(static_cast<int>(k.interpolation) >= 0 && static_cast<int>(k.interpolation) <= 2,
                    "Unknown interpolation.");
            previous = k.frame;
        }
    }
    for (const auto& l : layers) {
        std::set<Id> chain{l.id};
        Id parent = l.parent;
        while (parent) {
            require(chain.insert(parent).second, "Layer hierarchy contains a cycle.");
            parent = layer(parent).parent;
        }
    }
    for (const auto& m : markers)
        require(m.frame >= 0 && m.frame < duration && m.name.size() <= 4096, "Invalid scene marker.");
}
Document makeDocument() {
    Document d;
    for (auto s : std::vector<Swatch>{{0, "Ink", {0.06, 0.06, 0.06, 1}},
                                      {0, "Paper", {1, 1, 1, 1}},
                                      {0, "Graphite", {0.45, 0.45, 0.45, 1}},
                                      {0, "Light gray", {0.8, 0.8, 0.8, 1}}}) {
        s.id = d.allocateId();
        d.palette.push_back(s);
    }
    Layer l;
    l.id = d.allocateId();
    l.name = "Drawing 1";
    d.layers.push_back(l);
    d.validate();
    return d;
}
Transform evaluateTransform(const Layer& layer, Frame frame) {
    if (layer.keys.empty())
        return layer.transform;
    auto right = std::upper_bound(layer.keys.begin(), layer.keys.end(), frame,
                                  [](Frame f, const Keyframe& k) { return f < k.frame; });
    if (right == layer.keys.begin())
        return right->value;
    auto left = right - 1;
    if (right == layer.keys.end() || left->interpolation == Interpolation::Step)
        return left->value;
    double t = static_cast<double>(frame - left->frame) / (right->frame - left->frame);
    if (left->interpolation == Interpolation::Smooth)
        t = t * t * (3 - 2 * t);
    Transform result;
    auto mix = [t](double a, double b) { return a + (b - a) * t; };
    result.x = mix(left->value.x, right->value.x);
    result.y = mix(left->value.y, right->value.y);
    result.rotation = mix(left->value.rotation, right->value.rotation);
    result.scaleX = mix(left->value.scaleX, right->value.scaleX);
    result.scaleY = mix(left->value.scaleY, right->value.scaleY);
    result.opacity = mix(left->value.opacity, right->value.opacity);
    result.pivotX = mix(left->value.pivotX, right->value.pivotX);
    result.pivotY = mix(left->value.pivotY, right->value.pivotY);
    return result;
}
void insertFrames(Document& d, Frame at, Frame count) {
    require(at >= 0 && at <= d.duration && count > 0 && count <= 1000000 - d.duration,
            "Invalid insertion range.");
    for (auto& l : d.layers) {
        for (auto& e : l.exposures) {
            if (e.start >= at) {
                e.start += count;
                e.end += count;
            } else if (e.end > at)
                e.end += count;
        }
        for (auto& k : l.keys)
            if (k.frame >= at)
                k.frame += count;
    }
    for (auto& m : d.markers)
        if (m.frame >= at)
            m.frame += count;
    d.duration += count;
}
void removeFrames(Document& d, Frame at, Frame count) {
    require(at >= 0 && count > 0 && count < d.duration && at + count <= d.duration, "Invalid removal range.");
    const Frame end = at + count;
    auto collapse = [=](Frame f) { return f <= at ? f : f < end ? at : f - count; };
    for (auto& l : d.layers) {
        std::vector<Exposure> spans;
        for (auto e : l.exposures) {
            e.start = collapse(e.start);
            e.end = collapse(e.end);
            if (e.end > e.start)
                spans.push_back(e);
        }
        l.exposures = spans;
        std::erase_if(l.keys, [=](auto k) { return k.frame >= at && k.frame < end; });
        for (auto& k : l.keys)
            k.frame = collapse(k.frame);
    }
    std::erase_if(d.markers, [=](const auto& m) { return m.frame >= at && m.frame < end; });
    for (auto& m : d.markers)
        m.frame = collapse(m.frame);
    d.duration -= count;
}
void eraseAt(Drawing& d, Point center, double radius, Id& nextId) {
    require(radius > 0 && std::isfinite(radius), "Invalid eraser radius.");
    std::vector<Stroke> result;
    for (const auto& original : d.strokes) {
        if (original.shape != Shape::Stroke) {
            bool hit = false;
            for (auto p : original.points)
                hit = hit || distance(center, p) <= radius;
            if (!hit)
                result.push_back(original);
            continue;
        }
        Stroke part = original;
        part.points.clear();
        bool changed = false;
        // Sample each segment before cutting so a fast gesture cannot skip a long segment.
        std::vector<Point> samples;
        for (std::size_t i = 0; i < original.points.size(); ++i) {
            if (i) {
                auto a = original.points[i - 1], b = original.points[i];
                const auto steps = std::clamp(
                    static_cast<int>(std::ceil(distance(a, b) / std::max(radius / 3, 0.5))), 1, 10000);
                for (int n = 1; n < steps; ++n) {
                    double t = double(n) / steps;
                    samples.push_back({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                                       a.pressure + (b.pressure - a.pressure) * t});
                }
            }
            samples.push_back(original.points[i]);
        }
        for (auto p : samples) {
            if (distance(center, p) <= radius + original.width * p.pressure / 2) {
                changed = true;
                if (!part.points.empty()) {
                    part.id = nextId++;
                    result.push_back(part);
                    part.points.clear();
                }
            } else
                part.points.push_back(p);
        }
        if (!changed) {
            result.push_back(original);
        } else if (!part.points.empty()) {
            part.id = nextId++;
            result.push_back(part);
        }
    }
    d.strokes = std::move(result);
}
std::optional<Id> hitStroke(const Drawing& d, Point p, double tolerance) {
    for (auto it = d.strokes.rbegin(); it != d.strokes.rend(); ++it) {
        if ((it->shape == Shape::Rectangle || it->shape == Shape::Ellipse) && it->points.size() >= 2) {
            const auto a = it->points.front(), b = it->points.back();
            const double left = std::min(a.x, b.x), right = std::max(a.x, b.x);
            const double top = std::min(a.y, b.y), bottom = std::max(a.y, b.y);
            if (p.x >= left - tolerance && p.x <= right + tolerance && p.y >= top - tolerance &&
                p.y <= bottom + tolerance) {
                if (it->shape == Shape::Rectangle)
                    return it->id;
                const double rx = (right - left) / 2 + tolerance, ry = (bottom - top) / 2 + tolerance;
                if (rx > 0 && ry > 0 &&
                    std::pow((p.x - (left + right) / 2) / rx, 2) +
                            std::pow((p.y - (top + bottom) / 2) / ry, 2) <=
                        1)
                    return it->id;
            }
        }
        for (std::size_t i = 0; i < it->points.size(); ++i) {
            double dist =
                i ? segmentDistance(p, it->points[i - 1], it->points[i]) : distance(p, it->points[i]);
            if (dist <= tolerance + it->width / 2)
                return it->id;
        }
    }
    return {};
}
Document makeBouncingBall() {
    Document d = makeDocument();
    d.name = "Bouncing ball";
    d.width = 1920;
    d.height = 1080;
    auto& l = d.layers.front();
    for (Frame f = 0; f < 48; f += 2) {
        Id id = d.allocateId();
        Drawing drawing{id, "Ball " + std::to_string(f / 2 + 1), {}, std::nullopt};
        double t = double(f) / 48;
        double bounce = std::abs(std::sin(t * 3.141592653589793 * 2));
        Stroke s;
        s.id = d.allocateId();
        s.swatch = d.palette.front().id;
        s.width = 5;
        s.shape = Shape::Ellipse;
        s.filled = false;
        s.points = {{240 + t * 1400, 860 - bounce * 600, 1}, {360 + t * 1400, 980 - bounce * 600, 1}};
        drawing.strokes.push_back(s);
        d.drawings.emplace(id, drawing);
        l.exposures.push_back({f, f + 2, id});
    }
    d.markers = {{0, "Start"}, {24, "Second bounce"}};
    d.validate();
    return d;
}
} // namespace opentoon
