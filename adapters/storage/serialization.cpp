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
std::string serializeDocument(const Document& d) {
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
        if (drawing.image)
            x["image"] = {{"width", drawing.image->width},
                          {"height", drawing.image->height},
                          {"rgba", drawing.image->rgba}};
        j["drawings"].push_back(std::move(x));
    }
    for (const auto& l : d.layers) {
        Json x = {{"id", l.id},
                  {"name", l.name},
                  {"visible", l.visible},
                  {"locked", l.locked},
                  {"solo", l.solo},
                  {"parent", l.parent},
                  {"transform", transform(l.transform)},
                  {"exposures", Json::array()},
                  {"keys", Json::array()}};
        for (auto e : l.exposures)
            x["exposures"].push_back({e.start, e.end, e.drawing});
        for (auto k : l.keys)
            x["keys"].push_back({{"frame", k.frame},
                                 {"value", transform(k.value)},
                                 {"interpolation", static_cast<int>(k.interpolation)}});
        j["layers"].push_back(std::move(x));
    }
    for (const auto& m : d.markers)
        j["markers"].push_back({m.frame, m.name});
    return j.dump();
}
Document deserializeDocument(const std::string& text) {
    if (text.size() > 64 * 1024 * 1024)
        throw std::runtime_error("Project revision exceeds the 64 MiB prototype limit.");
    auto callback = [](int depth, Json::parse_event_t, const Json&) {
        if (depth > 32)
            throw std::runtime_error("Project nesting exceeds the supported limit.");
        return true;
    };
    const auto j = Json::parse(text, callback);
    if (j.at("format") != "OPEN-TOON" || j.at("version") != Document::formatVersion)
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
            pixels += im.at("rgba").size();
            if (pixels > 256 * 1024 * 1024)
                throw std::runtime_error("Image budget exceeded.");
            drawing.image =
                ImageAsset{im.at("width"), im.at("height"), im.at("rgba").get<std::vector<std::uint8_t>>()};
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
        l.transform = readTransform(x.at("transform"));
        limit(x.at("exposures"), 1000000);
        limit(x.at("keys"), 1000000);
        for (const auto& e : x.at("exposures"))
            l.exposures.push_back({e.at(0), e.at(1), e.at(2)});
        for (const auto& k : x.at("keys"))
            l.keys.push_back({k.at("frame"), readTransform(k.at("value")),
                              static_cast<Interpolation>(k.at("interpolation").get<int>())});
        d.layers.push_back(std::move(l));
    }
    for (const auto& m : j.at("markers"))
        d.markers.push_back({m.at(0), m.at(1)});
    d.validate();
    return d;
}
} // namespace opentoon
