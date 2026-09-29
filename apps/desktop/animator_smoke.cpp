#include "desktop_smoke.h"
#include "editor_controller.h"
#include "scene_renderer.h"
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <sys/resource.h>
#include <vector>

void scheduleAnimatorSmoke(const QStringList& args, QGuiApplication& app, EditorController& editor,
                           QQmlApplicationEngine& engine) {
    if (args.contains("--hm07-smoke")) {
        QTimer::singleShot(1200, &app, [&] {
            try {
                if (engine.rootObjects().isEmpty())
                    throw std::runtime_error("No QML window for HM-07 smoke.");
                auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
                auto* dashboard = window->findChild<QQuickItem*>("animatorDashboard");
                auto* posePicker = window->findChild<QQuickItem*>("animatorPosePicker");
                auto* poseBlend = window->findChild<QQuickItem*>("animatorPoseBlend");
                if (!dashboard || !posePicker || !poseBlend)
                    throw std::runtime_error("Animator dashboard controls are missing.");
                editor.setWorkspaceMode("Rig");
                const auto scene = QDir::currentPath() + "/examples/clockwork-continuous.otoon";
                if (!editor.openProject(QUrl::fromLocalFile(scene)))
                    throw std::runtime_error("Cannot open the original continuous-character project.");
                editor.setOnionSkin(false);
                editor.setTool("Select");
                auto root = std::find_if(
                    editor.document().layers.begin(), editor.document().layers.end(),
                    [](const auto& layer) { return layer.kind == opentoon::LayerKind::Character; });
                if (root == editor.document().layers.end())
                    throw std::runtime_error("Continuous-character root is missing.");
                const auto rootId = root->id;
                auto torso = std::find_if(editor.document().layers.begin(), editor.document().layers.end(),
                                          [](const auto& layer) { return layer.name == "torso"; });
                if (torso == editor.document().layers.end())
                    throw std::runtime_error("Continuous-character torso is missing.");
                const auto torsoId = torso->id;
                const auto originalTorsoX = torso->transform.x;
                auto mouth = std::find_if(editor.document().layers.begin(), editor.document().layers.end(),
                                          [](const auto& layer) { return layer.name == "mouth"; });
                if (mouth == editor.document().layers.end() || mouth->variants.size() < 2)
                    throw std::runtime_error("Continuous-character mouth variants are missing.");
                const auto mouthId = mouth->id;
                const auto alternateMouth = mouth->variants[1].drawing;
                editor.setAnimateMode(false);
                editor.setSelectedLayer(int(torsoId));
                editor.setTransform("x", originalTorsoX + 80);
                editor.setSelectedLayer(int(rootId));
                editor.captureSelectedCharacterPose(opentoon::PoseChannels::PositionX, true);
                const int poseId = editor.selectedCharacterPose();
                editor.setSelectedLayer(int(torsoId));
                editor.setTransform("x", originalTorsoX);
                editor.setSelectedLayer(int(rootId));
                editor.setSelectedCharacterPosePublished(true);
                editor.setSelectedViewPublished(true);
                editor.setSelectedLayer(int(mouthId));
                const auto originalMouth = editor.selectedSubstitution();
                editor.setSelectedSubstitutionPublished(true);
                editor.selectSubstitution(int(alternateMouth));
                editor.setSelectedSubstitutionPublished(true);
                editor.selectSubstitution(originalMouth);
                editor.setSelectedLayer(int(rootId));
                const auto selected = editor.selectedLayer();
                const auto frame = editor.frame();
                editor.setWorkspaceMode("Animator");
                QCoreApplication::processEvents();
                if (!window->grabWindow().save("build/hm07-dashboard-smoke.png"))
                    throw std::runtime_error("Cannot save the Animator dashboard screenshot.");
                QCoreApplication::processEvents();
                const auto findVisualItem = [](auto&& self, QQuickItem* parent,
                                               const QString& name) -> QQuickItem* {
                    if (!parent)
                        return nullptr;
                    if (parent->objectName() == name)
                        return parent;
                    for (auto* child : parent->childItems())
                        if (auto* match = self(self, child, name))
                            return match;
                    return nullptr;
                };
                auto* drawingGroup = findVisualItem(findVisualItem, window->contentItem(),
                                                    QStringLiteral("publishedDrawingGroup"));
                auto* canvasControls = findVisualItem(findVisualItem, window->contentItem(),
                                                      QStringLiteral("canvasAnimatorControls"));
                auto* canvasBlend =
                    findVisualItem(findVisualItem, window->contentItem(), QStringLiteral("canvasPoseBlend"));
                auto* canvasDrawing = findVisualItem(findVisualItem, window->contentItem(),
                                                     QStringLiteral("canvasDrawingPicker"));
                if (editor.selectedLayer() != selected || editor.frame() != frame ||
                    !dashboard->isVisible() || !posePicker->isVisible() || !poseBlend->isVisible() ||
                    !drawingGroup || !drawingGroup->isVisible() || !canvasControls ||
                    !canvasControls->isVisible() || !canvasBlend || !canvasBlend->isVisible() ||
                    !canvasDrawing || !canvasDrawing->isVisible() ||
                    !editor.characterPoses().front().toMap().value("published").toBool() ||
                    !editor.characterViews().front().toMap().value("published").toBool() ||
                    editor.publishedCharacterSubstitutions().isEmpty())
                    throw std::runtime_error(
                        "Published Animator controls are not visible or changed selection: " +
                        editor.status().toStdString() +
                        ", dashboard=" + std::to_string(dashboard->isVisible()) +
                        ", picker=" + std::to_string(posePicker->isVisible()) +
                        ", slider=" + std::to_string(poseBlend->isVisible()) +
                        ", poses=" + std::to_string(editor.characterPoses().size()) +
                        ", views=" + std::to_string(editor.characterViews().size()) + ", posePublished=" +
                        std::to_string(editor.characterPoses().front().toMap().value("published").toBool()) +
                        ", poseId=" + std::to_string(editor.selectedCharacterPose()) +
                        ", drawingGroups=" + std::to_string(editor.publishedCharacterSubstitutions().size()) +
                        ", groupVisible=" + std::to_string(drawingGroup && drawingGroup->isVisible()) +
                        ", groupSize=" + std::to_string(drawingGroup ? drawingGroup->width() : 0) + "x" +
                        std::to_string(drawingGroup ? drawingGroup->height() : 0));
                if (!window->grabWindow().save("build/hm07-canvas-controls-smoke.png"))
                    throw std::runtime_error("Cannot save the canvas controls screenshot.");
                const auto beforeMouth = editor.document();
                if (!editor.applyPublishedSubstitution(int(mouthId), int(alternateMouth)) ||
                    editor.document().drawingAt(mouthId, frame)->id != alternateMouth)
                    throw std::runtime_error("Published mouth switch did not change its target.");
                for (const auto& layer : beforeMouth.layers)
                    if (layer.id != mouthId && editor.document().layer(layer.id) != layer)
                        throw std::runtime_error("Published mouth switch changed another Part.");
                editor.undo();
                if (editor.document() != beforeMouth)
                    throw std::runtime_error("Published mouth switch did not undo atomically.");
                const auto baseline = editor.document();
                window->raise();
                window->requestActivate();
                QElapsedTimer activation;
                activation.start();
                while (!window->isActive() && activation.elapsed() < 3000) {
                    QCoreApplication::processEvents();
                    QThread::msleep(10);
                }
                if (!window->isActive() || poseBlend->width() < 100)
                    throw std::runtime_error("HM-07 slider window is not active or usable.");
                auto sendSlider = [&](QQuickItem* slider, QEvent::Type type, double fraction,
                                      Qt::MouseButton button, Qt::MouseButtons held) {
                    const auto point =
                        slider->mapToScene(QPointF(slider->width() * fraction, slider->height() / 2));
                    QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()), button, held,
                                      Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &event);
                };
                if (canvasBlend->width() < 70 || canvasDrawing->width() < 100)
                    throw std::runtime_error("HM-07 canvas controls are too small to use.");
                sendSlider(canvasBlend, QEvent::MouseButtonPress, .05, Qt::LeftButton, Qt::LeftButton);
                QCoreApplication::processEvents();
                sendSlider(canvasBlend, QEvent::MouseMove, .8, Qt::NoButton, Qt::LeftButton);
                QCoreApplication::processEvents();
                sendSlider(canvasBlend, QEvent::MouseButtonRelease, .8, Qt::LeftButton, Qt::NoButton);
                if (opentoon::evaluateTransform(editor.document().layer(torsoId), 0).x <= originalTorsoX + 40)
                    throw std::runtime_error("HM-07 canvas slider did not move its mapped Part.");
                if (editor.document().drawingAt(mouthId, 0)->id != baseline.drawingAt(mouthId, 0)->id ||
                    opentoon::evaluateTransform(editor.document().layer(torsoId), 0).rotation !=
                        opentoon::evaluateTransform(baseline.layer(torsoId), 0).rotation ||
                    editor.selectedLayer() != selected || editor.frame() != frame)
                    throw std::runtime_error(
                        "HM-07 canvas slider changed an unbound property or view state.");
                editor.undo();
                if (editor.document() != baseline)
                    throw std::runtime_error("HM-07 canvas slider drag did not undo in one step.");
                sendSlider(poseBlend, QEvent::MouseButtonPress, .05, Qt::LeftButton, Qt::LeftButton);
                QCoreApplication::processEvents();
                std::vector<double> samples;
                samples.reserve(40);
                for (int index = 0; index < 45; ++index) {
                    QEventLoop loop;
                    QTimer timeout;
                    timeout.setSingleShot(true);
                    bool presented = false;
                    const auto connection = QObject::connect(
                        window, &QQuickWindow::frameSwapped, &loop,
                        [&] {
                            presented = true;
                            loop.quit();
                        },
                        Qt::QueuedConnection);
                    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
                    QElapsedTimer timer;
                    timer.start();
                    sendSlider(poseBlend, QEvent::MouseMove, index % 2 ? .2 : .8, Qt::NoButton,
                               Qt::LeftButton);
                    timeout.start(3000);
                    if (!presented)
                        loop.exec();
                    QObject::disconnect(connection);
                    if (!presented)
                        throw std::runtime_error("HM-07 slider did not present a frame after input.");
                    if (index >= 5)
                        samples.push_back(timer.nsecsElapsed() / 1e6);
                }
                if (opentoon::evaluateTransform(editor.document().layer(torsoId), 0).x <= originalTorsoX + 40)
                    throw std::runtime_error("HM-07 slider did not move its mapped Part.");
                sendSlider(poseBlend, QEvent::MouseButtonRelease, .8, Qt::LeftButton, Qt::NoButton);
                editor.undo();
                if (editor.document() != baseline)
                    throw std::runtime_error("HM-07 slider drag did not undo in one step.");
                std::sort(samples.begin(), samples.end());
                struct rusage usage{};
                if (getrusage(RUSAGE_SELF, &usage) != 0)
                    throw std::runtime_error("HM-07 slider benchmark could not read memory.");
#ifdef __APPLE__
                const auto peakBytes = qint64(usage.ru_maxrss);
#else
                    const auto peakBytes = qint64(usage.ru_maxrss) * 1024;
#endif
                const QJsonObject timing{{"profile", "native published-pose mouse-to-frameSwapped"},
                                         {"documentParts", 15},
                                         {"sampleCount", int(samples.size())},
                                         {"p95Ms", samples[std::size_t(std::ceil(samples.size() * .95)) - 1]},
                                         {"peakProcessResidentBytes", peakBytes}};
                editor.setWorkspaceMode("Rig");
                if (canvasControls->isVisible())
                    throw std::runtime_error("Animator canvas controls remained visible in Rig.");
                editor.duplicateCharacter();
                const int targetCharacter = editor.characterId();
                if (targetCharacter == int(rootId) || editor.characterPoses().isEmpty())
                    throw std::runtime_error("HM-07 pose destination copy is missing.");
                editor.removeSelectedCharacterPose();
                editor.setSelectedLayer(int(rootId));
                window->setProperty("inspectorMode", QStringLiteral("layer"));
                QCoreApplication::processEvents();
                auto* transferPicker = findVisualItem(findVisualItem, window->contentItem(),
                                                      QStringLiteral("poseTransferTargetPicker"));
                for (auto* ancestor = transferPicker ? transferPicker->parentItem() : nullptr; ancestor;
                     ancestor = ancestor->parentItem()) {
                    if (ancestor->property("contentY").isValid()) {
                        const double rowY = transferPicker->mapToItem(ancestor, QPointF(0, 0)).y();
                        ancestor->setProperty("contentY",
                                              ancestor->property("contentY").toDouble() + rowY - 280);
                        break;
                    }
                }
                QCoreApplication::processEvents();
                if (!window->grabWindow().save("build/hm07-transfer-smoke.png") || !transferPicker ||
                    !transferPicker->isVisible() || transferPicker->width() < 80 ||
                    transferPicker->mapToScene(QPointF(0, 0)).y() > window->height())
                    throw std::runtime_error("HM-07 pose transfer control is not visible.");
                const auto beforeTransfer = editor.document();
                if (!editor.transferSelectedCharacterPose(targetCharacter) ||
                    editor.characterId() != targetCharacter || editor.characterPoses().size() != 1)
                    throw std::runtime_error("HM-07 pose transfer did not select its independent copy.");
                editor.undo();
                if (editor.document() != beforeTransfer)
                    throw std::runtime_error("HM-07 pose transfer did not undo atomically.");
                editor.setSelectedLayer(int(rootId));
                editor.selectCharacterPose(int(poseId));
                QCoreApplication::processEvents();
                auto* mirrorButton =
                    findVisualItem(findVisualItem, window->contentItem(), QStringLiteral("mirrorPoseButton"));
                if (!mirrorButton || !mirrorButton->isVisible() ||
                    mirrorButton->mapToScene(QPointF(0, 0)).y() > window->height())
                    throw std::runtime_error("HM-07 mirror pose control is not visible.");
                const auto beforeMirror = editor.document();
                if (!editor.mirrorSelectedCharacterPose() || editor.characterPoses().size() != 2)
                    throw std::runtime_error("HM-07 mirror pose command failed.");
                editor.undo();
                if (editor.document() != beforeMirror)
                    throw std::runtime_error("HM-07 mirror pose did not undo atomically.");
                editor.toggleLayer(targetCharacter, "visible");
                editor.setSelectedCharacterPoseControlGroup("Body");
                editor.setSelectedViewControlGroup("Stage");
                editor.setSelectedLayer(int(mouthId));
                editor.setSelectedSubstitutionControlGroup("Face");
                editor.selectSubstitution(int(alternateMouth));
                editor.setSelectedSubstitutionControlGroup("Face");
                editor.selectSubstitution(originalMouth);
                editor.setSelectedLayer(int(rootId));
                const auto beforeGroupSwitch = editor.document();
                const auto groupPixels =
                    opentoon::SceneRenderer::render(beforeGroupSwitch, frame, {320, 180});
                editor.setWorkspaceMode("Animator");
                if (editor.characterControlGroups().size() != 3)
                    throw std::runtime_error("HM-07 published control groups are missing.");
                editor.setSelectedControlGroup("Face");
                QCoreApplication::processEvents();
                auto* groupPicker = findVisualItem(findVisualItem, window->contentItem(),
                                                   QStringLiteral("canvasControlGroupPicker"));
                if (!groupPicker || !groupPicker->isVisible() || !canvasControls->isVisible() ||
                    editor.selectedCharacterPose() != 0 ||
                    !window->grabWindow().save("build/hm07-groups-smoke.png"))
                    throw std::runtime_error("HM-07 Face control group is not visible and isolated.");
                editor.setSelectedControlGroup("Body");
                if (editor.selectedCharacterPose() != poseId || !canvasControls->isVisible())
                    throw std::runtime_error("HM-07 Body control group did not expose its pose.");
                editor.setSelectedControlGroup("Stage");
                if (editor.selectedCharacterPose() != 0 || canvasControls->isVisible() ||
                    editor.document() != beforeGroupSwitch ||
                    opentoon::SceneRenderer::render(editor.document(), frame, {320, 180}) != groupPixels)
                    throw std::runtime_error("HM-07 control group switch changed document output.");
                std::cout << "HM-07 dashboard smoke passed: continuous toon project, published view, "
                             "mouth drawing and pose, workspace selection/frame, native QML screenshot, "
                             "mouth switch, canvas and panel sliders, pose transfer, mirroring and "
                             "control groups with undo.\n";
                std::cout << QJsonDocument(timing).toJson(QJsonDocument::Compact).constData() << '\n';
                app.exit(0);
            } catch (const std::exception& error) {
                std::cerr << error.what() << '\n';
                app.exit(1);
            }
        });
    }
}
