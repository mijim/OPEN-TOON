#include "project_store.h"
#include "serialization.h"
#include <memory>
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
        sqlite3_limit(db, SQLITE_LIMIT_LENGTH, 64 * 1024 * 1024);
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
void verifyProject(Database& db) {
    Statement type(db, "PRAGMA application_id");
    if (sqlite3_step(type.value) != SQLITE_ROW)
        throw std::runtime_error(std::string("Cannot read project: ") + sqlite3_errmsg(db.db));
    if (sqlite3_column_int(type.value, 0) != 0x4f544f4e)
        throw std::runtime_error("This file is not an OPEN-TOON project.");
    Statement version(db, "PRAGMA user_version");
    if (sqlite3_step(version.value) != SQLITE_ROW || sqlite3_column_int(version.value, 0) != 1)
        throw std::runtime_error("Unsupported project schema. Open it with a compatible OPEN-TOON version.");
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
} // namespace
std::int64_t ProjectStore::save(const std::filesystem::path& path, const Document& document,
                                const std::string& label, std::int64_t expectedRevision, FailureHook hook) {
    const auto data = serializeDocument(document);
    if (data.size() > 64 * 1024 * 1024)
        throw std::runtime_error("Scene exceeds the 64 MiB prototype save limit.");
    const bool existing = std::filesystem::exists(path) && std::filesystem::file_size(path) != 0;
    Database db(path, true);
    if (existing) {
        verifyProject(db);
    } else {
        db.execute("PRAGMA application_id=1330925390; PRAGMA user_version=1;");
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
        Statement s(db, "INSERT INTO revisions(label,document) VALUES(?,?)");
        sqlite3_bind_text(s.value, 1, label.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s.value, 2, data.data(), static_cast<int>(data.size()), SQLITE_TRANSIENT);
        if (sqlite3_step(s.value) != SQLITE_DONE)
            throw std::runtime_error(sqlite3_errmsg(db.db));
        auto revision = sqlite3_last_insert_rowid(db.db);
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
    return {deserializeDocument(column(s.value, 1)), sqlite3_column_int64(s.value, 0)};
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
} // namespace opentoon
