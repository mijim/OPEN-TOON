#pragma once
#include "opentoon/document.h"
#include <QImage>
#include <QPainter>
#include <QTransform>
namespace opentoon {
struct RenderOptions {
    bool background = true;
    bool onionSkin = false;
    int onionRange = 1;
    Id isolatedLayer = 0;
};
class SceneRenderer {
  public:
    static QImage render(const Document&, Frame, QSize size = {}, RenderOptions = {});
    static void paint(QPainter&, const Document&, Frame, RenderOptions = {});
    static void paintStroke(QPainter&, const Stroke&, const std::vector<Swatch>&, double opacity = 1);
    static QTransform worldTransform(const Document&, const Layer&, Frame);
};
} // namespace opentoon
