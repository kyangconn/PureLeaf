#pragma once

#include "pureleaf/file_repo.h"
#include "pureleaf/lock_manager.h"
#include "pureleaf/project.h"
#include "pureleaf/repo.h"
#include "pureleaf/types.h"

#include <string>
#include <vector>

namespace pureleaf {

class Database;

/// High-level project lifecycle service.
///
/// Orchestrates DB metadata + disk directory creation with compensation
/// (rollback on failure). Uses ProjectLockManager to serialize concurrent
/// ops on the same project.
class ProjectService {
public:
    ProjectService(Database& db, ProjectLockManager& lockManager);

    /// Creates a new project: directory on disk + DB metadata + main.tex template.
    /// If the directory already exists, it is reused (no overwrite).
    /// On any failure after partial steps, compensating cleanup is performed.
    Result<Project> createProject(const std::string& name, const std::string& rootPath);

    /// Registers an existing folder as a project without writing template files.
    /// If a main.tex is present it is recorded as the main file.
    Result<Project> registerProjectFolder(const std::string& name, const std::string& rootPath);

    /// Opens an existing project by id. Does NOT scan disk.
    Result<Project> getProject(const std::string& id);

    /// Lists all projects.
    Result<std::vector<Project>> listProjects();

    /// Renames a project (metadata only; disk directory is not moved).
    Result<Project> renameProject(const std::string& id, const std::string& newName);

    /// Sets the main .tex file path.
    Result<Project> setMainTex(const std::string& id, const std::string& mainTex);

    /// Deletes a project: DB metadata + files + disk directory.
    /// Revisions are preserved (by design — history outlives deletion).
    Result<void> deleteProject(const std::string& id);

    /// Removes project metadata from the recent/project list without touching files on disk.
    Result<void> forgetProject(const std::string& id);

private:
    Database& db_;
    ProjectLockManager& lockManager_;
    ProjectRepo projectRepo_;

    /// Writes the default main.tex template to a directory.
    bool writeDefaultTemplate(const std::string& dir);

    /// Finds a likely main .tex file in a project directory.
    std::string detectMainTex(const std::string& dir);
};

}  // namespace pureleaf
