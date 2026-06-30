#include "desktopqmlcontroller.h"

#include <QColor>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStyleHints>
#include <QUrl>
#include <QtGlobal>
#include <algorithm>
#include <filesystem>
#include <stdexcept>

#include "pureleaf/database.h"
#include "pureleaf/desktop/platform.h"
#include "pureleaf/lock_manager.h"
#include "pureleaf/project_service.h"
#include "pureleaf/version.h"

namespace pureleaf::qml {

namespace {

QString systemColorScheme() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (const auto* app = qApp) {
        return app->styleHints()->colorScheme() == Qt::ColorScheme::Dark ? QStringLiteral("dark")
                                                                         : QStringLiteral("light");
    }
#endif
    return QStringLiteral("light");
}

QString fallbackUserDataDir() {
    QString path = QString::fromStdString(pureleaf::desktop::getPaths().userDataDir);
    if (path.isEmpty()) {
        path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    }
    if (path.isEmpty()) {
        path = QDir::home().filePath(QStringLiteral(".pureleaf"));
    }
    return QDir::cleanPath(path);
}

QString sanitizedProjectDirName(QString name) {
    name = name.trimmed();
    name.replace(QRegularExpression(QStringLiteral("[<>:\"/\\\\|?*]+")), QStringLiteral("-"));
    name.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    name = name.trimmed();
    return name.isEmpty() ? QStringLiteral("Untitled") : name;
}

QString leafNameForPath(const QString& path) {
    const QFileInfo info(path);
    const QString name = info.fileName().trimmed();
    return name.isEmpty() ? QStringLiteral("Untitled") : name;
}

QString normalizedPath(const QString& path) {
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

bool isSkippedProjectPath(const QString& relativePath) {
    return relativePath == QStringLiteral("build") ||
           relativePath.startsWith(QStringLiteral("build/")) ||
           relativePath == QStringLiteral(".git") ||
           relativePath.startsWith(QStringLiteral(".git/"));
}

bool isEditableExtension(const QString& suffix) {
    const QString lower = suffix.toLower();
    return lower == QStringLiteral("tex") || lower == QStringLiteral("bib") ||
           lower == QStringLiteral("sty") || lower == QStringLiteral("cls") ||
           lower == QStringLiteral("txt") || lower == QStringLiteral("md");
}

QString errorText(pureleaf::Error error) {
    switch (error) {
        case pureleaf::Error::None:
            return QStringLiteral("ok");
        case pureleaf::Error::NotFound:
            return QStringLiteral("not found");
        case pureleaf::Error::AlreadyExists:
            return QStringLiteral("already exists");
        case pureleaf::Error::InvalidPath:
            return QStringLiteral("invalid path");
        case pureleaf::Error::IoError:
            return QStringLiteral("I/O error");
        case pureleaf::Error::InvalidArgument:
            return QStringLiteral("invalid argument");
        case pureleaf::Error::Internal:
            return QStringLiteral("internal error");
    }
    return QStringLiteral("unknown error");
}

}  // namespace

DesktopQmlController::DesktopQmlController(pureleaf::ui::AppSettings settings, QObject* parent)
    : QObject(parent), settings_(std::move(settings)) {
    autoSaveTimer_.setSingleShot(true);
    compileTimeoutTimer_.setSingleShot(true);
    connect(&autoSaveTimer_, &QTimer::timeout, this, [this]() {
        if (editorDirty_) {
            saveCurrentFile();
        }
    });
    connect(&compileTimeoutTimer_, &QTimer::timeout, this, [this]() {
        if (compileProcess_.state() != QProcess::NotRunning) {
            appendCompileLog(tr("\nCompile timed out. Process terminated.\n"));
            compileProcess_.kill();
        }
    });
    connect(&compileProcess_, &QProcess::readyReadStandardOutput, this, [this]() {
        appendCompileLog(QString::fromLocal8Bit(compileProcess_.readAllStandardOutput()));
    });
    connect(&compileProcess_, &QProcess::readyReadStandardError, this, [this]() {
        appendCompileLog(QString::fromLocal8Bit(compileProcess_.readAllStandardError()));
    });
    connect(&compileProcess_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &DesktopQmlController::finishCompile);

    currentPage_ = QStringLiteral("home");
    initServices();
    loadRecentProjects();
    recomputeTheme();

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (auto* hints = qGuiApp ? qGuiApp->styleHints() : nullptr) {
        connect(hints, &QStyleHints::colorSchemeChanged, this, [this]() {
            if (settings_.appTheme == QStringLiteral("system")) {
                recomputeTheme();
                emit themeChanged();
            }
        });
    }
#endif
}

DesktopQmlController::~DesktopQmlController() {
    if (compileProcess_.state() != QProcess::NotRunning) {
        compileProcess_.kill();
        compileProcess_.waitForFinished(300);
    }
}

void DesktopQmlController::initServices() {
    const QString dataDir = userDataDir();
    QDir dir(dataDir);
    dir.mkpath(QStringLiteral("."));
    dir.mkpath(QStringLiteral("projects"));

    try {
        const QString databasePath = dir.filePath(QStringLiteral("pureleaf.sqlite3"));
        database_ = std::make_unique<pureleaf::Database>(databasePath.toStdString());
        lockManager_ = std::make_unique<pureleaf::ProjectLockManager>();
        projectService_ = std::make_unique<pureleaf::ProjectService>(*database_, *lockManager_);
    } catch (const std::exception& ex) {
        database_.reset();
        lockManager_.reset();
        projectService_.reset();
        qWarning("PureLeaf QML project service init failed: %s", ex.what());
    }
}

void DesktopQmlController::loadRecentProjects() {
    recentProjects_.clear();
    if (!projectService_) {
        emit recentProjectsChanged();
        emit windowProfileChanged();
        return;
    }

    const auto result = projectService_->listProjects();
    if (!result.ok()) {
        emit recentProjectsChanged();
        emit windowProfileChanged();
        return;
    }

    const auto& projects = result.value();
    for (const pureleaf::Project& project : projects) {
        recentProjects_.append(projectToMap(project));
    }

    emit recentProjectsChanged();
    emit windowProfileChanged();
}

bool DesktopQmlController::effectiveDark() const {
    if (settings_.appTheme == QStringLiteral("dark")) return true;
    if (settings_.appTheme == QStringLiteral("light")) return false;
    return systemColorScheme() == QStringLiteral("dark");
}

void DesktopQmlController::recomputeTheme() {
    const bool dark = effectiveDark();
    QVariantMap t;
    t.insert(QStringLiteral("dark"), dark);
    t.insert(QStringLiteral("bg"),
             QColor(dark ? QStringLiteral("#1f1f1f") : QStringLiteral("#fafafa")));
    t.insert(QStringLiteral("surface"),
             QColor(dark ? QStringLiteral("#262626") : QStringLiteral("#ffffff")));
    t.insert(QStringLiteral("surfaceAlt"),
             QColor(dark ? QStringLiteral("#2d2d2d") : QStringLiteral("#f2f4f7")));
    t.insert(QStringLiteral("fg"),
             QColor(dark ? QStringLiteral("#e7e7e7") : QStringLiteral("#1f1f1f")));
    t.insert(QStringLiteral("fgMuted"),
             QColor(dark ? QStringLiteral("#9a9a9a") : QStringLiteral("#667085")));
    t.insert(QStringLiteral("border"),
             QColor(dark ? QStringLiteral("#3a3a3a") : QStringLiteral("#d0d5dd")));
    t.insert(QStringLiteral("accent"),
             QColor(dark ? QStringLiteral("#3fb950") : QStringLiteral("#2e7d32")));
    t.insert(QStringLiteral("accentSoft"),
             QColor(dark ? QStringLiteral("#1f3d2a") : QStringLiteral("#e7f4e8")));
    t.insert(QStringLiteral("danger"),
             QColor(dark ? QStringLiteral("#f85149") : QStringLiteral("#d92d20")));
    t.insert(QStringLiteral("radius"), 10);
    t.insert(QStringLiteral("spacing"), 16);
    t.insert(QStringLiteral("titleBarHeight"), 44);
    theme_ = t;
}

QVariantMap DesktopQmlController::settingsToMap() const {
    QVariantMap map;
    map.insert(QStringLiteral("compilerType"), settings_.compilerType);
    map.insert(QStringLiteral("compilerPath"), settings_.compilerPath);
    map.insert(QStringLiteral("bibtexPath"), settings_.bibtexPath);
    map.insert(QStringLiteral("compileTimeoutSeconds"), settings_.compileTimeoutSeconds);
    map.insert(QStringLiteral("outputDirectory"), settings_.outputDirectory);
    map.insert(QStringLiteral("cleanBeforeCompile"), settings_.cleanBeforeCompile);
    map.insert(QStringLiteral("editorFontFamily"), settings_.editorFontFamily);
    map.insert(QStringLiteral("editorMonospaceOnly"), settings_.editorMonospaceOnly);
    map.insert(QStringLiteral("editorFontSize"), settings_.editorFontSize);
    map.insert(QStringLiteral("tabWidth"), settings_.tabWidth);
    map.insert(QStringLiteral("insertSpaces"), settings_.insertSpaces);
    map.insert(QStringLiteral("autoSaveEnabled"), settings_.autoSaveEnabled);
    map.insert(QStringLiteral("autoSaveDelayMs"), settings_.autoSaveDelayMs);
    map.insert(QStringLiteral("completionEnabled"), settings_.completionEnabled);
    map.insert(QStringLiteral("showLineNumbers"), settings_.showLineNumbers);
    map.insert(QStringLiteral("wordWrap"), settings_.wordWrap);
    map.insert(QStringLiteral("autoCloseBrackets"), settings_.autoCloseBrackets);
    map.insert(QStringLiteral("appTheme"), settings_.appTheme);
    map.insert(QStringLiteral("controlsStyle"), settings_.controlsStyle);
    map.insert(QStringLiteral("colorScheme"), settings_.colorScheme);
    map.insert(QStringLiteral("language"), settings_.language);
    map.insert(QStringLiteral("uiScalePercent"), settings_.uiScalePercent);
    return map;
}

QVariantMap DesktopQmlController::projectToMap(const pureleaf::Project& project) const {
    QVariantMap map;
    const QString rootPath = QString::fromStdString(project.rootPath);
    map.insert(QStringLiteral("id"), QString::fromStdString(project.id));
    map.insert(QStringLiteral("name"), QString::fromStdString(project.name));
    map.insert(QStringLiteral("rootPath"), rootPath);
    map.insert(QStringLiteral("mainTex"), QString::fromStdString(project.mainTex));
    map.insert(QStringLiteral("createdAt"), QVariant::fromValue<qlonglong>(project.createdAt));
    map.insert(QStringLiteral("updatedAt"), QVariant::fromValue<qlonglong>(project.updatedAt));
    map.insert(QStringLiteral("gitState"), QDir(rootPath).exists(QStringLiteral(".git"))
                                               ? QStringLiteral("Git")
                                               : QStringLiteral("本地"));
    return map;
}

QString DesktopQmlController::existingProjectIdForRoot(const QString& rootPath) const {
    if (!projectService_) {
        return {};
    }

    const auto result = projectService_->listProjects();
    if (!result.ok()) {
        return {};
    }

    const QString wanted = normalizedPath(rootPath);
    for (const pureleaf::Project& project : result.value()) {
        if (normalizedPath(QString::fromStdString(project.rootPath)) == wanted) {
            return QString::fromStdString(project.id);
        }
    }
    return {};
}

bool DesktopQmlController::activateProject(const QString& id) {
    if (!projectService_ || id.trimmed().isEmpty()) {
        return false;
    }

    const auto result = projectService_->getProject(id.toStdString());
    if (!result.ok()) {
        emit infoMessage(tr("Open project"),
                         tr("Failed to open project: %1").arg(errorText(result.error)));
        loadRecentProjects();
        return false;
    }

    activateProjectValue(result.value());
    setCurrentPage(QStringLiteral("editor"));
    return true;
}

void DesktopQmlController::activateProjectValue(const pureleaf::Project& project) {
    currentProject_ = projectToMap(project);
    projectRootPath_ = normalizedPath(QString::fromStdString(project.rootPath));
    emit currentProjectChanged();

    clearCompileOutput();
    refreshProjectFiles();

    const QString preferred = QString::fromStdString(project.mainTex);
    if (!preferred.isEmpty() && QFileInfo::exists(absoluteProjectPath(preferred))) {
        openProjectFile(preferred);
        return;
    }

    const QString mainTex = findMainTex();
    if (!mainTex.isEmpty()) {
        openProjectFile(relativeProjectPath(mainTex));
        return;
    }

    currentFilePath_.clear();
    currentFileContent_.clear();
    setEditorDirty(false);
    setEditorStatus(tr("No editable .tex file found."));
    emit currentFileChanged();
    emit currentFileContentChanged();
}

QString DesktopQmlController::currentPage() const {
    return currentPage_;
}

void DesktopQmlController::setCurrentPage(const QString& page) {
    if (currentPage_ == page) return;
    currentPage_ = page;
    emit currentPageChanged();
    emit windowProfileChanged();
}

QString DesktopQmlController::appVersion() const {
    return QString::fromStdString(pureleaf::getVersion().displayString());
}

QString DesktopQmlController::userDataDir() const {
    return fallbackUserDataDir();
}

QVariantList DesktopQmlController::recentProjects() const {
    return recentProjects_;
}

QVariantMap DesktopQmlController::currentProject() const {
    return currentProject_;
}

QVariantList DesktopQmlController::projectFiles() const {
    return projectFiles_;
}

QString DesktopQmlController::currentFileRelativePath() const {
    return currentFilePath_.isEmpty() ? QString() : relativeProjectPath(currentFilePath_);
}

QString DesktopQmlController::currentFileContent() const {
    return currentFileContent_;
}

bool DesktopQmlController::editorDirty() const {
    return editorDirty_;
}

QString DesktopQmlController::editorStatus() const {
    return editorStatus_;
}

bool DesktopQmlController::compiling() const {
    return compiling_;
}

QString DesktopQmlController::compileLog() const {
    return compileLog_;
}

QString DesktopQmlController::pdfPath() const {
    return pdfPath_;
}

QVariantMap DesktopQmlController::settings() const {
    return settingsToMap();
}

QVariantMap DesktopQmlController::theme() const {
    return theme_;
}

QVariantMap DesktopQmlController::windowProfile() const {
    QVariantMap profile;

    if (currentPage_ == QStringLiteral("editor")) {
        profile.insert(QStringLiteral("width"), 1280);
        profile.insert(QStringLiteral("height"), 820);
        profile.insert(QStringLiteral("minWidth"), 960);
        profile.insert(QStringLiteral("minHeight"), 600);
        return profile;
    }
    if (currentPage_ == QStringLiteral("settings")) {
        profile.insert(QStringLiteral("width"), 920);
        profile.insert(QStringLiteral("height"), 660);
        profile.insert(QStringLiteral("minWidth"), 760);
        profile.insert(QStringLiteral("minHeight"), 520);
        return profile;
    }

    const bool hasRecent = !recentProjects_.isEmpty();
    profile.insert(QStringLiteral("width"), hasRecent ? 1080 : 560);
    profile.insert(QStringLiteral("height"), hasRecent ? 700 : 460);
    profile.insert(QStringLiteral("minWidth"), hasRecent ? 720 : 520);
    profile.insert(QStringLiteral("minHeight"), hasRecent ? 520 : 420);
    return profile;
}

void DesktopQmlController::switchTo(const QString& page) {
    setCurrentPage(page);
}

void DesktopQmlController::backHome() {
    loadRecentProjects();
    setCurrentPage(QStringLiteral("home"));
}

void DesktopQmlController::createBlankProject(const QString& name) {
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        emit infoMessage(tr("Invalid name"), tr("Project name cannot be empty."));
        return;
    }
    if (!projectService_) {
        emit infoMessage(tr("Project service"), tr("Project database is not available."));
        return;
    }

