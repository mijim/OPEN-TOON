#include "opentoon/character_pose.h"
#include "opentoon/animation.h"
#include "opentoon/property_address.h"
#include "opentoon/rigging.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>

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
std::string mirroredRole(const std::string& role) {
    constexpr std::string_view left = "_left", right = "_right";
    if (role.ends_with(left))
        return role.substr(0, role.size() - left.size()) + std::string(right);
    if (role.ends_with(right))
        return role.substr(0, role.size() - right.size()) + std::string(left);
    return role;
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
PosePart capturePart(const Document& document, Id root, Id partId, Frame frame,
                     std::uint16_t channels) {
    checkedPart(document, root, partId);
    if (!channels || (channels & ~PoseChannels::All))
        throw std::invalid_argument("Pose target has an invalid channel mask.");
    const auto& layer = document.layer(partId);
    PosePart entry;
    entry.part = partId;
    entry.channels = channels;
    entry.transform = evaluateTransform(layer, frame);
    if (channels & PoseChannels::Drawing) {
        const auto* drawing = document.drawingAt(partId, frame);
        if (!drawing)
            throw std::invalid_argument("Pose drawing channel needs an exposed substitution.");
        entry.drawing = drawing->id;
    }
    return entry;
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
        if (!unique.insert(target.part).second)
            throw std::invalid_argument("Pose target is duplicated.");
        result.parts.push_back(capturePart(document, rootId, target.part, frame, target.channels));
    }
    result.id = document.allocateId();
    const Id id = result.id;
    document.layer(rootId).poses.push_back(std::move(result));
    return id;
}

void applyCharacterPose(Document& document, Id rootId, Id poseId, Frame frame) {
    blendCharacterPose(document, rootId, poseId, frame, 1.0);
}

