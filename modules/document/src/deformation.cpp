#include "opentoon/deformation.h"
#include "opentoon/deformer.h"
#include "opentoon/drawing_selection.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace opentoon {
namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::invalid_argument(message);
}
bool bounded(MeshPoint point, double limit) {
    return std::isfinite(point.x) && std::isfinite(point.y) &&
           std::abs(point.x) <= limit && std::abs(point.y) <= limit;
}
double area(MeshPoint a, MeshPoint b, MeshPoint c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}
void triangles(const MeshBinding& binding, bool posed) {
    const auto point = [posed](const MeshVertex& vertex) {
        return posed ? vertex.pose : vertex.rest;
    };
    for (int row = 0; row < binding.rows; ++row)
        for (int column = 0; column < binding.columns; ++column) {
            const auto index = std::size_t(row) * (binding.columns + 1) + column;
            const auto a = point(binding.vertices[index]);
            const auto b = point(binding.vertices[index + 1]);
            const auto c = point(binding.vertices[index + binding.columns + 2]);
            const auto d = point(binding.vertices[index + binding.columns + 1]);
            require(area(a, b, c) > 1e-8 && area(a, c, d) > 1e-8,
                    "Mesh contains a degenerate or flipped triangle.");
        }
}
Layer& editablePart(Document& document, Id partId) {
    auto& layer = document.layer(partId);
    require(layer.kind == LayerKind::Part && !layer.locked,
            "Select an unlocked character part before editing its mesh.");
    return layer;
}
MeshBinding& bindingFor(Layer& layer, Id drawing) {
    auto found = std::find_if(layer.bindings.begin(), layer.bindings.end(),
                              [drawing](const MeshBinding& binding) {
                                  return binding.drawing == drawing;
                              });
    require(found != layer.bindings.end(), "This substitution has no mesh binding.");
    return *found;
}
bool posed(const MeshBinding& binding) {
    return std::any_of(binding.vertices.begin(), binding.vertices.end(),
                       [](const MeshVertex& vertex) { return vertex.pose != vertex.rest; });
}
void installGrid(Document& document, Layer& part, Id drawing, int columns, int rows,
                 int sourceWidth, int sourceHeight, int left, int top, int right, int bottom) {
    require(columns >= 1 && columns <= 32 && rows >= 1 && rows <= 32,
            "Mesh grid needs between one and 32 cells per axis.");
    require(part.bindings.size() < 256 || meshBindingFor(part, drawing),
            "Part exceeds its mesh-binding limit.");
    MeshBinding next;
    next.drawing = drawing;
    next.sourceWidth = sourceWidth;
    next.sourceHeight = sourceHeight;
    next.columns = columns;
    next.rows = rows;
    next.vertices.reserve(std::size_t(columns + 1) * (rows + 1));
    for (int row = 0; row <= rows; ++row)
        for (int column = 0; column <= columns; ++column) {
            const MeshPoint rest{left + (right - left) * double(column) / columns,
                                 top + (bottom - top) * double(row) / rows};
            const MeshPoint uv{rest.x / sourceWidth, rest.y / sourceHeight};
            next.vertices.push_back({rest, rest, uv});
        }
    validateMeshBinding(document, part, next);
    auto old = std::find_if(part.bindings.begin(), part.bindings.end(),
                            [drawing](const MeshBinding& binding) {
                                return binding.drawing == drawing;
                            });
    if (old != part.bindings.end()) {
        require(!posed(*old) && !old->bone && !old->curve,
                "Rebinding would discard a mesh pose or deformer animation; reset/remove it first.");
        *old = std::move(next);
    } else
        part.bindings.push_back(std::move(next));
}
} // namespace

const MeshBinding* meshBindingFor(const Layer& layer, Id drawing) {
    const auto found = std::find_if(layer.bindings.begin(), layer.bindings.end(),
                                    [drawing](const MeshBinding& binding) {
                                        return binding.drawing == drawing;
                                    });
    return found == layer.bindings.end() ? nullptr : &*found;
}

