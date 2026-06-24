#include "pureleaf/project_service.h"

#include "pureleaf/database.h"

#include <filesystem>
#include <fstream>

namespace pureleaf {

namespace fs = std::filesystem;

static const char* kDefaultMainTex = R"TEX(\documentclass{article}
\usepackage[utf8]{inputenc}
\usepackage[T1]{fontenc}
\usepackage{geometry}
\geometry{a4paper, margin=2.5cm}

\title{Untitled}
\author{}
\date{}

\begin{document}
\maketitle

\end{document}
)TEX";

ProjectService::ProjectService(Database& db, ProjectLockManager& lockManager)
    : db_(db), lockManager_(lockManager), projectRepo_(db) {}

bool ProjectService::writeDefaultTemplate(const std::string& dir) {
    std::string path = (fs::path(dir) / "main.tex").string();
    // Don't overwrite if main.tex already exists.
    if (fs::exists(path)) return true;

    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << kDefaultMainTex;
    return static_cast<bool>(out);
}

Result<Project> ProjectService::createProject(const std::string& name,
                                              const std::string& rootPath) {
    auto guard = lockManager_.acquire("new:" + rootPath);

    bool dirCreated = false;
    std::error_code ec;

    // Step 1: Create directory on disk.
    if (!fs::exists(rootPath, ec)) {
        fs::create_directories(rootPath, ec);
        if (ec) {
            return Result<Project>::Err(Error::IoError);
        }
        dirCreated = true;
    }

    // Step 2: Write default template.
    if (!writeDefaultTemplate(rootPath)) {
        if (dirCreated) fs::remove_all(rootPath, ec);
        return Result<Project>::Err(Error::IoError);
    }

    // Step 3: Write DB metadata.
    auto result = projectRepo_.create(name, rootPath);
    if (!result.ok()) {
        // Compensating: remove directory only if we created it.
        if (dirCreated) fs::remove_all(rootPath, ec);
        return result;
    }

    auto project = result.value();

    // Step 4: Auto-detect main.tex.
    projectRepo_.setMainTex(project.id, "main.tex");

    // Re-read the project to get the updated main_tex.
    return projectRepo_.get(project.id);
}

Result<Project> ProjectService::getProject(const std::string& id) {
    return projectRepo_.get(id);
}

Result<std::vector<Project>> ProjectService::listProjects() {
    return projectRepo_.list();
}

Result<Project> ProjectService::renameProject(const std::string& id, const std::string& newName) {
    auto guard = lockManager_.acquire(id);
    return projectRepo_.rename(id, newName);
}

Result<Project> ProjectService::setMainTex(const std::string& id, const std::string& mainTex) {
    auto guard = lockManager_.acquire(id);
    return projectRepo_.setMainTex(id, mainTex);
}

Result<void> ProjectService::deleteProject(const std::string& id) {
    auto guard = lockManager_.acquire(id);

    // Get the project to know its root path.
    auto project = projectRepo_.get(id);
    if (!project.ok()) {
        return Result<void>::Err(project.error);
    }

    // Step 1: Delete DB metadata (cascades to files, snapshots).
    if (!projectRepo_.remove(id)) {
        return Result<void>::Err(Error::Internal);
    }

    // Step 2: Delete disk directory.
    std::error_code ec;
    fs::remove_all(project.value().rootPath, ec);
    // Ignore disk errors — metadata is already gone.

    return Result<void>::Ok();
}

}  // namespace pureleaf
