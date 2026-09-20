#pragma once
#include "opentoon/shared_buffer.h"
#include <algorithm>
#include <array>
#include <compare>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace opentoon {
using Id = std::uint64_t;
using Frame = std::int32_t;
struct FrameRate {
    std::int32_t numerator = 24;
    std::int32_t denominator = 1;
    void validate() const;
    [[nodiscard]] double seconds(Frame frame) const;
    [[nodiscard]] std::int64_t sampleAt(Frame frame, std::int32_t sampleRate) const;
    auto operator<=>(const FrameRate&) const = default;
};
struct Point {
    double x = 0, y = 0, pressure = 1;
    auto operator<=>(const Point&) const = default;
};
struct Color {
    double r = 0, g = 0, b = 0, a = 1;
    auto operator<=>(const Color&) const = default;
};
struct Swatch {
    Id id = 0;
    std::string name;
    Color color;
    auto operator<=>(const Swatch&) const = default;
};
enum class Shape { Stroke, Rectangle, Ellipse, Polygon };
struct Stroke {
    Id id = 0, swatch = 0;
    double width = 4;
    Shape shape = Shape::Stroke;
    bool filled = false;
    int artLayer = 2;
    std::vector<Point> points;
    auto operator<=>(const Stroke&) const = default;
};
struct ImageAsset {
    int width = 0, height = 0;
    SharedBuffer<std::uint8_t> rgba;
    auto operator<=>(const ImageAsset&) const = default;
};
struct RasterImage {
    static constexpr int tileSize = 64;
    static constexpr int channels = 4;
    int width = 1920, height = 1080;
    // Premultiplied RGBA at 15-bit precision, matching the MyPaint surface contract.
    std::map<std::pair<int, int>, SharedBuffer<std::uint16_t>> tiles;
    auto operator<=>(const RasterImage&) const = default;
};
struct Drawing {
    Id id = 0;
    std::string name;
    std::vector<Stroke> strokes;
    std::optional<ImageAsset> image;
    std::optional<RasterImage> raster;
    auto operator<=>(const Drawing&) const = default;
};
struct Exposure {
    Frame start = 0, end = 1;
    Id drawing = 0;
    auto operator<=>(const Exposure&) const = default;
};
struct Transform {
    double x = 0, y = 0, rotation = 0, scaleX = 1, scaleY = 1, opacity = 1;
    double pivotX = 0, pivotY = 0;
    auto operator<=>(const Transform&) const = default;
};
enum class Interpolation { Linear, Step, Smooth };
struct Keyframe {
    Frame frame = 0;
    Transform value;
    Interpolation interpolation = Interpolation::Linear;
    auto operator<=>(const Keyframe&) const = default;
};
struct Layer {
    Id id = 0;
    std::string name;
    bool visible = true, locked = false, solo = false;
    Id parent = 0;
    Transform transform;
    std::vector<Exposure> exposures;
    std::vector<Keyframe> keys;
    auto operator<=>(const Layer&) const = default;
};
struct Marker {
    Frame frame = 0;
    std::string name;
    auto operator<=>(const Marker&) const = default;
};
struct Document {
    static constexpr int formatVersion = 2;
    std::string name = "Untitled scene";
    int width = 1920, height = 1080;
    Frame duration = 48;
    FrameRate rate;
    Color background{1, 1, 1, 1};
    Id nextId = 1;
    std::vector<Layer> layers;
    std::map<Id, Drawing> drawings;
    std::vector<Swatch> palette;
    std::vector<Marker> markers;
    [[nodiscard]] Id allocateId() { return nextId++; }
    [[nodiscard]] Layer& layer(Id id);
    [[nodiscard]] const Layer& layer(Id id) const;
    [[nodiscard]] const Drawing* drawingAt(Id layerId, Frame frame) const;
    [[nodiscard]] Drawing& editableDrawing(Id layerId, Frame frame);
    void validate() const;
    auto operator<=>(const Document&) const = default;
};
[[nodiscard]] Document makeDocument();
[[nodiscard]] Document makeBouncingBall();
[[nodiscard]] Transform evaluateTransform(const Layer&, Frame);
void expose(Layer&, Frame start, Frame end, Id drawing);
void insertFrames(Document&, Frame at, Frame count);
void removeFrames(Document&, Frame at, Frame count);
void eraseAt(Drawing&, Point center, double radius, Id& nextId);
[[nodiscard]] std::optional<Id> hitStroke(const Drawing&, Point, double tolerance);
} // namespace opentoon
