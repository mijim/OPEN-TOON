#include "opentoon/vector_edit.h"
#include <cmath>
#include <set>
#include <stdexcept>
namespace opentoon {
namespace {
std::set<Id> checked(const Drawing& drawing, const std::vector<Id>& ids) {
    std::set<Id> selected(ids.begin(), ids.end());
    if (selected.empty())
        throw std::invalid_argument("Select vector objects first.");
    std::set<Id> available;
    for (const auto& s : drawing.strokes)
        available.insert(s.id);
    for (auto id : selected)
        if (!available.contains(id))
            throw std::invalid_argument("A selected vector no longer exists.");
    return selected;
}
void translate(Stroke& s, double x, double y) {
    for (auto& p : s.points) {
        p.x += x;
        p.y += y;
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || std::abs(p.x) > 1e7 || std::abs(p.y) > 1e7)
            throw std::invalid_argument("Vectors would leave the supported coordinate range.");
    }
}
} // namespace
VectorBlock copyVectors(const Document& doc, Id layer, Frame frame, const std::vector<Id>& ids) {
    const auto* drawing = doc.drawingAt(layer, frame);
    if (!drawing)
        throw std::invalid_argument("No drawing to copy at this frame.");
    const auto selected = checked(*drawing, ids);
    VectorBlock block;
    std::set<Id> colors;
    for (const auto& s : drawing->strokes)
        if (selected.contains(s.id)) {
            block.strokes.push_back(s);
            colors.insert(s.swatch);
        }
    for (const auto& swatch : doc.palette)
        if (colors.contains(swatch.id))
            block.palette.push_back(swatch);
    return block;
}
std::vector<Id> pasteVectors(Document& doc, Id layer, Frame frame, const VectorBlock& block, double dx,
                             double dy) {
    if (block.strokes.empty() || block.strokes.size() > 100000 || !std::isfinite(dx) || !std::isfinite(dy))
        throw std::invalid_argument("Copy vectors before pasting a valid selection.");
    auto candidate = doc;
    auto& drawing = candidate.editableDrawing(layer, frame);
    std::map<Id, Id> colors;
    for (const auto& source : block.palette) {
        auto found = std::find_if(candidate.palette.begin(), candidate.palette.end(), [&](const auto& s) {
            return s.name == source.name && s.color == source.color;
        });
        if (found != candidate.palette.end())
            colors[source.id] = found->id;
        else {
            const auto id = candidate.allocateId();
            candidate.palette.push_back({id, source.name, source.color});
            colors[source.id] = id;
        }
    }
    std::vector<Id> result;
    for (auto stroke : block.strokes) {
        if (!colors.contains(stroke.swatch))
            throw std::invalid_argument("The vector clipboard is missing a palette color.");
        stroke.swatch = colors.at(stroke.swatch);
        stroke.id = candidate.allocateId();
        translate(stroke, dx, dy);
        result.push_back(stroke.id);
        drawing.strokes.push_back(std::move(stroke));
    }
    candidate.validate();
    doc = std::move(candidate);
    return result;
}
void arrangeVectors(Drawing& drawing, const std::vector<Id>& ids, VectorLayout layout) {
    const auto selected = checked(drawing, ids);
    if (int(layout) < 0 || int(layout) > 7 || selected.size() < (int(layout) >= 6 ? 3u : 2u))
        throw std::invalid_argument("Alignment needs two vectors; distribution needs three.");
    auto result = drawing;
    const auto group = *strokeSelectionBounds(drawing, ids);
    std::vector<std::pair<std::size_t, double>> centers;
    const bool horizontal = layout == VectorLayout::DistributeX;
    for (std::size_t i = 0; i < result.strokes.size(); ++i) {
        auto& stroke = result.strokes[i];
        if (!selected.contains(stroke.id))
            continue;
        const auto r = *strokeBounds(stroke);
        double dx = 0, dy = 0;
        switch (layout) {
        case VectorLayout::Left:
            dx = group.x - r.x;
            break;
        case VectorLayout::CenterX:
            dx = group.x + group.width / 2. - r.x - r.width / 2.;
            break;
        case VectorLayout::Right:
            dx = group.x + group.width - r.x - r.width;
            break;
        case VectorLayout::Top:
            dy = group.y - r.y;
            break;
        case VectorLayout::CenterY:
            dy = group.y + group.height / 2. - r.y - r.height / 2.;
            break;
        case VectorLayout::Bottom:
            dy = group.y + group.height - r.y - r.height;
            break;
        default:
            centers.emplace_back(i, horizontal ? r.x + r.width / 2. : r.y + r.height / 2.);
            break;
        }
        translate(stroke, dx, dy);
    }
    if (!centers.empty()) {
        std::stable_sort(centers.begin(), centers.end(), [](auto a, auto b) { return a.second < b.second; });
        for (std::size_t i = 1; i + 1 < centers.size(); ++i) {
            const double target = centers.front().second +
                                  (centers.back().second - centers.front().second) * i / (centers.size() - 1);
            const double delta = target - centers[i].second;
            translate(result.strokes[centers[i].first], horizontal ? delta : 0, horizontal ? 0 : delta);
        }
    }
    drawing = std::move(result);
}
void orderVectors(Drawing& drawing, const std::vector<Id>& ids, VectorOrder order) {
    const auto selected = checked(drawing, ids);
    if (int(order) < 0 || int(order) > 3)
        throw std::invalid_argument("Invalid vector stacking operation.");
    auto result = drawing;
    for (int art = 0; art < 4; ++art) {
        std::vector<std::size_t> slots;
        std::vector<Stroke> strokes;
        for (std::size_t i = 0; i < drawing.strokes.size(); ++i)
            if (drawing.strokes[i].artLayer == art) {
                slots.push_back(i);
                strokes.push_back(drawing.strokes[i]);
            }
        auto chosen = [&](const auto& s) { return selected.contains(s.id); };
        if (order == VectorOrder::Back || order == VectorOrder::Front)
            std::stable_partition(strokes.begin(), strokes.end(), [&](const auto& s) {
                return order == VectorOrder::Back ? chosen(s) : !chosen(s);
            });
        else if (order == VectorOrder::Backward) {
            for (std::size_t i = 1; i < strokes.size(); ++i)
                if (chosen(strokes[i]) && !chosen(strokes[i - 1]))
                    std::swap(strokes[i], strokes[i - 1]);
        } else {
            for (std::size_t i = strokes.size(); i > 1; --i)
                if (chosen(strokes[i - 2]) && !chosen(strokes[i - 1]))
                    std::swap(strokes[i - 2], strokes[i - 1]);
        }
        for (std::size_t i = 0; i < slots.size(); ++i)
            result.strokes[slots[i]] = std::move(strokes[i]);
    }
    drawing = std::move(result);
}
void styleVectors(Drawing& drawing, const std::vector<Id>& ids, std::string_view property, double value) {
    const auto selected = checked(drawing, ids);
    if (!std::isfinite(value) ||
        (property == "strokeWidth" ? value < .01 || value > 4096
         : property == "artLayer"  ? value < 0 || value > 3 || value != std::floor(value)
         : property == "swatch"    ? value < 1 || value > 1e9 || value != std::floor(value)
         : property == "filled"    ? value != 0 && value != 1
                                   : true))
        throw std::invalid_argument("Invalid vector style value.");
    for (auto& stroke : drawing.strokes)
        if (selected.contains(stroke.id)) {
            if (property == "strokeWidth")
                stroke.width = value;
            else if (property == "artLayer")
                stroke.artLayer = int(value);
            else if (property == "swatch")
                stroke.swatch = Id(value);
            else
                stroke.filled = value != 0;
        }
}
void smoothVectors(Drawing& drawing, const std::vector<Id>& ids, double strength) {
    const auto selected = checked(drawing, ids);
    if (!std::isfinite(strength) || strength < 0 || strength > 1)
        throw std::invalid_argument("Smoothing strength must be between 0 and 1.");
    for (auto& stroke : drawing.strokes) {
        if (!selected.contains(stroke.id) || stroke.shape != Shape::Stroke)
            continue;
        const auto source = stroke.points;
        for (std::size_t i = 1; i + 1 < source.size(); ++i) {
            const auto a = source[i - 1], b = source[i], c = source[i + 1];
            const double ux = b.x - a.x, uy = b.y - a.y, vx = c.x - b.x, vy = c.y - b.y;
            const double length = std::hypot(ux, uy) * std::hypot(vx, vy);
            if (length < 1e-12 || (ux * vx + uy * vy) / length < .5)
                continue; // Protect corners with at least 60 degrees of turn.
            stroke.points[i].x += strength * ((a.x + c.x) / 2 - b.x);
            stroke.points[i].y += strength * ((a.y + c.y) / 2 - b.y);
        }
    }
}
void simplifyVectors(Drawing& drawing, const std::vector<Id>& ids, double tolerance) {
    const auto selected = checked(drawing, ids);
    if (!std::isfinite(tolerance) || tolerance < .01 || tolerance > 100)
        throw std::invalid_argument("Simplify tolerance must be between 0.01 and 100 local pixels.");
    auto result = drawing;
    std::size_t budget = 5000000;
    for (auto& stroke : result.strokes) {
        if (!selected.contains(stroke.id) || stroke.shape != Shape::Stroke || stroke.points.size() < 3)
            continue;
        const auto& points = stroke.points;
        std::vector<bool> keep(points.size());
        keep.front() = keep.back() = true;
        std::vector<std::pair<std::size_t, std::size_t>> stack{{0, points.size() - 1}};
        while (!stack.empty()) {
            const auto [first, last] = stack.back();
            stack.pop_back();
            const auto a = points[first], b = points[last];
            const double dx = b.x - a.x, dy = b.y - a.y, length = dx * dx + dy * dy;
            double maximum = 1;
            std::size_t split = first;
            for (auto i = first + 1; i < last; ++i) {
                if (budget-- == 0)
                    throw std::runtime_error("Simplification work limit reached. Select fewer strokes.");
                const auto p = points[i];
                const double t =
                    length < 1e-12 ? 0 : std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / length, 0., 1.);
                const double error =
                    std::max(std::hypot(p.x - a.x - t * dx, p.y - a.y - t * dy) / tolerance,
                             std::abs(p.pressure - a.pressure - t * (b.pressure - a.pressure)) / .02);
                if (error > maximum) {
                    maximum = error;
                    split = i;
                }
            }
            if (split != first) {
                keep[split] = true;
                stack.emplace_back(first, split);
                stack.emplace_back(split, last);
            }
        }
        std::vector<Point> reduced;
        for (std::size_t i = 0; i < points.size(); ++i)
            if (keep[i])
                reduced.push_back(points[i]);
        stroke.points = std::move(reduced);
    }
    drawing = std::move(result);
}
} // namespace opentoon
