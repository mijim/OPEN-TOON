#include "editor_controller.h"
#include "project_store.h"
#include "scene_renderer.h"
#include <QCoreApplication>
#include <QColorSpace>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
namespace {
const QString partFixture = QStringLiteral(OPENTOON_SOURCE_DIR "/tests/fixtures/harmony-moment/parts/");
QVariantList paths(std::initializer_list<QString> values) {
    QVariantList result;
    for (const auto& value : values)
        result.push_back(QUrl::fromLocalFile(value));
    return result;
}
void waitForExport(EditorController& editor) {
    QElapsedTimer timeout;
    timeout.start();
    while (editor.exporting() && timeout.elapsed() < 30000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    REQUIRE_FALSE(editor.exporting());
}
QJsonObject manifest(const QDir& root, QString folder) {
    QFile file(root.filePath(folder + "/manifest.json"));
    REQUIRE(file.open(QIODevice::ReadOnly));
    return QJsonDocument::fromJson(file.readAll()).object();
}
} // namespace
TEST_CASE("Registered PNG parts preserve a shared canvas and undo as one edit") {
    EditorController editor;
    editor.newScene();
    editor.setFrame(7);
    const auto before = editor.document();
    const auto originalPalette = before.palette;
    REQUIRE(editor.importParts(paths({partFixture + "hand_right__open.png",
                                      partFixture + "torso__base.png",
                                      partFixture + "head__front.png"})));
    const auto imported = editor.document();
    REQUIRE(imported.layers.size() == before.layers.size() + 3);
    REQUIRE(imported.drawings.size() == before.drawings.size() + 3);
    REQUIRE(imported.palette == originalPalette);
    REQUIRE(imported.layers[before.layers.size()].name == "hand_right__open");
    REQUIRE(imported.layers[before.layers.size() + 1].name == "head__front");
    REQUIRE(imported.layers[before.layers.size() + 2].name == "torso__base");
    for (std::size_t index = before.layers.size(); index < imported.layers.size(); ++index) {
        const auto& layer = imported.layers[index];
        REQUIRE(layer.transform.x == 832);
        REQUIRE(layer.transform.y == 412);
        REQUIRE(layer.exposures.size() == 1);
        REQUIRE(layer.exposures.front().start == 7);
        REQUIRE(layer.exposures.front().end == imported.duration);
        const auto& asset = *imported.drawings.at(layer.exposures.front().drawing).image;
        REQUIRE(asset.width == 256);
        const auto source = QImage(partFixture + QString::fromStdString(layer.name) + ".png")
                                .convertToFormat(QImage::Format_RGBA8888);
        REQUIRE_FALSE(source.isNull());
        for (int y = 0; y < source.height(); ++y)
            REQUIRE(std::memcmp(asset.rgba.data() + y * asset.width * 4,
                                source.constScanLine(y), asset.width * 4) == 0);
    }
    const auto rendered = opentoon::SceneRenderer::render(imported, 7);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto project = QUrl::fromLocalFile(directory.filePath("registered-parts.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == imported);
    REQUIRE(opentoon::SceneRenderer::render(reopened.document(), 7) == rendered);
    editor.undo();
    REQUIRE(editor.document() == before);
    editor.redo();
    REQUIRE(editor.document() == imported);
}

TEST_CASE("Character inspector actions build a saved rigid rig with held substitutions") {
    EditorController editor;
    editor.newScene();
    REQUIRE(editor.importParts(paths({partFixture + "torso__base.png", partFixture + "head__front.png"})));
    const auto head = editor.selectedLayer();
    const auto body = int(editor.document().layers[1].id);
    const auto before = opentoon::SceneRenderer::render(editor.document(), 0, {320, 180});
    editor.makeCharacter();
    REQUIRE(editor.document().layer(head).kind == opentoon::LayerKind::Part);
    const auto character = editor.document().layer(head).parent;
    editor.addPeg();
    REQUIRE(editor.document().layer(head).parent != character);
    editor.setPartRole("Head");
    editor.centerRestPivot();
    REQUIRE(editor.document().layer(head).transform.pivotX == 128);
    REQUIRE(editor.document().layer(head).transform.pivotY == 128);
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 0, {320, 180}) == before);
    editor.setSelectedLayer(body);
    editor.setParent(int(character));
    REQUIRE(editor.document().layer(body).kind == opentoon::LayerKind::Part);
    REQUIRE(editor.substitutions().size() == 1);
    const auto original = editor.selectedSubstitution();
    editor.createSubstitution(true);
    const auto open = editor.selectedSubstitution();
    REQUIRE(open != original);
    editor.renameSubstitution(open, "Turned head");
    REQUIRE(editor.substitutions().back().toMap().value("name").toString() == "Turned head");
    editor.setFrame(12);
    editor.selectSubstitution(original);
    REQUIRE(editor.selectedSubstitution() == original);
    REQUIRE(editor.document().drawingAt(body, 11)->id == open);
    editor.removeSubstitution(original);
    REQUIRE(editor.selectedSubstitution() == open);
    const auto rigged = editor.document();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto project = QUrl::fromLocalFile(directory.filePath("rig.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == rigged);
    REQUIRE(opentoon::SceneRenderer::render(reopened.document(), 0, {320, 180}) ==
            opentoon::SceneRenderer::render(rigged, 0, {320, 180}));
    editor.undo();
    REQUIRE(editor.document() != rigged);
    editor.redo();
    REQUIRE(editor.document() == rigged);
}

TEST_CASE("Rejected registered part batch leaves the scene and selection intact") {
    EditorController editor;
    editor.newScene();
    const auto before = editor.document();
    const auto selected = editor.selectedLayer();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto corrupt = directory.filePath("broken.png");
    QFile invalid(corrupt);
    REQUIRE(invalid.open(QIODevice::WriteOnly));
    REQUIRE(invalid.write("not a PNG") == 9);
    invalid.close();
    REQUIRE_FALSE(editor.importParts(paths({partFixture + "torso__base.png", corrupt})));
    REQUIRE(editor.document() == before);
    REQUIRE(editor.selectedLayer() == selected);
    QImage mismatched(16, 16, QImage::Format_RGBA8888);
    mismatched.fill(Qt::transparent);
    const auto wrongSize = directory.filePath("wrong-size.png");
    REQUIRE(mismatched.save(wrongSize));
    REQUIRE_FALSE(editor.importParts(paths({partFixture + "torso__base.png", wrongSize})));
    REQUIRE(editor.document() == before);
    REQUIRE(editor.selectedLayer() == selected);
}

TEST_CASE("Numbered PNG sequence orders frames, reports gaps and round-trips") {
    EditorController editor;
    editor.newScene();
    editor.setFrame(47);
    const auto before = editor.document();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto first = directory.filePath("walk_0001.png");
    const auto third = directory.filePath("walk_0003.png");
    REQUIRE(QFile::copy(partFixture + "torso__base.png", first));
    REQUIRE(QFile::copy(partFixture + "head__front.png", third));
    REQUIRE(editor.importImageSequence(paths({third, first})));
    const auto imported = editor.document();
    REQUIRE(imported.duration == 50);
    REQUIRE(imported.layers.size() == before.layers.size() + 1);
    const auto& layer = imported.layers.back();
    REQUIRE(layer.name == "walk_");
    REQUIRE(layer.exposures.size() == 2);
    REQUIRE(layer.exposures[0].start == 47);
    REQUIRE(layer.exposures[0].end == 48);
    REQUIRE(layer.exposures[1].start == 49);
    REQUIRE(layer.exposures[1].end == 50);
    REQUIRE_FALSE(imported.drawingAt(layer.id, 48));
    REQUIRE(editor.status().contains("1 missing frame"));
    const auto project = QUrl::fromLocalFile(directory.filePath("sequence.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == imported);
    REQUIRE(opentoon::SceneRenderer::render(reopened.document(), 49) ==
            opentoon::SceneRenderer::render(imported, 49));
    editor.undo();
    REQUIRE(editor.document() == before);
    REQUIRE_FALSE(editor.importImageSequence(paths({first, first})));
    REQUIRE(editor.document() == before);
}
TEST_CASE("Asynchronous export uses one snapshot and publishes complete rational-time metadata") {
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    EditorController editor;
    editor.loadDemo();
    editor.setScene("Export fixture", 1920, 1080, 48, 24000, 1001);
    const auto snapshot = editor.document();
    editor.exportFrames(QUrl::fromLocalFile(temporary.path()));
    REQUIRE(editor.exporting());
    // A visible edit after dispatch must not leak into the running render.
    editor.setSwatchColor(editor.selectedSwatch(), Qt::red);
    waitForExport(editor);
    QDir root(temporary.path());
    auto folders = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    REQUIRE(folders.size() == 1);
    auto info = manifest(root, folders.front());
    REQUIRE(info["status"].toString() == "complete");
    REQUIRE(info["framesWritten"].toInt() == 48);
    REQUIRE(info["fpsNumerator"].toInt() == 24000);
    REQUIRE(info["fpsDenominator"].toInt() == 1001);
    QDir output(root.filePath(folders.front()));
    REQUIRE(output.entryList({"frame_*.png"}, QDir::Files).size() == 48);
    auto actual =
        QImage(output.filePath("frame_000001.png")).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    REQUIRE(actual == opentoon::SceneRenderer::render(snapshot, 0));
}
TEST_CASE("Cancellation publishes an explicitly partial export and the next job can complete") {
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    EditorController editor;
    editor.setScene("Cancellation fixture", 64, 64, 1000, 24, 1);
    editor.exportFrames(QUrl::fromLocalFile(temporary.path()));
    editor.cancelExport();
    waitForExport(editor);
    QDir root(temporary.path());
    auto first = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    REQUIRE(first.size() == 1);
    auto cancelled = manifest(root, first.front());
    REQUIRE(cancelled["status"].toString() == "cancelled");
    REQUIRE(cancelled["framesWritten"].toInt() < 1000);
    editor.newScene();
    editor.setScene("Next job", 64, 64, 48, 24, 1);
    editor.exportFrames(QUrl::fromLocalFile(temporary.path()));
    waitForExport(editor);
    auto next = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    REQUIRE(next.size() == 2);
    next.removeAll(first.front());
    REQUIRE(manifest(root, next.front())["status"].toString() == "complete");
}
TEST_CASE("Recovery saves an immutable snapshot without clearing newer unsaved edits") {
    EditorController editor;
    editor.setScene("Before autosave", 128, 128, 48, 24, 1);
    const auto saved = editor.document();
    editor.autosave();
    REQUIRE(editor.savingRecovery());
    editor.setScene("After autosave", 128, 128, 48, 24, 1);
    QElapsedTimer timeout;
    timeout.start();
    while (editor.savingRecovery() && timeout.elapsed() < 30000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    REQUIRE_FALSE(editor.savingRecovery());
    REQUIRE(editor.modified());
    REQUIRE(editor.sceneName() == "After autosave");
    auto path = QSettings().value("recoveryPath").toString();
    INFO(editor.status().toStdString());
    REQUIRE_FALSE(path.isEmpty());
    REQUIRE(opentoon::ProjectStore::load(std::filesystem::path(path.toStdString())).document == saved);
    QFile::remove(path);
}

TEST_CASE("Visual curve commands preserve poses and undo atomically") {
    EditorController editor;
    editor.newScene();
    auto before = editor.document();
    REQUIRE(editor.addCurveKey(20, "x", 100));
    REQUIRE(editor.setCurveHandles(0, "x", .25, 0, .65, 1.8));
    auto eased = editor.document();
    editor.setFrame(0);
    editor.addKey();
    REQUIRE(editor.document() == eased);
    REQUIRE_FALSE(editor.setCurveHandles(0, "x", .8, 0, .2, 1));
    REQUIRE(editor.document() == eased);
    editor.undo();
    editor.undo();
    REQUIRE(editor.document() == before);
}

TEST_CASE("Clear removes range exposures and keys together and undo restores both") {
    EditorController editor;
    editor.newScene();
    editor.holdDrawing(40);
    REQUIRE(editor.addCurveKey(10, "x", 100));
    REQUIRE(editor.addCurveKey(20, "y", 60));
    REQUIRE(editor.setPoseCurveHandles(0, .3, 0, .7, 1));
    auto original = editor.document();
    editor.selectTimelineRange(0, 10, 0, 0);
    editor.clearTimelineRange();
    const auto& layer = editor.document().layer(editor.selectedLayer());
    REQUIRE(layer.keys.size() == 1);
    REQUIRE(layer.keys.front().frame == 20);
    REQUIRE_FALSE(editor.document().drawingAt(layer.id, 0));
    REQUIRE(editor.document().drawingAt(layer.id, 11));
    editor.undo();
    REQUIRE(editor.document() == original);
    editor.setFrame(10);
    editor.clearExposure();
    REQUIRE(editor.document().layer(editor.selectedLayer()).keys.size() == 2);
    REQUIRE_FALSE(editor.document().drawingAt(editor.selectedLayer(), 10));
    editor.undo();
    REQUIRE(editor.document() == original);
    editor.toggleLayer(editor.selectedLayer(), "locked");
    auto locked = editor.document();
    editor.clearTimelineRange();
    REQUIRE(editor.document() == locked);
}
int main(int argc, char** argv) {
    QGuiApplication application(argc, argv);
    QCoreApplication::setApplicationName("OPEN-TOON-export-tests");
    QCoreApplication::setOrganizationName("OPEN-TOON-tests");
    QTemporaryDir settingsDirectory;
    if (!settingsDirectory.isValid())
        return 1;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
    QStandardPaths::setTestModeEnabled(true);
    return Catch::Session().run(argc, argv);
}

TEST_CASE("Animation inspector edits, curve sampling, navigation and saved rendering agree") {
    EditorController editor;
    editor.loadDemo();
    const auto layer = editor.selectedLayer();
    editor.setAnimateMode(true);
    editor.setAutoKey(true);
    editor.setFrame(0);
    editor.setTransform("x", 0);
    editor.setFrame(20);
    editor.setTransform("x", 200);
    editor.setAutoKey(false);
    editor.setFrame(10);
    const auto before = editor.document();
    editor.setTransform("x", 999);
    REQUIRE(editor.document() == before);
    REQUIRE(editor.keyState() == "Interpolated pose");
    editor.nextKey(1);
    REQUIRE(editor.frame() == 20);
    editor.nextKey(-1);
    REQUIRE(editor.frame() == 0);
    REQUIRE(editor.updateKey(20, 24, "x", 240, 1));
    editor.setFrame(12);
    REQUIRE(editor.transform()["x"].toDouble() == 120);
    auto samples = editor.curveSamples("x", 48);
    REQUIRE_FALSE(samples.empty());
    for (auto sample : samples) {
        auto map = sample.toMap();
        REQUIRE(map["value"].toDouble() ==
                opentoon::evaluateTransform(editor.document().layer(layer), map["frame"].toInt()).x);
    }
    QTemporaryDir dir;
    auto path = QUrl::fromLocalFile(dir.filePath("animation.otoon"));
    REQUIRE(editor.saveProject(path));
    auto image = opentoon::SceneRenderer::render(editor.document(), 12);
    EditorController reopened;
    REQUIRE(reopened.openProject(path));
    REQUIRE(opentoon::SceneRenderer::render(reopened.document(), 12) == image);
    auto snapshot = editor.document();
    REQUIRE_FALSE(editor.updateKey(24, 0, "x", 240, 0));
    REQUIRE(editor.document() == snapshot);
    editor.undo();
    REQUIRE(editor.document().layer(layer).keys.back().frame == 20);
}

TEST_CASE("Drawing selections respect locked layers and media filters without adding empty drawings") {
    EditorController editor;
    const auto empty = editor.document();
    REQUIRE_FALSE(editor.editDrawingRegion({0, 0, 1920, 1080}, opentoon::SelectionMedia::Both,
                                           opentoon::SelectionAction::Delete));
    REQUIRE(editor.document() == empty);
    editor.loadDemo();
    const auto before = editor.document();
    REQUIRE_FALSE(editor.editDrawingRegion({0, 0, 1920, 1080}, opentoon::SelectionMedia::Raster,
                                           opentoon::SelectionAction::Delete));
    REQUIRE(editor.document() == before);
    editor.toggleLayer(editor.selectedLayer(), "locked");
    const auto locked = editor.document();
    REQUIRE_FALSE(editor.editDrawingRegion({0, 0, 1920, 1080}, opentoon::SelectionMedia::Both,
                                           opentoon::SelectionAction::Delete));
    REQUIRE(editor.document() == locked);
    editor.setBrushOpacity(.25);
    REQUIRE(editor.brushOpacity() == .25);
}

TEST_CASE("Pose-key selection edits only animation and reconciles after history or layer changes") {
    EditorController editor;
    editor.newScene();
    editor.addCurveKey(4, "x", 20);
    editor.addCurveKey(8, "x", 40);
    editor.addCurveKey(12, "x", 80);
    const auto layer = editor.selectedLayer();
    editor.clearPoseSelection();
    editor.selectPoseKey(4);
    editor.selectPoseKey(12, true);
    REQUIRE(editor.selectedPoseFrames() == QVariantList{4, 8, 12});
    editor.selectPoseKey(8, false, true);
    REQUIRE(editor.selectedPoseFrames() == QVariantList{4, 12});
    auto before = editor.document();
    REQUIRE(editor.moveSelectedPoseKeys(2));
    REQUIRE(editor.selectedPoseFrames() == QVariantList{6, 14});
    REQUIRE(editor.document().drawings == before.drawings);
    REQUIRE(editor.document().layer(layer).exposures == before.layer(layer).exposures);
    REQUIRE_FALSE(editor.moveSelectedPoseKeys(2)); // frame 8 is occupied by the unselected key.
    REQUIRE(editor.selectedPoseFrames() == QVariantList{6, 14});
    editor.undo();
    REQUIRE(editor.document() == before);
    REQUIRE(editor.selectedPoseFrames().empty());
    editor.selectPoseRange(4, 12);
    editor.copyPoseKeys();
    editor.addLayer();
    const auto target = editor.selectedLayer();
    REQUIRE(editor.selectedPoseFrames().empty());
    REQUIRE(editor.hasPoseClipboard());
    editor.setFrame(20);
    before = editor.document();
    REQUIRE(editor.pastePoseKeys());
    REQUIRE(editor.selectedPoseFrames() == QVariantList{20, 24, 28});
    REQUIRE(editor.document().drawings == before.drawings);
    REQUIRE(editor.document().layer(layer) == before.layer(layer));
    REQUIRE(editor.document().layer(target).exposures == before.layer(target).exposures);
    auto pasted = editor.document();
    REQUIRE_FALSE(editor.pastePoseKeys());
    REQUIRE(editor.document() == pasted);
    REQUIRE(editor.deleteSelectedPoseKeys());
    REQUIRE(editor.document() == before);
    editor.undo();
    REQUIRE(editor.document() == pasted);
    editor.selectPoseRange(20, 28);
    editor.toggleLayer(target, "locked");
    before = editor.document();
    REQUIRE_FALSE(editor.moveSelectedPoseKeys(1));
    REQUIRE_FALSE(editor.deleteSelectedPoseKeys());
    REQUIRE(editor.document() == before);
    editor.newScene();
    REQUIRE(editor.selectedPoseFrames().empty());
    REQUIRE(editor.hasPoseClipboard());
}

TEST_CASE("Motion-path position commands preserve easing and pixels through undo and persistence") {
    EditorController editor;
    editor.loadDemo();
    const auto id = editor.selectedLayer();
    editor.addCurveKey(12, "x", 50);
    editor.addCurveKey(24, "x", 100);
    editor.setCurveHandles(12, "x", .2, 0, .8, 1.4);
    const auto before = editor.document();
    REQUIRE(editor.setPoseKeyPosition(12, 140, 30));
    REQUIRE(editor.document().drawings == before.drawings);
    auto key = std::find_if(editor.document().layer(id).keys.begin(), editor.document().layer(id).keys.end(),
                            [](const auto& key) { return key.frame == 12; });
    REQUIRE(key->easing.at("x").y2 == 1.4);
    QTemporaryDir tmp;
    auto file = QUrl::fromLocalFile(tmp.path() + "/path.otoon");
    auto pixels = opentoon::SceneRenderer::render(editor.document(), 18);
    REQUIRE(editor.saveProject(file));
    editor.undo();
    REQUIRE(editor.document() == before);
    REQUIRE(editor.openProject(file));
    REQUIRE(opentoon::SceneRenderer::render(editor.document(), 18) == pixels);
    editor.toggleLayer(id, "locked");
    const auto locked = editor.document();
    REQUIRE_FALSE(editor.setPoseKeyPosition(12, 0, 0));
    REQUIRE(editor.document() == locked);
    REQUIRE_FALSE(editor.setPoseKeyPosition(999, 0, 0));
    REQUIRE(editor.document() == locked);
}

TEST_CASE("Layer pose menu copies full and masked transforms without changing other channels") {
    EditorController editor;
    editor.newScene();
    REQUIRE_FALSE(editor.pasteTransformPose(0));
    editor.setTransform("x", 80);
    editor.setTransform("y", 40);
    editor.setTransform("rotation", 15);
    editor.setTransform("scaleX", 1.5);
    editor.setTransform("scaleY", .8);
    editor.setTransform("opacity", .7);
    editor.setTransform("pivotX", 5);
    editor.setTransform("pivotY", 7);
    const auto copied = editor.document().layer(editor.selectedLayer()).transform;
    editor.copyTransformPose();
    REQUIRE(editor.hasCopiedTransform());
    editor.addLayer();
    const auto target = editor.selectedLayer();
    const auto baseline = editor.document();
    for (int mode = 0; mode <= 7; ++mode) {
        REQUIRE(editor.pasteTransformPose(mode));
        const auto& pose = editor.document().layer(target).transform;
        REQUIRE(pose.x == (mode == 0 || mode == 1 || mode >= 6 ? copied.x : 0));
        REQUIRE(pose.y == (mode == 0 || mode == 1 || mode >= 6 ? copied.y : 0));
        REQUIRE(pose.rotation == (mode == 0 || mode == 2 || mode >= 6 ? copied.rotation : 0));
        REQUIRE(pose.scaleX == (mode == 6 ? -copied.scaleX :
                                mode == 0 || mode == 3 || mode == 7 ? copied.scaleX : 1));
        REQUIRE(pose.scaleY == (mode == 7 ? -copied.scaleY :
                                mode == 0 || mode == 3 || mode == 6 ? copied.scaleY : 1));
        REQUIRE(pose.opacity == (mode == 0 || mode == 4 || mode >= 6 ? copied.opacity : 1));
        REQUIRE(pose.pivotX == (mode == 0 || mode == 5 || mode >= 6 ? copied.pivotX : 0));
        REQUIRE(pose.pivotY == (mode == 0 || mode == 5 || mode >= 6 ? copied.pivotY : 0));
        editor.undo();
        REQUIRE(editor.document() == baseline);
    }
    REQUIRE_FALSE(editor.pasteTransformPose(8));
    REQUIRE(editor.document() == baseline);
    REQUIRE(editor.pasteTransformPose(0));
    REQUIRE(editor.resetTransformPose());
    REQUIRE(editor.document() == baseline);
    editor.toggleLayer(target, "locked");
    const auto locked = editor.document();
    REQUIRE_FALSE(editor.pasteTransformPose(0));
    REQUIRE_FALSE(editor.resetTransformPose());
    REQUIRE(editor.document() == locked);
}

TEST_CASE("Explicit pose paste creates a later key, preserves rest and reopens identically") {
    EditorController editor;
    editor.newScene();
    editor.setTransform("x", 100);
    editor.setTransform("scaleX", -1);
    editor.copyTransformPose();
    editor.addLayer();
    const auto target = editor.selectedLayer();
    editor.setFrame(12);
    editor.setAnimateMode(true);
    editor.setAutoKey(false);
    const auto before = editor.document();
    REQUIRE(editor.pasteTransformPose(0));
    const auto pasted = editor.document();
    REQUIRE(pasted.layer(target).transform.x == 0);
    REQUIRE(pasted.layer(target).keys.size() == 2);
    REQUIRE(pasted.layer(target).keys[0].frame == 0);
    REQUIRE(pasted.layer(target).keys[0].value.x == 0);
    REQUIRE(pasted.layer(target).keys[1].frame == 12);
    REQUIRE(pasted.layer(target).keys[1].value.x == 100);
    REQUIRE(pasted.layer(target).keys[1].value.scaleX == -1);
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    auto project = QUrl::fromLocalFile(directory.filePath("copied-pose.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == pasted);
    editor.undo();
    REQUIRE(editor.document() == before);
    REQUIRE(editor.resetTransformPose());
    REQUIRE(editor.document() != before);
    REQUIRE(editor.document().layer(target).keys.back().value == before.layer(target).transform);
}

TEST_CASE("Single image import creates its layer atomically and converts tagged color to sRGB") {
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    QImage source(2, 2, QImage::Format_RGBA8888);
    source.fill(QColor(220, 45, 120, 180));
    source.setColorSpace(QColorSpace(QColorSpace::DisplayP3));
    const auto imagePath = directory.filePath("wide-gamut.png");
    REQUIRE(source.save(imagePath));
    auto expected = QImage(imagePath).convertedToColorSpace(QColorSpace(QColorSpace::SRgb))
                                    .convertToFormat(QImage::Format_RGBA8888);
    REQUIRE_FALSE(expected.isNull());
    EditorController editor;
    editor.newScene();
    editor.removeLayer();
    const auto empty = editor.document();
    REQUIRE(empty.layers.empty());
    editor.importImage(QUrl::fromLocalFile(directory.filePath("missing.png")));
    REQUIRE(editor.document() == empty);
    editor.importImage(QUrl::fromLocalFile(imagePath));
    const auto imported = editor.document();
    REQUIRE(imported.layers.size() == 1);
    REQUIRE(imported.drawings.size() == 1);
    REQUIRE(imported.palette == empty.palette);
    const auto& asset = *imported.drawings.begin()->second.image;
    REQUIRE(asset.width == 2);
    REQUIRE(asset.height == 2);
    for (int row = 0; row < 2; ++row)
        REQUIRE(std::memcmp(asset.rgba.data() + row * 8, expected.constScanLine(row), 8) == 0);
    const auto project = QUrl::fromLocalFile(directory.filePath("imported.otoon"));
    REQUIRE(editor.saveProject(project));
    EditorController reopened;
    REQUIRE(reopened.openProject(project));
    REQUIRE(reopened.document() == imported);
    editor.undo();
    REQUIRE(editor.document() == empty);
    editor.redo();
    REQUIRE(editor.document() == imported);
}
