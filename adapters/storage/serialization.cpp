#include "serialization.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace opentoon {
using Json = nlohmann::json;
namespace {
Json transform(const Transform& t) {
    return {t.x, t.y, t.rotation, t.scaleX, t.scaleY, t.opacity, t.pivotX, t.pivotY};
}
Transform readTransform(const Json& j) {
    if (!j.is_array() || j.size() != 8)
        throw std::runtime_error("Invalid transform encoding.");
    return {j.at(0), j.at(1), j.at(2), j.at(3), j.at(4), j.at(5), j.at(6), j.at(7)};
}
Json color(Color c) {
    return {c.r, c.g, c.b, c.a};
}
Color readColor(const Json& j) {
    if (!j.is_array() || j.size() != 4)
        throw std::runtime_error("Invalid color encoding.");
    return {j.at(0), j.at(1), j.at(2), j.at(3)};
}
void limit(const Json& j, std::size_t max) {
    if (!j.is_array() || j.size() > max)
        throw std::runtime_error("Invalid array or resource limit exceeded.");
}
} // namespace
std::string serializeDocument(const Document& d, ResourceWriter write) {
    d.validate();
    Json j = {{"format", "OPEN-TOON"},
              {"version", Document::formatVersion},
              {"name", d.name},
              {"width", d.width},
              {"height", d.height},
              {"duration", d.duration},
              {"rate", {d.rate.numerator, d.rate.denominator}},
              {"background", color(d.background)},
              {"nextId", d.nextId},
              {"layers", Json::array()},
              {"drawings", Json::array()},
              {"palette", Json::array()},
              {"markers", Json::array()}};
    for (auto s : d.palette)
        j["palette"].push_back({{"id", s.id}, {"name", s.name}, {"color", color(s.color)}});
    for (const auto& [id, drawing] : d.drawings) {
        Json x = {{"id", id}, {"name", drawing.name}, {"strokes", Json::array()}};
        for (const auto& s : drawing.strokes) {
            Json points = Json::array();
            for (auto p : s.points)
                points.push_back({p.x, p.y, p.pressure});
            x["strokes"].push_back({{"id", s.id},
                                    {"swatch", s.swatch},
                                    {"width", s.width},
                                    {"shape", static_cast<int>(s.shape)},
                                    {"filled", s.filled},
                                    {"artLayer", s.artLayer},
                                    {"points", points}});
        }
        if (drawing.image) {
            Json im = {{"width", drawing.image->width}, {"height", drawing.image->height}};
            if (write)
                im["resource"] = write({drawing.image->rgba.data(), drawing.image->rgba.size()});
            else
                im["rgba"] = drawing.image->rgba.values();
            x["image"] = std::move(im);
        }
        if (drawing.raster) {
            Json raster = {{"width", drawing.raster->width},
                           {"height", drawing.raster->height},
                           {"tiles", Json::array()}};
            for (const auto& [position, tile] : drawing.raster->tiles) {
                std::vector<std::uint8_t> bytes;
                bytes.reserve(tile.size() * 2);
                for (auto value : tile) {
                    bytes.push_back(value & 255);
                    bytes.push_back(value >> 8);
                }
                Json t = {{"x", position.first}, {"y", position.second}};
                if (write)
                    t["resource"] = write(bytes);
                else
                    t["bytes"] = std::move(bytes);
                raster["tiles"].push_back(std::move(t));
            }
            x["raster"] = std::move(raster);
        }
        j["drawings"].push_back(std::move(x));
    }
    for (const auto& l : d.layers) {
        Json x = {{"id", l.id},
                  {"name", l.name},
                  {"visible", l.visible},
                  {"locked", l.locked},
                  {"solo", l.solo},
                  {"parent", l.parent},
                  {"kind", static_cast<int>(l.kind)},
                  {"role", l.role},
                  {"variants", Json::array()},
                  {"views", Json::array()},
                  {"transform", transform(l.transform)},
                  {"exposures", Json::array()},
                  {"keys", Json::array()}};
        for (auto e : l.exposures)
            x["exposures"].push_back({e.start, e.end, e.drawing});
        for (const auto& variant : l.variants)
            x["variants"].push_back({{"drawing", variant.drawing}, {"name", variant.name}});
        for (const auto& view : l.views) {
            Json choices = Json::array();
            for (const auto& choice : view.choices)
                choices.push_back({choice.part, choice.drawing});
            x["views"].push_back({{"id", view.id}, {"name", view.name}, {"choices", choices}});
        }
        for (const auto& k : l.keys) {
            Json ease = Json::object();
            for (const auto& [channel, e] : k.easing)
                ease[channel] = {e.x1, e.y1, e.x2, e.y2};
            x["keys"].push_back({{"frame", k.frame},
                                 {"value", transform(k.value)},
                                 {"interpolation", static_cast<int>(k.interpolation)},
                                 {"easing", ease}});
        }
        j["layers"].push_back(std::move(x));
    }
    for (const auto& m : d.markers)
        j["markers"].push_back({m.frame, m.name});
    return j.dump();
}
Document deserializeDocument(const std::string& text, ResourceReader read) {
    if (text.size() > 64 * 1024 * 1024)
        throw std::runtime_error("Project revision exceeds the 64 MiB prototype limit.");
    auto callback = [](int depth, Json::parse_event_t, const Json&) {
        if (depth > 32)
            throw std::runtime_error("Project nesting exceeds the supported limit.");
        return true;
    };
    const auto j = Json::parse(text, callback);
    if (j.at("format") != "OPEN-TOON" ||
        (j.at("version").get<int>() < 1 || j.at("version").get<int>() > Document::formatVersion))
        throw std::runtime_error(
            "Unsupported project format version. The original file has not been changed.");
    Document d;
    d.name = j.at("name");
    d.width = j.at("width");
    d.height = j.at("height");
    d.duration = j.at("duration");
    d.rate = {j.at("rate").at(0), j.at("rate").at(1)};
    d.nextId = j.at("nextId");
    d.background = readColor(j.at("background"));
    limit(j.at("palette"), 65536);
    limit(j.at("drawings"), 50000);
    limit(j.at("layers"), 2000);
    limit(j.at("markers"), 1000000);
    for (const auto& s : j.at("palette"))
        d.palette.push_back({s.at("id"), s.at("name"), readColor(s.at("color"))});
    std::size_t points = 0, pixels = 0;
    for (const auto& x : j.at("drawings")) {
        Drawing drawing;
        drawing.id = x.at("id");
        drawing.name = x.at("name");
        limit(x.at("strokes"), 2000000);
        for (const auto& s : x.at("strokes")) {
            Stroke stroke;
            stroke.id = s.at("id");
            stroke.swatch = s.at("swatch");
            stroke.width = s.at("width");
            stroke.shape = static_cast<Shape>(s.at("shape").get<int>());
            stroke.filled = s.at("filled");
            stroke.artLayer = s.at("artLayer");
            limit(s.at("points"), 2000000);
            points += s.at("points").size();
            if (points > 2000000)
                throw std::runtime_error("Project contains too many points.");
            for (const auto& p : s.at("points")) {
                if (p.size() != 3)
                    throw std::runtime_error("Invalid stroke point.");
                stroke.points.push_back({p.at(0), p.at(1), p.at(2)});
            }
            drawing.strokes.push_back(std::move(stroke));
        }
        if (x.contains("image")) {
            const auto& im = x.at("image");
            std::vector<std::uint8_t> bytes;
            if (im.contains("resource")) {
                if (!read)
                    throw std::runtime_error("An external image resource resolver is required.");
                bytes = read(im.at("resource"));
            } else
                bytes = im.at("rgba").get<std::vector<std::uint8_t>>();
            pixels += bytes.size();
            if (pixels > 512 * 1024 * 1024)
                throw std::runtime_error("Image budget exceeded.");
            drawing.image = ImageAsset{im.at("width"), im.at("height"), std::move(bytes)};
        }
        if (x.contains("raster")) {
            const auto& im = x.at("raster");
            RasterImage raster;
            raster.width = im.at("width");
            raster.height = im.at("height");
            limit(im.at("tiles"), 16384);
            for (const auto& tile : im.at("tiles")) {
                std::vector<std::uint8_t> bytes;
                if (tile.contains("resource")) {
                    if (!read)
                        throw std::runtime_error("An external tile resource resolver is required.");
                    bytes = read(tile.at("resource"));
                } else
                    bytes = tile.at("bytes").get<std::vector<std::uint8_t>>();
                pixels += bytes.size();
                if (bytes.size() != 64 * 64 * 4 * 2 || pixels > 512 * 1024 * 1024)
                    throw std::runtime_error("Invalid raster tile or memory budget exceeded.");
                std::vector<std::uint16_t> values;
                values.reserve(bytes.size() / 2);
                for (std::size_t i = 0; i < bytes.size(); i += 2) {
                    auto value = std::uint16_t(bytes[i] | (std::uint16_t(bytes[i + 1]) << 8));
                    if (value > 32768)
                        throw std::runtime_error("Raster channel exceeds 15-bit range.");
                    values.push_back(value);
                }
                for (std::size_t i = 0; i < values.size(); i += 4)
                    if (values[i] > values[i + 3] || values[i + 1] > values[i + 3] ||
                        values[i + 2] > values[i + 3])
                        throw std::runtime_error("Raster tile is not premultiplied.");
                if (!raster.tiles.emplace(std::pair<int, int>{tile.at("x"), tile.at("y")}, std::move(values))
                         .second)
                    throw std::runtime_error("Duplicate raster tile coordinate.");
            }
            drawing.raster = std::move(raster);
        }
        if (!d.drawings.emplace(drawing.id, std::move(drawing)).second)
            throw std::runtime_error("Duplicate drawing identity.");
    }
    for (const auto& x : j.at("layers")) {
        Layer l;
        l.id = x.at("id");
        l.name = x.at("name");
        l.visible = x.at("visible");
        l.locked = x.at("locked");
        l.solo = x.at("solo");
        l.parent = x.at("parent");
        if (j.at("version").get<int>() >= 4) {
            l.kind = static_cast<LayerKind>(x.at("kind").get<int>());
            l.role = x.at("role").get<std::string>();
            limit(x.at("variants"), 10000);
            for (const auto& variant : x.at("variants"))
                l.variants.push_back({variant.at("drawing"), variant.at("name")});
        }
        if (j.at("version").get<int>() >= 5) {
            limit(x.at("views"), 1000);
            for (const auto& view : x.at("views")) {
                CharacterView entry;
                entry.id = view.at("id");
                entry.name = view.at("name");
                limit(view.at("choices"), 2000);
                for (const auto& choice : view.at("choices"))
                    entry.choices.push_back({choice.at(0), choice.at(1)});
                l.views.push_back(std::move(entry));
            }
        }
        l.transform = readTransform(x.at("transform"));
        limit(x.at("exposures"), 1000000);
        limit(x.at("keys"), 1000000);
        for (const auto& e : x.at("exposures"))
            l.exposures.push_back({e.at(0), e.at(1), e.at(2)});
        for (const auto& k : x.at("keys")) {
            Keyframe key{k.at("frame"), readTransform(k.at("value")),
                         static_cast<Interpolation>(k.at("interpolation").get<int>())};
            if (k.contains("easing")) {
                const auto& ease = k.at("easing");
                if (!ease.is_object() || ease.size() > 8)
                    throw std::runtime_error("Invalid channel easing map.");
                for (auto it = ease.begin(); it != ease.end(); ++it) {
                    const auto& e = it.value();
                    if (!e.is_array() || e.size() != 4)
                        throw std::runtime_error("Invalid Bezier handles.");
                    key.easing[it.key()] = {e.at(0), e.at(1), e.at(2), e.at(3)};
                }
            }
            l.keys.push_back(std::move(key));
        }
        d.layers.push_back(std::move(l));
    }
    for (const auto& m : j.at("markers"))
        d.markers.push_back({m.at(0), m.at(1)});
    d.validate();
    return d;
}
} // namespace opentoon
