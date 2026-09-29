#include "authoring_smoke.h"
#include "opentoon/audio.h"
#include "opentoon/composition_graph.h"
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
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
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
#include <array>
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
        args.contains("--hm07-smoke") || args.contains("--hm10-smoke") ||
        args.contains("--hm12-smoke") || args.contains("--hm-integrated-smoke")) {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName(args.contains("--smoke-test")
                                                 ? "OPEN-TOON-smoke"
                                                 : args.contains("--hm07-smoke")
                                                       ? "OPEN-TOON-hm07-smoke"
                                                       : args.contains("--hm10-smoke")
                                                             ? "OPEN-TOON-hm10-smoke"
                                                       : args.contains("--hm12-smoke")
                                                             ? "OPEN-TOON-hm12-smoke"
                                                       : args.contains("--hm-integrated-smoke")
                                                             ? "OPEN-TOON-integrated-smoke"
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
        if (args.contains("--open")) {
            const int index = args.indexOf("--open");
            if (index + 1 >= args.size())
                throw std::runtime_error("Usage: open-toon --open PROJECT");
            const auto path = QFileInfo(args[index + 1]).absoluteFilePath();
            if (!editor.openProject(QUrl::fromLocalFile(path)))
                throw std::runtime_error("Could not open project: " + path.toStdString());
        }
        if (args.contains("--demo"))
            editor.loadDemo();
        if (args.contains("--hm-integrated-smoke")) {
            QTimer::singleShot(1200, &app, [&] {
                try {
                    if (engine.rootObjects().isEmpty())
                        throw std::runtime_error("No QML window for integrated shot smoke.");
                    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
                    if (!window || !window->findChild<QQuickItem*>("timelineCanvas"))
                        throw std::runtime_error("The integrated shot has no native timeline.");
                    const int openIndex = args.indexOf("--open");
                    if (openIndex < 0 || openIndex + 1 >= args.size())
                        throw std::runtime_error("Integrated smoke requires --open PROJECT.");
                    const auto expected = opentoon::ProjectStore::load(std::filesystem::path(
                        args[openIndex + 1].toStdString())).document;
                    if (editor.document() != expected || editor.audioClips().size() != 1)
                        throw std::runtime_error("The integrated project did not open intact in Qt Quick.");
                    const auto initial = opentoon::SceneRenderer::render(editor.document(), 0,
                                                                         QSize(480, 270));
                    editor.setFrame(264);
                    QCoreApplication::processEvents();
                    if (editor.frame() != 264 ||
                        opentoon::SceneRenderer::render(editor.document(), editor.frame(),
                                                         QSize(480, 270)) == initial)
                        throw std::runtime_error("The integrated shot did not advance visually.");
                    if (!window->grabWindow().save("build/hm-integrated-shot-smoke.png"))
                        throw std::runtime_error("Cannot capture the integrated shot window.");
                    std::cout << "Integrated shot smoke passed: project reopen, native timeline, "
                                 "audio track, visual frame change and Qt Quick screenshot.\n";
                    app.exit(0);
                } catch (const std::exception& error) {
                    std::cerr << error.what() << '\n';
                    app.exit(1);
                }
            });
        }
        if (args.contains("--hm12-smoke")) {
            QTimer::singleShot(1200, &app, [&] {
                try {
                    if (engine.rootObjects().isEmpty())
                        throw std::runtime_error("No QML window for HM-12 smoke.");
                    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
                    auto document = opentoon::makeDocument();
                    document.width = document.height = 1;
                    document.background = {0, 0, 0, 0};
                    auto& targetDrawing = document.editableDrawing(document.layers.front().id, 0);
                    targetDrawing.image = opentoon::ImageAsset{1, 1, {255, 0, 0, 128}};
                    opentoon::Layer source = document.layers.front();
                    source.id = document.allocateId();
                    source.name = "Cutter";
                    auto sourceDrawing = targetDrawing;
                    sourceDrawing.id = document.allocateId();
                    sourceDrawing.image = opentoon::ImageAsset{1, 1, {0, 0, 255, 64}};
                    document.drawings.emplace(sourceDrawing.id, sourceDrawing);
                    for (auto& exposure : source.exposures)
                        exposure.drawing = sourceDrawing.id;
                    document.layers.push_back(source);
                    document.validate();
                    QTemporaryDir directory;
                    if (!directory.isValid())
                        throw std::runtime_error("Cannot create HM-12 project fixture.");
                    const auto path = directory.filePath("cutter.otoon");
                    (void)opentoon::ProjectStore::save(std::filesystem::path(path.toStdString()), document);
                    if (!editor.openProject(QUrl::fromLocalFile(path)))
                        throw std::runtime_error("Cannot open HM-12 project fixture.");
                    editor.setWorkspaceMode("Rig");
                    editor.setSelectedLayer(int(document.layers.front().id));
                    window->setProperty("inspectorMode", "layer");
                    QCoreApplication::processEvents();
                    auto* picker = window->findChild<QQuickItem*>("cutterMattePicker");
                    if (!picker || !picker->isVisible())
                        throw std::runtime_error("Cutter matte inspector is unavailable.");
                    if (!editor.setLayerMatte(int(source.id)))
                        throw std::runtime_error("Cannot assign a cutter matte.");
                    window->setProperty("showNodes", true);
                    QCoreApplication::processEvents();
                    auto* nodes = window->findChild<QQuickItem*>("compositionNodesPanel");
                    auto* strip = window->findChild<QQuickItem*>("compositionNodeStrip");
                    if (!nodes || !nodes->isVisible() || !strip || !strip->isVisible() ||
                        editor.compositionNodes().size() != 8)
                        throw std::runtime_error("Derived composition nodes are unavailable.");
                    auto clickNode = [&](int graphId, bool altClick = false) {
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
                        auto* card = findVisualItem(findVisualItem, window->contentItem(),
                                                    QString("compositionNode%1").arg(graphId));
                        if (!card || !card->isVisible())
                            throw std::runtime_error("A composition node card is unavailable.");
                        const auto point = card->mapToScene(QPointF(20, 20));
                        const auto modifiers = altClick ? Qt::AltModifier : Qt::NoModifier;
                        QMouseEvent press(QEvent::MouseButtonPress, point,
                                          window->mapToGlobal(point.toPoint()),
                                          Qt::LeftButton, Qt::LeftButton, modifiers);
                        QMouseEvent release(QEvent::MouseButtonRelease, point,
                                            window->mapToGlobal(point.toPoint()),
                                            Qt::LeftButton, Qt::NoButton, modifiers);
                        QCoreApplication::sendEvent(window, &press);
                        QCoreApplication::sendEvent(window, &release);
                        QCoreApplication::processEvents();
                    };
                    (void)window->grabWindow(); // Complete the newly shown panel's layout before native input.
                    clickNode(3);
                    if (editor.selectedLayer() != int(source.id))
                        throw std::runtime_error("Clicking a Drawing node did not select its layer.");
                    auto* previewPanel = window->findChild<QQuickItem*>("compositionNodePreviewPanel");
                    if (!previewPanel || !previewPanel->isVisible())
                        throw std::runtime_error("Node preview panel is unavailable.");
                    const auto previewUrl = nodes->property("previewData").toString();
                    if (!previewUrl.startsWith("data:image/png;base64,"))
                        throw std::runtime_error("Drawing node preview did not render.");
                    const auto previewImage = QImage::fromData(
                        QByteArray::fromBase64(previewUrl.mid(22).toLatin1()), "PNG");
                    if (previewImage.isNull() || qBlue(previewImage.pixel(previewImage.width() / 2,
                                                                           previewImage.height() / 2)) == 0)
                        throw std::runtime_error("Drawing node preview has the wrong pixels.");
                    auto* canvasView = window->findChild<QQuickItem*>("drawingCanvas");
                    auto* displayButton = window->findChild<QQuickItem*>("nodeDisplayButton");
                    if (!canvasView || !displayButton || !displayButton->isVisible())
                        throw std::runtime_error("Alternate Display control is unavailable.");
                    const auto canvasCenter = canvasView->mapToScene(
                        QPointF(canvasView->width() / 2, canvasView->height() / 2)).toPoint();
                    const auto canvasPixel = [&] {
                        const auto grab = window->grabWindow();
                        return grab.pixelColor(QPoint(int(canvasCenter.x() * grab.devicePixelRatio()),
                                                      int(canvasCenter.y() * grab.devicePixelRatio())));
                    };
                    const auto finalPixel = canvasPixel();
                    const auto displayPoint = displayButton->mapToScene(QPointF(
                        displayButton->width() / 2, displayButton->height() / 2));
                    auto clickDisplay = [&] {
                        QMouseEvent press(QEvent::MouseButtonPress, displayPoint,
                            window->mapToGlobal(displayPoint.toPoint()),
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                        QMouseEvent release(QEvent::MouseButtonRelease, displayPoint,
                            window->mapToGlobal(displayPoint.toPoint()),
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                        QCoreApplication::sendEvent(window, &press);
                        QCoreApplication::sendEvent(window, &release);
                        QCoreApplication::processEvents();
                    };
                    const auto beforeDisplayRevision = editor.documentRevision();
                    clickDisplay();
                    const auto isolatedPixel = canvasPixel();
                    if (canvasView->property("displayNodeId").toInt() != 3 ||
                        editor.documentRevision() != beforeDisplayRevision ||
                        isolatedPixel.blue() <= isolatedPixel.red() ||
                        finalPixel.red() <= finalPixel.blue() ||
                        qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 32)
                        throw std::runtime_error("Alternate Display did not isolate the source without changing Write: id=" +
                            std::to_string(canvasView->property("displayNodeId").toInt()) +
                            " before=" + std::to_string(finalPixel.red()) + "," + std::to_string(finalPixel.blue()) +
                            " after=" + std::to_string(isolatedPixel.red()) + "," + std::to_string(isolatedPixel.blue()) +
                            " revision=" + std::to_string(editor.documentRevision()) +
                            " oldRevision=" + std::to_string(beforeDisplayRevision));
                    clickDisplay();
                    const auto restoredPixel = canvasPixel();
                    if (canvasView->property("displayNodeId").toInt() != 0 ||
                        restoredPixel.red() <= restoredPixel.blue())
                        throw std::runtime_error("Show final output did not restore the canvas.");
                    auto* drawingCanvas = qobject_cast<CanvasItem*>(canvasView);
                    if (!drawingCanvas ||
                        drawingCanvas->showCompositionNode(4, int(opentoon::GraphNodeKind::MatteFromImage),
                                                           int(source.id)) ||
                        drawingCanvas->showCompositionNode(99999, int(opentoon::GraphNodeKind::LayerImage),
                                                           int(source.id)) ||
                        drawingCanvas->displayNodeId() != 0)
                        throw std::runtime_error("Invalid Display source was accepted.");
                    clickDisplay();
                    if (canvasView->property("displayNodeId").toInt() != 3 ||
                        !editor.moveDrawingAfter(int(document.layers.front().id), int(source.id)) ||
                        canvasView->property("displayNodeId").toInt() != 0)
                        throw std::runtime_error("Alternate Display retained a stale node after reordering.");
                    editor.undo();
                    QCoreApplication::processEvents();
                    clickNode(3);
                    (void)window->grabWindow();
                    auto* displayedPreview = window->findChild<QQuickItem*>("compositionNodePreviewImage");
                    QElapsedTimer previewWait;
                    previewWait.start();
                    while (displayedPreview && displayedPreview->property("status").toInt() != 1 &&
                           previewWait.elapsed() < 1000) {
                        QCoreApplication::processEvents();
                        QThread::msleep(10);
                    }
                    if (!displayedPreview || displayedPreview->property("status").toInt() != 1)
                        throw std::runtime_error("Drawing node preview did not appear in Qt Quick.");
                    clickNode(4);
                    if (editor.selectedLayer() != int(source.id))
                        throw std::runtime_error("Clicking a Cutter node did not select its source.");
                    const auto matteUrl = nodes->property("previewData").toString();
                    const auto matteImage = QImage::fromData(
                        QByteArray::fromBase64(matteUrl.mid(22).toLatin1()), "PNG");
                    if (matteImage.isNull() ||
                        qRed(matteImage.pixel(matteImage.width() / 2,
                                              matteImage.height() / 2)) != 64)
                        throw std::runtime_error("Matte preview did not show fractional alpha.");
                    (void)window->grabWindow();
                    clickNode(5);
                    if (editor.selectedLayer() != int(document.layers.front().id) ||
                        nodes->property("previewNodeId").toInt() != 5)
                        throw std::runtime_error("Clicking Apply matte did not select its target.");
                    clickNode(5, true);
                    if (!editor.document().layer(document.layers.front().id).matteBypassed ||
                        qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 128)
                        throw std::runtime_error("Alt-clicking Apply matte did not bypass its cutter.");
                    editor.undo();
                    if (editor.document().layer(document.layers.front().id).matteBypassed ||
                        qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 32)
                        throw std::runtime_error("Node Alt-click bypass did not undo atomically.");
                    editor.redo();
                    int bypassCardId = 0;
                    for (const auto& variant : editor.compositionNodes()) {
                        const auto card = variant.toMap();
                        if (card.value("kind").toString() == "Bypassed cutter" &&
                            card.value("layer").toInt() == int(document.layers.front().id))
                            bypassCardId = card.value("id").toInt();
                    }
                    if (!bypassCardId)
                        throw std::runtime_error("Bypassed cutter card is missing.");
                    (void)window->grabWindow();
                    clickNode(bypassCardId, true);
                    if (editor.document().layer(document.layers.front().id).matteBypassed ||
                        qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 32)
                        throw std::runtime_error("Alt-clicking a bypassed cutter did not re-enable it.");
                    (void)window->grabWindow();
                    clickNode(6);
                    if (editor.selectedLayer() != int(document.layers.front().id))
                        throw std::runtime_error("Clicking a Composite node did not select its drawing.");
                    editor.setSelectedLayer(int(document.layers.front().id));
                    const auto clipped = opentoon::SceneRenderer::render(editor.document(), 0);
                    if (qAlpha(clipped.pixel(0, 0)) != 32 || qBlue(clipped.pixel(0, 0)) != 0)
                        throw std::runtime_error("Cutter matte did not clip the image correctly.");
                    if (!editor.setMatteInverted(true))
                        throw std::runtime_error("Cannot invert a cutter matte.");
                    const auto outside = opentoon::SceneRenderer::render(editor.document(), 0);
                    if (qAlpha(outside.pixel(0, 0)) != 96 ||
                        editor.compositionNodes().size() != 9)
                        throw std::runtime_error("Inverted cutter does not retain fractional coverage.");
                    QCoreApplication::processEvents();
                    if (!window->grabWindow().save("build/hm12-matte-smoke.png"))
                        throw std::runtime_error("Cannot capture HM-12 inspector.");
                    if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(path)) ||
                        opentoon::SceneRenderer::render(editor.document(), 0) != outside)
                        throw std::runtime_error("Cutter matte changed after save and reopen.");
                    editor.setSelectedLayer(int(source.id));
                    QCoreApplication::processEvents();
                    auto* opacityField = window->findChild<QQuickItem*>("nodeOpacity");
                    if (!opacityField || !opacityField->isVisible())
                        throw std::runtime_error("Node opacity control is unavailable.");
                    editor.setTransform("opacity", .5);
                    if (qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 112 ||
                        editor.compositionNodes().size() != 10)
                        throw std::runtime_error("Opacity node did not attenuate inverted cutter alpha.");
                    QCoreApplication::processEvents();
                    auto* bypassOpacity = window->findChild<QQuickItem*>("nodeBypassOpacity");
                    if (!bypassOpacity || !bypassOpacity->isVisible())
                        throw std::runtime_error("Node opacity bypass control is unavailable.");
                    const auto bypassPoint = bypassOpacity->mapToScene(
                        QPointF(bypassOpacity->width() / 2, bypassOpacity->height() / 2));
                    QMouseEvent bypassPress(QEvent::MouseButtonPress, bypassPoint,
                                            window->mapToGlobal(bypassPoint.toPoint()),
                                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QMouseEvent bypassRelease(QEvent::MouseButtonRelease, bypassPoint,
                                              window->mapToGlobal(bypassPoint.toPoint()),
                                              Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &bypassPress);
                    QCoreApplication::sendEvent(window, &bypassRelease);
                    QCoreApplication::processEvents();
                    if (!editor.document().layer(source.id).opacityBypassed ||
                        qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 96)
                        throw std::runtime_error("Clicking opacity bypass did not restore the original cutter alpha.");
                    editor.undo();
                    if (qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 112)
                        throw std::runtime_error("Opacity bypass undo did not restore attenuation.");
                    editor.redo();
                    if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(path)) ||
                        !editor.document().layer(source.id).opacityBypassed ||
                        qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 96)
                        throw std::runtime_error("Opacity bypass changed after save and reopen.");
                    editor.setSelectedLayer(int(source.id));
                    if (!editor.setOpacityBypassed(false) ||
                        qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 112)
                        throw std::runtime_error("Re-enabled opacity did not preserve its stored value.");
                    editor.setTransform("opacity", 1);
                    if (opentoon::SceneRenderer::render(editor.document(), 0) != outside)
                        throw std::runtime_error("Reset opacity did not restore cutter coverage.");
                    editor.setSelectedLayer(int(document.layers.front().id));
                    QCoreApplication::processEvents();
                    auto* bypassControl = window->findChild<QQuickItem*>("nodeBypassMatte");
                    if (!bypassControl || !bypassControl->isVisible() ||
                        !editor.setMatteBypassed(true))
                        throw std::runtime_error("Cannot bypass the saved cutter binding.");
                    const auto bypassed = opentoon::SceneRenderer::render(editor.document(), 0);
                    if (qAlpha(bypassed.pixel(0, 0)) != 128 || qBlue(bypassed.pixel(0, 0)) != 0 ||
                        editor.compositionNodes().size() != 7)
                        throw std::runtime_error("Bypassed cutter changed the target or painted its source.");
                    editor.undo();
                    if (opentoon::SceneRenderer::render(editor.document(), 0) != outside)
                        throw std::runtime_error("Cutter bypass undo did not restore coverage.");
                    editor.redo();
                    if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(path)) ||
                        opentoon::SceneRenderer::render(editor.document(), 0) != bypassed)
                        throw std::runtime_error("Cutter bypass changed after save and reopen.");
                    editor.setSelectedLayer(int(document.layers.front().id));
                    if (!editor.setMatteBypassed(false) ||
                        opentoon::SceneRenderer::render(editor.document(), 0) != outside)
                        throw std::runtime_error("Re-enabled cutter did not restore coverage.");
                    QCoreApplication::processEvents();
                    auto* paintControl = window->findChild<QQuickItem*>("paintCutterSource");
                    if (!paintControl || !paintControl->isVisible() ||
                        !editor.setMatteSourceVisible(true))
                        throw std::runtime_error("Paint cutter source control is unavailable.");
                    const auto painted = opentoon::SceneRenderer::render(editor.document(), 0);
                    if (qAlpha(painted.pixel(0, 0)) != 136 || qBlue(painted.pixel(0, 0)) == 0 ||
                        editor.compositionNodes().size() != 10)
                        throw std::runtime_error("Visible cutter does not paint with its own alpha.");
                    editor.undo();
                    if (opentoon::SceneRenderer::render(editor.document(), 0) != outside)
                        throw std::runtime_error("Paint cutter source undo did not restore output.");
                    editor.redo();
                    if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(path)) ||
                        opentoon::SceneRenderer::render(editor.document(), 0) != painted)
                        throw std::runtime_error("Visible cutter changed after save and reopen.");
                    editor.setSelectedLayer(int(document.layers.front().id));
                    if (!editor.setMatteSourceVisible(false) ||
                        opentoon::SceneRenderer::render(editor.document(), 0) != outside)
                        throw std::runtime_error("Hiding cutter source did not restore output.");
                    if (!editor.setLayerMatte(0))
                        throw std::runtime_error("Cannot remove a cutter matte.");
                    editor.undo();
                    if (opentoon::SceneRenderer::render(editor.document(), 0) != outside)
                        throw std::runtime_error("Cutter matte removal did not undo.");
                    if (!editor.setMatteSourceVisible(true) ||
                        opentoon::SceneRenderer::render(editor.document(), 0) != painted)
                        throw std::runtime_error("Cannot prepare the painted-source blend fixture.");
                    editor.setSelectedLayer(int(source.id));
                    QCoreApplication::processEvents();
                    auto* blendPicker = window->findChild<QQuickItem*>("nodeBlendMode");
                    if (!blendPicker || !blendPicker->isVisible())
                        throw std::runtime_error("Node blend mode control is unavailable.");
                    const auto blendPoint = blendPicker->mapToScene(
                        QPointF(blendPicker->width() / 2, blendPicker->height() / 2));
                    QMouseEvent blendPress(QEvent::MouseButtonPress, blendPoint,
                                           window->mapToGlobal(blendPoint.toPoint()),
                                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QMouseEvent blendRelease(QEvent::MouseButtonRelease, blendPoint,
                                             window->mapToGlobal(blendPoint.toPoint()),
                                             Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &blendPress);
                    QCoreApplication::sendEvent(window, &blendRelease);
                    QCoreApplication::processEvents();
                    auto* blendPopup = blendPicker->property("popup").value<QObject*>();
                    auto* blendList = blendPopup
                                          ? blendPopup->property("contentItem").value<QQuickItem*>()
                                          : nullptr;
                    if (!blendPopup || !blendPopup->property("visible").toBool() || !blendList)
                        throw std::runtime_error("Blend mode menu did not open.");
                    const auto multiplyPoint = blendList->mapToScene(QPointF(blendList->width() / 2, 39));
                    QMouseEvent multiplyPress(QEvent::MouseButtonPress, multiplyPoint,
                                              window->mapToGlobal(multiplyPoint.toPoint()),
                                              Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QMouseEvent multiplyRelease(QEvent::MouseButtonRelease, multiplyPoint,
                                                window->mapToGlobal(multiplyPoint.toPoint()),
                                                Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &multiplyPress);
                    QCoreApplication::sendEvent(window, &multiplyRelease);
                    QCoreApplication::processEvents();
                    const auto multiplied = opentoon::SceneRenderer::render(editor.document(), 0);
                    if (editor.document().layer(source.id).blendMode !=
                            opentoon::LayerBlendMode::Multiply ||
                        qAlpha(multiplied.pixel(0, 0)) != qAlpha(painted.pixel(0, 0)) ||
                        qBlue(multiplied.pixel(0, 0)) >= qBlue(painted.pixel(0, 0)))
                        throw std::runtime_error("Selecting Multiply did not change the painted source: mode=" +
                            std::to_string(int(editor.document().layer(source.id).blendMode)) +
                            " blue=" + std::to_string(qBlue(multiplied.pixel(0, 0))) +
                            " baseline=" + std::to_string(qBlue(painted.pixel(0, 0))));
                    editor.undo();
                    if (opentoon::SceneRenderer::render(editor.document(), 0) != painted)
                        throw std::runtime_error("Blend mode undo did not restore Normal.");
                    editor.redo();
                    if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(path)) ||
                        opentoon::SceneRenderer::render(editor.document(), 0) != multiplied)
                        throw std::runtime_error("Blend mode changed after save and reopen.");
                    editor.setSelectedLayer(int(source.id));
                    QCoreApplication::processEvents();
                    (void)window->grabWindow();
                    auto* addPicker = window->findChild<QQuickItem*>("nodeBlendMode");
                    if (!addPicker || !addPicker->isVisible())
                        throw std::runtime_error("Add blend control is unavailable.");
                    const auto addPickerPoint = addPicker->mapToScene(QPointF(
                        addPicker->width() / 2, addPicker->height() / 2));
                    QMouseEvent addOpenPress(QEvent::MouseButtonPress, addPickerPoint,
                                             window->mapToGlobal(addPickerPoint.toPoint()),
                                             Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QMouseEvent addOpenRelease(QEvent::MouseButtonRelease, addPickerPoint,
                                               window->mapToGlobal(addPickerPoint.toPoint()),
                                               Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &addOpenPress);
                    QCoreApplication::sendEvent(window, &addOpenRelease);
                    QCoreApplication::processEvents();
                    auto* addPopup = addPicker->property("popup").value<QObject*>();
                    auto* addList = addPopup
                                        ? addPopup->property("contentItem").value<QQuickItem*>()
                                        : nullptr;
                    if (!addPopup || !addPopup->property("visible").toBool() || !addList)
                        throw std::runtime_error("Add blend menu did not open.");
                    const auto addPoint = addList->mapToScene(QPointF(addList->width() / 2, 91));
                    QMouseEvent addPress(QEvent::MouseButtonPress, addPoint,
                                         window->mapToGlobal(addPoint.toPoint()),
                                         Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QMouseEvent addRelease(QEvent::MouseButtonRelease, addPoint,
                                           window->mapToGlobal(addPoint.toPoint()),
                                           Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &addPress);
                    QCoreApplication::sendEvent(window, &addRelease);
                    QCoreApplication::processEvents();
                    const auto added = opentoon::SceneRenderer::render(editor.document(), 0);
                    if (editor.document().layer(source.id).blendMode !=
                            opentoon::LayerBlendMode::Add ||
                        qAlpha(added.pixel(0, 0)) != qAlpha(multiplied.pixel(0, 0)) ||
                        qBlue(added.pixel(0, 0)) <= qBlue(multiplied.pixel(0, 0)))
                        throw std::runtime_error("Add blend did not brighten the painted cutter source.");
                    editor.undo();
                    if (opentoon::SceneRenderer::render(editor.document(), 0) != multiplied)
                        throw std::runtime_error("Add blend did not undo in one step.");
                    editor.redo();
                    if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(path)) ||
                        opentoon::SceneRenderer::render(editor.document(), 0) != added)
                        throw std::runtime_error("Add blend changed after save and reopen.");
                    editor.setSelectedLayer(int(source.id));
                    QCoreApplication::processEvents();
                    auto* bypassBlend = window->findChild<QQuickItem*>("nodeBypassBlend");
                    if (!bypassBlend || !bypassBlend->isVisible() || !bypassBlend->isEnabled())
                        throw std::runtime_error("Blend bypass control is unavailable.");
                    const auto bypassBlendPoint = bypassBlend->mapToScene(QPointF(
                        bypassBlend->width() / 2, bypassBlend->height() / 2));
                    QMouseEvent bypassBlendPress(QEvent::MouseButtonPress, bypassBlendPoint,
                        window->mapToGlobal(bypassBlendPoint.toPoint()),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QMouseEvent bypassBlendRelease(QEvent::MouseButtonRelease, bypassBlendPoint,
                        window->mapToGlobal(bypassBlendPoint.toPoint()),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &bypassBlendPress);
                    QCoreApplication::sendEvent(window, &bypassBlendRelease);
                    QCoreApplication::processEvents();
                    if (!editor.document().layer(source.id).blendBypassed ||
                        opentoon::SceneRenderer::render(editor.document(), 0) != painted)
                        throw std::runtime_error("Blend bypass did not restore Normal output.");
                    editor.undo();
                    if (opentoon::SceneRenderer::render(editor.document(), 0) != added)
                        throw std::runtime_error("Blend bypass did not undo in one step.");
                    editor.redo();
                    if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(path)) ||
                        !editor.document().layer(source.id).blendBypassed ||
                        editor.document().layer(source.id).blendMode != opentoon::LayerBlendMode::Add ||
                        opentoon::SceneRenderer::render(editor.document(), 0) != painted)
                        throw std::runtime_error("Blend bypass changed after save and reopen.");
                    editor.setSelectedLayer(int(source.id));
                    if (!editor.setBlendBypassed(false) ||
                        opentoon::SceneRenderer::render(editor.document(), 0) != added)
                        throw std::runtime_error("Re-enabled blend did not preserve Add.");

                    auto reordered = opentoon::makeDocument();
                    reordered.width = reordered.height = 1;
                    reordered.background = {0, 0, 0, 0};
                    const auto red = reordered.layers.front().id;
                    reordered.editableDrawing(red, 0).image =
                        opentoon::ImageAsset{1, 1, {255, 0, 0, 255}};
                    auto addOpaqueLayer = [&](const char* name, std::array<std::uint8_t, 4> pixel) {
                        auto layer = reordered.layers.front();
                        layer.id = reordered.allocateId();
                        layer.name = name;
                        auto drawing = reordered.drawings.at(layer.exposures.front().drawing);
                        drawing.id = reordered.allocateId();
                        drawing.image = opentoon::ImageAsset{1, 1,
                            {pixel[0], pixel[1], pixel[2], pixel[3]}};
                        reordered.drawings.emplace(drawing.id, drawing);
                        layer.exposures.front().drawing = drawing.id;
                        reordered.layers.push_back(layer);
                        return layer.id;
                    };
                    (void)addOpaqueLayer("Green", {0, 255, 0, 255});
                    const auto blue = addOpaqueLayer("Blue", {0, 0, 255, 255});
                    const auto reorderPath = directory.filePath("reorder.otoon");
                    reordered.validate();
                    (void)opentoon::ProjectStore::save(
                        std::filesystem::path(reorderPath.toStdString()), reordered);
                    if (!editor.openProject(QUrl::fromLocalFile(reorderPath)))
                        throw std::runtime_error("Cannot open node reorder fixture.");
                    nodes->setProperty("previewNodeId", 0);
                    QCoreApplication::processEvents();
                    (void)window->grabWindow();
                    auto findDrawingCard = [&](int layer) -> QQuickItem* {
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
                        for (const auto& variant : editor.compositionNodes()) {
                            const auto node = variant.toMap();
                            if (node.value("kind").toString() == "Drawing" &&
                                node.value("layer").toInt() == layer)
                                return findVisualItem(findVisualItem, window->contentItem(),
                                    QString("compositionNode%1").arg(node.value("id").toInt()));
                        }
                        return nullptr;
                    };
                    auto* redCard = findDrawingCard(int(red));
                    auto* blueCard = findDrawingCard(int(blue));
                    if (!redCard || !blueCard || !redCard->isVisible() || !blueCard->isVisible())
                        throw std::runtime_error("Drawing drag cards are unavailable.");
                    const auto from = redCard->mapToScene(QPointF(redCard->width() / 2, 30));
                    const auto to = blueCard->mapToScene(QPointF(blueCard->width() / 2, 30));
                    const auto movePoint = [&](QEvent::Type type, QPointF point,
                                               Qt::MouseButtons buttons) {
                        QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()),
                                          type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                                          buttons, Qt::NoModifier);
                        QCoreApplication::sendEvent(window, &event);
                        QCoreApplication::processEvents();
                    };
                    movePoint(QEvent::MouseButtonPress, from, Qt::LeftButton);
                    movePoint(QEvent::MouseMove, from + QPointF(16, 0), Qt::LeftButton);
                    movePoint(QEvent::MouseMove, to, Qt::LeftButton);
                    if (nodes->property("draggedLayer").toInt() != int(red) ||
                        nodes->property("dropLayer").toInt() != int(blue))
                        throw std::runtime_error("Drawing drag did not target the visible card.");
                    movePoint(QEvent::MouseButtonRelease, to, Qt::NoButton);
                    if (editor.document().layers.back().id != red ||
                        qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                        throw std::runtime_error("Dragging a Drawing did not place it above its target.");
                    editor.undo();
                    if (editor.document().layers.back().id != blue ||
                        qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                        throw std::runtime_error("Drawing drag did not undo atomically.");
                    editor.redo();
                    if (!editor.saveProject({}) ||
                        !editor.openProject(QUrl::fromLocalFile(reorderPath)) ||
                        editor.document().layers.back().id != red)
                        throw std::runtime_error("Drawing drag order changed after reopen.");
                    nodes->setProperty("previewNodeId", 0);
                    strip->setProperty("contentX", 0);
                    QCoreApplication::processEvents();
                    (void)window->grabWindow();
                    redCard = findDrawingCard(int(red));
                    blueCard = findDrawingCard(int(blue));
                    if (!redCard || !blueCard)
                        throw std::runtime_error("Drawing cards vanished after reopening order.");
                    const auto cutterFrom = redCard->mapToScene(QPointF(redCard->width() / 2, 30));
                    const auto cutterTo = blueCard->mapToScene(QPointF(blueCard->width() / 2, 30));
                    const auto altMove = [&](QEvent::Type type, QPointF point,
                                             Qt::MouseButtons buttons) {
                        QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()),
                                          type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                                          buttons, Qt::AltModifier);
                        QCoreApplication::sendEvent(window, &event);
                        QCoreApplication::processEvents();
                    };
                    altMove(QEvent::MouseButtonPress, cutterFrom, Qt::LeftButton);
                    altMove(QEvent::MouseMove, cutterFrom + QPointF(16, 0), Qt::LeftButton);
                    altMove(QEvent::MouseMove, cutterTo, Qt::LeftButton);
                    const auto altDragSource = nodes->property("draggedLayer").toInt();
                    const auto altDragTarget = nodes->property("dropLayer").toInt();
                    altMove(QEvent::MouseButtonRelease, cutterTo, Qt::NoButton);
                    if (editor.document().layer(blue).matte != red ||
                        qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                        throw std::runtime_error("Alt-drag did not bind a cutter to its target: source=" +
                            std::to_string(altDragSource) + " target=" + std::to_string(altDragTarget) +
                            " matte=" + std::to_string(editor.document().layer(blue).matte) +
                            " blue=" + std::to_string(qBlue(
                                opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0))));
                    editor.undo();
                    if (editor.document().layer(blue).matte ||
                        qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                        throw std::runtime_error("Alt-drag cutter did not undo atomically.");
                    editor.redo();
                    if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(reorderPath)) ||
                        editor.document().layer(blue).matte != red ||
                        qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                        throw std::runtime_error("Alt-drag cutter changed after reopen.");
                    window->setWidth(1000);
                    QCoreApplication::processEvents();
                    (void)window->grabWindow();
                    auto* search = window->findChild<QQuickItem*>("nodeSearch");
                    if (!search || !search->isVisible())
                        throw std::runtime_error("Node search is unavailable.");
                    search->forceActiveFocus();
                    for (const auto letter : QStringLiteral("Write")) {
                        const auto key = Qt::Key(letter.toUpper().unicode());
                        QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier, QString(letter));
                        QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier, QString(letter));
                        QCoreApplication::sendEvent(window, &press);
                        QCoreApplication::sendEvent(window, &release);
                    }
                    QCoreApplication::processEvents();
                    if (search->property("text").toString() != "Write" ||
                        nodes->property("matchingNodeIds").toList().size() != 1 ||
                        strip->property("contentX").toDouble() <= 0 ||
                        qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                        throw std::runtime_error("Typed node search did not navigate to Write without an edit: text=" +
                            search->property("text").toString().toStdString() +
                            " matches=" + std::to_string(nodes->property("matchingNodeIds").toList().size()) +
                            " index=" + std::to_string(nodes->property("matchIndex").toInt()) +
                            " scroll=" + std::to_string(strip->property("contentX").toDouble()) +
                            " width=" + std::to_string(strip->width()) +
                            " content=" + std::to_string(strip->property("contentWidth").toDouble()));
                    search->setProperty("text", "Drawing");
                    QCoreApplication::processEvents();
                    auto* nextMatch = window->findChild<QQuickItem*>("nodeSearchNext");
                    if (!nextMatch || !nextMatch->isVisible() ||
                        nodes->property("matchingNodeIds").toList().size() < 3)
                        throw std::runtime_error("Drawing search did not find all Drawing cards.");
                    const auto nextPoint = nextMatch->mapToScene(
                        QPointF(nextMatch->width() / 2, nextMatch->height() / 2));
                    movePoint(QEvent::MouseButtonPress, nextPoint, Qt::LeftButton);
                    movePoint(QEvent::MouseButtonRelease, nextPoint, Qt::NoButton);
                    if (nodes->property("matchIndex").toInt() != 1)
                        throw std::runtime_error("Next did not navigate to the second Drawing match.");
                    std::cout << "HM-12 native smoke passed: inspector, clickable node preview, opacity bypass, fractional cutter, "
                                 "painted-source Multiply/Add, blend bypass, alternate Display/Write, drawing drag order, Alt-drag cutter, Alt-click bypass, typed search, save/reopen and undo.\n";
                    app.exit(0);
                } catch (const std::exception& error) {
                    std::cerr << error.what() << '\n';
                    app.exit(1);
                }
            });
        }
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
                    editor.setFrame(24);
                    QCoreApplication::processEvents();
                    const auto findDuplicateItem = [](auto&& self, QQuickItem* parent,
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
                    auto* duplicateButton = findDuplicateItem(findDuplicateItem,
                                                            window->contentItem(),
                                                            "audioDuplicateClip");
                    if (!duplicateButton || !duplicateButton->isVisible())
                        throw std::runtime_error("Audio duplicate button is unavailable.");
                    const auto duplicatePoint = duplicateButton->mapToScene(QPointF(
                        duplicateButton->width() / 2, duplicateButton->height() / 2));
                    QMouseEvent duplicatePress(QEvent::MouseButtonPress, duplicatePoint,
                                               window->mapToGlobal(duplicatePoint.toPoint()),
                                               Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QMouseEvent duplicateRelease(QEvent::MouseButtonRelease, duplicatePoint,
                                                 window->mapToGlobal(duplicatePoint.toPoint()),
                                                 Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &duplicatePress);
                    QCoreApplication::sendEvent(window, &duplicateRelease);
                    QCoreApplication::processEvents();
                    if (editor.audioClips().size() != 2 ||
                        editor.document().audioAssets.size() != 1 ||
                        editor.audioClips().back().toMap().value("start").toInt() != 24)
                        throw std::runtime_error("Native audio duplication did not reuse the source.");
                    editor.undo();
                    if (editor.audioClips().size() != 1)
                        throw std::runtime_error("Native audio duplication did not undo in one step.");
                    editor.setFrame(12);
                    QCoreApplication::processEvents();
                    const auto beforeSplit = opentoon::AudioMixPlan(editor.document(), 48000)
                                                 .renderBlock(0, 48000);
                    auto* splitButton = findDuplicateItem(findDuplicateItem,
                                                           window->contentItem(), "audioSplitClip");
                    if (!splitButton || !splitButton->isVisible() ||
                        !splitButton->property("enabled").toBool())
                        throw std::runtime_error("Audio split button is unavailable at an interior frame.");
                    const auto splitPoint = splitButton->mapToScene(QPointF(
                        splitButton->width() / 2, splitButton->height() / 2));
                    QMouseEvent splitPress(QEvent::MouseButtonPress, splitPoint,
                                           window->mapToGlobal(splitPoint.toPoint()),
                                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QMouseEvent splitRelease(QEvent::MouseButtonRelease, splitPoint,
                                             window->mapToGlobal(splitPoint.toPoint()),
                                             Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &splitPress);
                    QCoreApplication::sendEvent(window, &splitRelease);
                    QCoreApplication::processEvents();
                    if (editor.audioClips().size() != 2 ||
                        editor.audioClips().back().toMap().value("start").toInt() != 12 ||
                        opentoon::AudioMixPlan(editor.document(), 48000)
                                .renderBlock(0, 48000) != beforeSplit)
                        throw std::runtime_error("Native audio split changed the canonical mix.");
                    editor.undo();
                    if (editor.audioClips().size() != 1)
                        throw std::runtime_error("Native audio split did not undo in one step.");
                    QCoreApplication::processEvents();
                    (void)window->grabWindow();
                    auto* muteButton = findDuplicateItem(findDuplicateItem,
                                                         window->contentItem(), "audioMuteClip");
                    if (!muteButton || !muteButton->isVisible())
                        throw std::runtime_error("Audio mute button is unavailable.");
                    const auto mutePoint = muteButton->mapToScene(QPointF(
                        muteButton->width() / 2, muteButton->height() / 2));
                    QMouseEvent mutePress(QEvent::MouseButtonPress, mutePoint,
                                          window->mapToGlobal(mutePoint.toPoint()),
                                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QMouseEvent muteRelease(QEvent::MouseButtonRelease, mutePoint,
                                            window->mapToGlobal(mutePoint.toPoint()),
                                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &mutePress);
                    QCoreApplication::sendEvent(window, &muteRelease);
                    QCoreApplication::processEvents();
                    if (!editor.audioClips().front().toMap().value("muted").toBool())
                        throw std::runtime_error("Native audio mute button did not silence its clip.");
                    if (!window->grabWindow().save("build/hm10-muted-smoke.png"))
                        throw std::runtime_error("Cannot capture the muted audio timeline.");
                    editor.undo();
                    if (editor.audioClips().front().toMap().value("muted").toBool())
                        throw std::runtime_error("Native audio mute did not undo in one step.");
                    QCoreApplication::processEvents();
                    (void)window->grabWindow();
                    auto* soloButton = findDuplicateItem(findDuplicateItem,
                                                         window->contentItem(), "audioSoloClip");
                    if (!soloButton || !soloButton->isVisible())
                        throw std::runtime_error("Audio solo button is unavailable.");
                    const auto soloPoint = soloButton->mapToScene(QPointF(
                        soloButton->width() / 2, soloButton->height() / 2));
                    QMouseEvent soloPress(QEvent::MouseButtonPress, soloPoint,
                                          window->mapToGlobal(soloPoint.toPoint()),
                                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QMouseEvent soloRelease(QEvent::MouseButtonRelease, soloPoint,
                                            window->mapToGlobal(soloPoint.toPoint()),
                                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &soloPress);
                    QCoreApplication::sendEvent(window, &soloRelease);
                    QCoreApplication::processEvents();
                    if (!editor.audioClips().front().toMap().value("solo").toBool())
                        throw std::runtime_error("Native audio solo button did not isolate its clip.");
                    if (!window->grabWindow().save("build/hm10-solo-smoke.png"))
                        throw std::runtime_error("Cannot capture the soloed audio timeline.");
                    editor.undo();
                    if (editor.audioClips().front().toMap().value("solo").toBool())
                        throw std::runtime_error("Native audio solo did not undo in one step.");
                    QCoreApplication::processEvents();
                    (void)window->grabWindow();
                    auto* balanceInput = findDuplicateItem(findDuplicateItem,
                                                           window->contentItem(), "audioBalance");
                    if (!balanceInput || !balanceInput->isVisible())
                        throw std::runtime_error("Audio balance input is unavailable.");
                    const auto balancePoint = balanceInput->mapToScene(QPointF(
                        balanceInput->width() / 2, balanceInput->height() / 2));
                    QMouseEvent balancePress(QEvent::MouseButtonPress, balancePoint,
                                             window->mapToGlobal(balancePoint.toPoint()),
                                             Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    QMouseEvent balanceRelease(QEvent::MouseButtonRelease, balancePoint,
                                               window->mapToGlobal(balancePoint.toPoint()),
                                               Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &balancePress);
                    QCoreApplication::sendEvent(window, &balanceRelease);
                    balanceInput->forceActiveFocus();
                    if (!QMetaObject::invokeMethod(balanceInput, "selectAll"))
                        throw std::runtime_error("Audio balance input could not select its current value.");
                    QKeyEvent balanceMinus(QEvent::KeyPress, Qt::Key_Minus, Qt::NoModifier, "-");
                    QKeyEvent balanceOne(QEvent::KeyPress, Qt::Key_1, Qt::NoModifier, "1");
                    QKeyEvent balanceEnter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &balanceMinus);
                    QCoreApplication::sendEvent(window, &balanceOne);
                    QCoreApplication::sendEvent(window, &balanceEnter);
                    QCoreApplication::processEvents();
                    if (editor.audioClips().front().toMap().value("balance").toDouble() != -1 ||
                        opentoon::AudioMixPlan(editor.document(), 48000).renderBlock(2002, 1)[1] != 0)
                        throw std::runtime_error("Native audio balance field did not isolate the left channel: text=" +
                                                 balanceInput->property("text").toString().toStdString() +
                                                 ", value=" + std::to_string(editor.audioClips().front()
                                                                                .toMap().value("balance").toDouble()));
                    if (!window->grabWindow().save("build/hm10-balance-smoke.png"))
                        throw std::runtime_error("Cannot capture the balanced audio control.");
                    editor.undo();
                    if (editor.audioClips().front().toMap().value("balance").toDouble() != 0)
                        throw std::runtime_error("Native audio balance did not undo in one step.");
                    editor.setFrame(0);
                    const auto peaks = editor.audioWaveform(clip, 0, 3);
                    if (peaks.size() != 3 || peaks[1].toDouble() < 0.99)
                        throw std::runtime_error("Native waveform cue is not sample aligned.");
                    QCoreApplication::processEvents();
                    if (!window->grabWindow().save("build/hm10-audio-smoke.png"))
                        throw std::runtime_error("Cannot capture the audio timeline.");
                    qputenv("OPENTOON_TEST_NULL_AUDIO_BACKEND", "1");
                    editor.togglePlayback();
                    QElapsedTimer playbackWait;
                    playbackWait.start();
                    while (editor.frame() == 0 && playbackWait.elapsed() < 1000) {
                        QCoreApplication::processEvents();
                        QThread::msleep(5);
                    }
                    if (!editor.playing() || editor.frame() == 0)
                        throw std::runtime_error("Audio device did not advance the native playhead.");
                    editor.setFrame(18);
                    if (editor.frame() != 18)
                        throw std::runtime_error("Native audio playhead seek failed.");
                    editor.togglePlayback();
                    qunsetenv("OPENTOON_TEST_NULL_AUDIO_BACKEND");
                    if (editor.playbackDiagnostics().value("callbacks").toULongLong() == 0)
                        throw std::runtime_error("Native audio callback diagnostics are empty.");
                    editor.setFrame(0);
                    auto* timelineInput = window->findChild<QQuickItem*>("timelineInput");
                    if (!timelineInput)
                        throw std::runtime_error("Native audio drag target is missing.");
                    const auto rowY = 30 + editor.layers().size() * 34 + 17;
                    const auto startPoint = timelineInput->mapToScene(QPointF(11, rowY));
                    const auto endPoint = timelineInput->mapToScene(QPointF(4 * 22 + 11, rowY));
                    const auto sendDrag = [&](QEvent::Type type, QPointF point,
                                              Qt::MouseButton button, Qt::MouseButtons held) {
                        QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()),
                                          button, held, Qt::NoModifier);
                        QCoreApplication::sendEvent(window, &event);
                        QCoreApplication::processEvents();
                    };
                    sendDrag(QEvent::MouseButtonPress, startPoint, Qt::LeftButton, Qt::LeftButton);
                    sendDrag(QEvent::MouseMove, endPoint, Qt::NoButton, Qt::LeftButton);
                    sendDrag(QEvent::MouseButtonRelease, endPoint, Qt::LeftButton, Qt::NoButton);
                    if (editor.audioClips().front().toMap().value("start").toInt() != 4)
                        throw std::runtime_error("Dragging the native waveform did not move its clip.");
                    editor.undo();
                    if (editor.audioClips().front().toMap().value("start").toInt() != 0)
                        throw std::runtime_error("Native waveform drag did not undo atomically.");
                    const auto rulerStart = timelineInput->mapToScene(QPointF(11, 15));
                    const auto rulerEnd = timelineInput->mapToScene(QPointF(2 * 22 + 11, 15));
                    sendDrag(QEvent::MouseButtonPress, rulerStart, Qt::LeftButton, Qt::LeftButton);
                    if (!editor.audioScrubbing())
                        throw std::runtime_error("Native timeline did not start audio scrubbing.");
                    sendDrag(QEvent::MouseMove, rulerEnd, Qt::NoButton, Qt::LeftButton);
                    if (editor.frame() != 2)
                        throw std::runtime_error("Native audio scrub did not follow the marked frame.");
                    sendDrag(QEvent::MouseButtonRelease, rulerEnd, Qt::LeftButton, Qt::NoButton);
                    if (editor.audioScrubbing())
                        throw std::runtime_error("Native audio scrub did not stop on release.");
                    if (!editor.setAudioClipRepeats(clip, 2) ||
                        editor.audioWaveform(clip, 25, 1).front().toDouble() < 0.99)
                        throw std::runtime_error("Repeated native waveform lost its second cue.");
                    const auto output = directory.filePath("mix.wav");
                    editor.exportAudio(QUrl::fromLocalFile(output));
                    QElapsedTimer timeout;
                    timeout.start();
                    while (editor.exporting() && timeout.elapsed() < 15000) {
                        QCoreApplication::processEvents();
                        QThread::msleep(1);
                    }
                    QFile rendered(output);
                    if (editor.exporting() || !rendered.open(QIODevice::ReadOnly) ||
                        rendered.size() != 44 + 96000 * 4)
                        throw std::runtime_error("Native PCM WAV export length is incorrect.");
                    const auto mix = rendered.readAll();
                    if (quint8(mix[44 + 2002 * 4]) != 255 ||
                        quint8(mix[44 + 2002 * 4 + 1]) != 127 ||
                        mix.mid(44 + 50002 * 4, 4) != mix.mid(44 + 2002 * 4, 4))
                        throw std::runtime_error("Native PCM WAV cue shifted during export.");
                    const auto selectedOutput = directory.filePath("selected.wav");
                    editor.exportAudioRange(QUrl::fromLocalFile(selectedOutput), 1, 3);
                    timeout.restart();
                    while (editor.exporting() && timeout.elapsed() < 15000) {
                        QCoreApplication::processEvents();
                        QThread::msleep(1);
                    }
                    QFile selected(selectedOutput);
                    if (editor.exporting() || !selected.open(QIODevice::ReadOnly) ||
                        selected.size() != 44 + 4000 * 4 ||
                        selected.readAll().mid(44) != mix.mid(44 + 2000 * 4, 4000 * 4))
                        throw std::runtime_error("Native selected WAV range shifted or changed duration.");
                    const auto beforeFadeWindow = window->grabWindow();
                    const auto findVisualItem = [](auto&& self, QQuickItem* parent,
                                                   const QString& name) -> QQuickItem* {
                        if (!parent)
                            return nullptr;
                        if (parent->objectName() == name)
                            return parent;
                        for (auto* child : parent->childItems())
                            if (auto* found = self(self, child, name))
                                return found;
                        return nullptr;
                    };
                    const auto* fadeInControl = findVisualItem(findVisualItem, window->contentItem(),
                                                                "audioFadeInSamples");
                    const auto* fadeOutControl = findVisualItem(findVisualItem, window->contentItem(),
                                                                 "audioFadeOutSamples");
                    if (!fadeInControl || !fadeOutControl)
                        throw std::runtime_error("Native audio fade controls are unavailable.");
                    if (!editor.setAudioClipFades(clip, 4000, 4000))
                        throw std::runtime_error("Native audio fades were rejected.");
                    if (editor.audioClips().front().toMap().value("fadeInSamples").toInt() != 4000)
                        throw std::runtime_error("Native audio fade did not reach the document.");
                    const auto afterFadeWindow = window->grabWindow();
                    if (!afterFadeWindow.save("build/hm10-fade-smoke.png"))
                        throw std::runtime_error("Cannot capture the native fade guide.");
                    const auto rowTop = timelineInput->mapToScene(QPointF(
                        0, 30 + editor.layers().size() * 34));
                    const auto scale = afterFadeWindow.devicePixelRatio();
                    const int left = int(std::floor(rowTop.x() * scale));
                    const int top = int(std::floor(rowTop.y() * scale));
                    int changedPixels = 0;
                    for (int y = top; y < top + int(34 * scale) && y < afterFadeWindow.height(); ++y)
                        for (int x = left; x < left + int(44 * scale) && x < afterFadeWindow.width(); ++x)
                            changedPixels += beforeFadeWindow.pixel(x, y) != afterFadeWindow.pixel(x, y);
                    if (changedPixels < 5)
                        throw std::runtime_error("The audio fade guide did not appear on the timeline.");
                    const auto fadeHandle = timelineInput->mapToScene(QPointF(44, rowY - 11));
                    const auto fadeDragged = timelineInput->mapToScene(QPointF(66, rowY - 11));
                    sendDrag(QEvent::MouseButtonPress, fadeHandle, Qt::LeftButton, Qt::LeftButton);
                    sendDrag(QEvent::MouseMove, fadeDragged, Qt::NoButton, Qt::LeftButton);
                    sendDrag(QEvent::MouseButtonRelease, fadeDragged, Qt::LeftButton, Qt::NoButton);
                    if (editor.audioClips().front().toMap().value("fadeInSamples").toInt() != 6000)
                        throw std::runtime_error("Dragging the native fade handle did not adjust samples.");
                    editor.undo();
                    if (editor.audioClips().front().toMap().value("fadeInSamples").toInt() != 4000)
                        throw std::runtime_error("Native fade-handle drag did not undo in one step.");
                    editor.redo();
                    if (editor.audioClips().front().toMap().value("fadeInSamples").toInt() != 6000)
                        throw std::runtime_error("Native fade-handle drag did not redo.");
                    editor.undo();
                    const auto fadeCancelled = timelineInput->mapToScene(QPointF(88, rowY - 11));
                    sendDrag(QEvent::MouseButtonPress, fadeHandle, Qt::LeftButton, Qt::LeftButton);
                    sendDrag(QEvent::MouseMove, fadeCancelled, Qt::NoButton, Qt::LeftButton);
                    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &escape);
                    QCoreApplication::processEvents();
                    sendDrag(QEvent::MouseButtonRelease, fadeCancelled, Qt::LeftButton, Qt::NoButton);
                    if (editor.audioClips().front().toMap().value("fadeInSamples").toInt() != 4000)
                        throw std::runtime_error("Escape did not cancel the native fade-handle drag.");
                    const auto fadeOutHandle = timelineInput->mapToScene(QPointF(1012, rowY - 11));
                    const auto fadeOutDragged = timelineInput->mapToScene(QPointF(990, rowY - 11));
                    sendDrag(QEvent::MouseButtonPress, fadeOutHandle, Qt::LeftButton, Qt::LeftButton);
                    sendDrag(QEvent::MouseMove, fadeOutDragged, Qt::NoButton, Qt::LeftButton);
                    sendDrag(QEvent::MouseButtonRelease, fadeOutDragged, Qt::LeftButton, Qt::NoButton);
                    if (editor.audioClips().front().toMap().value("fadeOutSamples").toInt() != 6000)
                        throw std::runtime_error("Dragging the native fade-out handle did not adjust samples.");
                    editor.undo();
                    if (editor.audioClips().front().toMap().value("fadeOutSamples").toInt() != 4000)
                        throw std::runtime_error("Native fade-out handle did not undo in one step.");
                    editor.redo();
                    if (editor.audioClips().front().toMap().value("fadeOutSamples").toInt() != 6000)
                        throw std::runtime_error("Native fade-out handle did not redo.");
                    editor.undo();
                    editor.undo();
                    if (editor.audioClips().front().toMap().value("fadeInSamples").toInt() != 0)
                        throw std::runtime_error("Native audio fade undo failed.");
                    editor.redo();
                    const auto fadedOutput = directory.filePath("faded.wav");
                    editor.exportAudio(QUrl::fromLocalFile(fadedOutput));
                    timeout.restart();
                    while (editor.exporting() && timeout.elapsed() < 15000) {
                        QCoreApplication::processEvents();
                        QThread::msleep(1);
                    }
                    QFile fadedFile(fadedOutput);
                    if (editor.exporting() || !fadedFile.open(QIODevice::ReadOnly))
                        throw std::runtime_error("Native faded WAV export failed.");
                    const auto fadedMix = fadedFile.readAll();
                    if (fadedMix.size() != mix.size() ||
                        fadedMix.mid(44, 4) != QByteArray(4, '\0') ||
                        fadedMix.mid(44 + 2002 * 4, 4) == mix.mid(44 + 2002 * 4, 4) ||
                        fadedMix.mid(44 + 50002 * 4, 4) != mix.mid(44 + 50002 * 4, 4))
                        throw std::runtime_error("Native fade changed the repeated cue or its endpoints.");
                    editor.undo(); // Fades.
                    editor.undo(); // Repeat count.
                    (void)window->grabWindow();
                    const auto beforeEdgeTrim = opentoon::AudioMixPlan(editor.document(), 48000)
                                                    .renderBlock(0, 48000);
                    const auto rightTrimHandle = timelineInput->mapToScene(QPointF(24 * 22, rowY));
                    const auto rightTrimTarget = timelineInput->mapToScene(QPointF(22 * 22, rowY));
                    sendDrag(QEvent::MouseButtonPress, rightTrimHandle, Qt::LeftButton, Qt::LeftButton);
                    sendDrag(QEvent::MouseMove, rightTrimTarget, Qt::NoButton, Qt::LeftButton);
                    sendDrag(QEvent::MouseButtonRelease, rightTrimTarget, Qt::LeftButton, Qt::NoButton);
                    if (editor.audioClips().front().toMap().value("outSample").toLongLong() != 44000)
                        throw std::runtime_error("Dragging the native right trim edge did not change its sample.");
                    if (!window->grabWindow().save("build/hm10-edge-trim-smoke.png"))
                        throw std::runtime_error("Cannot capture the trimmed audio timeline.");
                    const auto afterEdgeTrim = opentoon::AudioMixPlan(editor.document(), 48000)
                                                   .renderBlock(0, 48000);
                    if (!std::equal(beforeEdgeTrim.begin(), beforeEdgeTrim.begin() + 44000 * 2,
                                    afterEdgeTrim.begin()) ||
                        !std::all_of(afterEdgeTrim.begin() + 44000 * 2, afterEdgeTrim.end(),
                                     [](auto sample) { return sample == 0; }))
                        throw std::runtime_error("Native right trim changed surviving PCM samples.");
                    editor.undo();
                    if (editor.audioClips().front().toMap().value("outSample").toLongLong() != 48000)
                        throw std::runtime_error("Native right trim did not undo in one step.");
                    (void)window->grabWindow();
                    sendDrag(QEvent::MouseButtonPress, rightTrimHandle, Qt::LeftButton, Qt::LeftButton);
                    sendDrag(QEvent::MouseMove, rightTrimTarget, Qt::NoButton, Qt::LeftButton);
                    QKeyEvent trimEscape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
                    QCoreApplication::sendEvent(window, &trimEscape);
                    QCoreApplication::processEvents();
                    sendDrag(QEvent::MouseButtonRelease, rightTrimTarget, Qt::LeftButton, Qt::NoButton);
                    if (editor.audioClips().front().toMap().value("outSample").toLongLong() != 48000)
                        throw std::runtime_error("Escape did not cancel the native edge trim.");
                    (void)window->grabWindow();
                    const auto leftTrimHandle = timelineInput->mapToScene(QPointF(1, rowY));
                    const auto leftTrimTarget = timelineInput->mapToScene(QPointF(2 * 22, rowY));
                    sendDrag(QEvent::MouseButtonPress, leftTrimHandle, Qt::LeftButton, Qt::LeftButton);
                    sendDrag(QEvent::MouseMove, leftTrimTarget, Qt::NoButton, Qt::LeftButton);
                    sendDrag(QEvent::MouseButtonRelease, leftTrimTarget, Qt::LeftButton, Qt::NoButton);
                    if (editor.audioClips().front().toMap().value("start").toInt() != 2 ||
                        editor.audioClips().front().toMap().value("inSample").toLongLong() != 4000)
                        throw std::runtime_error("Dragging the native left trim edge did not retain source timing.");
                    editor.undo();
                    if (editor.audioClips().front().toMap().value("start").toInt() != 0 ||
                        editor.audioClips().front().toMap().value("inSample").toLongLong() != 0)
                        throw std::runtime_error("Native left trim did not undo in one step.");
                    editor.undo();
                    if (editor.document() != baseline)
                        throw std::runtime_error("WAV import did not undo atomically.");
                    std::cout << "HM-10 audio smoke passed: native PCM16 import, shared-source clip duplication/undo, exact split/undo, mute/undo, solo/undo, balance/undo, cue waveform, "
                                 "device-clock playhead/seek, waveform and edge-trim drags/undo, audio scrub/repeat, source-sample fades, exact full and selected-range WAV export, timeline screenshot and atomic undo.\n";
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
