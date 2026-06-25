#include "pureleaf/project_service.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

#include "pureleaf/database.h"

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

std::string ProjectService::detectMainTex(const std::string& dir) {
    std::error_code ec;
    const fs::path root(dir);
    const fs::path preferred = root / "main.tex";
    if (fs::is_regular_file(preferred, ec)) {
        return "main.tex";
    }

    fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
    const fs::recursive_directory_iterator end;
    for (; !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;

        auto extension = it->path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (extension != ".tex") continue;

        auto relative = fs::relative(it->path(), root, ec);
        if (ec) return {};
        return relative.generic_string();
    }

    return {};
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

Result<Project> ProjectService::registerProjectFolder(const std::string& name,
                                                      const std::string& rootPath) {
    auto guard = lockManager_.acquire("open:" + rootPath);

    std::error_code ec;
    if (!fs::exists(rootPath, ec) || !fs::is_directory(rootPath, ec)) {
        return Result<Project>::Err(Error::IoError);
    }

    auto result = projectRepo_.create(name, rootPath);
    if (!result.ok()) {
        return result;
    }

    auto project = result.value();
    const auto mainTex = detectMainTex(rootPath);
    if (!mainTex.empty()) {
        projectRepo_.setMainTex(project.id, mainTex);
        return projectRepo_.get(project.id);
    }

    return result;
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

Result<void> ProjectService::forgetProject(const std::string& id) {
    auto guard = lockManager_.acquire(id);
    if (!projectRepo_.remove(id)) {
        return Result<void>::Err(Error::Internal);
    }

    return Result<void>::Ok();
}

}  // namespace pureleaf