void validateMeshBinding(const Document& document, const Layer& part, const MeshBinding& binding) {
    require(part.kind == LayerKind::Part && binding.drawing != 0,
            "Only a character part may own a mesh binding.");
    require(std::any_of(part.variants.begin(), part.variants.end(),
                        [&](const Substitution& variant) {
                            return variant.drawing == binding.drawing;
                        }),
            "Mesh binding references a missing substitution.");
    const auto source = document.drawings.find(binding.drawing);
    require(source != document.drawings.end() && !source->second.raster &&
                (source->second.image.has_value() != !source->second.strokes.empty()),
            "This mesh profile requires image-only or vector-only artwork.");
    const auto& drawing = source->second;
    const int width = drawing.image ? drawing.image->width : document.width;
    const int height = drawing.image ? drawing.image->height : document.height;
    require(binding.sourceWidth == width && binding.sourceHeight == height &&
                width > 0 && height > 0 && width <= 4096 && height <= 4096,
            "Mesh binding does not match the substitution image dimensions.");
    if (!drawing.image)
        for (const auto& stroke : drawing.strokes) {
            const auto bounds = strokeBounds(stroke);
            require(bounds && bounds->x >= 0 && bounds->y >= 0 &&
                        std::int64_t(bounds->x) + bounds->width <= width &&
                        std::int64_t(bounds->y) + bounds->height <= height,
                    "Vector stroke exceeds the mesh render proxy.");
        }
    require(binding.columns >= 1 && binding.columns <= 32 &&
                binding.rows >= 1 && binding.rows <= 32 &&
                binding.vertices.size() ==
                    std::size_t(binding.columns + 1) * (binding.rows + 1),
            "Mesh grid is outside the bounded profile.");
    for (const auto& vertex : binding.vertices)
        require(bounded(vertex.rest, 1e6) && bounded(vertex.pose, 1e6) &&
                    bounded(vertex.uv, 1) && vertex.uv.x >= 0 && vertex.uv.y >= 0,
                "Mesh vertex or UV lies outside the supported coordinates.");
    triangles(binding, false);
    triangles(binding, true);
    double minX = binding.vertices.front().pose.x, maxX = minX;
    double minY = binding.vertices.front().pose.y, maxY = minY;
    for (const auto& vertex : binding.vertices) {
        minX = std::min(minX, vertex.pose.x);
        maxX = std::max(maxX, vertex.pose.x);
        minY = std::min(minY, vertex.pose.y);
        maxY = std::max(maxY, vertex.pose.y);
    }
    require(std::ceil(maxX) - std::floor(minX) <= 4096 &&
                std::ceil(maxY) - std::floor(minY) <= 4096,
            "Mesh pose exceeds its 4096-pixel render proxy.");
    validateMeshDeformer(document, binding);
}

void bindRegularImageMesh(Document& document, Id partId, Id drawing, int columns, int rows) {
    auto& part = editablePart(document, partId);
    const auto source = document.drawings.find(drawing);
    require(source != document.drawings.end() && source->second.image.has_value() &&
                source->second.strokes.empty() && !source->second.raster,
            "Bind an image-only substitution.");
    const auto& image = *source->second.image;
    int left = image.width, top = image.height, right = 0, bottom = 0;
    for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x)
            if (image.rgba[(std::size_t(y) * image.width + x) * 4 + 3] != 0) {
                left = std::min(left, x);
                top = std::min(top, y);
                right = std::max(right, x + 1);
                bottom = std::max(bottom, y + 1);
            }
    require(right > left && bottom > top, "Bind a substitution with visible image pixels.");
    installGrid(document, part, drawing, columns, rows, image.width, image.height,
                left, top, right, bottom);
}

void bindRegularVectorMesh(Document& document, Id partId, Id drawing, int columns, int rows) {
    auto& part = editablePart(document, partId);
    const auto source = document.drawings.find(drawing);
    require(source != document.drawings.end() && !source->second.image &&
                !source->second.raster && !source->second.strokes.empty(),
            "Bind a vector-only substitution.");
    int left = document.width, top = document.height, right = 0, bottom = 0;
    for (const auto& stroke : source->second.strokes)
        if (const auto bounds = strokeBounds(stroke)) {
            left = std::min(left, bounds->x);
            top = std::min(top, bounds->y);
            right = std::max(right, bounds->x + bounds->width);
            bottom = std::max(bottom, bounds->y + bounds->height);
        }
    require(left >= 0 && top >= 0 && right <= document.width &&
                bottom <= document.height && right > left && bottom > top,
            "Vector artwork exceeds the bounded mesh render proxy.");
    installGrid(document, part, drawing, columns, rows, document.width, document.height,
                left, top, right, bottom);
}

void moveMeshRestVertex(Document& document, Id partId, Id drawing, std::size_t index,
                        MeshPoint position) {
    auto& part = editablePart(document, partId);
    auto candidate = bindingFor(part, drawing);
    require(!candidate.bone && !candidate.curve,
            "Remove the deformer before editing mesh rest vertices.");
    require(index < candidate.vertices.size(), "Mesh vertex does not exist.");
    require(!posed(candidate), "Reset the mesh pose before editing its rest shape.");
    candidate.vertices[index].rest = position;
    candidate.vertices[index].pose = position;
    validateMeshBinding(document, part, candidate);
    bindingFor(part, drawing) = std::move(candidate);
}

void moveMeshPoseVertex(Document& document, Id partId, Id drawing, std::size_t index,
                        MeshPoint position) {
    auto& part = editablePart(document, partId);
    auto candidate = bindingFor(part, drawing);
    require(!candidate.bone && !candidate.curve,
            "Remove the deformer before editing static mesh pose vertices.");
    require(index < candidate.vertices.size(), "Mesh vertex does not exist.");
    candidate.vertices[index].pose = position;
    validateMeshBinding(document, part, candidate);
    bindingFor(part, drawing) = std::move(candidate);
}

void resetMeshPose(Document& document, Id partId, Id drawing) {
    auto& part = editablePart(document, partId);
    auto& binding = bindingFor(part, drawing);
    require(!binding.bone && !binding.curve,
            "Animated deformer keys need their own reset command.");
    for (auto& vertex : binding.vertices)
        vertex.pose = vertex.rest;
}

void removeMeshBinding(Document& document, Id partId, Id drawing) {
    auto& part = editablePart(document, partId);
    require(meshBindingFor(part, drawing), "This substitution has no mesh binding.");
    std::erase_if(part.bindings, [drawing](const MeshBinding& binding) {
        return binding.drawing == drawing;
    });
}
} // namespace opentoon
