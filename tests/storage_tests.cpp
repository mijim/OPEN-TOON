#include "project_store.h"
#include "serialization.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <fstream>
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
    auto position = text.find("\"version\":1");
    REQUIRE(position != std::string::npos);
    text.replace(position, 11, "\"version\":9");
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
        next.editableDrawing(next.layers.front().id, 0).image =
            ImageAsset{1024, 1024, std::vector<std::uint8_t>(1024 * 1024 * 4, 128)};
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
        const char futureVersion[] = {0, 0, 0, 9};
        stream.write(futureVersion, 4);
    }
    auto size = std::filesystem::file_size(p.file);
    REQUIRE_THROWS(ProjectStore::load(p.file));
    REQUIRE_THROWS(ProjectStore::save(p.file, makeDocument()));
    REQUIRE(std::filesystem::file_size(p.file) == size);
}
