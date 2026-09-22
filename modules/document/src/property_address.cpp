#include "opentoon/property_address.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

namespace opentoon {
namespace {
std::string_view layerChannel(const Document& document, PropertyAddress address) {
    const auto kind = document.layer(address.layer).kind;
    const bool compatible = address.entity == PropertyEntityKind::Layer ||
                            (address.entity == PropertyEntityKind::Character && kind == LayerKind::Character) ||
                            (address.entity == PropertyEntityKind::Peg && kind == LayerKind::Peg) ||
                            (address.entity == PropertyEntityKind::Part && kind == LayerKind::Part);
    if (!compatible)
        throw std::invalid_argument("Property entity kind does not match its stable identity.");
    return propertyChannel(address.kind);
}
} // namespace
PropertyKind propertyKind(std::string_view channel) {
    if (channel == "x")
        return PropertyKind::PositionX;
    if (channel == "y")
        return PropertyKind::PositionY;
    if (channel == "rotation")
        return PropertyKind::Rotation;
    if (channel == "scaleX")
        return PropertyKind::ScaleX;
    if (channel == "scaleY")
        return PropertyKind::ScaleY;
    if (channel == "opacity")
        return PropertyKind::Opacity;
    if (channel == "pivotX")
        return PropertyKind::PivotX;
    if (channel == "pivotY")
        return PropertyKind::PivotY;
    throw std::invalid_argument("Unknown transform property.");
}
std::string_view propertyChannel(PropertyKind kind) {
    switch (kind) {
    case PropertyKind::PositionX:
        return "x";
    case PropertyKind::PositionY:
        return "y";
    case PropertyKind::Rotation:
        return "rotation";
    case PropertyKind::ScaleX:
        return "scaleX";
    case PropertyKind::ScaleY:
        return "scaleY";
    case PropertyKind::Opacity:
        return "opacity";
    case PropertyKind::PivotX:
        return "pivotX";
    case PropertyKind::PivotY:
        return "pivotY";
    }
    throw std::invalid_argument("Unknown transform property.");
}
double propertyValue(const Document& document, PropertyAddress address, Frame frame, PropertySource source) {
    const auto channel = layerChannel(document, address);
    const auto& layer = document.layer(address.layer);
    if (source == PropertySource::Rest)
        return transformValue(layer.transform, channel);
    if (frame < 0 || frame >= document.duration)
        throw std::invalid_argument("Property frame is outside the scene.");
    if (source == PropertySource::Evaluated)
        return transformValue(evaluateTransform(layer, frame), channel);
    if (source == PropertySource::AuthoredKey) {
        const auto key = std::find_if(layer.keys.begin(), layer.keys.end(),
                                      [frame](const auto& item) { return item.frame == frame; });
        if (key == layer.keys.end())
            throw std::invalid_argument("No authored key at this frame.");
        return transformValue(key->value, channel);
    }
    throw std::invalid_argument("Unknown property source.");
}
void editProperties(Document& document, std::span<const PropertyEdit> edits, Frame frame,
                    AnimationEditMode mode, bool autokey) {
    if (frame < 0 || frame >= document.duration)
        throw std::invalid_argument("Property frame is outside the scene.");
    if (edits.empty())
        return;
    std::set<PropertyAddress> unique;
    std::set<Id> anchorLayers;
    for (const auto& edit : edits) {
        (void)layerChannel(document, edit.address);
        if (!unique.insert(edit.address).second)
            throw std::invalid_argument("Duplicate property in one edit.");
        const auto& layer = document.layer(edit.address.layer);
        if (layer.locked)
            throw std::invalid_argument("Unlock the layer before editing.");
        if (!std::isfinite(edit.value) || std::abs(edit.value) > 10000000 ||
            (edit.address.kind == PropertyKind::Opacity && (edit.value < 0 || edit.value > 1)))
            throw std::invalid_argument("Property value is outside the supported range.");
        if (mode == AnimationEditMode::Animate && !autokey &&
            std::none_of(layer.keys.begin(), layer.keys.end(),
                         [frame](const auto& key) { return key.frame == frame; }))
            throw std::invalid_argument("Add a key or enable Auto key before editing an unkeyed frame.");
        if (mode == AnimationEditMode::Animate && autokey && frame > 0 && layer.keys.empty())
            anchorLayers.insert(layer.id);
    }
    for (Id id : anchorLayers)
        recordPose(document.layer(id), 0, document.layer(id).transform);
    for (const auto& edit : edits)
        editTransform(document.layer(edit.address.layer), frame, layerChannel(document, edit.address), edit.value, mode,
                      autokey);
}
void recordPropertyKey(Document& document, PropertyAddress address, Frame frame, double value) {
    const auto channel = layerChannel(document, address);
    auto& layer = document.layer(address.layer);
    if (layer.locked)
        throw std::invalid_argument("Unlock the layer before editing.");
    if (frame < 0 || frame >= document.duration)
        throw std::invalid_argument("Property frame is outside the scene.");
    auto pose = evaluateTransform(layer, frame);
    setTransformValue(pose, channel, value);
    recordPose(layer, frame, pose);
}
void editPropertyKey(Document& document, PropertyAddress address, Frame source, Frame destination,
                     double value, Interpolation interpolation) {
    if (document.layer(address.layer).locked)
        throw std::invalid_argument("Unlock the layer before editing.");
    editKey(document.layer(address.layer), source, destination, layerChannel(document, address), value, interpolation);
}
void setPropertyEase(Document& document, PropertyAddress address, Frame frame, const BezierEase& ease) {
    if (document.layer(address.layer).locked)
        throw std::invalid_argument("Unlock the layer before editing.");
    setKeyEase(document.layer(address.layer), frame, layerChannel(document, address), ease);
}
} // namespace opentoon
