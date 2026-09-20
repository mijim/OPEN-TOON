#include "raster_brush.h"
#include <array>
#include <cmath>
#include <exception>
#include <mypaint-brush.h>
#include <mypaint-tiled-surface.h>
#include <set>
#include <stdexcept>
namespace opentoon {
namespace {
constexpr std::size_t tileSamples = RasterImage::tileSize * RasterImage::tileSize * 4;
std::array<float, 3> hsv(Color c) {
    double maximum = std::max({c.r, c.g, c.b}), minimum = std::min({c.r, c.g, c.b});
    double delta = maximum - minimum, hue = 0;
    if (delta > 0) {
        if (maximum == c.r)
            hue = std::fmod((c.g - c.b) / delta, 6.0);
        else if (maximum == c.g)
            hue = (c.b - c.r) / delta + 2;
        else
            hue = (c.r - c.g) / delta + 4;
        hue /= 6;
        if (hue < 0)
            hue += 1;
    }
    return {float(hue), float(maximum == 0 ? 0 : delta / maximum), float(maximum)};
}
} // namespace
struct RasterBrush::Impl : MyPaintTiledSurface {
    RasterImage image;
    std::map<std::pair<int, int>, std::vector<std::uint16_t>> buffers;
    std::set<std::pair<int, int>> dirty;
    std::array<std::uint16_t, tileSamples> outside{};
    std::exception_ptr error;
    MyPaintBrush* brush = nullptr;
    bool started = false;
    static void start(MyPaintTiledSurface* surface, MyPaintTileRequest* request) noexcept {
        auto& self = *static_cast<Impl*>(surface);
        request->buffer = self.outside.data();
        std::fill(self.outside.begin(), self.outside.end(), 0);
        if (self.error || request->tx < 0 || request->ty < 0 || request->tx >= (self.image.width + 63) / 64 ||
            request->ty >= (self.image.height + 63) / 64)
            return;
        try {
            const auto key = std::pair{request->tx, request->ty};
            auto found = self.buffers.find(key);
            if (found == self.buffers.end()) {
                auto original = self.image.tiles.find(key);
                auto bytes = original == self.image.tiles.end() ? std::vector<std::uint16_t>(tileSamples)
                                                                : original->second.values();
                // A single gesture cannot allocate an unbounded number of tiles.
                if (self.buffers.size() >= 4096)
                    throw std::runtime_error(
                        "Brush gesture exceeds 128 MiB. Release the pointer before continuing.");
                found = self.buffers.emplace(key, std::move(bytes)).first;
            }
            if (!request->readonly)
                self.dirty.insert(key);
            request->buffer = found->second.data();
        } catch (...) {
            self.error = std::current_exception();
        }
    }
    static void end(MyPaintTiledSurface*, MyPaintTileRequest*) noexcept {}
    Impl(const RasterImage& source, BrushSettings settings) : image(source) {
        if (image.width < 1 || image.width > 8192 || image.height < 1 || image.height > 8192 ||
            !std::isfinite(settings.diameter) || settings.diameter < 1 || settings.diameter > 1024 ||
            !std::isfinite(settings.opacity) || settings.opacity < 0 || settings.opacity > 1)
            throw std::invalid_argument("Invalid raster brush settings.");
        for (double channel : {settings.color.r, settings.color.g, settings.color.b, settings.color.a})
            if (!std::isfinite(channel) || channel < 0 || channel > 1)
                throw std::invalid_argument("Invalid brush color.");
        for (const auto& [key, tile] : image.tiles)
            if (tile.size() != tileSamples)
                throw std::invalid_argument("Invalid raster surface.");
        brush = mypaint_brush_new();
        if (!brush)
            throw std::bad_alloc();
        mypaint_tiled_surface_init(this, start, end);
        threadsafe_tile_requests = false;
        mypaint_brush_from_defaults(brush);
        auto set = [&](MyPaintBrushSetting id, float value) {
            mypaint_brush_set_base_value(brush, id, value);
        };
        const auto color = hsv(settings.color);
        set(MYPAINT_BRUSH_SETTING_COLOR_H, color[0]);
        set(MYPAINT_BRUSH_SETTING_COLOR_S, color[1]);
        set(MYPAINT_BRUSH_SETTING_COLOR_V, color[2]);
        set(MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC, float(std::log(settings.diameter / 2)));
        set(MYPAINT_BRUSH_SETTING_OPAQUE,
            float(settings.opacity * (settings.preset == BrushPreset::Eraser ? 1 : settings.color.a)));
        set(MYPAINT_BRUSH_SETTING_HARDNESS, settings.preset == BrushPreset::Soft ? 0.25f : 0.85f);
        set(MYPAINT_BRUSH_SETTING_DABS_PER_ACTUAL_RADIUS, 4);
        set(MYPAINT_BRUSH_SETTING_DABS_PER_BASIC_RADIUS, 0);
        set(MYPAINT_BRUSH_SETTING_ERASER, settings.preset == BrushPreset::Eraser ? 1 : 0);
        if (settings.preset == BrushPreset::Dry) {
            set(MYPAINT_BRUSH_SETTING_RADIUS_BY_RANDOM, 0.4f);
            set(MYPAINT_BRUSH_SETTING_OFFSET_BY_RANDOM, 0.4f);
            set(MYPAINT_BRUSH_SETTING_OPAQUE, float(settings.opacity * settings.color.a * 0.45));
        }
        if (settings.preset == BrushPreset::Smudge) {
            set(MYPAINT_BRUSH_SETTING_SMUDGE, 1);
            set(MYPAINT_BRUSH_SETTING_SMUDGE_LENGTH, 0.7f);
        }
        mypaint_brush_set_mapping_n(brush, MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC,
                                    MYPAINT_BRUSH_INPUT_PRESSURE, 2);
        mypaint_brush_set_mapping_point(brush, MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC,
                                        MYPAINT_BRUSH_INPUT_PRESSURE, 0, 0, -1.6f);
        mypaint_brush_set_mapping_point(brush, MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC,
                                        MYPAINT_BRUSH_INPUT_PRESSURE, 1, 1, 0);
        mypaint_brush_new_stroke(brush);
    }
    ~Impl() {
        mypaint_brush_unref(brush);
        mypaint_tiled_surface_destroy(this);
    }
    void dab(Point point, double time, double tiltX, double tiltY) {
        mypaint_tiled_surface_begin_atomic(this);
        mypaint_brush_stroke_to(brush, &parent, float(point.x), float(point.y), float(point.pressure),
                                float(tiltX), float(tiltY), time);
        mypaint_tiled_surface_end_atomic(this, nullptr);
        if (error)
            std::rethrow_exception(error);
    }
};
RasterBrush::RasterBrush(const RasterImage& image, BrushSettings settings)
    : impl_(std::make_unique<Impl>(image, settings)) {}
RasterBrush::~RasterBrush() = default;
void RasterBrush::sample(Point point, double seconds, double tiltX, double tiltY) {
    if (!std::isfinite(point.x) || !std::isfinite(point.y) || std::abs(point.x) > 100000 ||
        std::abs(point.y) > 100000 || !std::isfinite(point.pressure) || point.pressure < 0 ||
        point.pressure > 1 || !std::isfinite(seconds) || seconds <= 0 || !std::isfinite(tiltX) ||
        !std::isfinite(tiltY) || std::abs(tiltX) > 1 || std::abs(tiltY) > 1)
        throw std::invalid_argument("Invalid brush sample.");
    point.x = std::clamp(point.x, -1024.0, impl_->image.width + 1024.0);
    point.y = std::clamp(point.y, -1024.0, impl_->image.height + 1024.0);
    if (!impl_->started) {
        impl_->dab({point.x, point.y, 0}, 1, tiltX, tiltY);
        // Seed two engine-generated dabs at the initial point so a click paints.
        // Normal motion remains distance-based and independent of input frequency.
        mypaint_brush_set_base_value(impl_->brush, MYPAINT_BRUSH_SETTING_DABS_PER_SECOND, 120);
        impl_->dab(point, 1.0 / 60, tiltX, tiltY);
        mypaint_brush_set_base_value(impl_->brush, MYPAINT_BRUSH_SETTING_DABS_PER_SECOND, 0);
        impl_->started = true;
        return;
    }
    impl_->dab(point, std::clamp(seconds, 0.001, 0.1), tiltX, tiltY);
}
RasterImage RasterBrush::snapshot() {
    for (auto key : impl_->dirty) {
        auto& data = impl_->buffers.at(key);
        // Clip edge tiles so artwork cannot reappear outside the original canvas.
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                if (key.first * 64 + x >= impl_->image.width || key.second * 64 + y >= impl_->image.height)
                    std::fill_n(data.begin() + (y * 64 + x) * 4, 4, 0);
        if (std::all_of(data.begin(), data.end(), [](auto value) { return value == 0; }))
            impl_->image.tiles.erase(key);
        else
            impl_->image.tiles[key] = data;
    }
    impl_->dirty.clear();
    return impl_->image;
}
} // namespace opentoon
