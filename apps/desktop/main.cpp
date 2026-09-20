#include "canvas_item.h"
#include "editor_controller.h"
#include "project_store.h"
#include "scene_renderer.h"
#include "serialization.h"
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTabletEvent>
#include <QTemporaryDir>
#include <QTimer>
#include <cmath>
#include <iostream>
#include <stdexcept>
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
    if (args.contains("--smoke-test")) {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName("OPEN-TOON-smoke");
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
        if (args.contains("--smoke-test")) {
            QTimer::singleShot(1200, &app, [&] {
                try {
                    if (engine.rootObjects().isEmpty())
                        throw std::runtime_error("No QML window.");
                    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
                    auto* canvas = window->findChild<CanvasItem*>("drawingCanvas");
                    if (!canvas || canvas->height() < 200)
                        throw std::runtime_error("Canvas layout is not usable.");
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
                    editor.setTool("Raster ink");
                    QTimer::singleShot(250, &app, [&, window] {
                        auto image = window->grabWindow();
                        if (image.isNull() || !image.save("build/ui-smoke.png")) {
                            app.exit(1);
                            return;
                        }
                        std::cout << "UI smoke passed: mouse stroke, synthetic pen pressure, cancelled "
                                     "gesture, raster painting and pixel round trip, range clipboard, "
                                     "undo/redo, save/reopen, canvas layout "
                                     "and screenshot.\n";
                        app.exit(0);
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
