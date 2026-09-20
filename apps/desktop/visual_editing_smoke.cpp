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
    const auto beforeInsert = editor.document();
    const auto pointCount = beforeInsert.drawingAt(editor.selectedLayer(), 0)->strokes.front().points.size();
    send(QEvent::MouseButtonPress, center, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseButtonDblClick, center, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseButtonRelease, center, Qt::LeftButton, Qt::NoButton);
    require(editor.document().drawingAt(editor.selectedLayer(), 0)->strokes.front().points.size() ==
                pointCount + 1,
            "Native double-click did not insert a vector point.");
    canvas.deleteSelection();
    require(editor.document() == beforeInsert, "Deleting the inserted point changed the remaining stroke.");
    editor.undo();
    require(editor.document().drawingAt(editor.selectedLayer(), 0)->strokes.front().points.size() ==
                pointCount + 1,
            "Point deletion undo failed.");
    editor.undo();
    require(editor.document() == beforeInsert, "Point insertion undo failed.");
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
    auto settle = [&] {
        QEventLoop loop;
        QTimer::singleShot(80, &loop, &QEventLoop::quit);
        loop.exec();
        window.grabWindow();
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
    QMetaObject::invokeMethod(panel, "selectChannel", Q_ARG(QVariant, QVariant("all")));
    settle();
    require(panel->property("combined").toBool(), "All motion did not become active.");
    auto* combined = window.findChild<QQuickItem*>("combinedMotionCanvas");
    auto* overview = window.findChild<QQuickItem*>("motionOverview");
    require(combined && overview && combined->height() > 150, "Combined motion graph has no usable area.");
    require(overview->property("curveCount").toInt() >= 2, "Combined motion omitted animated X or Y.");
    auto keyPosition = [&](int frame) {
        double left = combined->property("plotLeft").toDouble(),
               right = combined->property("plotRight").toDouble();
        return combined->mapToScene(QPointF(left + frame / double(editor.duration() - 1) * (right - left),
                                            combined->property("keyY").toDouble()));
    };
    const auto beforeRetime = editor.document();
    sendGraph(QEvent::MouseButtonPress, keyPosition(12), Qt::LeftButton, Qt::LeftButton);
    sendGraph(QEvent::MouseMove, keyPosition(18), Qt::NoButton, Qt::LeftButton);
    sendGraph(QEvent::MouseButtonRelease, keyPosition(18), Qt::LeftButton, Qt::NoButton);
    require(editor.document().layer(editor.selectedLayer()).keys.back().frame == 18,
            "Combined pose diamond did not retime.");
    require(editor.document().layer(editor.selectedLayer()).keys.back().value ==
                beforeRetime.layer(editor.selectedLayer()).keys.back().value,
            "Combined retiming modified the pose values.");
    editor.undo();
    require(editor.document() == beforeRetime, "Combined retiming undo failed.");
    editor.setFrame(0);
    settle();
    auto tangentPosition = [&] {
        QVariant value;
        require(QMetaObject::invokeMethod(overview, "tangent", Q_RETURN_ARG(QVariant, value),
                                          Q_ARG(QVariant, QVariant(0))),
                "Cannot locate overview handle.");
        require(value.canConvert<QPointF>(), "Overview has no editable outgoing handle.");
        return combined->mapToScene(value.toPointF());
    };
    const auto beforeLinked = editor.document();
    auto linkedFrom = tangentPosition(), linkedTo = linkedFrom + QPointF(5, -15);
    sendGraph(QEvent::MouseButtonPress, linkedFrom, Qt::LeftButton, Qt::LeftButton);
    sendGraph(QEvent::MouseMove, linkedTo, Qt::NoButton, Qt::LeftButton);
    require(editor.document() == beforeLinked, "Linked easing preview changed the document before release.");
    sendGraph(QEvent::MouseButtonRelease, linkedTo, Qt::LeftButton, Qt::NoButton);
    const auto linked = editor.document().layer(editor.selectedLayer()).keys.front().easing;
    require(linked.size() == 8 && linked.at("x") == linked.at("y") &&
                linked.at("x") != beforeLinked.layer(editor.selectedLayer()).keys.front().easing.at("x"),
            "All motion did not ease the complete pose through a handle drag.");
    require(panel->property("combined").toBool(), "Editing a handle left All motion.");
    editor.undo();
    require(editor.document() == beforeLinked, "Linked easing undo was not atomic.");
    overview->setProperty("linkEasing", false);
    settle();
    linkedFrom = tangentPosition();
    linkedTo = linkedFrom + QPointF(5, -15);
    sendGraph(QEvent::MouseButtonPress, linkedFrom, Qt::LeftButton, Qt::LeftButton);
    sendGraph(QEvent::MouseMove, linkedTo, Qt::NoButton, Qt::LeftButton);
    sendGraph(QEvent::MouseButtonRelease, linkedTo, Qt::LeftButton, Qt::NoButton);
    auto unlinked = editor.document().layer(editor.selectedLayer()).keys.front().easing;
    require(unlinked.size() == 1 &&
                unlinked.at("x") != beforeLinked.layer(editor.selectedLayer()).keys.front().easing.at("x"),
            "Unlinked handle editing did not isolate the selected channel.");
    editor.undo();
    require(editor.document() == beforeLinked, "Unlinked easing undo failed.");
    overview->setProperty("linkEasing", true);
    settle();
    auto motionPoint = [&](int frame, double value) {
        QVariant result;
        require(QMetaObject::invokeMethod(combined, "pointFor", Q_RETURN_ARG(QVariant, result),
                                          Q_ARG(QVariant, QVariant(frame)), Q_ARG(QVariant, QVariant(value))),
                "Cannot map overview curve coordinates.");
        return combined->mapToScene(result.toPointF());
    };
    const auto beforeValue = editor.document();
    const auto oldPose = beforeValue.layer(editor.selectedLayer()).keys.back().value;
    auto valueFrom = motionPoint(12, oldPose.x), valueTo = motionPoint(16, oldPose.x + 8);
    sendGraph(QEvent::MouseButtonPress, valueFrom, Qt::LeftButton, Qt::LeftButton);
    sendGraph(QEvent::MouseMove, valueTo, Qt::NoButton, Qt::LeftButton);
    sendGraph(QEvent::MouseButtonRelease, valueTo, Qt::LeftButton, Qt::NoButton);
    const auto valueKey = editor.document().layer(editor.selectedLayer()).keys.back();
    require(valueKey.frame == 16 && std::abs(valueKey.value.x - oldPose.x - 8) < 1 &&
                valueKey.value.y == oldPose.y,
            "All motion square drag did not edit time and channel value.");
    editor.undo();
    require(editor.document() == beforeValue, "All motion square drag undo failed.");
    auto addPoint = motionPoint(18, oldPose.x * .6);
    sendGraph(QEvent::MouseButtonPress, addPoint, Qt::LeftButton, Qt::LeftButton);
    sendGraph(QEvent::MouseButtonDblClick, addPoint, Qt::LeftButton, Qt::LeftButton);
    sendGraph(QEvent::MouseButtonRelease, addPoint, Qt::LeftButton, Qt::NoButton);
    require(editor.document().layer(editor.selectedLayer()).keys.size() == 3 &&
                editor.document().layer(editor.selectedLayer()).keys.back().frame == 18,
            "All motion double-click did not add a curve key.");
    editor.undo();
    require(editor.document() == beforeValue, "Visual key insertion undo failed.");
    editor.setFrame(12);
    const auto unzoomedX = motionPoint(12, oldPose.x).x() - motionPoint(0, 0).x();
    QMetaObject::invokeMethod(panel, "zoomTime", Q_ARG(QVariant, QVariant(2)));
    settle();
    require(motionPoint(12, oldPose.x).x() - motionPoint(0, 0).x() > unzoomedX * 1.8,
            "Time zoom did not spread nearby keys.");
    panel->setProperty("timeZoom", 1);
    panel->setProperty("timeStart", 0);
    window.setProperty("showCurves", false);
    window.setProperty("keyEditing", true);
    settle();
    auto* timeline = window.findChild<QQuickItem*>("timelineCanvas");
    require(timeline, "Timeline is missing.");
    auto timelineKey = [&](int frame) {
        QVariant result;
        require(QMetaObject::invokeMethod(timeline, "keyPosition", Q_RETURN_ARG(QVariant, result),
                                          Q_ARG(QVariant, QVariant(frame)), Q_ARG(QVariant, QVariant(0))),
                "Cannot map timeline key.");
        return timeline->mapToScene(result.toPointF());
    };
    for (bool xsheet : {false, true}) {
        window.setProperty("xsheet", xsheet);
        settle();
        // Xsheet's frame 12 is below the viewport: use frame zero to test the same gesture.
        const int source = xsheet ? 0 : 12, destinationFrame = xsheet ? 2 : 18;
        const auto beforeTimeline = editor.document();
        auto start = timelineKey(source), end = timelineKey(destinationFrame);
        sendGraph(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
        sendGraph(QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
        require(editor.document() == beforeTimeline, "Timeline key preview modified the document.");
        sendGraph(QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
        const auto& timelineKeys = editor.document().layer(editor.selectedLayer()).keys;
        require(std::any_of(timelineKeys.begin(), timelineKeys.end(),
                            [&](const auto& k) { return k.frame == destinationFrame; }),
                "Timeline/Xsheet diamond drag did not move the pose.");
        editor.undo();
        require(editor.document() == beforeTimeline, "Timeline key drag undo failed.");
    }
    window.setProperty("xsheet", false);
    settle();
    auto newTimelineKey = timelineKey(20);
    sendGraph(QEvent::MouseButtonPress, newTimelineKey, Qt::LeftButton, Qt::LeftButton);
    sendGraph(QEvent::MouseButtonDblClick, newTimelineKey, Qt::LeftButton, Qt::LeftButton);
    sendGraph(QEvent::MouseButtonRelease, newTimelineKey, Qt::LeftButton, Qt::NoButton);
    require(editor.document().layer(editor.selectedLayer()).keys.size() == 3 &&
                editor.document().layer(editor.selectedLayer()).keys.back().frame == 20,
            "Timeline Keys mode double-click did not add a pose.");
    editor.undo();
    window.setProperty("showTimingTools", true);
    editor.selectTimelineRange(0, 12, 0, 0);
    settle();
    auto* clear = window.findChild<QQuickItem*>("clearTimelineButton");
    require(clear, "Clear button is missing.");
    auto clearPoint = clear->mapToScene(QPointF(clear->width() / 2, clear->height() / 2));
    auto beforeClear = editor.document();
    sendGraph(QEvent::MouseButtonPress, clearPoint, Qt::LeftButton, Qt::LeftButton);
    sendGraph(QEvent::MouseButtonRelease, clearPoint, Qt::LeftButton, Qt::NoButton);
    require(editor.document().layer(editor.selectedLayer()).keys.empty(),
            "Clear button left animation keys behind.");
    require(!editor.document().drawingAt(editor.selectedLayer(), 0),
            "Clear button left the selected exposure behind.");
    editor.undo();
    require(editor.document() == beforeClear, "Clear undo failed to restore animation and drawing exposure.");
    window.setProperty("showCurves", true);
    editor.setFrame(0);
    settle();
}
