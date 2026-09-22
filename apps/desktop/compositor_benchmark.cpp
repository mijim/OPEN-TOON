#include "opentoon/document.h"
#include "revision_render_cache.h"
#include "scene_renderer.h"
#include <QGuiApplication>
#include <chrono>
#include <iostream>

using namespace opentoon;
int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    auto document = makeDocument();
    document.width = 1920;
    document.height = 1080;
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
    std::cout << "scene=1920x1080, layers=19, frames=3, fixture=synthetic-rgba\n"
              << "legacy_ms_per_frame=" << legacy << '\n'
              << "linear_ms_per_frame=" << linear << '\n'
              << "cached_lookup_us="
              << std::chrono::duration<double, std::micro>(cached).count() / 3000 << '\n'
              << "cache_retained_bytes=" << cache.retainedBytes() << '\n'
              << "checksum=" << checksum << '\n';
}
