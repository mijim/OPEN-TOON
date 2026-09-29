#include "editor_controller.h"
#include "opentoon/animation.h"
#include "opentoon/property_address.h"
#include "opentoon/rigging.h"
#include "opentoon/character_pose.h"
#include "opentoon/deformation.h"
#include "opentoon/deformer.h"
#include "opentoon/audio.h"
#include "opentoon/composition_graph.h"
#include "project_store.h"
#include "image_batch_importer.h"
#include "scene_renderer.h"
#include "graph_renderer.h"
#include "audio_wav_writer.h"
#include "audio_device.h"
#include <QColorSpace>
#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
using namespace opentoon;
namespace {
std::filesystem::path nativePath(const QString& path) {
    return std::filesystem::path(reinterpret_cast<const char8_t*>(path.toUtf8().constData()));
}
Color color(QColor c) {
    return {c.redF(), c.greenF(), c.blueF(), c.alphaF()};
}
QString local(QUrl url) {
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}
} // namespace
EditorController::EditorController(QObject* parent) : QObject(parent) {
    resetSelection();
    connect(this, &EditorController::changed, this, &EditorController::reconcilePoseSelection);
    playTimer_.setTimerType(Qt::PreciseTimer);
    playTimer_.setInterval(8);
    connect(&playTimer_, &QTimer::timeout, this, [this] {
        if (audioDevice_ && audioDevice_->interrupted()) {
            stopPlayback();
            report("Audio output was interrupted. Playback stopped.");
            return;
        }
        const int next = audioDevice_
                             ? audioDevice_->currentFrame()
                             : (playStart_ + int(std::floor(playClock_.elapsed() / 1000.0 * fps()))) % duration();
        if (next > frame_ + 1)
            skippedPlayheadFrames_ += std::uint64_t(next - frame_ - 1);
        if (next != frame_) {
            endSelectedCharacterPoseBlend();
            frame_ = next;
            emit frameChanged();
        }
    });
    auto recoveryDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/recovery";
    QDir().mkpath(recoveryDir);
    QSettings settings;
    const auto workspace = settings.value("workspaceMode", "Rig").toString();
    workspaceMode_ = workspace == "Animator" ? workspace : "Rig";
    previousRecovery_ = settings.value("recoveryPath").toString();
    if (!QFileInfo::exists(previousRecovery_))
        previousRecovery_.clear();
    recovery_ = recoveryDir + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".otoon";
    autosaveTimer_.setInterval(60000);
    connect(&autosaveTimer_, &QTimer::timeout, this, &EditorController::autosave);
    autosaveTimer_.start();
}
EditorController::~EditorController() {
    stopPlayback();
    cancelExport_ = true;
    if (exportThread_.joinable())
        exportThread_.join();
    if (recoveryThread_.joinable())
        recoveryThread_.join();
}
QString EditorController::sceneName() const {
    return QString::fromStdString(document().name);
}
void EditorController::report(QString message) {
    status_ = std::move(message);
    emit statusChanged();
}
void EditorController::resetSelection() {
    audioPeakCache_.clear();
    endSelectedCharacterPoseBlend();
    clearPoseSelection();
    ++sceneGeneration_;
    rangeStart_ = 0;
    rangeEnd_ = 1;
    rangeLayers_.clear();
    emit rangeChanged();
    layer_ = document().layers.empty() ? 0 : document().layers.back().id;
    selectedView_ = 0;
    emit viewSelectionChanged();
    selectedCharacterPose_ = 0;
    emit poseSelectionChanged();
    swatch_ = document().palette.empty() ? 0 : document().palette.front().id;
    frame_ = 0;
    emit selectionChanged();
    emit frameChanged();
    emit changed();
    emit controlGroupChanged();
}
bool EditorController::edit(const std::string& label, const std::function<void(Document&)>& operation) {
    endSelectedCharacterPoseBlend();
    stopPlayback();
    try {
        bool result = session_.apply(label, operation);
        if (result) {
            frame_ = std::clamp(frame_, 0, duration() - 1);
            if (layer_ && std::none_of(document().layers.begin(), document().layers.end(),
                                       [this](const Layer& layer) { return layer.id == layer_; })) {
                layer_ = document().layers.empty() ? 0 : document().layers.back().id;
                rangeLayers_.clear();
                selectedView_ = 0;
                selectedCharacterPose_ = 0;
                emit selectionChanged();
                emit rangeChanged();
            }
            emit changed();
            emit controlGroupChanged();
            emit frameChanged();
            emit viewSelectionChanged();
            emit poseSelectionChanged();
            report(QString::fromStdString(label));
        }
        return result;
    } catch (const std::exception& e) {
        report(QString::fromUtf8(e.what()));
        return false;
    }
}
QVariantList EditorController::layers() const {
    QVariantList result;
    for (auto it = document().layers.rbegin(); it != document().layers.rend(); ++it) {
        QVariantList spans;
        for (auto e : it->exposures)
            spans.push_back(QVariantMap{
                {"start", e.start}, {"end", e.end}, {"drawing", QVariant::fromValue<qulonglong>(e.drawing)}});
        QVariantList keys;
        for (auto k : it->keys)
            keys.push_back(k.frame);
        result.push_back(QVariantMap{{"id", int(it->id)},
                                     {"name", QString::fromStdString(it->name)},
                                     {"visible", it->visible},
                                     {"locked", it->locked},
                                     {"solo", it->solo},
                                     {"parent", int(it->parent)},
                                     {"matte", int(it->matte)},
                                     {"invertMatte", it->invertMatte},
                                     {"matteBypassed", it->matteBypassed},
                                     {"paintMatteSource", it->paintMatteSource},
                                     {"opacityBypassed", it->opacityBypassed},
                                     {"blendMode", int(it->blendMode)},
                                     {"blendBypassed", it->blendBypassed},
                                     {"compositeBypassed", it->compositeBypassed},
                                     {"kind", int(it->kind)},
                                     {"role", QString::fromStdString(it->role)},
                                     {"spans", spans},
                                     {"keys", keys}});
    }
    return result;
}
QVariantList EditorController::compositionNodes() const {
    const auto graph = CompositionGraph::orderedLayers(document());
    QVariantList result;
    std::map<GraphNodeId, const GraphNode*> indexed;
    for (const auto& node : graph.nodes)
        indexed.emplace(node.id, &node);
    for (const auto id : graph.topologicalOrder()) {
        const auto* node = indexed.at(id);
        QString kind;
        switch (node->kind) {
        case GraphNodeKind::Background: kind = "Background"; break;
        case GraphNodeKind::LayerImage: kind = "Drawing"; break;
        case GraphNodeKind::LayerTransform: kind = "Transform"; break;
        case GraphNodeKind::Opacity: kind = "Opacity"; break;
        case GraphNodeKind::BypassOpacity: kind = "Bypassed opacity"; break;
        case GraphNodeKind::Over: kind = "Composite"; break;
        case GraphNodeKind::Multiply: kind = "Multiply"; break;
        case GraphNodeKind::Screen: kind = "Screen"; break;
        case GraphNodeKind::Add: kind = "Add"; break;
        case GraphNodeKind::BypassBlend: kind = "Bypassed blend"; break;
        case GraphNodeKind::BypassComposite: kind = "Bypassed composite"; break;
        case GraphNodeKind::MatteFromImage: kind = "Cutter"; break;
        case GraphNodeKind::InvertMatte: kind = "Invert matte"; break;
        case GraphNodeKind::ApplyMatte: kind = "Apply matte"; break;
        case GraphNodeKind::BypassMatte: kind = "Bypassed cutter"; break;
        case GraphNodeKind::DisplayOutput: kind = "Display"; break;
        case GraphNodeKind::WriteOutput: kind = "Write"; break;
        }
        QVariantList inputs;
        for (const auto& input : node->inputs)
            inputs.push_back(QVariantMap{{"source", int(input.source)}, {"slot", int(input.slot)}});
        const QString name = node->layer
                                 ? QString::fromStdString(document().layer(node->layer).name)
                                 : kind;
        result.push_back(QVariantMap{{"id", int(id)}, {"kind", kind}, {"kindCode", int(node->kind)}, {"name", name},
                                     {"layer", int(node->layer)}, {"inputs", inputs}});
    }
    return result;
}
QString EditorController::compositionNodePreview(int nodeId) const {
    try {
        const auto graph = CompositionGraph::orderedLayers(document());
        const auto found = std::find_if(graph.nodes.begin(), graph.nodes.end(),
                                        [nodeId](const auto& node) { return node.id == GraphNodeId(nodeId); });
        if (nodeId <= 0 || found == graph.nodes.end() ||
            found->kind == GraphNodeKind::LayerTransform)
            return {};
        const QSize size = QSize(document().width, document().height)
                               .scaled(256, 144, Qt::KeepAspectRatio);
        auto preview = GraphRenderer::renderNode(graph, document(), frame_, GraphNodeId(nodeId),
                                                  size, {.background = false});
        if (found->kind == GraphNodeKind::MatteFromImage ||
            found->kind == GraphNodeKind::InvertMatte) {
            for (int y = 0; y < preview.height(); ++y) {
                auto* pixels = reinterpret_cast<QRgb*>(preview.scanLine(y));
                for (int x = 0; x < preview.width(); ++x) {
                    const int alpha = qAlpha(pixels[x]);
                    pixels[x] = qRgba(alpha, alpha, alpha, 255);
                }
            }
        }
        QByteArray bytes;
        QBuffer buffer(&bytes);
        if (!buffer.open(QIODevice::WriteOnly) || !preview.save(&buffer, "PNG"))
            return {};
        return QStringLiteral("data:image/png;base64,") +
               QString::fromLatin1(bytes.toBase64());
    } catch (const std::exception&) {
        return {};
    }
}
QVariantList EditorController::substitutions() const {
    QVariantList result;
    if (!layer_)
        return result;
    const auto& selected = document().layer(layer_);
    if (selected.kind != LayerKind::Part)
        return result;
    for (const auto& substitution : selected.variants)
        result.push_back(QVariantMap{{"id", int(substitution.drawing)},
                                     {"name", QString::fromStdString(substitution.name)},
                                     {"published", substitution.published},
                                     {"controlGroup", QString::fromStdString(substitution.controlGroup)},
                                     {"image", document().drawings.at(substitution.drawing).image.has_value()}});
    return result;
}
QVariantList EditorController::publishedCharacterSubstitutions() const {
    QVariantList result;
    const int root = characterId();
    if (!root)
        return result;
    for (const auto& layer : document().layers) {
        if (layer.kind != LayerKind::Part || opentoon::characterFor(document(), layer.id) != Id(root))
            continue;
        std::map<std::string, QVariantList> groups;
        for (const auto& variant : layer.variants)
            if (variant.published)
                groups[variant.controlGroup].push_back(QVariantMap{{"id", int(variant.drawing)},
                                                                 {"name", QString::fromStdString(variant.name)}});
        const auto* selected = document().drawingAt(layer.id, frame_);
        for (const auto& [group, options] : groups)
            result.push_back(QVariantMap{{"part", int(layer.id)},
                                         {"name", QString::fromStdString(layer.role)},
                                         {"group", QString::fromStdString(group)},
                                         {"selected", selected ? int(selected->id) : 0},
                                         {"options", options}});
    }
    return result;
}
int EditorController::selectedSubstitution() const {
    if (!layer_ || document().layer(layer_).kind != LayerKind::Part)
        return 0;
    const auto* drawing = document().drawingAt(layer_, frame_);
    return drawing ? int(drawing->id) : 0;
}
int EditorController::characterId() const {
    return layer_ ? int(opentoon::characterFor(document(), layer_)) : 0;
}
QVariantList EditorController::characterViews() const {
    QVariantList result;
    const int root = characterId();
    if (!root)
        return result;
    for (const auto& view : document().layer(root).views)
        result.push_back(QVariantMap{{"id", int(view.id)},
                                     {"name", QString::fromStdString(view.name)},
                                     {"parts", int(view.choices.size())},
                                     {"published", view.published},
                                     {"controlGroup", QString::fromStdString(view.controlGroup)}});
    return result;
}
int EditorController::selectedView() const {
    const int root = characterId();
    if (!root)
        return 0;
    const auto& views = document().layer(root).views;
    if (std::any_of(views.begin(), views.end(), [&](const auto& item) { return item.id == selectedView_; }))
        return int(selectedView_);
    return views.empty() ? 0 : int(views.front().id);
}
void EditorController::selectView(int view) {
    const auto options = characterViews();
    if (std::any_of(options.begin(), options.end(), [&](const QVariant& option) {
            const auto item = option.toMap();
            return item.value("id").toInt() == view &&
                   (workspaceMode_ != "Animator" ||
                    (item.value("published").toBool() &&
                     item.value("controlGroup").toString() == selectedControlGroup()));
        })) {
        selectedView_ = view;
        emit viewSelectionChanged();
    }
}
QVariantList EditorController::characterPoses() const {
    QVariantList result;
    const int root = characterId();
    if (!root)
        return result;
    for (const auto& pose : document().layer(root).poses) {
        QVariantList entries;
        for (const auto& entry : pose.parts)
            entries.push_back(QVariantMap{{"part", int(entry.part)},
                                          {"name", QString::fromStdString(document().layer(entry.part).role)},
                                          {"channels", int(entry.channels)},
                                          {"drawing", int(entry.drawing)}});
        result.push_back(QVariantMap{{"id", int(pose.id)},
                                     {"name", QString::fromStdString(pose.name)},
                                     {"parts", int(pose.parts.size())},
                                     {"published", pose.published},
                                     {"controlGroup", QString::fromStdString(pose.controlGroup)},
                                     {"entries", entries}});
    }
    return result;
}
QVariantList EditorController::audioClips() const {
    QVariantList result;
    for (const auto& clip : document().audioClips) {
        const auto& asset = *std::find_if(document().audioAssets.begin(),
            document().audioAssets.end(), [&](const auto& value) { return value.id == clip.asset; });
        result.push_back(QVariantMap{{"id", int(clip.id)},
                                     {"name", QString::fromStdString(asset.name)},
                                     {"start", int(clip.start)},
                                     {"end", int(opentoon::audioClipEndFrame(document(), clip, asset))},
                                     {"inSample", qint64(clip.inSample)},
                                     {"outSample", qint64(clip.outSample)},
                                     {"sampleRate", asset.sampleRate},
                                     {"channels", asset.channels},
                                     {"gain", clip.gain},
                                     {"repeats", clip.repeats},
                                     {"fadeInSamples", qint64(clip.fadeInSamples)},
                                     {"fadeOutSamples", qint64(clip.fadeOutSamples)},
                                     {"muted", clip.muted},
                                     {"solo", clip.solo},
                                     {"balance", clip.balance}});
    }
    return result;
}
QVariantList EditorController::audioWaveform(int clipId, int firstFrame, int frameCount) const {
    QVariantList result;
    if (frameCount < 0 || frameCount > 4096 || firstFrame < 0 ||
        firstFrame > document().duration || frameCount > document().duration - firstFrame)
        return result;
    const auto clip = std::find_if(document().audioClips.begin(), document().audioClips.end(),
                                   [=](const auto& value) { return value.id == Id(clipId); });
    if (clip == document().audioClips.end())
        return result;
    const auto asset = std::find_if(document().audioAssets.begin(), document().audioAssets.end(),
                                    [&](const auto& value) { return value.id == clip->asset; });
    if (asset == document().audioAssets.end())
        return result;
    auto cached = audioPeakCache_.find(asset->id);
    if (cached == audioPeakCache_.end() || !cached->second.matches(*asset)) {
        audioPeakCache_.erase(asset->id);
        cached = audioPeakCache_.emplace(asset->id, opentoon::AudioPeakIndex(*asset)).first;
    }
    for (int frame = firstFrame; frame < firstFrame + frameCount; ++frame) {
        if (frame < clip->start) {
            result.push_back(0.0);
            continue;
        }
        result.push_back(std::min(1.0,
            opentoon::audioClipFramePeak(document(), *clip, *asset, cached->second, frame) * clip->gain));
    }
    return result;
}
QVariantList EditorController::characterControlGroups() const {
    QVariantList result;
    const int root = characterId();
    if (!root)
        return result;
    std::set<std::string> groups;
    const auto& character = document().layer(root);
    for (const auto& view : character.views)
        if (view.published)
            groups.insert(view.controlGroup);
    for (const auto& pose : character.poses)
        if (pose.published)
            groups.insert(pose.controlGroup);
    for (const auto& layer : document().layers)
        if (layer.kind == LayerKind::Part && opentoon::characterFor(document(), layer.id) == Id(root))
            for (const auto& variant : layer.variants)
                if (variant.published)
                    groups.insert(variant.controlGroup);
    if (groups.erase("Main"))
        result.push_back(QStringLiteral("Main"));
    for (const auto& group : groups)
        result.push_back(QString::fromStdString(group));
    return result;
}
QString EditorController::selectedControlGroup() const {
    const auto groups = characterControlGroups();
    return groups.contains(selectedControlGroup_) ? selectedControlGroup_
                                                  : (groups.isEmpty() ? QStringLiteral("Main")
                                                                      : groups.front().toString());
}
void EditorController::setSelectedControlGroup(QString group) {
    if (!characterControlGroups().contains(group) || selectedControlGroup_ == group)
        return;
    endSelectedCharacterPoseBlend();
    selectedControlGroup_ = std::move(group);
    emit controlGroupChanged();
    emit poseSelectionChanged();
}
QVariantList EditorController::poseTransferTargets() const {
    QVariantList result;
    const int source = characterId();
    if (!source)
        return result;
    for (const auto& layer : document().layers)
        if (layer.kind == LayerKind::Character && layer.id != Id(source) && !layer.locked)
            result.push_back(QVariantMap{{"id", int(layer.id)},
                                         {"name", QString::fromStdString(layer.name)}});
    return result;
}
int EditorController::selectedCharacterPose() const {
    const int root = characterId();
    if (!root)
        return 0;
    const auto& poses = document().layer(root).poses;
    const auto group = selectedControlGroup().toStdString();
    if (std::any_of(poses.begin(), poses.end(), [&](const auto& item) {
            return item.id == selectedCharacterPose_ &&
                   (workspaceMode_ != "Animator" ||
                    (item.published && item.controlGroup == group));
        }))
        return int(selectedCharacterPose_);
    if (workspaceMode_ == "Animator") {
        const auto found = std::find_if(poses.begin(), poses.end(),
                                         [&](const auto& item) {
                                             return item.published && item.controlGroup == group;
                                         });
        return found == poses.end() ? 0 : int(found->id);
    }
    return poses.empty() ? 0 : int(poses.front().id);
}
void EditorController::selectCharacterPose(int poseId) {
    const auto options = characterPoses();
    if (std::any_of(options.begin(), options.end(), [&](const QVariant& option) {
            const auto item = option.toMap();
            return item.value("id").toInt() == poseId &&
                   (workspaceMode_ != "Animator" ||
                    (item.value("published").toBool() &&
                     item.value("controlGroup").toString() == selectedControlGroup()));
        })) {
        endSelectedCharacterPoseBlend();
        selectedCharacterPose_ = poseId;
        emit poseSelectionChanged();
    }
}
void EditorController::setWorkspaceMode(QString mode) {
    if ((mode != "Rig" && mode != "Animator") || mode == workspaceMode_)
        return;
    endSelectedCharacterPoseBlend();
    workspaceMode_ = std::move(mode);
    QSettings().setValue("workspaceMode", workspaceMode_);
    emit workspaceModeChanged();
    emit poseSelectionChanged();
}
QString EditorController::substitutionThumbnail(int drawingId) const {
    if (!layer_ || drawingId <= 0)
        return {};
    const auto& layer = document().layer(layer_);
    if (layer.kind != LayerKind::Part ||
        std::none_of(layer.variants.begin(), layer.variants.end(),
                     [drawingId](const auto& item) { return item.drawing == Id(drawingId); }))
        return {};
    if (thumbnailRevision_ != session_.revision()) {
        thumbnailCache_.clear();
        thumbnailRevision_ = session_.revision();
    }
    const qulonglong cacheKey = (qulonglong(layer_) << 32) ^ qulonglong(drawingId);
    if (auto found = thumbnailCache_.constFind(cacheKey); found != thumbnailCache_.cend())
        return *found;
    const auto& artwork = document().drawings.at(Id(drawingId));
    double left = 0, top = 0, right = 0, bottom = 0;
    bool hasArtwork = false;
    auto include = [&](double x, double y) {
        if (!hasArtwork) {
            left = right = x;
            top = bottom = y;
            hasArtwork = true;
        } else {
            left = std::min(left, x);
            top = std::min(top, y);
            right = std::max(right, x);
            bottom = std::max(bottom, y);
        }
    };
    if (artwork.image) {
        include(0, 0);
        include(artwork.image->width, artwork.image->height);
    }
    if (artwork.raster) {
        include(0, 0);
        include(artwork.raster->width, artwork.raster->height);
    }
    for (const auto& stroke : artwork.strokes)
        for (const auto& point : stroke.points) {
            include(point.x - stroke.width / 2, point.y - stroke.width / 2);
            include(point.x + stroke.width / 2, point.y + stroke.width / 2);
        }
    QImage preview(56, 56, QImage::Format_ARGB32_Premultiplied);
    preview.fill(Qt::transparent);
    if (hasArtwork) {
        const double width = std::max(1.0, right - left);
        const double height = std::max(1.0, bottom - top);
        const double extent = std::max(width, height) * 1.08;
        const double scale = std::min(1.0, 8192.0 / extent);
        const int side = std::max(1, int(std::ceil(extent * scale)));
        Document thumbnail;
        thumbnail.width = thumbnail.height = side;
        thumbnail.duration = 1;
        thumbnail.palette = document().palette;
        thumbnail.drawings.emplace(Id(drawingId), artwork);
        Layer sample;
        sample.id = layer_;
        sample.exposures.push_back({0, 1, Id(drawingId)});
        sample.transform.scaleX = sample.transform.scaleY = scale;
        sample.transform.x = (side - width * scale) / 2 - left * scale;
        sample.transform.y = (side - height * scale) / 2 - top * scale;
        thumbnail.layers.push_back(std::move(sample));
        RenderOptions options;
        options.background = false;
        preview = SceneRenderer::render(thumbnail, 0, {56, 56}, options);
    }
    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly) || !preview.save(&buffer, "PNG"))
        return {};
    const QString url = "data:image/png;base64," + QString::fromLatin1(bytes.toBase64());
    thumbnailCache_.insert(cacheKey, url);
    return url;
}
QVariantList EditorController::palette() const {
    QVariantList result;
    for (auto s : document().palette)
        result.push_back(
            QVariantMap{{"id", int(s.id)},
                        {"name", QString::fromStdString(s.name)},
                        {"color", QColor::fromRgbF(s.color.r, s.color.g, s.color.b, s.color.a)}});
    return result;
}
QVariantList EditorController::revisions() const {
    QVariantList result;
    if (path_.isEmpty())
        return result;
    try {
        for (auto r : ProjectStore::revisions(nativePath(path_)))
            result.push_back(QVariantMap{{"id", int(r.id)},
                                         {"label", QString::fromStdString(r.label)},
                                         {"created", QString::fromStdString(r.created)}});
    } catch (...) {
    }
    return result;
}
QVariantMap EditorController::transform() const {
    if (!layer_)
        return {};
    auto t = animateMode_ ? evaluateTransform(document().layer(layer_), frame_)
                          : document().layer(layer_).transform;
    return {{"x", t.x},           {"y", t.y},           {"rotation", t.rotation},
            {"scaleX", t.scaleX}, {"scaleY", t.scaleY}, {"opacity", t.opacity},
            {"pivotX", t.pivotX}, {"pivotY", t.pivotY}};
}
void EditorController::setFrame(int value) {
    value = std::clamp(value, 0, duration() - 1);
    if (audioDevice_)
        audioDevice_->seek(value);
    else if (scrubDevice_ && scrubDevice_->running() && value != frame_)
        scrubDevice_->scrub(value);
    else if (playing()) {
        playStart_ = value;
        playClock_.restart();
    }
    if (value == frame_)
        return;
    endSelectedCharacterPoseBlend();
    frame_ = value;
    emit frameChanged();
}
void EditorController::setSelectedLayer(int value) {
    try {
        (void)document().layer(value);
        endSelectedCharacterPoseBlend();
        layer_ = value;
        selectedView_ = 0;
        emit viewSelectionChanged();
        selectedCharacterPose_ = 0;
        emit poseSelectionChanged();
        rangeLayers_ = {layer_};
        rangeStart_ = frame_;
        rangeEnd_ = frame_ + 1;
        emit rangeChanged();
        emit selectionChanged();
        emit frameChanged();
        emit changed();
        emit controlGroupChanged();
    } catch (...) {
    }
}
void EditorController::setSelectedSwatch(int value) {
    for (auto s : document().palette)
        if (s.id == Id(value)) {
            swatch_ = value;
            emit selectionChanged();
        }
}
void EditorController::setCompositionProfile(int profile) {
    if (profile < 0 || profile > 1) {
        report("Unknown composition profile");
        return;
    }
    if (compositionProfile() == profile)
        return;
    edit("Set composition profile", [profile](Document& d) {
        d.composition = static_cast<CompositionProfile>(profile);
    });
}
double EditorController::cameraZoom() const {
    return document().activeCamera
               ? evaluateTransform(document().layer(document().activeCamera), frame_).scaleX
               : 1.0;
}
void EditorController::addCamera() {
    if (document().activeCamera) {
        setSelectedLayer(int(document().activeCamera));
        setTool("Camera");
        return;
    }
    Id added = 0;
    if (edit("Add output camera", [&](Document& d) {
            Layer camera;
            camera.id = d.allocateId();
            camera.kind = LayerKind::Camera;
            camera.name = "Output camera";
            camera.transform.x = d.width / 2.0;
            camera.transform.y = d.height / 2.0;
            added = camera.id;
            d.layers.push_back(std::move(camera));
            d.activeCamera = added;
        })) {
        setSelectedLayer(int(added));
        setTool("Camera");
    }
}
void EditorController::resetCameraPose() {
    if (!document().activeCamera)
        return;
    edit("Reset camera framing", [&](Document& d) {
        auto& camera = d.layer(d.activeCamera);
        if (camera.locked)
            throw std::runtime_error("Unlock the camera before editing.");
        Transform pose;
        pose.x = d.width / 2.0;
        pose.y = d.height / 2.0;
        recordPose(camera, frame_, pose);
    });
}
void EditorController::setCameraZoom(double zoom) {
    if (!document().activeCamera || !std::isfinite(zoom) || zoom < .05 || zoom > 20) {
        report("Camera zoom must be between 0.05 and 20.");
        return;
    }
    edit("Set camera zoom", [&](Document& d) {
        auto& camera = d.layer(d.activeCamera);
        if (camera.locked)
            throw std::runtime_error("Unlock the camera before editing.");
        auto pose = evaluateTransform(camera, frame_);
        pose.scaleX = pose.scaleY = zoom;
        recordPose(camera, frame_, pose);
    });
}
bool EditorController::commitCameraPose(const Transform& pose) {
    if (!document().activeCamera)
        return false;
    return edit("Move output camera", [&](Document& d) {
        auto& camera = d.layer(d.activeCamera);
        if (camera.locked)
            throw std::runtime_error("Unlock the camera before editing.");
        recordPose(camera, frame_, pose);
    });
}
void EditorController::setTool(QString value) {
    if (value == "Camera" && !document().activeCamera) {
        report("Add an output camera before choosing Camera.");
        return;
    }
    if (QStringList{"Animate", "Camera", "Mesh", "Pencil", "Eraser", "Select", "Marquee", "Lasso", "Line", "Rectangle",
                    "Ellipse", "Recolor", "Edit points", "Raster ink", "Raster soft", "Raster dry",
                    "Raster smudge", "Raster eraser"}
            .contains(value)) {
        tool_ = std::move(value);
        if (tool_ == "Animate" || tool_ == "Camera")
            setAnimateMode(true);
        if (tool_ == "Camera" && document().activeCamera)
            setSelectedLayer(int(document().activeCamera));
        emit toolChanged();
    }
}
void EditorController::setBrushSize(double v) {
    if (!std::isfinite(v))
        return;
    brushSize_ = std::clamp(v, 0.5, 200.0);
    emit toolChanged();
}
void EditorController::setOnionSkin(bool v) {
    onion_ = v;
    emit toolChanged();
}
void EditorController::setFilled(bool v) {
    filled_ = v;
    emit toolChanged();
}
void EditorController::setArtLayer(int v) {
    artLayer_ = std::clamp(v, 0, 3);
    emit toolChanged();
}
void EditorController::commitStroke(std::vector<Point> points) {
    if (points.empty() || !layer_)
        return;
    edit("Draw " + tool_.toStdString(), [&](Document& d) {
        auto& drawing = d.editableDrawing(layer_, frame_);
        Shape shape = tool_ == "Rectangle" ? Shape::Rectangle
                      : tool_ == "Ellipse" ? Shape::Ellipse
                                           : Shape::Stroke;
        drawing.strokes.push_back({d.allocateId(), swatch_, brushSize_, shape, filled_, artLayer_, points});
    });
}
void EditorController::eraseGesture(std::vector<Point> points) {
    if (!layer_)
        return;
    edit("Erase stroke", [&](Document& d) {
        if (!d.drawingAt(layer_, frame_))
            return;
        auto& drawing = d.editableDrawing(layer_, frame_);
        for (auto p : points)
            eraseAt(drawing, p, brushSize_ / 2, d.nextId);
    });
}
void EditorController::translateStroke(Id id, double x, double y) {
    if (!layer_)
        return;
    edit("Move selection", [&](Document& d) {
        auto& drawing = d.editableDrawing(layer_, frame_);
        for (auto& s : drawing.strokes)
            if (s.id == id)
                for (auto& p : s.points) {
                    p.x += x;
                    p.y += y;
                }
    });
}
void EditorController::recolorStroke(Id id) {
    if (!layer_)
        return;
    edit("Recolor shape", [&](Document& d) {
        auto& drawing = d.editableDrawing(layer_, frame_);
        for (auto& s : drawing.strokes)
            if (s.id == id) {
                s.swatch = swatch_;
                if (s.shape != Shape::Stroke)
                    s.filled = true;
            }
    });
}
void EditorController::newScene() {
    stopPlayback();
    session_.replace(makeDocument());
    path_.clear();
    diskRevision_ = -1;
    resetSelection();
    report("New scene — draw with your mouse or pen.");
}
void EditorController::loadDemo() {
    stopPlayback();
    session_.replace(makeBouncingBall(), false);
    path_.clear();
    diskRevision_ = -1;
    resetSelection();
    report("Bouncing ball example — 24 drawings on twos, 48 frames.");
}
bool EditorController::openProject(QUrl url) {
    try {
        auto filename = local(url);
        auto loaded = ProjectStore::load(nativePath(filename));
        stopPlayback();
        session_.replace(std::move(loaded.document));
        path_ = filename;
        diskRevision_ = loaded.revision;
        resetSelection();
        report("Opened " + QFileInfo(path_).fileName());
        return true;
    } catch (const std::exception& e) {
        report(QString::fromUtf8(e.what()));
        return false;
    }
}
bool EditorController::saveProject(QUrl url) {
    auto filename = url.isEmpty() ? path_ : local(url);
    if (filename.isEmpty()) {
        report("Choose a project file with Save As.");
        return false;
    }
    try {
        auto snapshot = session_.snapshot();
        auto expected = filename == path_ ? diskRevision_ : -1;
        auto revision = ProjectStore::save(nativePath(filename), *snapshot, "Manual save", expected);
        path_ = filename;
        diskRevision_ = revision;
        session_.markSaved(snapshot);
        QFile::remove(recovery_);
        QSettings().remove("recoveryPath");
        emit changed();
        report("Saved " + QFileInfo(path_).fileName());
        return true;
    } catch (const std::exception& e) {
        report(QString::fromUtf8(e.what()));
        return false;
    }
}
void EditorController::restoreRevision(int revision) {
    if (path_.isEmpty())
        return;
    try {
        auto loaded = ProjectStore::load(nativePath(path_), revision);
        edit("Restore revision " + std::to_string(revision), [&](Document& d) { d = loaded.document; });
        resetSelection();
    } catch (const std::exception& e) {
        report(QString::fromUtf8(e.what()));
    }
}
void EditorController::undo() {
    stopPlayback();
    endSelectedCharacterPoseBlend();
    if (session_.undo()) {
        frame_ = std::min(frame_, duration() - 1);
        try {
            (void)document().layer(layer_);
        } catch (...) {
            layer_ = document().layers.empty() ? 0 : document().layers.back().id;
        }
        if (std::none_of(document().palette.begin(), document().palette.end(),
                         [&](const auto& swatch) { return swatch.id == swatch_; }))
            swatch_ = document().palette.empty() ? 0 : document().palette.front().id;
        emit changed();
        emit controlGroupChanged();
        emit frameChanged();
        emit viewSelectionChanged();
        emit poseSelectionChanged();
        emit selectionChanged();
        report("Undone");
    }
}
void EditorController::redo() {
    stopPlayback();
    endSelectedCharacterPoseBlend();
    if (session_.redo()) {
        frame_ = std::min(frame_, duration() - 1);
        try {
            (void)document().layer(layer_);
        } catch (...) {
            layer_ = document().layers.empty() ? 0 : document().layers.back().id;
        }
        if (std::none_of(document().palette.begin(), document().palette.end(),
                         [&](const auto& swatch) { return swatch.id == swatch_; }))
            swatch_ = document().palette.empty() ? 0 : document().palette.front().id;
        emit changed();
        emit controlGroupChanged();
        emit frameChanged();
        emit viewSelectionChanged();
        emit poseSelectionChanged();
        emit selectionChanged();
        report("Redone");
    }
}
void EditorController::addLayer() {
    Id added = 0;
    if (edit("Add drawing layer", [&](Document& d) {
            Layer l;
            l.id = d.allocateId();
            added = l.id;
            l.name = "Drawing " + std::to_string(d.layers.size() + 1);
            d.layers.push_back(l);
        }))
        setSelectedLayer(int(added));
}
void EditorController::removeLayer() {
    if (!layer_)
        return;
    const auto kind = document().layer(layer_).kind;
    if (kind == LayerKind::Peg) {
        dissolvePeg();
        return;
    }
    if (edit("Remove layer", [&](Document& d) {
            for (const auto& l : d.layers)
                if (l.parent == layer_)
                    throw std::runtime_error("Remove child layers before deleting their parent.");
            for (const auto& l : d.layers)
                if (l.matte == layer_)
                    throw std::runtime_error("Remove the matte binding before deleting its source.");
            if (kind == LayerKind::Part) {
                opentoon::removeRigBranch(d, layer_);
                return;
            }
            if (kind == LayerKind::Camera)
                d.activeCamera = 0;
            Id parent = d.layer(layer_).parent;
            for (auto& l : d.layers)
                if (l.parent == layer_)
                    l.parent = parent;
            std::erase_if(d.layers, [&](auto& l) { return l.id == layer_; });
        })) {
        if (kind == LayerKind::Camera && tool_ == "Camera")
            setTool("Select");
        resetSelection();
    }
}
void EditorController::duplicateLayer(bool linked) {
    if (!layer_)
        return;
    if (document().layer(layer_).kind == LayerKind::Camera) {
        report("The current shot supports one output camera.");
        return;
    }
    if (document().layer(layer_).kind == LayerKind::Character) {
        duplicateCharacter();
        return;
    }
    if (document().layer(layer_).kind == LayerKind::Part ||
        document().layer(layer_).kind == LayerKind::Peg) {
        Id added = 0;
        if (edit(linked ? "Clone rig branch" : "Duplicate rig branch", [&](Document& d) {
                added = opentoon::duplicateRigBranch(d, layer_, linked);
            }))
            setSelectedLayer(int(added));
        return;
    }
    Id added = 0;
    if (edit(linked ? "Clone layer" : "Duplicate layer", [&](Document& d) {
            auto copy = d.layer(layer_);
            copy.id = d.allocateId();
            added = copy.id;
            copy.name += linked ? " clone" : " copy";
            if (!linked) {
                std::map<Id, Id> copies;
                for (auto& e : copy.exposures) {
                    if (!copies.contains(e.drawing)) {
                        auto drawing = d.drawings.at(e.drawing);
                        drawing.id = d.allocateId();
                        for (auto& s : drawing.strokes)
                            s.id = d.allocateId();
                        copies[e.drawing] = drawing.id;
                        d.drawings.emplace(drawing.id, std::move(drawing));
                    }
                    e.drawing = copies[e.drawing];
                }
                for (auto& variant : copy.variants) {
                    if (!copies.contains(variant.drawing)) {
                        auto drawing = d.drawings.at(variant.drawing);
                        drawing.id = d.allocateId();
                        for (auto& stroke : drawing.strokes)
                            stroke.id = d.allocateId();
                        copies[variant.drawing] = drawing.id;
                        d.drawings.emplace(drawing.id, std::move(drawing));
                    }
                    variant.drawing = copies[variant.drawing];
                }
            }
            d.layers.push_back(copy);
        }))
        setSelectedLayer(int(added));
}
void EditorController::renameLayer(int id, QString name) {
    edit("Rename layer", [&](Document& d) { d.layer(id).name = name.toStdString(); });
}
void EditorController::toggleLayer(int id, QString flag) {
    edit("Change layer state", [&](Document& d) {
        auto& l = d.layer(id);
        if (l.kind == LayerKind::Camera && flag != "locked")
            throw std::runtime_error("Camera visibility is controlled by the guide overlay.");
        if (flag == "visible")
            l.visible = !l.visible;
        else if (flag == "locked")
            l.locked = !l.locked;
        else if (flag == "solo")
            l.solo = !l.solo;
    });
}
void EditorController::moveLayer(int direction) {
    if (!layer_)
        return;
    edit("Reorder layer", [&](Document& d) {
        auto it = std::find_if(d.layers.begin(), d.layers.end(), [&](auto& l) { return l.id == layer_; });
        auto position = std::distance(d.layers.begin(), it), target = position + direction;
        if (target >= 0 && target < std::ptrdiff_t(d.layers.size()))
            std::iter_swap(it, d.layers.begin() + target);
    });
}
bool EditorController::moveDrawingAfter(int sourceLayer, int targetLayer) {
    if (sourceLayer <= 0 || targetLayer <= 0 || sourceLayer == targetLayer)
        return false;
    const auto& current = document().layers;
    const auto source = std::find_if(current.begin(), current.end(),
                                     [sourceLayer](const auto& layer) { return layer.id == Id(sourceLayer); });
    const auto target = std::find_if(current.begin(), current.end(),
                                     [targetLayer](const auto& layer) { return layer.id == Id(targetLayer); });
    if (source == current.end() || target == current.end() || source == target + 1)
        return false;
    return edit("Reorder drawing", [&](Document& d) {
        auto from = std::find_if(d.layers.begin(), d.layers.end(),
                                 [sourceLayer](const auto& layer) { return layer.id == Id(sourceLayer); });
        auto to = std::find_if(d.layers.begin(), d.layers.end(),
                               [targetLayer](const auto& layer) { return layer.id == Id(targetLayer); });
        const auto isDrawing = [](const Layer& layer) {
            return layer.kind == LayerKind::Drawing || layer.kind == LayerKind::Part;
        };
        if (!isDrawing(*from) || !isDrawing(*to) || from->locked || to->locked)
            throw std::runtime_error("Reordering needs two unlocked drawings or Parts.");
        Layer moving = std::move(*from);
        d.layers.erase(from);
        to = std::find_if(d.layers.begin(), d.layers.end(),
                          [targetLayer](const auto& layer) { return layer.id == Id(targetLayer); });
        d.layers.insert(to + 1, std::move(moving));
    });
}
bool EditorController::moveDrawingBefore(int sourceLayer, int targetLayer) {
    if (sourceLayer <= 0 || targetLayer <= 0 || sourceLayer == targetLayer)
        return false;
    const auto& current = document().layers;
    const auto source = std::find_if(current.begin(), current.end(),
                                     [sourceLayer](const auto& layer) { return layer.id == Id(sourceLayer); });
    const auto target = std::find_if(current.begin(), current.end(),
                                     [targetLayer](const auto& layer) { return layer.id == Id(targetLayer); });
    if (source == current.end() || target == current.end() || source + 1 == target)
        return false;
    return edit("Reorder drawing behind", [&](Document& d) {
        auto from = std::find_if(d.layers.begin(), d.layers.end(),
                                 [sourceLayer](const auto& layer) { return layer.id == Id(sourceLayer); });
        auto to = std::find_if(d.layers.begin(), d.layers.end(),
                               [targetLayer](const auto& layer) { return layer.id == Id(targetLayer); });
        const auto isDrawing = [](const Layer& layer) {
            return layer.kind == LayerKind::Drawing || layer.kind == LayerKind::Part;
        };
        if (!isDrawing(*from) || !isDrawing(*to) || from->locked || to->locked)
            throw std::runtime_error("Reordering needs two unlocked drawings or Parts.");
        Layer moving = std::move(*from);
        d.layers.erase(from);
        to = std::find_if(d.layers.begin(), d.layers.end(),
                          [targetLayer](const auto& layer) { return layer.id == Id(targetLayer); });
        d.layers.insert(to, std::move(moving));
    });
}
void EditorController::setParent(int parent) {
    if (!layer_)
        return;
    edit("Set parent", [&](Document& d) {
        auto& layer = d.layer(layer_);
        if (layer.kind == LayerKind::Camera ||
            (parent && d.layer(parent).kind == LayerKind::Camera))
            throw std::runtime_error("The output camera stays outside the artwork hierarchy.");
        if (layer.locked)
            throw std::runtime_error("Unlock the layer before editing.");
        if (layer.kind == LayerKind::Part || layer.kind == LayerKind::Peg)
            opentoon::reparentPreservingWorld(d, layer_, parent);
        else if (layer.kind == LayerKind::Drawing && parent &&
                 (d.layer(parent).kind == LayerKind::Character || d.layer(parent).kind == LayerKind::Peg))
            opentoon::attachDrawingAsPart(d, layer_, parent, layer.name);
        else
            layer.parent = parent;
    });
}
bool EditorController::setLayerMatte(int sourceLayer) {
    if (!layer_ || sourceLayer < 0)
        return false;
    return edit(sourceLayer ? "Set cutter matte" : "Remove cutter matte", [&](Document& d) {
        auto& target = d.layer(layer_);
        if (target.locked)
            throw std::runtime_error("Unlock the layer before editing.");
        if (target.kind != LayerKind::Drawing && target.kind != LayerKind::Part)
            throw std::runtime_error("A cutter matte needs a drawing or part target.");
        if (sourceLayer) {
            const auto& source = d.layer(sourceLayer);
            if (source.id == target.id ||
                (source.kind != LayerKind::Drawing && source.kind != LayerKind::Part) ||
                !source.visible || source.matte)
                throw std::runtime_error("Choose a visible drawing or part without its own matte.");
        }
        const Id oldMatte = target.matte;
        target.matte = Id(sourceLayer);
        if (!sourceLayer) {
            target.invertMatte = false;
            target.matteBypassed = false;
        } else if (Id(sourceLayer) != oldMatte)
            target.matteBypassed = false;
    });
}
bool EditorController::setMatteInverted(bool inverted) {
    if (!layer_)
        return false;
    return edit(inverted ? "Invert cutter matte" : "Use cutter matte inside", [&](Document& d) {
        auto& target = d.layer(layer_);
        if (target.locked || !target.matte)
            throw std::runtime_error("Select an unlocked layer with a cutter matte.");
        target.invertMatte = inverted;
    });
}
bool EditorController::setMatteBypassed(bool bypassed) {
    if (!layer_)
        return false;
    return edit(bypassed ? "Bypass cutter matte" : "Enable cutter matte", [&](Document& d) {
        auto& target = d.layer(layer_);
        if (target.locked || !target.matte)
            throw std::runtime_error("Select an unlocked layer with a cutter matte.");
        target.matteBypassed = bypassed;
    });
}
bool EditorController::setOpacityBypassed(bool bypassed) {
    if (!layer_)
        return false;
    return edit(bypassed ? "Bypass layer opacity" : "Enable layer opacity", [&](Document& d) {
        auto& target = d.layer(layer_);
        if (target.locked ||
            (target.kind != LayerKind::Drawing && target.kind != LayerKind::Part))
            throw std::runtime_error("Select an unlocked drawing or Part to bypass opacity.");
        target.opacityBypassed = bypassed;
    });
}
bool EditorController::setLayerBlendMode(int mode) {
    if (!layer_ || mode < int(LayerBlendMode::Normal) || mode > int(LayerBlendMode::Add))
        return false;
    return edit("Set layer blend mode", [&](Document& d) {
        auto& target = d.layer(layer_);
        if (target.locked ||
            (target.kind != LayerKind::Drawing && target.kind != LayerKind::Part))
            throw std::runtime_error("Select an unlocked drawing or Part to change its blend mode.");
        target.blendMode = static_cast<LayerBlendMode>(mode);
    });
}
bool EditorController::setBlendBypassed(bool bypassed) {
    if (!layer_)
        return false;
    return edit(bypassed ? "Bypass layer blend" : "Enable layer blend", [&](Document& d) {
        auto& target = d.layer(layer_);
        if (target.locked ||
            (target.kind != LayerKind::Drawing && target.kind != LayerKind::Part))
            throw std::runtime_error("Select an unlocked drawing or Part to bypass its blend mode.");
        target.blendBypassed = bypassed;
    });
}
bool EditorController::setCompositeBypassed(bool bypassed) {
    if (!layer_)
        return false;
    return edit(bypassed ? "Bypass layer composite" : "Enable layer composite",
                [&](Document& d) {
        auto& target = d.layer(layer_);
        if (target.locked ||
            (target.kind != LayerKind::Drawing && target.kind != LayerKind::Part))
            throw std::runtime_error("Select an unlocked drawing or Part to bypass its composite node.");
        target.compositeBypassed = bypassed;
    });
}
bool EditorController::setMatteSourceVisible(bool visible) {
    if (!layer_)
        return false;
    return edit(visible ? "Show cutter source" : "Hide cutter source", [&](Document& d) {
        const auto& target = d.layer(layer_);
        if (target.locked || !target.matte)
            throw std::runtime_error("Select an unlocked layer with a cutter matte.");
        auto& source = d.layer(target.matte);
        if (source.locked)
            throw std::runtime_error("Unlock the cutter source before changing its visibility.");
        source.paintMatteSource = visible;
    });
}
bool EditorController::selectedCanFollowBoneTip() const {
    if (!layer_)
        return false;
    const auto& layer = document().layer(layer_);
    if (layer.kind != LayerKind::Part || layer.locked || !layer.parent ||
        document().layer(layer.parent).kind != LayerKind::Part)
        return false;
    if (layer.boneTipAnchor)
        return true;
    const auto& source = document().layer(layer.parent);
    if (source.locked || !layer.keys.empty())
        return false;
    Frame covered = 0;
    for (const auto& exposure : source.exposures) {
        const auto* binding = opentoon::meshBindingFor(source, exposure.drawing);
        if (exposure.start != covered || !binding || !binding->bone)
            return false;
        const auto& keys = binding->bone->keys;
        if (!keys.empty() && keys.front().frame == 0 &&
            (keys.front().shoulderAngle != 0 || keys.front().elbowAngle != 0))
            return false;
        covered = exposure.end;
    }
    return covered == document().duration;
}
bool EditorController::selectedFollowsBoneTip() const {
    return layer_ && document().layer(layer_).boneTipAnchor.has_value();
}
bool EditorController::toggleSelectedBoneTipAttachment() {
    if (!selectedCanFollowBoneTip())
        return false;
    return edit(selectedFollowsBoneTip() ? "Detach from bone tip" : "Follow parent bone tip",
                [&](Document& d) {
                    const auto& child = d.layer(layer_);
                    if (child.boneTipAnchor)
                        opentoon::detachPartFromBoneTip(d, layer_);
                    else
                        opentoon::attachPartToBoneTip(d, layer_, child.parent);
                });
}
void EditorController::makeCharacter() {
    if (layer_)
        edit("Create character", [&](Document& d) {
            opentoon::makeCharacter(d, layer_, "Character " + std::to_string(d.nextId));
        });
}
void EditorController::attachUnparentedDrawings() {
    const int root = characterId();
    if (!root)
        return;
    edit("Attach unparented drawings", [&](Document& d) {
        const auto count = opentoon::attachUnparentedDrawings(d, root, frame_);
        if (!count)
            throw std::runtime_error("No exposed, unlocked root drawings are available to attach.");
    });
}
void EditorController::addPeg() {
    if (layer_)
        edit("Add parent peg", [&](Document& d) {
            opentoon::addPeg(d, layer_, "Peg " + std::to_string(d.nextId));
        });
}
void EditorController::deleteRigBranch() {
    if (layer_ && (document().layer(layer_).kind == LayerKind::Part ||
                   document().layer(layer_).kind == LayerKind::Peg) &&
        edit("Delete rig branch", [&](Document& d) {
            opentoon::removeRigBranch(d, layer_);
        }))
        resetSelection();
}
void EditorController::detachPart() {
    if (layer_ && edit("Detach character part", [&](Document& d) {
            opentoon::detachPart(d, layer_);
        }))
        emit selectionChanged();
}
void EditorController::dissolvePeg() {
    if (layer_ && edit("Dissolve peg", [&](Document& d) {
            opentoon::dissolvePeg(d, layer_);
        }))
        resetSelection();
}
void EditorController::setPartRole(QString role) {
    if (layer_)
        edit("Set part role", [&](Document& d) { opentoon::setPartRole(d, layer_, role.toStdString()); });
}
void EditorController::setRestPivot(double x, double y) {
    if (layer_)
        edit("Place rest pivot", [&](Document& d) { opentoon::setPivotPreservingArtwork(d, layer_, x, y); });
}
void EditorController::centerRestPivot() {
    if (!layer_)
        return;
    const auto* drawing = document().drawingAt(layer_, frame_);
    if (!drawing) {
        report("Expose a drawing before centering its pivot.");
        return;
    }
    double left = 0, top = 0, right = 0, bottom = 0;
    bool present = false;
    auto include = [&](double x, double y) {
        if (!present) {
            left = right = x;
            top = bottom = y;
            present = true;
        } else {
            left = std::min(left, x);
            top = std::min(top, y);
            right = std::max(right, x);
            bottom = std::max(bottom, y);
        }
    };
    if (drawing->image) {
        include(0, 0);
        include(drawing->image->width, drawing->image->height);
    }
    if (drawing->raster) {
        include(0, 0);
        include(drawing->raster->width, drawing->raster->height);
    }
    for (const auto& stroke : drawing->strokes)
        for (const auto& point : stroke.points)
            include(point.x, point.y);
    if (!present) {
        report("Draw or import artwork before centering its pivot.");
        return;
    }
    setRestPivot((left + right) / 2, (top + bottom) / 2);
}
void EditorController::createSubstitution(bool duplicate) {
    if (layer_)
        edit(duplicate ? "Duplicate substitution" : "Add blank substitution", [&](Document& d) {
            opentoon::createSubstitution(d, layer_, frame_, duplicate,
                                         "Drawing " + std::to_string(d.nextId));
        });
}
void EditorController::renameSubstitution(int drawing, QString name) {
    if (layer_)
        edit("Rename substitution", [&](Document& d) {
            opentoon::renameSubstitution(d, layer_, drawing, name.toStdString());
        });
}
void EditorController::setSelectedSubstitutionPublished(bool published) {
    const int drawing = selectedSubstitution();
    if (layer_ && drawing)
        edit(published ? "Publish substitution" : "Unpublish substitution", [&](Document& d) {
            opentoon::publishSubstitution(d, layer_, drawing, published);
        });
}
void EditorController::setSelectedSubstitutionControlGroup(QString group) {
    const int drawing = selectedSubstitution();
    if (layer_ && drawing)
        edit("Set drawing control group", [&](Document& d) {
            opentoon::setSubstitutionControlGroup(d, layer_, drawing, group.toStdString());
        });
}
bool EditorController::applyPublishedSubstitution(int partId, int drawing) {
    const int root = characterId();
    if (!root || partId <= 0 || drawing <= 0)
        return false;
    return edit("Apply published substitution", [&](Document& d) {
        const auto& part = d.layer(partId);
        if (part.kind != LayerKind::Part || opentoon::characterFor(d, partId) != Id(root) ||
            std::none_of(part.variants.begin(), part.variants.end(), [&](const auto& item) {
                return item.drawing == Id(drawing) && item.published &&
                       (workspaceMode_ != "Animator" ||
                        QString::fromStdString(item.controlGroup) == selectedControlGroup());
            }))
            throw std::invalid_argument("Select a published substitution of this character.");
        opentoon::selectSubstitution(d, partId, frame_, drawing);
    });
}
void EditorController::selectSubstitution(int drawing) {
    if (layer_)
        edit("Switch substitution", [&](Document& d) {
            opentoon::selectSubstitution(d, layer_, frame_, drawing);
        });
}
void EditorController::removeSubstitution(int drawing) {
    if (layer_)
        edit("Remove substitution", [&](Document& d) {
            opentoon::removeSubstitution(d, layer_, drawing);
        });
}
void EditorController::moveSubstitution(int drawing, int direction) {
    if (layer_)
        edit("Reorder substitution", [&](Document& d) {
            opentoon::reorderSubstitution(d, layer_, drawing, direction);
        });
}
void EditorController::stepSubstitution(int direction) {
    if (layer_)
        edit("Step substitution", [&](Document& d) {
            opentoon::stepSubstitution(d, layer_, frame_, direction);
        });
}
bool EditorController::selectedMeshBound() const {
    if (!layer_ || document().layer(layer_).kind != LayerKind::Part)
        return false;
    return opentoon::meshBindingFor(document().layer(layer_), selectedSubstitution()) != nullptr;
}
int EditorController::selectedMeshDeformer() const {
    if (!selectedMeshBound())
        return 0;
    const auto* mesh = opentoon::meshBindingFor(document().layer(layer_), selectedSubstitution());
    return mesh->bone ? 1 : mesh->curve ? 2 : 0;
}
bool EditorController::selectedCanMatchPreviousDeformerPose() const {
    return layer_ && opentoon::canMatchPreviousDeformerPose(document(), layer_, frame_);
}
int EditorController::selectedMeshColumns() const {
    if (!selectedMeshBound())
        return 0;
    return opentoon::meshBindingFor(document().layer(layer_), selectedSubstitution())->columns;
}
int EditorController::selectedMeshRows() const {
    if (!selectedMeshBound())
        return 0;
    return opentoon::meshBindingFor(document().layer(layer_), selectedSubstitution())->rows;
}
double EditorController::selectedBoneTransition() const {
    if (selectedMeshDeformer() != 1)
        return 0;
    return opentoon::meshBindingFor(document().layer(layer_), selectedSubstitution())->bone->elbowTransition;
}
double EditorController::selectedBoneMaxTransition() const {
    if (selectedMeshDeformer() != 1)
        return 0;
    const auto& joints = opentoon::meshBindingFor(document().layer(layer_),
                                                  selectedSubstitution())->bone->restJoints;
    return std::hypot(joints[1].x - joints[0].x, joints[1].y - joints[0].y) +
           std::hypot(joints[2].x - joints[1].x, joints[2].y - joints[1].y);
}
bool EditorController::bindSelectedBone() {
    const Id drawing = selectedSubstitution();
    if (!layer_ || !drawing || !selectedMeshBound())
        return false;
    return edit("Bind bone chain", [&](Document& d) {
        const auto* mesh = opentoon::meshBindingFor(d.layer(layer_), drawing);
        double minX = 1e100, minY = 1e100, maxX = -1e100, maxY = -1e100;
        for (const auto& vertex : mesh->vertices) {
            minX = std::min(minX, vertex.rest.x); minY = std::min(minY, vertex.rest.y);
            maxX = std::max(maxX, vertex.rest.x); maxY = std::max(maxY, vertex.rest.y);
        }
        const bool horizontal = maxX - minX >= maxY - minY;
        const double middleX = (minX + maxX) / 2, middleY = (minY + maxY) / 2;
        const std::array<opentoon::MeshPoint, 3> joints = horizontal
            ? std::array<opentoon::MeshPoint, 3>{{{minX, middleY}, {middleX, middleY}, {maxX, middleY}}}
            : std::array<opentoon::MeshPoint, 3>{{{middleX, minY}, {middleX, middleY}, {middleX, maxY}}};
        const double axis = horizontal ? maxX - minX : maxY - minY;
        const double cross = horizontal ? maxY - minY : maxX - minX;
        const double transition = std::min(axis,
            std::max(0.01, std::max(axis * 0.15, cross * 0.5)));
        opentoon::bindBoneChain(d, layer_, drawing, joints, transition);
    });
}
bool EditorController::bindSelectedCurve() {
    const Id drawing = selectedSubstitution();
    if (!layer_ || !drawing || !selectedMeshBound())
        return false;
    return edit("Bind curve deformer", [&](Document& d) {
        const auto* mesh = opentoon::meshBindingFor(d.layer(layer_), drawing);
        double minX = 1e100, minY = 1e100, maxX = -1e100, maxY = -1e100;
        for (const auto& vertex : mesh->vertices) {
            minX = std::min(minX, vertex.rest.x); minY = std::min(minY, vertex.rest.y);
            maxX = std::max(maxX, vertex.rest.x); maxY = std::max(maxY, vertex.rest.y);
        }
        const bool horizontal = maxX - minX >= maxY - minY;
        const double middleX = (minX + maxX) / 2, middleY = (minY + maxY) / 2;
        const opentoon::MeshPoint first = horizontal ? opentoon::MeshPoint{minX, middleY}
                                                     : opentoon::MeshPoint{middleX, minY};
        const opentoon::MeshPoint last = horizontal ? opentoon::MeshPoint{maxX, middleY}
                                                    : opentoon::MeshPoint{middleX, maxY};
        std::array<opentoon::MeshPoint, 4> controls;
        for (int index = 0; index < 4; ++index)
            controls[index] = {first.x + (last.x - first.x) * index / 3,
                               first.y + (last.y - first.y) * index / 3};
        opentoon::bindCurveDeformer(d, layer_, drawing, controls);
    });
}
bool EditorController::recordSelectedBonePose(double shoulder, double elbow) {
    const Id drawing = selectedSubstitution();
    return layer_ && drawing && edit("Pose bone chain", [&](Document& d) {
        opentoon::recordBonePose(d, layer_, drawing, frame_, shoulder, elbow);
    });
}
bool EditorController::moveSelectedBoneRestJoint(int joint, double x, double y) {
    const Id drawing = selectedSubstitution();
    return layer_ && drawing && edit("Move bone rest joint", [&](Document& d) {
        opentoon::moveBoneRestJoint(d, layer_, drawing, joint, {x, y});
    });
}
bool EditorController::setSelectedBoneTransition(double radius) {
    const Id drawing = selectedSubstitution();
    return layer_ && drawing && edit("Set elbow transition", [&](Document& d) {
        opentoon::setBoneElbowTransition(d, layer_, drawing, radius);
    });
}
bool EditorController::moveSelectedCurveControl(int control, double x, double y) {
    const Id drawing = selectedSubstitution();
    return layer_ && drawing && control >= 0 && control < 4 &&
        edit("Pose curve control", [&](Document& d) {
            const auto* mesh = opentoon::meshBindingFor(d.layer(layer_), drawing);
            if (!mesh || !mesh->curve)
                throw std::invalid_argument("This mesh has no curve deformer.");
            auto controls = opentoon::sampleCurveControls(*mesh->curve, frame_);
            controls[control] = {x, y};
            opentoon::recordCurvePose(d, layer_, drawing, frame_, controls);
        });
}
bool EditorController::moveSelectedCurveRestControl(int control, double x, double y) {
    const Id drawing = selectedSubstitution();
    return layer_ && drawing && edit("Move curve rest control", [&](Document& d) {
        opentoon::moveCurveRestControl(d, layer_, drawing, control, {x, y});
    });
}
bool EditorController::resetSelectedDeformerPose() {
    const Id drawing = selectedSubstitution();
    return layer_ && drawing && edit("Key deformer rest pose", [&](Document& d) {
        const auto* mesh = opentoon::meshBindingFor(d.layer(layer_), drawing);
        if (!mesh)
            throw std::invalid_argument("This drawing has no mesh binding.");
        if (mesh->bone)
            opentoon::recordBonePose(d, layer_, drawing, frame_, 0, 0);
        else if (mesh->curve)
            opentoon::recordCurvePose(d, layer_, drawing, frame_, mesh->curve->restControls);
        else
            throw std::invalid_argument("This mesh has no deformer.");
    });
}
bool EditorController::matchSelectedPreviousDeformerPose() {
    return layer_ && edit("Match previous deformer pose", [&](Document& d) {
        opentoon::matchPreviousDeformerPose(d, layer_, frame_);
    });
}
bool EditorController::removeSelectedDeformer() {
    const Id drawing = selectedSubstitution();
    return layer_ && drawing && edit("Remove mesh deformer", [&](Document& d) {
        opentoon::removeMeshDeformer(d, layer_, drawing);
    });
}
bool EditorController::bindSelectedMesh(int columns, int rows) {
    const Id drawing = selectedSubstitution();
    return layer_ && drawing && edit("Bind drawing mesh", [&](Document& d) {
        if (d.drawings.at(drawing).image)
            opentoon::bindRegularImageMesh(d, layer_, drawing, columns, rows);
        else
            opentoon::bindRegularVectorMesh(d, layer_, drawing, columns, rows);
    });
}
bool EditorController::bindSelectedContourMesh(int columns, int rows) {
    const Id drawing = selectedSubstitution();
    return layer_ && drawing && edit("Bind contour mesh", [&](Document& d) {
        opentoon::bindContourImageMesh(d, layer_, drawing, columns, rows);
    });
}
bool EditorController::moveSelectedMeshVertex(int vertex, double x, double y, bool rest) {
    const Id drawing = selectedSubstitution();
    return layer_ && drawing && vertex >= 0 && edit(rest ? "Edit mesh rest" : "Pose mesh", [&](Document& d) {
        if (rest)
            opentoon::moveMeshRestVertex(d, layer_, drawing, std::size_t(vertex), {x, y});
        else
            opentoon::moveMeshPoseVertex(d, layer_, drawing, std::size_t(vertex), {x, y});
    });
}
bool EditorController::resetSelectedMeshPose() {
    const Id drawing = selectedSubstitution();
    return layer_ && drawing && edit("Reset mesh pose", [&](Document& d) {
        opentoon::resetMeshPose(d, layer_, drawing);
    });
}
bool EditorController::removeSelectedMesh() {
    const Id drawing = selectedSubstitution();
    return layer_ && drawing && edit("Remove mesh binding", [&](Document& d) {
        opentoon::removeMeshBinding(d, layer_, drawing);
    });
}
void EditorController::captureSelectedCharacterPose(int channels, bool allParts) {
    const int root = characterId();
    if (!root)
        return;
    Id created = 0;
    if (edit("Capture character pose", [&](Document& d) {
            const auto& poses = d.layer(root).poses;
            int number = 1;
            auto name = "Pose " + std::to_string(number);
            while (std::any_of(poses.begin(), poses.end(),
                               [&](const auto& pose) { return pose.name == name; }))
                name = "Pose " + std::to_string(++number);
            std::vector<opentoon::PoseCaptureTarget> targets;
            if (allParts) {
                for (const auto& layer : d.layers)
                    if (layer.kind == LayerKind::Part && opentoon::characterFor(d, layer.id) == Id(root))
                        targets.push_back({layer.id, std::uint16_t(channels)});
            } else {
                targets.push_back({layer_, std::uint16_t(channels)});
            }
            created = opentoon::captureCharacterPose(d, root, frame_, targets, name);
        })) {
        selectedCharacterPose_ = created;
        emit poseSelectionChanged();
    }
}
void EditorController::applySelectedCharacterPose() {
    const int root = characterId(), poseId = selectedCharacterPose();
    if (root && poseId)
        edit("Apply character pose", [&](Document& d) {
            opentoon::applyCharacterPose(d, root, poseId, frame_);
        });
}
void EditorController::renameSelectedCharacterPose(QString name) {
    const int root = characterId(), poseId = selectedCharacterPose();
    if (root && poseId)
        edit("Rename character pose", [&](Document& d) {
            opentoon::renameCharacterPose(d, root, poseId, name.toStdString());
        });
}
void EditorController::removeSelectedCharacterPose() {
    const int root = characterId(), poseId = selectedCharacterPose();
    if (root && poseId && edit("Remove character pose", [&](Document& d) {
            opentoon::removeCharacterPose(d, root, poseId);
        })) {
        selectedCharacterPose_ = 0;
        emit poseSelectionChanged();
    }
}
void EditorController::setSelectedCharacterPosePublished(bool published) {
    const int root = characterId(), poseId = selectedCharacterPose();
    if (root && poseId)
        edit(published ? "Publish character pose" : "Unpublish character pose", [&](Document& d) {
            opentoon::publishCharacterPose(d, root, poseId, published);
        });
}
void EditorController::setSelectedCharacterPoseControlGroup(QString group) {
    const int root = characterId(), poseId = selectedCharacterPose();
    if (root && poseId)
        edit("Set pose control group", [&](Document& d) {
            opentoon::setCharacterPoseControlGroup(d, root, poseId, group.toStdString());
        });
}
void EditorController::setSelectedViewPublished(bool published) {
    const int root = characterId(), viewId = selectedView();
    if (root && viewId)
        edit(published ? "Publish character view" : "Unpublish character view", [&](Document& d) {
            opentoon::publishCharacterView(d, root, viewId, published);
        });
}
void EditorController::setSelectedViewControlGroup(QString group) {
    const int root = characterId(), viewId = selectedView();
    if (root && viewId)
        edit("Set view control group", [&](Document& d) {
            opentoon::setCharacterViewControlGroup(d, root, viewId, group.toStdString());
        });
}
void EditorController::setSelectedPartInCharacterPose(int channels) {
    const int root = characterId(), poseId = selectedCharacterPose();
    if (root && poseId && layer_)
        edit("Set Part in character pose", [&](Document& d) {
            opentoon::setCharacterPosePart(d, root, poseId, layer_, frame_,
                                           std::uint16_t(channels));
        });
}
void EditorController::removeSelectedPartFromCharacterPose() {
    const int root = characterId(), poseId = selectedCharacterPose();
    if (root && poseId && layer_)
        edit("Remove Part from character pose", [&](Document& d) {
            opentoon::removeCharacterPosePart(d, root, poseId, layer_);
        });
}
bool EditorController::transferSelectedCharacterPose(int targetCharacter) {
    const int source = characterId(), poseId = selectedCharacterPose();
    if (!source || !poseId || targetCharacter <= 0)
        return false;
    Id created = 0;
    if (!edit("Copy pose to character", [&](Document& d) {
            created = opentoon::transferCharacterPose(d, source, poseId, targetCharacter);
        }))
        return false;
    setSelectedLayer(targetCharacter);
    selectedCharacterPose_ = created;
    emit poseSelectionChanged();
    return true;
}
bool EditorController::mirrorSelectedCharacterPose() {
    const int root = characterId(), poseId = selectedCharacterPose();
    if (!root || !poseId)
        return false;
    Id created = 0;
    if (!edit("Mirror character pose", [&](Document& d) {
            created = opentoon::mirrorCharacterPose(d, root, poseId);
        }))
        return false;
    selectedCharacterPose_ = created;
    emit poseSelectionChanged();
    return true;
}
void EditorController::beginSelectedCharacterPoseBlend() {
    endSelectedCharacterPoseBlend();
    const int root = characterId(), poseId = selectedCharacterPose();
    if (!root || !poseId)
        return;
    poseBlendGesture_ = ++poseBlendSerial_;
    poseBlendRoot_ = root;
    poseBlendPose_ = poseId;
    poseBlendFrame_ = frame_;
}
bool EditorController::updateSelectedCharacterPoseBlend(double amount) {
    const int root = characterId(), poseId = selectedCharacterPose();
    if (!root || !poseId)
        return false;
    const bool temporary = !poseBlendGesture_;
    if (!poseBlendGesture_ || poseBlendRoot_ != Id(root) || poseBlendPose_ != Id(poseId) ||
        poseBlendFrame_ != frame_)
        beginSelectedCharacterPoseBlend();
    try {
        const bool published = session_.applyCoalesced("Blend character pose", poseBlendGesture_,
            [&](Document& document) {
                opentoon::blendCharacterPose(document, root, poseId, frame_, amount);
            });
        if (published) {
            emit changed();
            emit frameChanged();
            report("Blend character pose");
        }
        if (temporary)
            endSelectedCharacterPoseBlend();
        return published;
    } catch (const std::exception& e) {
        endSelectedCharacterPoseBlend();
        report(QString::fromUtf8(e.what()));
        return false;
    }
}
void EditorController::endSelectedCharacterPoseBlend() {
    if (poseBlendGesture_)
        session_.endCoalesced(poseBlendGesture_);
    poseBlendGesture_ = 0;
    poseBlendRoot_ = 0;
    poseBlendPose_ = 0;
}
void EditorController::captureCharacterView() {
    const int root = characterId();
    if (!root)
        return;
    Id created = 0;
    if (edit("Capture character view", [&](Document& d) {
            const auto& views = d.layer(root).views;
            int number = 1;
            auto name = "View " + std::to_string(number);
            while (std::any_of(views.begin(), views.end(),
                               [&](const auto& view) { return view.name == name; }))
                name = "View " + std::to_string(++number);
            created = opentoon::captureCharacterView(d, root, frame_, name);
        })) {
        selectedView_ = created;
        emit viewSelectionChanged();
    }
}
void EditorController::applyCharacterView() {
    const int root = characterId(), view = selectedView();
    if (root && view && workspaceMode_ == "Animator" &&
        std::none_of(document().layer(root).views.begin(), document().layer(root).views.end(),
                     [&](const auto& item) {
                         return item.id == Id(view) && item.published &&
                                QString::fromStdString(item.controlGroup) == selectedControlGroup();
                     }))
        return;
    if (root && view)
        edit("Apply character view", [&](Document& d) {
            opentoon::applyCharacterView(d, root, view, frame_);
        });
}
void EditorController::applyCharacterViewToRange() {
    const int root = characterId(), view = selectedView();
    if (root && view)
        edit("Apply character view to range", [&](Document& d) {
            opentoon::applyCharacterViewRange(d, root, view, rangeStart_, rangeEnd_);
        });
}
void EditorController::updateCharacterView() {
    const int root = characterId(), view = selectedView();
    if (root && view)
        edit("Update character view", [&](Document& d) {
            opentoon::updateCharacterView(d, root, view, frame_);
        });
}
void EditorController::updateSelectedPartInView() {
    const int root = characterId(), view = selectedView();
    if (root && view && layer_)
        edit("Update part in character view", [&](Document& d) {
            opentoon::updateCharacterViewPart(d, root, view, layer_, frame_);
        });
}
void EditorController::renameCharacterView(QString name) {
    const int root = characterId(), view = selectedView();
    if (root && view)
        edit("Rename character view", [&](Document& d) {
            opentoon::renameCharacterView(d, root, view, name.toStdString());
        });
}
void EditorController::duplicateCharacterView() {
    const int root = characterId(), view = selectedView();
    if (!root || !view)
        return;
    Id created = 0;
    if (edit("Duplicate character view", [&](Document& d) {
            created = opentoon::duplicateCharacterView(d, root, view);
        })) {
        selectedView_ = created;
        emit viewSelectionChanged();
    }
}
void EditorController::removeCharacterView() {
    const int root = characterId(), view = selectedView();
    if (root && view && edit("Remove character view", [&](Document& d) {
            opentoon::removeCharacterView(d, root, view);
        })) {
        selectedView_ = 0;
        emit viewSelectionChanged();
    }
}
void EditorController::moveCharacterView(int direction) {
    const int root = characterId(), view = selectedView();
    if (root && view)
        edit("Reorder character view", [&](Document& d) {
            opentoon::reorderCharacterView(d, root, view, direction);
        });
}
void EditorController::stepCharacterView(int direction) {
    const auto views = characterViews();
    if (views.isEmpty() || (direction != -1 && direction != 1))
        return;
    auto current = selectedView();
    int index = 0;
    for (int i = 0; i < views.size(); ++i)
        if (views[i].toMap().value("id").toInt() == current) {
            index = i;
            break;
        }
    const int next = (index + direction + views.size()) % views.size();
    selectView(views[next].toMap().value("id").toInt());
}
void EditorController::duplicateCharacter() {
    const int root = characterId();
    if (!root)
        return;
    Id created = 0;
    if (edit("Duplicate character", [&](Document& d) {
            created = opentoon::duplicateCharacter(d, root);
        }))
        setSelectedLayer(int(created));
}
void EditorController::newDrawing(bool duplicate) {
    if (!layer_)
        return;
    edit(duplicate ? "Duplicate drawing" : "New drawing", [&](Document& d) {
        auto& l = d.layer(layer_);
        if (l.locked)
            throw std::runtime_error("Unlock the layer before editing.");
        Drawing drawing;
        if (duplicate)
            if (auto* source = d.drawingAt(layer_, frame_))
                drawing = *source;
        drawing.id = d.allocateId();
        drawing.name = "Drawing " + std::to_string(drawing.id);
        for (auto& s : drawing.strokes)
            s.id = d.allocateId();
        Id id = drawing.id;
        d.drawings.emplace(id, std::move(drawing));
        if (l.kind == LayerKind::Part)
            l.variants.push_back({id, d.drawings.at(id).name});
        expose(l, frame_, std::min(frame_ + 2, d.duration), id);
    });
}
void EditorController::holdDrawing(int count) {
    if (!layer_ || count < 1)
        return;
    edit("Extend exposure", [&](Document& d) {
        auto& drawing = d.editableDrawing(layer_, frame_);
        expose(d.layer(layer_), frame_, std::min(frame_ + count, d.duration), drawing.id);
    });
}
void EditorController::clearExposure() {
    if (!layer_)
        return;
    edit("Clear frame and key", [&](Document& d) {
        if (d.layer(layer_).locked)
            throw std::runtime_error("Unlock the layer before editing.");
        expose(d.layer(layer_), frame_, frame_ + 1, 0);
        std::erase_if(d.layer(layer_).keys, [&](const auto& key) { return key.frame == frame_; });
    });
}
void EditorController::insertFrames(int n) {
    edit("Insert frames", [&](Document& d) { opentoon::insertFrames(d, frame_, n); });
}
void EditorController::removeFrames(int n) {
    edit("Remove frames", [&](Document& d) { opentoon::removeFrames(d, frame_, n); });
}
void EditorController::nextDrawing(int direction) {
    if (!layer_)
        return;
    const auto* current = document().drawingAt(layer_, frame_);
    Id id = current ? current->id : 0;
    for (int f = frame_ + direction; f >= 0 && f < duration(); f += direction) {
        auto* drawing = document().drawingAt(layer_, f);
        if (drawing && drawing->id != id) {
            setFrame(f);
            return;
        }
    }
}
void EditorController::addSwatch(QColor c) {
    if (!c.isValid())
        return;
    Id added = 0;
    if (edit("Add swatch", [&](Document& d) {
            added = d.allocateId();
            d.palette.push_back({added, "Color " + std::to_string(d.palette.size() + 1), color(c)});
        }))
        setSelectedSwatch(int(added));
}
void EditorController::setSwatchColor(int id, QColor c) {
    if (!c.isValid())
        return;
    edit("Edit palette color", [&](Document& d) {
        for (auto& s : d.palette)
            if (s.id == Id(id))
                s.color = color(c);
    });
}
bool EditorController::importAudio(QUrl url) {
    if (!url.isLocalFile() || QFileInfo(url.toLocalFile()).suffix().compare("wav", Qt::CaseInsensitive)) {
        report("Import a local PCM16 WAV file.");
        return false;
    }
    QFile source(url.toLocalFile());
    if (source.size() < 44 || source.size() > 128 * 1024 * 1024 || !source.open(QIODevice::ReadOnly)) {
        report("Cannot open WAV or file exceeds the 128 MiB import limit.");
        return false;
    }
    const auto bytes = source.readAll();
    if (bytes.size() != source.size()) {
        report("Could not read the complete WAV file.");
        return false;
    }
    std::vector<std::uint8_t> pcm(bytes.begin(), bytes.end());
    const auto name = QFileInfo(url.toLocalFile()).completeBaseName().toStdString();
    return edit("Import audio", [&](Document& d) {
        (void)opentoon::importPcm16Wav(d, name, std::move(pcm), frame_);
    });
}
bool EditorController::moveAudioClip(int clipId, int start) {
    return edit("Move audio clip", [&](Document& d) {
        opentoon::moveAudioClip(d, Id(clipId), start);
    });
}
bool EditorController::duplicateAudioClip(int clipId, int start) {
    return edit("Duplicate audio clip", [&](Document& d) {
        (void)opentoon::duplicateAudioClip(d, Id(clipId), start);
    });
}
bool EditorController::splitAudioClip(int clipId, int frame) {
    return edit("Split audio clip", [&](Document& d) {
        (void)opentoon::splitAudioClipAtFrame(d, Id(clipId), frame);
    });
}
bool EditorController::trimAudioClip(int clipId, int inSample, int outSample) {
    if (inSample < 0 || outSample < 0)
        return false;
    return edit("Trim audio clip", [&](Document& d) {
        opentoon::trimAudioClip(d, Id(clipId), std::uint64_t(inSample),
                                std::uint64_t(outSample));
    });
}
bool EditorController::trimAudioClipAtFrame(int clipId, int frame, bool leftEdge) {
    return edit("Trim audio clip at frame", [&](Document& d) {
        opentoon::trimAudioClipAtFrame(d, Id(clipId), frame, leftEdge);
    });
}
bool EditorController::setAudioClipGain(int clipId, double gain) {
    return edit("Set audio gain", [&](Document& d) {
        opentoon::setAudioClipGain(d, Id(clipId), gain);
    });
}
bool EditorController::setAudioClipRepeats(int clipId, int repeats) {
    return edit("Set audio repeats", [&](Document& d) {
        opentoon::setAudioClipRepeats(d, Id(clipId), repeats);
    });
}
bool EditorController::setAudioClipFades(int clipId, int fadeInSamples, int fadeOutSamples) {
    if (fadeInSamples < 0 || fadeOutSamples < 0)
        return false;
    return edit("Set audio fades", [&](Document& d) {
        opentoon::setAudioClipFades(d, Id(clipId), std::uint64_t(fadeInSamples),
                                    std::uint64_t(fadeOutSamples));
    });
}
bool EditorController::setAudioClipMuted(int clipId, bool muted) {
    return edit("Mute audio clip", [&](Document& d) {
        opentoon::setAudioClipMuted(d, Id(clipId), muted);
    });
}
bool EditorController::setAudioClipSolo(int clipId, bool solo) {
    return edit("Solo audio clip", [&](Document& d) {
        opentoon::setAudioClipSolo(d, Id(clipId), solo);
    });
}
bool EditorController::setAudioClipBalance(int clipId, double balance) {
    return edit("Set audio balance", [&](Document& d) {
        opentoon::setAudioClipBalance(d, Id(clipId), balance);
    });
}
bool EditorController::removeAudioClip(int clipId) {
    return edit("Remove audio clip", [&](Document& d) {
        opentoon::removeAudioClip(d, Id(clipId));
    });
}
void EditorController::setScene(QString name, int width, int height, int duration, int numerator,
                                int denominator) {
    edit("Scene settings", [&](Document& d) {
        if (duration < d.duration)
            throw std::runtime_error("Use Remove Frames to shorten the scene without ambiguous data loss.");
        d.name = name.toStdString();
        d.width = width;
        d.height = height;
        d.duration = duration;
        d.rate = {numerator, denominator};
    });
}
void EditorController::setTransform(QString field, double value) {
    if (!layer_)
        return;
    edit("Set " + field.toStdString(), [&](Document& d) {
        if (d.layer(layer_).kind == LayerKind::Camera) {
            auto& camera = d.layer(layer_);
            if (camera.locked)
                throw std::runtime_error("Unlock the camera before editing.");
            auto pose = evaluateTransform(camera, frame_);
            setTransformValue(pose, field.toStdString(), value);
            recordPose(camera, frame_, pose);
            return;
        }
        const PropertyEdit property{{layer_, propertyKind(field.toStdString())}, value};
        editProperties(d, std::span(&property, 1), frame_,
                       animateMode_ ? AnimationEditMode::Animate : AnimationEditMode::Setup, autoKey_);
    });
}
void EditorController::addKey(int interpolation) {
    if (interpolation < 0 || interpolation > 2)
        return;
    setAnimateMode(true);
    if (!layer_)
        return;
    edit("Add transform key", [&](Document& d) {
        auto& l = d.layer(layer_);
        if (l.locked)
            throw std::runtime_error("Unlock the layer before editing.");
        auto value = evaluateTransform(l, frame_);
        if (std::any_of(l.keys.begin(), l.keys.end(), [&](const auto& k) { return k.frame == frame_; }))
            return;
        l.keys.push_back({frame_, value, static_cast<Interpolation>(interpolation)});
        std::sort(l.keys.begin(), l.keys.end(), [](auto a, auto b) { return a.frame < b.frame; });
    });
}
void EditorController::deleteKey() {
    if (!layer_)
        return;
    edit("Delete key", [&](Document& d) {
        auto& layer = d.layer(layer_);
        if (layer.locked)
            throw std::runtime_error("Unlock the layer before editing.");
        std::erase_if(layer.keys, [&](auto k) { return k.frame == frame_; });
    });
}
void EditorController::stopPlayback() {
    endAudioScrub();
    scrubDevice_.reset();
    if (audioDevice_) {
        audioDevice_->stop();
        const auto stats = audioDevice_->stats();
        playbackCallbacks_ = stats.callbacks;
        playbackProcessingOverruns_ = stats.processingOverruns;
        playbackMaximumCallbackNanoseconds_ = stats.maximumCallbackNanoseconds;
        audioDevice_.reset();
    }
    if (playing()) {
        playTimer_.stop();
        emit playbackChanged();
        if (skippedPlayheadFrames_ || playbackProcessingOverruns_)
            report(QString("Playback stopped: %1 skipped playhead frames, %2 mixer callbacks over period.")
                       .arg(skippedPlayheadFrames_).arg(playbackProcessingOverruns_));
    }
}
void EditorController::togglePlayback() {
    if (playing()) {
        stopPlayback();
        return;
    }
    endAudioScrub();
    scrubDevice_.reset();
    playbackCallbacks_ = playbackProcessingOverruns_ = playbackMaximumCallbackNanoseconds_ = 0;
    skippedPlayheadFrames_ = 0;
    playStart_ = frame_;
    playClock_.start();
    if (!document().audioClips.empty()) {
        try {
            audioDevice_ = std::make_unique<opentoon::AudioDevice>(
                session_.snapshot(), qEnvironmentVariableIntValue("OPENTOON_TEST_NULL_AUDIO_BACKEND") == 1);
            audioDevice_->start(frame_);
        } catch (const std::exception& e) {
            audioDevice_.reset();
            report("Audio output unavailable; preview continues silently: " + QString::fromUtf8(e.what()));
        }
    }
    playTimer_.start();
    emit playbackChanged();
}
QVariantMap EditorController::playbackDiagnostics() const {
    const auto current = audioDevice_ ? audioDevice_->stats() : opentoon::AudioDeviceStats{
        playbackCallbacks_, playbackProcessingOverruns_, playbackMaximumCallbackNanoseconds_};
    return {{"callbacks", qulonglong(current.callbacks)},
            {"processingOverruns", qulonglong(current.processingOverruns)},
            {"maximumCallbackMs", double(current.maximumCallbackNanoseconds) / 1000000.0},
            {"skippedPlayheadFrames", qulonglong(skippedPlayheadFrames_)}};
}
void EditorController::beginAudioScrub() {
    if (playing() || document().audioClips.empty())
        return;
    try {
        if (!scrubDevice_)
            scrubDevice_ = std::make_unique<opentoon::AudioDevice>(
                session_.snapshot(), qEnvironmentVariableIntValue("OPENTOON_TEST_NULL_AUDIO_BACKEND") == 1);
        scrubDevice_->scrub(frame_);
    } catch (const std::exception& e) {
        scrubDevice_.reset();
        report("Audio scrub unavailable: " + QString::fromUtf8(e.what()));
    }
}
void EditorController::endAudioScrub() {
    if (scrubDevice_)
        scrubDevice_->stop();
}
bool EditorController::audioScrubbing() const {
    return scrubDevice_ && scrubDevice_->running();
}
void EditorController::importImage(QUrl url) {
    QImageReader reader(local(url));
    reader.setAutoTransform(true);
    if (reader.size().width() > 4096 || reader.size().height() > 4096) {
        report("Image import currently supports up to 4096 × 4096 pixels.");
        return;
    }
    auto image = reader.read();
    if (image.isNull()) {
        report("Could not import image: " + reader.errorString());
        return;
    }
    const bool untagged = !image.colorSpace().isValid();
    if (!untagged && image.colorSpace() != QColorSpace(QColorSpace::SRgb)) {
        image = image.convertedToColorSpace(QColorSpace(QColorSpace::SRgb));
        if (image.isNull()) {
            report("Could not convert the image color profile to sRGB.");
            return;
        }
    }
    image = image.convertToFormat(QImage::Format_RGBA8888);
    if (image.width() > 4096 || image.height() > 4096) {
        report("Image import currently supports up to 4096 × 4096 pixels.");
        return;
    }
    Id destination = layer_;
    const bool createdLayer = !destination;
    if (!edit("Import image", [&](Document& d) {
        if (!destination) {
            Layer layer;
            layer.id = d.allocateId();
            layer.name = "Imported image";
            destination = layer.id;
            d.layers.push_back(std::move(layer));
        }
        auto& drawing = d.editableDrawing(destination, frame_);
        ImageAsset asset{image.width(), image.height(), {}};
        asset.rgba.assign(image.constBits(), image.constBits() + image.sizeInBytes());
        drawing.image = std::move(asset);
    }))
        return;
    if (createdLayer)
        setSelectedLayer(static_cast<int>(destination));
    if (untagged)
        report("Imported image; untagged colors were interpreted as sRGB.");
}
bool EditorController::importParts(QVariantList urls) {
    return importImageBatch(std::move(urls), false);
}
bool EditorController::importImageSequence(QVariantList urls) {
    return importImageBatch(std::move(urls), true);
}
bool EditorController::importImageBatch(QVariantList urls, bool sequence) {
    QString error;
    const auto batch = loadImageBatch(urls, sequence, frame_, error);
    if (!batch) {
        report(error);
        return false;
    }
    const auto& inputs = batch->images;
    const auto& registration = batch->canvas;
    const int span = batch->span;
    const auto& sequencePrefix = batch->sequencePrefix;
    Id selected = 0;
    const int start = frame_;
    if (!edit(sequence ? "Import PNG sequence" : "Import registered PNG parts", [&](Document& d) {
            if (sequence) {
                d.duration = std::max(d.duration, start + span);
                Layer layer;
                layer.id = d.allocateId();
                selected = layer.id;
                layer.name = (sequencePrefix.isEmpty() ? QString("Image sequence") : sequencePrefix).toUtf8().toStdString();
                layer.transform.x = (d.width - registration.width()) / 2.0;
                layer.transform.y = (d.height - registration.height()) / 2.0;
                for (const auto& input : inputs) {
                    Drawing drawing;
                    drawing.id = d.allocateId();
                    drawing.name = input.name.toUtf8().toStdString();
                    drawing.image = input.image;
                    const Id drawingId = drawing.id;
                    d.drawings.emplace(drawingId, std::move(drawing));
                    const Frame at = start + input.number - inputs.front().number;
                    layer.exposures.push_back({at, at + 1, drawingId});
                }
                d.layers.push_back(std::move(layer));
            } else {
                for (const auto& input : inputs) {
                    Drawing drawing;
                    drawing.id = d.allocateId();
                    drawing.name = input.name.toUtf8().toStdString();
                    drawing.image = input.image;
                    const Id drawingId = drawing.id;
                    d.drawings.emplace(drawingId, std::move(drawing));
                    Layer layer;
                    layer.id = d.allocateId();
                    selected = layer.id;
                    layer.name = input.name.toUtf8().toStdString();
                    layer.transform.x = (d.width - registration.width()) / 2.0;
                    layer.transform.y = (d.height - registration.height()) / 2.0;
                    layer.exposures.push_back({start, d.duration, drawingId});
                    d.layers.push_back(std::move(layer));
                }
            }
        }))
        return false;
    setSelectedLayer(static_cast<int>(selected));
    const int gaps = sequence ? span - int(inputs.size()) : 0;
    const QString summary = sequence
                                ? QString("Imported %1 PNG frames; %2 missing %3 left empty.")
                                      .arg(inputs.size()).arg(gaps).arg(gaps == 1 ? "frame" : "frames")
                                : QString("Imported %1 registered PNG parts.").arg(inputs.size());
    report(summary + (batch->hasUntaggedColor ? " Untagged PNGs were interpreted as sRGB." : ""));
    return true;
}
void EditorController::exportAudio(QUrl url) {
    exportAudioRange(url, 0, duration());
}
void EditorController::exportAudioRange(QUrl url, int start, int end) {
    if (exporting_ || !url.isLocalFile())
        return;
    if (start < 0 || end > duration() || start >= end) {
        report("Select a nonempty audio export frame range.");
        return;
    }
    const auto destination = url.toLocalFile();
    if (destination.isEmpty())
        return;
    if (exportThread_.joinable())
        exportThread_.join();
    exporting_ = true;
    exportProgress_ = 0;
    cancelExport_ = false;
    emit exportChanged();
    const auto snapshot = session_.snapshot();
    exportThread_ = std::thread([this, snapshot, destination, start, end] {
        QString error;
        bool cancelled = false;
        std::int64_t samples = 0;
        try {
            QSaveFile file(destination);
            if (!file.open(QIODevice::WriteOnly))
                throw std::runtime_error("Could not create the PCM WAV output.");
            int lastPercent = -1;
            const auto result = opentoon::writeAudioWavRange(*snapshot, file, start, end,
                [this] { return cancelExport_.load(); },
                [this, &lastPercent](double progress) {
                    const int percent = int(progress * 100);
                    if (percent != lastPercent) {
                        lastPercent = percent;
                        QMetaObject::invokeMethod(this, [this, progress] {
                            exportProgress_ = progress;
                            emit exportChanged();
                        }, Qt::QueuedConnection);
                    }
                });
            samples = result.sampleFrames;
            if (cancelExport_)
                throw opentoon::AudioExportCancelled();
            if (!file.commit())
                throw std::runtime_error("Could not commit the PCM WAV output.");
        } catch (const opentoon::AudioExportCancelled&) {
            cancelled = true;
        } catch (const std::exception& e) {
            error = QString::fromUtf8(e.what());
        }
        QMetaObject::invokeMethod(this, [this, destination, samples, cancelled, error,
                                         start, end] {
            exporting_ = false;
            emit exportChanged();
            report(cancelled ? "Audio export cancelled." :
                   error.isEmpty() ? QString("Exported %1 audio samples (frames %2–%3) to %4")
                                        .arg(samples).arg(start + 1).arg(end).arg(destination)
                                   : "Audio export failed: " + error);
        }, Qt::QueuedConnection);
    });
}
void EditorController::exportFrames(QUrl url) {
    if (exporting_)
        return;
    auto folder = local(url);
    if (folder.isEmpty())
        return;
    auto destination = folder + "/OPEN-TOON-" +
                       QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz") + "-" +
                       QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    if (!QDir().mkpath(destination)) {
        report("Could not create export directory.");
        return;
    }
    if (exportThread_.joinable())
        exportThread_.join();
    exporting_ = true;
    exportProgress_ = 0;
    cancelExport_ = false;
    emit exportChanged();
    auto snapshot = session_.snapshot();
    exportThread_ = std::thread([this, snapshot, destination] {
        int written = 0;
        QString error;
        try {
            for (int frame = 0; frame < snapshot->duration; ++frame) {
                if (cancelExport_)
                    break;
                RenderOptions options;
                options.cancelled = [this] { return cancelExport_.load(); };
                auto image = SceneRenderer::render(*snapshot, frame, {}, options);
                QSaveFile file(destination + QString("/frame_%1.png").arg(frame + 1, 6, 10, QChar('0')));
                if (!file.open(QIODevice::WriteOnly) || !image.save(&file, "PNG") || !file.commit())
                    throw std::runtime_error("Could not write an exported frame.");
                ++written;
                QMetaObject::invokeMethod(
                    this,
                    [this, written, total = snapshot->duration] {
                        exportProgress_ = double(written) / total;
                        emit exportChanged();
                    },
                    Qt::QueuedConnection);
            }
        } catch (const RenderCancelled&) {
            cancelExport_ = true;
        } catch (const std::exception& e) {
            error = QString::fromUtf8(e.what());
        }
        QString state = error.isEmpty() ? (cancelExport_ ? "cancelled" : "complete") : "failed";
        QJsonObject manifest{{"status", state},
                             {"framesWritten", written},
                             {"expectedFrames", snapshot->duration},
                             {"fpsNumerator", snapshot->rate.numerator},
                             {"fpsDenominator", snapshot->rate.denominator},
                             {"error", error}};
        QSaveFile file(destination + "/manifest.json");
        const auto bytes = QJsonDocument(manifest).toJson();
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
            state = "failed";
            error = "Could not write the export manifest.";
        }
        QMetaObject::invokeMethod(
            this,
            [this, written, destination, state, error] {
                exporting_ = false;
                emit exportChanged();
                report(state == "complete" ? QString("Exported %1 frames to %2").arg(written).arg(destination)
                                           : "Export " + state + ": " + error + " — " + destination);
            },
            Qt::QueuedConnection);
    });
}
void EditorController::cancelExport() {
    cancelExport_ = true;
}
void EditorController::autosave() {
    if (!modified() || savingRecovery_)
        return;
    if (recoveryThread_.joinable())
        recoveryThread_.join();
    const auto snapshot = session_.snapshot();
    const auto generation = sceneGeneration_;
    const auto destination = recovery_;
    savingRecovery_ = true;
    emit recoveryChanged();
    recoveryThread_ = std::thread([this, snapshot, generation, destination] {
        QString error;
        try {
            (void)ProjectStore::save(nativePath(destination), *snapshot, "Recovery snapshot");
            // Persist even when the application closes before queued UI delivery.
            QSettings settings;
            settings.setValue("recoveryPath", destination);
            settings.sync();
            if (settings.status() != QSettings::NoError)
                throw std::runtime_error("Recovery file was saved but its location could not be recorded: " +
                                         destination.toStdString());
        } catch (const std::exception& e) {
            error = QString::fromUtf8(e.what());
        }
        QMetaObject::invokeMethod(
            this,
            [this, generation, error] {
                savingRecovery_ = false;
                emit recoveryChanged();
                if (generation == sceneGeneration_)
                    report(error.isEmpty() ? "Recovery snapshot saved" : "Autosave failed: " + error);
            },
            Qt::QueuedConnection);
    });
}
void EditorController::compactProject(int retain) {
    if (path_.isEmpty() || modified()) {
        report("Save this scene before compacting its history.");
        return;
    }
    try {
        auto result = ProjectStore::compact(nativePath(path_), retain, diskRevision_);
        report(QString("Retained %1 revisions. Original history backup: %2")
                   .arg(result.retainedRevisions)
                   .arg(QString::fromStdString(result.backup.string())));
        emit changed();
    } catch (const std::exception& e) {
        report(QString::fromUtf8(e.what()));
    }
}
void EditorController::recover() {
    if (previousRecovery_.isEmpty())
        return;
    if (openProject(QUrl::fromLocalFile(previousRecovery_))) {
        path_.clear();
        diskRevision_ = -1;
        session_.replace(session_.document(), false);
        emit changed();
        report("Recovered scene. Use Save As to keep it.");
    }
}

