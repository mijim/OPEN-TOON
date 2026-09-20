#include "vector_selection_smoke.h"
#include "canvas_item.h"
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
void vectorSelectionSmoke(EditorController& editor, CanvasItem& canvas, QQuickWindow& window) {
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
    auto count = [&] { return canvas.objectProperties().value("count").toInt(); };
    editor.newScene();
    window.setWidth(1440);
    window.setHeight(940);
    window.setProperty("bottomHeight", 240);
    window.setProperty("showCurves", false);
    canvas.setMirrored(false);
    canvas.fit();
    editor.setTool("Rectangle");
    editor.setFilled(true);
    editor.setBrushSize(4);
    for (double x : {550., 860., 1170.})
        editor.commitStroke({{x, 410, 1}, {x + 200, 650, 1}});
    editor.setTool("Select");
    settle();
    auto screen = [&](double x, double y) {
        const double scale = std::min((canvas.width() - 64) / editor.sceneWidth(),
                                      (canvas.height() - 64) / editor.sceneHeight()) *
                             canvas.zoom();
        QTransform view;
        view.translate(canvas.width() / 2, canvas.height() / 2);
        view.rotate(canvas.rotationAngle());
        view.scale(canvas.mirrored() ? -scale : scale, scale);
        view.translate(-editor.sceneWidth() / 2., -editor.sceneHeight() / 2.);
        return view.map(QPointF(x, y));
    };
    auto click = [&](double x, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        drag(screen(x, 530), screen(x, 530), modifiers);
    };
    auto selectOuter = [&] {
        canvas.clearRegion();
        click(650);
        click(1270, Qt::ShiftModifier);
        require(count() == 2, "Shift-click did not select two separate vectors.");
    };
    const auto before = editor.document();
    const auto layer = editor.selectedLayer();
    selectOuter();
    click(1270, Qt::ShiftModifier);
    require(count() == 2 && editor.document() == before,
            "Adding twice duplicated selection or edited artwork.");
    require(canvas.regionInfo().contains("2 vector strokes"),
            "Selection summary counted an unselected bystander.");
    click(650, Qt::AltModifier);
    require(count() == 1 && canvas.objectProperties().value("kind") == "vector",
            "Alt-click did not subtract a vector or restore single-object Properties.");
    selectOuter();
    click(960);
    require(count() == 1, "Clicking an unselected object inside a group box moved the group instead.");
    selectOuter();
    editor.setTool("Marquee");
    require(count() == 2 && canvas.selectionMedia() == int(SelectionMedia::Vectors),
            "Changing selection tools lost membership or selected raster pixels.");
    drag(screen(830, 380), screen(1090, 680), Qt::ShiftModifier);
    require(count() == 3, "Shift-marquee did not add enclosed vectors.");
    drag(screen(830, 380), screen(1090, 680), Qt::AltModifier);
    require(count() == 2, "Alt-marquee did not subtract enclosed vectors.");
    send(QEvent::MouseButtonPress, screen(500, 380), Qt::LeftButton, Qt::LeftButton, Qt::AltModifier);
    send(QEvent::MouseMove, screen(1400, 680), Qt::NoButton, Qt::LeftButton, Qt::AltModifier);
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &escape);
    send(QEvent::MouseButtonRelease, screen(1400, 680), Qt::LeftButton, Qt::NoButton);
    require(count() == 2 && editor.document() == before, "Cancelled marquee changed membership or history.");
    editor.setTool("Select");
    canvas.setMirrored(true);
    canvas.setRotationAngle(15);
    settle();
    const auto from = screen(650, 530), to = from + QPointF(25, 20);
    send(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton);
    require(editor.document() == before, "Sparse vector preview changed the document before release.");
    send(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton);
    const auto& moved = editor.document().drawingAt(layer, 0)->strokes;
    const auto& original = before.drawingAt(layer, 0)->strokes;
    require(moved[0] != original[0] && moved[2] != original[2] && moved[1] == original[1],
            "Group drag moved an unselected vector or lost sparse membership.");
    require(count() == 2, "Committed group move lost selection.");
    editor.undo();
    require(editor.document() == before, "Sparse group move did not undo in one step.");
    canvas.setMirrored(false);
    canvas.fit();
    selectOuter();
    auto handle = canvas.handlePosition(4);
    send(QEvent::MouseButtonPress, handle, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, handle + QPointF(25, 15), Qt::NoButton, Qt::LeftButton, Qt::ShiftModifier);
    QCoreApplication::sendEvent(&window, &escape);
    send(QEvent::MouseButtonRelease, handle + QPointF(25, 15), Qt::LeftButton, Qt::NoButton);
    require(editor.document() == before && count() == 2, "Cancelled group scale lost artwork or selection.");
    drag(handle, handle + QPointF(25, 15));
    require(editor.document().drawingAt(layer, 0)->strokes[1] == original[1],
            "Group scale captured the enclosed bystander.");
    editor.undo();
    require(editor.document() == before, "Group scale undo failed.");
    selectOuter();
    canvas.transformRegion(int(SelectionAction::Duplicate), 0, 0);
    require(count() == 2 && editor.document().drawingAt(layer, 0)->strokes.size() == 5,
            "Duplicate did not retain exactly the new vector identities.");
    canvas.transformRegion(int(SelectionAction::Move), 24, 24);
    const auto& duplicated = editor.document().drawingAt(layer, 0)->strokes;
    require(duplicated[0] == original[0] && duplicated[1] == original[1] && duplicated[2] == original[2],
            "Moving overlapping duplicates moved an original or bystander.");
    canvas.deleteSelection();
    require(editor.document().drawingAt(layer, 0)->strokes == original,
            "Delete captured vectors outside the explicit duplicate selection.");
    editor.undo();
    editor.undo();
    editor.undo();
    require(editor.document() == before, "Duplicate/move/delete history did not restore exact artwork.");
    selectOuter();
    canvas.transformRegion(int(SelectionAction::Move), 0, -60);
    const auto saved = editor.document();
    const auto pixels = SceneRenderer::render(saved, 0);
    settle();
    require(window.grabWindow().save("build/vector-selection-smoke.png"),
            "Vector selection screenshot failed.");
    QTemporaryDir tmp;
    const auto file = QUrl::fromLocalFile(tmp.path() + "/vector-selection.otoon");
    require(editor.saveProject(file) && editor.openProject(file), "Vector selection save/reopen failed.");
    require(editor.document() == saved && SceneRenderer::render(editor.document(), 0) == pixels,
            "Vector selection persistence changed artwork or rendered pixels.");
    std::cout << "Vector selection smoke passed: additive/subtractive picking and marquee, tool switch, "
                 "sparse transforms, cancellation, duplicate identity, undo and persistence.\n";
}
