#include "project_store.h"
#include "serialization.h"
#include "opentoon/rigging.h"
#include "opentoon/deformation.h"
#include "opentoon/deformer.h"
#include "opentoon/character_pose.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace opentoon;
namespace {
struct TemporaryProject {
    std::filesystem::path directory =
        std::filesystem::temp_directory_path() /
        ("opentoon-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::path file;
    TemporaryProject() {
        std::filesystem::create_directories(directory);
        file = directory / "scene.otoon";
    }
    ~TemporaryProject() { std::filesystem::remove_all(directory); }
};
} // namespace
TEST_CASE("Projects round trip Unicode names keys and images with stable identities") {
    auto d = makeBouncingBall();
    d.name = "Scene \xe6\x9d\xb1\xe4\xba\xac";
    d.drawings.begin()->second.image = ImageAsset{1, 1, {10, 20, 30, 40}};
    REQUIRE(deserializeDocument(serializeDocument(d)) == d);
    TemporaryProject p;
    auto revision = ProjectStore::save(p.file, d);
    auto loaded = ProjectStore::load(p.file);
    REQUIRE(loaded.document == d);
    REQUIRE(loaded.revision == revision);
}
TEST_CASE("Format 13 persists published pose controls and loads older pose schemas") {
    auto d = makeDocument();
    const Id part = d.layers.front().id;
    const Id root = makeCharacter(d, part, "Hero");
    (void)createSubstitution(d, part, 0, false, "Default");
    const Id pose = captureCharacterPose(d, root, 0,
        std::vector<PoseCaptureTarget>{{part, PoseChannels::PositionX | PoseChannels::Drawing}},
        "Reach");
    publishCharacterPose(d, root, pose, true);
    REQUIRE(deserializeDocument(serializeDocument(d)) == d);
    TemporaryProject project;
    (void)ProjectStore::save(project.file, d);
    REQUIRE(ProjectStore::load(project.file).document == d);
    auto version12 = nlohmann::json::parse(serializeDocument(d));
    version12["version"] = 12;
    for (auto& layer : version12["layers"])
        for (auto& entry : layer["poses"])
            entry.erase("published");
    const auto oldPose = deserializeDocument(version12.dump());
    REQUIRE(oldPose.layer(root).poses.size() == 1);
    REQUIRE_FALSE(oldPose.layer(root).poses.front().published);
    auto previous = nlohmann::json::parse(serializeDocument(d));
    previous["version"] = 11;
    for (auto& layer : previous["layers"])
        layer.erase("poses");
    const auto old = deserializeDocument(previous.dump());
    REQUIRE(old.layer(root).poses.empty());
    REQUIRE(old.nextId == d.nextId);
    REQUIRE(d.layer(root).poses.front().id == pose);
}
TEST_CASE("Failure injection preserves the previous or fully committed revision") {
    for (auto point : {ProjectStore::SavePoint::BeforeTransaction, ProjectStore::SavePoint::AfterInsert,
                       ProjectStore::SavePoint::BeforeCommit, ProjectStore::SavePoint::AfterCommit}) {
        TemporaryProject p;
        auto first = makeDocument();
        auto original = ProjectStore::save(p.file, first);
        auto next = first;
        next.name = "Next";
        REQUIRE_THROWS(ProjectStore::save(p.file, next, "Failure injection", original, [=](auto stage) {
            if (stage == point)
                throw std::runtime_error("Injected failure");
        }));
        auto loaded = ProjectStore::load(p.file);
        REQUIRE(loaded.document == (point == ProjectStore::SavePoint::AfterCommit ? next : first));
    }
}
TEST_CASE("Saving a stale revision cannot overwrite another writer") {
    TemporaryProject p;
    auto d = makeDocument();
    auto revision = ProjectStore::save(p.file, d);
    d.name = "Writer A";
    (void)ProjectStore::save(p.file, d, "A", revision);
    d.name = "Writer B";
    REQUIRE_THROWS(ProjectStore::save(p.file, d, "B", revision));
    REQUIRE(ProjectStore::load(p.file).document.name == "Writer A");
    REQUIRE(ProjectStore::revisions(p.file).size() == 2);
}
TEST_CASE("Unknown versions and excessive nesting do not enter the document model") {
    auto text = serializeDocument(makeDocument());
    const auto currentVersion = "\"version\":" + std::to_string(Document::formatVersion);
    auto position = text.find(currentVersion);
    REQUIRE(position != std::string::npos);
    text.replace(position, currentVersion.size(),
                 "\"version\":" + std::to_string(Document::formatVersion + 1));
    REQUIRE_THROWS(deserializeDocument(text));
    REQUIRE_THROWS(deserializeDocument(std::string(40, '[') + "0" + std::string(40, ']')));
}

#if defined(__unix__) || defined(__APPLE__)
#include <sys/wait.h>
#include <unistd.h>
TEST_CASE("Abrupt process termination at each save boundary leaves a recoverable project") {
    for (auto point : {ProjectStore::SavePoint::BeforeTransaction, ProjectStore::SavePoint::AfterInsert,
                       ProjectStore::SavePoint::BeforeCommit, ProjectStore::SavePoint::AfterCommit}) {
        TemporaryProject p;
        auto first = makeDocument();
        auto revision = ProjectStore::save(p.file, first);
        auto next = first;
        next.name = "Committed child revision";
        // Exceed SQLite page cache so the failure creates a hot rollback journal.
        std::vector<std::uint8_t> noise(1024 * 1024 * 4);
        std::uint32_t random = 42;
        for (auto& pixel : noise) {
            random ^= random << 13;
            random ^= random >> 17;
            random ^= random << 5;
            pixel = static_cast<std::uint8_t>(random);
        }
        next.editableDrawing(next.layers.front().id, 0).image = ImageAsset{1024, 1024, std::move(noise)};
        const auto child = fork();
        REQUIRE(child >= 0);
        if (child == 0) {
            (void)ProjectStore::save(p.file, next, "Crash test", revision, [=](auto stage) {
                if (stage == point)
                    _exit(89);
            });
            _exit(90);
        }
        int status = 0;
        REQUIRE(waitpid(child, &status, 0) == child);
        REQUIRE(WIFEXITED(status));
        REQUIRE(WEXITSTATUS(status) == 89);
        REQUIRE(ProjectStore::load(p.file).document ==
                (point == ProjectStore::SavePoint::AfterCommit ? next : first));
    }
}
#endif

TEST_CASE("Saving to a foreign file fails without overwriting its contents") {
    TemporaryProject p;
    const std::string content = "Unrelated document that must remain intact";
    {
        std::ofstream stream(p.file);
        stream << content;
    }
    REQUIRE_THROWS(ProjectStore::save(p.file, makeDocument()));
    std::ifstream stream(p.file);
    REQUIRE(std::string(std::istreambuf_iterator<char>(stream), {}) == content);
}
TEST_CASE("Future database versions are rejected before saving and missing loads create no file") {
    TemporaryProject p;
    REQUIRE_THROWS(ProjectStore::load(p.file));
    REQUIRE_FALSE(std::filesystem::exists(p.file));
    (void)ProjectStore::save(p.file, makeDocument());
    // SQLite's documented user_version field is a big-endian integer at offset 60.
    {
        std::fstream stream(p.file, std::ios::binary | std::ios::in | std::ios::out);
        stream.seekp(60);
        const char futureVersion[] = {0, 0, 0, char(Document::formatVersion + 1)};
        stream.write(futureVersion, 4);
    }
    auto size = std::filesystem::file_size(p.file);
    REQUIRE_THROWS(ProjectStore::load(p.file));
    REQUIRE_THROWS(ProjectStore::save(p.file, makeDocument()));
    REQUIRE(std::filesystem::file_size(p.file) == size);
}

#include <sqlite3.h>
namespace {
struct FixtureDatabase {
    sqlite3* db = nullptr;
    explicit FixtureDatabase(const std::filesystem::path& path) {
        const auto utf8 = path.u8string();
        REQUIRE(sqlite3_open(reinterpret_cast<const char*>(utf8.c_str()), &db) == SQLITE_OK);
    }
    ~FixtureDatabase() { sqlite3_close(db); }
    void execute(const char* sql) { REQUIRE(sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK); }
    int count(const char* sql) {
        sqlite3_stmt* query = nullptr;
        REQUIRE(sqlite3_prepare_v2(db, sql, -1, &query, nullptr) == SQLITE_OK);
        const auto result = sqlite3_step(query);
        const int count = result == SQLITE_ROW ? sqlite3_column_int(query, 0) : -1;
        sqlite3_finalize(query);
        REQUIRE(result == SQLITE_ROW);
        return count;
    }
    void replaceDocument(const std::string& json) {
        sqlite3_stmt* update = nullptr;
        REQUIRE(sqlite3_prepare_v2(db, "UPDATE revisions SET document=?", -1, &update, nullptr) == SQLITE_OK);
        REQUIRE(sqlite3_bind_text(update, 1, json.data(), int(json.size()), SQLITE_TRANSIENT) == SQLITE_OK);
        REQUIRE(sqlite3_step(update) == SQLITE_DONE);
        sqlite3_finalize(update);
    }
};
} // namespace
TEST_CASE("Version one projects migrate with an independently readable original backup") {
    TemporaryProject p;
    std::filesystem::copy_file(std::filesystem::path(OPENTOON_SOURCE_DIR) / "tests/fixtures/v1-scene.otoon",
                               p.file);
    const auto original = ProjectStore::load(p.file);
    {
        FixtureDatabase db(p.file);
        REQUIRE(db.count("PRAGMA user_version") == 1);
    }
    auto changed = original.document;
    changed.name = "Migrated scene";
    const auto revision = ProjectStore::save(p.file, changed, "Upgrade", original.revision);
    REQUIRE(ProjectStore::load(p.file).document == changed);
    REQUIRE(ProjectStore::load(p.file, original.revision).document == original.document);
    auto backup = p.file;
    backup += ".pre-v1.bak";
    REQUIRE(ProjectStore::load(backup).document == original.document);
    {
        FixtureDatabase db(backup);
        REQUIRE(db.count("PRAGMA user_version") == 1);
    }
    REQUIRE(revision > original.revision);
}
TEST_CASE("Immutable media is shared across snapshots and deduplicated across stored revisions") {
    TemporaryProject p;
    auto d = makeDocument();
    auto& drawing = d.editableDrawing(d.layers.front().id, 0);
    drawing.image = ImageAsset{1, 1, {10, 20, 30, 255}};
    drawing.raster = RasterImage{128, 128, {}};
    std::vector<std::uint16_t> pixels(64 * 64 * 4, 16000);
    drawing.raster->tiles[{0, 0}] = pixels;
    drawing.raster->tiles[{1, 0}] = pixels;
    auto snapshot = d;
    REQUIRE(snapshot.drawings.begin()->second.image->rgba.data() == drawing.image->rgba.data());
    auto head = ProjectStore::save(p.file, d);
    for (int n = 0; n < 3; ++n) {
        d.name += " next";
        head = ProjectStore::save(p.file, d, "Metadata only", head);
    }
    REQUIRE(ProjectStore::load(p.file).document == d);
    FixtureDatabase db(p.file);
    REQUIRE(db.count("SELECT count(*) FROM resources") == 2);
    REQUIRE(db.count("SELECT count(*) FROM revision_resources") == 8);
    REQUIRE(db.count("SELECT max(length(document)) FROM revisions") < 4096);
    REQUIRE_THROWS(ProjectStore::compact(p.file, 1, head - 1));
    const auto result = ProjectStore::compact(p.file, 1, head);
    REQUIRE(result.retainedRevisions == 1);
    REQUIRE(ProjectStore::revisions(result.backup).size() == 4);
    REQUIRE(ProjectStore::load(result.backup, 1).document == snapshot);
    REQUIRE(ProjectStore::load(p.file).document == d);
}
TEST_CASE("Corrupted and missing resources fail instead of returning damaged artwork") {
    for (const auto* damage : {"UPDATE resources SET data=zeroblob(length(data))", "DELETE FROM resources",
                               "UPDATE resources SET raw_size=raw_size+4"}) {
        TemporaryProject p;
        auto d = makeDocument();
        d.editableDrawing(d.layers.front().id, 0).image = ImageAsset{1, 1, {0, 0, 0, 255}};
        (void)ProjectStore::save(p.file, d);
        {
            FixtureDatabase db(p.file);
            db.execute(damage);
        }
        REQUIRE_THROWS(ProjectStore::load(p.file));
    }
}
TEST_CASE("Compaction collects only resources no retained revision needs") {
    TemporaryProject p;
    auto d = makeDocument();
    auto& drawing = d.editableDrawing(d.layers.front().id, 0);
    drawing.image = ImageAsset{1, 1, {0, 0, 0, 255}};
    auto first = ProjectStore::save(p.file, d);
    drawing.image = ImageAsset{1, 1, {255, 255, 255, 255}};
    auto second = ProjectStore::save(p.file, d, "Replace image", first);
    auto result = ProjectStore::compact(p.file, 1, second);
    REQUIRE(ProjectStore::load(p.file).document == d);
    FixtureDatabase db(p.file);
    REQUIRE(db.count("SELECT count(*) FROM resources") == 1);
    REQUIRE(ProjectStore::load(result.backup, first).document.drawings.begin()->second.image->rgba[0] == 0);
}

TEST_CASE("Schema two upgrades preserve an original backup and protect Bezier metadata") {
    TemporaryProject p;
    auto d = makeDocument();
    auto rev = ProjectStore::save(p.file, d);
    auto oldJson = nlohmann::json::parse(serializeDocument(d));
    oldJson["version"] = 2;
    for (auto& layer : oldJson["layers"]) {
        layer.erase("kind");
        layer.erase("role");
        layer.erase("variants");
        layer.erase("views");
    }
    {
        FixtureDatabase db(p.file);
        db.replaceDocument(oldJson.dump());
        db.execute("PRAGMA user_version=2");
    }
    Transform target;
    target.x = 100;
    d.layers.front().keys = {{0, {}, Interpolation::Linear}, {20, target, Interpolation::Linear}};
    d.layers.front().keys.front().easing["x"] = {.25, 0, .65, 1.8};
    (void)ProjectStore::save(p.file, d, "Bezier upgrade", rev);
    REQUIRE(ProjectStore::load(p.file).document == d);
    auto backup = p.file;
    backup += ".pre-v2.bak";
    REQUIRE(ProjectStore::load(backup).document == makeDocument());
    {
        FixtureDatabase db(backup);
        REQUIRE(db.count("PRAGMA user_version") == 2);
    }
    {
        FixtureDatabase db(p.file);
        REQUIRE(db.count("PRAGMA user_version") == Document::formatVersion);
    }
}
TEST_CASE("Format three scene migrates through typed characters with an original backup") {
    TemporaryProject p;
    auto legacy = makeDocument();
    const auto oldRevision = ProjectStore::save(p.file, legacy);
    auto oldJson = nlohmann::json::parse(serializeDocument(legacy));
    oldJson["version"] = 3;
    for (auto& layer : oldJson["layers"]) {
        layer.erase("kind");
        layer.erase("role");
        layer.erase("variants");
        layer.erase("views");
    }
    {
        FixtureDatabase db(p.file);
        db.replaceDocument(oldJson.dump());
        db.execute("PRAGMA user_version=3");
    }
    REQUIRE(ProjectStore::load(p.file).document == legacy);
    auto upgraded = legacy;
    const Id partId = upgraded.layers.front().id;
    Layer character;
    character.id = upgraded.allocateId();
    character.name = "Hero";
    character.kind = LayerKind::Character;
    upgraded.layers.push_back(character);
    auto& part = upgraded.layer(partId);
    part.kind = LayerKind::Part;
    part.parent = character.id;
    part.role = "Body";
    REQUIRE_THROWS(ProjectStore::save(p.file, upgraded, "Rejected migration", oldRevision,
                                     [](auto point) {
                                         if (point == ProjectStore::SavePoint::BeforeTransaction)
                                             throw std::runtime_error("Injected migration failure");
                                     }));
    REQUIRE(ProjectStore::load(p.file).document == legacy);
    auto backup = p.file;
    backup += ".pre-v3.bak";
    REQUIRE(std::filesystem::exists(backup));
    REQUIRE(ProjectStore::load(backup).document == legacy);
    REQUIRE(ProjectStore::save(p.file, upgraded, "Character migration", oldRevision) > oldRevision);
    REQUIRE(ProjectStore::load(p.file).document == upgraded);
    REQUIRE(ProjectStore::load(backup).document == legacy);
    FixtureDatabase current(p.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}
TEST_CASE("Format four character scene migrates to view sets with a preserved backup") {
    TemporaryProject p;
    auto legacy = makeDocument();
    const Id part = legacy.layers.front().id;
    const Id root = makeCharacter(legacy, part, "Hero");
    const Id closed = createSubstitution(legacy, part, 0, false, "Closed");
    const auto revision = ProjectStore::save(p.file, legacy);
    auto oldJson = nlohmann::json::parse(serializeDocument(legacy));
    oldJson["version"] = 4;
    for (auto& layer : oldJson["layers"])
        layer.erase("views");
    {
        FixtureDatabase db(p.file);
        db.replaceDocument(oldJson.dump());
        db.execute("PRAGMA user_version=4");
    }
    REQUIRE(ProjectStore::load(p.file).document == legacy);
    auto upgraded = legacy;
    const Id view = captureCharacterView(upgraded, root, 0, "Front");
    REQUIRE(upgraded.layer(root).views.front().id == view);
    REQUIRE(upgraded.layer(root).views.front().choices.front().drawing == closed);
    REQUIRE_THROWS(ProjectStore::save(p.file, upgraded, "Injected format-five failure", revision,
                                      [](auto point) {
                                          if (point == ProjectStore::SavePoint::BeforeTransaction)
                                              throw std::runtime_error("Injected migration failure");
                                      }));
    REQUIRE(ProjectStore::load(p.file).document == legacy);
    auto backup = p.file;
    backup += ".pre-v4.bak";
    REQUIRE(std::filesystem::exists(backup));
    REQUIRE(ProjectStore::load(backup).document == legacy);
    REQUIRE(ProjectStore::save(p.file, upgraded, "View sets", revision) > revision);
    REQUIRE(ProjectStore::load(p.file).document == upgraded);
    REQUIRE(ProjectStore::load(backup).document == legacy);
    FixtureDatabase current(p.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}
TEST_CASE("Format five scenes default to legacy appearance and migrate with a backup") {
    TemporaryProject project;
    auto old = makeBouncingBall();
    const auto revision = ProjectStore::save(project.file, old);
    auto json = nlohmann::json::parse(serializeDocument(old));
    json["version"] = 5;
    json.erase("composition");
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(json.dump());
        db.execute("PRAGMA user_version=5");
    }
    REQUIRE(ProjectStore::load(project.file).document == old);
    auto next = old;
    next.composition = CompositionProfile::LinearSrgb;
    REQUIRE_THROWS(ProjectStore::save(project.file, next, "Injected profile migration failure",
                                      revision, [](auto point) {
                                          if (point == ProjectStore::SavePoint::BeforeTransaction)
                                              throw std::runtime_error("Injected migration failure");
                                      }));
    REQUIRE(ProjectStore::load(project.file).document == old);
    auto backup = project.file;
    backup += ".pre-v5.bak";
    REQUIRE(std::filesystem::exists(backup));
    REQUIRE(ProjectStore::load(backup).document == old);
    REQUIRE(ProjectStore::save(project.file, next, "Linear composition", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == next);
    FixtureDatabase current(project.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}
TEST_CASE("Format six scene gains an output camera with a readable original backup") {
    TemporaryProject project;
    auto original = makeDocument();
    const auto revision = ProjectStore::save(project.file, original);
    auto oldJson = nlohmann::json::parse(serializeDocument(original));
    oldJson["version"] = 6;
    oldJson.erase("activeCamera");
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(oldJson.dump());
        db.execute("PRAGMA user_version=6");
    }
    REQUIRE(ProjectStore::load(project.file).document == original);
    auto next = original;
    Layer camera;
    camera.id = next.allocateId();
    camera.name = "Output camera";
    camera.kind = LayerKind::Camera;
    camera.transform.x = next.width / 2;
    camera.transform.y = next.height / 2;
    next.activeCamera = camera.id;
    next.layers.push_back(camera);
    REQUIRE(ProjectStore::save(project.file, next, "Add camera", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == next);
    auto backup = project.file;
    backup += ".pre-v6.bak";
    REQUIRE(ProjectStore::load(backup).document == original);
    FixtureDatabase current(project.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}

TEST_CASE("Format seven mesh binding migration preserves the original project") {
    TemporaryProject project;
    auto original = makeDocument();
    const Id part = original.layers.front().id;
    makeCharacter(original, part, "Hero");
    const Id drawing = createSubstitution(original, part, 0, false, "Front");
    original.drawings.at(drawing).image =
        ImageAsset{16, 16, std::vector<std::uint8_t>(16 * 16 * 4, 255)};
    const auto revision = ProjectStore::save(project.file, original);
    auto oldJson = nlohmann::json::parse(serializeDocument(original));
    oldJson["version"] = 7;
    for (auto& layer : oldJson["layers"])
        layer.erase("bindings");
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(oldJson.dump());
        db.execute("PRAGMA user_version=7");
    }
    REQUIRE(ProjectStore::load(project.file).document == original);
    auto next = original;
    bindRegularImageMesh(next, part, drawing, 2, 2);
    REQUIRE_THROWS(ProjectStore::save(project.file, next, "Rejected migration", revision,
                                      [](ProjectStore::SavePoint point) {
                                          if (point == ProjectStore::SavePoint::BeforeTransaction)
                                              throw std::runtime_error("Injected migration failure");
                                      }));
    REQUIRE(ProjectStore::load(project.file).document == original);
    REQUIRE(ProjectStore::save(project.file, next, "Bind mesh", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == next);
    auto backup = project.file;
    backup += ".pre-v7.bak";
    REQUIRE(ProjectStore::load(backup).document == original);
    FixtureDatabase current(project.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}

TEST_CASE("Format eight mesh scene migrates to animated controls with a readable backup") {
    TemporaryProject project;
    auto original = makeDocument();
    const Id part = original.layers.front().id;
    makeCharacter(original, part, "Hero");
    const Id drawing = createSubstitution(original, part, 0, false, "Arm");
    original.drawings.at(drawing).image =
        ImageAsset{16, 16, std::vector<std::uint8_t>(16 * 16 * 4, 255)};
    bindRegularImageMesh(original, part, drawing, 4, 2);
    const auto revision = ProjectStore::save(project.file, original);
    auto oldJson = nlohmann::json::parse(serializeDocument(original));
    oldJson["version"] = 8;
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(oldJson.dump());
        db.execute("PRAGMA user_version=8");
    }
    REQUIRE(ProjectStore::load(project.file).document == original);
    auto next = original;
    bindBoneChain(next, part, drawing, {{{0, 8}, {8, 8}, {16, 8}}}, 3);
    recordBonePose(next, part, drawing, 12, 0, 30);
    REQUIRE(ProjectStore::save(project.file, next, "Bend arm", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == next);
    auto backup = project.file;
    backup += ".pre-v8.bak";
    REQUIRE(ProjectStore::load(backup).document == original);
    FixtureDatabase current(project.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}

TEST_CASE("Format nine scene migrates to bone tip links with a readable backup") {
    TemporaryProject project;
    auto original = makeDocument();
    const auto revision = ProjectStore::save(project.file, original);
    auto oldJson = nlohmann::json::parse(serializeDocument(original));
    oldJson["version"] = 9;
    for (auto& layer : oldJson["layers"])
        layer.erase("boneTipAnchor");
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(oldJson.dump());
        db.execute("PRAGMA user_version=9");
    }
    REQUIRE(ProjectStore::load(project.file).document == original);
    auto next = original;
    next.name = "Current rig";
    REQUIRE(ProjectStore::save(project.file, next, "Upgrade rig", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == next);
    auto backup = project.file;
    backup += ".pre-v9.bak";
    REQUIRE(ProjectStore::load(backup).document == original);
    FixtureDatabase current(project.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}

TEST_CASE("Format ten linked rig migrates explicit rest anchors with a readable backup") {
    TemporaryProject project;
    std::filesystem::copy_file(std::filesystem::path(OPENTOON_SOURCE_DIR) /
        "tests/fixtures/harmony-continuous-limbs/format10-linked.otoon", project.file);
    {
        FixtureDatabase legacy(project.file);
        REQUIRE(legacy.count("PRAGMA user_version") == 10);
    }
    const auto original = ProjectStore::load(project.file);
    REQUIRE(std::count_if(original.document.layers.begin(), original.document.layers.end(),
        [](const Layer& layer) { return layer.boneTipAnchor.has_value(); }) == 4);
    auto invalid = original.document;
    auto linked = std::find_if(invalid.layers.begin(), invalid.layers.end(),
        [](const Layer& layer) { return layer.boneTipAnchor.has_value(); });
    REQUIRE(linked != invalid.layers.end());
    linked->boneTipAnchor->distalAxis = {0, 0};
    REQUIRE_THROWS_AS(invalid.validate(), std::invalid_argument);
    auto changed = original.document;
    changed.name = "Upgraded continuous rig";
    REQUIRE(ProjectStore::save(project.file, changed, "Store stable anchors",
                               original.revision) > original.revision);
    REQUIRE(ProjectStore::load(project.file).document == changed);
    auto backup = project.file;
    backup += ".pre-v10.bak";
    REQUIRE(ProjectStore::load(backup).document == original.document);
    FixtureDatabase current(project.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}
TEST_CASE("Format twelve pose scene upgrades published controls with a readable backup") {
    TemporaryProject project;
    auto original = makeDocument();
    const Id part = original.layers.front().id;
    const Id root = makeCharacter(original, part, "Hero");
    (void)createSubstitution(original, part, 0, false, "Mouth A");
    const Id pose = captureCharacterPose(original, root, 0,
        std::vector<PoseCaptureTarget>{{part, PoseChannels::Rotation}}, "Turn");
    const Id view = captureCharacterView(original, root, 0, "Front");
    const auto revision = ProjectStore::save(project.file, original);
    auto legacy = nlohmann::json::parse(serializeDocument(original));
    legacy["version"] = 12;
    for (auto& layer : legacy["layers"]) {
        for (auto& entry : layer["poses"])
            entry.erase("published");
        for (auto& entry : layer["views"])
            entry.erase("published");
    }
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(legacy.dump());
        db.execute("PRAGMA user_version=12");
    }
    REQUIRE(ProjectStore::load(project.file).document == original);
    auto changed = original;
    publishCharacterPose(changed, root, pose, true);
    publishCharacterView(changed, root, view, true);
    REQUIRE(ProjectStore::save(project.file, changed, "Publish controls", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == changed);
    auto backup = project.file;
    backup += ".pre-v12.bak";
    REQUIRE(ProjectStore::load(backup).document == original);
    FixtureDatabase current(project.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}
TEST_CASE("Format thirteen substitutes migrate published drawings with a readable backup") {
    TemporaryProject project;
    auto original = makeDocument();
    const Id part = original.layers.front().id;
    (void)makeCharacter(original, part, "Hero");
    const Id drawing = createSubstitution(original, part, 0, false, "Mouth A");
    const auto revision = ProjectStore::save(project.file, original);
    auto legacy = nlohmann::json::parse(serializeDocument(original));
    legacy["version"] = 13;
    for (auto& layer : legacy["layers"])
        for (auto& variant : layer["variants"])
            variant.erase("published");
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(legacy.dump());
        db.execute("PRAGMA user_version=13");
    }
    REQUIRE(ProjectStore::load(project.file).document == original);
    auto changed = original;
    publishSubstitution(changed, part, drawing, true);
    REQUIRE(ProjectStore::save(project.file, changed, "Publish drawing", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == changed);
    auto backup = project.file;
    backup += ".pre-v13.bak";
    REQUIRE(ProjectStore::load(backup).document == original);
    FixtureDatabase current(project.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}
TEST_CASE("Format fourteen control groups migrate with a readable source backup") {
    TemporaryProject project;
    auto original = makeDocument();
    const Id part = original.layers.front().id;
    const Id root = makeCharacter(original, part, "Hero");
    const Id drawing = createSubstitution(original, part, 0, false, "Smile");
    const Id view = captureCharacterView(original, root, 0, "Front");
    const Id pose = captureCharacterPose(original, root, 0,
        std::vector<PoseCaptureTarget>{{part, PoseChannels::PositionX}}, "Reach");
    publishSubstitution(original, part, drawing, true);
    publishCharacterView(original, root, view, true);
    publishCharacterPose(original, root, pose, true);
    const auto revision = ProjectStore::save(project.file, original);
    auto legacy = nlohmann::json::parse(serializeDocument(original));
    legacy["version"] = 14;
    for (auto& layer : legacy["layers"]) {
        for (auto& variant : layer["variants"])
            variant.erase("controlGroup");
        for (auto& entry : layer["views"])
            entry.erase("controlGroup");
        for (auto& entry : layer["poses"])
            entry.erase("controlGroup");
    }
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(legacy.dump());
        db.execute("PRAGMA user_version=14");
    }
    REQUIRE(ProjectStore::load(project.file).document == original);
    auto changed = original;
    setSubstitutionControlGroup(changed, part, drawing, "Face");
    setCharacterViewControlGroup(changed, root, view, "Stage");
    setCharacterPoseControlGroup(changed, root, pose, "Body");
    REQUIRE(ProjectStore::save(project.file, changed, "Group controls", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == changed);
    auto backup = project.file;
    backup += ".pre-v14.bak";
    REQUIRE(ProjectStore::load(backup).document == original);
    FixtureDatabase current(project.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}
TEST_CASE("Format seventeen cutter matte migration keeps a readable source backup") {
    TemporaryProject project;
    auto original = makeDocument();
    const auto revision = ProjectStore::save(project.file, original);
    auto legacy = nlohmann::json::parse(serializeDocument(original));
    legacy["version"] = 17;
    for (auto& layer : legacy["layers"])
        layer.erase("matte");
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(legacy.dump());
        db.execute("PRAGMA user_version=17");
    }
    REQUIRE(ProjectStore::load(project.file).document == original);
    auto changed = original;
    Layer source = changed.layers.front();
    source.id = changed.allocateId();
    source.name = "Cutter";
    changed.layers.push_back(source);
    changed.layers.front().matte = source.id;
    changed.validate();
    REQUIRE(ProjectStore::save(project.file, changed, "Add cutter matte", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == changed);
    auto backup = project.file;
    backup += ".pre-v17.bak";
    REQUIRE(ProjectStore::load(backup).document == original);
    FixtureDatabase current(project.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}
TEST_CASE("Format eighteen cutter inversion migrates with a readable source backup") {
    TemporaryProject project;
    auto original = makeDocument();
    Layer source = original.layers.front();
    source.id = original.allocateId();
    source.name = "Cutter";
    original.layers.push_back(source);
    original.layers.front().matte = source.id;
    const auto revision = ProjectStore::save(project.file, original);
    auto legacy = nlohmann::json::parse(serializeDocument(original));
    legacy["version"] = 18;
    for (auto& layer : legacy["layers"])
        layer.erase("invertMatte");
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(legacy.dump());
        db.execute("PRAGMA user_version=18");
    }
    REQUIRE(ProjectStore::load(project.file).document == original);
    auto changed = original;
    changed.layers.front().invertMatte = true;
    REQUIRE(ProjectStore::save(project.file, changed, "Invert cutter", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == changed);
    auto backup = project.file;
    backup += ".pre-v18.bak";
    REQUIRE(ProjectStore::load(backup).document == original);
    FixtureDatabase current(project.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}
TEST_CASE("Format nineteen cutters migrate disabled bypass with a readable source backup") {
    TemporaryProject project;
    auto original = makeDocument();
    Layer source = original.layers.front();
    source.id = original.allocateId();
    source.name = "Cutter";
    original.layers.push_back(source);
    original.layers.front().matte = source.id;
    const auto revision = ProjectStore::save(project.file, original);
    auto legacy = nlohmann::json::parse(serializeDocument(original));
    legacy["version"] = 19;
    for (auto& layer : legacy["layers"])
        layer.erase("matteBypassed");
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(legacy.dump());
        db.execute("PRAGMA user_version=19");
    }
    REQUIRE(ProjectStore::load(project.file).document == original);
    auto changed = original;
    changed.layers.front().matteBypassed = true;
    REQUIRE(ProjectStore::save(project.file, changed, "Bypass cutter", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == changed);
    auto backup = project.file;
    backup += ".pre-v19.bak";
    REQUIRE(ProjectStore::load(backup).document == original);
    FixtureDatabase current(project.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}
TEST_CASE("Format twenty cutters migrate hidden source painting with a readable backup") {
    TemporaryProject project;
    auto original = makeDocument();
    Layer source = original.layers.front();
    source.id = original.allocateId();
    source.name = "Cutter";
    original.layers.push_back(source);
    original.layers.front().matte = source.id;
    const auto revision = ProjectStore::save(project.file, original);
    auto legacy = nlohmann::json::parse(serializeDocument(original));
    legacy["version"] = 20;
    for (auto& layer : legacy["layers"])
        layer.erase("paintMatteSource");
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(legacy.dump());
        db.execute("PRAGMA user_version=20");
    }
    REQUIRE(ProjectStore::load(project.file).document == original);
    auto changed = original;
    changed.layers.back().paintMatteSource = true;
    REQUIRE(ProjectStore::save(project.file, changed, "Paint cutter source", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == changed);
    auto backup = project.file;
    backup += ".pre-v20.bak";
    REQUIRE(ProjectStore::load(backup).document == original);
    FixtureDatabase current(project.file);
    REQUIRE(current.count("PRAGMA user_version") == Document::formatVersion);
}
TEST_CASE("Format fifteen audio migration preserves a readable source backup") {
    TemporaryProject project;
    auto original = makeDocument();
    const auto revision = ProjectStore::save(project.file, original);
    auto legacy = nlohmann::json::parse(serializeDocument(original));
    legacy["version"] = 15;
    legacy.erase("audioAssets");
    legacy.erase("audioClips");
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(legacy.dump());
        db.execute("PRAGMA user_version=15");
    }
    REQUIRE(ProjectStore::load(project.file).document == original);
    auto changed = original;
    changed.name = "Sound-ready scene";
    REQUIRE(ProjectStore::save(project.file, changed, "Migrate audio schema", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == changed);
    auto backup = project.file;
    backup += ".pre-v15.bak";
    REQUIRE(ProjectStore::load(backup).document == original);
}
TEST_CASE("Format sixteen clips migrate to one repeat with a readable source backup") {
    TemporaryProject project;
    const auto original = makeDocument();
    const auto revision = ProjectStore::save(project.file, original);
    auto legacy = nlohmann::json::parse(serializeDocument(original));
    legacy["version"] = 16;
    {
        FixtureDatabase db(project.file);
        db.replaceDocument(legacy.dump());
        db.execute("PRAGMA user_version=16");
    }
    REQUIRE(ProjectStore::load(project.file).document == original);
    auto changed = original;
    changed.name = "Repeat-ready scene";
    REQUIRE(ProjectStore::save(project.file, changed, "Migrate repeat schema", revision) > revision);
    REQUIRE(ProjectStore::load(project.file).document == changed);
    auto backup = project.file;
    backup += ".pre-v16.bak";
    REQUIRE(ProjectStore::load(backup).document == original);
}
