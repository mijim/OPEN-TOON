#pragma once
#include "opentoon/document.h"
namespace opentoon {
// Clipboard frames are relative to the first selected key. Poses remain in local layer units.
struct KeyBlock {
    std::vector<Keyframe> keys;
};
KeyBlock copyKeyBlock(const Layer&, const std::vector<Frame>& selection);
std::vector<Frame> pasteKeyBlock(Layer&, const KeyBlock&, Frame destination);
// Inclusive endpoints; nearest-frame rounding. Collisions reject the entire edit.
std::vector<Frame> retimeKeyBlock(Layer&, const std::vector<Frame>& selection, Frame first, Frame last,
                                  bool duplicate = false);
void deleteKeyBlock(Layer&, const std::vector<Frame>& selection);
} // namespace opentoon
