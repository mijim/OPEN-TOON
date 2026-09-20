#pragma once
#include "opentoon/document.h"
#include <QPointF>
#include <QTransform>
namespace opentoon {
// Radius is measured in logical screen pixels, independent of layer/view transforms and DPR.
std::optional<Id> hitVectorOnScreen(const Drawing&, const QTransform& localToScreen, QPointF,
                                    double radius = 8);
} // namespace opentoon
