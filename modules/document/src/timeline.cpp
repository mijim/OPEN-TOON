#include "opentoon/timeline.h"
#include "opentoon/deformer.h"
#include "opentoon/deformation.h"
#include <set>
#include <stdexcept>
namespace opentoon {
namespace {
void range(const Document& d, const std::vector<Id>& layers, Frame start, Frame end, bool editing) {
    if (start < 0 || end <= start || end > d.duration || layers.empty())
        throw std::invalid_argument("Select a valid frame range and at least one layer.");
    std::set<Id> distinct;
    for (auto id : layers) {
        const auto& layer = d.layer(id);
        if (!distinct.insert(id).second)
            throw std::invalid_argument("A range cannot contain the same layer twice.");
        if (editing && layer.locked)
            throw std::invalid_argument("Unlock every selected layer before editing the range.");
    }
}
Frame scaled(Frame value, Frame oldLength, Frame newLength) {
    return static_cast<Frame>((std::int64_t(value) * newLength + oldLength / 2) / oldLength);
}
} // namespace
ExposureClipboard copyRange(const Document& d, const std::vector<Id>& layers, Frame start, Frame end) {
    range(d, layers, start, end, false);
    ExposureClipboard result;
    result.duration = end - start;
    result.palette = d.palette;
    for (auto id : layers) {
        const auto& layer = d.layer(id);
        ClipboardTrack track;
        track.sourceLayer = id;
        for (auto exposure : layer.exposures) {
            if (exposure.end <= start || exposure.start >= end)
                continue;
            result.drawings.emplace(exposure.drawing, d.drawings.at(exposure.drawing));
            if (meshBindingFor(layer, exposure.drawing) &&
                std::find(track.boundDrawings.begin(), track.boundDrawings.end(),
                          exposure.drawing) == track.boundDrawings.end())
                track.boundDrawings.push_back(exposure.drawing);
            exposure.start = std::max(exposure.start, start) - start;
            exposure.end = std::min(exposure.end, end) - start;
            track.exposures.push_back(exposure);
        }
        for (auto key : layer.keys)
            if (key.frame >= start && key.frame < end) {
                key.frame -= start;
                track.keys.push_back(key);
            }
        for (const auto& binding : layer.bindings) {
            DeformerClipboardKeys copied;
            copied.drawing = binding.drawing;
            if (binding.bone) {
                copied.boneBasis = BoneChain{binding.bone->restJoints,
                                             binding.bone->elbowTransition,
                                             binding.bone->distalWeights, {}};
                for (auto key : binding.bone->keys)
                    if (key.frame >= start && key.frame < end) {
                        key.frame -= start;
                        copied.bone.push_back(key);
                    }
            }
            if (binding.curve) {
                copied.curveBasis = CurveDeformer{binding.curve->restControls,
                                                   binding.curve->coordinates, {}};
                for (auto key : binding.curve->keys)
                    if (key.frame >= start && key.frame < end) {
                        key.frame -= start;
                        copied.curve.push_back(key);
                    }
            }
            if (!copied.bone.empty() || !copied.curve.empty())
                track.deformerKeys.push_back(std::move(copied));
        }
        result.tracks.push_back(std::move(track));
    }
    return result;
}
void pasteRange(Document& d, const std::vector<Id>& layers, Frame at, const ExposureClipboard& clip,
                PasteContent content, bool insert, bool allowLinkedDrawings) {
    if (clip.duration < 1 || clip.duration > 1000000 || clip.tracks.size() != layers.size() || at < 0 ||
        at > d.duration || std::int64_t(at) + clip.duration > 1000000)
        throw std::invalid_argument("Clipboard range does not fit the selected layers or scene limits.");
    range(d, layers, std::min(at, d.duration - 1), std::min(at, d.duration - 1) + 1, true);
    const bool keys = content == PasteContent::Keys || content == PasteContent::All;
    const bool exposures = content != PasteContent::Keys;
    if (exposures)
        for (std::size_t index = 0; index < layers.size(); ++index) {
            const auto& track = clip.tracks[index];
            if (track.boundDrawings.empty())
                continue;
            const auto& layer = d.layer(layers[index]);
            if (content == PasteContent::IndependentDrawings || !allowLinkedDrawings ||
                layers[index] != track.sourceLayer ||
                std::any_of(track.boundDrawings.begin(), track.boundDrawings.end(),
                            [&](Id drawing) {
                                return !d.drawings.contains(drawing) ||
                                       !meshBindingFor(layer, drawing);
                            }))
                throw std::invalid_argument(
                    "Copy a bound Part through its rig; this range cannot clone mesh bindings.");
        }
    if (keys)
        for (std::size_t index = 0; index < layers.size(); ++index) {
            const auto& track = clip.tracks[index];
            if (track.deformerKeys.empty())
                continue;
            if (!allowLinkedDrawings || layers[index] != track.sourceLayer)
                throw std::invalid_argument(
                    "Paste deformer keys into their original Part; cross-scene transfer needs a rig copy.");
            const auto& layer = d.layer(layers[index]);
            for (const auto& copied : track.deformerKeys) {
                const auto* binding = meshBindingFor(layer, copied.drawing);
                if (!binding || (!copied.bone.empty() && !binding->bone) ||
                    (!copied.curve.empty() && !binding->curve) ||
                    (copied.boneBasis &&
                     (binding->bone->restJoints != copied.boneBasis->restJoints ||
                      binding->bone->elbowTransition != copied.boneBasis->elbowTransition ||
                      binding->bone->distalWeights != copied.boneBasis->distalWeights)) ||
                    (copied.curveBasis &&
                     (binding->curve->restControls != copied.curveBasis->restControls ||
                      binding->curve->coordinates != copied.curveBasis->coordinates)))
                    throw std::invalid_argument("Destination Part no longer has the copied deformer.");
            }
        }
    if (insert)
        insertFrames(d, at, clip.duration);
    d.duration = std::max(d.duration, at + clip.duration);
    std::map<Id, Id> drawings, colors;
    auto drawingId = [&](Id source) {
        if (auto found = drawings.find(source); found != drawings.end())
            return found->second;
        if (content != PasteContent::IndependentDrawings && allowLinkedDrawings &&
            d.drawings.contains(source))
            return drawings[source] = source;
        auto copy = clip.drawings.at(source);
        copy.id = d.allocateId();
        copy.name += " copy";
        for (auto& stroke : copy.strokes) {
            stroke.id = d.allocateId();
            auto original = std::find_if(clip.palette.begin(), clip.palette.end(),
                                         [&](const auto& swatch) { return swatch.id == stroke.swatch; });
            if (original == clip.palette.end())
                throw std::invalid_argument("Clipboard references a missing palette color.");
            if (!colors.contains(stroke.swatch)) {
                auto same = std::find_if(d.palette.begin(), d.palette.end(),
                                         [&](const auto& swatch) { return swatch.color == original->color; });
                if (same != d.palette.end())
                    colors[stroke.swatch] = same->id;
                else {
                    auto color = *original;
                    color.id = d.allocateId();
                    colors[stroke.swatch] = color.id;
                    d.palette.push_back(color);
                }
            }
            stroke.swatch = colors.at(stroke.swatch);
        }
        auto id = copy.id;
        d.drawings.emplace(id, std::move(copy));
        return drawings[source] = id;
    };
    for (std::size_t i = 0; i < layers.size(); ++i) {
        auto& layer = d.layer(layers[i]);
        if (exposures) {
            expose(layer, at, at + clip.duration, 0);
            for (auto e : clip.tracks[i].exposures) {
                const Id target = drawingId(e.drawing);
                if (layer.kind == LayerKind::Part &&
                    std::none_of(layer.variants.begin(), layer.variants.end(),
                                 [target](const auto& variant) { return variant.drawing == target; }))
                    layer.variants.push_back({target, d.drawings.at(target).name.empty()
                                                          ? "Drawing " + std::to_string(target)
                                                          : d.drawings.at(target).name.substr(0, 128)});
                expose(layer, at + e.start, at + e.end, target);
            }
        }
        if (keys) {
            std::erase_if(layer.keys,
                          [&](auto key) { return key.frame >= at && key.frame < at + clip.duration; });
            for (auto key : clip.tracks[i].keys) {
                key.frame += at;
                layer.keys.push_back(key);
            }
            std::sort(layer.keys.begin(), layer.keys.end(), [](auto a, auto b) { return a.frame < b.frame; });
            for (auto& binding : layer.bindings) {
                if (binding.bone)
                    std::erase_if(binding.bone->keys, [&](const auto& key) {
                        return key.frame >= at && key.frame < at + clip.duration;
                    });
                if (binding.curve)
                    std::erase_if(binding.curve->keys, [&](const auto& key) {
                        return key.frame >= at && key.frame < at + clip.duration;
                    });
            }
            for (const auto& copied : clip.tracks[i].deformerKeys) {
                auto destination = std::find_if(layer.bindings.begin(), layer.bindings.end(),
                                                [&](const auto& binding) {
                                                    return binding.drawing == copied.drawing;
                                                });
                for (auto key : copied.bone) {
                    key.frame += at;
                    destination->bone->keys.push_back(key);
                }
                for (auto key : copied.curve) {
                    key.frame += at;
                    destination->curve->keys.push_back(key);
                }
                if (destination->bone)
                    std::sort(destination->bone->keys.begin(), destination->bone->keys.end(),
                              [](const auto& a, const auto& b) { return a.frame < b.frame; });
                if (destination->curve)
                    std::sort(destination->curve->keys.begin(), destination->curve->keys.end(),
                              [](const auto& a, const auto& b) { return a.frame < b.frame; });
            }
        }
    }
}
void clearRange(Document& d, const std::vector<Id>& layers, Frame start, Frame end, bool keys) {
    range(d, layers, start, end, true);
    for (auto id : layers) {
        auto& layer = d.layer(id);
        expose(layer, start, end, 0);
        if (keys)
            std::erase_if(layer.keys, [&](auto k) { return k.frame >= start && k.frame < end; });
        if (keys)
            for (auto& binding : layer.bindings) {
                if (binding.bone)
                    std::erase_if(binding.bone->keys, [&](const auto& key) {
                        return key.frame >= start && key.frame < end;
                    });
                if (binding.curve)
                    std::erase_if(binding.curve->keys, [&](const auto& key) {
                        return key.frame >= start && key.frame < end;
                    });
            }
    }
}
void repeatRange(Document& d, const std::vector<Id>& layers, Frame start, Frame end, int repeats) {
    range(d, layers, start, end, true);
    if (repeats < 1 || repeats > 1000 || std::int64_t(end) + (end - start) * std::int64_t(repeats) > 1000000)
        throw std::invalid_argument("Repeat count exceeds the supported range.");
    auto clip = copyRange(d, layers, start, end);
    for (int i = 0; i < repeats; ++i)
        pasteRange(d, layers, end + i * clip.duration, clip, PasteContent::Exposures);
}
void retimeRange(Document& d, const std::vector<Id>& layers, Frame start, Frame end, Frame newLength) {
    range(d, layers, start, end, true);
    if (newLength < 1 || newLength > 1000000 || std::int64_t(start) + newLength > 1000000)
        throw std::invalid_argument("Invalid retimed range length.");
    auto clip = copyRange(d, layers, start, end);
    const Frame oldLength = end - start;
    // Preserve every drawing: refuse compression that would silently drop a cell.
    for (auto& track : clip.tracks) {
        for (auto& e : track.exposures) {
            e.start = scaled(e.start, oldLength, newLength);
            e.end = scaled(e.end, oldLength, newLength);
            if (e.end <= e.start)
                throw std::invalid_argument("Retiming would remove a drawing. Choose a longer range.");
        }
        Frame previous = -1;
        for (auto& key : track.keys) {
            key.frame = std::min(newLength - 1, scaled(key.frame, oldLength, newLength));
            if (key.frame <= previous)
                throw std::invalid_argument("Retiming would merge keys. Choose a longer range.");
            previous = key.frame;
        }
        for (auto& copied : track.deformerKeys) {
            auto scaleKeys = [&](auto& keys) {
                Frame last = -1;
                for (auto& key : keys) {
                    key.frame = std::min(newLength - 1, scaled(key.frame, oldLength, newLength));
                    if (key.frame <= last)
                        throw std::invalid_argument(
                            "Retiming would merge deformer keys. Choose a longer range.");
                    last = key.frame;
                }
            };
            scaleKeys(copied.bone);
            scaleKeys(copied.curve);
        }
    }
    clip.duration = newLength;
    const auto clearEnd = std::min(d.duration, std::max(end, start + newLength));
    clearRange(d, layers, start, clearEnd, true);
    pasteRange(d, layers, start, clip, PasteContent::All);
}
void timeDrawings(Document& d, const std::vector<Id>& layers, Frame start, Frame end, int step) {
    range(d, layers, start, end, true);
    if (step < 1 || step > 1000)
        throw std::invalid_argument("Drawing timing must be between 1 and 1000 frames.");
    auto clip = copyRange(d, layers, start, end);
    for (std::size_t i = 0; i < layers.size(); ++i) {
        const auto& track = clip.tracks[i];
        if (std::int64_t(start) + track.exposures.size() * std::int64_t(step) > 1000000)
            throw std::invalid_argument("Drawing timing exceeds the scene limit.");
        Frame at = start;
        auto& layer = d.layer(layers[i]);
        expose(layer, start, std::max(end, start + Frame(track.exposures.size()) * step), 0);
        for (auto e : track.exposures) {
            expose(layer, at, at + step, e.drawing);
            at += step;
        }
        d.duration = std::max(d.duration, at);
    }
}
void setMarker(Document& d, Frame frame, const std::string& name) {
    if (frame < 0 || frame >= d.duration || name.size() > 4096)
        throw std::invalid_argument("Invalid marker frame or label.");
    std::erase_if(d.markers, [&](const Marker& m) { return m.frame == frame; });
    if (!name.empty())
        d.markers.push_back({frame, name});
    std::sort(d.markers.begin(), d.markers.end(),
              [](const auto& a, const auto& b) { return a.frame < b.frame; });
}
} // namespace opentoon
