#include "camera_smoke.h"
#include "canvas_item.h"
#include "editor_controller.h"
#include "scene_renderer.h"
#include <QCoreApplication>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QUrl>
#include <stdexcept>

using namespace opentoon;
void cameraSmoke(EditorController& editor, CanvasItem& canvas, QQuickWindow& window) {
    editor.setFrame(0);
    editor.addCamera();
    if (!editor.activeCamera() || editor.tool() != "Camera")
        throw std::runtime_error("Output camera was not created and selected.");
    const auto cameraId = Id(editor.activeCamera());
    const auto original = editor.document();
    const auto originalImage = SceneRenderer::render(original, 0, QSize(240, 135));
    auto send = [&](QEvent::Type type, QPointF local, Qt::MouseButton button, Qt::MouseButtons held,
                    Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        const auto scene = canvas.mapToScene(local);
        QMouseEvent event(type, scene, window.mapToGlobal(scene.toPoint()), button, held, modifiers);
        QCoreApplication::sendEvent(&window, &event);
    };
    auto drag = [&](QPointF from, QPointF to, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        send(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton, modifiers);
        send(QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton, modifiers);
        send(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton, modifiers);
    };
    const auto center = canvas.cameraHandlePosition(5);
    drag(center, center + QPointF(24, 12));
    const auto moved = evaluateTransform(editor.document().layer(cameraId), 0);
    if (moved.x == original.layer(cameraId).transform.x ||
        SceneRenderer::render(editor.document(), 0, QSize(240, 135)) == originalImage)
        throw std::runtime_error("Direct camera pan did not change output framing.");
    editor.undo();
    if (editor.document() != original)
        throw std::runtime_error("Camera pan did not undo atomically.");
    editor.redo();
    if (evaluateTransform(editor.document().layer(cameraId), 0) != moved)
        throw std::runtime_error("Camera pan did not redo.");
    editor.undo();
    const auto rotate = canvas.cameraHandlePosition(4);
    drag(rotate, rotate + QPointF(24, 12));
    if (evaluateTransform(editor.document().layer(cameraId), 0).rotation == 0)
        throw std::runtime_error("Camera rotation handle did not change the pose.");
    editor.undo();
    const auto corner = canvas.cameraHandlePosition(0);
    drag(corner, corner + QPointF(18, 14));
    if (editor.cameraZoom() == 1)
        throw std::runtime_error("Camera corner handle did not change zoom.");
    editor.setCameraZoom(1.25);
    if (editor.cameraZoom() != 1.25)
        throw std::runtime_error("Camera zoom property did not update both axes.");
    editor.resetCameraPose();
    if (editor.cameraZoom() != 1 ||
        evaluateTransform(editor.document().layer(cameraId), 0).x != editor.sceneWidth() / 2)
        throw std::runtime_error("Reset camera framing failed.");
    canvas.setCameraGuidesVisible(true);
    const auto pixels = SceneRenderer::render(editor.document(), 0, QSize(240, 135));
    canvas.setZoom(1.4);
    if (SceneRenderer::render(editor.document(), 0, QSize(240, 135)) != pixels ||
        window.grabWindow().isNull())
        throw std::runtime_error("Canvas navigation or guides changed output pixels.");
    canvas.fit();
    QTemporaryDir temporary;
    if (!temporary.isValid())
        throw std::runtime_error("Camera smoke could not create a temporary project.");
    const auto file = QUrl::fromLocalFile(temporary.path() + "/camera.otoon");
    const auto saved = editor.document();
    if (!editor.saveProject(file) || !editor.openProject(file) || editor.document() != saved ||
        SceneRenderer::render(editor.document(), 0, QSize(240, 135)) != pixels)
        throw std::runtime_error("Camera save/reopen changed framing or pixels.");
    editor.setSelectedLayer(int(cameraId));
    editor.toggleLayer(int(cameraId), "locked");
    const auto locked = editor.document();
    const auto lockedCenter = canvas.cameraHandlePosition(5);
    drag(lockedCenter, lockedCenter + QPointF(20, 0));
    if (editor.document() != locked)
        throw std::runtime_error("A locked camera accepted a direct gesture.");
    editor.toggleLayer(int(cameraId), "locked");
    editor.removeLayer();
    if (editor.activeCamera() != 0 || editor.tool() == "Camera")
        throw std::runtime_error("Removing the camera kept a dangling active output.");
    editor.undo();
    if (editor.activeCamera() != int(cameraId))
        throw std::runtime_error("Removing the camera did not undo.");
}
