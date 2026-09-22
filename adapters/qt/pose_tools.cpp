#include "editor_controller.h"
#include "opentoon/property_address.h"
#include <array>

using namespace opentoon;
namespace {
constexpr std::array<PropertyKind, 8> properties{
    PropertyKind::PositionX, PropertyKind::PositionY, PropertyKind::Rotation, PropertyKind::ScaleX,
    PropertyKind::ScaleY,    PropertyKind::Opacity,   PropertyKind::PivotX,   PropertyKind::PivotY};

bool includes(int mode, PropertyKind kind) {
    switch (mode) {
    case 0:
    case 6:
    case 7:
        return true;
    case 1:
        return kind == PropertyKind::PositionX || kind == PropertyKind::PositionY;
    case 2:
        return kind == PropertyKind::Rotation;
    case 3:
        return kind == PropertyKind::ScaleX || kind == PropertyKind::ScaleY;
    case 4:
        return kind == PropertyKind::Opacity;
    case 5:
        return kind == PropertyKind::PivotX || kind == PropertyKind::PivotY;
    default:
        return false;
    }
}
std::vector<PropertyEdit> values(Id layer, const Transform& pose, int mode) {
    std::vector<PropertyEdit> result;
    for (const auto kind : properties) {
        if (!includes(mode, kind))
            continue;
        double value = transformValue(pose, propertyChannel(kind));
        if ((mode == 6 && kind == PropertyKind::ScaleX) || (mode == 7 && kind == PropertyKind::ScaleY))
            value = -value;
        result.push_back({{layer, kind}, value});
    }
    return result;
}
} // namespace

void EditorController::copyTransformPose() {
    if (!layer_) {
        report("Select a layer before copying a pose.");
        return;
    }
    const auto& layer = document().layer(layer_);
    transformClipboard_ = animateMode_ ? evaluateTransform(layer, frame_) : layer.transform;
    emit poseClipboardChanged();
    report(animateMode_ ? "Copied evaluated layer pose." : "Copied setup transform.");
}
bool EditorController::pasteTransformPose(int mode) {
    if (!layer_ || !transformClipboard_ || mode < 0 || mode > 7) {
        report("Copy a layer pose before pasting.");
        return false;
    }
    const auto edits = values(layer_, *transformClipboard_, mode);
    return edit("Paste layer pose", [&](Document& document) {
        editProperties(document, edits, frame_,
                       animateMode_ ? AnimationEditMode::Animate : AnimationEditMode::Setup,
                       true); // An explicit paste authors a key without requiring Auto key.
    });
}
bool EditorController::resetTransformPose() {
    if (!layer_) {
        report("Select a layer before resetting its pose.");
        return false;
    }
    return edit("Reset layer pose", [&](Document& document) {
        const Transform target = animateMode_ ? document.layer(layer_).transform : Transform{};
        const auto edits = values(layer_, target, 0);
        editProperties(document, edits, frame_,
                       animateMode_ ? AnimationEditMode::Animate : AnimationEditMode::Setup, true);
    });
}