    const QString dirName = sanitizedProjectDirName(trimmed);
    const QString rootPath =
        QDir(userDataDir()).filePath(QStringLiteral("projects/%1").arg(dirName));
    if (const QString existingId = existingProjectIdForRoot(rootPath); !existingId.isEmpty()) {
        activateProject(existingId);
        return;
    }

    const auto result = projectService_->createProject(trimmed.toStdString(),
                                                       normalizedPath(rootPath).toStdString());
    if (!result.ok()) {
        emit infoMessage(tr("Create project"),
                         tr("Failed to create project: %1").arg(errorText(result.error)));
        loadRecentProjects();
        return;
    }

    activateProjectValue(result.value());
    loadRecentProjects();
    setCurrentPage(QStringLiteral("editor"));
    emit infoMessage(tr("Project created"), tr("%1 is ready.").arg(trimmed));
}

void DesktopQmlController::openFolder(const QUrl& folderUrl) {
    if (!projectService_) {
        emit infoMessage(tr("Project service"), tr("Project database is not available."));
        return;
    }

    const QString localPath = normalizedPath(folderUrl.toLocalFile());
    if (localPath.isEmpty() || !QFileInfo(localPath).isDir()) {
        emit infoMessage(tr("Open folder"), tr("No valid folder selected."));
        return;
    }
    if (const QString existingId = existingProjectIdForRoot(localPath); !existingId.isEmpty()) {
        activateProject(existingId);
        return;
    }

    const QString name = leafNameForPath(localPath);
    const auto result =
        projectService_->registerProjectFolder(name.toStdString(), localPath.toStdString());
    if (!result.ok()) {
        emit infoMessage(tr("Open folder"),
                         tr("Failed to register folder: %1").arg(errorText(result.error)));
        loadRecentProjects();
        return;
    }

    activateProjectValue(result.value());
    loadRecentProjects();
    setCurrentPage(QStringLiteral("editor"));
}

