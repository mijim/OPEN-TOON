#include "opentoon/session.h"
#include "project_store.h"
#include "raster_brush.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace opentoon;
int main(int argc, char** argv) {
    try {
        if (argc != 2)
            throw std::invalid_argument("Usage: opentoon_brush_benchmark NEW_PROJECT_PATH");
        const std::filesystem::path path(argv[1]);
        if (std::filesystem::exists(path))
            throw std::invalid_argument("Benchmark output already exists.");
        auto d = makeDocument();
        d.width = 3840;
        d.height = 2160;
        auto& drawing = d.editableDrawing(d.layers.front().id, 0);
        drawing.raster = RasterImage{3840, 2160, {}};
        std::vector<double> samples;
        auto start = std::chrono::steady_clock::now();
        for (int gesture = 0; gesture < 24; ++gesture) {
            RasterBrush brush(*drawing.raster, {48, 1, {0.1, 0.2, 0.5, 1}, BrushPreset(gesture % 4)});
            for (int sample = 0; sample < 480; ++sample) {
                auto before = std::chrono::steady_clock::now();
                brush.sample({100 + sample * 7.0, 100 + gesture * 80 + std::sin(sample / 24.0) * 50,
                              0.3 + 0.7 * sample / 479.0});
                drawing.raster = brush.snapshot();
                samples.push_back(
                    std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - before)
                        .count());
            }
        }
        d.validate();
        auto paintMs =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        start = std::chrono::steady_clock::now();
        auto head = ProjectStore::save(path, d);
        for (int revision = 0; revision < 10; ++revision) {
            d.name = "4K brush benchmark revision " + std::to_string(revision);
            head = ProjectStore::save(path, d, "Metadata edit", head);
        }
        auto saveMs =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        start = std::chrono::steady_clock::now();
        if (ProjectStore::load(path).document != d)
            throw std::runtime_error("Round trip mismatch.");
        auto loadMs =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        std::sort(samples.begin(), samples.end());
        std::cout << "{\"samples\":" << samples.size() << ",\"paint_ms\":" << paintMs
                  << ",\"sample_p50_ms\":" << samples[samples.size() / 2]
                  << ",\"sample_p95_ms\":" << samples[samples.size() * 95 / 100]
                  << ",\"sample_max_ms\":" << samples.back() << ",\"tiles\":" << drawing.raster->tiles.size()
                  << ",\"eleven_saves_ms\":" << saveMs << ",\"load_ms\":" << loadMs
                  << ",\"file_bytes\":" << std::filesystem::file_size(path) << "}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
