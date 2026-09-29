#include "mesh_smoke.h"
#include "canvas_item.h"
#include "editor_controller.h"
#include "opentoon/deformer.h"
#include "opentoon/deformation.h"
#include "opentoon/timeline.h"
#include "project_store.h"
#include "scene_renderer.h"
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <sys/resource.h>
#include <vector>

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
    const auto beforeContour = editor.document();
    if (!editor.bindSelectedContourMesh(6, 16) || editor.selectedMeshColumns() != 6 ||
        editor.selectedMeshRows() != 16)
        throw std::runtime_error("Contour mesh could not be bound through the editor.");
    const auto contourBound = editor.document();
    editor.undo();
    if (editor.document() != beforeContour)
        throw std::runtime_error("Contour mesh binding did not undo atomically.");
    editor.redo();
    if (editor.document() != contourBound)
        throw std::runtime_error("Contour mesh binding did not redo identically.");
    if (editor.selectedMeshColumns() != 6 ||
        editor.selectedMeshRows() != 16 || !editor.bindSelectedBone() ||
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
    const auto beforeRadius = editor.document();
    const double oldRadius = editor.selectedBoneTransition();
    if (editor.selectedBoneMaxTransition() <= oldRadius ||
        !editor.setSelectedBoneTransition(oldRadius * 1.5) ||
        editor.selectedBoneTransition() <= oldRadius)
        throw std::runtime_error("Elbow influence could not be edited through the editor: " +
                                 editor.status().toStdString());
    const auto radiusPixels = SceneRenderer::render(editor.document(), 12);
    if (radiusPixels == bonePixels ||
        SceneRenderer::render(editor.document(), 0) !=
            SceneRenderer::render(beforeRadius, 0))
        throw std::runtime_error("Elbow influence did not change the bend safely.");
    editor.undo();
    if (editor.document() != beforeRadius)
        throw std::runtime_error("Elbow influence did not undo atomically.");
    editor.redo();
    if (SceneRenderer::render(editor.document(), 12) != radiusPixels)
        throw std::runtime_error("Elbow influence did not redo identically.");
    editor.undo();
    canvas.setMeshRestEditing(true);
    if (!window.grabWindow().save("build/hm06-influence-handle.png"))
        throw std::runtime_error("Influence-handle UI screenshot failed.");
    const auto radiusHandle = canvas.meshInfluenceHandlePosition();
    const auto elbowCenter = canvas.meshControlPosition(1);
    const auto radiusTarget = elbowCenter + (radiusHandle - elbowCenter) * 1.25;
    const auto beforeRadiusDrag = editor.document();
    send(QEvent::MouseButtonPress, radiusHandle, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, radiusTarget, Qt::NoButton, Qt::LeftButton);
    if (editor.document() != beforeRadiusDrag || window.grabWindow().isNull())
        throw std::runtime_error("Influence handle did not keep an isolated live preview.");
    canvas.cancelGesture();
    if (editor.document() != beforeRadiusDrag)
        throw std::runtime_error("Cancelled influence-handle drag changed the document.");
    drag(radiusHandle, radiusTarget);
    const auto handledPixels = SceneRenderer::render(editor.document(), 12);
    if (editor.selectedBoneTransition() <= oldRadius || handledPixels == bonePixels ||
        SceneRenderer::render(editor.document(), 0) !=
            SceneRenderer::render(beforeRadiusDrag, 0))
        throw std::runtime_error("Influence handle did not retarget the keyed bend safely: " +
                                 editor.status().toStdString());
    editor.undo();
    if (editor.document() != beforeRadiusDrag)
        throw std::runtime_error("Influence handle did not undo atomically.");
    editor.redo();
    if (SceneRenderer::render(editor.document(), 12) != handledPixels)
        throw std::runtime_error("Influence-handle redo changed the rendered frame.");
    editor.undo();
    const auto restElbow = canvas.meshControlPosition(1);
    const auto beforeRestEdit = editor.document();
    send(QEvent::MouseButtonPress, restElbow, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, restElbow + QPointF(0, 2), Qt::NoButton, Qt::LeftButton);
    if (editor.document() != beforeRestEdit || window.grabWindow().isNull())
        throw std::runtime_error("Bone rest-joint preview changed the authored document.");
    canvas.cancelGesture();
    if (editor.document() != beforeRestEdit)
        throw std::runtime_error("Cancelled rest-joint drag changed the document.");
    drag(restElbow, restElbow + QPointF(0, 2));
    const auto boneRetargetedPixels = SceneRenderer::render(editor.document(), 12);
    if (boneRetargetedPixels == bonePixels ||
        SceneRenderer::render(editor.document(), 0) !=
            SceneRenderer::render(beforeRestEdit, 0))
        throw std::runtime_error("Rest joint drag did not retarget the keyed bone safely: " +
                                 editor.status().toStdString());
    editor.undo();
    if (editor.document() != beforeRestEdit)
        throw std::runtime_error("Rest joint drag did not undo atomically.");
    editor.redo();
    if (SceneRenderer::render(editor.document(), 12) != boneRetargetedPixels)
        throw std::runtime_error("Rest joint redo changed the rendered frame.");
    canvas.setMeshRestEditing(false);
    const auto boneSaved = editor.document();
    if (!editor.saveProject(project) || !editor.openProject(project) ||
        editor.document() != boneSaved ||
        SceneRenderer::render(editor.document(), 12) != boneRetargetedPixels)
        throw std::runtime_error("Bone animation changed after project reopen.");
    const Id character = editor.characterId();
    editor.addLayer();
    editor.importImage(QUrl::fromLocalFile(imagePath));
    const Id follower = editor.selectedLayer();
    editor.setParent(int(character));
    editor.setParent(int(part));
    if (!editor.selectedCanFollowBoneTip() ||
        !editor.toggleSelectedBoneTipAttachment() || !editor.selectedFollowsBoneTip())
        throw std::runtime_error("The editor could not attach a child Part to its parent bone tip: " +
                                 editor.status().toStdString());
    const auto attached = editor.document();
    editor.undo();
    if (editor.selectedFollowsBoneTip())
        throw std::runtime_error("Bone tip attachment did not undo.");
    editor.redo();
    if (editor.document() != attached || !editor.selectedFollowsBoneTip())
        throw std::runtime_error("Bone tip attachment did not redo.");
    if (!editor.saveProject(project) || !editor.openProject(project) ||
        editor.document() != attached || !editor.selectedFollowsBoneTip())
        throw std::runtime_error("Bone tip attachment changed after project reopen.");
    editor.setSelectedLayer(int(follower));
    editor.deleteRigBranch();
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
    canvas.setMeshRestEditing(true);
    const auto curveRestControl = canvas.meshControlPosition(2);
    const auto beforeCurveRetarget = editor.document();
    send(QEvent::MouseButtonPress, curveRestControl, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, curveRestControl + QPointF(0, 2), Qt::NoButton, Qt::LeftButton);
    if (editor.document() != beforeCurveRetarget || window.grabWindow().isNull())
        throw std::runtime_error("Curve rest-control preview changed the authored document.");
    canvas.cancelGesture();
    if (editor.document() != beforeCurveRetarget)
        throw std::runtime_error("Cancelled curve rest-control drag changed the document.");
    drag(curveRestControl, curveRestControl + QPointF(0, 2));
    if (SceneRenderer::render(editor.document(), 24) == curvePixels ||
        SceneRenderer::render(editor.document(), 0) !=
            SceneRenderer::render(beforeCurveRetarget, 0))
        throw std::runtime_error("Curve rest-control drag did not retarget the keyed pose safely: " +
                                 editor.status().toStdString());
    editor.undo();
    if (editor.document() != beforeCurveRetarget)
        throw std::runtime_error("Curve rest-control drag did not undo atomically.");
    canvas.setMeshRestEditing(false);
    if (!editor.resetSelectedDeformerPose() ||
        SceneRenderer::render(editor.document(), 24) != SceneRenderer::render(curveRest, 24))
        throw std::runtime_error("Rest key did not recover original curve pixels.");
    editor.undo();
    if (SceneRenderer::render(editor.document(), 24) != curvePixels)
        throw std::runtime_error("Rest key did not undo without deleting prior animation.");
    canvas.setZoom(0.75);
    canvas.setMirrored(true);
    canvas.setRotationAngle(15);
    const auto transformedTangent = canvas.meshControlPosition(1);
    drag(transformedTangent, transformedTangent + QPointF(0, -8));
    if (SceneRenderer::render(editor.document(), 24) == curvePixels)
        throw std::runtime_error("Mirrored and rotated curve drag missed its control.");
    editor.undo();
    if (SceneRenderer::render(editor.document(), 24) != curvePixels)
        throw std::runtime_error("Transformed curve drag did not undo.");
    canvas.setMirrored(false);
    canvas.setRotationAngle(0);
    canvas.fit();
    int partRow = -1;
    for (std::size_t index = 0; index < editor.document().layers.size(); ++index)
        if (editor.document().layers[index].id == part)
            partRow = int(editor.document().layers.size() - 1 - index);
    if (partRow < 0)
        throw std::runtime_error("Timeline lost the animated Part.");
    editor.selectTimelineRange(24, 24, partRow, partRow);
    editor.copyTimelineRange();
    const auto beforePaste = editor.document();
    editor.setFrame(30);
    editor.pasteTimelineRange(int(PasteContent::Keys));
    const auto* pasted = meshBindingFor(editor.document().layer(part), drawing);
    if (!pasted || !pasted->curve || pasted->curve->keys.back().frame != 30)
        throw std::runtime_error("Timeline key paste lost the curve pose: " +
                                 editor.status().toStdString());
    editor.undo();
    if (editor.document() != beforePaste)
        throw std::runtime_error("Timeline deformer paste did not undo atomically.");
    editor.moveTimelineRange(30, false);
    const auto* moved = meshBindingFor(editor.document().layer(part), drawing);
    if (!moved || !moved->curve || moved->curve->keys.back().frame != 30)
        throw std::runtime_error("Timeline range move lost the curve pose: " +
                                 editor.status().toStdString());
    editor.undo();
    if (editor.document() != beforePaste)
        throw std::runtime_error("Timeline deformer move did not undo atomically.");
    const auto example = QDir::current().filePath("examples/clockwork-continuous.otoon");
    if (!editor.openProject(QUrl::fromLocalFile(example)))
        throw std::runtime_error("Native editor could not open the continuous rig example: " +
                                 editor.status().toStdString());
    Id linkedHand = 0;
    int links = 0;
    for (const auto& layer : editor.document().layers) {
        links += layer.boneTipAnchor ? 1 : 0;
        if (layer.role == "hand_left")
            linkedHand = layer.id;
    }
    if (links != 4 || !linkedHand || editor.document().layers.size() != 16)
        throw std::runtime_error("Continuous rig example lost its character hierarchy.");
    editor.setSelectedLayer(int(linkedHand));
    if (!editor.selectedFollowsBoneTip())
        throw std::runtime_error("Native inspector lost the example hand attachment.");
    editor.setFrame(36);
    const auto sideHand = editor.selectedSubstitution();
    editor.setFrame(44);
    if (!sideHand || sideHand == editor.selectedSubstitution() ||
        SceneRenderer::render(editor.document(), 36) ==
            SceneRenderer::render(editor.document(), 44))
        throw std::runtime_error("Continuous rig example did not switch view and hand artwork.");
    canvas.fit();
    QCoreApplication::processEvents();
    if (window.grabWindow().isNull())
        throw std::runtime_error("Native canvas did not display the continuous rig example.");
    const auto visual = QDir::current().filePath("examples/clockwork-visual-shot.otoon");
    if (!editor.openProject(QUrl::fromLocalFile(visual)))
        throw std::runtime_error("Native editor could not open the visual shot: " +
                                 editor.status().toStdString());
    if (editor.document().duration != 480 || !editor.document().activeCamera)
        throw std::runtime_error("Visual shot lost its timing or output camera.");
    Id visualArm = 0, visualHand = 0, visualTorso = 0, visualLeg = 0;
    for (const auto& layer : editor.document().layers) {
        if (layer.role == "arm_left") visualArm = layer.id;
        if (layer.role == "hand_left") visualHand = layer.id;
        if (layer.role == "torso") visualTorso = layer.id;
        if (layer.role == "leg_left") visualLeg = layer.id;
    }
    if (!visualArm || !visualHand || !visualTorso || !visualLeg)
        throw std::runtime_error("Visual shot lost its character roles or torso curve.");
    const auto* torsoMesh = meshBindingFor(editor.document().layer(visualTorso),
                                           editor.document().drawingAt(visualTorso, 0)->id);
    if (!torsoMesh || !torsoMesh->curve)
        throw std::runtime_error("Visual shot lost its torso curve.");
    editor.setSelectedLayer(int(visualHand));
    if (!editor.selectedFollowsBoneTip())
        throw std::runtime_error("Visual shot lost its linked hand.");
    editor.setFrame(0);
    const Id openDrawing = editor.selectedSubstitution();
    editor.setFrame(240);
    const Id pointDrawing = editor.selectedSubstitution();
    editor.setFrame(336);
    const Id fistDrawing = editor.selectedSubstitution();
    if (!openDrawing || !pointDrawing || !fistDrawing ||
        openDrawing == pointDrawing || pointDrawing == fistDrawing)
        throw std::runtime_error("Visual shot did not show three hand substitutions.");
    const auto visualDocument = editor.document();
    const auto visualFrame = SceneRenderer::render(visualDocument, 360);
    for (int frame : {0, 120, 240, 336, 360, 479}) {
        editor.setFrame(frame);
        QCoreApplication::processEvents();
        if (window.grabWindow().isNull())
            throw std::runtime_error("Native canvas could not present a visual-shot beat.");
    }
    const auto visualCopy = QUrl::fromLocalFile(temporary.path() + "/visual-shot.otoon");
    if (!editor.saveProject(visualCopy) || !editor.openProject(visualCopy) ||
        editor.document() != visualDocument ||
        SceneRenderer::render(editor.document(), 360) != visualFrame)
        throw std::runtime_error("Visual-shot UI save/reopen changed the authored scene.");
    auto unmatched = visualDocument;
    const Id incoming = unmatched.drawingAt(visualArm, 300)->id;
    for (auto& binding : unmatched.layer(visualArm).bindings)
        if (binding.drawing == incoming && binding.bone)
            std::erase_if(binding.bone->keys,
                          [](const BonePoseKey& key) { return key.frame == 300; });
    unmatched.validate();
    const auto unmatchedPath = std::filesystem::path(
        (temporary.path() + "/unmatched-sleeve.otoon").toStdString());
    if (ProjectStore::save(unmatchedPath, unmatched) == 0)
        throw std::runtime_error("Native smoke could not save the unmatched sleeve.");
    if (!editor.openProject(QUrl::fromLocalFile(QString::fromStdString(unmatchedPath.string()))))
        throw std::runtime_error("Native editor could not open the unmatched sleeve.");
    editor.setSelectedLayer(int(visualArm));
    editor.setFrame(300);
    if (!editor.selectedCanMatchPreviousDeformerPose() ||
        editor.document() != unmatched ||
        !editor.matchSelectedPreviousDeformerPose())
        throw std::runtime_error("Native pose matching did not key the incoming sleeve.");
    const auto matched = editor.document();
    const Id outgoing = matched.drawingAt(visualArm, 299)->id;
    const auto outgoingAngles = sampleBoneAngles(
        *meshBindingFor(matched.layer(visualArm), outgoing)->bone, 300);
    const auto incomingAngles = sampleBoneAngles(
        *meshBindingFor(matched.layer(visualArm), incoming)->bone, 300);
    if (outgoingAngles != incomingAngles || matched == unmatched)
        throw std::runtime_error("Native pose matching left a sleeve jump.");
    editor.undo();
    if (editor.document() != unmatched)
        throw std::runtime_error("Undo did not restore the unmatched sleeve.");
    editor.redo();
    if (editor.document() != matched)
        throw std::runtime_error("Redo did not restore the matched sleeve.");
    const auto matchedCopy = QUrl::fromLocalFile(temporary.path() + "/matched-sleeve.otoon");
    if (!editor.saveProject(matchedCopy) || !editor.openProject(matchedCopy) ||
        editor.document() != matched)
        throw std::runtime_error("Reopening changed the matched sleeve pose.");
}