void DesktopQmlController::openRecentProject(const QString& id) {
    activateProject(id);
}

void DesktopQmlController::removeRecentProject(const QString& id) {
    if (!projectService_ || id.isEmpty()) return;
    const auto result = projectService_->forgetProject(id.toStdString());
    if (!result.ok()) {
        emit infoMessage(tr("Remove project"),
                         tr("Failed to forget project: %1").arg(errorText(result.error)));
        return;
    }
    if (currentProject_.value(QStringLiteral("id")).toString() == id) {
        currentProject_.clear();
        emit currentProjectChanged();
    }
    loadRecentProjects();
}

QVariantMap DesktopQmlController::fileEntryToMap(const QString& absolutePath) const {
    const QFileInfo info(absolutePath);
    const QString relativePath = relativeProjectPath(absolutePath);
    QVariantMap map;
    map.insert(QStringLiteral("path"), relativePath);
    map.insert(QStringLiteral("name"), info.fileName());
    map.insert(QStringLiteral("isDir"), info.isDir());
    map.insert(QStringLiteral("depth"), relativePath.count(QLatin1Char('/')));
    map.insert(QStringLiteral("icon"),
               info.isDir() ? QStringLiteral("folder-open") : QStringLiteral("leaf"));
    map.insert(QStringLiteral("editable"), !info.isDir() && isEditableFile(absolutePath));
    return map;
}

