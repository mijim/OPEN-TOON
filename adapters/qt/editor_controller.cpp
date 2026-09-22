#include "editor_controller.h"
#include "opentoon/animation.h"
#include "opentoon/property_address.h"
#include "opentoon/rigging.h"
#include "project_store.h"
#include "image_batch_importer.h"
#include "scene_renderer.h"
#include <QColorSpace>
#include <QBuffer>
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
#include <algorithm>
#include <cstdint>
#include <cmath>
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
    clearPoseSelection();
    ++sceneGeneration_;
    rangeStart_ = 0;
    rangeEnd_ = 1;
    rangeLayers_.clear();
    emit rangeChanged();
    layer_ = document().layers.empty() ? 0 : document().layers.back().id;
    selectedView_ = 0;
    emit viewSelectionChanged();
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
            emit viewSelectionChanged();
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
                                     {"kind", int(it->kind)},
                                     {"role", QString::fromStdString(it->role)},
                                     {"spans", spans},
                                     {"keys", keys}});
    }
    return result;
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
                                     {"name", QString::fromStdString(substitution.name)}});
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
                                     {"parts", int(view.choices.size())}});
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
    if (std::any_of(options.begin(), options.end(), [view](const QVariant& option) {
            return option.toMap().value("id").toInt() == view;
        })) {
        selectedView_ = view;
        emit viewSelectionChanged();
    }
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
    if (value == frame_)
        return;
    frame_ = value;
    emit frameChanged();
}
void EditorController::setSelectedLayer(int value) {
    try {
        (void)document().layer(value);
        layer_ = value;
        selectedView_ = 0;
        emit viewSelectionChanged();
        rangeLayers_ = {layer_};
        rangeStart_ = frame_;
        rangeEnd_ = frame_ + 1;
        emit rangeChanged();
        emit selectionChanged();
        emit frameChanged();
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
void EditorController::setTool(QString value) {
    if (QStringList{"Animate", "Pencil", "Eraser", "Select", "Marquee", "Lasso", "Line", "Rectangle",
                    "Ellipse", "Recolor", "Edit points", "Raster ink", "Raster soft", "Raster dry",
                    "Raster smudge", "Raster eraser"}
            .contains(value)) {
        tool_ = std::move(value);
        if (tool_ == "Animate")
            setAnimateMode(true);
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
        emit viewSelectionChanged();
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
        emit viewSelectionChanged();
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
            if (kind == LayerKind::Part) {
                opentoon::removeRigBranch(d, layer_);
                return;
            }
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
        if (layer.kind == LayerKind::Part || layer.kind == LayerKind::Peg)
            opentoon::reparentPreservingWorld(d, layer_, parent);
        else if (layer.kind == LayerKind::Drawing && parent &&
                 (d.layer(parent).kind == LayerKind::Character || d.layer(parent).kind == LayerKind::Peg))
            opentoon::attachDrawingAsPart(d, layer_, parent, layer.name);
        else
            layer.parent = parent;
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
