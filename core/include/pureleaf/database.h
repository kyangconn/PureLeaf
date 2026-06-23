#pragma once

#include <memory>
#include <string>

struct sqlite3;

namespace pureleaf {

/// Thin RAII wrapper around sqlite3*.
///
/// Opens the database file (creating it if necessary), runs migrations
/// on first open, and sets sensible pragmas (WAL mode, foreign keys).
class Database {
public:
    /// Opens or creates the database at `path`. The parent directory
    /// must already exist. Throws std::runtime_error on failure.
    explicit Database(const std::string& path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) = delete;
    Database& operator=(Database&&) = delete;

    /// Raw handle for use by repository classes.
    sqlite3* handle() const { return db_.get(); }

    /// Executes a SQL string. Returns false on error (sets lastError).
    bool exec(const std::string& sql);

    /// Returns the last error message from SQLite.
    std::string lastError() const;

private:
    struct Deleter {
        void operator()(sqlite3* db) const;
    };
    std::unique_ptr<sqlite3, Deleter> db_;
    mutable std::string lastErrMsg_;

    void runMigrations();
};

}  // namespace pureleaf