QString DesktopQmlController::relativeProjectPath(const QString& absolutePath) const {
    if (projectRootPath_.isEmpty()) {
        return QString();
    }
    return QDir(projectRootPath_)
        .relativeFilePath(absolutePath)
        .replace(QLatin1Char('\\'), QLatin1Char('/'));
}

QString DesktopQmlController::absoluteProjectPath(const QString& relativePath) const {
    if (projectRootPath_.isEmpty()) {
        return QString();
    }
    return normalizedPath(QDir(projectRootPath_).filePath(relativePath));
}

QString DesktopQmlController::findMainTex() const {
    if (projectRootPath_.isEmpty()) {
        return {};
    }

    const QString mainTex = QDir(projectRootPath_).filePath(QStringLiteral("main.tex"));
    if (QFileInfo::exists(mainTex)) {
        return normalizedPath(mainTex);
    }

    QDirIterator it(projectRootPath_, {QStringLiteral("*.tex")}, QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = normalizedPath(it.next());
        if (!isSkippedProjectPath(relativeProjectPath(path))) {
            return path;
        }
    }
    return {};
}

bool DesktopQmlController::isPathInsideProject(const QString& absolutePath) const {
    if (projectRootPath_.isEmpty()) {
        return false;
    }
    const QFileInfo info(absolutePath);
    QString cleanPath = info.exists() ? info.canonicalFilePath() : info.absoluteFilePath();
    cleanPath = QDir::cleanPath(cleanPath);
    const QString cleanRoot = QDir::cleanPath(projectRootPath_);
#ifdef Q_OS_WIN
    return cleanPath == cleanRoot ||
           cleanPath.startsWith(cleanRoot + QLatin1Char('/'), Qt::CaseInsensitive) ||
           cleanPath.startsWith(cleanRoot + QLatin1Char('\\'), Qt::CaseInsensitive);
#else
    return cleanPath == cleanRoot || cleanPath.startsWith(cleanRoot + QLatin1Char('/'));
#endif
}

