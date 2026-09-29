#include "canvas_item.h"
#include "desktop_smoke.h"
#include "editor_controller.h"
#include "project_store.h"
#include "scene_renderer.h"
#include "serialization.h"
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>
#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName("OPEN-TOON");
    QCoreApplication::setApplicationVersion(OPENTOON_VERSION);
    QCoreApplication::setOrganizationName("OPEN-TOON");
    QQuickStyle::setStyle("Basic");
    const auto args = app.arguments();
    std::optional<QTemporaryDir> smokeSettings;
    if (args.contains("--version")) {
        std::cout << OPENTOON_VERSION << '\n';
        return 0;
    }
    constexpr std::array smokeModes{
        std::pair{"--workspace-smoke", "OPEN-TOON-workspace-smoke"},
        std::pair{"--smoke-test", "OPEN-TOON-smoke"},
        std::pair{"--hm07-smoke", "OPEN-TOON-hm07-smoke"},
        std::pair{"--hm10-smoke", "OPEN-TOON-hm10-smoke"},
        std::pair{"--hm12-smoke", "OPEN-TOON-hm12-smoke"},
        std::pair{"--milo-smoke", "OPEN-TOON-milo-smoke"},
        std::pair{"--hm-integrated-smoke", "OPEN-TOON-integrated-smoke"},
        std::pair{"--hm06-benchmark", "OPEN-TOON-benchmark"},
    };
    const auto smokeMode = std::find_if(smokeModes.begin(), smokeModes.end(),
                                        [&args](const auto& mode) { return args.contains(mode.first); });
    if (smokeMode != smokeModes.end()) {
        smokeSettings.emplace();
        if (!smokeSettings->isValid())
            return 1;
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, smokeSettings->path());
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName(smokeMode->second);
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
        scheduleWorkspaceSmoke(args, app, editor, engine);
        scheduleMiloSmoke(args, app, editor, engine);
        scheduleIntegratedSmoke(args, app, editor, engine);
        scheduleCompositionSmoke(args, app, editor, engine);
        scheduleAudioSmoke(args, app, editor, engine);
        scheduleAnimatorSmoke(args, app, editor, engine);
        scheduleMeshBenchmark(args, app, editor, engine);
        scheduleEditorSmoke(args, app, editor, engine);
        return app.exec();
    } catch (const std::exception& e) {
        std::cerr << "OPEN-TOON: " << e.what() << '\n';
        return 1;
    }
}
