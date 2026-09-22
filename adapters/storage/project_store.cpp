#include "project_store.h"
#include "resources.h"
#include "serialization.h"
#include <memory>
#include <set>
#include <sqlite3.h>
#include <stdexcept>
namespace opentoon {
namespace {
class Database {
  public:
    sqlite3* db = nullptr;
    Database(const std::filesystem::path& path, bool write) {
        auto utf8 = path.u8string();
        int result = sqlite3_open_v2(
            reinterpret_cast<const char*>(utf8.c_str()), &db,
            write ? SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE : SQLITE_OPEN_READWRITE, nullptr);
        // Existing projects may need SQLite to roll back an interrupted transaction.
        // Never create a missing file while loading; clean read-only media still work.
        if (result != SQLITE_OK && !write) {
            if (db)
                sqlite3_close(db);
            db = nullptr;
            result = sqlite3_open_v2(reinterpret_cast<const char*>(utf8.c_str()), &db, SQLITE_OPEN_READONLY,
                                     nullptr);
        }
        if (result != SQLITE_OK) {
            std::string message = db ? sqlite3_errmsg(db) : "Could not open project.";
            if (db)
                sqlite3_close(db);
            db = nullptr;
            throw std::runtime_error(message);
        }
        sqlite3_limit(db, SQLITE_LIMIT_LENGTH, 512 * 1024 * 1024);
        sqlite3_busy_timeout(db, 2000);
    }
    ~Database() {
        if (db)
            sqlite3_close(db);
    }
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    void execute(const char* sql) {
        char* error = nullptr;
        if (sqlite3_exec(db, sql, nullptr, nullptr, &error) != SQLITE_OK) {
            std::string message = error ? error : "Project database error.";
            sqlite3_free(error);
            throw std::runtime_error(message);
        }
    }
};
class Statement {
  public:
    sqlite3_stmt* value = nullptr;
    Statement(Database& db, const char* sql) {
        if (sqlite3_prepare_v2(db.db, sql, -1, &value, nullptr) != SQLITE_OK)
            throw std::runtime_error(sqlite3_errmsg(db.db));
    }
    ~Statement() { sqlite3_finalize(value); }
};
int verifyProject(Database& db) {
    Statement type(db, "PRAGMA application_id");
    if (sqlite3_step(type.value) != SQLITE_ROW)
        throw std::runtime_error(std::string("Cannot read project: ") + sqlite3_errmsg(db.db));
    if (sqlite3_column_int(type.value, 0) != 0x4f544f4e)
        throw std::runtime_error("This file is not an OPEN-TOON project.");
    Statement version(db, "PRAGMA user_version");
    if (sqlite3_step(version.value) != SQLITE_ROW)
        throw std::runtime_error("Cannot read database schema version.");
    auto number = sqlite3_column_int(version.value, 0);
    if (number < 1 || number > Document::formatVersion)
        throw std::runtime_error("Unsupported project schema. Open it with a compatible OPEN-TOON version.");
    return number;
}
std::int64_t head(Database& db) {
    Statement s(db, "SELECT coalesce(max(id),0) FROM revisions");
    if (sqlite3_step(s.value) != SQLITE_ROW)
        throw std::runtime_error("Cannot read project revision.");
    return sqlite3_column_int64(s.value, 0);
}
std::string column(sqlite3_stmt* s, int index) {
    auto* value = sqlite3_column_text(s, index);
    if (!value)
        return {};
    return {reinterpret_cast<const char*>(value), static_cast<std::size_t>(sqlite3_column_bytes(s, index))};
}
std::vector<std::uint8_t> readResource(Database& db, const std::string& hash) {
    if (hash.size() != 64 || hash.find_first_not_of("0123456789abcdef") != std::string::npos)
        throw std::runtime_error("Invalid resource identity.");
    Statement s(db, "SELECT raw_size,data FROM resources WHERE hash=?");
    sqlite3_bind_text(s.value, 1, hash.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(s.value) != SQLITE_ROW)
        throw std::runtime_error("Project references a missing resource: " + hash);
    const auto raw = sqlite3_column_int64(s.value, 0);
    if (raw < 0 || raw > 256 * 1024 * 1024)
        throw std::runtime_error("Resource exceeds the read budget.");
    auto data = static_cast<const std::uint8_t*>(sqlite3_column_blob(s.value, 1));
    auto result = decompressResource({data, std::size_t(sqlite3_column_bytes(s.value, 1))}, std::size_t(raw));
    if (resourceHash(result) != hash)
        throw std::runtime_error("Project resource checksum mismatch: " + hash);
    return result;
}
void backupDatabase(Database& source, const std::filesystem::path& destination) {
    Database target(destination, true);
    auto* backup = sqlite3_backup_init(target.db, "main", source.db, "main");
    if (!backup)
        throw std::runtime_error("Cannot create a project migration backup.");
    int result = sqlite3_backup_step(backup, -1);
    int finish = sqlite3_backup_finish(backup);
    if (result != SQLITE_DONE || finish != SQLITE_OK)
        throw std::runtime_error("Project backup failed; migration was not started.");
}
} // namespace
std::int64_t ProjectStore::save(const std::filesystem::path& path, const Document& document,
                                const std::string& label, std::int64_t expectedRevision, FailureHook hook) {
    document.validate();
    const bool existing = std::filesystem::exists(path) && std::filesystem::file_size(path) != 0;
    Database db(path, true);
    if (existing) {
        const auto version = verifyProject(db);
        if (version < Document::formatVersion) {
            auto backup = path;
            const auto source = ".pre-v" + std::to_string(version);
            backup += source + ".bak";
            for (int suffix = 1; std::filesystem::exists(backup); ++suffix) {
                backup = path;
                backup += source + "-" + std::to_string(suffix) + ".bak";
            }
            backupDatabase(db, backup);
        }
    } else {
        db.execute("PRAGMA application_id=1330925390; PRAGMA user_version=5;");
    }
    db.execute("PRAGMA journal_mode=DELETE; PRAGMA synchronous=FULL; PRAGMA foreign_keys=ON;");
    db.execute("CREATE TABLE IF NOT EXISTS revisions (id INTEGER PRIMARY KEY, created TEXT NOT NULL DEFAULT "
               "(strftime('%Y-%m-%dT%H:%M:%fZ','now')), label TEXT NOT NULL, document TEXT NOT NULL)");
    auto point = [&](SavePoint p) {
        if (hook)
            hook(p);
    };
    point(SavePoint::BeforeTransaction);
    db.execute("BEGIN IMMEDIATE");
    bool committed = false;
    try {
        const auto current = head(db);
        if (expectedRevision >= 0 && current != expectedRevision)
            throw std::runtime_error(
                "Project changed on disk. Save a copy or reopen to resolve the conflict.");
        db.execute("CREATE TABLE IF NOT EXISTS resources(hash TEXT PRIMARY KEY,raw_size INTEGER NOT "
                   "NULL,data BLOB NOT NULL);"
                   "CREATE TABLE IF NOT EXISTS revision_resources(revision INTEGER NOT NULL REFERENCES "
                   "revisions(id) ON DELETE CASCADE,hash TEXT NOT NULL REFERENCES resources(hash),PRIMARY "
                   "KEY(revision,hash));"
                   "PRAGMA user_version=5;");
        std::set<std::string> references;
        auto data = serializeDocument(document, [&](std::span<const std::uint8_t> bytes) {
            auto hash = resourceHash(bytes);
            if (references.insert(hash).second) {
                Statement exists(db, "SELECT 1 FROM resources WHERE hash=?");
                sqlite3_bind_text(exists.value, 1, hash.c_str(), -1, SQLITE_TRANSIENT);
                const auto state = sqlite3_step(exists.value);
                if (state != SQLITE_ROW && state != SQLITE_DONE)
                    throw std::runtime_error(sqlite3_errmsg(db.db));
                if (state == SQLITE_ROW) {
                    auto stored = readResource(db, hash);
                    if (stored.size() != bytes.size() ||
                        !std::equal(stored.begin(), stored.end(), bytes.begin()))
                        throw std::runtime_error("Resource identity collision.");
                } else {
                    auto compressed = compressResource(bytes);
                    Statement resource(db, "INSERT INTO resources(hash,raw_size,data) VALUES(?,?,?)");
                    sqlite3_bind_text(resource.value, 1, hash.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_int64(resource.value, 2, sqlite3_int64(bytes.size()));
                    sqlite3_bind_blob(resource.value, 3, compressed.data(), int(compressed.size()),
                                      SQLITE_TRANSIENT);
                    if (sqlite3_step(resource.value) != SQLITE_DONE)
                        throw std::runtime_error(sqlite3_errmsg(db.db));
                }
            }
            return hash;
        });
        if (data.size() > 64 * 1024 * 1024)
            throw std::runtime_error("Scene metadata exceeds 64 MiB.");
        Statement s(db, "INSERT INTO revisions(label,document) VALUES(?,?)");
        sqlite3_bind_text(s.value, 1, label.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s.value, 2, data.data(), static_cast<int>(data.size()), SQLITE_TRANSIENT);
        if (sqlite3_step(s.value) != SQLITE_DONE)
            throw std::runtime_error(sqlite3_errmsg(db.db));
        auto revision = sqlite3_last_insert_rowid(db.db);
        for (const auto& hash : references) {
            Statement reference(db, "INSERT INTO revision_resources(revision,hash) VALUES(?,?)");
            sqlite3_bind_int64(reference.value, 1, revision);
            sqlite3_bind_text(reference.value, 2, hash.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(reference.value) != SQLITE_DONE)
                throw std::runtime_error(sqlite3_errmsg(db.db));
        }
        point(SavePoint::AfterInsert);
        point(SavePoint::BeforeCommit);
        db.execute("COMMIT");
        committed = true;
        point(SavePoint::AfterCommit);
        return revision;
    } catch (...) {
        if (!committed)
            db.execute("ROLLBACK");
        throw;
    }
}
LoadedProject ProjectStore::load(const std::filesystem::path& path, std::int64_t revision) {
    Database db(path, false);
    verifyProject(db);
    db.execute("PRAGMA query_only=ON;");
    Statement check(db, "PRAGMA quick_check(1)");
    if (sqlite3_step(check.value) != SQLITE_ROW || column(check.value, 0) != "ok")
        throw std::runtime_error("Project integrity check failed. Open a recovery copy.");
    Statement s(db, revision ? "SELECT id,document FROM revisions WHERE id=?"
                             : "SELECT id,document FROM revisions ORDER BY id DESC LIMIT 1");
    if (revision)
        sqlite3_bind_int64(s.value, 1, revision);
    if (sqlite3_step(s.value) != SQLITE_ROW)
        throw std::runtime_error("No valid scene revision was found.");
    return {deserializeDocument(column(s.value, 1),
                                [&](const std::string& hash) { return readResource(db, hash); }),
            sqlite3_column_int64(s.value, 0)};
}
std::vector<StoredRevision> ProjectStore::revisions(const std::filesystem::path& path) {
    Database db(path, false);
    verifyProject(db);
    Statement s(db, "SELECT id,created,label FROM revisions ORDER BY id DESC LIMIT 1000");
    std::vector<StoredRevision> result;
    int state;
    while ((state = sqlite3_step(s.value)) == SQLITE_ROW)
        result.push_back({sqlite3_column_int64(s.value, 0), column(s.value, 1), column(s.value, 2)});
    if (state != SQLITE_DONE)
        throw std::runtime_error("Cannot list revisions.");
    return result;
}
ProjectStore::CompactionResult ProjectStore::compact(const std::filesystem::path& path, int retain,
                                                     std::int64_t expected) {
    if (retain < 1 || retain > 1000 || expected < 1)
        throw std::invalid_argument("Invalid compaction request.");
    auto before = std::filesystem::file_size(path);
    Database db(path, false);
    if (verifyProject(db) != Document::formatVersion)
        throw std::runtime_error("Save this project in the current format before compacting.");
    if (head(db) != expected)
        throw std::runtime_error("Project changed on disk; reopen before compacting.");
    auto backup = path;
    backup += ".pre-compact.bak";
    for (int suffix = 1; std::filesystem::exists(backup); ++suffix) {
        backup = path;
        backup += ".pre-compact-" + std::to_string(suffix) + ".bak";
    }
    backupDatabase(db, backup);
    db.execute("PRAGMA foreign_keys=ON; BEGIN IMMEDIATE;");
    try {
        if (head(db) != expected)
            throw std::runtime_error("Project changed while preparing compaction.");
        Statement remove(
            db, "DELETE FROM revisions WHERE id NOT IN (SELECT id FROM revisions ORDER BY id DESC LIMIT ?)");
        sqlite3_bind_int(remove.value, 1, retain);
        if (sqlite3_step(remove.value) != SQLITE_DONE)
            throw std::runtime_error(sqlite3_errmsg(db.db));
        db.execute("DELETE FROM resources WHERE hash NOT IN (SELECT hash FROM revision_resources); COMMIT;");
    } catch (...) {
        db.execute("ROLLBACK");
        throw;
    }
    db.execute("VACUUM;");
    return {before, std::filesystem::file_size(path), revisions(path).size(), backup};
}
} // namespace opentoon
