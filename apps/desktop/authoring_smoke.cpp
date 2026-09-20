#include "authoring_smoke.h"
#include "canvas_item.h"
#include "scene_renderer.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace opentoon;
void authoringSmoke(EditorController& editor, CanvasItem& canvas, QQuickWindow& window) {
    auto require = [](bool ok, const char* message) {
        if (!ok)
            throw std::runtime_error(message);
    };
    auto settle = [&] {
        QEventLoop loop;
        QTimer::singleShot(100, &loop, &QEventLoop::quit);
        loop.exec();
        window.grabWindow();
    };
    auto send = [&](QEvent::Type type, QPointF local, Qt::MouseButton button, Qt::MouseButtons buttons,
                    Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        const auto point = canvas.mapToScene(local);
        QMouseEvent event(type, point, window.mapToGlobal(point.toPoint()), button, buttons, modifiers);
        QCoreApplication::sendEvent(&window, &event);
    };
    auto drag = [&](QPointF a, QPointF b, Qt::KeyboardModifiers mods = Qt::NoModifier) {
        send(QEvent::MouseButtonPress, a, Qt::LeftButton, Qt::LeftButton, mods);
        send(QEvent::MouseMove, b, Qt::NoButton, Qt::LeftButton, mods);
        send(QEvent::MouseButtonRelease, b, Qt::LeftButton, Qt::NoButton, mods);
    };
    auto key = [&](int code, Qt::KeyboardModifiers mods = Qt::NoModifier) {
        window.requestActivate();
        settle();
        canvas.forceActiveFocus();
        QKeyEvent press(QEvent::KeyPress, code, mods), release(QEvent::KeyRelease, code, mods);
        QCoreApplication::sendEvent(&window, &press);
        QCoreApplication::sendEvent(&window, &release);
    };
    editor.newScene();
    canvas.fit();
    canvas.setMirrored(false);
    canvas.setSnapToGrid(false);
    window.setProperty("showCurves", false);
    window.setProperty("bottomHeight", 240);
    settle();
    auto screen = [&](double x, double y) {
        const double scale = std::min((canvas.width() - 64) / editor.sceneWidth(),
                                      (canvas.height() - 64) / editor.sceneHeight());
        return QPointF(canvas.width() / 2 + (x - editor.sceneWidth() / 2.) * scale,
                       canvas.height() / 2 + (y - editor.sceneHeight() / 2.) * scale);
    };
    auto count = [&] { return canvas.objectProperties().value("count").toInt(); };
    auto strokes = [&] {
        return editor.document().drawingAt(editor.selectedLayer(), editor.frame())->strokes;
    };
    editor.setTool("Line");
    drag(screen(550, 400), screen(800, 440), Qt::ShiftModifier);
    auto line = strokes().back();
    require(line.points.size() == 2 && std::abs(line.points[0].y - line.points[1].y) < 1e-6,
            "Shift-line did not constrain the segment to 45-degree increments.");
    editor.setTool("Ellipse");
    editor.setFilled(false);
    drag(screen(1100, 550), screen(1250, 630), Qt::ShiftModifier);
    auto circle = strokes().back();
    require(std::abs((circle.points.back().x - circle.points.front().x) -
                     (circle.points.back().y - circle.points.front().y)) < 1e-6,
            "Shift-ellipse did not create a circle.");
    canvas.setGridVisible(true);
    canvas.setGridSpacing(40);
    canvas.setSnapToGrid(true);
    editor.setTool("Rectangle");
    drag(screen(901, 301), screen(1089, 489));
    auto rect = strokes().back();
    require(rect.points.front().x == 920 && rect.points.front().y == 320 && rect.points.back().x == 1080 &&
                rect.points.back().y == 480,
            "Primitive endpoints did not snap to the drawing-local grid.");
    const auto drawing = editor.document();
    const auto pixels = SceneRenderer::render(drawing, 0);
    canvas.setGridVisible(false);
    canvas.setSnapToGrid(false);
    require(editor.document() == drawing && SceneRenderer::render(editor.document(), 0) == pixels,
            "Guides leaked into the document or render output.");
    editor.setTool("Lasso");
    const std::vector<QPointF> loop{screen(530, 375), screen(835, 375), screen(835, 425), screen(530, 425),
                                    screen(530, 375)};
    auto lasso = [&](Qt::KeyboardModifiers mods) {
        send(QEvent::MouseButtonPress, loop.front(), Qt::LeftButton, Qt::LeftButton, mods);
        for (std::size_t i = 1; i < loop.size(); ++i)
            send(QEvent::MouseMove, loop[i], Qt::NoButton, Qt::LeftButton, mods);
        send(QEvent::MouseButtonRelease, loop.back(), Qt::LeftButton, Qt::NoButton, mods);
    };
    lasso(Qt::NoModifier);
    require(count() == 1, "Native lasso did not enclose exactly the line.");
    canvas.selectAllVectors(true);
    require(count() == 2, "Invert selection did not exclude the selected line.");
    lasso(Qt::ShiftModifier);
    require(count() == 3, "Shift-lasso did not add the line to the group.");
    lasso(Qt::AltModifier);
    require(count() == 2, "Alt-lasso did not remove the line from the group.");
    const auto beforeStyle = editor.document();
    require(canvas.editVectorSelection("strokeWidth", 9), "Batch width edit failed.");
    require(strokes()[0] == line && strokes()[1].width == 9 && strokes()[2].width == 9,
            "Batch style touched the excluded line or missed selected shapes.");
    require(canvas.editVectorSelection("arrange", 3), "Top alignment failed.");
    require(strokeBounds(strokes()[1])->y == strokeBounds(strokes()[2])->y, "Top edges were not aligned.");
    canvas.selectAllVectors();
    require(canvas.editVectorSelection("arrange", 6), "Horizontal distribution failed.");
    const auto beforeNudge = editor.document();
    key(Qt::Key_Right, Qt::ShiftModifier);
    require(strokes()[0].points[0].x ==
                beforeNudge.drawingAt(editor.selectedLayer(), 0)->strokes[0].points[0].x + 10,
            "Shift-arrow did not nudge the focused vector selection.");
    editor.undo();
    require(editor.document() == beforeNudge, "Vector nudge undo failed.");
    editor.setTool("Select");
    canvas.selectAllVectors();
    key(Qt::Key_C, Qt::ControlModifier);
    require(canvas.hasVectorClipboard(), "Canvas copy shortcut did not fill the vector clipboard.");
    const auto beforeCut = editor.document();
    key(Qt::Key_X, Qt::ControlModifier);
    require(strokes().empty(), "Canvas cut shortcut failed.");
    editor.undo();
    require(editor.document() == beforeCut, "Cut undo failed.");
    editor.addLayer();
    editor.setFrame(3);
    canvas.forceActiveFocus();
    key(Qt::Key_V, Qt::ControlModifier);
    require(count() == 3 && strokes().size() == 3,
            "Paste did not create and select independent vectors on an empty frame.");
    const auto beforeLock = editor.document();
    editor.toggleLayer(editor.selectedLayer(), "locked");
    const auto locked = editor.document();
    require(!canvas.pasteVectorSelection() && editor.document() == locked,
            "Locked layer accepted a clipboard paste.");
    editor.undo();
    require(editor.document() == beforeLock, "Unlock undo failed.");
    editor.setTool("Pencil");
    std::vector<Point> samples;
    for (int i = 0; i <= 80; ++i)
        samples.push_back({550. + i * 5, 700. + .15 * std::sin(i), .5});
    editor.commitStroke(samples);
    editor.setTool("Select");
    canvas.selectAllVectors();
    const auto dirty = editor.document();
    require(canvas.editVectorSelection("smooth", .5), "Batch pencil smoothing failed.");
    require(canvas.editVectorSelection("simplify", .5), "Batch pencil simplification failed.");
    require(strokes().back().points.size() < 10, "Simplify did not reduce the sampled stroke.");
    editor.undo();
    editor.undo();
    require(editor.document() == dirty, "Cleanup undo did not recover exact samples.");
    canvas.selectAllVectors();
    canvas.editVectorSelection("simplify", .5);
    const auto id = editor.selectedLayer();
    editor.addCurveKey(0, "x", 0);
    editor.addCurveKey(6, "x", 100);
    editor.addCurveKey(12, "x", 0);
    editor.selectPoseRange(0, 12);
    const auto beforeKeys = editor.document();
    require(editor.interpolateSelectedPoseKeys(3), "Batch pose easing failed.");
    require(editor.document().layer(id).keys[0].easing.size() == 8, "Batch easing missed pose channels.");
    editor.undo();
    require(editor.document() == beforeKeys, "Batch easing undo failed.");
    editor.selectPoseRange(0, 12);
    require(editor.repeatSelectedPoseKeys(3), "Pose-key repeat failed.");
    require(editor.document().layer(id).keys.size() == 12 && editor.duration() >= 52,
            "Repeat did not extend duration or copy all keys.");
    editor.selectPoseRange(0, 12);
    const auto repeated = editor.document();
    require(!editor.repeatSelectedPoseKeys(1) && editor.document() == repeated,
            "Repeat overwrote occupied keys.");
    QTemporaryDir tmp;
    auto file = QUrl::fromLocalFile(tmp.path() + "/authoring.otoon");
    auto rendered = SceneRenderer::render(editor.document(), 20);
    require(editor.saveProject(file) && editor.openProject(file), "Authoring scene save/reopen failed.");
    require(editor.document() == repeated && SceneRenderer::render(editor.document(), 20) == rendered,
            "Authoring workflow changed after persistence.");
    editor.setSelectedLayer(id);
    editor.setFrame(3);
    editor.setTool("Select");
    canvas.selectAllVectors();
    settle();
    require(window.grabWindow().save("build/authoring-smoke.png"), "Authoring screenshot failed.");
    std::cout
        << "Authoring smoke passed: constrained primitives, grid, lasso modifiers, invert, styles, layout, "
           "shortcuts, cross-layer clipboard, cleanup, key interpolation/repetition and persistence.\n";
}