void EditorController::movePoint(Id strokeId, int index, Point point) {
    if (!layer_)
        return;
    edit("Move control point", [&](Document& d) {
        auto& drawing = d.editableDrawing(layer_, frame_);
        for (auto& s : drawing.strokes)
            if (s.id == strokeId && index >= 0 && index < int(s.points.size()))
                s.points[index] = point;
    });
}
void EditorController::smoothStroke(Id strokeId) {
    if (!layer_)
        return;
    edit("Smooth stroke", [&](Document& d) {
        auto& drawing = d.editableDrawing(layer_, frame_);
        for (auto& s : drawing.strokes)
            if (s.id == strokeId && s.shape == Shape::Stroke) {
                auto source = s.points;
                for (std::size_t i = 1; i + 1 < source.size(); ++i) {
                    s.points[i].x = (source[i - 1].x + 2 * source[i].x + source[i + 1].x) / 4;
                    s.points[i].y = (source[i - 1].y + 2 * source[i].y + source[i + 1].y) / 4;
                }
            }
    });
}
void EditorController::deleteStroke(Id strokeId) {
    if (!layer_)
        return;
    edit("Delete stroke", [&](Document& d) {
        auto& drawing = d.editableDrawing(layer_, frame_);
        std::erase_if(drawing.strokes, [=](const Stroke& stroke) { return stroke.id == strokeId; });
    });
}

void EditorController::commitRaster(opentoon::RasterImage image) {
    edit("Raster brush gesture", [&](Document& d) {
        if (d.layer(layer_).locked)
            throw std::runtime_error("Unlock the layer before painting.");
        d.editableDrawing(layer_, frame_).raster = std::move(image);
    });
}
