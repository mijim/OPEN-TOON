#pragma once
#include "opentoon/document.h"
#include <span>

namespace opentoon {
struct PoseCaptureTarget {
    Id part = 0;
    std::uint16_t channels = 0;
};

Id captureCharacterPose(Document&, Id character, Frame, std::span<const PoseCaptureTarget>, std::string name);
void applyCharacterPose(Document&, Id character, Id pose, Frame);
void renameCharacterPose(Document&, Id character, Id pose, std::string name);
void removeCharacterPose(Document&, Id character, Id pose);
} // namespace opentoon
