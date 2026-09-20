#include "canvas_item.h"
#include <QPainter>
#include <cmath>
using namespace opentoon;
void CanvasItem::setGridVisible(bool value) {
    gridVisible_ = value;
    emit viewChanged();
    update();
}
void CanvasItem::setSnapToGrid(bool value) {
    cancelGesture();
    snapToGrid_ = value;
    emit viewChanged();
    update();
}
void CanvasItem::setGridSpacing(int value) {
    if (value < 2 || value > 1000)
        return;
    cancelGesture();
    gridSpacing_ = value;
    emit viewChanged();
    update();
}
Point CanvasItem::snapDrawingPoint(Point p) const {
    if (snapToGrid_ && editor_ &&
        (editor_->tool() == "Pencil" || editor_->tool() == "Line" || editor_->tool() == "Rectangle" ||
         editor_->tool() == "Ellipse")) {
        p.x = std::round(p.x / gridSpacing_) * gridSpacing_;
        p.y = std::round(p.y / gridSpacing_) * gridSpacing_;
    }
    return p;
}
Point CanvasItem::constrainedEndpoint(Point p) const {
    if (!shift_ || samples_.empty())
        return p;
    const auto a = samples_.front();
    double dx = p.x - a.x, dy = p.y - a.y;
    if (editor_->tool() == "Line") {
        const double angle = std::round(std::atan2(dy, dx) / (std::acos(-1) / 4)) * (std::acos(-1) / 4);
        const double length = std::hypot(dx, dy);
        p.x = a.x + length * std::cos(angle);
        p.y = a.y + length * std::sin(angle);
    } else {
        const double size = std::max(std::abs(dx), std::abs(dy));
        p.x = a.x + std::copysign(size, dx);
        p.y = a.y + std::copysign(size, dy);
    }
    return p;
}
void CanvasItem::paintGrid(QPainter* painter) {
    if (!gridVisible_)
        return;
    // Drawing-local grid follows the selected layer. Guides never enter rendered output.
    painter->save();
    QPen pen(QColor(127, 127, 127, 65), 1);
    pen.setCosmetic(true);
    painter->setPen(pen);
    const auto world = selectionWorld();
    const double screenStep = QLineF(world.map(QPointF()), world.map(QPointF(gridSpacing_, 0))).length();
    const int stride =
        screenStep < 8 ? std::clamp(int(std::ceil(8 / std::max(screenStep, .001))), 1, 1000) : 1;
    const int step = gridSpacing_ * stride;
    // Bound both axes to 512 lines even in a very large scene.
    const int sx = step * std::max(1, int(std::ceil(editor_->sceneWidth() / double(step * 512))));
    const int sy = step * std::max(1, int(std::ceil(editor_->sceneHeight() / double(step * 512))));
    for (int x = 0; x <= editor_->sceneWidth(); x += sx)
        painter->drawLine(x, 0, x, editor_->sceneHeight());
    for (int y = 0; y <= editor_->sceneHeight(); y += sy)
        painter->drawLine(0, y, editor_->sceneWidth(), y);
    painter->restore();
}