bool DesktopQmlController::isEditableFile(const QString& absolutePath) const {
    const QFileInfo info(absolutePath);
    return info.isFile() && isEditableExtension(info.suffix()) && info.size() <= 4 * 1024 * 1024;
}

void DesktopQmlController::setEditorDirty(bool dirty) {
    if (editorDirty_ == dirty) {
        return;
    }
    editorDirty_ = dirty;
    emit editorDirtyChanged();
}

void DesktopQmlController::setEditorStatus(const QString& status) {
    if (editorStatus_ == status) {
        return;
    }
    editorStatus_ = status;
    emit editorStatusChanged();
}

void DesktopQmlController::setCompiling(bool compiling) {
    if (compiling_ == compiling) {
        return;
    }
    compiling_ = compiling;
    emit compilingChanged();
}

void DesktopQmlController::clearCompileOutput() {
    compileLog_.clear();
    pdfPath_.clear();
    emit compileLogChanged();
    emit pdfPathChanged();
}

void DesktopQmlController::appendCompileLog(const QString& text) {
    if (text.isEmpty()) {
        return;
    }
    compileLog_.append(text);
    emit compileLogChanged();
}

void DesktopQmlController::refreshProjectFiles() {
    projectFiles_.clear();
    if (projectRootPath_.isEmpty() || !QFileInfo(projectRootPath_).isDir()) {
        emit projectFilesChanged();
        return;
    }

    QStringList paths;
    QDirIterator it(projectRootPath_, QDir::AllEntries | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = normalizedPath(it.next());
        const QString relative = relativeProjectPath(path);
        if (relative.isEmpty() || isSkippedProjectPath(relative)) {
            continue;
        }
        paths.append(path);
        if (paths.size() >= 400) {
            break;
        }
    }

    std::sort(paths.begin(), paths.end(), [this](const QString& lhs, const QString& rhs) {
        const QFileInfo leftInfo(lhs);
        const QFileInfo rightInfo(rhs);
        if (leftInfo.isDir() != rightInfo.isDir()) {
            return leftInfo.isDir();
        }
        return relativeProjectPath(lhs).compare(relativeProjectPath(rhs), Qt::CaseInsensitive) < 0;
    });

    for (const QString& path : paths) {
        projectFiles_.append(fileEntryToMap(path));
    }
    emit projectFilesChanged();
}

