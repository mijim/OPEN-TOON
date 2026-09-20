#include "editor_controller.h"
#include <set>
using namespace opentoon;
QVariantList EditorController::selectedPoseFrames() const {
    QVariantList result;
    for (auto frame : poseSelection_)
        result.push_back(frame);
    return result;
}
void EditorController::setPoseSelection(std::vector<Frame> frames) {
    std::sort(frames.begin(), frames.end());
    frames.erase(std::unique(frames.begin(), frames.end()), frames.end());
    if (poseSelection_ == frames && poseSelectionLayer_ == layer_)
        return;
    poseSelection_ = std::move(frames);
    poseSelectionLayer_ = layer_;
    emit keySelectionChanged();
}
void EditorController::clearPoseSelection() {
    poseSelectionAnchor_ = -1;
    setPoseSelection({});
}
void EditorController::reconcilePoseSelection() {
    const auto layer = std::find_if(document().layers.begin(), document().layers.end(),
                                    [&](const auto& item) { return item.id == layer_; });
    if (poseSelectionLayer_ != layer_ || layer == document().layers.end()) {
        clearPoseSelection();
        return;
    }
    std::set<Frame> valid;
    for (const auto& key : layer->keys)
        valid.insert(key.frame);
    auto frames = poseSelection_;
    std::erase_if(frames, [&](auto frame) { return !valid.contains(frame); });
    if (!valid.contains(poseSelectionAnchor_))
        poseSelectionAnchor_ = -1;
    setPoseSelection(std::move(frames));
}
void EditorController::selectPoseKey(int frame, bool extend, bool toggle) {
    if (!layer_)
        return;
    const auto& keys = document().layer(layer_).keys;
    if (std::none_of(keys.begin(), keys.end(), [=](const auto& k) { return k.frame == frame; }))
        return;
    if (extend && poseSelectionAnchor_ >= 0)
        selectPoseRange(poseSelectionAnchor_, frame, toggle);
    else {
        auto frames = poseSelection_;
        auto found = std::find(frames.begin(), frames.end(), frame);
        if (toggle) {
            if (found == frames.end())
                frames.push_back(frame);
            else
                frames.erase(found);
        } else if (found == frames.end())
            frames = {frame};
        setPoseSelection(std::move(frames));
        poseSelectionAnchor_ = frame;
    }
    setFrame(frame);
}
void EditorController::selectPoseRange(int first, int last, bool additive) {
    if (!layer_)
        return;
    auto frames = additive ? poseSelection_ : std::vector<Frame>{};
    for (const auto& key : document().layer(layer_).keys)
        if (key.frame >= std::min(first, last) && key.frame <= std::max(first, last))
            frames.push_back(key.frame);
    setPoseSelection(std::move(frames));
}
void EditorController::copyPoseKeys() {
    if (!layer_)
        return;
    try {
        poseClipboard_ = copyKeyBlock(document().layer(layer_), poseSelection_);
        emit keySelectionChanged();
        report(QString("Copied %1 pose keys in local layer units (including pivots)")
                   .arg(poseClipboard_.keys.size()));
    } catch (const std::exception& e) {
        report(QString::fromUtf8(e.what()));
    }
}
bool EditorController::pastePoseKeys() {
    if (!layer_)
        return false;
    std::vector<Frame> result;
    if (!edit("Paste pose keys (local units)", [&](Document& d) {
            result = pasteKeyBlock(d.layer(layer_), poseClipboard_, frame_);
            d.duration = std::max(d.duration, result.back() + 1);
        }))
        return false;
    setPoseSelection(result);
    poseSelectionAnchor_ = result.front();
    return true;
}
bool EditorController::retimePoseSelection(int first, int last, bool duplicate) {
    if (!layer_)
        return false;
    const auto source = poseSelection_;
    const auto oldFrame = frame_;
    std::vector<Frame> result;
    if (!edit(duplicate ? "Duplicate pose keys" : "Retime pose keys", [&](Document& d) {
            result = retimeKeyBlock(d.layer(layer_), source, first, last, duplicate);
            d.duration = std::max(d.duration, result.back() + 1);
        }))
        return false;
    setPoseSelection(result);
    poseSelectionAnchor_ = result.front();
    auto current = std::find(source.begin(), source.end(), oldFrame);
    setFrame(current == source.end() ? result.front() : result[std::distance(source.begin(), current)]);
    return true;
}
bool EditorController::moveSelectedPoseKeys(int offset, bool duplicate) {
    if (poseSelection_.empty()) {
        report("Select pose keys to move.");
        return false;
    }
    const auto first = std::int64_t(poseSelection_.front()) + offset,
               last = std::int64_t(poseSelection_.back()) + offset;
    if (first < 0 || last >= 1000000) {
        report("Keys must remain within frames 1–1000000.");
        return false;
    }
    return retimePoseSelection(int(first), int(last), duplicate);
}
bool EditorController::stretchSelectedPoseKeys(int last) {
    if (poseSelection_.size() < 2) {
        report("Select at least two pose keys to stretch timing.");
        return false;
    }
    return retimePoseSelection(poseSelection_.front(), last, false);
}
bool EditorController::deleteSelectedPoseKeys() {
    if (!layer_)
        return false;
    const auto selected = poseSelection_;
    return edit("Delete selected pose keys", [&](Document& d) { deleteKeyBlock(d.layer(layer_), selected); });
}
