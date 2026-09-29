#include "canvas_item.h"
#include "desktop_smoke.h"
#include "editor_controller.h"
#include "native_smoke_support.h"
#include "opentoon/rigging.h"
#include "project_store.h"
#include "scene_renderer.h"
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QThread>
#include <QUrl>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

void scheduleCompositionSmoke(const QStringList& args, QGuiApplication& app, EditorController& editor,
                              QQmlApplicationEngine& engine) {
    if (args.contains("--hm12-smoke")) {
        scheduleNativeSmoke(app, [&] {
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
            editor.setBottomPanelTab("Nodes");
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
                QMouseEvent press(QEvent::MouseButtonPress, point, window->mapToGlobal(point.toPoint()),
                                  Qt::LeftButton, Qt::LeftButton, modifiers);
                QMouseEvent release(QEvent::MouseButtonRelease, point, window->mapToGlobal(point.toPoint()),
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
            const auto previewImage =
                QImage::fromData(QByteArray::fromBase64(previewUrl.mid(22).toLatin1()), "PNG");
            if (previewImage.isNull() ||
                qBlue(previewImage.pixel(previewImage.width() / 2, previewImage.height() / 2)) == 0)
                throw std::runtime_error("Drawing node preview has the wrong pixels.");
            auto* canvasView = window->findChild<QQuickItem*>("drawingCanvas");
            auto* displayButton = window->findChild<QQuickItem*>("nodeDisplayButton");
            if (!canvasView || !displayButton || !displayButton->isVisible())
                throw std::runtime_error("Alternate Display control is unavailable.");
            const auto canvasCenter =
                canvasView->mapToScene(QPointF(canvasView->width() / 2, canvasView->height() / 2)).toPoint();
            const auto canvasPixel = [&] {
                const auto grab = window->grabWindow();
                return grab.pixelColor(QPoint(int(canvasCenter.x() * grab.devicePixelRatio()),
                                              int(canvasCenter.y() * grab.devicePixelRatio())));
            };
            const auto finalPixel = canvasPixel();
            auto clickDisplay = [&] {
                const auto displayPoint = displayButton->mapToScene(
                    QPointF(displayButton->width() / 2, displayButton->height() / 2));
                QMouseEvent press(QEvent::MouseButtonPress, displayPoint,
                                  window->mapToGlobal(displayPoint.toPoint()), Qt::LeftButton, Qt::LeftButton,
                                  Qt::NoModifier);
                QMouseEvent release(QEvent::MouseButtonRelease, displayPoint,
                                    window->mapToGlobal(displayPoint.toPoint()), Qt::LeftButton, Qt::NoButton,
                                    Qt::NoModifier);
                QCoreApplication::sendEvent(window, &press);
                QCoreApplication::sendEvent(window, &release);
                QCoreApplication::processEvents();
            };
            const auto beforeDisplayRevision = editor.documentRevision();
            clickDisplay();
            const auto isolatedPixel = canvasPixel();
            if (canvasView->property("displayNodeId").toInt() != 3 ||
                editor.documentRevision() != beforeDisplayRevision ||
                isolatedPixel.blue() <= isolatedPixel.red() || finalPixel.red() <= finalPixel.blue() ||
                qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 32)
                throw std::runtime_error(
                    "Alternate Display did not isolate the source without changing Write: id=" +
                    std::to_string(canvasView->property("displayNodeId").toInt()) +
                    " before=" + std::to_string(finalPixel.red()) + "," + std::to_string(finalPixel.blue()) +
                    " after=" + std::to_string(isolatedPixel.red()) + "," +
                    std::to_string(isolatedPixel.blue()) +
                    " revision=" + std::to_string(editor.documentRevision()) +
                    " oldRevision=" + std::to_string(beforeDisplayRevision));
            clickDisplay();
            const auto restoredPixel = canvasPixel();
            if (canvasView->property("displayNodeId").toInt() != 0 ||
                restoredPixel.red() <= restoredPixel.blue())
                throw std::runtime_error("Show final output did not restore the canvas.");
            auto* drawingCanvas = qobject_cast<CanvasItem*>(canvasView);
            if (!drawingCanvas ||
                drawingCanvas->showCompositionNode(4, int(opentoon::GraphNodeKind::LayerTransform),
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
            const auto matteImage =
                QImage::fromData(QByteArray::fromBase64(matteUrl.mid(22).toLatin1()), "PNG");
            if (matteImage.isNull() ||
                qRed(matteImage.pixel(matteImage.width() / 2, matteImage.height() / 2)) != 64)
                throw std::runtime_error("Matte preview did not show fractional alpha.");
            clickDisplay();
            const auto matteCanvasPixel = canvasPixel();
            if (drawingCanvas->displayNodeId() != 4 || std::abs(matteCanvasPixel.red() - 64) > 3 ||
                std::abs(matteCanvasPixel.green() - 64) > 3 || std::abs(matteCanvasPixel.blue() - 64) > 3 ||
                qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 32)
                throw std::runtime_error("Matte Display did not show fractional alpha as grayscale.");
            clickDisplay();
            if (drawingCanvas->displayNodeId() != 0)
                throw std::runtime_error("Matte Display did not return to final output.");
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
            if (qAlpha(outside.pixel(0, 0)) != 96 || editor.compositionNodes().size() != 9)
                throw std::runtime_error("Inverted cutter does not retain fractional coverage.");
            (void)window->grabWindow();
            clickNode(5);
            clickDisplay();
            const auto inverseCanvasPixel = canvasPixel();
            if (drawingCanvas->displayNodeId() != 5 || std::abs(inverseCanvasPixel.red() - 191) > 3 ||
                std::abs(inverseCanvasPixel.green() - 191) > 3 ||
                std::abs(inverseCanvasPixel.blue() - 191) > 3 ||
                qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 96)
                throw std::runtime_error("Inverted matte Display did not show outside alpha.");
            clickDisplay();
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
            const auto opacityAlpha =
                qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0));
            const auto opacityNodeCount = editor.compositionNodes().size();
            if (opacityAlpha != 112 || opacityNodeCount != 10)
                throw std::runtime_error("Opacity node did not attenuate inverted cutter alpha: alpha=" +
                                         std::to_string(opacityAlpha) +
                                         " nodes=" + std::to_string(opacityNodeCount));
            QCoreApplication::processEvents();
            auto* bypassOpacity = window->findChild<QQuickItem*>("nodeBypassOpacity");
            if (!bypassOpacity || !bypassOpacity->isVisible())
                throw std::runtime_error("Node opacity bypass control is unavailable.");
            const auto bypassPoint =
                bypassOpacity->mapToScene(QPointF(bypassOpacity->width() / 2, bypassOpacity->height() / 2));
            QMouseEvent bypassPress(QEvent::MouseButtonPress, bypassPoint,
                                    window->mapToGlobal(bypassPoint.toPoint()), Qt::LeftButton,
                                    Qt::LeftButton, Qt::NoModifier);
            QMouseEvent bypassRelease(QEvent::MouseButtonRelease, bypassPoint,
                                      window->mapToGlobal(bypassPoint.toPoint()), Qt::LeftButton,
                                      Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &bypassPress);
            QCoreApplication::sendEvent(window, &bypassRelease);
            QCoreApplication::processEvents();
            if (!editor.document().layer(source.id).opacityBypassed ||
                qAlpha(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 96)
                throw std::runtime_error(
                    "Clicking opacity bypass did not restore the original cutter alpha.");
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
            if (!bypassControl || !bypassControl->isVisible() || !editor.setMatteBypassed(true))
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
            if (!paintControl || !paintControl->isVisible() || !editor.setMatteSourceVisible(true))
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
            const auto blendPoint =
                blendPicker->mapToScene(QPointF(blendPicker->width() / 2, blendPicker->height() / 2));
            QMouseEvent blendPress(QEvent::MouseButtonPress, blendPoint,
                                   window->mapToGlobal(blendPoint.toPoint()), Qt::LeftButton, Qt::LeftButton,
                                   Qt::NoModifier);
            QMouseEvent blendRelease(QEvent::MouseButtonRelease, blendPoint,
                                     window->mapToGlobal(blendPoint.toPoint()), Qt::LeftButton, Qt::NoButton,
                                     Qt::NoModifier);
            QCoreApplication::sendEvent(window, &blendPress);
            QCoreApplication::sendEvent(window, &blendRelease);
            QCoreApplication::processEvents();
            auto* blendPopup = blendPicker->property("popup").value<QObject*>();
            auto* blendList = blendPopup ? blendPopup->property("contentItem").value<QQuickItem*>() : nullptr;
            if (!blendPopup || !blendPopup->property("visible").toBool() || !blendList)
                throw std::runtime_error("Blend mode menu did not open.");
            const auto multiplyPoint = blendList->mapToScene(QPointF(blendList->width() / 2, 39));
            QMouseEvent multiplyPress(QEvent::MouseButtonPress, multiplyPoint,
                                      window->mapToGlobal(multiplyPoint.toPoint()), Qt::LeftButton,
                                      Qt::LeftButton, Qt::NoModifier);
            QMouseEvent multiplyRelease(QEvent::MouseButtonRelease, multiplyPoint,
                                        window->mapToGlobal(multiplyPoint.toPoint()), Qt::LeftButton,
                                        Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &multiplyPress);
            QCoreApplication::sendEvent(window, &multiplyRelease);
            QCoreApplication::processEvents();
            const auto multiplied = opentoon::SceneRenderer::render(editor.document(), 0);
            if (editor.document().layer(source.id).blendMode != opentoon::LayerBlendMode::Multiply ||
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
            const auto addPickerPoint =
                addPicker->mapToScene(QPointF(addPicker->width() / 2, addPicker->height() / 2));
            QMouseEvent addOpenPress(QEvent::MouseButtonPress, addPickerPoint,
                                     window->mapToGlobal(addPickerPoint.toPoint()), Qt::LeftButton,
                                     Qt::LeftButton, Qt::NoModifier);
            QMouseEvent addOpenRelease(QEvent::MouseButtonRelease, addPickerPoint,
                                       window->mapToGlobal(addPickerPoint.toPoint()), Qt::LeftButton,
                                       Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &addOpenPress);
            QCoreApplication::sendEvent(window, &addOpenRelease);
            QCoreApplication::processEvents();
            auto* addPopup = addPicker->property("popup").value<QObject*>();
            auto* addList = addPopup ? addPopup->property("contentItem").value<QQuickItem*>() : nullptr;
            if (!addPopup || !addPopup->property("visible").toBool() || !addList)
                throw std::runtime_error("Add blend menu did not open.");
            const auto addPoint = addList->mapToScene(QPointF(addList->width() / 2, 91));
            QMouseEvent addPress(QEvent::MouseButtonPress, addPoint, window->mapToGlobal(addPoint.toPoint()),
                                 Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QMouseEvent addRelease(QEvent::MouseButtonRelease, addPoint,
                                   window->mapToGlobal(addPoint.toPoint()), Qt::LeftButton, Qt::NoButton,
                                   Qt::NoModifier);
            QCoreApplication::sendEvent(window, &addPress);
            QCoreApplication::sendEvent(window, &addRelease);
            QCoreApplication::processEvents();
            const auto added = opentoon::SceneRenderer::render(editor.document(), 0);
            if (editor.document().layer(source.id).blendMode != opentoon::LayerBlendMode::Add ||
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
            const auto bypassBlendPoint =
                bypassBlend->mapToScene(QPointF(bypassBlend->width() / 2, bypassBlend->height() / 2));
            QMouseEvent bypassBlendPress(QEvent::MouseButtonPress, bypassBlendPoint,
                                         window->mapToGlobal(bypassBlendPoint.toPoint()), Qt::LeftButton,
                                         Qt::LeftButton, Qt::NoModifier);
            QMouseEvent bypassBlendRelease(QEvent::MouseButtonRelease, bypassBlendPoint,
                                           window->mapToGlobal(bypassBlendPoint.toPoint()), Qt::LeftButton,
                                           Qt::NoButton, Qt::NoModifier);
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
            reordered.editableDrawing(red, 0).image = opentoon::ImageAsset{1, 1, {255, 0, 0, 255}};
            auto addOpaqueLayer = [&](const char* name, std::array<std::uint8_t, 4> pixel) {
                auto layer = reordered.layers.front();
                layer.id = reordered.allocateId();
                layer.name = name;
                auto drawing = reordered.drawings.at(layer.exposures.front().drawing);
                drawing.id = reordered.allocateId();
                drawing.image = opentoon::ImageAsset{1, 1, {pixel[0], pixel[1], pixel[2], pixel[3]}};
                reordered.drawings.emplace(drawing.id, drawing);
                layer.exposures.front().drawing = drawing.id;
                reordered.layers.push_back(layer);
                return layer.id;
            };
            const auto green = addOpaqueLayer("Green", {0, 255, 0, 255});
            const auto blue = addOpaqueLayer("Blue", {0, 0, 255, 255});
            const auto reorderPath = directory.filePath("reorder.otoon");
            reordered.validate();
            (void)opentoon::ProjectStore::save(std::filesystem::path(reorderPath.toStdString()), reordered);
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
                    if (node.value("kind").toString() == "Drawing" && node.value("layer").toInt() == layer)
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
            const auto movePoint = [&](QEvent::Type type, QPointF point, Qt::MouseButtons buttons) {
                QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()),
                                  type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton, buttons,
                                  Qt::NoModifier);
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
            if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(reorderPath)) ||
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
            const auto altMove = [&](QEvent::Type type, QPointF point, Qt::MouseButtons buttons) {
                QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()),
                                  type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton, buttons,
                                  Qt::AltModifier);
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
                throw std::runtime_error(
                    "Alt-drag did not bind a cutter to its target: source=" + std::to_string(altDragSource) +
                    " target=" + std::to_string(altDragTarget) +
                    " matte=" + std::to_string(editor.document().layer(blue).matte) + " blue=" +
                    std::to_string(qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0))));
            editor.undo();
            if (editor.document().layer(blue).matte ||
                qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Alt-drag cutter did not undo atomically.");
            editor.redo();
            if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(reorderPath)) ||
                editor.document().layer(blue).matte != red ||
                qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Alt-drag cutter changed after reopen.");
            strip->setProperty("contentX", 0);
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            redCard = findDrawingCard(int(red));
            blueCard = findDrawingCard(int(blue));
            if (!redCard || !blueCard)
                throw std::runtime_error("Private cutter Drawing cards are unavailable.");
            const auto privateFrom = redCard->mapToScene(QPointF(redCard->width() / 2, 30));
            const auto privateTo = blueCard->mapToScene(QPointF(blueCard->width() / 2, 30));
            const auto privateMove = [&](QEvent::Type type, QPointF point, Qt::MouseButtons buttons) {
                QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()),
                                  type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton, buttons,
                                  Qt::AltModifier | Qt::ShiftModifier);
                QCoreApplication::sendEvent(window, &event);
                QCoreApplication::processEvents();
            };
            privateMove(QEvent::MouseButtonPress, privateFrom, Qt::LeftButton);
            privateMove(QEvent::MouseMove, privateFrom + QPointF(16, 0), Qt::LeftButton);
            privateMove(QEvent::MouseMove, privateTo, Qt::LeftButton);
            privateMove(QEvent::MouseButtonRelease, privateTo, Qt::NoButton);
            if (editor.document().layer(blue).matte == red ||
                !editor.document().layer(red).compositeBypassed ||
                qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Alt+Shift-drag did not create a private cutter.");
            editor.undo();
            if (editor.document().layer(blue).matte != red)
                throw std::runtime_error("Private cutter did not undo atomically.");
            editor.redo();
            if (editor.document().layer(blue).matte == red)
                throw std::runtime_error("Private cutter did not redo atomically.");
            editor.undo();
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
                throw std::runtime_error(
                    "Typed node search did not navigate to Write without an edit: text=" +
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
            const auto nextPoint =
                nextMatch->mapToScene(QPointF(nextMatch->width() / 2, nextMatch->height() / 2));
            movePoint(QEvent::MouseButtonPress, nextPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, nextPoint, Qt::NoButton);
            if (nodes->property("matchIndex").toInt() != 1)
                throw std::runtime_error("Next did not navigate to the second Drawing match.");
            search->setProperty("text", "");
            editor.setSelectedLayer(int(blue));
            QCoreApplication::processEvents();
            auto* bypassComposite = window->findChild<QQuickItem*>("nodeBypassComposite");
            if (!bypassComposite || !bypassComposite->isVisible() || !bypassComposite->isEnabled())
                throw std::runtime_error("Composite bypass control is unavailable.");
            const auto compositeBypassPoint = bypassComposite->mapToScene(
                QPointF(bypassComposite->width() / 2, bypassComposite->height() / 2));
            movePoint(QEvent::MouseButtonPress, compositeBypassPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, compositeBypassPoint, Qt::NoButton);
            const auto bypassedImage = opentoon::SceneRenderer::render(editor.document(), 0);
            if (!editor.document().layer(blue).compositeBypassed ||
                qGreen(bypassedImage.pixel(0, 0)) != 255 || qBlue(bypassedImage.pixel(0, 0)) != 0)
                throw std::runtime_error("Composite bypass did not remove the target output.");
            int bypassNode = 0;
            for (const auto& variant : editor.compositionNodes()) {
                const auto node = variant.toMap();
                if (node.value("kind").toString() == "Bypassed composite" &&
                    node.value("layer").toInt() == int(blue))
                    bypassNode = node.value("id").toInt();
            }
            if (!bypassNode)
                throw std::runtime_error("Bypassed composite card is missing.");
            editor.undo();
            if (editor.document().layer(blue).compositeBypassed ||
                qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Composite bypass did not undo atomically.");
            editor.redo();
            if (!editor.document().layer(blue).compositeBypassed ||
                opentoon::SceneRenderer::render(editor.document(), 0) != bypassedImage)
                throw std::runtime_error("Composite bypass did not redo atomically.");
            if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(reorderPath)) ||
                !editor.document().layer(blue).compositeBypassed ||
                opentoon::SceneRenderer::render(editor.document(), 0) != bypassedImage)
                throw std::runtime_error("Composite bypass changed after reopen.");
            nodes->setProperty("previewNodeId", 0);
            strip->setProperty("contentX", 0);
            search->setProperty("text", "Bypassed composite");
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            strip->setProperty("contentX",
                               std::max(0.0, strip->property("contentWidth").toDouble() - strip->width()));
            QCoreApplication::processEvents();
            clickNode(bypassNode, true);
            if (editor.document().layer(blue).compositeBypassed ||
                qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error(
                    "Alt-click did not restore the composite node: node=" + std::to_string(bypassNode) +
                    " selected=" + std::to_string(editor.selectedLayer()) +
                    " bypass=" + std::to_string(editor.document().layer(blue).compositeBypassed) +
                    " search=" + search->property("text").toString().toStdString() +
                    " scroll=" + std::to_string(strip->property("contentX").toDouble()));
            const auto behindPath = directory.filePath("behind.otoon");
            (void)opentoon::ProjectStore::save(std::filesystem::path(behindPath.toStdString()), reordered);
            if (!editor.openProject(QUrl::fromLocalFile(behindPath)))
                throw std::runtime_error("Cannot open direct behind-order fixture.");
            search->setProperty("text", "");
            strip->setProperty("contentX", 0);
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            redCard = findDrawingCard(int(red));
            blueCard = findDrawingCard(int(blue));
            if (!redCard || !blueCard)
                throw std::runtime_error("Behind-order Drawing cards are unavailable.");
            const auto behindFrom = blueCard->mapToScene(QPointF(blueCard->width() / 2, 30));
            const auto behindTo = redCard->mapToScene(QPointF(redCard->width() / 2, 30));
            const auto shiftMove = [&](QEvent::Type type, QPointF point, Qt::MouseButtons buttons) {
                QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()),
                                  type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton, buttons,
                                  Qt::ShiftModifier);
                QCoreApplication::sendEvent(window, &event);
                QCoreApplication::processEvents();
            };
            shiftMove(QEvent::MouseButtonPress, behindFrom, Qt::LeftButton);
            shiftMove(QEvent::MouseMove, behindFrom + QPointF(16, 0), Qt::LeftButton);
            shiftMove(QEvent::MouseMove, behindTo, Qt::LeftButton);
            if (nodes->property("draggedLayer").toInt() != int(blue) ||
                nodes->property("dropLayer").toInt() != int(red))
                throw std::runtime_error("Shift-drag did not target the behind-order card.");
            shiftMove(QEvent::MouseButtonRelease, behindTo, Qt::NoButton);
            if (editor.document().layers.front().id != blue ||
                qGreen(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Shift-drag did not place Blue behind Red and Green.");
            editor.undo();
            if (editor.document().layers.back().id != blue ||
                qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Shift-drag behind order did not undo atomically.");
            editor.redo();
            if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(behindPath)) ||
                editor.document().layers.front().id != blue ||
                qGreen(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Shift-drag behind order changed after reopen.");
            strip->setProperty("contentX", 0);
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            redCard = findDrawingCard(int(red));
            blueCard = findDrawingCard(int(blue));
            if (!redCard || !blueCard)
                throw std::runtime_error("Group-boundary Drawing cards are unavailable.");
            const auto groupFirst = blueCard->mapToScene(QPointF(blueCard->width() / 2, 30));
            const auto groupLast = redCard->mapToScene(QPointF(redCard->width() / 2, 30));
            shiftMove(QEvent::MouseButtonPress, groupFirst, Qt::LeftButton);
            shiftMove(QEvent::MouseButtonRelease, groupFirst, Qt::NoButton);
            if (nodes->property("groupStartLayer").toInt() != int(blue))
                throw std::runtime_error("Shift-click did not mark the first group member.");
            shiftMove(QEvent::MouseButtonPress, groupLast, Qt::LeftButton);
            shiftMove(QEvent::MouseButtonRelease, groupLast, Qt::NoButton);
            if (editor.document().compositeGroups.size() != 1 ||
                editor.document().compositeGroups.front().members != std::vector<opentoon::Id>{blue, red} ||
                qGreen(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Shift-click grouping changed members or pixels.");
            const auto groupId = editor.document().compositeGroups.front().id;
            const auto groupedNodes = editor.compositionNodes();
            if (std::count_if(groupedNodes.begin(), groupedNodes.end(), [groupId](const auto& variant) {
                    const auto node = variant.toMap();
                    return node.value("group").toInt() == int(groupId);
                }) != 2)
                throw std::runtime_error("Composite group input/output cards are missing.");
            strip->setProperty("contentX", 0);
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            blueCard = findDrawingCard(int(blue));
            redCard = findDrawingCard(int(red));
            if (!blueCard || !redCard)
                throw std::runtime_error("Group-member drag cards are unavailable.");
            const auto memberFrom = blueCard->mapToScene(QPointF(blueCard->width() / 2, 30));
            const auto memberTo = redCard->mapToScene(QPointF(redCard->width() / 2, 30));
            movePoint(QEvent::MouseButtonPress, memberFrom, Qt::LeftButton);
            movePoint(QEvent::MouseMove, memberFrom + QPointF(16, 0), Qt::LeftButton);
            movePoint(QEvent::MouseMove, memberTo, Qt::LeftButton);
            if (nodes->property("draggedLayer").toInt() != int(blue) ||
                nodes->property("dropLayer").toInt() != int(red))
                throw std::runtime_error("Group-member drag did not target its member card: " +
                                         std::to_string(nodes->property("draggedLayer").toInt()) + "/" +
                                         std::to_string(nodes->property("dropLayer").toInt()) + " from " +
                                         std::to_string(memberFrom.x()) + " to " +
                                         std::to_string(memberTo.x()) + " strip " +
                                         std::to_string(strip->x()) + "/" + std::to_string(strip->width()));
            movePoint(QEvent::MouseButtonRelease, memberTo, Qt::NoButton);
            if (editor.document().compositeGroups.front().members != std::vector<opentoon::Id>{red, blue} ||
                editor.document().layers[0].id != red || editor.document().layers[1].id != blue)
                throw std::runtime_error("Group-member drag did not reorder group and layers.");
            editor.undo();
            if (editor.document().compositeGroups.front().members != std::vector<opentoon::Id>{blue, red})
                throw std::runtime_error("Group-member drag did not undo atomically.");
            const auto altShiftClick = [&](QQuickItem* card) {
                if (!card)
                    throw std::runtime_error("Group-member Drawing card is unavailable.");
                const auto withinStrip = card->mapToItem(strip, QPointF(card->width() / 2, 30));
                if (withinStrip.x() < 20 || withinStrip.x() > strip->width() - 20)
                    strip->setProperty("contentX", std::max(0.0, strip->property("contentX").toDouble() +
                                                                     withinStrip.x() - strip->width() / 2));
                QCoreApplication::processEvents();
                (void)window->grabWindow();
                const auto point = card->mapToScene(QPointF(card->width() / 2, 30));
                for (const auto type : {QEvent::MouseButtonPress, QEvent::MouseButtonRelease}) {
                    QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()), Qt::LeftButton,
                                      type == QEvent::MouseButtonPress ? Qt::LeftButton : Qt::NoButton,
                                      Qt::AltModifier | Qt::ShiftModifier);
                    QCoreApplication::sendEvent(window, &event);
                    QCoreApplication::processEvents();
                }
            };
            altShiftClick(findDrawingCard(int(green)));
            if (editor.document().compositeGroups.front().members !=
                std::vector<opentoon::Id>{blue, red, green})
                throw std::runtime_error("Alt+Shift-click did not add adjacent group member.");
            altShiftClick(findDrawingCard(int(green)));
            if (editor.document().compositeGroups.front().members != std::vector<opentoon::Id>{blue, red})
                throw std::runtime_error("Alt+Shift-click did not remove group edge member.");
            editor.undo();
            editor.undo();
            editor.undo();
            if (!editor.document().compositeGroups.empty())
                throw std::runtime_error("Composite grouping did not undo atomically.");
            editor.redo();
            if (editor.document().compositeGroups.size() != 1)
                throw std::runtime_error("Composite grouping did not redo atomically.");
            editor.setSelectedLayer(int(red));
            QCoreApplication::processEvents();
            auto* groupName = window->findChild<QQuickItem*>("nodeGroupName");
            if (!groupName || !groupName->isVisible())
                throw std::runtime_error("Composite group name field is unavailable.");
            groupName->setProperty("text", "Body");
            groupName->forceActiveFocus();
            QKeyEvent groupNamePress(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            QKeyEvent groupNameRelease(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &groupNamePress);
            QCoreApplication::sendEvent(window, &groupNameRelease);
            QCoreApplication::processEvents();
            if (editor.document().compositeGroups.front().name != "Body" || !editor.saveProject({}) ||
                !editor.openProject(QUrl::fromLocalFile(behindPath)) ||
                editor.document().compositeGroups.front().id != groupId ||
                editor.document().compositeGroups.front().name != "Body" ||
                qGreen(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Composite grouping changed after reopen.");
            editor.setSelectedLayer(int(red));
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            auto* front = window->findChild<QQuickItem*>("nodeFront");
            if (!front || !front->isEnabled())
                throw std::runtime_error("Composite group Front control is unavailable.");
            const auto frontPoint = front->mapToScene(QPointF(front->width() / 2, front->height() / 2));
            movePoint(QEvent::MouseButtonPress, frontPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, frontPoint, Qt::NoButton);
            if (editor.document().layers.back().id != red ||
                qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Front did not move the whole composite group.");
            editor.undo();
            if (qGreen(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Composite group order did not undo.");
            editor.redo();
            if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(behindPath)) ||
                editor.document().layers.back().id != red ||
                qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Composite group order changed after reopen.");
            editor.setSelectedLayer(int(red));
            QCoreApplication::processEvents();
            auto* bypassGroup = window->findChild<QQuickItem*>("nodeBypassGroup");
            if (!bypassGroup || !bypassGroup->isVisible() || !bypassGroup->isEnabled())
                throw std::runtime_error("Composite group bypass control is unavailable.");
            const auto bypassGroupPoint =
                bypassGroup->mapToScene(QPointF(bypassGroup->width() / 2, bypassGroup->height() / 2));
            movePoint(QEvent::MouseButtonPress, bypassGroupPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, bypassGroupPoint, Qt::NoButton);
            if (!editor.document().compositeGroups.front().bypassed ||
                qGreen(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Bypass group did not remove its composite output.");
            editor.undo();
            if (qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Bypass group did not undo.");
            editor.redo();
            if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(behindPath)) ||
                !editor.document().compositeGroups.front().bypassed ||
                qGreen(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Bypass group changed after reopen.");
            editor.setSelectedLayer(int(red));
            QCoreApplication::processEvents();
            bypassGroup = window->findChild<QQuickItem*>("nodeBypassGroup");
            const auto restoreGroupPoint =
                bypassGroup->mapToScene(QPointF(bypassGroup->width() / 2, bypassGroup->height() / 2));
            movePoint(QEvent::MouseButtonPress, restoreGroupPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, restoreGroupPoint, Qt::NoButton);
            if (editor.document().compositeGroups.front().bypassed ||
                qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Group processing did not restore from bypass.");
            (void)window->grabWindow();
            auto* duplicateGroup = window->findChild<QQuickItem*>("nodeDuplicateGroup");
            if (!duplicateGroup || !duplicateGroup->isVisible() || !duplicateGroup->isEnabled())
                throw std::runtime_error("Composite group Duplicate control is unavailable.");
            const auto duplicatePoint = duplicateGroup->mapToScene(
                QPointF(duplicateGroup->width() / 2, duplicateGroup->height() / 2));
            movePoint(QEvent::MouseButtonPress, duplicatePoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, duplicatePoint, Qt::NoButton);
            if (editor.document().compositeGroups.size() != 2 ||
                editor.document().compositeGroups.back().id == groupId ||
                editor.document().compositeGroups.back().members.front() !=
                    opentoon::Id(editor.selectedLayer()))
                throw std::runtime_error("Duplicate did not select an independent group.");
            editor.undo();
            if (editor.document().compositeGroups.size() != 1)
                throw std::runtime_error("Duplicated group did not undo atomically.");
            editor.redo();
            if (editor.document().compositeGroups.size() != 2)
                throw std::runtime_error("Duplicated group did not redo atomically.");
            editor.undo();
            editor.setSelectedLayer(int(red));
            QCoreApplication::processEvents();
            auto* back = window->findChild<QQuickItem*>("nodeBack");
            if (!back || !back->isEnabled())
                throw std::runtime_error("Composite group Back control is unavailable.");
            const auto backPoint = back->mapToScene(QPointF(back->width() / 2, back->height() / 2));
            movePoint(QEvent::MouseButtonPress, backPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, backPoint, Qt::NoButton);
            if (qGreen(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Back did not move the whole composite group.");
            auto* ungroup = window->findChild<QQuickItem*>("nodeUngroup");
            if (!ungroup || !ungroup->isVisible() || !ungroup->isEnabled())
                throw std::runtime_error("Composite Ungroup control is unavailable.");
            const auto ungroupPoint =
                ungroup->mapToScene(QPointF(ungroup->width() / 2, ungroup->height() / 2));
            movePoint(QEvent::MouseButtonPress, ungroupPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, ungroupPoint, Qt::NoButton);
            if (!editor.document().compositeGroups.empty() ||
                qGreen(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Ungroup did not restore the ungrouped graph and pixels.");
            editor.setSelectedLayer(int(red));
            QCoreApplication::processEvents();
            auto* operatorButton = window->findChild<QQuickItem*>("nodeOperatorButton");
            if (!operatorButton || !operatorButton->isVisible())
                throw std::runtime_error("Composition operator library is unavailable.");
            const auto operatorPoint = operatorButton->mapToScene(
                QPointF(operatorButton->width() / 2, operatorButton->height() / 2));
            movePoint(QEvent::MouseButtonPress, operatorPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, operatorPoint, Qt::NoButton);
            auto* operatorSearch = window->findChild<QQuickItem*>("nodeOperatorSearch");
            if (!operatorSearch || !operatorSearch->isVisible())
                throw std::runtime_error("Composition operator search did not open.");
            operatorSearch->setProperty("text", "Matte");
            QCoreApplication::processEvents();
            if (nodes->property("matchingOperators").toList().size() != 2)
                throw std::runtime_error("Operator category search did not find both mattes.");
            operatorSearch->setProperty("text", "Multiply");
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            if (nodes->property("matchingOperators").toList().size() != 1)
                throw std::runtime_error("Operator name search did not find Multiply.");
            const auto findOperatorItem = [](auto&& self, QQuickItem* parent,
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
            auto* multiplyOperator =
                findOperatorItem(findOperatorItem, window->contentItem(), "nodeOperatormultiply");
            if (!multiplyOperator || !multiplyOperator->isVisible())
                throw std::runtime_error("Multiply operator is unavailable.");
            const auto operatorMultiplyPoint = multiplyOperator->mapToScene(
                QPointF(multiplyOperator->width() / 2, multiplyOperator->height() / 2));
            movePoint(QEvent::MouseButtonPress, operatorMultiplyPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, operatorMultiplyPoint, Qt::NoButton);
            if (editor.document().layer(red).blendMode != opentoon::LayerBlendMode::Multiply)
                throw std::runtime_error("Operator library did not apply Multiply.");
            editor.undo();
            if (editor.document().layer(red).blendMode != opentoon::LayerBlendMode::Normal)
                throw std::runtime_error("Operator library blend edit did not undo.");
            const auto beforeOperatorAdd = editor.document().layers.size();
            movePoint(QEvent::MouseButtonPress, operatorPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, operatorPoint, Qt::NoButton);
            operatorSearch = window->findChild<QQuickItem*>("nodeOperatorSearch");
            operatorSearch->setProperty("text", "Source");
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            if (nodes->property("matchingOperators").toList().size() != 1)
                throw std::runtime_error("Operator source category search is incorrect.");
            auto* drawingOperator =
                findOperatorItem(findOperatorItem, window->contentItem(), "nodeOperatordrawing");
            if (!drawingOperator || !drawingOperator->isVisible())
                throw std::runtime_error("Drawing source operator is unavailable.");
            const auto drawingOperatorPoint = drawingOperator->mapToScene(
                QPointF(drawingOperator->width() / 2, drawingOperator->height() / 2));
            movePoint(QEvent::MouseButtonPress, drawingOperatorPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, drawingOperatorPoint, Qt::NoButton);
            if (editor.document().layers.size() != beforeOperatorAdd + 1)
                throw std::runtime_error("Drawing operator did not add a layer.");
            editor.undo();
            if (editor.document().layers.size() != beforeOperatorAdd)
                throw std::runtime_error("Drawing operator did not undo atomically.");
            editor.setSelectedLayer(int(blue));
            if (!editor.setLayerMatte(int(red)))
                throw std::runtime_error("Cannot prepare referenced source deletion.");
            const auto beforeSourceDelete = editor.document();
            strip->setProperty("contentX", 0);
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            redCard = findDrawingCard(int(red));
            if (!redCard || !redCard->isVisible())
                throw std::runtime_error("Referenced Drawing source card is unavailable.");
            const auto deleteCardPoint = redCard->mapToScene(QPointF(redCard->width() / 2, 30));
            movePoint(QEvent::MouseButtonPress, deleteCardPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, deleteCardPoint, Qt::NoButton);
            auto* deleteSource = window->findChild<QQuickItem*>("nodeDeleteSource");
            if (!deleteSource || !deleteSource->isVisible())
                throw std::runtime_error("Delete source choice is unavailable.");
            const auto deleteSourcePoint =
                deleteSource->mapToScene(QPointF(deleteSource->width() / 2, deleteSource->height() / 2));
            movePoint(QEvent::MouseButtonPress, deleteSourcePoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, deleteSourcePoint, Qt::NoButton);
            auto* protectReferences = window->findChild<QQuickItem*>("nodeDeleteProtect");
            auto* disconnectReferences = window->findChild<QQuickItem*>("nodeDeleteDisconnect");
            if (!protectReferences || !disconnectReferences || !protectReferences->isVisible() ||
                !disconnectReferences->isVisible())
                throw std::runtime_error("Delete source policies are unavailable.");
            const auto protectPoint = protectReferences->mapToScene(
                QPointF(protectReferences->width() / 2, protectReferences->height() / 2));
            movePoint(QEvent::MouseButtonPress, protectPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, protectPoint, Qt::NoButton);
            if (editor.document() != beforeSourceDelete)
                throw std::runtime_error("Protect references changed a used source.");
            const auto disconnectPoint = disconnectReferences->mapToScene(
                QPointF(disconnectReferences->width() / 2, disconnectReferences->height() / 2));
            movePoint(QEvent::MouseButtonPress, disconnectPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, disconnectPoint, Qt::NoButton);
            if (editor.document().layers.size() != beforeSourceDelete.layers.size() - 1 ||
                editor.document().layer(blue).matte != 0)
                throw std::runtime_error("Disconnect and delete left a partial cutter graph.");
            editor.undo();
            if (editor.document() != beforeSourceDelete)
                throw std::runtime_error("Disconnect and delete did not undo atomically.");
            editor.redo();
            if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(behindPath)) ||
                editor.document().layer(blue).matte != 0)
                throw std::runtime_error("Disconnected source deletion changed after reopen.");
            auto joint = opentoon::makeDocument();
            joint.width = joint.height = 1;
            joint.background = {0, 0, 0, 0};
            const auto torsoPart = joint.layers.front().id;
            joint.editableDrawing(torsoPart, 0).image = opentoon::ImageAsset{1, 1, {255, 0, 0, 255}};
            auto armPart = joint.layers.front();
            armPart.id = joint.allocateId();
            armPart.name = "Arm";
            auto armArt = joint.drawings.at(armPart.exposures.front().drawing);
            armArt.id = joint.allocateId();
            armArt.image = opentoon::ImageAsset{1, 1, {0, 0, 255, 255}};
            joint.drawings.emplace(armArt.id, armArt);
            armPart.exposures.front().drawing = armArt.id;
            const auto armPartId = armPart.id;
            const auto character = opentoon::makeCharacter(joint, torsoPart, "Actor");
            joint.layers.insert(joint.layers.begin(), armPart);
            opentoon::attachDrawingAsPart(joint, armPartId, character, "Arm");
            joint.validate();
            const auto jointPath = directory.filePath("joint-patch.otoon");
            if (!opentoon::ProjectStore::save(std::filesystem::path(jointPath.toStdString()), joint))
                throw std::runtime_error("Cannot save joint patch fixture.");
            if (!editor.openProject(QUrl::fromLocalFile(jointPath)))
                throw std::runtime_error("Cannot open joint patch fixture.");
            editor.setSelectedLayer(int(torsoPart));
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            auto* jointPicker = window->findChild<QQuickItem*>("nodeJointPatchSource");
            auto* jointButton = window->findChild<QQuickItem*>("nodeCreateJointPatch");
            if (!jointPicker || !jointButton || !jointPicker->isVisible() || !jointButton->isVisible())
                throw std::runtime_error("Joint patch recipe is unavailable in Nodes.");
            jointPicker->setProperty("currentIndex", 1);
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            if (!jointButton->isEnabled())
                throw std::runtime_error("Joint patch source cannot be selected.");
            const auto jointButtonPoint =
                jointButton->mapToScene(QPointF(jointButton->width() / 2, jointButton->height() / 2));
            movePoint(QEvent::MouseButtonPress, jointButtonPoint, Qt::LeftButton);
            movePoint(QEvent::MouseButtonRelease, jointButtonPoint, Qt::NoButton);
            const auto jointPatchId = opentoon::Id(editor.selectedLayer());
            if (jointPatchId == torsoPart || editor.document().layer(jointPatchId).role != "Arm patch" ||
                qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Joint patch click did not cover the torso seam.");
            editor.undo();
            if (qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Joint patch undo did not restore the torso.");
            editor.redo();
            if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(jointPath)) ||
                qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Joint patch changed after save and reopen.");
            const auto gesturePath = directory.filePath("joint-patch-gesture.otoon");
            if (!opentoon::ProjectStore::save(std::filesystem::path(gesturePath.toStdString()), joint) ||
                !editor.openProject(QUrl::fromLocalFile(gesturePath)))
                throw std::runtime_error("Cannot open direct joint patch fixture.");
            strip->setProperty("contentX", 0);
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            auto* armJointCard = findDrawingCard(int(armPartId));
            auto* torsoJointCard = findDrawingCard(int(torsoPart));
            if (!armJointCard || !torsoJointCard || !armJointCard->isVisible() ||
                !torsoJointCard->isVisible())
                throw std::runtime_error("Part cards are unavailable for patch drag.");
            const auto patchFrom = armJointCard->mapToScene(QPointF(armJointCard->width() / 2, 30));
            const auto patchTo = torsoJointCard->mapToScene(QPointF(torsoJointCard->width() / 2, 30));
            const auto patchMove = [&](QEvent::Type type, QPointF point, Qt::MouseButtons buttons) {
                QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()),
                                  type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton, buttons,
                                  Qt::AltModifier | Qt::ControlModifier);
                QCoreApplication::sendEvent(window, &event);
                QCoreApplication::processEvents();
            };
            patchMove(QEvent::MouseButtonPress, patchFrom, Qt::LeftButton);
            patchMove(QEvent::MouseMove, patchFrom + QPointF(16, 0), Qt::LeftButton);
            patchMove(QEvent::MouseMove, patchTo, Qt::LeftButton);
            patchMove(QEvent::MouseButtonRelease, patchTo, Qt::NoButton);
            const auto gesturePatch = opentoon::Id(editor.selectedLayer());
            if (gesturePatch == torsoPart || editor.document().layer(gesturePatch).role != "Arm patch" ||
                qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Control+Alt-drag did not create an editable patch.");
            editor.undo();
            if (qRed(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Direct joint patch did not undo atomically.");
            editor.redo();
            if (!editor.saveProject({}) || !editor.openProject(QUrl::fromLocalFile(gesturePath)) ||
                qBlue(opentoon::SceneRenderer::render(editor.document(), 0).pixel(0, 0)) != 255)
                throw std::runtime_error("Direct joint patch changed after reopen.");
            std::cout << "HM-12 native smoke passed: inspector, clickable node preview, opacity bypass, "
                         "fractional cutter, "
                         "painted-source Multiply/Add, blend and composite bypass, alternate Display/Write, "
                         "front/behind drawing drag, group order, member edits, bypass, duplicate and "
                         "ungroup ports, Alt-drag cutter, Alt+Shift-drag private cutter, editable joint "
                         "patch and direct Control+Alt-drag, Alt-click bypass, operator library "
                         "search/apply, explicit source deletion, typed node search, save/reopen and undo.\n";
        });
    }
}