void DesktopQmlController::openProjectFile(const QString& relativePath) {
    if (projectRootPath_.isEmpty()) {
        return;
    }
    if (editorDirty_ && !saveCurrentFile()) {
        return;
    }

    const QString absolutePath = absoluteProjectPath(relativePath);
    if (!isPathInsideProject(absolutePath)) {
        emit infoMessage(tr("Open file"), tr("File is outside the current project."));
        return;
    }
    if (!isEditableFile(absolutePath)) {
        setEditorStatus(tr("This file type is not editable yet."));
        return;
    }

    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit infoMessage(tr("Open file"), file.errorString());
        return;
    }

    loadingFile_ = true;
    currentFilePath_ = normalizedPath(absolutePath);
    currentFileContent_ = QString::fromUtf8(file.readAll());
    loadingFile_ = false;
    setEditorDirty(false);
    setEditorStatus(tr("Ready"));
    emit currentFileChanged();
    emit currentFileContentChanged();
}

void DesktopQmlController::updateCurrentFileContent(const QString& content) {
    if (loadingFile_ || currentFilePath_.isEmpty() || currentFileContent_ == content) {
        return;
    }
    currentFileContent_ = content;
    setEditorDirty(true);
    setEditorStatus(tr("Unsaved changes"));
    if (settings_.autoSaveEnabled) {
        autoSaveTimer_.start(settings_.autoSaveDelayMs);
    }
}

