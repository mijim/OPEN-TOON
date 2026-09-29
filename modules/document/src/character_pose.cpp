#include "opentoon/character_pose.h"
#include "opentoon/animation.h"
#include "opentoon/property_address.h"
#include "opentoon/rigging.h"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace opentoon {
namespace {
Layer& character(Document& document, Id id) {
    auto& root = document.layer(id);
    if (root.kind != LayerKind::Character || root.locked)
        throw std::invalid_argument("Select an unlocked character root.");
    return root;
}
CharacterPose& pose(Document& document, Id root, Id id) {
    auto& poses = character(document, root).poses;
    const auto found = std::find_if(poses.begin(), poses.end(),
                                    [id](const auto& item) { return item.id == id; });
    if (found == poses.end())
        throw std::invalid_argument("Character pose does not exist.");
    return *found;
}
void nameAvailable(const Layer& root, const std::string& name, Id except = 0) {
    if (name.empty() || name.size() > 128 ||
        std::any_of(root.poses.begin(), root.poses.end(), [&](const auto& item) {
            return item.id != except && item.name == name;
        }))
        throw std::invalid_argument("Character pose name is invalid or already used.");
}
void checkedPart(const Document& document, Id root, Id partId) {
    const auto& part = document.layer(partId);
    if (part.kind != LayerKind::Part || part.locked || characterFor(document, partId) != root)
        throw std::invalid_argument("Pose target must be an unlocked part of this character.");
}
double channelValue(const Transform& value, unsigned index) {
    switch (index) {
    case 0: return value.x;
    case 1: return value.y;
    case 2: return value.rotation;
    case 3: return value.scaleX;
    case 4: return value.scaleY;
    case 5: return value.opacity;
    case 6: return value.pivotX;
    default: return value.pivotY;
    }
}
} // namespace

Id captureCharacterPose(Document& document, Id rootId, Frame frame,
                        std::span<const PoseCaptureTarget> targets, std::string name) {
    const auto& root = character(document, rootId);
    nameAvailable(root, name);
    if (frame < 0 || frame >= document.duration || targets.empty() || targets.size() > 2000 ||
        root.poses.size() >= 1000)
        throw std::invalid_argument("Pose capture frame, targets or count is invalid.");
    CharacterPose result;
    result.name = std::move(name);
    std::set<Id> unique;
    for (const auto& target : targets) {
        checkedPart(document, rootId, target.part);
        if (!target.channels || (target.channels & ~PoseChannels::All) ||
            !unique.insert(target.part).second)
            throw std::invalid_argument("Pose target has a duplicate or invalid channel mask.");
        const auto& layer = document.layer(target.part);
        PosePart entry;
        entry.part = target.part;
        entry.channels = target.channels;
        entry.transform = evaluateTransform(layer, frame);
        if (target.channels & PoseChannels::Drawing) {
            const auto* drawing = document.drawingAt(target.part, frame);
            if (!drawing)
                throw std::invalid_argument("Pose drawing channel needs an exposed substitution.");
            entry.drawing = drawing->id;
        }
        result.parts.push_back(entry);
    }
    result.id = document.allocateId();
    const Id id = result.id;
    document.layer(rootId).poses.push_back(std::move(result));
    return id;
}

void applyCharacterPose(Document& document, Id rootId, Id poseId, Frame frame) {
    const auto entries = pose(document, rootId, poseId).parts;
    if (frame < 0 || frame >= document.duration)
        throw std::invalid_argument("Pose frame is outside the scene.");
    std::vector<PropertyEdit> edits;
    for (const auto& entry : entries) {
        checkedPart(document, rootId, entry.part);
        if (entry.channels & PoseChannels::Drawing) {
            const auto& variants = document.layer(entry.part).variants;
            if (std::none_of(variants.begin(), variants.end(), [&](const auto& item) {
                    return item.drawing == entry.drawing;
                }))
                throw std::invalid_argument("Pose substitution is no longer registered.");
        }
        for (unsigned index = 0; index < 8; ++index)
            if (entry.channels & (1u << index))
                edits.push_back({{entry.part, static_cast<PropertyKind>(index)},
                                 channelValue(entry.transform, index)});
    }
    // Validate the entire operation before publishing any key or exposure.
    Document candidate = document;
    editProperties(candidate, edits, frame, AnimationEditMode::Animate, true);
    for (const auto& entry : entries)
        if (entry.channels & PoseChannels::Drawing)
            selectSubstitution(candidate, entry.part, frame, entry.drawing);
    candidate.validate();
    document = std::move(candidate);
}

void renameCharacterPose(Document& document, Id rootId, Id poseId, std::string name) {
    auto& root = character(document, rootId);
    (void)pose(document, rootId, poseId);
    nameAvailable(root, name, poseId);
    pose(document, rootId, poseId).name = std::move(name);
}

void removeCharacterPose(Document& document, Id rootId, Id poseId) {
    (void)pose(document, rootId, poseId);
    auto& poses = document.layer(rootId).poses;
    std::erase_if(poses, [poseId](const auto& item) { return item.id == poseId; });
}
} // namespace opentoon
