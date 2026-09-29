#include "serialization.h"
#include "opentoon/deformation.h"
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
Json meshPoint(MeshPoint point) { return {point.x, point.y}; }
MeshPoint readMeshPoint(const Json& value) {
    if (!value.is_array() || value.size() != 2)
        throw std::runtime_error("Invalid deformer point encoding.");
    return {value.at(0), value.at(1)};
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
              {"composition", static_cast<int>(d.composition)},
              {"activeCamera", d.activeCamera},
              {"nextId", d.nextId},
              {"layers", Json::array()},
              {"drawings", Json::array()},
              {"palette", Json::array()},
              {"markers", Json::array()},
              {"audioAssets", Json::array()},
              {"audioClips", Json::array()}};
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
                  {"matte", l.matte},
                  {"invertMatte", l.invertMatte},
                  {"matteBypassed", l.matteBypassed},
                  {"paintMatteSource", l.paintMatteSource},
                  {"kind", static_cast<int>(l.kind)},
                  {"role", l.role},
                  {"variants", Json::array()},
                  {"views", Json::array()},
                  {"poses", Json::array()},
                  {"bindings", Json::array()},
                  {"transform", transform(l.transform)},
                  {"exposures", Json::array()},
                  {"keys", Json::array()}};
        if (l.boneTipAnchor)
            x["boneTipAnchor"] = {{"tip", meshPoint(l.boneTipAnchor->tip)},
                                  {"distalAxis", meshPoint(l.boneTipAnchor->distalAxis)}};
        else
            x["boneTipAnchor"] = nullptr;
        for (auto e : l.exposures)
            x["exposures"].push_back({e.start, e.end, e.drawing});
        for (const auto& variant : l.variants)
            x["variants"].push_back({{"drawing", variant.drawing}, {"name", variant.name},
                                      {"published", variant.published},
                                      {"controlGroup", variant.controlGroup}});
        for (const auto& view : l.views) {
            Json choices = Json::array();
            for (const auto& choice : view.choices)
                choices.push_back({choice.part, choice.drawing});
            x["views"].push_back({{"id", view.id}, {"name", view.name},
                                  {"choices", choices}, {"published", view.published},
                                  {"controlGroup", view.controlGroup}});
        }
        for (const auto& pose : l.poses) {
            Json parts = Json::array();
            for (const auto& part : pose.parts)
                parts.push_back({{"part", part.part}, {"channels", part.channels},
                                 {"transform", transform(part.transform)}, {"drawing", part.drawing}});
            x["poses"].push_back({{"id", pose.id}, {"name", pose.name},
                                  {"parts", parts}, {"published", pose.published},
                                  {"controlGroup", pose.controlGroup}});
        }
        for (const auto& binding : l.bindings) {
            Json vertices = Json::array();
            for (const auto& vertex : binding.vertices)
                vertices.push_back({vertex.rest.x, vertex.rest.y, vertex.pose.x,
                                    vertex.pose.y, vertex.uv.x, vertex.uv.y});
            Json entry = {{"drawing", binding.drawing},
                          {"sourceWidth", binding.sourceWidth},
                          {"sourceHeight", binding.sourceHeight},
                          {"columns", binding.columns},
                          {"rows", binding.rows},
                          {"vertices", std::move(vertices)}};
            if (binding.bone) {
                const auto& bone = *binding.bone;
                Json joints = Json::array(), keys = Json::array();
                for (auto joint : bone.restJoints)
                    joints.push_back(meshPoint(joint));
                for (const auto& key : bone.keys)
                    keys.push_back({{"frame", key.frame},
                                    {"shoulder", key.shoulderAngle},
                                    {"elbow", key.elbowAngle},
                                    {"interpolation", static_cast<int>(key.interpolation)}});
                entry["bone"] = {{"restJoints", std::move(joints)},
                                 {"elbowTransition", bone.elbowTransition},
                                 {"distalWeights", bone.distalWeights},
                                 {"keys", std::move(keys)}};
            }
            if (binding.curve) {
                const auto& curve = *binding.curve;
                Json rest = Json::array(), coordinates = Json::array(), keys = Json::array();
                for (auto point : curve.restControls)
                    rest.push_back(meshPoint(point));
                for (const auto& value : curve.coordinates)
                    coordinates.push_back(value.t);
                for (const auto& key : curve.keys) {
                    Json controls = Json::array();
                    for (auto point : key.controls)
                        controls.push_back(meshPoint(point));
                    keys.push_back({{"frame", key.frame}, {"controls", std::move(controls)},
                                    {"interpolation", static_cast<int>(key.interpolation)}});
                }
                entry["curve"] = {{"restControls", std::move(rest)},
                                  {"coordinates", std::move(coordinates)},
                                  {"keys", std::move(keys)}};
            }
            x["bindings"].push_back(std::move(entry));
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
    for (const auto& asset : d.audioAssets) {
        Json entry = {{"id", asset.id}, {"name", asset.name},
                      {"sampleRate", asset.sampleRate}, {"channels", asset.channels},
                      {"sampleFrames", asset.sampleFrames}};
        if (write)
            entry["resource"] = write({asset.wav.data(), asset.wav.size()});
        else
            entry["wav"] = asset.wav.values();
        j["audioAssets"].push_back(std::move(entry));
    }
    for (const auto& clip : d.audioClips)
        j["audioClips"].push_back({{"id", clip.id}, {"asset", clip.asset},
                                   {"start", clip.start}, {"inSample", clip.inSample},
                                   {"outSample", clip.outSample}, {"gain", clip.gain},
                                   {"repeats", clip.repeats},
                                   {"fadeInSamples", clip.fadeInSamples},
                                   {"fadeOutSamples", clip.fadeOutSamples}});
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
    if (j.at("version").get<int>() >= 6)
        d.composition = static_cast<CompositionProfile>(j.at("composition").get<int>());
    if (j.at("version").get<int>() >= 7)
        d.activeCamera = j.at("activeCamera").get<Id>();
    limit(j.at("palette"), 65536);
    limit(j.at("drawings"), 50000);
    limit(j.at("layers"), 2000);
    limit(j.at("markers"), 1000000);
    if (j.at("version").get<int>() >= 16) {
        limit(j.at("audioAssets"), 64);
        limit(j.at("audioClips"), 1000);
        std::size_t audioBytes = 0;
        for (const auto& entry : j.at("audioAssets")) {
            std::vector<std::uint8_t> bytes;
            if (entry.contains("resource")) {
                if (!read)
                    throw std::runtime_error("An external audio resource resolver is required.");
                bytes = read(entry.at("resource"));
            } else
                bytes = entry.at("wav").get<std::vector<std::uint8_t>>();
            audioBytes += bytes.size();
            if (bytes.size() > 128 * 1024 * 1024 || audioBytes > 512 * 1024 * 1024)
                throw std::runtime_error("Audio asset exceeds the import budget.");
            d.audioAssets.push_back({entry.at("id"), entry.at("name"),
                                     entry.at("sampleRate"), entry.at("channels"),
                                     entry.at("sampleFrames"), std::move(bytes)});
        }
        for (const auto& entry : j.at("audioClips"))
            d.audioClips.push_back({entry.at("id"), entry.at("asset"),
                                    entry.at("start"), entry.at("inSample"),
                                    entry.at("outSample"), entry.at("gain"),
                                    j.at("version").get<int>() >= 17
                                        ? entry.at("repeats").get<int>() : 1,
                                    j.at("version").get<int>() >= 22
                                        ? entry.at("fadeInSamples").get<std::uint64_t>() : 0,
                                    j.at("version").get<int>() >= 22
                                        ? entry.at("fadeOutSamples").get<std::uint64_t>() : 0});
    }
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
    std::vector<Id> legacyLinked;
    for (const auto& x : j.at("layers")) {
        Layer l;
        l.id = x.at("id");
        l.name = x.at("name");
        l.visible = x.at("visible");
        l.locked = x.at("locked");
        l.solo = x.at("solo");
        l.parent = x.at("parent");
        if (j.at("version").get<int>() >= 18)
            l.matte = x.at("matte");
        if (j.at("version").get<int>() >= 19)
            l.invertMatte = x.at("invertMatte");
        if (j.at("version").get<int>() >= 20)
            l.matteBypassed = x.at("matteBypassed");
        if (j.at("version").get<int>() >= 21)
            l.paintMatteSource = x.at("paintMatteSource");
        if (j.at("version").get<int>() >= 11) {
            const auto& anchor = x.at("boneTipAnchor");
            if (!anchor.is_null())
                l.boneTipAnchor = BoneTipAnchor{readMeshPoint(anchor.at("tip")),
                                                 readMeshPoint(anchor.at("distalAxis"))};
        } else if (j.at("version").get<int>() == 10 &&
                   x.at("followParentBoneTip").get<bool>()) {
            legacyLinked.push_back(l.id);
        }
        if (j.at("version").get<int>() >= 4) {
            l.kind = static_cast<LayerKind>(x.at("kind").get<int>());
            l.role = x.at("role").get<std::string>();
            limit(x.at("variants"), 10000);
            for (const auto& variant : x.at("variants"))
                l.variants.push_back({variant.at("drawing"), variant.at("name"),
                    j.at("version").get<int>() >= 14 ? variant.at("published").get<bool>() : false,
                    j.at("version").get<int>() >= 15 ? variant.at("controlGroup").get<std::string>()
                                                      : "Main"});
        }
        if (j.at("version").get<int>() >= 5) {
            limit(x.at("views"), 1000);
            for (const auto& view : x.at("views")) {
                CharacterView entry;
                entry.id = view.at("id");
                entry.name = view.at("name");
                if (j.at("version").get<int>() >= 13)
                    entry.published = view.at("published");
                if (j.at("version").get<int>() >= 15)
                    entry.controlGroup = view.at("controlGroup");
                limit(view.at("choices"), 2000);
                for (const auto& choice : view.at("choices"))
                    entry.choices.push_back({choice.at(0), choice.at(1)});
                l.views.push_back(std::move(entry));
            }
        }
        if (j.at("version").get<int>() >= 12) {
            limit(x.at("poses"), 1000);
            for (const auto& pose : x.at("poses")) {
                CharacterPose entry;
                entry.id = pose.at("id");
                entry.name = pose.at("name");
                if (j.at("version").get<int>() >= 13)
                    entry.published = pose.at("published");
                if (j.at("version").get<int>() >= 15)
                    entry.controlGroup = pose.at("controlGroup");
                limit(pose.at("parts"), 2000);
                for (const auto& part : pose.at("parts"))
                    entry.parts.push_back({part.at("part"), part.at("channels"),
                                           readTransform(part.at("transform")), part.at("drawing")});
                l.poses.push_back(std::move(entry));
            }
        }
        if (j.at("version").get<int>() >= 8) {
            limit(x.at("bindings"), 256);
            for (const auto& item : x.at("bindings")) {
                MeshBinding binding;
                binding.drawing = item.at("drawing");
                binding.sourceWidth = item.at("sourceWidth");
                binding.sourceHeight = item.at("sourceHeight");
                binding.columns = item.at("columns");
                binding.rows = item.at("rows");
                limit(item.at("vertices"), 1089);
                for (const auto& value : item.at("vertices")) {
                    if (!value.is_array() || value.size() != 6)
                        throw std::runtime_error("Invalid mesh vertex encoding.");
                    binding.vertices.push_back({{value.at(0), value.at(1)},
                                                {value.at(2), value.at(3)},
                                                {value.at(4), value.at(5)}});
                }
                if (j.at("version").get<int>() >= 9 && item.contains("bone")) {
                    const auto& source = item.at("bone");
                    BoneChain bone;
                    limit(source.at("restJoints"), 3);
                    if (source.at("restJoints").size() != 3)
                        throw std::runtime_error("Bone chain needs three rest joints.");
                    for (std::size_t i = 0; i < 3; ++i)
                        bone.restJoints[i] = readMeshPoint(source.at("restJoints").at(i));
                    bone.elbowTransition = source.at("elbowTransition");
                    limit(source.at("distalWeights"), 1089);
                    bone.distalWeights = source.at("distalWeights").get<std::vector<double>>();
                    limit(source.at("keys"), 10000);
                    for (const auto& key : source.at("keys"))
                        bone.keys.push_back({key.at("frame"), key.at("shoulder"),
                                             key.at("elbow"),
                                             static_cast<Interpolation>(key.at("interpolation").get<int>())});
                    binding.bone = std::move(bone);
                }
                if (j.at("version").get<int>() >= 9 && item.contains("curve")) {
                    const auto& source = item.at("curve");
                    CurveDeformer curve;
                    limit(source.at("restControls"), 4);
                    if (source.at("restControls").size() != 4)
                        throw std::runtime_error("Curve needs four rest controls.");
                    for (std::size_t i = 0; i < 4; ++i)
                        curve.restControls[i] = readMeshPoint(source.at("restControls").at(i));
                    limit(source.at("coordinates"), 1089);
                    for (const auto& value : source.at("coordinates"))
                        curve.coordinates.push_back({value.get<double>()});
                    limit(source.at("keys"), 10000);
                    for (const auto& key : source.at("keys")) {
                        CurvePoseKey pose;
                        pose.frame = key.at("frame");
                        pose.interpolation =
                            static_cast<Interpolation>(key.at("interpolation").get<int>());
                        limit(key.at("controls"), 4);
                        if (key.at("controls").size() != 4)
                            throw std::runtime_error("Curve key needs four controls.");
                        for (std::size_t i = 0; i < 4; ++i)
                            pose.controls[i] = readMeshPoint(key.at("controls").at(i));
                        curve.keys.push_back(std::move(pose));
                    }
                    binding.curve = std::move(curve);
                }
                l.bindings.push_back(std::move(binding));
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
    for (const Id childId : legacyLinked) {
        auto& child = d.layer(childId);
        const auto& source = d.layer(child.parent);
        const auto* drawing = d.drawingAt(source.id, 0);
        const auto* binding = drawing ? meshBindingFor(source, drawing->id) : nullptr;
        if (!binding || !binding->bone)
            throw std::runtime_error("Legacy bone tip attachment has no rest source bone.");
        const auto& joints = binding->bone->restJoints;
        child.boneTipAnchor = BoneTipAnchor{
            joints[2], {joints[2].x - joints[1].x, joints[2].y - joints[1].y}};
    }
    for (const auto& m : j.at("markers"))
        d.markers.push_back({m.at(0), m.at(1)});
    d.validate();
    return d;
}
} // namespace opentoon
