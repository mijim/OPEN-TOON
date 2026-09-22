#include "opentoon/document.h"
#include "revision_render_cache.h"
#include "scene_renderer.h"
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <chrono>
#include <iostream>
#include <stdexcept>

using namespace opentoon;
namespace {
Document originalArt() {
    const QString base = QStringLiteral(OPENTOON_SOURCE_DIR "/tests/fixtures/harmony-moment/");
    QFile file(base + "shot.json");
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Cannot open original-art scene specification.");
    const auto spec = QJsonDocument::fromJson(file.readAll()).object();
    Document document;
    document.name = "Clockwork Hello benchmark";
    document.duration = 480;
    document.background = {0, 0, 0, 0};
    Layer root;
    root.id = document.allocateId();
    root.name = "Character root";
    document.layers.push_back(root);
    const auto centers = spec.value("reference_centers_px").toObject();
    for (const auto& entry : spec.value("reference_paint_order").toArray()) {
        const auto role = entry.toString();
        const QString variant = role == "mouth" ? "front__rest" :
                                (role == "head" || role == "hair" || role == "eyes") ? "front" :
                                role.startsWith("hand_") ? "open" : "base";
        QImage image(base + "parts/" + role + "__" + variant + ".png");
        if (image.isNull())
            throw std::runtime_error("Cannot load original-art character part.");
        image = image.convertToFormat(QImage::Format_RGBA8888);
        std::vector<std::uint8_t> bytes;
        bytes.reserve(std::size_t(image.width()) * image.height() * 4);
        for (int y = 0; y < image.height(); ++y) {
            const auto* row = image.constScanLine(y);
            bytes.insert(bytes.end(), row, row + image.width() * 4);
        }
        Drawing drawing;
        drawing.id = document.allocateId();
        drawing.image = ImageAsset{image.width(), image.height(), std::move(bytes)};
        const auto drawingId = drawing.id;
        document.drawings.emplace(drawingId, std::move(drawing));
        Layer layer;
        layer.id = document.allocateId();
        layer.name = role.toStdString();
        layer.parent = root.id;
        const auto center = centers.value(role).toArray();
        layer.transform.x = center.at(0).toDouble() - image.width() / 2.0;
        layer.transform.y = center.at(1).toDouble() - image.height() / 2.0;
        layer.exposures.push_back({0, document.duration, drawingId});
        document.layers.push_back(std::move(layer));
    }
    document.validate();
    return document;
}
} // namespace
int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    const bool original = app.arguments().contains("--original-art");
    auto document = original ? originalArt() : makeDocument();
    document.width = 1920;
    document.height = 1080;
    if (!original) {
        auto& drawing = document.editableDrawing(document.layers.front().id, 0);
        std::vector<std::uint8_t> pixels(256 * 256 * 4);
        for (int y = 0; y < 256; ++y)
            for (int x = 0; x < 256; ++x) {
                const auto i = std::size_t((y * 256 + x) * 4);
                pixels[i] = std::uint8_t((x * 17 + y * 7) % 256);
                pixels[i + 1] = std::uint8_t((x * 3 + y * 19) % 256);
                pixels[i + 2] = std::uint8_t((x * 11 + y * 5) % 256);
                pixels[i + 3] = std::uint8_t((x + y) % 3 == 0 ? 0 : 160);
            }
        drawing.image = ImageAsset{256, 256, std::move(pixels)};
        auto base = document.layers.front();
        for (int i = 1; i < 19; ++i) {
            auto layer = base;
            layer.id = document.allocateId();
            layer.name = "Synthetic part " + std::to_string(i);
            layer.transform.x = (i % 5) * 210;
            layer.transform.y = (i / 5) * 180;
            document.layers.push_back(std::move(layer));
        }
    }
    document.validate();
    auto measure = [&](CompositionProfile profile) {
        document.composition = profile;
        (void)SceneRenderer::render(document, 0); // Warm renderer and color tables.
        const auto start = std::chrono::steady_clock::now();
        for (int frame = 0; frame < 3; ++frame)
            (void)SceneRenderer::render(document, frame);
        const auto elapsed = std::chrono::steady_clock::now() - start;
        return std::chrono::duration<double, std::milli>(elapsed).count() / 3;
    };
    const auto legacy = measure(CompositionProfile::LegacyQt);
    const auto linear = measure(CompositionProfile::LinearSrgb);
    document.composition = CompositionProfile::LinearSrgb;
    RevisionRenderCache cache;
    RenderCacheKey key{1, 1, 0, document.width, document.height,
                       CompositionProfile::LinearSrgb};
    (void)cache.resolve(key, [&] { return SceneRenderer::render(document, 0); });
    const auto start = std::chrono::steady_clock::now();
    std::uint64_t checksum = 0;
    for (int i = 0; i < 3000; ++i) {
        auto image = cache.resolve(key, [&] { return SceneRenderer::render(document, 0); });
        checksum += qAlpha(image.pixel(i % image.width(), i % image.height()));
    }
    const auto cached = std::chrono::steady_clock::now() - start;
    std::cout << "scene=1920x1080, layers=" << document.layers.size()
              << ", frames=3, fixture=" << (original ? "original-character-art" : "synthetic-rgba") << '\n'
              << "legacy_ms_per_frame=" << legacy << '\n'
              << "linear_ms_per_frame=" << linear << '\n'
              << "cached_lookup_us="
              << std::chrono::duration<double, std::micro>(cached).count() / 3000 << '\n'
              << "cache_retained_bytes=" << cache.retainedBytes() << '\n'
              << "checksum=" << checksum << '\n';
}
