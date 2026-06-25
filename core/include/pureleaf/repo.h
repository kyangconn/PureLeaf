#pragma once

#include <string>
#include <vector>

#include "pureleaf/project.h"
#include "pureleaf/types.h"

namespace pureleaf {

class Database;

/// Data access for the `projects` table.
class ProjectRepo {
public:
    explicit ProjectRepo(Database& db);

    /// Creates a new project. The `id` is generated if empty.
    Result<Project> create(const std::string& name, const std::string& rootPath);

    /// Fetches a single project by id.
    Result<Project> get(const std::string& id);

    /// Lists all projects, ordered by `updated_at` descending.
    Result<std::vector<Project>> list();

    /// Renames a project. Returns the updated row.
    Result<Project> rename(const std::string& id, const std::string& newName);

    /// Sets the main .tex file path (relative to root).
    Result<Project> setMainTex(const std::string& id, const std::string& mainTex);

    /// Deletes a project and cascades to files/snapshots.
    /// Revisions are NOT cascaded (history is preserved by design).
    bool remove(const std::string& id);

private:
    Database& db_;
};

/// Data access for the `file_revisions` table.
class RevisionRepo {
public:
    explicit RevisionRepo(Database& db);

    /// Records a new revision for a file.
    Result<Revision> create(const std::string& fileId, const std::string& blobHash, int64_t size);

    /// Lists revisions for a file, newest first.
    Result<std::vector<Revision>> listByFile(const std::string& fileId);

    /// Gets the most recent revision of a file.
    Result<Revision> latest(const std::string& fileId);

private:
    Database& db_;
};

}  // namespace pureleaf
