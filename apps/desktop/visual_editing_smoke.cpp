#include "visual_editing_smoke.h"
#include "canvas_item.h"
#include "scene_renderer.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTimer>
#include <cmath>
#include <stdexcept>
using namespace opentoon;
void visualEditingSmoke(EditorController& editor, CanvasItem& canvas, QQuickWindow& window) {
    auto require = [](bool condition, const char* message) {
        if (!condition)
            throw std::runtime_error(message);
    };
    auto send = [&](QEvent::Type type, QPointF local, Qt::MouseButton button, Qt::MouseButtons buttons) {
        auto point = canvas.mapToScene(local);
        QMouseEvent event(type, point, window.mapToGlobal(point.toPoint()), button, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(&window, &event);
    };
    auto drag = [&](QPointF from, QPointF to) {
        send(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
        send(QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton);
        send(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton);
    };
    auto hover = [&](QPointF p) {
        QHoverEvent event(QEvent::HoverMove, p, p, Qt::NoModifier);
        QCoreApplication::sendEvent(&canvas, &event);
    };
    editor.newScene();
    canvas.fit();
    editor.setTool("Pencil");
    editor.setBrushSize(.5);
    QPointF center(canvas.width() / 2, canvas.height() / 2);
    drag(center - QPointF(60, 0), center + QPointF(60, 0));
    editor.setTool("Select");
    hover(center + QPointF(0, 6));
    require(canvas.cursor().shape() == Qt::OpenHandCursor, "Thin-line hover did not signal selection.");
    drag(center + QPointF(0, 6), center + QPointF(0, 6));
    require(canvas.hasRegion(), "Thin line could not be selected six screen pixels away.");
    editor.holdDrawing(editor.duration());
    auto beforePoint = editor.document();
    editor.setTool("Edit points");
    auto endpoint = center - QPointF(60, 0);
    drag(endpoint, endpoint);
    hover(endpoint + QPointF(0, 9));
    require(canvas.cursor().shape() == Qt::PointingHandCursor,
            "Point hover did not match the handle margin.");
    send(QEvent::MouseButtonPress, endpoint + QPointF(0, 9), Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, endpoint - QPointF(0, 20), Qt::NoButton, Qt::LeftButton);
    require(editor.document() == beforePoint, "Point preview modified the document before release.");
    send(QEvent::MouseButtonRelease, endpoint - QPointF(0, 20), Qt::LeftButton, Qt::NoButton);
    require(editor.document().drawings != beforePoint.drawings, "Point handle drag did not edit the stroke.");
    editor.undo();
    require(editor.document() == beforePoint, "Point undo changed unrelated data.");
    const auto drawing = editor.document().drawings;
    editor.setFrame(12);
    editor.setTool("Animate");
    require(canvas.hasRegion(), "Animate did not expose layer handles.");
    auto original = editor.document();
    drag(center, center + QPointF(30, 20));
    const auto& layer = editor.document().layer(editor.selectedLayer());
    require(layer.keys.size() == 2 && layer.keys.front().frame == 0 && layer.keys.back().frame == 12,
            "Visual gesture did not create anchored keys.");
    require(layer.keys.back().value.x > 0 && layer.keys.back().value.y > 0,
            "Visual move did not update the pose.");
    require(editor.document().drawings == drawing, "Visual animation modified drawing geometry.");
    auto moved = editor.document();
    auto from = canvas.handlePosition(4);
    drag(from, from + QPointF(25, 12));
    require(editor.document().layer(editor.selectedLayer()).keys.back().value.scaleX > 1,
            "Visual scale did not update the pose.");
    require(editor.document().drawings == drawing, "Visual scale baked drawing geometry.");
    editor.undo();
    require(editor.document() == moved, "Visual scale undo was not atomic.");
    auto rotate = canvas.handlePosition(8), pivot = (canvas.handlePosition(0) + canvas.handlePosition(4)) / 2;
    auto radius = rotate - pivot;
    drag(rotate, pivot + QPointF(-radius.y(), radius.x()));
    require(std::abs(editor.document().layer(editor.selectedLayer()).keys.back().value.rotation) > 45,
            "Visual rotation did not update the pose.");
    editor.undo();
    require(editor.document() == moved, "Visual rotation undo was not atomic.");
    center = (canvas.handlePosition(0) + canvas.handlePosition(4)) / 2;
    send(QEvent::MouseButtonPress, center, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, center + QPointF(50, 0), Qt::NoButton, Qt::LeftButton);
    canvas.cancelGesture();
    send(QEvent::MouseButtonRelease, center + QPointF(50, 0), Qt::LeftButton, Qt::NoButton);
    require(editor.document() == moved, "Cancelled pose modified the document.");
    editor.undo();
    require(editor.document() == original, "Visual move undo did not restore the document.");
    editor.redo();
    QTemporaryDir temp;
    auto file = QUrl::fromLocalFile(temp.path() + "/visual.otoon");
    auto pixels = SceneRenderer::render(editor.document(), 6);
    require(editor.saveProject(file) && editor.openProject(file), "Visual animation save/reopen failed.");
    require(pixels == SceneRenderer::render(editor.document(), 6),
            "Visual animation changed after reopening.");
    // Leave an animated scene with an editable outgoing curve for the screenshot.
    require(editor.setCurveHandles(0, "x", .25, 0, .65, 1.8), "Bezier creation failed.");
    editor.setFrame(0);
    editor.setTool("Animate");
    window.setProperty("showCurves", true);
    window.setProperty("bottomHeight", 330);
    QCoreApplication::processEvents();
    auto* graph = window.findChild<QQuickItem*>("animationCurveCanvas");
    auto* panel = window.findChild<QQuickItem*>("curveEditorPanel");
    require(graph && panel, "Integrated curve panel is missing.");
    // Drag the outgoing handle using its graph/value mapping, then verify undo restores the curve.
    auto graphPoint = [&](double frame, double value) {
        double low = panel->property("low").toDouble(), high = panel->property("high").toDouble();
        double left = graph->property("plotLeft").toDouble(), right = graph->property("plotRight").toDouble();
        double top = graph->property("plotTop").toDouble(), bottom = graph->property("plotBottom").toDouble();
        return graph->mapToScene(QPointF(left + frame / (editor.duration() - 1) * (right - left),
                                         bottom - (value - low) / (high - low) * (bottom - top)));
    };
    auto sendGraph = [&](QEvent::Type type, QPointF p, Qt::MouseButton button, Qt::MouseButtons buttons) {
        QMouseEvent event(type, p, window.mapToGlobal(p.toPoint()), button, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(&window, &event);
    };
    const auto beforeHandle = editor.document();
    auto handle = graphPoint(3, 0), destination = handle + QPointF(8, -12);
    sendGraph(QEvent::MouseButtonPress, handle, Qt::LeftButton, Qt::LeftButton);
    sendGraph(QEvent::MouseMove, destination, Qt::NoButton, Qt::LeftButton);
    sendGraph(QEvent::MouseButtonRelease, destination, Qt::LeftButton, Qt::NoButton);
    require(editor.document().layer(editor.selectedLayer()).keys.front().easing.at("x") !=
                beforeHandle.layer(editor.selectedLayer()).keys.front().easing.at("x"),
            "Native Bezier handle drag failed.");
    editor.undo();
    require(editor.document() == beforeHandle, "Bezier handle undo changed other data.");
    auto settle = [] {
        QEventLoop loop;
        QTimer::singleShot(80, &loop, &QEventLoop::quit);
        loop.exec();
    };
    settle();
    require(graph->height() >= panel->height() - 65, "Curve toolbar consumes too much graph height.");
    auto* splitter = window.findChild<QQuickItem*>("workspaceSplitter");
    require(splitter, "Workspace splitter is missing.");
    double beforeHeight = graph->height();
    auto grip = splitter->mapToScene(QPointF(splitter->width() / 2, splitter->height() / 2));
    sendGraph(QEvent::MouseButtonPress, grip, Qt::LeftButton, Qt::LeftButton);
    sendGraph(QEvent::MouseMove, grip - QPointF(0, 150), Qt::NoButton, Qt::LeftButton);
    sendGraph(QEvent::MouseButtonRelease, grip - QPointF(0, 150), Qt::LeftButton, Qt::NoButton);
    settle();
    require(graph->height() > beforeHeight + 100,
            "Dragging the workspace separator did not enlarge the curve graph.");
    require(canvas.height() >= 159, "Workspace resizing collapsed the canvas below its minimum.");
    beforeHeight = graph->height();
    grip = splitter->mapToScene(QPointF(splitter->width() / 2, splitter->height() / 2));
    sendGraph(QEvent::MouseButtonPress, grip, Qt::LeftButton, Qt::LeftButton);
    sendGraph(QEvent::MouseMove, grip + QPointF(0, 90), Qt::NoButton, Qt::LeftButton);
    sendGraph(QEvent::MouseButtonRelease, grip + QPointF(0, 90), Qt::LeftButton, Qt::NoButton);
    settle();
    require(graph->height() < beforeHeight - 60,
            "Dragging the workspace separator down did not shrink the panel.");
    window.resize(1080, 720);
    settle();
    require(panel->height() <= window.height() - 250 && graph->height() > 100,
            "Compact layout failed at minimum window size.");
    panel->setProperty("showNumbers", true);
    settle();
    require(graph->height() > 90, "Optional numeric row collapsed the graph.");
    panel->setProperty("showNumbers", false);
    window.resize(1440, 920);
    window.setProperty("bottomHeight", 280);
    settle();
}
