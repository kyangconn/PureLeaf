#include "pureleaf/database.h"

#include <sqlite3.h>

#include <stdexcept>

namespace pureleaf {

// clang-format off
static const char* kSchemaSql = R"SQL(
-- ── Projects ────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS projects (
    id          TEXT PRIMARY KEY,
    name        TEXT NOT NULL,
    root_path   TEXT NOT NULL,
    main_tex    TEXT,
    created_at  INTEGER NOT NULL,
    updated_at  INTEGER NOT NULL
);

-- ── Files (tracked entries in the project tree) ────────────
CREATE TABLE IF NOT EXISTS files (
    id          TEXT PRIMARY KEY,
    project_id  TEXT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
    path        TEXT NOT NULL,
    is_dir      INTEGER NOT NULL DEFAULT 0,
    created_at  INTEGER NOT NULL,
    UNIQUE(project_id, path)
);

-- ── File revisions (point to content-addressed blobs) ──────
-- file_id has NO cascade: we keep history even after file deletion.
CREATE TABLE IF NOT EXISTS file_revisions (
    id          TEXT PRIMARY KEY,
    file_id     TEXT NOT NULL REFERENCES files(id),
    blob_hash   TEXT NOT NULL,
    size        INTEGER NOT NULL,
    created_at  INTEGER NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_revisions_file
    ON file_revisions(file_id, created_at DESC);

-- ── Snapshots (project-level point-in-time copies) ─────────
CREATE TABLE IF NOT EXISTS snapshots (
    id          TEXT PRIMARY KEY,
    project_id  TEXT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
    name        TEXT NOT NULL,
    created_at  INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS snapshot_files (
    snapshot_id TEXT NOT NULL REFERENCES snapshots(id) ON DELETE CASCADE,
    file_path   TEXT NOT NULL,
    blob_hash   TEXT NOT NULL,
    PRIMARY KEY(snapshot_id, file_path)
);
)SQL";
// clang-format on

void Database::Deleter::operator()(sqlite3* db) const {
    if (db) sqlite3_close(db);
}

Database::Database(const std::string& path) {
    sqlite3* raw = nullptr;
    int rc = sqlite3_open(path.c_str(), &raw);
    if (rc != SQLITE_OK) {
        std::string msg = raw ? sqlite3_errmsg(raw) : "unknown error";
        if (raw) sqlite3_close(raw);
        throw std::runtime_error("Failed to open database: " + msg);
    }
    db_.reset(raw);

    // Pragmas: WAL for concurrency, FK enforcement, busy timeout.
    exec("PRAGMA journal_mode=WAL;");
    exec("PRAGMA foreign_keys=ON;");
    exec("PRAGMA busy_timeout=5000;");

    runMigrations();
}

Database::~Database() = default;

bool Database::exec(const std::string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(handle(), sql.c_str(), nullptr, nullptr, &err);
    if (err) {
        lastErrMsg_ = err;
        sqlite3_free(err);
    }
    return rc == SQLITE_OK;
}

std::string Database::lastError() const {
    if (!lastErrMsg_.empty()) return lastErrMsg_;
    return db_ ? sqlite3_errmsg(db_.get()) : "no database";
}

void Database::runMigrations() {
    if (!exec(kSchemaSql)) {
        throw std::runtime_error("Schema migration failed: " + lastError());
    }
}

}  // namespace pureleaf
