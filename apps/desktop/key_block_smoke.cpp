#include "key_block_smoke.h"
#include "editor_controller.h"
#include "scene_renderer.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTimer>
#include <stdexcept>
void keyBlockSmoke(EditorController& editor, QQuickWindow& window) {
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
    auto send = [&](QEvent::Type type, QPointF point, Qt::MouseButton button, Qt::MouseButtons buttons,
                    Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        QMouseEvent event(type, point, window.mapToGlobal(point.toPoint()), button, buttons, modifiers);
        QCoreApplication::sendEvent(&window, &event);
    };
    auto drag = [&](QPointF from, QPointF to, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        send(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton, modifiers);
        send(QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton, modifiers);
        send(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton, modifiers);
    };
    auto click = [&](QPointF p, Qt::KeyboardModifiers modifiers = Qt::NoModifier) { drag(p, p, modifiers); };
    auto key = [&](int code, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        QKeyEvent press(QEvent::KeyPress, code, modifiers), release(QEvent::KeyRelease, code, modifiers);
        QCoreApplication::sendEvent(&window, &press);
        QCoreApplication::sendEvent(&window, &release);
    };
    editor.addCurveKey(4, "x", 30);
    editor.addCurveKey(8, "x", 60);
    editor.addCurveKey(24, "x", 100);
    editor.clearPoseSelection();
    window.setProperty("showCurves", true);
    window.setProperty("bottomHeight", 350);
    settle();
    auto* panel = window.findChild<QQuickItem*>("curveEditorPanel");
    require(panel, "Curve panel missing during key-block workflow.");
    QMetaObject::invokeMethod(panel, "selectChannel", Q_ARG(QVariant, QVariant("all")));
    settle();
    QQuickItem* strip = nullptr;
    for (auto* item : window.findChildren<QQuickItem*>("poseKeyStrip"))
        if (item->isVisible())
            strip = item;
    require(strip, "Visible pose key strip missing.");
    auto point = [&](double frame) {
        double left = strip->property("plotLeft").toDouble(), right = strip->property("plotRight").toDouble();
        return strip->mapToScene(QPointF(left + frame / (editor.duration() - 1) * (right - left),
                                         strip->property("keyY").toDouble()));
    };
    auto select = [&] {
        editor.clearPoseSelection();
        click(point(4));
        click(point(12), Qt::ShiftModifier);
    };
    const auto before = editor.document();
    drag(point(2), point(14));
    require(editor.selectedPoseFrames() == QVariantList{4, 8, 12},
            "Dragging a box in the Keys lane did not select the block.");
    require(editor.document() == before, "Key selection modified the document.");
    auto start = point(8), end = point(10);
    send(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
    require(editor.document() == before, "Group preview changed keys before release.");
    send(QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
    require(editor.selectedPoseFrames() == QVariantList{6, 10, 14},
            "Native group drag failed to move all selected poses.");
    require(editor.document().drawings == before.drawings, "Key retiming modified drawings.");
    editor.undo();
    require(editor.document() == before, "Group drag undo failed.");
    select();
    require(editor.selectedPoseFrames() == QVariantList{4, 8, 12}, "Shift-click did not select a key span.");
    click(point(8), Qt::ControlModifier);
    require(editor.selectedPoseFrames() == QVariantList{4, 12}, "Modifier-click did not toggle a key.");
    click(point(8), Qt::ControlModifier);
    start = point(12) + QPointF(10, 0);
    end = point(20) + QPointF(10, 0);
    drag(start, end);
    require(editor.selectedPoseFrames() == QVariantList{4, 12, 20},
            "Right-edge timing stretch did not preserve the middle key.");
    editor.undo();
    require(editor.document() == before, "Timing stretch undo failed.");
    select();
    drag(point(8), point(20)); // Last selected key would collide with unselected frame 24.
    require(editor.document() == before, "Group collision overwrote an existing key.");
    select();
    start = point(8);
    end = point(10);
    send(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
    key(Qt::Key_Escape);
    send(QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
    require(editor.document() == before, "Escape did not cancel the key group drag.");
    select();
    send(QEvent::MouseButtonPress, point(8), Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, point(10), Qt::NoButton, Qt::LeftButton);
    editor.selectPoseRange(0, 24);
    send(QEvent::MouseButtonRelease, point(10), Qt::LeftButton, Qt::NoButton);
    require(editor.document() == before, "Changing key selection during a drag committed a stale preview.");
    select();
    drag(point(8), point(9), Qt::AltModifier);
    require(editor.selectedPoseFrames() == QVariantList{5, 9, 13},
            "Alt-drag failed to duplicate the key block.");
    require(editor.document().layer(editor.selectedLayer()).keys.size() ==
                before.layer(editor.selectedLayer()).keys.size() + 3,
            "Alt-drag removed original pose keys.");
    editor.undo();
    require(editor.document() == before, "Duplicate undo failed.");
    select();
    key(Qt::Key_Right);
    require(editor.selectedPoseFrames() == QVariantList{5, 9, 13},
            "Arrow shortcut did not nudge the focused key selection.");
    editor.undo();
    require(editor.document() == before, "Nudge undo failed.");
    select();
    key(Qt::Key_Delete);
    require(editor.document().layer(editor.selectedLayer()).keys.size() == 2,
            "Delete shortcut did not remove only the selected pose keys.");
    require(editor.document().drawings == before.drawings, "Focused key Delete removed artwork.");
    editor.undo();
    require(editor.document() == before, "Delete block undo failed.");
    select();
    key(Qt::Key_C, Qt::ControlModifier);
    require(editor.hasPoseClipboard(), "Copy shortcut did not copy pose keys from the focused strip.");
    const auto source = editor.selectedLayer();
    editor.addLayer();
    editor.setFrame(28);
    settle();
    click(point(28));
    key(Qt::Key_V, Qt::ControlModifier);
    require(editor.selectedPoseFrames() == QVariantList{28, 32, 36},
            "Paste shortcut failed to transfer motion to another layer.");
    require(editor.document().drawings == before.drawings, "Motion paste copied drawing content.");
    require(editor.document().layer(source) == before.layer(source),
            "Motion paste changed the source layer.");
    QTemporaryDir temp;
    const auto file = QUrl::fromLocalFile(temp.path() + "/key-block.otoon");
    auto saved = editor.document();
    auto pixels = opentoon::SceneRenderer::render(saved, 6);
    require(editor.saveProject(file) && editor.openProject(file), "Key block project did not save/reopen.");
    require(editor.document() == saved && opentoon::SceneRenderer::render(editor.document(), 6) == pixels,
            "Key block round trip changed document or rendered output.");
    editor.setSelectedLayer(source);
    select();
    window.setProperty("showCurves", false);
    window.setProperty("keyEditing", true);
    settle();
    auto* timeline = window.findChild<QQuickItem*>("timelineCanvas");
    require(timeline, "Timeline missing.");
    auto timelinePoint = [&](int frame) {
        QVariant value;
        // Newly added layer is the first row; source is now row one.
        QMetaObject::invokeMethod(timeline, "keyPosition", Q_RETURN_ARG(QVariant, value),
                                  Q_ARG(QVariant, QVariant(frame)), Q_ARG(QVariant, QVariant(1)));
        return timeline->mapToScene(value.toPointF());
    };
    auto beforeTimeline = editor.document();
    drag(timelinePoint(8), timelinePoint(10));
    require(editor.selectedPoseFrames() == QVariantList{6, 10, 14},
            "Timeline failed to move the shared key selection.");
    editor.undo();
    require(editor.document() == beforeTimeline, "Timeline group move undo failed.");
    window.setProperty("showCurves", true);
    settle();
    select();
    editor.setFrame(4);
    window.resize(1080, 720);
    settle();
    require(strip->width() > 700 && panel->height() > 200, "Key tools broke the compact workspace.");
    window.resize(1440, 920);
    settle();
}