bool DesktopQmlController::saveCurrentFile() {
    if (currentFilePath_.isEmpty() || !isPathInsideProject(currentFilePath_)) {
        return false;
    }

    QSaveFile file(currentFilePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit infoMessage(tr("Save failed"), file.errorString());
        return false;
    }

    file.write(currentFileContent_.toUtf8());
    if (!file.commit()) {
        emit infoMessage(tr("Save failed"), file.errorString());
        return false;
    }

    setEditorDirty(false);
    setEditorStatus(tr("Saved"));
    refreshProjectFiles();
    return true;
}

void DesktopQmlController::compileCurrentProject() {
    if (projectRootPath_.isEmpty()) {
        return;
    }
    if (editorDirty_ && !saveCurrentFile()) {
        return;
    }
    if (compileProcess_.state() != QProcess::NotRunning) {
        emit infoMessage(tr("Compile"), tr("The project is already compiling."));
        return;
    }

    const QString mainTex = findMainTex();
    if (mainTex.isEmpty()) {
        emit infoMessage(tr("Compile"), tr("No main.tex or other .tex file found."));
        return;
    }

    const QDir projectDir(projectRootPath_);
    const QString configuredOutputDir = settings_.outputDirectory.trimmed().isEmpty()
                                            ? QStringLiteral("build")
                                            : settings_.outputDirectory.trimmed();
    const QString outputDir = QDir::cleanPath(QFileInfo(configuredOutputDir).isAbsolute()
                                                  ? configuredOutputDir
                                                  : projectDir.filePath(configuredOutputDir));
    if (!QDir().mkpath(outputDir)) {
        emit infoMessage(tr("Compile"), tr("Failed to create output directory."));
        return;
    }

    if (settings_.cleanBeforeCompile && isPathInsideProject(outputDir)) {
        const QStringList auxiliaryPatterns = {
            QStringLiteral("*.aux"),         QStringLiteral("*.log"),
            QStringLiteral("*.out"),         QStringLiteral("*.toc"),
            QStringLiteral("*.synctex.gz"),  QStringLiteral("*.fls"),
            QStringLiteral("*.fdb_latexmk"),
        };
        QDir cleanDir(outputDir);
        for (const QString& fileName :
             cleanDir.entryList(auxiliaryPatterns, QDir::Files | QDir::NoDotAndDotDot)) {
            cleanDir.remove(fileName);
        }
    }

    clearCompileOutput();
    const QString compilerProgram = settings_.compilerPath.trimmed().isEmpty()
                                        ? settings_.compilerType
                                        : settings_.compilerPath.trimmed();
    appendCompileLog(
        tr("Compile: %1\nCompiler: %2\n").arg(relativeProjectPath(mainTex), compilerProgram));
    setEditorStatus(tr("Compiling..."));
    setCompiling(true);

    compileProcess_.setProgram(compilerProgram);
    compileProcess_.setWorkingDirectory(projectRootPath_);
    compileProcess_.setArguments({
        QStringLiteral("-interaction=nonstopmode"),
        QStringLiteral("-synctex=1"),
        QStringLiteral("-output-directory=%1").arg(outputDir),
        relativeProjectPath(mainTex),
    });
    compileProcess_.start();
    if (!compileProcess_.waitForStarted(500)) {
        setCompiling(false);
        setEditorStatus(tr("Compiler failed to start"));
        appendCompileLog(
            tr("Failed to start compiler. Check LaTeX installation or compiler path.\n"));
        return;
    }
    compileTimeoutTimer_.start(settings_.compileTimeoutSeconds * 1000);
}

void DesktopQmlController::finishCompile(int exitCode, QProcess::ExitStatus exitStatus) {
    compileTimeoutTimer_.stop();
    setCompiling(false);

    const QString mainTex = findMainTex();
    const QString pdfName = QFileInfo(mainTex).completeBaseName() + QStringLiteral(".pdf");
    const QString configuredOutputDir = settings_.outputDirectory.trimmed().isEmpty()
                                            ? QStringLiteral("build")
                                            : settings_.outputDirectory.trimmed();
    const QString outputDir =
        QDir::cleanPath(QFileInfo(configuredOutputDir).isAbsolute()
                            ? configuredOutputDir
                            : QDir(projectRootPath_).filePath(configuredOutputDir));
    const QString outputPdfPath = QDir(outputDir).filePath(pdfName);

    if (exitStatus == QProcess::NormalExit && exitCode == 0 && QFileInfo::exists(outputPdfPath)) {
        pdfPath_ = normalizedPath(outputPdfPath);
        emit pdfPathChanged();
        setEditorStatus(tr("PDF generated"));
        appendCompileLog(tr("\nCompile finished.\n"));
        return;
    }

    setEditorStatus(tr("Compile failed"));
    appendCompileLog(tr("\nCompile failed, exit code: %1\n").arg(exitCode));
}

