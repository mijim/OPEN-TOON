#pragma once
#include "opentoon/document.h"
#include <string_view>
namespace opentoon {
enum class AnimationEditMode { Setup, Animate };
[[nodiscard]] double transformValue(const Transform&, std::string_view channel);
void setTransformValue(Transform&, std::string_view channel, double value);
// A visual pose gesture records one full pose; the first later-frame edit anchors frame zero.
void recordPose(Layer&, Frame, const Transform&);
void movePoseKey(Layer&, Frame source, Frame destination);
void setPoseEase(Layer&, Frame, const BezierEase&);
void setKeyEase(Layer&, Frame, std::string_view channel, const BezierEase&);
void validateEase(std::string_view channel, const BezierEase&);
double evaluateEase(const BezierEase&, double time);
void editTransform(Layer&, Frame, std::string_view channel, double value, AnimationEditMode, bool autokey);
void editKey(Layer&, Frame source, Frame destination, std::string_view channel, double value, Interpolation);
// Maps inclusive key endpoints within a half-open selection to inclusive destination endpoints.
// Collisions are rejected; exposures, markers and unselected keys never move.
void retimeKeys(Document&, const std::vector<Id>&, Frame start, Frame end, Frame destination, Frame length);
} // namespace opentoon
