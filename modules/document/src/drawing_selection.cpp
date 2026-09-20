#include "opentoon/drawing_selection.h"
#include <array>
#include <cmath>
#include <set>
#include <stdexcept>
namespace opentoon {
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
                          int dx, int dy, Id& nextId) {
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
        auto ids = enclosedStrokes(drawing, rect);
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
} // namespace opentoon
