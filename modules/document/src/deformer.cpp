#include "opentoon/deformer.h"
#include "opentoon/deformation.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace opentoon {
namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::invalid_argument(message);
}
MeshPoint add(MeshPoint a, MeshPoint b) { return {a.x + b.x, a.y + b.y}; }
MeshPoint sub(MeshPoint a, MeshPoint b) { return {a.x - b.x, a.y - b.y}; }
MeshPoint mul(MeshPoint a, double amount) { return {a.x * amount, a.y * amount}; }
double dot(MeshPoint a, MeshPoint b) { return a.x * b.x + a.y * b.y; }
double length(MeshPoint a) { return std::hypot(a.x, a.y); }
bool finite(MeshPoint a) {
    return std::isfinite(a.x) && std::isfinite(a.y) &&
           std::abs(a.x) <= 1e6 && std::abs(a.y) <= 1e6;
}
MeshPoint rotate(MeshPoint point, double angle) {
    const double radians = angle * std::numbers::pi / 180;
    const double cosine = std::cos(radians), sine = std::sin(radians);
    return {cosine * point.x - sine * point.y, sine * point.x + cosine * point.y};
}
double projection(MeshPoint p, MeshPoint a, MeshPoint b) {
    const auto segment = sub(b, a);
    return std::clamp(dot(sub(p, a), segment) / dot(segment, segment), 0.0, 1.0);
}
double distanceToSegment(MeshPoint p, MeshPoint a, MeshPoint b) {
    return length(sub(p, add(a, mul(sub(b, a), projection(p, a, b)))));
}
MeshPoint cubic(const std::array<MeshPoint, 4>& c, double t) {
    const double u = 1 - t;
    return add(add(mul(c[0], u * u * u), mul(c[1], 3 * u * u * t)),
               add(mul(c[2], 3 * u * t * t), mul(c[3], t * t * t)));
}
CurveCoordinate coordinate(MeshPoint vertex, const std::array<MeshPoint, 4>& rest) {
    double best = 0, distance = 1e100;
    for (int sample = 0; sample <= 64; ++sample) {
        const double t = double(sample) / 64;
        const auto delta = sub(vertex, cubic(rest, t));
        const double squared = dot(delta, delta);
        if (squared < distance) {
            distance = squared;
            best = t;
        }
    }
    double left = std::max(0.0, best - 1.0 / 64);
    double right = std::min(1.0, best + 1.0 / 64);
    for (int step = 0; step < 16; ++step) {
        const double a = left + (right - left) / 3;
        const double b = right - (right - left) / 3;
        const auto da = sub(vertex, cubic(rest, a));
        const auto db = sub(vertex, cubic(rest, b));
        if (dot(da, da) < dot(db, db))
            right = b;
        else
            left = a;
    }
    return {(left + right) / 2};
}
void editable(const Document& document, Id part, Id drawing) {
    const auto& layer = document.layer(part);
    require(layer.kind == LayerKind::Part && !layer.locked && meshBindingFor(layer, drawing),
            "Select an unlocked bound character part.");
}
MeshBinding& binding(Document& document, Id part, Id drawing) {
    auto& layer = document.layer(part);
    auto found = std::find_if(layer.bindings.begin(), layer.bindings.end(),
                              [drawing](const MeshBinding& value) {
                                  return value.drawing == drawing;
                              });
    require(found != layer.bindings.end(), "Drawing has no mesh binding.");
    return *found;
}
template <typename Key> void validateKeys(const Document& document, const std::vector<Key>& keys) {
    require(keys.size() <= 10000, "Too many deformer keys.");
    Frame previous = -1;
    for (const auto& key : keys) {
        require(key.frame > previous && key.frame < document.duration &&
                    static_cast<int>(key.interpolation) >= 0 &&
                    static_cast<int>(key.interpolation) <= 2,
                "Deformer keys are invalid or out of order.");
        previous = key.frame;
    }
}
template <typename Key, typename Value> Value sampled(const std::vector<Key>& keys,
                                                        Frame frame, Value rest,
                                                        const auto& valueOf,
                                                        const auto& blend) {
    if (keys.empty() || frame < keys.front().frame)
        return rest;
    const auto right = std::upper_bound(keys.begin(), keys.end(), frame,
                                        [](Frame f, const Key& key) { return f < key.frame; });
    const auto& left = *(right - 1);
    if (right == keys.end() || left.interpolation == Interpolation::Step)
        return valueOf(left);
    double t = double(frame - left.frame) / (right->frame - left.frame);
    if (left.interpolation == Interpolation::Smooth)
        t = t * t * (3 - 2 * t);
    return blend(valueOf(left), valueOf(*right), t);
}
double area(MeshPoint a, MeshPoint b, MeshPoint c) {
    const auto ab = sub(b, a), ac = sub(c, a);
    return ab.x * ac.y - ab.y * ac.x;
}
void validateEvaluatedPose(const MeshBinding& binding, Frame frame) {
    const auto evaluated = evaluateMeshBinding(binding, frame);
    double minX = 1e100, minY = 1e100, maxX = -1e100, maxY = -1e100;
    for (const auto& vertex : evaluated.vertices) {
        require(finite(vertex.pose), "Evaluated mesh vertex is invalid.");
        minX = std::min(minX, vertex.pose.x);
        minY = std::min(minY, vertex.pose.y);
        maxX = std::max(maxX, vertex.pose.x);
        maxY = std::max(maxY, vertex.pose.y);
    }
    require(std::ceil(maxX) - std::floor(minX) <= 4096 &&
                std::ceil(maxY) - std::floor(minY) <= 4096,
            "Animated mesh exceeds its bounded render proxy.");
    for (int row = 0; row < binding.rows; ++row)
        for (int column = 0; column < binding.columns; ++column) {
            const auto index = std::size_t(row) * (binding.columns + 1) + column;
            const auto a = evaluated.vertices[index].pose;
            const auto b = evaluated.vertices[index + 1].pose;
            const auto c = evaluated.vertices[index + binding.columns + 2].pose;
            const auto d = evaluated.vertices[index + binding.columns + 1].pose;
            require(area(a, b, c) > 1e-8 && area(a, c, d) > 1e-8,
                    "Animated mesh folds or collapses at a key.");
        }
}
} // namespace

