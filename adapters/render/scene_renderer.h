#pragma once
#include "opentoon/document.h"
#include <QImage>
#include <QPainter>
#include <QTransform>
#include <functional>
#include <stdexcept>
namespace opentoon {
class RenderCancelled : public std::runtime_error {
  public:
    RenderCancelled() : std::runtime_error("Render cancelled.") {}
};
struct RenderOptions {
    bool background = true;
    bool onionSkin = false;
    int onionRange = 1;
    Id isolatedLayer = 0;
    Id previewLayer = 0;
    const Drawing* previewDrawing = nullptr;
    std::function<bool()> cancelled;
    bool ignoreCamera = false; // Stage editing view; never used for saved output.
};
class SceneRenderer {
  public:
    static QImage render(const Document&, Frame, QSize size = {}, RenderOptions = {});
    static void paint(QPainter&, const Document&, Frame, RenderOptions = {});
    static void paintStroke(QPainter&, const Stroke&, const std::vector<Swatch>&, double opacity = 1);
    static QTransform worldTransform(const Document&, const Layer&, Frame);
    static QTransform cameraTransform(const Document&, Frame);
    static QRect layerInkBounds(const Document&, const Layer&, Frame, QSize, RenderOptions = {});
};
} // namespace opentoon
