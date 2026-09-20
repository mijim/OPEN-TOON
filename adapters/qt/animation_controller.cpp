#include "editor_controller.h"
#include "opentoon/animation.h"
#include <set>
using namespace opentoon;
namespace {
QVariantMap poseMap(const Transform& t) {
    return {{"x", t.x},           {"y", t.y},           {"rotation", t.rotation},
            {"scaleX", t.scaleX}, {"scaleY", t.scaleY}, {"opacity", t.opacity},
            {"pivotX", t.pivotX}, {"pivotY", t.pivotY}};
}
} // namespace
void EditorController::setAnimateMode(bool value) {
    if (animateMode_ == value)
        return;
    animateMode_ = value;
    emit animationModeChanged();
    emit changed();
}
void EditorController::setAutoKey(bool value) {
    if (autoKey_ == value)
        return;
    autoKey_ = value;
    emit animationModeChanged();
}
QVariantList EditorController::animationKeys() const {
    QVariantList result;
    if (!layer_)
        return result;
    for (const auto& key : document().layer(layer_).keys) {
        auto item = poseMap(key.value);
        item.insert("frame", key.frame);
        item.insert("interpolation", int(key.interpolation));
        result.push_back(item);
    }
    return result;
}
QString EditorController::keyState() const {
    if (!layer_)
        return "No layer";
    const auto& keys = document().layer(layer_).keys;
    if (keys.empty())
        return "Rest pose";
    if (std::any_of(keys.begin(), keys.end(), [&](const auto& k) { return k.frame == frame_; }))
        return "Keyed pose";
    if (frame_ < keys.front().frame || frame_ > keys.back().frame)
        return "Held endpoint";
    auto right = std::upper_bound(keys.begin(), keys.end(), frame_,
                                  [](Frame f, const Keyframe& k) { return f < k.frame; });
    return (right - 1)->interpolation == Interpolation::Step ? "Held pose" : "Interpolated pose";
}
QVariantList EditorController::curveSamples(QString channel, int samples) const {
    QVariantList result;
    if (!layer_)
        return result;
    const auto& layer = document().layer(layer_);
    samples = std::clamp(samples, 2, 2000);
    try {
        std::set<Frame> frames;
        for (int i = 0; i < samples; ++i)
            frames.insert(Frame(std::int64_t(i) * (duration() - 1) / (samples - 1)));
        for (const auto& key : layer.keys) {
            frames.insert(key.frame);
            if (key.frame > 0)
                frames.insert(key.frame - 1);
        }
        for (auto frame : frames)
            result.push_back(QVariantMap{
                {"frame", frame},
                {"value", transformValue(evaluateTransform(layer, frame), channel.toStdString())}});
    } catch (const std::exception&) {
        return {};
    }
    return result;
}
void EditorController::nextKey(int direction) {
    if (!layer_ || direction == 0)
        return;
    const auto& keys = document().layer(layer_).keys;
    if (direction > 0) {
        for (const auto& key : keys)
            if (key.frame > frame_) {
                setFrame(key.frame);
                return;
            }
    } else {
        for (auto it = keys.rbegin(); it != keys.rend(); ++it)
            if (it->frame < frame_) {
                setFrame(it->frame);
                return;
            }
    }
}
bool EditorController::updateKey(int source, int destination, QString channel, double value,
                                 int interpolation) {
    if (!layer_)
        return false;
    bool result = edit("Edit animation key", [&](Document& d) {
        editKey(d.layer(layer_), source, destination, channel.toStdString(), value,
                static_cast<Interpolation>(interpolation));
        d.duration = std::max(d.duration, destination + 1);
    });
    if (result)
        setFrame(destination);
    return result;
}
void EditorController::retimeSelectedKeys(int destination, int length) {
    if (edit("Retime selected keys", [&](Document& d) {
            retimeKeys(d, validRangeLayers(), rangeStart_, std::min(rangeEnd_, duration()), destination,
                       length);
        })) {
        rangeStart_ = destination;
        rangeEnd_ = std::min(destination + length, duration());
        emit rangeChanged();
    }
}
