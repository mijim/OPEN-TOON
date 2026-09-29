#include "desktop_smoke.h"
#include "editor_controller.h"
#include "native_smoke_support.h"
#include <QMouseEvent>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <cmath>
#include <iostream>
#include <stdexcept>

void scheduleWorkspaceSmoke(const QStringList& args, QGuiApplication& app, EditorController& editor,
                            QQmlApplicationEngine& engine) {
    if (args.contains("--workspace-smoke")) {
        scheduleNativeSmoke(app, [&] {
            if (engine.rootObjects().isEmpty())
                throw std::runtime_error("Workspace smoke has no QML window.");
            auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            if (!window)
                throw std::runtime_error("Workspace smoke has no native window.");
            editor.newScene();
            editor.setFrame(7);
            const auto before = editor.document();
            const auto revision = editor.documentRevision();
            const auto layer = editor.selectedLayer();
            QCoreApplication::processEvents();
            (void)window->grabWindow();
            const auto click = [&](const char* name) {
                auto* item = window->findChild<QQuickItem*>(name);
                if (!item || !item->isVisible() || !item->isEnabled())
                    throw std::runtime_error(std::string("Workspace control unavailable: ") + name);
                const auto point = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
                QMouseEvent press(QEvent::MouseButtonPress, point, window->mapToGlobal(point.toPoint()),
                                  Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QMouseEvent release(QEvent::MouseButtonRelease, point, window->mapToGlobal(point.toPoint()),
                                    Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                QCoreApplication::sendEvent(window, &press);
                QCoreApplication::sendEvent(window, &release);
                QCoreApplication::processEvents();
            };
            click("timelineZoomIn");
            click("bottomTimingToolsTab");
            if (editor.timelineCellWidth() != 32 || !editor.timingToolsVisible())
                throw std::runtime_error("Timeline preferences did not follow native clicks.");
            auto* splitter = window->findChild<QQuickItem*>("workspaceResizeHandle");
            if (!splitter || !splitter->isVisible())
                throw std::runtime_error("Workspace resize handle is unavailable.");
            const auto start = splitter->mapToScene(QPointF(splitter->width() / 2, splitter->height() / 2));
            const auto end = start + QPointF(0, -48);
            const auto drag = [&](QEvent::Type type, QPointF point, Qt::MouseButtons buttons) {
                QMouseEvent event(type, point, window->mapToGlobal(point.toPoint()),
                                  type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton, buttons,
                                  Qt::NoModifier);
                QCoreApplication::sendEvent(window, &event);
                QCoreApplication::processEvents();
            };
            drag(QEvent::MouseButtonPress, start, Qt::LeftButton);
            drag(QEvent::MouseMove, end, Qt::LeftButton);
            drag(QEvent::MouseButtonRelease, end, Qt::NoButton);
            if (editor.bottomPanelHeight() <= 280)
                throw std::runtime_error("Workspace separator drag was not saved.");
            auto* inspectorSplitter = window->findChild<QQuickItem*>("inspectorResizeHandle");
            if (!inspectorSplitter || !inspectorSplitter->isVisible())
                throw std::runtime_error("Inspector resize handle is unavailable.");
            const auto inspectorStart = inspectorSplitter->mapToScene(
                QPointF(inspectorSplitter->width() / 2, inspectorSplitter->height() / 2));
            const auto inspectorEnd = inspectorStart + QPointF(-64, 0);
            drag(QEvent::MouseButtonPress, inspectorStart, Qt::LeftButton);
            drag(QEvent::MouseMove, inspectorEnd, Qt::LeftButton);
            if (editor.inspectorWidth() != 250 || window->property("effectiveInspectorWidth").toInt() <= 250)
                throw std::runtime_error("Inspector resize preview was not live or saved too early.");
            drag(QEvent::MouseButtonRelease, inspectorEnd, Qt::NoButton);
            if (editor.inspectorWidth() <= 250 ||
                window->property("effectiveInspectorWidth").toInt() != editor.inspectorWidth())
                throw std::runtime_error("Inspector separator drag was not saved.");
            click("bottomNodesTab");
            if (editor.bottomPanelTab() != "Nodes" || !window->property("showNodes").toBool())
                throw std::runtime_error("Nodes workspace tab did not open.");
            editor.setWorkspaceMode("Animator");
            QSettings().sync();
            EditorController restored;
            if (restored.bottomPanelTab() != "Nodes" ||
                restored.bottomPanelHeight() != editor.bottomPanelHeight() ||
                restored.inspectorWidth() != editor.inspectorWidth() || restored.timelineCellWidth() != 32 ||
                !restored.timingToolsVisible() || restored.workspaceMode() != "Animator")
                throw std::runtime_error("Workspace layout changed after controller reopen.");
            editor.resetWorkspaceLayout();
            QCoreApplication::processEvents();
            EditorController reset;
            if (reset.bottomPanelTab() != "Timeline" || reset.bottomPanelHeight() != 280 ||
                reset.timelineCellWidth() != 22 || reset.inspectorWidth() != 250 ||
                reset.timingToolsVisible() || reset.workspaceMode() != "Rig" ||
                window->property("showNodes").toBool() ||
                window->property("effectiveBottomHeight").toInt() != 280 || editor.document() != before ||
                editor.documentRevision() != revision || editor.frame() != 7 ||
                editor.selectedLayer() != layer)
                throw std::runtime_error("Workspace reset changed the scene or left stale UI state.");
            auto* presetPicker = window->findChild<QQuickItem*>("workspaceLayoutPicker");
            if (!presetPicker || !presetPicker->isVisible() || !editor.applyWorkspacePreset("Compositing"))
                throw std::runtime_error("Named workspace layout picker is unavailable.");
            QCoreApplication::processEvents();
            if (presetPicker->property("currentIndex").toInt() != 4 ||
                !window->property("showNodes").toBool() || editor.inspectorWidth() != 320 ||
                editor.document() != before || editor.documentRevision() != revision || editor.frame() != 7 ||
                editor.selectedLayer() != layer)
                throw std::runtime_error("Compositing layout changed document state or missed QML.");
            window->raise();
            window->requestActivate();
            QCoreApplication::processEvents();
            click("workspaceLayoutPicker");
            (void)window->grabWindow();
            auto* layoutPopup = presetPicker->property("popup").value<QObject*>();
            auto* popupContent =
                layoutPopup ? layoutPopup->property("contentItem").value<QQuickItem*>() : nullptr;
            QQuickItem* animationChoice = nullptr;
            const auto findChoice = [&](const auto& self, QQuickItem* parent) -> void {
                if (!parent || animationChoice)
                    return;
                if (parent->isVisible() && parent->property("text").toString() == "Animation" &&
                    parent->width() > 0 && parent->height() > 0) {
                    animationChoice = parent;
                    return;
                }
                for (auto* child : parent->childItems())
                    self(self, child);
            };
            findChoice(findChoice, popupContent);
            if (!animationChoice)
                throw std::runtime_error("Animation layout menu item is unavailable.");
            const auto choicePoint = animationChoice->mapToScene(
                QPointF(animationChoice->width() / 2, animationChoice->height() / 2));
            QMouseEvent choicePress(QEvent::MouseButtonPress, choicePoint,
                                    window->mapToGlobal(choicePoint.toPoint()), Qt::LeftButton,
                                    Qt::LeftButton, Qt::NoModifier);
            QMouseEvent choiceRelease(QEvent::MouseButtonRelease, choicePoint,
                                      window->mapToGlobal(choicePoint.toPoint()), Qt::LeftButton,
                                      Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(window, &choicePress);
            QCoreApplication::sendEvent(window, &choiceRelease);
            QCoreApplication::processEvents();
            if (editor.workspacePreset() != "Animation" || editor.bottomPanelTab() != "Curves" ||
                editor.document() != before || editor.frame() != 7)
                throw std::runtime_error("Native Animation layout choice did not apply.");
            editor.setBottomPanelHeight(470);
            editor.setInspectorWidth(410);
            if (editor.workspacePreset() != "Custom" || !editor.saveWorkspacePreset("Drawing") ||
                !editor.applyWorkspacePreset("Animation") || !editor.applyWorkspacePreset("Drawing") ||
                editor.bottomPanelHeight() != 470 || editor.inspectorWidth() != 410)
                throw std::runtime_error("Named workspace layout did not retain its edited height.");
            const auto layoutShot = window->grabWindow();
            if (layoutShot.isNull() || !layoutShot.save("build/workspace-layout-smoke.png"))
                throw std::runtime_error("Named workspace layout screenshot failed.");
            std::cout << "Workspace native smoke passed: tab, timeline zoom, timing tools, "
                         "both splitter drags, named layouts, preferences reopen and reset "
                         "without document changes.\n";
        });
    }
}
