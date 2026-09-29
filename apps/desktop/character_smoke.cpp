#include "canvas_item.h"
#include "desktop_smoke.h"
#include "editor_controller.h"
#include "native_smoke_support.h"
#include "project_store.h"
#include "scene_renderer.h"
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QUrl>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

void scheduleMiloSmoke(const QStringList& args, QGuiApplication& app, EditorController& editor,
                       QQmlApplicationEngine& engine) {
    if (args.contains("--milo-smoke")) {
        scheduleNativeSmoke(app, [&] {
            if (!args.contains("--open") || engine.rootObjects().isEmpty())
                throw std::runtime_error("Milo smoke requires --open PROJECT and a QML window.");
            auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            auto* canvas = window ? window->findChild<CanvasItem*>("drawingCanvas") : nullptr;
            if (!canvas || canvas->height() < 200)
                throw std::runtime_error("Milo canvas is unavailable.");
            const auto original = editor.document();
            if (original.name != "Milo — continuous rig study" || original.layers.size() != 18 ||
                original.duration != 48)
                throw std::runtime_error("Milo project identity or part count changed.");
            const auto deformed = std::count_if(
                original.layers.begin(), original.layers.end(), [](const opentoon::Layer& layer) {
                    return std::any_of(
                        layer.bindings.begin(), layer.bindings.end(),
                        [](const opentoon::MeshBinding& binding) { return binding.bone.has_value(); });
                });
            if (deformed != 4)
                throw std::runtime_error("Milo's four continuous limb bones are missing.");
            const auto rest = opentoon::SceneRenderer::render(original, 0);
            const auto bent = opentoon::SceneRenderer::render(original, 24);
            if (rest == bent)
                throw std::runtime_error("Milo's elbow and knee pose did not render.");
            editor.setWorkspaceMode("Rig");
            editor.setFrame(24);
            QCoreApplication::processEvents();
            window->update();
            QCoreApplication::processEvents();
            const auto screenshot = window->grabWindow();
            if (screenshot.isNull())
                throw std::runtime_error("Milo's native canvas did not present.");
            QTemporaryDir temporary;
            if (!temporary.isValid() ||
                !editor.saveProject(QUrl::fromLocalFile(temporary.filePath("milo.otoon"))))
                throw std::runtime_error("Milo's native project save failed.");
            editor.newScene();
            if (!editor.openProject(QUrl::fromLocalFile(temporary.filePath("milo.otoon"))) ||
                editor.document() != original ||
                opentoon::SceneRenderer::render(editor.document(), 24) != bent)
                throw std::runtime_error("Milo's native reopen changed the bent pose.");
            std::cout
                << "Milo native smoke passed: 17 Parts, four continuous bones, frame 24, save/reopen.\n";
        });
    }
}
void scheduleIntegratedSmoke(const QStringList& args, QGuiApplication& app, EditorController& editor,
                             QQmlApplicationEngine& engine) {
    if (args.contains("--hm-integrated-smoke")) {
        scheduleNativeSmoke(app, [&] {
            if (engine.rootObjects().isEmpty())
                throw std::runtime_error("No QML window for integrated shot smoke.");
            auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            if (!window || !window->findChild<QQuickItem*>("timelineCanvas"))
                throw std::runtime_error("The integrated shot has no native timeline.");
            const int openIndex = args.indexOf("--open");
            if (openIndex < 0 || openIndex + 1 >= args.size())
                throw std::runtime_error("Integrated smoke requires --open PROJECT.");
            const auto expected =
                opentoon::ProjectStore::load(std::filesystem::path(args[openIndex + 1].toStdString()))
                    .document;
            if (editor.document() != expected || editor.audioClips().size() != 1)
                throw std::runtime_error("The integrated project did not open intact in Qt Quick.");
            const auto initial = opentoon::SceneRenderer::render(editor.document(), 0, QSize(480, 270));
            editor.setFrame(264);
            QCoreApplication::processEvents();
            if (editor.frame() != 264 || opentoon::SceneRenderer::render(editor.document(), editor.frame(),
                                                                         QSize(480, 270)) == initial)
                throw std::runtime_error("The integrated shot did not advance visually.");
            if (!window->grabWindow().save("build/hm-integrated-shot-smoke.png"))
                throw std::runtime_error("Cannot capture the integrated shot window.");
            std::cout << "Integrated shot smoke passed: project reopen, native timeline, "
                         "audio track, visual frame change and Qt Quick screenshot.\n";
        });
    }
}
