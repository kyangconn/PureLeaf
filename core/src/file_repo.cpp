#include "pureleaf/file_repo.h"

#include <sqlite3.h>

#include <chrono>
#include <cstdio>
#include <random>

#include "pureleaf/database.h"

namespace pureleaf {

static int64_t nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

static std::string generateId() {
    auto t = nowMs();
    std::random_device rd;
    std::mt19937_64 gen(rd());
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%llx-%08lx", static_cast<unsigned long long>(t),
                  static_cast<unsigned long>(gen()));
    return buf;
}

static FileEntry fileEntryFromStmt(sqlite3_stmt* stmt) {
    FileEntry e;
    e.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    e.projectId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (const auto* p = sqlite3_column_text(stmt, 2)) {
        e.parentId = reinterpret_cast<const char*>(p);
    }
    e.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    e.isDir = sqlite3_column_int(stmt, 4) != 0;
    e.createdAt = sqlite3_column_int64(stmt, 5);
    return e;
}

static void bindParentId(sqlite3_stmt* stmt, int idx, const std::string& parentId) {
    if (parentId.empty()) {
        sqlite3_bind_null(stmt, idx);
    } else {
        sqlite3_bind_text(stmt, idx, parentId.c_str(), -1, SQLITE_TRANSIENT);
    }
}

// ── FileRepo ─────────────────────────────────────────────────────

FileRepo::FileRepo(Database& db) : db_(db) {}

Result<FileEntry> FileRepo::create(const std::string& projectId, const std::string& parentId,
                                   const std::string& name, bool isDir) {
    auto id = generateId();
    auto t = nowMs();

    const char* sql =
        "INSERT INTO files(id, project_id, parent_id, name, is_dir, created_at) "
        "VALUES(?,?,?,?,?,?)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<FileEntry>::Err(Error::Internal);
    }

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, projectId.c_str(), -1, SQLITE_TRANSIENT);
    bindParentId(stmt, 3, parentId);
    sqlite3_bind_text(stmt, 4, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, isDir ? 1 : 0);
    sqlite3_bind_int64(stmt, 6, t);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        // UNIQUE constraint violation = AlreadyExists
        return Result<FileEntry>::Err(Error::AlreadyExists);
    }

    return get(id);
}

Result<FileEntry> FileRepo::get(const std::string& id) {
    const char* sql =
        "SELECT id, project_id, parent_id, name, is_dir, created_at "
        "FROM files WHERE id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<FileEntry>::Err(Error::Internal);
    }
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return Result<FileEntry>::Err(Error::NotFound);
    }

    auto e = fileEntryFromStmt(stmt);
    sqlite3_finalize(stmt);
    return Result<FileEntry>::Ok(std::move(e));
}

Result<FileEntry> FileRepo::findByName(const std::string& projectId, const std::string& parentId,
                                       const std::string& name) {
    // Use COALESCE to match NULL parent_id when parentId is empty.
    std::string sql =
        "SELECT id, project_id, parent_id, name, is_dir, created_at "
        "FROM files WHERE project_id = ? AND name = ? AND ";

    if (parentId.empty()) {
        sql += "parent_id IS NULL";
    } else {
        sql += "parent_id = ?";
    }
    sql += " LIMIT 1";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<FileEntry>::Err(Error::Internal);
    }

    sqlite3_bind_text(stmt, 1, projectId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    if (!parentId.empty()) {
        sqlite3_bind_text(stmt, 3, parentId.c_str(), -1, SQLITE_TRANSIENT);
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return Result<FileEntry>::Err(Error::NotFound);
    }

    auto e = fileEntryFromStmt(stmt);
    sqlite3_finalize(stmt);
    return Result<FileEntry>::Ok(std::move(e));
}

Result<std::vector<FileEntry>> FileRepo::listChildren(const std::string& projectId,
                                                      const std::string& parentId) {
    std::string sql =
        "SELECT id, project_id, parent_id, name, is_dir, created_at "
        "FROM files WHERE project_id = ? AND ";

    if (parentId.empty()) {
        sql += "parent_id IS NULL ORDER BY is_dir DESC, name ASC";
    } else {
        sql += "parent_id = ? ORDER BY is_dir DESC, name ASC";
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<std::vector<FileEntry>>::Err(Error::Internal);
    }

    sqlite3_bind_text(stmt, 1, projectId.c_str(), -1, SQLITE_TRANSIENT);
    if (!parentId.empty()) {
        sqlite3_bind_text(stmt, 2, parentId.c_str(), -1, SQLITE_TRANSIENT);
    }

    std::vector<FileEntry> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(fileEntryFromStmt(stmt));
    }
    sqlite3_finalize(stmt);
    return Result<std::vector<FileEntry>>::Ok(std::move(results));
}

Result<std::vector<FileEntry>> FileRepo::listByProject(const std::string& projectId) {
    const char* sql =
        "SELECT id, project_id, parent_id, name, is_dir, created_at "
        "FROM files WHERE project_id = ? ORDER BY is_dir DESC, name ASC";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<std::vector<FileEntry>>::Err(Error::Internal);
    }
    sqlite3_bind_text(stmt, 1, projectId.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<FileEntry> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(fileEntryFromStmt(stmt));
    }
    sqlite3_finalize(stmt);
    return Result<std::vector<FileEntry>>::Ok(std::move(results));
}

Result<FileEntry> FileRepo::rename(const std::string& id, const std::string& newName) {
    const char* sql = "UPDATE files SET name = ? WHERE id = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return Result<FileEntry>::Err(Error::Internal);
    }
    sqlite3_bind_text(stmt, 1, newName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return Result<FileEntry>::Err(Error::AlreadyExists);
    return get(id);
}

bool FileRepo::remove(const std::string& id) {
    const char* sql = "DELETE FROM files WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool FileRepo::removeAllByProject(const std::string& projectId) {
    const char* sql = "DELETE FROM files WHERE project_id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, projectId.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

}  // namespace pureleaf
