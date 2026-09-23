#include "opentoon/rigging.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <map>
#include <set>
#include <stdexcept>

namespace opentoon {
namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::invalid_argument(message);
}
struct Matrix {
    double a = 1, b = 0, c = 0, d = 1, x = 0, y = 0;
};
Matrix compose(Matrix parent, Matrix child) {
    return {parent.a * child.a + parent.b * child.c,
            parent.a * child.b + parent.b * child.d,
            parent.c * child.a + parent.d * child.c,
            parent.c * child.b + parent.d * child.d,
            parent.a * child.x + parent.b * child.y + parent.x,
            parent.c * child.x + parent.d * child.y + parent.y};
}
Matrix inverse(Matrix m) {
    const double determinant = m.a * m.d - m.b * m.c;
    require(std::isfinite(determinant) && std::abs(determinant) > 1e-10,
            "Cannot reparent through a singular transform.");
    Matrix result{m.d / determinant, -m.b / determinant, -m.c / determinant, m.a / determinant};
    result.x = -(result.a * m.x + result.b * m.y);
    result.y = -(result.c * m.x + result.d * m.y);
    return result;
}
Matrix matrix(Transform t) {
    const double radians = t.rotation * std::numbers::pi / 180.0;
    const double cosine = std::cos(radians), sine = std::sin(radians);
    Matrix result{cosine * t.scaleX, -sine * t.scaleY, sine * t.scaleX, cosine * t.scaleY};
    result.x = t.x + t.pivotX - result.a * t.pivotX - result.b * t.pivotY;
    result.y = t.y + t.pivotY - result.c * t.pivotX - result.d * t.pivotY;
    return result;
}
Transform decompose(Matrix m, Transform previous) {
    const double sx = std::hypot(m.a, m.c);
    require(std::isfinite(sx) && sx > 1e-10, "Cannot reparent a singular layer transform.");
    const double sy = (m.a * m.d - m.b * m.c) / sx;
    const double shear = m.a * m.b + m.c * m.d;
    require(std::isfinite(sy) && std::abs(sy) > 1e-10 &&
                std::abs(shear) <= 1e-8 * std::max(1.0, sx * std::abs(sy)),
            "Reparenting would introduce unsupported shear.");
    previous.rotation = std::atan2(m.c, m.a) * 180.0 / std::numbers::pi;
    previous.scaleX = sx;
    previous.scaleY = sy;
    previous.x = m.x - previous.pivotX + m.a * previous.pivotX + m.b * previous.pivotY;
    previous.y = m.y - previous.pivotY + m.c * previous.pivotX + m.d * previous.pivotY;
    return previous;
}
Matrix ancestry(const Document& document, Id parent) {
    Matrix result;
    while (parent) {
        const auto& node = document.layer(parent);
        require(node.keys.empty(), "Reparenting under animated ancestors is not supported yet.");
        result = compose(matrix(node.transform), result);
        parent = node.parent;
    }
    return result;
}
double ancestryOpacity(const Document& document, Id parent) {
    double result = 1;
    while (parent) {
        const auto& node = document.layer(parent);
        result *= node.transform.opacity;
        parent = node.parent;
    }
    return result;
}
Layer& part(Document& document, Id id) {
    auto& layer = document.layer(id);
    require(layer.kind == LayerKind::Part && !layer.locked, "Select an unlocked character part.");
    return layer;
}
Frame spanEnd(const Layer& layer, Frame frame, Frame duration) {
    for (const auto& exposure : layer.exposures)
        if (exposure.start <= frame && frame < exposure.end)
            return exposure.end;
        else if (exposure.start > frame)
            return exposure.start;
    return duration;
}
void checkVariant(const Layer& layer, Id drawing) {
    require(std::any_of(layer.variants.begin(), layer.variants.end(),
                        [drawing](const auto& item) { return item.drawing == drawing; }),
            "This drawing is not a substitution of the selected part.");
}
std::string drawingLabel(const Document& document, Id drawing) {
    const auto& name = document.drawings.at(drawing).name;
    return name.empty() ? "Drawing " + std::to_string(drawing) : name.substr(0, 128);
}
Layer& character(Document& document, Id id) {
    auto& root = document.layer(id);
    require(root.kind == LayerKind::Character && !root.locked, "Select an unlocked character root.");
    return root;
}
CharacterView& findView(Layer& root, Id id) {
    auto found = std::find_if(root.views.begin(), root.views.end(),
                              [id](const CharacterView& view) { return view.id == id; });
    require(found != root.views.end(), "Character view set does not exist.");
    return *found;
}
std::vector<ViewChoice> currentChoices(const Document& document, Id root, Frame frame) {
    require(frame >= 0 && frame < document.duration, "View-set frame is outside the scene.");
    std::vector<ViewChoice> result;
    for (const auto& layer : document.layers) {
        if (layer.kind != LayerKind::Part || characterFor(document, layer.id) != root)
            continue;
        const auto* drawing = document.drawingAt(layer.id, frame);
        if (!drawing)
            throw std::invalid_argument("View set needs a substitution for part: " + layer.role);
        result.push_back({layer.id, drawing->id});
    }
    require(!result.empty(), "Character has no parts to capture.");
    return result;
}
std::vector<ViewChoice> checkedViewChoices(Document& document, Id rootId, Id viewId) {
    const auto choices = findView(character(document, rootId), viewId).choices;
    std::map<Id, Id> selected;
    for (const auto& choice : choices)
        selected.emplace(choice.part, choice.drawing);
    std::vector<ViewChoice> targets;
    for (const auto& layer : document.layers) {
        if (layer.kind != LayerKind::Part || characterFor(document, layer.id) != rootId)
            continue;
        require(!layer.locked, "Unlock every affected part before applying a character view.");
        if (!selected.contains(layer.id))
            throw std::invalid_argument("View set is missing part: " + layer.role);
        checkVariant(layer, selected.at(layer.id));
        targets.push_back({layer.id, selected.at(layer.id)});
    }
    require(targets.size() == choices.size(), "View set contains stale or duplicate parts.");
    return targets;
}
bool inBranch(const Document& document, Id node, Id branch) {
    for (Id current = node; current; current = document.layer(current).parent)
        if (current == branch)
            return true;
    return false;
}
} // namespace
Id characterFor(const Document& document, Id layerId) {
    Id current = layerId;
    while (current) {
        const auto& layer = document.layer(current);
        if (layer.kind == LayerKind::Character)
            return current;
        current = layer.parent;
    }
    return 0;
}
Id makeCharacter(Document& document, Id drawingLayer, std::string name) {
    auto& layer = document.layer(drawingLayer);
    require(layer.kind == LayerKind::Drawing && !layer.locked && layer.parent == 0,
            "Start a character from an unlocked root drawing layer.");
    require(!name.empty() && name.size() <= 128, "Character name is invalid.");
    Layer root;
    root.id = document.allocateId();
    const Id rootId = root.id;
    root.name = std::move(name);
    root.kind = LayerKind::Character;
    document.layers.push_back(root);
    auto& selected = document.layer(drawingLayer);
    selected.kind = LayerKind::Part;
    selected.role = "Body";
    selected.parent = rootId;
    std::set<Id> known;
    for (const auto& exposure : selected.exposures)
        if (known.insert(exposure.drawing).second)
            selected.variants.push_back({exposure.drawing, drawingLabel(document, exposure.drawing)});
    return rootId;
}
void attachDrawingAsPart(Document& document, Id drawingId, Id parentId, std::string role) {
    auto& drawing = document.layer(drawingId);
    require(drawing.kind == LayerKind::Drawing && !drawing.locked && drawing.parent == 0,
            "Attach an unlocked root drawing layer.");
    require(drawing.keys.empty(), "Attach artwork before animating its layer.");
    const auto& parent = document.layer(parentId);
    require(parent.kind == LayerKind::Character || parent.kind == LayerKind::Peg,
            "Attach artwork below a character root or peg.");
    require(!role.empty() && role.size() <= 128, "Part role is invalid.");
    const Id rootId = characterFor(document, parentId);
    require(rootId != 0, "Attach parts below a character.");
    require(document.layer(rootId).views.empty() || !drawing.exposures.empty(),
            "Expose a drawing before adding this part to character views.");
    const Matrix targetSpace = inverse(ancestry(document, parentId));
    const double targetOpacity = ancestryOpacity(document, parentId);
    require(targetOpacity > 1e-10, "Cannot attach below a transparent parent.");
    auto convert = [&](Transform& pose) {
        pose = decompose(compose(targetSpace, matrix(pose)), pose);
        pose.opacity /= targetOpacity;
        require(std::isfinite(pose.opacity) && pose.opacity >= 0 && pose.opacity <= 1,
                "Attaching cannot preserve opacity under this parent.");
    };
    convert(drawing.transform);
    for (auto& key : drawing.keys)
        convert(key.value);
    drawing.kind = LayerKind::Part;
    drawing.role = std::move(role);
    drawing.parent = parentId;
    std::set<Id> known;
    for (const auto& exposure : drawing.exposures)
        if (known.insert(exposure.drawing).second)
            drawing.variants.push_back({exposure.drawing, drawingLabel(document, exposure.drawing)});
    if (!drawing.variants.empty())
        for (auto& view : document.layer(rootId).views)
            view.choices.push_back({drawingId, drawing.variants.front().drawing});
}
std::size_t attachUnparentedDrawings(Document& document, Id rootId, Frame frame) {
    (void)character(document, rootId);
    require(frame >= 0 && frame < document.duration, "Assembly frame is outside the scene.");
    std::vector<Id> candidates;
    for (const auto& layer : document.layers)
        if (layer.kind == LayerKind::Drawing && !layer.parent && !layer.locked &&
            document.drawingAt(layer.id, frame))
            candidates.push_back(layer.id);
    for (const Id id : candidates) {
        const auto role = document.layer(id).name.substr(0, 128);
        attachDrawingAsPart(document, id, rootId, role);
    }
    return candidates.size();
}
Id addPeg(Document& document, Id childId, std::string name) {
    const auto child = document.layer(childId);
    require(!child.locked && (child.kind == LayerKind::Part || child.kind == LayerKind::Peg),
            "Select an unlocked character part or peg.");
    require(!name.empty() && name.size() <= 128, "Peg name is invalid.");
    Layer peg;
    peg.id = document.allocateId();
    const Id pegId = peg.id;
    peg.name = std::move(name);
    peg.kind = LayerKind::Peg;
    peg.parent = child.parent;
    document.layers.push_back(peg);
    document.layer(childId).parent = pegId;
    return pegId;
}
void setPartRole(Document& document, Id partId, std::string role) {
    auto& layer = part(document, partId);
    require(!role.empty() && role.size() <= 128, "Part role is invalid.");
    layer.role = std::move(role);
}
void reparentPreservingWorld(Document& document, Id childId, Id newParent) {
    auto& child = document.layer(childId);
    require(!child.locked && (child.kind == LayerKind::Part || child.kind == LayerKind::Peg),
            "Select an unlocked character part or peg.");
    require(child.keys.empty(), "Reparent before animating this part or peg.");
    const auto& parent = document.layer(newParent);
    require(parent.kind == LayerKind::Character || parent.kind == LayerKind::Peg,
            "Parts and pegs require a character root or peg parent.");
    require(childId != newParent, "A layer cannot parent itself.");
    Id oldRoot = childId;
    while (document.layer(oldRoot).parent)
        oldRoot = document.layer(oldRoot).parent;
    Id newRoot = newParent;
    while (document.layer(newRoot).parent) {
        require(newRoot != childId, "Layer hierarchy contains a cycle.");
        newRoot = document.layer(newRoot).parent;
    }
    require(oldRoot == newRoot, "Move parts only within their character.");
    if (child.parent == newParent)
        return;
    const Matrix oldSpace = ancestry(document, child.parent);
    const Matrix newSpace = inverse(ancestry(document, newParent));
    const double oldOpacity = ancestryOpacity(document, child.parent);
    const double newOpacity = ancestryOpacity(document, newParent);
    require(newOpacity > 1e-10, "Cannot preserve opacity below a transparent parent.");
    auto convert = [&](Transform& pose) {
        const auto desired = compose(newSpace, compose(oldSpace, matrix(pose)));
        pose = decompose(desired, pose);
        pose.opacity *= oldOpacity / newOpacity;
        require(std::isfinite(pose.opacity) && pose.opacity >= 0 && pose.opacity <= 1,
                "Reparenting cannot preserve opacity under this parent.");
    };
    convert(child.transform);
    child.parent = newParent;
}
void setPivotPreservingArtwork(Document& document, Id layerId, double x, double y) {
    auto& layer = document.layer(layerId);
    require(!layer.locked && std::isfinite(x) && std::isfinite(y) && std::abs(x) <= 1e7 &&
                std::abs(y) <= 1e7, "Invalid pivot placement.");
    require(layer.keys.empty(), "Place the rest pivot before animating this layer.");
    auto apply = [=](Transform& pose) {
        const auto before = matrix(pose);
        pose.pivotX = x;
        pose.pivotY = y;
        pose.x = before.x - x + before.a * x + before.b * y;
        pose.y = before.y - y + before.c * x + before.d * y;
    };
    apply(layer.transform);
}
Id createSubstitution(Document& document, Id partId, Frame frame, bool duplicateCurrent, std::string name) {
    auto& layer = part(document, partId);
    require(frame >= 0 && frame < document.duration && !name.empty() && name.size() <= 128,
            "Invalid substitution name or frame.");
    Drawing drawing;
    if (duplicateCurrent)
        if (const auto* current = document.drawingAt(partId, frame))
            drawing = *current;
    drawing.id = document.allocateId();
    drawing.name = name;
    for (auto& stroke : drawing.strokes)
        stroke.id = document.allocateId();
    const Id id = drawing.id;
    document.drawings.emplace(id, std::move(drawing));
    const Frame end = spanEnd(layer, frame, document.duration);
    layer.variants.push_back({id, std::move(name)});
    expose(layer, frame, end, id);
    return id;
}
void renameSubstitution(Document& document, Id partId, Id drawing, std::string name) {
    auto& layer = part(document, partId);
    require(!name.empty() && name.size() <= 128, "Substitution name is invalid.");
    checkVariant(layer, drawing);
    auto it = std::find_if(layer.variants.begin(), layer.variants.end(),
                           [drawing](const auto& item) { return item.drawing == drawing; });
    it->name = std::move(name);
}
void selectSubstitution(Document& document, Id partId, Frame frame, Id drawing) {
    auto& layer = part(document, partId);
    require(frame >= 0 && frame < document.duration, "Substitution frame is outside the scene.");
    checkVariant(layer, drawing);
    expose(layer, frame, spanEnd(layer, frame, document.duration), drawing);
}
void removeSubstitution(Document& document, Id partId, Id drawing) {
    auto& layer = part(document, partId);
    checkVariant(layer, drawing);
    const Id root = characterFor(document, partId);
    for (const auto& view : document.layer(root).views)
        for (const auto& choice : view.choices)
            require(choice.part != partId || choice.drawing != drawing,
                    "Remove this substitution from character view sets first.");
    std::erase_if(layer.variants, [drawing](const auto& item) { return item.drawing == drawing; });
    const Id replacement = layer.variants.empty() ? 0 : layer.variants.front().drawing;
    auto exposures = layer.exposures;
    for (const auto& exposure : exposures)
        if (exposure.drawing == drawing)
            expose(layer, exposure.start, exposure.end, replacement);
}
void reorderSubstitution(Document& document, Id partId, Id drawing, int direction) {
    auto& layer = part(document, partId);
    require(direction == -1 || direction == 1, "Substitution order must move one place.");
    checkVariant(layer, drawing);
    auto it = std::find_if(layer.variants.begin(), layer.variants.end(),
                           [drawing](const auto& value) { return value.drawing == drawing; });
    const auto index = std::distance(layer.variants.begin(), it);
    const auto target = index + direction;
    if (target >= 0 && target < std::ptrdiff_t(layer.variants.size()))
        std::iter_swap(it, layer.variants.begin() + target);
}
Id stepSubstitution(Document& document, Id partId, Frame frame, int direction) {
    auto& layer = part(document, partId);
    require((direction == -1 || direction == 1) && frame >= 0 && frame < document.duration,
            "Invalid substitution step.");
    require(!layer.variants.empty(), "This part has no substitutions.");
    const auto* drawing = document.drawingAt(partId, frame);
    auto it = std::find_if(layer.variants.begin(), layer.variants.end(),
                           [drawing](const auto& item) { return drawing && item.drawing == drawing->id; });
    const auto size = std::ptrdiff_t(layer.variants.size());
    const auto index = it == layer.variants.end() ? (direction > 0 ? -1 : 0)
                                                  : std::distance(layer.variants.begin(), it);
    const Id next = layer.variants[std::size_t((index + direction + size) % size)].drawing;
    selectSubstitution(document, partId, frame, next);
    return next;
}
Id captureCharacterView(Document& document, Id rootId, Frame frame, std::string name) {
    auto choices = currentChoices(document, rootId, frame);
    auto& root = character(document, rootId);
    require(!name.empty() && name.size() <= 128, "Character view name is invalid.");
    require(std::none_of(root.views.begin(), root.views.end(),
                         [&](const CharacterView& view) { return view.name == name; }),
            "Character view name already exists.");
    const Id id = document.allocateId();
    root.views.push_back({id, std::move(name), std::move(choices)});
    return id;
}
void applyCharacterView(Document& document, Id rootId, Id viewId, Frame frame) {
    require(frame >= 0 && frame < document.duration, "View-set frame is outside the scene.");
    const auto targets = checkedViewChoices(document, rootId, viewId);
    for (const auto& choice : targets)
        expose(document.layer(choice.part), frame,
               spanEnd(document.layer(choice.part), frame, document.duration), choice.drawing);
}
void applyCharacterViewRange(Document& document, Id rootId, Id viewId, Frame start, Frame end) {
    require(start >= 0 && end > start && end <= document.duration,
            "Character view range is outside the scene.");
    const auto targets = checkedViewChoices(document, rootId, viewId);
    for (const auto& choice : targets)
        expose(document.layer(choice.part), start, end, choice.drawing);
}
void updateCharacterView(Document& document, Id rootId, Id viewId, Frame frame) {
    auto choices = currentChoices(document, rootId, frame);
    auto& root = character(document, rootId);
    findView(root, viewId).choices = std::move(choices);
}
void updateCharacterViewPart(Document& document, Id rootId, Id viewId, Id partId, Frame frame) {
    auto& root = character(document, rootId);
    auto& target = part(document, partId);
    require(characterFor(document, partId) == rootId && frame >= 0 && frame < document.duration,
            "Selected part does not belong to this character or frame.");
    const auto* drawing = document.drawingAt(partId, frame);
    require(drawing != nullptr, "Expose a substitution before updating this view.");
    checkVariant(target, drawing->id);
    auto& choices = findView(root, viewId).choices;
    auto found = std::find_if(choices.begin(), choices.end(), [partId](const ViewChoice& choice) {
        return choice.part == partId;
    });
    if (found == choices.end())
        choices.push_back({partId, drawing->id});
    else
        found->drawing = drawing->id;
}
void renameCharacterView(Document& document, Id rootId, Id viewId, std::string name) {
    auto& root = character(document, rootId);
    require(!name.empty() && name.size() <= 128, "Character view name is invalid.");
    require(std::none_of(root.views.begin(), root.views.end(),
                         [&](const CharacterView& view) { return view.id != viewId && view.name == name; }),
            "Character view name already exists.");
    findView(root, viewId).name = std::move(name);
}
Id duplicateCharacterView(Document& document, Id rootId, Id viewId) {
    auto& root = character(document, rootId);
    const auto source = findView(root, viewId);
    auto name = source.name + " copy";
    for (int index = 2; std::any_of(root.views.begin(), root.views.end(),
                                   [&](const CharacterView& view) { return view.name == name; }); ++index)
        name = source.name + " copy " + std::to_string(index);
    require(name.size() <= 128, "Character view name is too long to duplicate.");
    CharacterView copy = source;
    copy.id = document.allocateId();
    copy.name = std::move(name);
    const Id id = copy.id;
    root.views.push_back(std::move(copy));
    return id;
}
void removeCharacterView(Document& document, Id rootId, Id viewId) {
    auto& root = character(document, rootId);
    (void)findView(root, viewId);
    std::erase_if(root.views, [viewId](const CharacterView& view) { return view.id == viewId; });
}
void reorderCharacterView(Document& document, Id rootId, Id viewId, int direction) {
    auto& root = character(document, rootId);
    require(direction == -1 || direction == 1, "Character view order must move one place.");
    auto& current = findView(root, viewId);
    const auto position = &current - root.views.data();
    const auto target = position + direction;
    if (target >= 0 && target < std::ptrdiff_t(root.views.size()))
        std::iter_swap(root.views.begin() + position, root.views.begin() + target);
}
Id duplicateCharacter(Document& document, Id rootId, double offsetX, double offsetY) {
    const auto& root = document.layer(rootId);
    require(root.kind == LayerKind::Character && std::isfinite(offsetX) && std::isfinite(offsetY),
            "Select a character to duplicate with a finite offset.");
    std::vector<Layer> original;
    for (const auto& layer : document.layers)
        if (characterFor(document, layer.id) == rootId)
            original.push_back(layer);
    std::map<Id, Id> layers, drawings;
    for (const auto& layer : original)
        layers[layer.id] = document.allocateId();
    auto copyDrawing = [&](Id old) {
        if (drawings.contains(old))
            return drawings.at(old);
        Drawing copy = document.drawings.at(old);
        copy.id = document.allocateId();
        for (auto& stroke : copy.strokes)
            stroke.id = document.allocateId();
        const Id id = copy.id;
        document.drawings.emplace(id, std::move(copy));
        drawings[old] = id;
        return id;
    };
    for (auto source : original) {
        const Id old = source.id;
        source.id = layers.at(old);
        source.parent = source.parent ? layers.at(source.parent) : 0;
        if (old == rootId) {
            source.name = source.name.substr(0, 4091) + " copy";
            source.transform.x += offsetX;
            source.transform.y += offsetY;
            for (auto& key : source.keys) {
                key.value.x += offsetX;
                key.value.y += offsetY;
            }
        }
        for (auto& exposure : source.exposures)
            exposure.drawing = copyDrawing(exposure.drawing);
        for (auto& variant : source.variants)
            variant.drawing = copyDrawing(variant.drawing);
        for (auto& view : source.views) {
            view.id = document.allocateId();
            for (auto& choice : view.choices) {
                choice.part = layers.at(choice.part);
                choice.drawing = copyDrawing(choice.drawing);
            }
        }
        document.layers.push_back(std::move(source));
    }
    return layers.at(rootId);
}
Id duplicateRigBranch(Document& document, Id branchId, bool linkedArtwork) {
    const auto sourceRoot = document.layer(branchId);
    require((sourceRoot.kind == LayerKind::Part || sourceRoot.kind == LayerKind::Peg) &&
                !sourceRoot.locked, "Select an unlocked part or peg branch to duplicate.");
    const Id rootId = characterFor(document, branchId);
    std::vector<Layer> originals;
    for (const auto& layer : document.layers)
        if (inBranch(document, layer.id, branchId)) {
            require(!layer.locked, "Unlock every layer in the branch before duplicating.");
            originals.push_back(layer);
        }
    std::map<Id, Id> layers, drawings;
    for (const auto& source : originals)
        layers[source.id] = document.allocateId();
    auto mapDrawing = [&](Id old) {
        if (linkedArtwork)
            return old;
        if (drawings.contains(old))
            return drawings.at(old);
        Drawing copy = document.drawings.at(old);
        copy.id = document.allocateId();
        for (auto& stroke : copy.strokes)
            stroke.id = document.allocateId();
        const Id id = copy.id;
        document.drawings.emplace(id, std::move(copy));
        drawings[old] = id;
        return id;
    };
    for (auto source : originals) {
        const Id old = source.id;
        source.id = layers.at(old);
        source.parent = layers.contains(source.parent) ? layers.at(source.parent) : source.parent;
        if (old == branchId) {
            source.name = source.name.substr(0, 4091) + (linkedArtwork ? " clone" : " copy");
            source.transform.x += 32;
            source.transform.y += 32;
            for (auto& key : source.keys) {
                key.value.x += 32;
                key.value.y += 32;
            }
        }
        for (auto& exposure : source.exposures)
            exposure.drawing = mapDrawing(exposure.drawing);
        for (auto& variant : source.variants)
            variant.drawing = mapDrawing(variant.drawing);
        document.layers.push_back(std::move(source));
    }
    for (auto& view : document.layer(rootId).views) {
        std::vector<ViewChoice> additions;
        for (const auto& choice : view.choices)
            if (layers.contains(choice.part))
                additions.push_back({layers.at(choice.part), mapDrawing(choice.drawing)});
        view.choices.insert(view.choices.end(), additions.begin(), additions.end());
    }
    return layers.at(branchId);
}
void removeRigBranch(Document& document, Id branchId) {
    const auto& branch = document.layer(branchId);
    require(branch.kind == LayerKind::Part || branch.kind == LayerKind::Peg,
            "Select a character part or peg branch to remove.");
    const Id rootId = characterFor(document, branchId);
    std::set<Id> removed;
    for (const auto& layer : document.layers)
        if (inBranch(document, layer.id, branchId)) {
            require(!layer.locked, "Unlock every layer in the branch before removing.");
            removed.insert(layer.id);
        }
    for (auto& view : document.layer(rootId).views)
        std::erase_if(view.choices, [&](const ViewChoice& choice) {
            return removed.contains(choice.part);
        });
    std::erase_if(document.layers, [&](const Layer& layer) { return removed.contains(layer.id); });
}
void detachPart(Document& document, Id partId) {
    auto& layer = part(document, partId);
    require(layer.keys.empty(), "Detach a part before animating it.");
    require(std::none_of(document.layers.begin(), document.layers.end(),
                         [partId](const Layer& child) { return child.parent == partId; }),
            "Detach child layers first.");
    const Id rootId = characterFor(document, partId);
    const Matrix world = compose(ancestry(document, layer.parent), matrix(layer.transform));
    Transform pose = decompose(world, layer.transform);
    pose.opacity *= ancestryOpacity(document, layer.parent);
    require(std::isfinite(pose.opacity) && pose.opacity >= 0 && pose.opacity <= 1,
            "Detaching cannot preserve opacity.");
    for (auto& view : document.layer(rootId).views)
        std::erase_if(view.choices, [partId](const ViewChoice& choice) { return choice.part == partId; });
    layer.transform = pose;
    layer.parent = 0;
    layer.kind = LayerKind::Drawing;
    layer.role.clear();
    layer.variants.clear();
}
void dissolvePeg(Document& document, Id pegId) {
    const auto peg = document.layer(pegId);
    require(peg.kind == LayerKind::Peg && !peg.locked && peg.keys.empty(),
            "Select an unlocked, unanimated peg to dissolve.");
    const Matrix oldSpace = ancestry(document, pegId);
    const Matrix newSpace = inverse(ancestry(document, peg.parent));
    const double opacityFactor = peg.transform.opacity;
    std::vector<std::pair<Id, Transform>> changes;
    for (const auto& child : document.layers)
        if (child.parent == pegId) {
            require(!child.locked && child.keys.empty(),
                    "Dissolve a peg before its children are animated.");
            Transform pose = decompose(compose(newSpace, compose(oldSpace, matrix(child.transform))),
                                       child.transform);
            pose.opacity *= opacityFactor;
            require(std::isfinite(pose.opacity) && pose.opacity >= 0 && pose.opacity <= 1,
                    "Dissolving cannot preserve child opacity.");
            changes.emplace_back(child.id, pose);
        }
    for (const auto& [id, pose] : changes) {
        auto& child = document.layer(id);
        child.parent = peg.parent;
        child.transform = pose;
    }
    std::erase_if(document.layers, [pegId](const Layer& layer) { return layer.id == pegId; });
}
} // namespace opentoon
