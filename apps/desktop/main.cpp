#include "authoring_smoke.h"
#include "camera_smoke.h"
#include "canvas_item.h"
#include "editor_controller.h"
#include "key_block_smoke.h"
#include "motion_path_smoke.h"
#include "mesh_smoke.h"
#include "project_store.h"
#include "scene_renderer.h"
#include "serialization.h"
#include "vector_selection_smoke.h"
#include "visual_editing_smoke.h"
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTabletEvent>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <sys/resource.h>
#include <vector>
int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName("OPEN-TOON");
    QCoreApplication::setApplicationVersion(OPENTOON_VERSION);
    QCoreApplication::setOrganizationName("OPEN-TOON");
    QQuickStyle::setStyle("Basic");
    const auto args = app.arguments();
    if (args.contains("--version")) {
        std::cout << OPENTOON_VERSION << '\n';
        return 0;
    }
    if (args.contains("--smoke-test") || args.contains("--hm06-benchmark") ||
        args.contains("--hm07-smoke") || args.contains("--hm10-smoke")) {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName(args.contains("--smoke-test")
                                                 ? "OPEN-TOON-smoke"
                                                 : args.contains("--hm07-smoke")
                                                       ? "OPEN-TOON-hm07-smoke"
                                                       : args.contains("--hm10-smoke")
                                                             ? "OPEN-TOON-hm10-smoke"
                                                       : "OPEN-TOON-benchmark");
    }
    try {
        if (args.contains("--render-demo")) {
            int index = args.indexOf("--render-demo");
            if (index + 1 >= args.size())
                throw std::runtime_error("Usage: open-toon --render-demo OUTPUT_DIRECTORY");
            QString output = args[index + 1];
            if (!QDir().mkpath(output))
                throw std::runtime_error("Cannot create output directory.");
            auto d = opentoon::makeBouncingBall();
            auto path = std::filesystem::path(
                reinterpret_cast<const char8_t*>((output + "/bouncing-ball.otoon").toUtf8().constData()));
            auto revision = opentoon::ProjectStore::save(path, d);
            auto reopened = opentoon::ProjectStore::load(path).document;
            QElapsedTimer timer;
            timer.start();
            for (int f = 0; f < d.duration; ++f) {
                auto image = opentoon::SceneRenderer::render(reopened, f);
                if (!image.save(output + QString("/frame_%1.png").arg(f + 1, 6, 10, QChar('0'))))
                    throw std::runtime_error("PNG export failed.");
            }
            QJsonObject report{{"status", "complete"},
                               {"frames", d.duration},
                               {"revision", qint64(revision)},
                               {"elapsedMs", timer.elapsed()},
                               {"semanticRoundTrip", d == reopened},
                               {"width", d.width},
                               {"height", d.height}};
            std::cout << QJsonDocument(report).toJson().constData();
            return 0;
        }
        if (args.contains("--inspect")) {
            int index = args.indexOf("--inspect");
            if (index + 1 >= args.size())
                throw std::runtime_error("Usage: open-toon --inspect PROJECT");
            auto loaded = opentoon::ProjectStore::load(std::filesystem::path(
                reinterpret_cast<const char8_t*>(args[index + 1].toUtf8().constData())));
            std::cout << opentoon::serializeDocument(loaded.document) << '\n';
            return 0;
        }
        qmlRegisterType<CanvasItem>("OpenToon.Native", 1, 0, "DrawingCanvas");
        qmlRegisterUncreatableType<EditorController>("OpenToon.Native", 1, 0, "EditorController",
                                                     "Provided by the application");
        EditorController editor;
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("editor", &editor);
        QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); },
            Qt::QueuedConnection);
        engine.loadFromModule("OpenToon", "Main");
        if (args.contains("--demo"))
            editor.loadDemo();
        if (args.contains("--hm10-smoke")) {
            QTimer::singleShot(1200, &app, [&] {
                try {
                    if (engine.rootObjects().isEmpty())
                        throw std::runtime_error("No QML window for HM-10 smoke.");
                    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
                    if (!window || !window->findChild<QQuickItem*>("timelineCanvas"))
                        throw std::runtime_error("The audio timeline has no native canvas.");
                    editor.newScene();
                    editor.setWorkspaceMode("Rig");
                    QTemporaryDir directory;
                    if (!directory.isValid())
                        throw std::runtime_error("Cannot create HM-10 WAV fixture.");
                    QByteArray bytes;
                    auto u16 = [&](quint16 value) {
                        bytes.append(char(value & 255)); bytes.append(char(value >> 8));
                    };
                    auto u32 = [&](quint32 value) { u16(value & 65535); u16(value >> 16); };
                    bytes.append("RIFF", 4); u32(36 + 48000 * 2); bytes.append("WAVEfmt ", 8);
                    u32(16); u16(1); u16(1); u32(48000); u32(96000); u16(2); u16(16);
                    bytes.append("data", 4); u32(48000 * 2);
                    for (int sample = 0; sample < 48000; ++sample)
                        u16(sample == 2002 ? 32767 : (sample % 80 < 40 ? 9000 : quint16(-9000)));
                    QFile source(directory.filePath("original-cue.wav"));
                    if (!source.open(QIODevice::WriteOnly) || source.write(bytes) != bytes.size())
                        throw std::runtime_error("Cannot write HM-10 WAV fixture.");
                    source.close();
                    const auto baseline = editor.document();
                    if (!editor.importAudio(QUrl::fromLocalFile(source.fileName())) ||
                        editor.audioClips().size() != 1)
                        throw std::runtime_error("Native WAV import did not create a clip.");
                    const auto clip = editor.audioClips().front().toMap().value("id").toInt();
                    const auto peaks = editor.audioWaveform(clip, 0, 3);
                    if (peaks.size() != 3 || peaks[1].toDouble() < 0.99)
                        throw std::runtime_error("Native waveform cue is not sample aligned.");
                    QCoreApplication::processEvents();
                    if (!window->grabWindow().save("build/hm10-audio-smoke.png"))
                        throw std::runtime_error("Cannot capture the audio timeline.");
                    editor.undo();
                    if (editor.document() != baseline)
                        throw std::runtime_error("WAV import did not undo atomically.");
                    std::cout << "HM-10 audio smoke passed: native PCM16 import, cue waveform, "
                                 "timeline screenshot and atomic undo.\n";
                    app.exit(0);
                } catch (const std::exception& error) {
                    std::cerr << error.what() << '\n';
                    app.exit(1);
                }
            });
        }
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
                    auto root = std::find_if(editor.document().layers.begin(), editor.document().layers.end(),
                                             [](const auto& layer) {
                                                 return layer.kind == opentoon::LayerKind::Character;
                                             });
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
                    auto* canvasBlend = findVisualItem(findVisualItem, window->contentItem(),
                                                        QStringLiteral("canvasPoseBlend"));
                    auto* canvasDrawing = findVisualItem(findVisualItem, window->contentItem(),
                                                          QStringLiteral("canvasDrawingPicker"));
                    if (editor.selectedLayer() != selected || editor.frame() != frame ||
                        !dashboard->isVisible() || !posePicker->isVisible() || !poseBlend->isVisible() ||
                        !drawingGroup || !drawingGroup->isVisible() ||
                        !canvasControls || !canvasControls->isVisible() ||
                        !canvasBlend || !canvasBlend->isVisible() ||
                        !canvasDrawing || !canvasDrawing->isVisible() ||
                        !editor.characterPoses().front().toMap().value("published").toBool() ||
                        !editor.characterViews().front().toMap().value("published").toBool() ||
                        editor.publishedCharacterSubstitutions().isEmpty())
                        throw std::runtime_error(
                            "Published Animator controls are not visible or changed selection: " +
                            editor.status().toStdString() + ", dashboard=" +
                            std::to_string(dashboard->isVisible()) + ", picker=" +
                            std::to_string(posePicker->isVisible()) + ", slider=" +
                            std::to_string(poseBlend->isVisible()) + ", poses=" +
                            std::to_string(editor.characterPoses().size()) + ", views=" +
                            std::to_string(editor.characterViews().size()) + ", posePublished=" +
                            std::to_string(editor.characterPoses().front().toMap().value("published").toBool()) +
                            ", poseId=" + std::to_string(editor.selectedCharacterPose()) +
                            ", drawingGroups=" +
                            std::to_string(editor.publishedCharacterSubstitutions().size()) +
                            ", groupVisible=" +
                            std::to_string(drawingGroup && drawingGroup->isVisible()) +
                            ", groupSize=" +
                            std::to_string(drawingGroup ? drawingGroup->width() : 0) + "x" +
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
                        const auto point = slider->mapToScene(
                            QPointF(slider->width() * fraction, slider->height() / 2));
                        QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()),
                                          button, held, Qt::NoModifier);
                        QCoreApplication::sendEvent(window, &event);
                    };
                    if (canvasBlend->width() < 70 || canvasDrawing->width() < 100)
                        throw std::runtime_error("HM-07 canvas controls are too small to use.");
                    sendSlider(canvasBlend, QEvent::MouseButtonPress, .05, Qt::LeftButton, Qt::LeftButton);
                    QCoreApplication::processEvents();
                    sendSlider(canvasBlend, QEvent::MouseMove, .8, Qt::NoButton, Qt::LeftButton);
                    QCoreApplication::processEvents();
                    sendSlider(canvasBlend, QEvent::MouseButtonRelease, .8, Qt::LeftButton, Qt::NoButton);
                    if (opentoon::evaluateTransform(editor.document().layer(torsoId), 0).x <=
                        originalTorsoX + 40)
                        throw std::runtime_error("HM-07 canvas slider did not move its mapped Part.");
                    if (editor.document().drawingAt(mouthId, 0)->id !=
                            baseline.drawingAt(mouthId, 0)->id ||
                        opentoon::evaluateTransform(editor.document().layer(torsoId), 0).rotation !=
                            opentoon::evaluateTransform(baseline.layer(torsoId), 0).rotation ||
                        editor.selectedLayer() != selected || editor.frame() != frame)
                        throw std::runtime_error("HM-07 canvas slider changed an unbound property or view state.");
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
                        const auto connection = QObject::connect(window, &QQuickWindow::frameSwapped,
                                                                 &loop, [&] {
                            presented = true;
                            loop.quit();
                        }, Qt::QueuedConnection);
                        QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
                        QElapsedTimer timer;
                        timer.start();
                        sendSlider(poseBlend, QEvent::MouseMove, index % 2 ? .2 : .8,
                                   Qt::NoButton, Qt::LeftButton);
                        timeout.start(3000);
                        if (!presented)
                            loop.exec();
                        QObject::disconnect(connection);
                        if (!presented)
                            throw std::runtime_error("HM-07 slider did not present a frame after input.");
                        if (index >= 5)
                            samples.push_back(timer.nsecsElapsed() / 1e6);
                    }
                    if (opentoon::evaluateTransform(editor.document().layer(torsoId), 0).x <=
                        originalTorsoX + 40)
                        throw std::runtime_error("HM-07 slider did not move its mapped Part.");
                    sendSlider(poseBlend, QEvent::MouseButtonRelease, .8, Qt::LeftButton, Qt::NoButton);
                    editor.undo();
                    if (editor.document() != baseline)
                        throw std::runtime_error("HM-07 slider drag did not undo in one step.");
                    std::sort(samples.begin(), samples.end());
                    struct rusage usage {};
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
                    for (auto* ancestor = transferPicker ? transferPicker->parentItem() : nullptr;
                         ancestor; ancestor = ancestor->parentItem()) {
                        if (ancestor->property("contentY").isValid()) {
                            const double rowY = transferPicker->mapToItem(ancestor, QPointF(0, 0)).y();
                            ancestor->setProperty("contentY", ancestor->property("contentY").toDouble() +
                                                              rowY - 280);
                            break;
                        }
                    }
                    QCoreApplication::processEvents();
                    if (!window->grabWindow().save("build/hm07-transfer-smoke.png") ||
                        !transferPicker || !transferPicker->isVisible() || transferPicker->width() < 80 ||
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
                    auto* mirrorButton = findVisualItem(findVisualItem, window->contentItem(),
                                                        QStringLiteral("mirrorPoseButton"));
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
                    const auto groupPixels = opentoon::SceneRenderer::render(beforeGroupSwitch, frame, {320, 180});
                    editor.setWorkspaceMode("Animator");
                    if (editor.characterControlGroups().size() != 3)
                        throw std::runtime_error("HM-07 published control groups are missing.");
                    editor.setSelectedControlGroup("Face");
                    QCoreApplication::processEvents();
                    auto* groupPicker = findVisualItem(findVisualItem, window->contentItem(),
                                                       QStringLiteral("canvasControlGroupPicker"));
                    if (!groupPicker || !groupPicker->isVisible() ||
                        !canvasControls->isVisible() || editor.selectedCharacterPose() != 0 ||
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
        if (args.contains("--hm06-benchmark")) {
            const int index = args.indexOf("--hm06-benchmark");
            if (index + 1 >= args.size())
                throw std::runtime_error("Usage: open-toon --hm06-benchmark PROJECT");
            const QString project = args[index + 1];
            QTimer::singleShot(1200, &app, [&, project] {
                try {
                    if (engine.rootObjects().isEmpty())
                        throw std::runtime_error("No QML window for HM-06 benchmark.");
                    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
                    auto* canvas = window->findChild<CanvasItem*>("drawingCanvas");
                    if (!canvas || canvas->height() < 200)
                        throw std::runtime_error("HM-06 benchmark canvas is not usable.");
                    meshInteractionBenchmark(editor, *canvas, *window, project);
                    app.exit(0);
                } catch (const std::exception& error) {
                    std::cerr << error.what() << '\n';
                    app.exit(1);
                }
            });
        }
        if (args.contains("--smoke-test")) {
            QTimer::singleShot(1200, &app, [&] {
                try {
                    if (engine.rootObjects().isEmpty())
                        throw std::runtime_error("No QML window.");
                    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
                    auto* canvas = window->findChild<CanvasItem*>("drawingCanvas");
                    if (!canvas || canvas->height() < 200)
                        throw std::runtime_error("Canvas layout is not usable.");
                    window->raise();
                    window->requestActivate();
                    QElapsedTimer activationTimer;
                    activationTimer.start();
                    while (!window->isActive() && activationTimer.elapsed() < 3000) {
                        QCoreApplication::processEvents();
                        QThread::msleep(10);
                    }
                    if (!window->isActive())
                        throw std::runtime_error("Native smoke window did not become active.");
                    editor.newScene();
                    auto start = canvas->mapToScene(QPointF(canvas->width() / 2 - 80, canvas->height() / 2));
                    auto send = [&](QEvent::Type type, QPointF point, Qt::MouseButton button,
                                    Qt::MouseButtons buttons,
                                    Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
                        QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()), button, buttons,
                                          modifiers);
                        QCoreApplication::sendEvent(window, &event);
                    };
                    send(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
                    for (int i = 1; i <= 30; ++i)
                        send(QEvent::MouseMove, start + QPointF(i * 5, std::sin(i * 0.2) * 30), Qt::NoButton,
                             Qt::LeftButton);
                    send(QEvent::MouseButtonRelease, start + QPointF(150, 0), Qt::LeftButton, Qt::NoButton);
                    const auto* drawing = editor.document().drawingAt(editor.selectedLayer(), 0);
                    if (!drawing || drawing->strokes.empty() || drawing->strokes.back().points.size() < 10)
                        throw std::runtime_error("Mouse drawing did not reach the document.");
                    const auto drawn = editor.document();
                    editor.undo();
                    if (editor.document() == drawn)
                        throw std::runtime_error("UI undo did not revert drawing.");
                    editor.redo();
                    if (editor.document() != drawn)
                        throw std::runtime_error("UI redo changed drawing semantics.");
                    QTemporaryDir temp;
                    if (!editor.saveProject(QUrl::fromLocalFile(temp.path() + "/mouse.otoon")))
                        throw std::runtime_error("UI save failed.");
                    editor.newScene();
                    if (!editor.openProject(QUrl::fromLocalFile(temp.path() + "/mouse.otoon")) ||
                        editor.document() != drawn)
                        throw std::runtime_error("UI reopen mismatch.");
                    editor.addSwatch(Qt::red);
                    editor.undo();
                    if (std::none_of(editor.document().palette.begin(), editor.document().palette.end(),
                                     [&](const auto& swatch) {
                                         return swatch.id == opentoon::Id(editor.selectedSwatch());
                                     }))
                        throw std::runtime_error("Undo left a stale palette selection.");
                    QPointingDevice pen(
                        "Synthetic test pen", 42, QInputDevice::DeviceType::Stylus,
                        QPointingDevice::PointerType::Pen,
                        QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 1);
                    auto tablet = [&](QEvent::Type type, QPointF point, double pressure) {
                        QTabletEvent event(type, &pen, point, window->mapToGlobal(point.toPoint()), pressure,
                                           0, 0, 0, 0, 0, Qt::NoModifier,
                                           type == QEvent::TabletMove ? Qt::NoButton : Qt::LeftButton,
                                           type == QEvent::TabletRelease ? Qt::NoButton : Qt::LeftButton);
                        QCoreApplication::sendEvent(window, &event);
                    };
                    tablet(QEvent::TabletPress, start, 0.2);
                    tablet(QEvent::TabletMove, start + QPointF(60, 30), 0.8);
                    tablet(QEvent::TabletRelease, start + QPointF(60, 30), 0);
                    const auto* ink = editor.document().drawingAt(editor.selectedLayer(), 0);
                    if (!ink || ink->strokes.size() != 2 ||
                        ink->strokes.back().points.front().pressure != 0.2 ||
                        ink->strokes.back().points.back().pressure != 0.8)
                        throw std::runtime_error("Tablet event routing did not preserve pressure.");
                    const auto beforeCancel = editor.document();
                    send(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
                    send(QEvent::MouseMove, start + QPointF(50, 10), Qt::NoButton, Qt::LeftButton);
                    canvas->cancelGesture();
                    send(QEvent::MouseButtonRelease, start + QPointF(50, 10), Qt::LeftButton, Qt::NoButton);
                    if (editor.document() != beforeCancel)
                        throw std::runtime_error("Cancelled gesture changed the document.");
                    editor.setTool("Raster ink");
                    editor.setBrushSize(48);
                    send(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
                    for (int i = 1; i <= 30; ++i)
                        send(QEvent::MouseMove, start + QPointF(i * 5, 40), Qt::NoButton, Qt::LeftButton);
                    send(QEvent::MouseButtonRelease, start + QPointF(150, 40), Qt::LeftButton, Qt::NoButton);
                    const auto painted = editor.document();
                    const auto* rasterDrawing = painted.drawingAt(editor.selectedLayer(), 0);
                    if (!rasterDrawing || !rasterDrawing->raster || rasterDrawing->raster->tiles.empty())
                        throw std::runtime_error("Native mouse input did not paint raster tiles: " +
                                                 editor.status().toStdString());
                    const auto rendered = opentoon::SceneRenderer::render(painted, 0);
                    editor.undo();
                    if (editor.document() != beforeCancel)
                        throw std::runtime_error("Raster undo changed previous artwork.");
                    editor.redo();
                    if (editor.document() != painted)
                        throw std::runtime_error("Raster redo mismatch.");
                    const auto rasterPath = QUrl::fromLocalFile(temp.path() + "/raster.otoon");
                    if (!editor.saveProject(rasterPath) || !editor.openProject(rasterPath) ||
                        opentoon::SceneRenderer::render(editor.document(), 0) != rendered)
                        throw std::runtime_error("Raster save and reopen changed rendered pixels.");
                    send(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
                    send(QEvent::MouseMove, start + QPointF(80, 60), Qt::NoButton, Qt::LeftButton);
                    canvas->cancelGesture();
                    send(QEvent::MouseButtonRelease, start + QPointF(80, 60), Qt::LeftButton, Qt::NoButton);
                    if (editor.document() != painted)
                        throw std::runtime_error("Cancelled raster gesture changed artwork.");
                    editor.setTool("Marquee");
                    canvas->setSelectionMedia(2);
                    const auto selectStart = start + QPointF(-50, -80);
                    const auto selectEnd = start + QPointF(210, 100);
                    send(QEvent::MouseButtonPress, selectStart, Qt::LeftButton, Qt::LeftButton);
                    send(QEvent::MouseMove, selectEnd, Qt::NoButton, Qt::LeftButton);
                    send(QEvent::MouseButtonRelease, selectEnd, Qt::LeftButton, Qt::NoButton);
                    if (!canvas->hasRegion())
                        throw std::runtime_error("Native drawing marquee did not select a region.");
                    const auto beforeRegionMove = editor.document();
                    const auto inside = start + QPointF(50, 20);
                    send(QEvent::MouseButtonPress, inside, Qt::LeftButton, Qt::LeftButton);
                    send(QEvent::MouseMove, inside + QPointF(20, 10), Qt::NoButton, Qt::LeftButton);
                    send(QEvent::MouseButtonRelease, inside + QPointF(20, 10), Qt::LeftButton, Qt::NoButton);
                    if (editor.document() == beforeRegionMove || !canvas->hasRegion())
                        throw std::runtime_error("Native mixed drawing selection move failed: " +
                                                 editor.status().toStdString());
                    const auto beforeScale = editor.document();
                    auto scaleStart = canvas->mapToScene(canvas->handlePosition(4));
                    send(QEvent::MouseButtonPress, scaleStart, Qt::LeftButton, Qt::LeftButton);
                    send(QEvent::MouseMove, scaleStart + QPointF(20, 12), Qt::NoButton, Qt::LeftButton);
                    if (editor.document() != beforeScale)
                        throw std::runtime_error("Transform preview modified document before release.");
                    send(QEvent::MouseButtonRelease, scaleStart + QPointF(20, 12), Qt::LeftButton,
                         Qt::NoButton);
                    if (editor.document() == beforeScale || !canvas->hasRegion())
                        throw std::runtime_error("Scale handle did not commit.");
                    auto beforeRotate = editor.document();
                    auto rotateStart = canvas->mapToScene(canvas->handlePosition(8));
                    send(QEvent::MouseButtonPress, rotateStart, Qt::LeftButton, Qt::LeftButton);
                    send(QEvent::MouseMove, rotateStart + QPointF(30, 15), Qt::NoButton, Qt::LeftButton);
                    send(QEvent::MouseButtonRelease, rotateStart + QPointF(30, 15), Qt::LeftButton,
                         Qt::NoButton);
                    if (editor.document() == beforeRotate)
                        throw std::runtime_error("Rotation handle did not commit.");
                    auto oldWidth = canvas->objectProperties().value("width").toDouble();
                    canvas->setObjectProperty("width", oldWidth + 10);
                    if (!canvas->hasRegion() ||
                        canvas->objectProperties().value("width").toDouble() <= oldWidth)
                        throw std::runtime_error("Object Properties lost the selection or failed to update.");
                    // Return to the moved drawing before checking cancellation and undo below.
                    editor.undo();
                    editor.undo();
                    editor.undo();
                    // Undo clears view selection. Re-select the moved artwork.
                    send(QEvent::MouseButtonPress, selectStart, Qt::LeftButton, Qt::LeftButton);
                    send(QEvent::MouseMove, selectEnd + QPointF(50, 40), Qt::NoButton, Qt::LeftButton);
                    send(QEvent::MouseButtonRelease, selectEnd + QPointF(50, 40), Qt::LeftButton,
                         Qt::NoButton);
                    const auto afterRegionMove = editor.document();
                    send(QEvent::MouseButtonPress, inside + QPointF(20, 10), Qt::LeftButton, Qt::LeftButton);
                    send(QEvent::MouseMove, inside + QPointF(40, 20), Qt::NoButton, Qt::LeftButton);
                    canvas->cancelGesture();
                    send(QEvent::MouseButtonRelease, inside + QPointF(40, 20), Qt::LeftButton, Qt::NoButton);
                    if (editor.document() != afterRegionMove)
                        throw std::runtime_error("Cancelled region drag changed artwork.");
                    editor.undo();
                    if (editor.document() != beforeRegionMove)
                        throw std::runtime_error("Region move undo changed artwork.");
                    editor.redo();
                    const auto selectionPath = QUrl::fromLocalFile(temp.path() + "/selection.otoon");
                    const auto selectionPixels = opentoon::SceneRenderer::render(editor.document(), 0);
                    if (!editor.saveProject(selectionPath) || !editor.openProject(selectionPath) ||
                        opentoon::SceneRenderer::render(editor.document(), 0) != selectionPixels)
                        throw std::runtime_error("Selection save/reopen changed pixels.");
                    editor.selectTimelineRange(0, 5, 0, 0);
                    editor.copyTimelineRange();
                    editor.setFrame(6);
                    editor.pasteTimelineRange(0, false);
                    if (!editor.document().drawingAt(editor.selectedLayer(), 6))
                        throw std::runtime_error("Timeline range paste failed.");
                    QCoreApplication::processEvents();
                    auto* timeline = window->findChild<QQuickItem*>("timelineCanvas");
                    if (!timeline)
                        throw std::runtime_error("Timeline canvas is missing.");
                    auto cell = window->property("timelineCell").toDouble();
                    auto timelinePoint = [&](int frame) {
                        return timeline->mapToScene(QPointF((frame + 0.5) * cell, 42));
                    };
                    send(QEvent::MouseButtonPress, timelinePoint(0), Qt::LeftButton, Qt::LeftButton);
                    send(QEvent::MouseMove, timelinePoint(3), Qt::NoButton, Qt::LeftButton);
                    send(QEvent::MouseButtonRelease, timelinePoint(3), Qt::LeftButton, Qt::NoButton);
                    if (editor.rangeStart() != 0 || editor.rangeEnd() != 4)
                        throw std::runtime_error("Native timeline drag did not select the expected frames.");
                    const auto beforeStretch = editor.document();
                    auto rangeEdge = timeline->mapToScene(QPointF(4 * cell - 1, 42));
                    send(QEvent::MouseButtonPress, rangeEdge, Qt::LeftButton, Qt::LeftButton);
                    send(QEvent::MouseMove, rangeEdge + QPointF(2 * cell, 0), Qt::NoButton, Qt::LeftButton);
                    send(QEvent::MouseButtonRelease, rangeEdge + QPointF(2 * cell, 0), Qt::LeftButton,
                         Qt::NoButton);
                    if (editor.rangeEnd() != 6)
                        throw std::runtime_error("Timeline range-end handle failed to stretch.");
                    editor.undo();
                    if (editor.document() != beforeStretch)
                        throw std::runtime_error("Stretch undo changed unrelated timing.");
                    editor.selectTimelineRange(0, 3, 0, 0);
                    const auto beforeMove = editor.document();
                    send(QEvent::MouseButtonPress, timelinePoint(1), Qt::LeftButton, Qt::LeftButton,
                         Qt::AltModifier);
                    send(QEvent::MouseMove, timelinePoint(11), Qt::NoButton, Qt::LeftButton, Qt::AltModifier);
                    send(QEvent::MouseButtonRelease, timelinePoint(11), Qt::LeftButton, Qt::NoButton,
                         Qt::AltModifier);
                    if (editor.rangeStart() != 10 || editor.document().drawingAt(editor.selectedLayer(), 0) ||
                        !editor.document().drawingAt(editor.selectedLayer(), 10))
                        throw std::runtime_error(
                            "Native Alt-drag did not move the selected range with its grab offset.");
                    editor.undo();
                    if (editor.document() != beforeMove)
                        throw std::runtime_error("Range move undo did not restore exposures.");
                    editor.setFrame(0);
                    editor.setAnimateMode(true);
                    editor.setAutoKey(true);
                    editor.setTransform("x", 0);
                    editor.setFrame(24);
                    editor.setTransform("x", 240);
                    editor.setFrame(12);
                    window->setProperty("showCurves", true);
                    window->setProperty("bottomHeight", 300);
                    QTimer::singleShot(250, &app, [&, window] {
                        auto* graph = window->findChild<QQuickItem*>("animationCurveCanvas");
                        if (!graph) {
                            std::cerr << "Curve editor layout failed.\n";
                            app.exit(1);
                            return;
                        }
                        auto* panel = window->findChild<QQuickItem*>("curveEditorPanel");
                        if (!panel || !panel->property("combined").toBool()) {
                            std::cerr << "Curves must default to all motion.\n";
                            app.exit(1);
                            return;
                        }
                        QMetaObject::invokeMethod(panel, "selectChannel", Q_ARG(QVariant, QVariant("x")));
                        QEventLoop layoutReady;
                        QTimer::singleShot(100, &layoutReady, &QEventLoop::quit);
                        layoutReady.exec();
                        window->grabWindow(); // Flush layout even when another window occludes the smoke
                                              // instance.
                        const auto beforeCurveDrag = editor.document();
                        auto point = [&](int frame, double value) {
                            double left = graph->property("plotLeft").toDouble();
                            double right = graph->property("plotRight").toDouble();
                            double top = graph->property("plotTop").toDouble();
                            double bottom = graph->property("plotBottom").toDouble();
                            return graph->mapToScene(QPointF(left + frame / 47.0 * (right - left),
                                                             bottom - (value + 36) / 312.0 * (bottom - top)));
                        };
                        auto sendCurve = [&](QEvent::Type type, QPointF p, Qt::MouseButton button,
                                             Qt::MouseButtons buttons) {
                            QMouseEvent event(type, p, window->mapToGlobal(p.toPoint()), button, buttons,
                                              Qt::NoModifier);
                            QCoreApplication::sendEvent(window, &event);
                        };
                        sendCurve(QEvent::MouseButtonPress, point(24, 240), Qt::LeftButton, Qt::LeftButton);
                        sendCurve(QEvent::MouseMove, point(30, 180), Qt::NoButton, Qt::LeftButton);
                        sendCurve(QEvent::MouseButtonRelease, point(30, 180), Qt::LeftButton, Qt::NoButton);
                        const auto& keys = editor.document().layer(editor.selectedLayer()).keys;
                        if (keys.back().frame != 30 || std::abs(keys.back().value.x - 180) > 1) {
                            std::cerr << "Native curve drag did not commit time and value: channel="
                                      << panel->property("channel").toString().toStdString()
                                      << " size=" << graph->width() << "," << graph->height()
                                      << " visible=" << graph->isVisible()
                                      << " low=" << panel->property("low").toDouble()
                                      << " high=" << panel->property("high").toDouble()
                                      << " frame=" << keys.back().frame << " x=" << keys.back().value.x
                                      << " status=" << editor.status().toStdString() << "\n";
                            window->grabWindow().save("build/curve-failure.png");
                            app.exit(1);
                            return;
                        }
                        editor.undo();
                        if (editor.document() != beforeCurveDrag) {
                            std::cerr << "Curve drag undo changed other scene data.\n";
                            app.exit(1);
                            return;
                        }
                        editor.setFrame(24);
                        QCoreApplication::processEvents();
                        auto image = window->grabWindow();
                        if (image.isNull() || !image.save("build/ui-smoke.png")) {
                            app.exit(1);
                            return;
                        }
                        std::cout << "UI smoke passed: mouse stroke, synthetic pen pressure, cancelled "
                                     "gesture, raster painting and pixel round trip, range clipboard, "
                                     "undo/redo, save/reopen, canvas layout "
                                     "mixed selection move/cancel/reopen, and native curve drag/undo and "
                                     "screenshot.\n";
                        window->setProperty("showCurves", false);
                        window->setProperty("showTimingTools", true);
                        window->setProperty("bottomHeight", 200);
                        editor.setFrame(0);
                        editor.setTool("Select");
                        QTimer::singleShot(100, &app, [&, window] {
                            auto* canvas = window->findChild<CanvasItem*>("drawingCanvas");
                            const auto* drawing = editor.document().drawingAt(editor.selectedLayer(), 0);
                            if (!canvas || !drawing || drawing->strokes.empty()) {
                                app.exit(1);
                                return;
                            }
                            const auto point = drawing->strokes.front().points.front();
                            const auto mapped =
                                opentoon::SceneRenderer::worldTransform(
                                    editor.document(), editor.document().layer(editor.selectedLayer()), 0)
                                    .map(QPointF(point.x, point.y));
                            const auto scale = std::min((canvas->width() - 64) / editor.sceneWidth(),
                                                        (canvas->height() - 64) / editor.sceneHeight());
                            const auto pos = canvas->mapToScene(QPointF(
                                canvas->width() / 2 + (mapped.x() - editor.sceneWidth() / 2.0) * scale,
                                canvas->height() / 2 + (mapped.y() - editor.sceneHeight() / 2.0) * scale));
                            QMouseEvent press(QEvent::MouseButtonPress, pos,
                                              window->mapToGlobal(pos.toPoint()), Qt::LeftButton,
                                              Qt::LeftButton, Qt::NoModifier);
                            QMouseEvent release(QEvent::MouseButtonRelease, pos,
                                                window->mapToGlobal(pos.toPoint()), Qt::LeftButton,
                                                Qt::NoButton, Qt::NoModifier);
                            QCoreApplication::sendEvent(window, &press);
                            QCoreApplication::sendEvent(window, &release);
                            if (canvas->objectProperties().value("kind").toString() != "vector") {
                                std::cerr << "Object selection did not reach Properties.\n";
                                app.exit(1);
                                return;
                            }
                            canvas->setObjectProperty("strokeWidth", 8);
                            if (canvas->objectProperties().value("strokeWidth").toDouble() != 8) {
                                app.exit(1);
                                return;
                            }
                            QTimer::singleShot(100, &app, [&, window] {
                                auto shot = window->grabWindow();
                                try {
                                    if (shot.isNull() || !shot.save("build/selection-ui-smoke.png"))
                                        throw std::runtime_error("Selection screenshot failed.");
                                    auto* canvas = window->findChild<CanvasItem*>("drawingCanvas");
                                    visualEditingSmoke(editor, *canvas, *window);
                                    keyBlockSmoke(editor, *window);
                                    vectorSelectionSmoke(editor, *canvas, *window);
                                    authoringSmoke(editor, *canvas, *window);
                                    motionPathSmoke(editor, *canvas, *window);
                                    const auto beforeComposition = editor.document();
                                    editor.setCompositionProfile(1);
                                    QCoreApplication::processEvents();
                                    if (window->grabWindow().isNull())
                                        throw std::runtime_error("Linear canvas preview failed.");
                                    const int nextPreview = (editor.frame() + 1) % editor.duration();
                                    QElapsedTimer previewClock;
                                    previewClock.start();
                                    while (!canvas->hasPreparedFrame(nextPreview) &&
                                           previewClock.elapsed() < 2000)
                                        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
                                    if (!canvas->hasPreparedFrame(nextPreview))
                                        throw std::runtime_error("Background canvas preview was not published.");
                                    editor.setFrame(nextPreview);
                                    if (window->grabWindow().isNull())
                                        throw std::runtime_error("Prepared frame was not displayed.");
                                    const int playbackStart = editor.frame();
                                    editor.togglePlayback();
                                    QElapsedTimer playbackClock;
                                    playbackClock.start();
                                    while (playbackClock.elapsed() < 150)
                                        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
                                    editor.togglePlayback();
                                    if (editor.frame() == playbackStart)
                                        throw std::runtime_error("Timed canvas playback did not advance.");
                                    editor.setFrame(0);
                                    editor.setFrame(editor.duration() - 1);
                                    if (window->grabWindow().isNull())
                                        throw std::runtime_error("Rapid scrub preview failed.");
                                    editor.undo();
                                    if (editor.document() != beforeComposition)
                                        throw std::runtime_error("Composition profile undo failed.");
                                    cameraSmoke(editor, *canvas, *window);
                                    meshSmoke(editor, *canvas, *window);
                                    std::cout << "Composition smoke passed: linear canvas preview, "
                                                 "background next-frame publication, playback, scrub "
                                                 "and undo.\n";
                                    std::cout << "Camera smoke passed: direct pan/rotate/zoom, atomic undo, "
                                                 "guides, viewport isolation and save/reopen.\n";
                                    std::cout << "Mesh smoke passed: vertex, bone and curve mouse drags, "
                                                 "cancellable preview, range key paste/move, undo/redo and reopen.\n";
                                    QTimer::singleShot(150, &app, [&, window] {
                                        auto image = window->grabWindow();
                                        std::cout << "Visual animation smoke passed: thin picking, cursor "
                                                     "feedback, move/scale/rotate, cancel, undo, save/reopen "
                                                     "and Bezier handle drag.\n";
                                        app.exit(!image.isNull() &&
                                                         image.save("build/visual-animation-smoke.png")
                                                     ? 0
                                                     : 1);
                                    });
                                } catch (const std::exception& e) {
                                    std::cerr << e.what() << '\n';
                                    app.exit(1);
                                }
                            });
                        });
                    });
                } catch (const std::exception& error) {
                    std::cerr << error.what() << '\n';
                    app.exit(1);
                }
            });
        }
        return app.exec();
    } catch (const std::exception& e) {
        std::cerr << "OPEN-TOON: " << e.what() << '\n';
        return 1;
    }
}
