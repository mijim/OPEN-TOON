#include "motion_path_smoke.h"
#include "canvas_item.h"
#include "opentoon/animation.h"
#include "scene_renderer.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QHoverEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTimer>
#include <iostream>
#include <stdexcept>
using namespace opentoon;
void motionPathSmoke(EditorController& editor, CanvasItem& canvas, QQuickWindow& window) {
    auto require = [](bool ok, const char* message) {
        if (!ok)
            throw std::runtime_error(message);
    };
    auto settle = [&] {
        QEventLoop loop;
        QTimer::singleShot(80, &loop, &QEventLoop::quit);
        loop.exec();
        window.grabWindow();
    };
    auto send = [&](QEvent::Type type, QPointF local, Qt::MouseButton button, Qt::MouseButtons buttons,
                    Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        const auto point = canvas.mapToScene(local);
        QMouseEvent event(type, point, window.mapToGlobal(point.toPoint()), button, buttons, modifiers);
        QCoreApplication::sendEvent(&window, &event);
    };
    auto drag = [&](QPointF from, QPointF to, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        send(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton, modifiers);
        send(QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton, modifiers);
        send(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton, modifiers);
    };
    editor.newScene();
    editor.setFrame(0);
    editor.setTool("Rectangle");
    editor.setBrushSize(3);
    editor.setFilled(true);
    editor.commitStroke({{900, 480, 1}, {960, 540, 1}});
    editor.holdDrawing(editor.duration());
    // A different drawing at the middle key must not move the captured local path reference.
    editor.setFrame(12);
    editor.newDrawing(false);
    editor.commitStroke({{980, 530, 1}, {1040, 590, 1}});
    editor.setFrame(0);
    const auto child = editor.selectedLayer();
    editor.setAnimateMode(false);
    for (const auto& [channel, value] : std::vector<std::pair<QString, double>>{
             {"pivotX", 930}, {"pivotY", 510}, {"rotation", 30}, {"scaleX", -.8}, {"scaleY", 1.2}})
        editor.setTransform(channel, value);
    editor.addLayer();
    const auto parent = editor.selectedLayer();
    editor.setAnimateMode(false);
    for (const auto& [channel, value] : std::vector<std::pair<QString, double>>{
             {"pivotX", 930}, {"pivotY", 510}, {"rotation", 15}, {"scaleX", -1.1}, {"scaleY", .7}})
        editor.setTransform(channel, value);
    editor.setAnimateMode(true);
    editor.setAutoKey(true);
    editor.setFrame(0);
    editor.setTransform("rotation", 15);
    editor.setFrame(24);
    editor.setTransform("rotation", 35);
    editor.setSelectedLayer(child);
    editor.setParent(parent);
    editor.addCurveKey(0, "x", 0);
    editor.addCurveKey(12, "x", 160);
    editor.addCurveKey(24, "x", 320);
    editor.setCurveHandles(0, "x", .2, 0, .8, 1);
    editor.setFrame(0);
    editor.setTool("Animate");
    window.setProperty("showCurves", true);
    window.setProperty("bottomHeight", 280);
    canvas.setMirrored(false);
    canvas.fit();
    settle();
    auto* button = window.findChild<QQuickItem*>("motionPathModeButton");
    require(button, "Motion path toolbar control missing.");
    const auto buttonPoint = button->mapToScene(QPointF(button->width() / 2, button->height() / 2));
    QMouseEvent press(QEvent::MouseButtonPress, buttonPoint, window.mapToGlobal(buttonPoint.toPoint()),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease, buttonPoint, window.mapToGlobal(buttonPoint.toPoint()),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &press);
    QCoreApplication::sendEvent(&window, &release);
    require(canvas.motionPathEditing(), "Path button did not enter editable motion mode.");
    settle();
    const auto before = editor.document();
    const auto start = canvas.motionPathPosition(12);
    QHoverEvent hover(QEvent::HoverMove, start + QPointF(0, 8), start, Qt::NoModifier);
    QCoreApplication::sendEvent(&canvas, &hover);
    require(canvas.cursor().shape() == Qt::OpenHandCursor,
            "Motion key hover did not use its screen-space margin.");
    const auto offset = QPointF(7, 0), delta = QPointF(32, -18);
    send(QEvent::MouseButtonPress, start + offset, Qt::LeftButton, Qt::LeftButton);
    require(editor.frame() == 12, "Clicking a motion key did not select its frame.");
    send(QEvent::MouseMove, start + offset + delta, Qt::NoButton, Qt::LeftButton);
    require(editor.document() == before, "Motion path preview modified the document.");
    require(QLineF(canvas.motionPathPosition(12), start + delta).length() < .01,
            "Path preview ignored the transformed parent or grab offset.");
    send(QEvent::MouseButtonRelease, start + offset + delta, Qt::LeftButton, Qt::NoButton);
    require(QLineF(canvas.motionPathPosition(12), start + delta).length() < .01,
            "Committed path key did not follow the cursor.");
    auto expected = before.layer(child).keys[1];
    expected.value.x = editor.document().layer(child).keys[1].value.x;
    expected.value.y = editor.document().layer(child).keys[1].value.y;
    require(editor.document().layer(child).keys[1] == expected,
            "Path drag changed a non-position pose field or easing.");
    require(editor.document().drawings == before.drawings &&
                editor.document().layer(parent) == before.layer(parent),
            "Path drag changed artwork or the parent animation.");
    editor.undo();
    require(editor.document() == before, "Path drag undo was not atomic.");
    require(QLineF(canvas.motionPathPosition(12), start).length() < .01,
            "Undo recentered the path on a different exposed drawing.");
    canvas.setMirrored(true);
    canvas.setRotationAngle(25);
    canvas.setZoom(1.2);
    settle();
    auto from = canvas.motionPathPosition(12);
    drag(from, from + QPointF(30, 10), Qt::ShiftModifier);
    require(QLineF(canvas.motionPathPosition(12), from + QPointF(30, 0)).length() < .01,
            "Shift-constrained path drag failed in a mirrored/rotated view.");
    editor.undo();
    require(editor.document() == before, "Constrained path drag undo failed.");
    from = canvas.motionPathPosition(12);
    send(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, from + QPointF(20, 20), Qt::NoButton, Qt::LeftButton);
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &escape);
    send(QEvent::MouseButtonRelease, from + QPointF(20, 20), Qt::LeftButton, Qt::NoButton);
    require(editor.document() == before, "Escape committed a motion-path preview.");
    send(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, from + QPointF(20, 20), Qt::NoButton, Qt::LeftButton);
    canvas.setZoom(1.1);
    send(QEvent::MouseButtonRelease, from + QPointF(20, 20), Qt::LeftButton, Qt::NoButton);
    require(editor.document() == before, "Changing the view committed a stale path preview.");
    canvas.setMirrored(false);
    canvas.fit();
    settle();
    const auto insertion = canvas.motionPathPosition(6);
    const auto sampled = evaluateTransform(before.layer(child), 6);
    send(QEvent::MouseButtonPress, insertion, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseButtonDblClick, insertion, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseButtonRelease, insertion, Qt::LeftButton, Qt::NoButton);
    const auto& keys = editor.document().layer(child).keys;
    auto added = std::find_if(keys.begin(), keys.end(), [](const auto& key) { return key.frame == 6; });
    require(keys.size() == 4 && added != keys.end() && added->value == sampled,
            "Path double-click did not insert the sampled full pose.");
    editor.undo();
    require(editor.document() == before, "Path key insertion undo failed.");
    editor.setFrame(12);
    editor.setTransform("scaleX", 0);
    const auto collapsed = editor.document();
    from = canvas.motionPathPosition(12);
    drag(from, from + QPointF(18, 12));
    require(QLineF(canvas.motionPathPosition(12), from + QPointF(18, 12)).length() < .01,
            "A zero-scale child could not be positioned through its nonsingular parent.");
    editor.undo();
    require(editor.document() == collapsed, "Zero-scale pose move undo failed.");
    editor.undo();
    editor.setSelectedLayer(parent);
    editor.setFrame(12);
    editor.setTransform("scaleX", 0);
    editor.setSelectedLayer(child);
    canvas.setMotionPathEditing(true);
    settle();
    const auto singular = editor.document();
    from = canvas.motionPathPosition(12);
    drag(from, from + QPointF(18, 12));
    require(editor.document() == singular, "A singular parent allowed an invalid path edit.");
    editor.undo();
    editor.setSelectedLayer(child);
    canvas.setMotionPathEditing(true);
    editor.toggleLayer(child, "locked");
    const auto locked = editor.document();
    from = canvas.motionPathPosition(12);
    drag(from, from + QPointF(18, 12));
    require(editor.document() == locked, "Path editing ignored the layer lock.");
    editor.undo();
    editor.setSelectedLayer(child);
    editor.setFrame(12);
    canvas.setMotionPathEditing(true);
    settle();
    from = canvas.motionPathPosition(12);
    drag(from, from + QPointF(18, -12));
    auto saved = editor.document();
    auto pixels = SceneRenderer::render(saved, 6);
    QTemporaryDir tmp;
    auto file = QUrl::fromLocalFile(tmp.path() + "/motion-path.otoon");
    require(editor.saveProject(file) && editor.openProject(file), "Motion path scene save/reopen failed.");
    require(editor.document() == saved && SceneRenderer::render(editor.document(), 6) == pixels,
            "Motion path persistence changed evaluated pixels.");
    editor.setSelectedLayer(child);
    editor.setFrame(12);
    editor.setTool("Animate");
    canvas.setMotionPathEditing(true);
    settle();
    std::cout << "Motion path smoke passed: parent-aware drag, Shift, mirror/rotation, cancellation, "
                 "insertion, singular/locked guards and persistence.\n";
}
