#include "opentoon/animation.h"
#include <cmath>
#include <set>
#include <stdexcept>
namespace opentoon {
namespace {
double Transform::* member(std::string_view channel) {
    if (channel == "x")
        return &Transform::x;
    if (channel == "y")
        return &Transform::y;
    if (channel == "rotation")
        return &Transform::rotation;
    if (channel == "scaleX")
        return &Transform::scaleX;
    if (channel == "scaleY")
        return &Transform::scaleY;
    if (channel == "opacity")
        return &Transform::opacity;
    if (channel == "pivotX")
        return &Transform::pivotX;
    if (channel == "pivotY")
        return &Transform::pivotY;
    throw std::invalid_argument("Unknown transform channel.");
}
void editable(const Layer& layer) {
    if (layer.locked)
        throw std::runtime_error("Unlock the layer before editing.");
}
} // namespace
double transformValue(const Transform& t, std::string_view channel) {
    return t.*member(channel);
}
void setTransformValue(Transform& t, std::string_view channel, double value) {
    if (!std::isfinite(value))
        throw std::invalid_argument("Transform values must be finite.");
    t.*member(channel) = value;
}
void editTransform(Layer& layer, Frame frame, std::string_view channel, double value, AnimationEditMode mode,
                   bool autokey) {
    editable(layer);
    if (mode == AnimationEditMode::Setup) {
        setTransformValue(layer.transform, channel, value);
        return;
    }
    auto key =
        std::find_if(layer.keys.begin(), layer.keys.end(), [=](const auto& k) { return k.frame == frame; });
    if (key != layer.keys.end()) {
        setTransformValue(key->value, channel, value);
        return;
    }
    if (!autokey)
        throw std::runtime_error("Add a key or enable Auto key before editing an unkeyed frame.");
    auto pose = evaluateTransform(layer, frame);
    setTransformValue(pose, channel, value);
    layer.keys.push_back({frame, pose, Interpolation::Linear});
    std::sort(layer.keys.begin(), layer.keys.end(),
              [](const auto& a, const auto& b) { return a.frame < b.frame; });
}
void editKey(Layer& layer, Frame source, Frame destination, std::string_view channel, double value,
             Interpolation interpolation) {
    editable(layer);
    if (destination < 0 || destination >= 1000000 || int(interpolation) < 0 || int(interpolation) > 2)
        throw std::invalid_argument("Invalid key timing or interpolation.");
    auto key =
        std::find_if(layer.keys.begin(), layer.keys.end(), [=](const auto& k) { return k.frame == source; });
    if (key == layer.keys.end())
        throw std::runtime_error("The selected key no longer exists.");
    if (destination != source && std::any_of(layer.keys.begin(), layer.keys.end(),
                                             [=](const auto& k) { return k.frame == destination; }))
        throw std::runtime_error("Another key already occupies the destination frame.");
    setTransformValue(key->value, channel, value);
    key->frame = destination;
    key->interpolation = interpolation;
    std::sort(layer.keys.begin(), layer.keys.end(),
              [](const auto& a, const auto& b) { return a.frame < b.frame; });
}
void retimeKeys(Document& d, const std::vector<Id>& ids, Frame start, Frame end, Frame destination,
                Frame length) {
    if (start < 0 || end <= start || end > d.duration || destination < 0 || length < 1 ||
        std::int64_t(destination) + length > 1000000)
        throw std::invalid_argument("Invalid key range.");
    std::map<Id, std::vector<Keyframe>> updates;
    Frame last = d.duration - 1;
    for (auto id : ids) {
        const auto& layer = d.layer(id);
        editable(layer);
        auto keys = layer.keys;
        std::set<Frame> occupied;
        for (auto& key : keys) {
            if (key.frame >= start && key.frame < end) {
                key.frame =
                    destination +
                    (end - start == 1
                         ? 0
                         : Frame(std::llround(double(key.frame - start) * (length - 1) / (end - start - 1))));
                last = std::max(last, key.frame);
            }
            if (!occupied.insert(key.frame).second)
                throw std::runtime_error("Retiming would merge keys. Choose a wider or unoccupied range.");
        }
        std::sort(keys.begin(), keys.end(), [](const auto& a, const auto& b) { return a.frame < b.frame; });
        updates.emplace(id, std::move(keys));
    }
    for (auto& [id, keys] : updates)
        d.layer(id).keys = std::move(keys);
    d.duration = last + 1;
}
} // namespace opentoon