void validateMeshDeformer(const Document& document, const MeshBinding& binding) {
    require(!(binding.bone && binding.curve), "A mesh cannot have two deformers.");
    for (const auto& vertex : binding.vertices)
        require(vertex.pose == vertex.rest || (!binding.bone && !binding.curve),
                "Reset the static mesh pose before adding a deformer.");
    if (binding.bone) {
        const auto& bone = *binding.bone;
        for (auto joint : bone.restJoints)
            require(finite(joint), "Bone rest joint is nonfinite or unbounded.");
        const double first = length(sub(bone.restJoints[1], bone.restJoints[0]));
        const double second = length(sub(bone.restJoints[2], bone.restJoints[1]));
        require(first > 1e-6 && second > 1e-6 &&
                    std::isfinite(bone.elbowTransition) && bone.elbowTransition > 0 &&
                    bone.elbowTransition <= first + second,
                "Bone chain has a zero-length segment or invalid influence transition.");
        require(bone.distalWeights.size() == binding.vertices.size(),
                "Bone weight count does not match the mesh.");
        for (double weight : bone.distalWeights)
            require(std::isfinite(weight) && weight >= 0 && weight <= 1,
                    "Bone weight must be finite and normalized.");
        validateKeys(document, bone.keys);
        for (const auto& key : bone.keys)
            require(std::isfinite(key.shoulderAngle) && std::isfinite(key.elbowAngle) &&
                        std::abs(key.shoulderAngle) <= 720 && std::abs(key.elbowAngle) <= 720,
                    "Bone pose angle is nonfinite or out of range.");
    }
    if (binding.curve) {
        const auto& curve = *binding.curve;
        for (auto point : curve.restControls)
            require(finite(point), "Curve rest control is nonfinite or unbounded.");
        require(length(sub(curve.restControls[3], curve.restControls[0])) > 1e-6,
                "Curve rest endpoints coincide.");
        require(curve.coordinates.size() == binding.vertices.size(),
                "Curve coordinate count does not match the mesh.");
        for (const auto& coordinate : curve.coordinates)
            require(std::isfinite(coordinate.t) && coordinate.t >= 0 && coordinate.t <= 1,
                    "Curve rest coordinate is invalid.");
        validateKeys(document, curve.keys);
        for (const auto& key : curve.keys)
            for (auto point : key.controls)
                require(finite(point), "Curve key control is nonfinite or unbounded.");
    }
    auto validateTimes = [&](const auto& keys) {
        for (std::size_t i = 0; i < keys.size(); ++i) {
            validateEvaluatedPose(binding, keys[i].frame);
            if (i + 1 < keys.size() && keys[i + 1].frame - keys[i].frame > 1)
                validateEvaluatedPose(binding,
                                       keys[i].frame + (keys[i + 1].frame - keys[i].frame) / 2);
        }
    };
    if (binding.bone)
        validateTimes(binding.bone->keys);
    if (binding.curve)
        validateTimes(binding.curve->keys);
}

