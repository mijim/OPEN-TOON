#pragma once
#include "opentoon/document.h"
#include <memory>
namespace opentoon {
enum class BrushPreset { Ink, Soft, Dry, Smudge, Eraser };
struct BrushSettings {
    double diameter = 24;
    double opacity = 1;
    Color color;
    BrushPreset preset = BrushPreset::Ink;
};
// One gesture owns a private mutable surface. Published tiles remain immutable.
class RasterBrush {
  public:
    RasterBrush(const RasterImage&, BrushSettings);
    ~RasterBrush();
    RasterBrush(const RasterBrush&) = delete;
    RasterBrush& operator=(const RasterBrush&) = delete;
    void sample(Point, double seconds = 1.0 / 120, double tiltX = 0, double tiltY = 0);
    [[nodiscard]] RasterImage snapshot();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace opentoon
