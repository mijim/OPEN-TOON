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
// Applies outgoing interpolation to selected full poses; easing presets skip a terminal key.
void interpolateKeyBlock(Layer&, const std::vector<Frame>&, int preset);
// Appends copies at an inclusive-span period; every destination is collision checked.
std::vector<Frame> repeatKeyBlock(Layer&, const std::vector<Frame>&, int copies);
void deleteKeyBlock(Layer&, const std::vector<Frame>& selection);
} // namespace opentoon
