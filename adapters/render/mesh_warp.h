#pragma once
#include "opentoon/document.h"
#include <QImage>
#include <QPoint>
#include <functional>

namespace opentoon {
struct WarpedImage {
    QImage pixels;
    QPoint origin;
};
[[nodiscard]] WarpedImage warpMeshImage(const ImageAsset&, const MeshBinding&,
                                         const std::function<bool()>& cancelled = {});
} // namespace opentoon
