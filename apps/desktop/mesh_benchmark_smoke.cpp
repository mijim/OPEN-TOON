#include "canvas_item.h"
#include "desktop_smoke.h"
#include "editor_controller.h"
#include "mesh_smoke.h"
#include "native_smoke_support.h"
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <stdexcept>

void scheduleMeshBenchmark(const QStringList& args, QGuiApplication& app, EditorController& editor,
                           QQmlApplicationEngine& engine) {
    if (args.contains("--hm06-benchmark")) {
        const int index = args.indexOf("--hm06-benchmark");
        if (index + 1 >= args.size())
            throw std::runtime_error("Usage: open-toon --hm06-benchmark PROJECT");
        const QString project = args[index + 1];
        scheduleNativeSmoke(app, [&, project] {
            if (engine.rootObjects().isEmpty())
                throw std::runtime_error("No QML window for HM-06 benchmark.");
            auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            auto* canvas = window->findChild<CanvasItem*>("drawingCanvas");
            if (!canvas || canvas->height() < 200)
                throw std::runtime_error("HM-06 benchmark canvas is not usable.");
            meshInteractionBenchmark(editor, *canvas, *window, project);
        });
    }
}