void DesktopQmlController::openPdf() {
    if (pdfPath_.isEmpty() || !QFileInfo::exists(pdfPath_)) {
        emit infoMessage(tr("Open PDF"), tr("No generated PDF is available."));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(pdfPath_));
}

void DesktopQmlController::applySettings(const QVariantMap& patch) {
    if (patch.isEmpty()) {
        emit settingsChanged();
        return;
    }

    const QString oldControlsStyle = settings_.controlsStyle;
    auto stringPatch = [&patch](const QString& key, QString& value) {
        if (patch.contains(key)) value = patch.value(key).toString().trimmed();
    };
    auto boolPatch = [&patch](const QString& key, bool& value) {
        if (patch.contains(key)) value = patch.value(key).toBool();
    };
    auto intPatch = [&patch](const QString& key, int& value, int min, int max) {
        if (patch.contains(key)) value = qBound(min, patch.value(key).toInt(), max);
    };

    stringPatch(QStringLiteral("compilerType"), settings_.compilerType);
    stringPatch(QStringLiteral("compilerPath"), settings_.compilerPath);
    stringPatch(QStringLiteral("bibtexPath"), settings_.bibtexPath);
    intPatch(QStringLiteral("compileTimeoutSeconds"), settings_.compileTimeoutSeconds, 5, 600);
    stringPatch(QStringLiteral("outputDirectory"), settings_.outputDirectory);
    boolPatch(QStringLiteral("cleanBeforeCompile"), settings_.cleanBeforeCompile);
    stringPatch(QStringLiteral("editorFontFamily"), settings_.editorFontFamily);
    boolPatch(QStringLiteral("editorMonospaceOnly"), settings_.editorMonospaceOnly);
    intPatch(QStringLiteral("editorFontSize"), settings_.editorFontSize, 8, 36);
    intPatch(QStringLiteral("tabWidth"), settings_.tabWidth, 2, 8);
    boolPatch(QStringLiteral("insertSpaces"), settings_.insertSpaces);
    boolPatch(QStringLiteral("autoSaveEnabled"), settings_.autoSaveEnabled);
    intPatch(QStringLiteral("autoSaveDelayMs"), settings_.autoSaveDelayMs, 500, 10000);
    boolPatch(QStringLiteral("completionEnabled"), settings_.completionEnabled);
    boolPatch(QStringLiteral("showLineNumbers"), settings_.showLineNumbers);
    boolPatch(QStringLiteral("wordWrap"), settings_.wordWrap);
    boolPatch(QStringLiteral("autoCloseBrackets"), settings_.autoCloseBrackets);
    stringPatch(QStringLiteral("appTheme"), settings_.appTheme);
    stringPatch(QStringLiteral("controlsStyle"), settings_.controlsStyle);
    stringPatch(QStringLiteral("colorScheme"), settings_.colorScheme);
    stringPatch(QStringLiteral("language"), settings_.language);
    intPatch(QStringLiteral("uiScalePercent"), settings_.uiScalePercent, 80, 160);

    if (settings_.outputDirectory.trimmed().isEmpty()) {
        settings_.outputDirectory = QStringLiteral("build");
    }
    if (settings_.controlsStyle.trimmed().isEmpty()) {
        settings_.controlsStyle = QStringLiteral("platform");
    }

    pureleaf::ui::saveAppSettings(settings_);
    emit settingsChanged();

    recomputeTheme();
    emit themeChanged();

    if (oldControlsStyle != settings_.controlsStyle) {
        emit infoMessage(tr("Style saved"), tr("Restart PureLeaf to apply the controls style."));
    } else {
        emit infoMessage(tr("Settings saved"), tr("Preferences have been updated."));
    }
}

}  // namespace pureleaf::qml