void bindBoneChain(Document& document, Id part, Id drawing,
                   std::array<MeshPoint, 3> restJoints, double elbowTransition) {
    editable(document, part, drawing);
    auto candidate = binding(document, part, drawing);
    require(!candidate.bone && !candidate.curve,
            "Remove the existing deformer before rebinding controls.");
    require(std::all_of(candidate.vertices.begin(), candidate.vertices.end(),
                        [](const MeshVertex& v) { return v.pose == v.rest; }),
            "Reset the static mesh pose before adding a deformer.");
    BoneChain bone;
    bone.restJoints = restJoints;
    bone.elbowTransition = elbowTransition;
    const double first = length(sub(restJoints[1], restJoints[0]));
    const double second = length(sub(restJoints[2], restJoints[1]));
    require(first > 1e-6 && second > 1e-6 && std::isfinite(elbowTransition) &&
                elbowTransition > 0, "Bone chain has invalid rest geometry.");
    bone.distalWeights.reserve(candidate.vertices.size());
    for (const auto& vertex : candidate.vertices) {
        const auto position = vertex.rest;
        const double a = distanceToSegment(position, restJoints[0], restJoints[1]);
        const double b = distanceToSegment(position, restJoints[1], restJoints[2]);
        const double arc = a <= b ? projection(position, restJoints[0], restJoints[1]) * first
                                  : first + projection(position, restJoints[1], restJoints[2]) * second;
        double weight = std::clamp((arc - first + elbowTransition) /
                                       (2 * elbowTransition), 0.0, 1.0);
        weight = weight * weight * (3 - 2 * weight);
        bone.distalWeights.push_back(weight);
    }
    candidate.bone = std::move(bone);
    validateMeshDeformer(document, candidate);
    binding(document, part, drawing) = std::move(candidate);
}

void recordBonePose(Document& document, Id part, Id drawing, Frame frame,
                    double shoulderAngle, double elbowAngle, Interpolation interpolation) {
    editable(document, part, drawing);
    require(frame >= 0 && frame < document.duration, "Bone key frame is outside the scene.");
    auto candidate = binding(document, part, drawing);
    require(candidate.bone.has_value(), "This mesh has no bone chain.");
    auto& keys = candidate.bone->keys;
    if (keys.empty() && frame > 0)
        keys.push_back({0, 0, 0, Interpolation::Linear});
    const BonePoseKey key{frame, shoulderAngle, elbowAngle, interpolation};
    auto found = std::lower_bound(keys.begin(), keys.end(), frame,
                                  [](const BonePoseKey& item, Frame f) { return item.frame < f; });
    if (found != keys.end() && found->frame == frame)
        *found = key;
    else
        keys.insert(found, key);
    validateMeshDeformer(document, candidate);
    binding(document, part, drawing) = std::move(candidate);
}

void bindCurveDeformer(Document& document, Id part, Id drawing,
                       std::array<MeshPoint, 4> restControls) {
    editable(document, part, drawing);
    auto candidate = binding(document, part, drawing);
    require(!candidate.bone && !candidate.curve,
            "Remove the existing deformer before rebinding controls.");
    require(std::all_of(candidate.vertices.begin(), candidate.vertices.end(),
                        [](const MeshVertex& v) { return v.pose == v.rest; }),
            "Reset the static mesh pose before adding a deformer.");
    CurveDeformer curve;
    curve.restControls = restControls;
    require(length(sub(restControls[3], restControls[0])) > 1e-6,
            "Curve rest endpoints coincide.");
    for (const auto& vertex : candidate.vertices)
        curve.coordinates.push_back(coordinate(vertex.rest, restControls));
    candidate.curve = std::move(curve);
    validateMeshDeformer(document, candidate);
    binding(document, part, drawing) = std::move(candidate);
}

