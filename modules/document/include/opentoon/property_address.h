#pragma once
#include "opentoon/animation.h"
#include <span>
#include <string_view>

namespace opentoon {
enum class PropertyEntityKind : std::uint8_t { Layer, Character, Peg, Part };
enum class PropertyKind : std::uint8_t {
    PositionX,
    PositionY,
    Rotation,
    ScaleX,
    ScaleY,
    Opacity,
    PivotX,
    PivotY
};
enum class PropertySource : std::uint8_t { Rest, AuthoredKey, Evaluated };
struct PropertyAddress {
    PropertyEntityKind entity = PropertyEntityKind::Layer;
    Id layer = 0;
    PropertyKind kind = PropertyKind::PositionX;
    PropertyAddress() = default;
    PropertyAddress(Id layerId, PropertyKind property) : layer(layerId), kind(property) {}
    auto operator<=>(const PropertyAddress&) const = default;
};
struct PropertyEdit {
    PropertyAddress address;
    double value = 0;
};

[[nodiscard]] PropertyKind propertyKind(std::string_view channel);
[[nodiscard]] std::string_view propertyChannel(PropertyKind kind);
[[nodiscard]] double propertyValue(const Document&, PropertyAddress, Frame, PropertySource);
// All edits are checked before publication; callers use Session::apply for one undo entry.
void editProperties(Document&, std::span<const PropertyEdit>, Frame, AnimationEditMode, bool autokey);
void recordPropertyKey(Document&, PropertyAddress, Frame, double);
void editPropertyKey(Document&, PropertyAddress, Frame source, Frame destination, double, Interpolation);
void setPropertyEase(Document&, PropertyAddress, Frame, const BezierEase&);
} // namespace opentoon
