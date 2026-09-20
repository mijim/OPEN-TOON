#include "opentoon/key_block.h"
#include "opentoon/animation.h"
#include <cmath>
#include <set>
#include <stdexcept>
namespace opentoon {
namespace {
void editable(const Layer& layer) {
    if (layer.locked)
        throw std::runtime_error("Unlock the layer before editing keys.");
}
std::set<Frame> checkedSelection(const Layer& layer, const std::vector<Frame>& frames) {
    std::set<Frame> selected(frames.begin(), frames.end());
    if (selected.empty())
        throw std::invalid_argument("Select one or more pose keys.");
    if (selected.size() != frames.size())
        throw std::invalid_argument("Duplicate key selection.");
    std::set<Frame> available;
    for (const auto& key : layer.keys)
        available.insert(key.frame);
    for (auto frame : selected)
        if (!available.contains(frame))
            throw std::runtime_error("A selected key no longer exists. Select the keys again.");
    return selected;
}
std::vector<Frame> merge(Layer& layer, std::vector<Keyframe> remaining, std::vector<Keyframe> incoming) {
    std::set<Frame> occupied;
    for (const auto& key : remaining)
        occupied.insert(key.frame);
    std::vector<Frame> result;
    for (const auto& key : incoming) {
        if (key.frame < 0 || key.frame >= 1000000)
            throw std::invalid_argument("Keys must remain within frames 1–1000000.");
        if (!occupied.insert(key.frame).second)
            throw std::runtime_error(
                "Keys would collide. Choose an empty destination or a wider timing span.");
        result.push_back(key.frame);
    }
    remaining.insert(remaining.end(), incoming.begin(), incoming.end());
    std::sort(remaining.begin(), remaining.end(),
              [](const auto& a, const auto& b) { return a.frame < b.frame; });
    layer.keys = std::move(remaining);
    return result;
}
} // namespace
KeyBlock copyKeyBlock(const Layer& layer, const std::vector<Frame>& frames) {
    const auto selected = checkedSelection(layer, frames);
    KeyBlock block;
    for (auto key : layer.keys)
        if (selected.contains(key.frame)) {
            key.frame -= *selected.begin();
            block.keys.push_back(std::move(key));
        }
    return block;
}
std::vector<Frame> pasteKeyBlock(Layer& layer, const KeyBlock& block, Frame destination) {
    editable(layer);
    if (block.keys.empty() || destination < 0 || destination >= 1000000)
        throw std::invalid_argument("Copy pose keys before pasting at a valid frame.");
    auto incoming = block.keys;
    Frame previous = -1;
    for (auto& key : incoming) {
        if (key.frame <= previous || std::int64_t(key.frame) + destination >= 1000000)
            throw std::invalid_argument("Invalid key clipboard timing.");
        previous = key.frame;
        key.frame += destination;
    }
    return merge(layer, layer.keys, std::move(incoming));
}
std::vector<Frame> retimeKeyBlock(Layer& layer, const std::vector<Frame>& frames, Frame first, Frame last,
                                  bool duplicate) {
    editable(layer);
    const auto selected = checkedSelection(layer, frames);
    if (first < 0 || last < first || last >= 1000000 || (selected.size() == 1 && last != first))
        throw std::invalid_argument("Choose an ordered key span within frames 1–1000000.");
    std::vector<Keyframe> remaining, incoming;
    const auto start = *selected.begin(), end = *selected.rbegin();
    for (auto key : layer.keys) {
        if (!selected.contains(key.frame) || duplicate)
            remaining.push_back(key);
        if (selected.contains(key.frame)) {
            key.frame = first + (end == start ? 0
                                              : Frame(std::llround(double(key.frame - start) *
                                                                   (last - first) / (end - start))));
            incoming.push_back(std::move(key));
        }
    }
    return merge(layer, std::move(remaining), std::move(incoming));
}
void interpolateKeyBlock(Layer& layer, const std::vector<Frame>& frames, int preset) {
    editable(layer);
    const auto selected = checkedSelection(layer, frames);
    if (preset < 0 || preset > 5)
        throw std::invalid_argument("Invalid interpolation preset.");
    auto candidate = layer;
    bool affected = false;
    for (auto& key : candidate.keys) {
        if (!selected.contains(key.frame))
            continue;
        if (preset < 3) {
            key.interpolation = static_cast<Interpolation>(preset);
            key.easing.clear();
        } else {
            if (key.frame == candidate.keys.back().frame)
                continue;
            const BezierEase ease = preset == 3   ? BezierEase{1. / 3, 0, 2. / 3, 1}
                                    : preset == 4 ? BezierEase{.25, 0, .65, 1.8}
                                                  : BezierEase{.2, .8, .8, 1};
            setPoseEase(candidate, key.frame, ease);
        }
        affected = true;
    }
    if (!affected)
        throw std::invalid_argument("Easing needs a selected key with a following key.");
    layer = std::move(candidate);
}
std::vector<Frame> repeatKeyBlock(Layer& layer, const std::vector<Frame>& frames, int copies) {
    editable(layer);
    const auto selected = checkedSelection(layer, frames);
    if (copies < 1 || copies > 32 || selected.size() * copies > 100000)
        throw std::invalid_argument("Repeat needs 1–32 copies and at most 100000 new keys.");
    const auto block = copyKeyBlock(layer, frames);
    const std::int64_t period = std::int64_t(*selected.rbegin()) - *selected.begin() + 1;
    if (*selected.begin() + period * (copies + 1) > 1000000)
        throw std::invalid_argument("Repeated keys exceed the supported duration.");
    std::vector<Keyframe> incoming;
    for (int copy = 1; copy <= copies; ++copy)
        for (auto key : block.keys) {
            key.frame += Frame(*selected.begin() + period * copy);
            incoming.push_back(std::move(key));
        }
    return merge(layer, layer.keys, std::move(incoming));
}
void deleteKeyBlock(Layer& layer, const std::vector<Frame>& frames) {
    editable(layer);
    const auto selected = checkedSelection(layer, frames);
    std::erase_if(layer.keys, [&](const auto& key) { return selected.contains(key.frame); });
}
} // namespace opentoon
