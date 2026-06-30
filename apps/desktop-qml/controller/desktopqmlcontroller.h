#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

#include "core/appsettings.h"

namespace pureleaf {
class Database;
class ProjectLockManager;
class ProjectService;
struct Project;
}  // namespace pureleaf

namespace pureleaf::qml {

/// Thin bridge between the QML spike UI and the PureLeaf core/platform layer.
///
/// Per todo.md §3 (C++/QML 边界): the first phase exposes mock / read-only
/// data — recent projects, app version, settings snapshot, theme palette —
/// plus navigation and project "intent" entry points. QML never touches
/// SQLite / JSON / QSettings directly; everything flows through here.
///
/// Theme colors live on the controller (rather than a QML singleton) so they
/// are reachable from every page through the context property without
/// cross-module type resolution.
class DesktopQmlController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString userDataDir READ userDataDir CONSTANT)
    Q_PROPERTY(QVariantList recentProjects READ recentProjects NOTIFY recentProjectsChanged)
    Q_PROPERTY(QVariantMap currentProject READ currentProject NOTIFY currentProjectChanged)
    Q_PROPERTY(QVariantList projectFiles READ projectFiles NOTIFY projectFilesChanged)
    Q_PROPERTY(
        QString currentFileRelativePath READ currentFileRelativePath NOTIFY currentFileChanged)
    Q_PROPERTY(QString currentFileContent READ currentFileContent NOTIFY currentFileContentChanged)
    Q_PROPERTY(bool editorDirty READ editorDirty NOTIFY editorDirtyChanged)
    Q_PROPERTY(QString editorStatus READ editorStatus NOTIFY editorStatusChanged)
    Q_PROPERTY(bool compiling READ compiling NOTIFY compilingChanged)
    Q_PROPERTY(QString compileLog READ compileLog NOTIFY compileLogChanged)
    Q_PROPERTY(QString pdfPath READ pdfPath NOTIFY pdfPathChanged)
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)
    Q_PROPERTY(QVariantMap theme READ theme NOTIFY themeChanged)
    Q_PROPERTY(QVariantMap windowProfile READ windowProfile NOTIFY windowProfileChanged)

public:
    explicit DesktopQmlController(pureleaf::ui::AppSettings settings, QObject* parent = nullptr);
    ~DesktopQmlController() override;

    QString currentPage() const;
    void setCurrentPage(const QString& page);

    QString appVersion() const;
    QString userDataDir() const;

    QVariantList recentProjects() const;
    QVariantMap currentProject() const;
    QVariantList projectFiles() const;
    QString currentFileRelativePath() const;
    QString currentFileContent() const;
    bool editorDirty() const;
    QString editorStatus() const;
    bool compiling() const;
    QString compileLog() const;
    QString pdfPath() const;
    QVariantMap settings() const;
    QVariantMap theme() const;
    QVariantMap windowProfile() const;

    // ── Navigation ───────────────────────────────────────────────
    Q_INVOKABLE void switchTo(const QString& page);
    Q_INVOKABLE void backHome();

    // ── Project intent (mock data for the spike) ─────────────────
    Q_INVOKABLE void createBlankProject(const QString& name);
    Q_INVOKABLE void openFolder(const QUrl& folderUrl);
    Q_INVOKABLE void openRecentProject(const QString& id);
    Q_INVOKABLE void removeRecentProject(const QString& id);

    // ── Editor / compile ────────────────────────────────────────
    Q_INVOKABLE void refreshProjectFiles();
    Q_INVOKABLE void openProjectFile(const QString& relativePath);
    Q_INVOKABLE void updateCurrentFileContent(const QString& content);
    Q_INVOKABLE bool saveCurrentFile();
    Q_INVOKABLE void compileCurrentProject();
    Q_INVOKABLE void openPdf();

    // ── Settings ─────────────────────────────────────────────────
    /// Applies a partial settings patch (key → value). Updates the snapshot,
    /// recomputes the theme palette, and emits settingsChanged + themeChanged.
    Q_INVOKABLE void applySettings(const QVariantMap& patch);

signals:
    void currentPageChanged();
    void recentProjectsChanged();
    void currentProjectChanged();
    void projectFilesChanged();
    void currentFileChanged();
    void currentFileContentChanged();
    void editorDirtyChanged();
    void editorStatusChanged();
    void compilingChanged();
    void compileLogChanged();
    void pdfPathChanged();
    void settingsChanged();
    void themeChanged();
    void windowProfileChanged();
    void infoMessage(const QString& title, const QString& message);

private:
    void initServices();
    void loadRecentProjects();
    void recomputeTheme();
    bool effectiveDark() const;
    QVariantMap settingsToMap() const;
    QVariantMap projectToMap(const pureleaf::Project& project) const;
    QString existingProjectIdForRoot(const QString& rootPath) const;
    bool activateProject(const QString& id);
    void activateProjectValue(const pureleaf::Project& project);
    QVariantMap fileEntryToMap(const QString& absolutePath) const;
    QString relativeProjectPath(const QString& absolutePath) const;
    QString absoluteProjectPath(const QString& relativePath) const;
    QString findMainTex() const;
    bool isPathInsideProject(const QString& absolutePath) const;
    bool isEditableFile(const QString& absolutePath) const;
    void setEditorDirty(bool dirty);
    void setEditorStatus(const QString& status);
    void setCompiling(bool compiling);
    void clearCompileOutput();
    void appendCompileLog(const QString& text);
    void finishCompile(int exitCode, QProcess::ExitStatus exitStatus);

    QString currentPage_;
    QVariantList recentProjects_;
    QVariantMap currentProject_;
    QVariantList projectFiles_;
    QString projectRootPath_;
    QString currentFilePath_;
    QString currentFileContent_;
    QString editorStatus_;
    QString compileLog_;
    QString pdfPath_;
    bool editorDirty_ = false;
    bool loadingFile_ = false;
    bool compiling_ = false;
    pureleaf::ui::AppSettings settings_;
    QVariantMap theme_;

    QTimer autoSaveTimer_;
    QTimer compileTimeoutTimer_;
    QProcess compileProcess_;

    std::unique_ptr<pureleaf::Database> database_;
    std::unique_ptr<pureleaf::ProjectLockManager> lockManager_;
    std::unique_ptr<pureleaf::ProjectService> projectService_;
};

}  // namespace pureleaf::qml
