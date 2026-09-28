#include "mesh_warp.h"
#include "scene_renderer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace opentoon {
namespace {
struct Triangle {
    MeshPoint a, b, c, uvA, uvB, uvC;
};
double cross(MeshPoint a, MeshPoint b, MeshPoint c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}
std::array<double, 4> texel(const ImageAsset& source, int x, int y) {
    x = std::clamp(x, 0, source.width - 1);
    y = std::clamp(y, 0, source.height - 1);
    const auto offset = (std::size_t(y) * source.width + x) * 4;
    const double alpha = source.rgba[offset + 3] / 255.0;
    return {source.rgba[offset] * alpha, source.rgba[offset + 1] * alpha,
            source.rgba[offset + 2] * alpha, source.rgba[offset + 3] * 1.0};
}
QRgb sample(const ImageAsset& source, MeshPoint uv) {
    const double x = std::clamp(uv.x * source.width - 0.5, 0.0, double(source.width - 1));
    const double y = std::clamp(uv.y * source.height - 0.5, 0.0, double(source.height - 1));
    const int x0 = int(std::floor(x)), y0 = int(std::floor(y));
    const double fx = x - x0, fy = y - y0;
    const auto topLeft = texel(source, x0, y0);
    const auto topRight = texel(source, x0 + 1, y0);
    const auto bottomLeft = texel(source, x0, y0 + 1);
    const auto bottomRight = texel(source, x0 + 1, y0 + 1);
    std::array<int, 4> out{};
    for (int channel = 0; channel < 4; ++channel) {
        const double value = (1 - fy) * ((1 - fx) * topLeft[channel] + fx * topRight[channel]) +
                             fy * ((1 - fx) * bottomLeft[channel] + fx * bottomRight[channel]);
        out[channel] = std::clamp(int(std::lround(value)), 0, 255);
    }
    return qRgba(out[0], out[1], out[2], out[3]);
}
} // namespace

WarpedImage warpMeshImage(const ImageAsset& source, const MeshBinding& binding,
                          const std::function<bool()>& cancelled) {
    if (source.width != binding.sourceWidth || source.height != binding.sourceHeight ||
        binding.columns < 1 || binding.rows < 1 ||
        binding.vertices.size() != std::size_t(binding.columns + 1) * (binding.rows + 1))
        throw std::invalid_argument("Mesh binding is incompatible with its image.");
    if (cancelled && cancelled())
        throw RenderCancelled();
    const bool rest = std::all_of(binding.vertices.begin(), binding.vertices.end(),
                                  [&](const MeshVertex& vertex) {
                                      return vertex.pose == vertex.rest &&
                                             vertex.rest == MeshPoint{vertex.uv.x * source.width,
                                                                      vertex.uv.y * source.height};
                                  });
    if (rest) {
        QImage image(source.rgba.data(), source.width, source.height, source.width * 4,
                     QImage::Format_RGBA8888);
        return {image, {0, 0}};
    }
    double minX = std::numeric_limits<double>::infinity();
    double minY = minX, maxX = -minX, maxY = -minX;
    for (const auto& vertex : binding.vertices) {
        minX = std::min(minX, vertex.pose.x);
        minY = std::min(minY, vertex.pose.y);
        maxX = std::max(maxX, vertex.pose.x);
        maxY = std::max(maxY, vertex.pose.y);
    }
    if (!std::isfinite(minX) || !std::isfinite(minY) || !std::isfinite(maxX) ||
        !std::isfinite(maxY))
        throw std::invalid_argument("Mesh pose has nonfinite bounds.");
    const int originX = int(std::floor(minX)), originY = int(std::floor(minY));
    const int width = int(std::ceil(maxX)) - originX;
    const int height = int(std::ceil(maxY)) - originY;
    if (width <= 0 || height <= 0 || width > 4096 || height > 4096 ||
        std::int64_t(width) * height > 16 * 1024 * 1024)
        throw std::invalid_argument("Mesh render proxy exceeds its 4096-pixel axis limit.");
    QImage output(width, height, QImage::Format_ARGB32_Premultiplied);
    if (output.isNull())
        throw std::runtime_error("Unable to allocate mesh render proxy.");
    output.fill(Qt::transparent);
    std::vector<std::uint8_t> covered(std::size_t(width) * height, 0);
    auto rasterize = [&](Triangle triangle) {
        const double determinant = cross(triangle.a, triangle.b, triangle.c);
        if (!(determinant > 1e-8))
            throw std::invalid_argument("Mesh pose has a degenerate triangle.");
        const int left = std::max(0, int(std::floor(std::min({triangle.a.x, triangle.b.x, triangle.c.x}))) - originX);
        const int top = std::max(0, int(std::floor(std::min({triangle.a.y, triangle.b.y, triangle.c.y}))) - originY);
        const int right = std::min(width, int(std::ceil(std::max({triangle.a.x, triangle.b.x, triangle.c.x}))) - originX);
        const int bottom = std::min(height, int(std::ceil(std::max({triangle.a.y, triangle.b.y, triangle.c.y}))) - originY);
        for (int y = top; y < bottom; ++y) {
            if ((y & 31) == 0 && cancelled && cancelled())
                throw RenderCancelled();
            auto* row = reinterpret_cast<QRgb*>(output.scanLine(y));
            for (int x = left; x < right; ++x) {
                const auto offset = std::size_t(y) * width + x;
                if (covered[offset])
                    continue;
                const MeshPoint point{originX + x + 0.5, originY + y + 0.5};
                const double wa = cross(triangle.b, triangle.c, point) / determinant;
                const double wb = cross(triangle.c, triangle.a, point) / determinant;
                const double wc = 1 - wa - wb;
                if (wa < -1e-10 || wb < -1e-10 || wc < -1e-10)
                    continue;
                covered[offset] = 1;
                const MeshPoint uv{wa * triangle.uvA.x + wb * triangle.uvB.x + wc * triangle.uvC.x,
                                   wa * triangle.uvA.y + wb * triangle.uvB.y + wc * triangle.uvC.y};
                row[x] = sample(source, uv);
            }
        }
    };
    for (int row = 0; row < binding.rows; ++row)
        for (int column = 0; column < binding.columns; ++column) {
            const auto index = std::size_t(row) * (binding.columns + 1) + column;
            const auto& a = binding.vertices[index];
            const auto& b = binding.vertices[index + 1];
            const auto& c = binding.vertices[index + binding.columns + 2];
            const auto& d = binding.vertices[index + binding.columns + 1];
            rasterize({a.pose, b.pose, c.pose, a.uv, b.uv, c.uv});
            rasterize({a.pose, c.pose, d.pose, a.uv, c.uv, d.uv});
        }
    return {std::move(output), {originX, originY}};
}
} // namespace opentoon
