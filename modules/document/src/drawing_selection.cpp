#include "opentoon/drawing_selection.h"
#include <array>
#include <cmath>
#include <set>
#include <stdexcept>
namespace opentoon {
void insertStrokePoint(Stroke& stroke, std::size_t segment, double fraction) {
    if (stroke.shape != Shape::Stroke && stroke.shape != Shape::Polygon)
        throw std::invalid_argument("Point insertion supports pencil strokes and polygons.");
    const auto count = stroke.points.size();
    const auto segments = stroke.shape == Shape::Polygon ? count : count > 0 ? count - 1 : 0;
    if (segment >= segments || !std::isfinite(fraction) || fraction <= 0 || fraction >= 1 || count >= 100000)
        throw std::invalid_argument("Choose a point inside an editable segment.");
    auto a = stroke.points[segment], b = stroke.points[(segment + 1) % count];
    Point p{a.x + (b.x - a.x) * fraction, a.y + (b.y - a.y) * fraction,
            a.pressure + (b.pressure - a.pressure) * fraction};
    stroke.points.insert(stroke.points.begin() + segment + 1, p);
}
void removeStrokePoint(Stroke& stroke, std::size_t point) {
    if (stroke.shape != Shape::Stroke && stroke.shape != Shape::Polygon)
        throw std::invalid_argument("Point deletion supports pencil strokes and polygons.");
    const std::size_t minimum = stroke.shape == Shape::Polygon ? 3 : 1;
    if (point >= stroke.points.size() || stroke.points.size() <= minimum)
        throw std::invalid_argument(
            "This stroke cannot lose another point. Use Select to delete the object.");
    stroke.points.erase(stroke.points.begin() + point);
}

namespace {
void validate(PixelRect r) {
    if (r.width < 1 || r.height < 1 || r.width > 20000000 || r.height > 20000000 ||
        std::abs(std::int64_t(r.x)) > 10000000 || std::abs(std::int64_t(r.y)) > 10000000)
        throw std::invalid_argument("Invalid drawing selection bounds.");
}
using Pixel = std::array<std::uint16_t, 4>;
using Tile = std::vector<std::uint16_t>;
using TileKey = std::pair<int, int>;
constexpr int side = RasterImage::tileSize;
constexpr int values = side * side * 4;
// Copy only touched tiles; read all source pixels from the immutable input snapshot.
RasterImage editRaster(const RasterImage& source, PixelRect r, SelectionAction action, int dx, int dy) {
    RasterImage output = source;
    std::map<TileKey, Tile> dirty;
    auto writable = [&](int x, int y) -> std::uint16_t* {
        TileKey key{x / side, y / side};
        auto it = dirty.find(key);
        if (it == dirty.end()) {
            if (dirty.size() >= 4096)
                throw std::runtime_error("Selection exceeds the 128 MiB edit budget. Select a smaller area.");
            auto original = source.tiles.find(key);
            it = dirty
                     .emplace(key,
                              original == source.tiles.end() ? Tile(values, 0) : original->second.values())
                     .first;
        }
        return it->second.data() + ((y % side) * side + x % side) * 4;
    };
    auto visit = [&](auto operation) {
        for (const auto& [key, tile] : source.tiles) {
            int x0 = std::max({0, r.x, key.first * side});
            int y0 = std::max({0, r.y, key.second * side});
            int x1 = std::min({source.width, r.x + r.width, (key.first + 1) * side});
            int y1 = std::min({source.height, r.y + r.height, (key.second + 1) * side});
            for (int y = y0; y < y1; ++y)
                for (int x = x0; x < x1; ++x) {
                    auto offset = ((y % side) * side + x % side) * 4;
                    if (!tile[offset + 3])
                        continue;
                    operation(x, y,
                              Pixel{tile[offset], tile[offset + 1], tile[offset + 2], tile[offset + 3]});
                }
        }
    };
    if (action != SelectionAction::Duplicate)
        visit([&](int x, int y, Pixel) { std::fill_n(writable(x, y), 4, 0); });
    if (action != SelectionAction::Delete)
        visit([&](int x, int y, Pixel pixel) {
            int targetX = action == SelectionAction::FlipHorizontal ? 2 * r.x + r.width - 1 - x : x + dx;
            int targetY = action == SelectionAction::FlipVertical ? 2 * r.y + r.height - 1 - y : y + dy;
            if (action == SelectionAction::RotateClockwise) {
                targetX = r.x + r.height - 1 - (y - r.y);
                targetY = r.y + (x - r.x);
            }
            if (targetX < 0 || targetY < 0 || targetX >= source.width || targetY >= source.height)
                throw std::runtime_error(
                    "Selected pixels would leave the raster canvas. Move them inside the canvas.");
            auto* destination = writable(targetX, targetY);
            for (int c = 0; c < 4; ++c)
                destination[c] = std::uint16_t(
                    pixel[c] + (std::uint32_t(destination[c]) * (32768 - pixel[3]) + 16384) / 32768);
        });
    for (auto& [key, tile] : dirty) {
        if (std::all_of(tile.begin(), tile.end(), [](auto v) { return v == 0; }))
            output.tiles.erase(key);
        else
            output.tiles.insert_or_assign(key, SharedBuffer<std::uint16_t>(std::move(tile)));
    }
    return output;
}
} // namespace
std::vector<Id> enclosedStrokes(const Drawing& drawing, PixelRect rect) {
    validate(rect);
    std::vector<Id> result;
    for (const auto& stroke : drawing.strokes) {
        if (stroke.points.empty())
            continue;
        double left = stroke.points.front().x, right = left, top = stroke.points.front().y, bottom = top;
        for (const auto& p : stroke.points) {
            left = std::min(left, p.x);
            right = std::max(right, p.x);
            top = std::min(top, p.y);
            bottom = std::max(bottom, p.y);
        }
        double margin = stroke.width / 2;
        if (left - margin >= rect.x && top - margin >= rect.y && right + margin <= rect.x + rect.width &&
            bottom + margin <= rect.y + rect.height)
            result.push_back(stroke.id);
    }
    return result;
}
void editDrawingSelection(Drawing& drawing, PixelRect rect, SelectionMedia media, SelectionAction action,
                          int dx, int dy, Id& nextId, const std::vector<Id>* selectedIds) {
    validate(rect);
    if (int(media) < 0 || int(media) > 2 || int(action) < 0 || int(action) > 5 ||
        std::abs(std::int64_t(dx)) > 10000000 || std::abs(std::int64_t(dy)) > 10000000)
        throw std::invalid_argument("Invalid selection operation.");
    if (action == SelectionAction::Move && dx == 0 && dy == 0)
        return;
    if (action == SelectionAction::FlipHorizontal || action == SelectionAction::FlipVertical ||
        action == SelectionAction::RotateClockwise)
        dx = dy = 0;
    auto result = drawing;
    auto allocated = nextId;
    if (media != SelectionMedia::Raster) {
        auto ids = selectedIds ? *selectedIds : enclosedStrokes(drawing, rect);
        std::set<Id> selected(ids.begin(), ids.end());
        if (action == SelectionAction::Delete)
            std::erase_if(result.strokes, [&](const auto& s) { return selected.contains(s.id); });
        else {
            for (std::size_t index = 0; index < drawing.strokes.size(); ++index) {
                const auto& original = drawing.strokes[index];
                if (!selected.contains(original.id))
                    continue;
                auto stroke = original;
                for (auto& point : stroke.points) {
                    if (action == SelectionAction::RotateClockwise) {
                        const auto x = point.x;
                        point.x = rect.x + rect.height - (point.y - rect.y);
                        point.y = rect.y + (x - rect.x);
                        continue;
                    }
                    point.x = action == SelectionAction::FlipHorizontal ? 2.0 * rect.x + rect.width - point.x
                                                                        : point.x + dx;
                    point.y = action == SelectionAction::FlipVertical ? 2.0 * rect.y + rect.height - point.y
                                                                      : point.y + dy;
                }
                if (action == SelectionAction::Duplicate) {
                    stroke.id = allocated++;
                    result.strokes.push_back(std::move(stroke));
                } else {
                    result.strokes[index] = std::move(stroke);
                }
            }
        }
    }
    if (media != SelectionMedia::Vectors && drawing.raster)
        result.raster = editRaster(*drawing.raster, rect, action, dx, dy);
    drawing = std::move(result);
    nextId = allocated;
}

void transformDrawingSelection(Drawing& drawing, PixelRect rect, SelectionMedia media,
                               const std::vector<Id>& ids, SelectionTransform m) {
    validate(rect);
    for (auto v : {m.a, m.b, m.c, m.d, m.tx, m.ty})
        if (!std::isfinite(v) || std::abs(v) > 10000000)
            throw std::invalid_argument("Invalid selection transform.");
    double determinant = m.a * m.d - m.b * m.c;
    if (std::abs(determinant) < 1e-8)
        throw std::invalid_argument("Selection cannot have zero size.");
    auto map = [&](Point p) {
        return Point{m.a * p.x + m.c * p.y + m.tx, m.b * p.x + m.d * p.y + m.ty, p.pressure};
    };
    auto result = drawing;
    if (media != SelectionMedia::Raster) {
        std::set<Id> selected(ids.begin(), ids.end());
        for (auto& stroke : result.strokes)
            if (selected.contains(stroke.id)) {
                // Preserve primitives for axis-aligned transforms. Rotated primitives become editable paths.
                if ((std::abs(m.b) > 1e-8 || std::abs(m.c) > 1e-8) && stroke.points.size() > 1 &&
                    (stroke.shape == Shape::Rectangle || stroke.shape == Shape::Ellipse)) {
                    auto first = stroke.points.front(), last = stroke.points.back();
                    double left = std::min(first.x, last.x), right = std::max(first.x, last.x);
                    double top = std::min(first.y, last.y), bottom = std::max(first.y, last.y);
                    stroke.points.clear();
                    if (stroke.shape == Shape::Rectangle)
                        stroke.points = {
                            {left, top, 1}, {right, top, 1}, {right, bottom, 1}, {left, bottom, 1}};
                    else
                        for (int i = 0; i < 128; ++i) {
                            double angle = i * 2 * 3.14159265358979323846 / 128;
                            stroke.points.push_back(
                                {(left + right) / 2 + (right - left) / 2 * std::cos(angle),
                                 (top + bottom) / 2 + (bottom - top) / 2 * std::sin(angle), 1});
                        }
                    stroke.shape = Shape::Polygon;
                }
                for (auto& point : stroke.points) {
                    point = map(point);
                    if (std::abs(point.x) > 10000000 || std::abs(point.y) > 10000000)
                        throw std::invalid_argument("Transformed vectors exceed coordinate limits.");
                }
                stroke.width = std::clamp(stroke.width * std::sqrt(std::abs(determinant)), 0.01, 4096.0);
            }
    }
    if (media != SelectionMedia::Vectors && drawing.raster) {
        const auto& source = *drawing.raster;
        // Transform only the selected nontransparent content bounds, not a large empty marquee.
        int left = source.width, top = source.height, right = 0, bottom = 0;
        for (const auto& [key, tile] : source.tiles) {
            for (int y = std::max(rect.y, key.second * 64);
                 y < std::min({rect.y + rect.height, (key.second + 1) * 64, source.height}); ++y)
                for (int x = std::max(rect.x, key.first * 64);
                     x < std::min({rect.x + rect.width, (key.first + 1) * 64, source.width}); ++x)
                    if (tile[((y % 64) * 64 + x % 64) * 4 + 3]) {
                        left = std::min(left, x);
                        top = std::min(top, y);
                        right = std::max(right, x + 1);
                        bottom = std::max(bottom, y + 1);
                    }
        }
        if (right > left && bottom > top) {
            double x0 = 1e30, y0 = 1e30, x1 = -1e30, y1 = -1e30;
            for (auto p : {Point{double(left), double(top)}, Point{double(right), double(top)},
                           Point{double(right), double(bottom)}, Point{double(left), double(bottom)}}) {
                auto q = map(p);
                x0 = std::min(x0, q.x);
                y0 = std::min(y0, q.y);
                x1 = std::max(x1, q.x);
                y1 = std::max(y1, q.y);
            }
            if (x0 < -1e-6 || y0 < -1e-6 || x1 > source.width + 1e-6 || y1 > source.height + 1e-6)
                throw std::runtime_error("Keep transformed raster pixels inside the canvas.");
            int beginX = std::max(0, int(std::floor(x0))), beginY = std::max(0, int(std::floor(y0)));
            int endX = std::min(source.width, int(std::ceil(x1))),
                endY = std::min(source.height, int(std::ceil(y1)));
            if (std::int64_t(endX - beginX) * (endY - beginY) > 16000000)
                throw std::runtime_error(
                    "Transform exceeds the 16-million-pixel work budget. Select a smaller area.");
            result.raster = editRaster(source, rect, SelectionAction::Delete, 0, 0);
            auto& output = *result.raster;
            std::map<TileKey, Tile> dirty;
            for (int y = beginY; y < endY; ++y)
                for (int x = beginX; x < endX; ++x) {
                    double px = x + .5 - m.tx, py = y + .5 - m.ty;
                    double sourceX = std::floor((m.d * px - m.c * py) / determinant),
                           sourceY = std::floor((-m.b * px + m.a * py) / determinant);
                    if (sourceX < 0 || sourceY < 0 || sourceX >= source.width || sourceY >= source.height)
                        continue;
                    int sx = int(sourceX), sy = int(sourceY);
                    if (sx < 0 || sy < 0 || sx >= source.width || sy >= source.height || sx < rect.x ||
                        sy < rect.y || sx >= rect.x + rect.width || sy >= rect.y + rect.height)
                        continue;
                    auto from = source.tiles.find({sx / 64, sy / 64});
                    if (from == source.tiles.end())
                        continue;
                    auto n = ((sy % 64) * 64 + sx % 64) * 4;
                    if (!from->second[n + 3])
                        continue;
                    TileKey key{x / 64, y / 64};
                    auto it = dirty.find(key);
                    if (it == dirty.end()) {
                        if (dirty.size() >= 4096)
                            throw std::runtime_error("Transform exceeds the tile budget.");
                        auto base = output.tiles.find(key);
                        it = dirty
                                 .emplace(key, base == output.tiles.end() ? Tile(values, 0)
                                                                          : base->second.values())
                                 .first;
                    }
                    auto dest = ((y % 64) * 64 + x % 64) * 4;
                    for (int c = 0; c < 4; ++c)
                        it->second[dest + c] = std::uint16_t(
                            from->second[n + c] +
                            (std::uint32_t(it->second[dest + c]) * (32768 - from->second[n + 3]) + 16384) /
                                32768);
                }
            for (auto& [key, tile] : dirty)
                output.tiles.insert_or_assign(key, SharedBuffer<std::uint16_t>(std::move(tile)));
        }
    }
    drawing = std::move(result);
}
} // namespace opentoon
