#include "mesh_smoke.h"
#include "canvas_item.h"
#include "editor_controller.h"
#include "opentoon/deformation.h"
#include "scene_renderer.h"
#include <QCoreApplication>
#include <QImage>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QUrl>
#include <stdexcept>

using namespace opentoon;

void meshSmoke(EditorController& editor, CanvasItem& canvas, QQuickWindow& window) {
    QTemporaryDir temporary;
    if (!temporary.isValid())
        throw std::runtime_error("Mesh smoke could not create a temporary directory.");
    QImage source(128, 128, QImage::Format_RGBA8888);
    source.fill(Qt::transparent);
    for (int y = 0; y < source.height(); ++y) {
        auto* row = source.scanLine(y);
        for (int x = 0; x < source.width(); ++x) {
            row[x * 4] = x < 64 ? 255 : 0;
            row[x * 4 + 1] = 0;
            row[x * 4 + 2] = x < 64 ? 0 : 255;
            row[x * 4 + 3] = 255;
        }
    }
    const auto imagePath = temporary.path() + "/checker.png";
    if (!source.save(imagePath))
        throw std::runtime_error("Mesh checker image could not be saved.");
    editor.newScene();
    canvas.fit();
    canvas.setCameraGuidesVisible(false);
    editor.importImage(QUrl::fromLocalFile(imagePath));
    editor.makeCharacter();
    if (!editor.selectedSubstitution() || !editor.bindSelectedMesh(2, 2) ||
        !editor.selectedMeshBound())
        throw std::runtime_error("Image mesh could not be bound through the editor.");
    const Id part = editor.selectedLayer(), drawing = editor.selectedSubstitution();
    const auto original = editor.document();
    const auto originalPixels = SceneRenderer::render(original, 0);
    editor.setTool("Mesh");
    canvas.setMeshRestEditing(false);
    auto send = [&](QEvent::Type type, QPointF local, Qt::MouseButton button, Qt::MouseButtons held) {
        const auto scene = canvas.mapToScene(local);
        QMouseEvent event(type, scene, window.mapToGlobal(scene.toPoint()), button, held,
                          Qt::NoModifier);
        QCoreApplication::sendEvent(&window, &event);
    };
    auto drag = [&](QPointF from, QPointF to) {
        send(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
        send(QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton);
        if (window.grabWindow().isNull())
            throw std::runtime_error("Mesh drag preview failed.");
        send(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton);
    };
    const auto center = canvas.meshVertexPosition(4);
    drag(center, center + QPointF(14, 0));
    if (!window.grabWindow().save("build/mesh-ui-smoke.png"))
        throw std::runtime_error("Mesh UI screenshot failed.");
    const auto* posed = meshBindingFor(editor.document().layer(part), drawing);
    if (!posed || posed->vertices[4].pose == posed->vertices[4].rest ||
        SceneRenderer::render(editor.document(), 0) == originalPixels)
        throw std::runtime_error("Direct mesh pose did not change the rendered image.");
    const auto posePixels = SceneRenderer::render(editor.document(), 0);
    editor.undo();
    if (editor.document() != original || SceneRenderer::render(editor.document(), 0) != originalPixels)
        throw std::runtime_error("Direct mesh pose did not undo atomically.");
    editor.redo();
    if (SceneRenderer::render(editor.document(), 0) != posePixels)
        throw std::runtime_error("Direct mesh pose did not redo.");
    if (editor.bindSelectedMesh(4, 4) || SceneRenderer::render(editor.document(), 0) != posePixels)
        throw std::runtime_error("Unsafe mesh rebind changed authored pose.");
    if (!editor.resetSelectedMeshPose() || SceneRenderer::render(editor.document(), 0) != originalPixels)
        throw std::runtime_error("Mesh reset did not restore original pixels.");
    canvas.setMeshRestEditing(true);
    const auto restCenter = canvas.meshVertexPosition(4);
    drag(restCenter, restCenter + QPointF(14, 0));
    const auto restPixels = SceneRenderer::render(editor.document(), 0);
    if (restPixels == originalPixels)
        throw std::runtime_error("Direct rest edit did not update rendered UVs.");
    const auto saved = editor.document();
    const auto project = QUrl::fromLocalFile(temporary.path() + "/mesh.otoon");
    if (!editor.saveProject(project) || !editor.openProject(project) ||
        editor.document() != saved || SceneRenderer::render(editor.document(), 0) != restPixels)
        throw std::runtime_error("Mesh project save/reopen changed geometry or pixels.");
    editor.setSelectedLayer(int(part));
    if (!editor.removeSelectedMesh())
        throw std::runtime_error("Removing the mesh was rejected: " + editor.status().toStdString());
    if (editor.selectedMeshBound())
        throw std::runtime_error("Removing the mesh left its binding active.");
    if (SceneRenderer::render(editor.document(), 0) != originalPixels)
        throw std::runtime_error("Removing the mesh did not restore source artwork pixels.");

    editor.holdDrawing(editor.duration());
    if (!editor.bindSelectedMesh(4, 2) || !editor.bindSelectedBone() ||
        editor.selectedMeshDeformer() != 1)
        throw std::runtime_error("Bone chain could not be bound through the editor.");
    canvas.setMeshRestEditing(false);
    editor.setFrame(12);
    const auto boneRest = editor.document();
    const auto boneRestPixels = SceneRenderer::render(boneRest, 12);
    const auto tip = canvas.meshControlPosition(2);
    send(QEvent::MouseButtonPress, tip, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, tip + QPointF(-8, 16), Qt::NoButton, Qt::LeftButton);
    if (editor.document() != boneRest || window.grabWindow().isNull())
        throw std::runtime_error("Bone drag did not keep an isolated live preview.");
    send(QEvent::MouseButtonRelease, tip + QPointF(-8, 16), Qt::LeftButton, Qt::NoButton);
    const auto bonePixels = SceneRenderer::render(editor.document(), 12);
    if (bonePixels == boneRestPixels)
        throw std::runtime_error("Bone drag did not change the rendered frame: " +
                                 editor.status().toStdString());
    if (!window.grabWindow().save("build/mesh-deformer-ui-smoke.png"))
        throw std::runtime_error("Bone control UI screenshot failed.");
    editor.undo();
    if (editor.document() != boneRest)
        throw std::runtime_error("Bone drag did not undo atomically.");
    editor.redo();
    if (SceneRenderer::render(editor.document(), 12) != bonePixels)
        throw std::runtime_error("Bone redo changed the rendered frame.");
    const auto boneSaved = editor.document();
    if (!editor.saveProject(project) || !editor.openProject(project) ||
        editor.document() != boneSaved || SceneRenderer::render(editor.document(), 12) != bonePixels)
        throw std::runtime_error("Bone animation changed after project reopen.");
    editor.setSelectedLayer(int(part));
    if (!editor.removeSelectedDeformer() || !editor.bindSelectedCurve() ||
        editor.selectedMeshDeformer() != 2)
        throw std::runtime_error("Curve control could not replace the bone chain.");
    editor.setFrame(24);
    const auto curveRest = editor.document();
    const auto tangent = canvas.meshControlPosition(1);
    send(QEvent::MouseButtonPress, tangent, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, tangent + QPointF(0, -12), Qt::NoButton, Qt::LeftButton);
    canvas.cancelGesture();
    if (editor.document() != curveRest)
        throw std::runtime_error("Cancelled curve drag changed the document.");
    drag(tangent, tangent + QPointF(0, -12));
    const auto curvePixels = SceneRenderer::render(editor.document(), 24);
    if (curvePixels == SceneRenderer::render(curveRest, 24))
        throw std::runtime_error("Curve tangent drag did not deform the rendered frame.");
    const auto curveSaved = editor.document();
    if (!editor.saveProject(project) || !editor.openProject(project) ||
        editor.document() != curveSaved || SceneRenderer::render(editor.document(), 24) != curvePixels)
        throw std::runtime_error("Curve animation changed after project reopen.");
    editor.setSelectedLayer(int(part));
    editor.setFrame(24);
    if (!editor.resetSelectedDeformerPose() ||
        SceneRenderer::render(editor.document(), 24) != SceneRenderer::render(curveRest, 24))
        throw std::runtime_error("Rest key did not recover original curve pixels.");
    editor.undo();
    if (SceneRenderer::render(editor.document(), 24) != curvePixels)
        throw std::runtime_error("Rest key did not undo without deleting prior animation.");
}
