#pragma once
#include "opentoon/document.h"
namespace opentoon {
enum class PasteContent { Exposures, IndependentDrawings, Keys, All };
struct DeformerClipboardKeys {
    Id drawing = 0;
    std::optional<BoneChain> boneBasis;
    std::optional<CurveDeformer> curveBasis;
    std::vector<BonePoseKey> bone;
    std::vector<CurvePoseKey> curve;
};
struct ClipboardTrack {
    Id sourceLayer = 0;
    std::vector<Exposure> exposures;
    std::vector<Id> boundDrawings;
    std::vector<Keyframe> keys;
    std::vector<DeformerClipboardKeys> deformerKeys;
};
struct ExposureClipboard {
    Frame duration = 0;
    std::vector<ClipboardTrack> tracks;
    std::map<Id, Drawing> drawings;
    std::vector<Swatch> palette;
};
// All ranges are half-open. Commands call these operations inside Session::apply.
[[nodiscard]] ExposureClipboard copyRange(const Document&, const std::vector<Id>& layers, Frame start,
                                          Frame end);
void pasteRange(Document&, const std::vector<Id>& layers, Frame at, const ExposureClipboard&, PasteContent,
                bool insert = false, bool allowLinkedDrawings = true);
void clearRange(Document&, const std::vector<Id>& layers, Frame start, Frame end, bool keys = false);
void repeatRange(Document&, const std::vector<Id>& layers, Frame start, Frame end, int repeats);
void retimeRange(Document&, const std::vector<Id>& layers, Frame start, Frame end, Frame newLength);
void timeDrawings(Document&, const std::vector<Id>& layers, Frame start, Frame end, int framesPerDrawing);
void setMarker(Document&, Frame frame, const std::string& name);
} // namespace opentoon
