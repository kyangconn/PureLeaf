#include "pureleaf/repo.h"

#include <sqlite3.h>

#include <chrono>
#include <cstdio>
#include <random>

#include "pureleaf/database.h"

namespace pureleaf {

// ── Helpers ─────────────────────────────────────────────────────

static int64_t nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

/// Generates a short unique id: timestamp + random suffix.
static std::string generateId() {
    auto t = nowMs();
    std::random_device rd;
    std::mt19937_64 gen(rd());
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%llx-%08lx", static_cast<unsigned long long>(t),
                  static_cast<unsigned long>(gen()));
    return buf;
}

static Project projectFromStmt(sqlite3_stmt* stmt) {
    Project p;
    p.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    p.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    p.rootPath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    if (const auto* mt = sqlite3_column_text(stmt, 3)) {
        p.mainTex = reinterpret_cast<const char*>(mt);
    }
    p.createdAt = sqlite3_column_int64(stmt, 4);
    p.updatedAt = sqlite3_column_int64(stmt, 5);
    return p;
}

static Revision revisionFromStmt(sqlite3_stmt* stmt) {
    Revision r;
    r.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    r.fileId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    r.blobHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    r.size = sqlite3_column_int64(stmt, 3);
    r.createdAt = sqlite3_column_int64(stmt, 4);
    return r;
}

/// Binds a string that may be null.
static void bindNullableText(sqlite3_stmt* stmt, int idx, const std::string& s) {
    if (s.empty()) {
        sqlite3_bind_null(stmt, idx);
    } else {
        sqlite3_bind_text(stmt, idx, s.c_str(), -1, SQLITE_TRANSIENT);
    }
}

// ── ProjectRepo ─────────────────────────────────────────────────

ProjectRepo::ProjectRepo(Database& db) : db_(db) {}

Result<Project> ProjectRepo::create(const std::string& name, const std::string& rootPath) {
    auto id = generateId();
    auto t = nowMs();

    const char* sql =
        "INSERT INTO projects(id, name, root_path, main_tex, created_at, updated_at) "
        "VALUES(?,?,?,?,?,?)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<Project>::Err(Error::Internal);
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, rootPath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_null(stmt, 4);
    sqlite3_bind_int64(stmt, 5, t);
    sqlite3_bind_int64(stmt, 6, t);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Result<Project>::Err(Error::Internal);
    }

    return get(id);
}

Result<Project> ProjectRepo::get(const std::string& id) {
    const char* sql =
        "SELECT id, name, root_path, main_tex, created_at, updated_at "
        "FROM projects WHERE id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<Project>::Err(Error::Internal);
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return Result<Project>::Err(Error::NotFound);
    }

    auto p = projectFromStmt(stmt);
    sqlite3_finalize(stmt);
    return Result<Project>::Ok(std::move(p));
}

Result<std::vector<Project>> ProjectRepo::list() {
    const char* sql =
        "SELECT id, name, root_path, main_tex, created_at, updated_at "
        "FROM projects ORDER BY updated_at DESC";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<std::vector<Project>>::Err(Error::Internal);
    }

    std::vector<Project> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(projectFromStmt(stmt));
    }
    sqlite3_finalize(stmt);
    return Result<std::vector<Project>>::Ok(std::move(results));
}

Result<Project> ProjectRepo::rename(const std::string& id, const std::string& newName) {
    const char* sql = "UPDATE projects SET name = ?, updated_at = ? WHERE id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<Project>::Err(Error::Internal);
    }

    sqlite3_bind_text(stmt, 1, newName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, nowMs());
    sqlite3_bind_text(stmt, 3, id.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return Result<Project>::Err(Error::Internal);
    return get(id);
}

Result<Project> ProjectRepo::setMainTex(const std::string& id, const std::string& mainTex) {
    const char* sql = "UPDATE projects SET main_tex = ?, updated_at = ? WHERE id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<Project>::Err(Error::Internal);
    }

    bindNullableText(stmt, 1, mainTex);
    sqlite3_bind_int64(stmt, 2, nowMs());
    sqlite3_bind_text(stmt, 3, id.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return Result<Project>::Err(Error::Internal);
    return get(id);
}

bool ProjectRepo::remove(const std::string& id) {
    const char* sql = "DELETE FROM projects WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ── RevisionRepo ────────────────────────────────────────────────

RevisionRepo::RevisionRepo(Database& db) : db_(db) {}

Result<Revision> RevisionRepo::create(const std::string& fileId, const std::string& blobHash,
                                      int64_t size) {
    auto id = generateId();
    auto t = nowMs();

    const char* sql =
        "INSERT INTO file_revisions(id, file_id, blob_hash, size, created_at) "
        "VALUES(?,?,?,?,?)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<Revision>::Err(Error::Internal);
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, fileId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, blobHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, size);
    sqlite3_bind_int64(stmt, 5, t);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return Result<Revision>::Err(Error::Internal);

    Revision r;
    r.id = id;
    r.fileId = fileId;
    r.blobHash = blobHash;
    r.size = size;
    r.createdAt = t;
    return Result<Revision>::Ok(std::move(r));
}

Result<std::vector<Revision>> RevisionRepo::listByFile(const std::string& fileId) {
    const char* sql =
        "SELECT id, file_id, blob_hash, size, created_at "
        "FROM file_revisions WHERE file_id = ? ORDER BY created_at DESC, rowid DESC";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<std::vector<Revision>>::Err(Error::Internal);
    }

    sqlite3_bind_text(stmt, 1, fileId.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<Revision> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(revisionFromStmt(stmt));
    }
    sqlite3_finalize(stmt);
    return Result<std::vector<Revision>>::Ok(std::move(results));
}

Result<Revision> RevisionRepo::latest(const std::string& fileId) {
    const char* sql =
        "SELECT id, file_id, blob_hash, size, created_at "
        "FROM file_revisions WHERE file_id = ? ORDER BY created_at DESC, rowid DESC LIMIT 1";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<Revision>::Err(Error::Internal);
    }

    sqlite3_bind_text(stmt, 1, fileId.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return Result<Revision>::Err(Error::NotFound);
    }

    auto r = revisionFromStmt(stmt);
    sqlite3_finalize(stmt);
    return Result<Revision>::Ok(std::move(r));
}

}  // namespace pureleaf
