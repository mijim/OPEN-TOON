#include "editor_controller.h"
#include "project_store.h"
#include "scene_renderer.h"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
namespace {
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
