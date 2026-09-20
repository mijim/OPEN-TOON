#include "editor_controller.h"
#include "scene_renderer.h"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
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
int main(int argc, char** argv) {
    QGuiApplication application(argc, argv);
    QCoreApplication::setApplicationName("OPEN-TOON-export-tests");
    QStandardPaths::setTestModeEnabled(true);
    return Catch::Session().run(argc, argv);
}