void recordCurvePose(Document& document, Id part, Id drawing, Frame frame,
                     std::array<MeshPoint, 4> controls, Interpolation interpolation) {
    editable(document, part, drawing);
    require(frame >= 0 && frame < document.duration, "Curve key frame is outside the scene.");
    auto candidate = binding(document, part, drawing);
    require(candidate.curve.has_value(), "This mesh has no curve deformer.");
    auto& keys = candidate.curve->keys;
    if (keys.empty() && frame > 0)
        keys.push_back({0, candidate.curve->restControls, Interpolation::Linear});
    const CurvePoseKey key{frame, controls, interpolation};
    auto found = std::lower_bound(keys.begin(), keys.end(), frame,
                                  [](const CurvePoseKey& item, Frame f) { return item.frame < f; });
    if (found != keys.end() && found->frame == frame)
        *found = key;
    else
        keys.insert(found, key);
    validateMeshDeformer(document, candidate);
    binding(document, part, drawing) = std::move(candidate);
}

void removeMeshDeformer(Document& document, Id part, Id drawing) {
    editable(document, part, drawing);
    auto& target = binding(document, part, drawing);
    require(target.bone || target.curve, "This mesh has no deformer to remove.");
    target.bone.reset();
    target.curve.reset();
}

bool hasDeformerKeys(const Layer& layer, Frame start, Frame end) {
    for (const auto& binding : layer.bindings) {
        auto contains = [start, end](const auto& keys) {
            return std::any_of(keys.begin(), keys.end(), [start, end](const auto& key) {
                return key.frame >= start && key.frame < end;
            });
        };
        if ((binding.bone && contains(binding.bone->keys)) ||
            (binding.curve && contains(binding.curve->keys)))
            return true;
    }
    return false;
}

std::array<double, 2> sampleBoneAngles(const BoneChain& bone, Frame frame) {
    return sampled<BonePoseKey, std::array<double, 2>>(
            bone.keys, frame, {0, 0},
            [](const BonePoseKey& key) {
                return std::array<double, 2>{key.shoulderAngle, key.elbowAngle};
            },
            [](std::array<double, 2> a, std::array<double, 2> b, double t) {
                return std::array<double, 2>{a[0] + (b[0] - a[0]) * t,
                                             a[1] + (b[1] - a[1]) * t};
            });
}

std::array<MeshPoint, 4> sampleCurveControls(const CurveDeformer& curve, Frame frame) {
    return sampled<CurvePoseKey, std::array<MeshPoint, 4>>(
            curve.keys, frame, curve.restControls,
            [](const CurvePoseKey& key) { return key.controls; },
            [](std::array<MeshPoint, 4> a, std::array<MeshPoint, 4> b, double t) {
                for (std::size_t i = 0; i < 4; ++i)
                    a[i] = add(mul(a[i], 1 - t), mul(b[i], t));
                return a;
            });
}

MeshBinding evaluateMeshBinding(const MeshBinding& binding, Frame frame) {
    auto evaluated = binding;
    if (binding.bone) {
        const auto& bone = *binding.bone;
        const auto angles = sampleBoneAngles(bone, frame);
        if (angles == std::array<double, 2>{0, 0})
            return evaluated;
        const auto& joints = bone.restJoints;
        const auto posedElbow = add(joints[0], rotate(sub(joints[1], joints[0]), angles[0]));
        for (std::size_t i = 0; i < evaluated.vertices.size(); ++i) {
            const auto rest = evaluated.vertices[i].rest;
            const auto proximal = add(joints[0], rotate(sub(rest, joints[0]), angles[0]));
            const auto distal = add(posedElbow,
                                    rotate(sub(rest, joints[1]), angles[0] + angles[1]));
            evaluated.vertices[i].pose = add(mul(proximal, 1 - bone.distalWeights[i]),
                                              mul(distal, bone.distalWeights[i]));
        }
    } else if (binding.curve) {
        const auto& curve = *binding.curve;
        const auto controls = sampleCurveControls(curve, frame);
        if (controls == curve.restControls)
            return evaluated;
        for (std::size_t i = 0; i < evaluated.vertices.size(); ++i) {
            const auto& value = curve.coordinates[i];
            evaluated.vertices[i].pose = add(evaluated.vertices[i].rest,
                                              sub(cubic(controls, value.t),
                                                  cubic(curve.restControls, value.t)));
        }
    }
    return evaluated;
}
} // namespace opentoon
