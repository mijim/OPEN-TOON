#include "editor_controller.h"
#include "project_store.h"
#include "scene_renderer.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>
#include <cmath>
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
    playTimer_.setTimerType(Qt::PreciseTimer);
    playTimer_.setInterval(8);
    connect(&playTimer_, &QTimer::timeout, this, [this] {
        auto elapsed = playClock_.elapsed() / 1000.0;
        setFrame((playStart_ + int(std::floor(elapsed * fps()))) % duration());
    });
    auto recoveryDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/recovery";
    QDir().mkpath(recoveryDir);
    QSettings settings;
    previousRecovery_ = settings.value("recoveryPath").toString();
    if (!QFileInfo::exists(previousRecovery_))
        previousRecovery_.clear();
    recovery_ = recoveryDir + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".otoon";
    autosaveTimer_.setInterval(60000);
    connect(&autosaveTimer_, &QTimer::timeout, this, &EditorController::autosave);
    autosaveTimer_.start();
}
EditorController::~EditorController() {
    cancelExport_ = true;
    if (exportThread_.joinable())
        exportThread_.join();
}
QString EditorController::sceneName() const {
    return QString::fromStdString(document().name);
}
void EditorController::report(QString message) {
    status_ = std::move(message);
    emit statusChanged();
}
void EditorController::resetSelection() {
    layer_ = document().layers.empty() ? 0 : document().layers.back().id;
    swatch_ = document().palette.empty() ? 0 : document().palette.front().id;
    frame_ = 0;
    emit selectionChanged();
    emit frameChanged();
    emit changed();
}
bool EditorController::edit(const std::string& label, const std::function<void(Document&)>& operation) {
    try {
        bool result = session_.apply(label, operation);
        if (result) {
            frame_ = std::clamp(frame_, 0, duration() - 1);
            emit changed();
            emit frameChanged();
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
                                     {"spans", spans},
                                     {"keys", keys}});
    }
    return result;
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
    auto t = evaluateTransform(document().layer(layer_), frame_);
    return {{"x", t.x},           {"y", t.y},           {"rotation", t.rotation},
            {"scaleX", t.scaleX}, {"scaleY", t.scaleY}, {"opacity", t.opacity},
            {"pivotX", t.pivotX}, {"pivotY", t.pivotY}};
}
void EditorController::setFrame(int value) {
    value = std::clamp(value, 0, duration() - 1);
    if (value == frame_)
        return;
    frame_ = value;
    emit frameChanged();
}
void EditorController::setSelectedLayer(int value) {
    try {
        (void)document().layer(value);
        layer_ = value;
        emit selectionChanged();
        emit changed();
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
void EditorController::setTool(QString value) {
    if (QStringList{"Pencil", "Eraser", "Select", "Rectangle", "Ellipse", "Recolor", "Edit points"}.contains(
            value)) {
        tool_ = std::move(value);
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
        emit frameChanged();
        emit selectionChanged();
        report("Undone");
    }
}
void EditorController::redo() {
    stopPlayback();
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
        emit frameChanged();
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
    if (edit("Remove layer", [&](Document& d) {
            Id parent = d.layer(layer_).parent;
            for (auto& l : d.layers)
                if (l.parent == layer_)
                    l.parent = parent;
            std::erase_if(d.layers, [&](auto& l) { return l.id == layer_; });
        }))
        resetSelection();
}
void EditorController::duplicateLayer(bool linked) {
    if (!layer_)
        return;
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
void EditorController::setParent(int parent) {
    if (!layer_)
        return;
    edit("Set parent", [&](Document& d) {
        auto& layer = d.layer(layer_);
        if (layer.locked)
            throw std::runtime_error("Unlock the layer before editing.");
        layer.parent = parent;
    });
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
    edit("Clear exposure", [&](Document& d) {
        if (d.layer(layer_).locked)
            throw std::runtime_error("Unlock the layer before editing.");
        expose(d.layer(layer_), frame_, frame_ + 1, 0);
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
        auto& l = d.layer(layer_);
        if (l.locked)
            throw std::runtime_error("Unlock the layer before editing.");
        auto set = [&](Transform& t) {
            if (field == "x")
                t.x = value;
            else if (field == "y")
                t.y = value;
            else if (field == "rotation")
                t.rotation = value;
            else if (field == "scaleX")
                t.scaleX = value;
            else if (field == "scaleY")
                t.scaleY = value;
            else if (field == "opacity")
                t.opacity = value;
            else if (field == "pivotX")
                t.pivotX = value;
            else if (field == "pivotY")
                t.pivotY = value;
        };
        if (l.keys.empty())
            set(l.transform);
        else {
            auto valueAt = evaluateTransform(l, frame_);
            set(valueAt);
            auto it = std::find_if(l.keys.begin(), l.keys.end(), [&](auto k) { return k.frame == frame_; });
            if (it == l.keys.end()) {
                l.keys.push_back({frame_, valueAt, Interpolation::Linear});
                std::sort(l.keys.begin(), l.keys.end(), [](auto a, auto b) { return a.frame < b.frame; });
            } else
                it->value = valueAt;
        }
    });
}
void EditorController::addKey(int interpolation) {
    if (!layer_)
        return;
    edit("Add transform key", [&](Document& d) {
        auto& l = d.layer(layer_);
        if (l.locked)
            throw std::runtime_error("Unlock the layer before editing.");
        auto value = evaluateTransform(l, frame_);
        std::erase_if(l.keys, [&](auto k) { return k.frame == frame_; });
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
    if (playing()) {
        playTimer_.stop();
        emit playbackChanged();
    }
}
void EditorController::togglePlayback() {
    if (playing()) {
        stopPlayback();
        return;
    }
    playStart_ = frame_;
    playClock_.start();
    playTimer_.start();
    emit playbackChanged();
}
void EditorController::importImage(QUrl url) {
    QImageReader reader(local(url));
    reader.setAutoTransform(true);
    if (reader.size().width() > 4096 || reader.size().height() > 4096) {
        report("Image import currently supports up to 4096 × 4096 pixels.");
        return;
    }
    auto image = reader.read().convertToFormat(QImage::Format_RGBA8888);
    if (image.isNull()) {
        report("Could not import image: " + reader.errorString());
        return;
    }
    if (image.width() > 4096 || image.height() > 4096) {
        report("Image import currently supports up to 4096 × 4096 pixels.");
        return;
    }
    if (!layer_)
        addLayer();
    edit("Import image", [&](Document& d) {
        auto& drawing = d.editableDrawing(layer_, frame_);
        ImageAsset asset{image.width(), image.height(), {}};
        asset.rgba.assign(image.constBits(), image.constBits() + image.sizeInBytes());
        drawing.image = std::move(asset);
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
                auto image = SceneRenderer::render(*snapshot, frame);
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
    if (!modified())
        return;
    try {
        (void)ProjectStore::save(nativePath(recovery_), document(), "Recovery snapshot");
        QSettings().setValue("recoveryPath", recovery_);
        report("Recovery snapshot saved");
    } catch (const std::exception& e) {
        report("Autosave failed: " + QString::fromUtf8(e.what()));
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
