#pragma once
#include "opentoon/document.h"
#include <QPointF>
#include <QTransform>
namespace opentoon {
// Radius is measured in logical screen pixels, independent of layer/view transforms and DPR.
std::optional<Id> hitVectorOnScreen(const Drawing&, const QTransform& localToScreen, QPointF,
                                    double radius = 8);
} // namespace opentoon
namespace opentoon {
// Closed odd-even lasso in drawing-local space; only fully contained visible vector ink is selected.
std::vector<Id> enclosedVectorsByLasso(const Drawing&, const std::vector<Point>&);
} // namespace opentoon