void meshInteractionBenchmark(EditorController& editor, CanvasItem& canvas,
                              QQuickWindow& window, const QString& project) {
    if (!editor.openProject(QUrl::fromLocalFile(project)))
        throw std::runtime_error("Cannot open the HM-06 benchmark project: " +
                                 editor.status().toStdString());
    Id arm = 0;
    for (const auto& layer : editor.document().layers)
        if (layer.name == "upper_arm_left" || layer.name == "arm_left")
            arm = layer.id;
    if (!arm)
        throw std::runtime_error("HM-06 benchmark project has no bound left arm Part.");
    int sampleFrame = 12;
    if (qEnvironmentVariableIsSet("OPENTOON_HM06_BENCH_FRAME")) {
        bool valid = false;
        sampleFrame = qEnvironmentVariable("OPENTOON_HM06_BENCH_FRAME").toInt(&valid);
        if (!valid || sampleFrame < 0 || sampleFrame >= editor.duration())
            throw std::runtime_error("HM-06 benchmark frame is outside the scene.");
    }
    editor.setSelectedLayer(int(arm));
    editor.setFrame(sampleFrame);
    editor.setTool("Mesh");
    canvas.setCameraGuidesVisible(false);
    canvas.fit();
    const int partCount = int(std::count_if(editor.document().layers.begin(),
                                           editor.document().layers.end(),
                                           [](const Layer& layer) {
                                               return layer.kind == LayerKind::Part;
                                           }));
    if (editor.selectedMeshDeformer() != 1 || (partCount != 19 && partCount != 15))
        throw std::runtime_error("HM-06 benchmark requires a bound Harmony scene.");
    const auto original = editor.document();
    QCoreApplication::processEvents();
    const auto before = window.grabWindow();
    if (before.isNull())
        throw std::runtime_error("HM-06 benchmark could not capture its initial canvas.");
    auto activateWindow = [&] {
        window.raise();
        window.requestActivate();
        QElapsedTimer activationTimer;
        activationTimer.start();
        while (!window.isActive() && activationTimer.elapsed() < 3000) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
        if (!window.isActive())
            throw std::runtime_error("HM-06 benchmark window did not become active.");
    };
    const QPointF tip = canvas.meshControlPosition(2);
    auto send = [&](QEvent::Type type, QPointF local, Qt::MouseButton button,
                    Qt::MouseButtons held) {
        const auto scene = canvas.mapToScene(local);
        QMouseEvent event(type, scene, window.mapToGlobal(scene.toPoint()), button, held,
                          Qt::NoModifier);
        QCoreApplication::sendEvent(&window, &event);
    };
    std::vector<double> samples;
    samples.reserve(40);
    bool pressed = false;
    for (int index = 0; index < 45; ++index) {
        bool measured = false;
        for (int attempt = 0; attempt < 5 && !measured; ++attempt) {
            QCoreApplication::processEvents();
            if (!window.isActive()) {
                pressed = false;
                activateWindow();
            }
            if (!pressed) {
                send(QEvent::MouseButtonPress, tip, Qt::LeftButton, Qt::LeftButton);
                pressed = true;
                QCoreApplication::processEvents();
                if (!window.isActive())
                    continue;
            }
            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot(true);
            bool presented = false;
            const auto frameConnection = QObject::connect(&window, &QQuickWindow::frameSwapped,
                                                          &loop, [&] {
                presented = true;
                loop.quit();
            }, Qt::QueuedConnection);
            const auto focusConnection = QObject::connect(&window, &QWindow::activeChanged,
                                                          &loop, [&] {
                if (!window.isActive())
                    loop.quit();
            });
            QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
            QElapsedTimer timer;
            timer.start();
            const auto target = tip + (index % 2 ? QPointF(-5, 6) : QPointF(-3, 4));
            send(QEvent::MouseMove, target, Qt::NoButton, Qt::LeftButton);
            timeout.start(3000);
            if (!presented && window.isActive())
                loop.exec();
            QObject::disconnect(frameConnection);
            QObject::disconnect(focusConnection);
            if (!presented) {
                if (!window.isActive()) {
                    pressed = false;
                    continue;
                }
                throw std::runtime_error("HM-06 preview did not present a frame after mouse input at move " +
                                         std::to_string(index) + ".");
            }
            if (index >= 5)
                samples.push_back(timer.nsecsElapsed() / 1e6);
            measured = true;
        }
        if (!measured)
            throw std::runtime_error("HM-06 benchmark lost focus during five attempts at move " +
                                     std::to_string(index) + ".");
    }
    const auto during = window.grabWindow();
    if (during.isNull() || during == before ||
        !during.save("build/hm06-interaction-ui.png"))
        throw std::runtime_error("HM-06 preview did not visibly change after arm drag.");
    canvas.cancelGesture();
    if (editor.document() != original)
        throw std::runtime_error("HM-06 preview benchmark changed the authored document.");
    std::sort(samples.begin(), samples.end());
    struct rusage usage {};
    if (getrusage(RUSAGE_SELF, &usage) != 0)
        throw std::runtime_error("HM-06 benchmark could not read process memory.");
#ifdef __APPLE__
    const auto peakBytes = qint64(usage.ru_maxrss);
#else
    const auto peakBytes = qint64(usage.ru_maxrss) * 1024;
#endif
    const QJsonObject report{{"profile", "native Qt Quick input-to-frameSwapped"},
                             {"documentParts", partCount},
                             {"sampleFrame", sampleFrame},
                             {"sceneWidth", editor.sceneWidth()},
                             {"sceneHeight", editor.sceneHeight()},
                             {"canvasWidth", canvas.width()},
                             {"canvasHeight", canvas.height()},
                             {"devicePixelRatio", window.devicePixelRatio()},
                             {"sampleCount", int(samples.size())},
                             {"medianMs", samples[samples.size() / 2]},
                             {"p95Ms", samples[std::size_t(std::ceil(samples.size() * .95)) - 1]},
                             {"peakProcessResidentBytes", peakBytes}};
    std::cout << QJsonDocument(report).toJson(QJsonDocument::Compact).constData() << '\n';
}