void blendCharacterPose(Document& document, Id rootId, Id poseId, Frame frame, double amount) {
    if (!std::isfinite(amount) || amount < 0 || amount > 1)
        throw std::invalid_argument("Pose blend amount must be between zero and one.");
    const auto entries = pose(document, rootId, poseId).parts;
    if (frame < 0 || frame >= document.duration)
        throw std::invalid_argument("Pose frame is outside the scene.");
    if (amount == 0)
        return;
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
        const auto baseline = evaluateTransform(document.layer(entry.part), frame);
        for (unsigned index = 0; index < 8; ++index)
            if (entry.channels & (1u << index))
                edits.push_back({{entry.part, static_cast<PropertyKind>(index)},
                                 amount == 1 ? channelValue(entry.transform, index)
                                             : channelValue(baseline, index) + amount *
                                                   (channelValue(entry.transform, index) -
                                                    channelValue(baseline, index))});
    }
    // Validate the entire operation before publishing any key or exposure.
    Document candidate = document;
    editProperties(candidate, edits, frame, AnimationEditMode::Animate, true);
    for (const auto& entry : entries)
        if ((entry.channels & PoseChannels::Drawing) && amount >= .5)
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
void publishCharacterPose(Document& document, Id rootId, Id poseId, bool published) {
    pose(document, rootId, poseId).published = published;
}
void setCharacterPoseControlGroup(Document& document, Id rootId, Id poseId, std::string group) {
    if (group.empty() || group.size() > 64)
        throw std::invalid_argument("Control group name must be 1–64 bytes.");
    pose(document, rootId, poseId).controlGroup = std::move(group);
}
void setCharacterPosePart(Document& document, Id rootId, Id poseId, Id partId,
                          Frame frame, std::uint16_t channels) {
    auto& saved = pose(document, rootId, poseId);
    if (frame < 0 || frame >= document.duration)
        throw std::invalid_argument("Pose frame is outside the scene.");
    const auto entry = capturePart(document, rootId, partId, frame, channels);
    auto found = std::find_if(saved.parts.begin(), saved.parts.end(),
                              [partId](const auto& item) { return item.part == partId; });
    if (found != saved.parts.end())
        *found = entry;
    else {
        if (saved.parts.size() >= 2000)
            throw std::invalid_argument("Pose has too many Part entries.");
        saved.parts.push_back(entry);
    }
}
void removeCharacterPosePart(Document& document, Id rootId, Id poseId, Id partId) {
    auto& saved = pose(document, rootId, poseId);
    const auto found = std::find_if(saved.parts.begin(), saved.parts.end(),
                                    [partId](const auto& item) { return item.part == partId; });
    if (found == saved.parts.end() || saved.parts.size() == 1)
        throw std::invalid_argument("Pose must retain at least one mapped Part.");
    saved.parts.erase(found);
}
Id transferCharacterPose(Document& document, Id sourceRoot, Id poseId, Id targetRoot) {
    if (sourceRoot == targetRoot)
        throw std::invalid_argument("Choose another character for pose transfer.");
    const auto& source = character(document, sourceRoot);
    const auto& target = character(document, targetRoot);
    const auto found = std::find_if(source.poses.begin(), source.poses.end(),
                                    [poseId](const auto& item) { return item.id == poseId; });
    if (found == source.poses.end() || target.poses.size() >= 1000)
        throw std::invalid_argument("Pose source or destination is unavailable.");

    std::map<std::string, Id> sourceRoles, targetRoles;
    for (const auto& layer : document.layers) {
        if (layer.kind != LayerKind::Part)
            continue;
        const Id owner = characterFor(document, layer.id);
        auto* roles = owner == sourceRoot ? &sourceRoles : owner == targetRoot ? &targetRoles : nullptr;
        if (roles && !roles->emplace(layer.role, layer.id).second)
            throw std::invalid_argument("Pose transfer needs unique Part roles in both characters.");
    }

    CharacterPose copy = *found;
    copy.published = false;
    for (auto& entry : copy.parts) {
        const auto& from = document.layer(entry.part);
        checkedPart(document, sourceRoot, from.id);
        const auto mapped = targetRoles.find(from.role);
        if (mapped == targetRoles.end() || sourceRoles.at(from.role) != from.id)
            throw std::invalid_argument("Pose transfer has no unique matching Part role.");
        const auto& to = document.layer(mapped->second);
        checkedPart(document, targetRoot, to.id);
        if (to.transform != from.transform)
            throw std::invalid_argument("Pose transfer needs matching Part rest transforms.");
        if (entry.channels & PoseChannels::Drawing) {
            const auto original = std::find_if(from.variants.begin(), from.variants.end(),
                                               [&](const auto& item) { return item.drawing == entry.drawing; });
            if (original == from.variants.end())
                throw std::invalid_argument("Pose source drawing is unavailable.");
            const auto matches = std::count_if(to.variants.begin(), to.variants.end(),
                                               [&](const auto& item) { return item.name == original->name; });
            if (matches != 1 || std::count_if(from.variants.begin(), from.variants.end(),
                                              [&](const auto& item) { return item.name == original->name; }) != 1)
                throw std::invalid_argument("Pose transfer needs one matching drawing name per Part.");
            entry.drawing = std::find_if(to.variants.begin(), to.variants.end(),
                                         [&](const auto& item) { return item.name == original->name; })->drawing;
        }
        entry.part = to.id;
    }
    std::string name = copy.name;
    for (int suffix = 2; std::any_of(target.poses.begin(), target.poses.end(),
                                    [&](const auto& item) { return item.name == name; }); ++suffix)
        name = copy.name + " " + std::to_string(suffix);
    nameAvailable(target, name);
    copy.name = std::move(name);
    copy.id = document.allocateId();
    const Id id = copy.id;
    document.layer(targetRoot).poses.push_back(std::move(copy));
    return id;
}
Id mirrorCharacterPose(Document& document, Id rootId, Id poseId) {
    const auto& root = character(document, rootId);
    const auto found = std::find_if(root.poses.begin(), root.poses.end(),
                                    [poseId](const auto& item) { return item.id == poseId; });
    if (found == root.poses.end() || root.poses.size() >= 1000)
        throw std::invalid_argument("Pose is unavailable for mirroring.");
    std::map<std::string, Id> roles;
    for (const auto& layer : document.layers)
        if (layer.kind == LayerKind::Part && characterFor(document, layer.id) == rootId &&
            !roles.emplace(layer.role, layer.id).second)
            throw std::invalid_argument("Pose mirroring needs unique Part roles.");
    CharacterPose reflected = *found;
    reflected.published = false;
    for (auto& entry : reflected.parts) {
        checkedPart(document, rootId, entry.part);
        const auto& from = document.layer(entry.part);
        const auto targetRole = mirroredRole(from.role);
        const auto mapped = roles.find(targetRole);
        if (mapped == roles.end())
            throw std::invalid_argument("Pose mirror Part has no opposite role.");
        const auto& to = document.layer(mapped->second);
        checkedPart(document, rootId, to.id);
        Transform value = to.transform;
        if (entry.channels & PoseChannels::PositionX)
            value.x -= entry.transform.x - from.transform.x;
        if (entry.channels & PoseChannels::PositionY)
            value.y += entry.transform.y - from.transform.y;
        if (entry.channels & PoseChannels::Rotation)
            value.rotation -= entry.transform.rotation - from.transform.rotation;
        if (entry.channels & PoseChannels::ScaleX)
            value.scaleX += entry.transform.scaleX - from.transform.scaleX;
        if (entry.channels & PoseChannels::ScaleY)
            value.scaleY += entry.transform.scaleY - from.transform.scaleY;
        if (entry.channels & PoseChannels::Opacity)
            value.opacity += entry.transform.opacity - from.transform.opacity;
        if (entry.channels & PoseChannels::PivotX)
            value.pivotX -= entry.transform.pivotX - from.transform.pivotX;
        if (entry.channels & PoseChannels::PivotY)
            value.pivotY += entry.transform.pivotY - from.transform.pivotY;
        if (entry.channels & PoseChannels::Drawing) {
            const auto original = std::find_if(from.variants.begin(), from.variants.end(),
                                               [&](const auto& item) { return item.drawing == entry.drawing; });
            if (original == from.variants.end() ||
                std::count_if(from.variants.begin(), from.variants.end(),
                              [&](const auto& item) { return item.name == original->name; }) != 1)
                throw std::invalid_argument("Pose mirror drawing name is ambiguous.");
            const auto matches = std::count_if(to.variants.begin(), to.variants.end(),
                                               [&](const auto& item) { return item.name == original->name; });
            if (matches != 1)
                throw std::invalid_argument("Pose mirror needs one matching drawing name.");
            entry.drawing = std::find_if(to.variants.begin(), to.variants.end(),
                                         [&](const auto& item) { return item.name == original->name; })->drawing;
        }
        entry.part = to.id;
        entry.transform = value;
    }
    std::string name = found->name + " mirrored";
    for (int suffix = 2; std::any_of(root.poses.begin(), root.poses.end(),
                                    [&](const auto& item) { return item.name == name; }); ++suffix)
        name = found->name + " mirrored " + std::to_string(suffix);
    nameAvailable(root, name);
    Document candidate = document;
    reflected.name = std::move(name);
    reflected.id = candidate.allocateId();
    const Id id = reflected.id;
    candidate.layer(rootId).poses.push_back(std::move(reflected));
    candidate.validate();
    document = std::move(candidate);
    return id;
}
} // namespace opentoon
