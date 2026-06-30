#pragma once

#include <QProcess>
#include <QString>

#include "core/appsettings.h"
#include "core/navpage.h"

class QFileSystemModel;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTimer;
class QTreeView;

namespace pureleaf::ui {

/// Editor page — the main writing surface.
///
/// Layout (from todo.md): file tree (left) | code editor (center) | PDF preview (right).
/// Top bar has: back button, project name (center), action buttons (right).
class EditorPage : public NavPage {
    Q_OBJECT

public:
    explicit EditorPage(QWidget* parent = nullptr);

    void onPageEntered(const QVariant& payload) override;
    void onPageLeft() override;
    void applySettings(AppSettings settings);

signals:
    /// Emitted when the user clicks "back" to return home.
    void backRequested();

private:
    void setupUi();
    void loadProject(const QString& rootPath);
    void openFile(const QString& filePath);
    bool saveCurrentFile();
    void markDirty(bool dirty);
    void scheduleAutoSave();
    void compileProject();
    void finishCompile(int exitCode, QProcess::ExitStatus exitStatus);
    void appendCompileLog(const QString& text);
    void showFileContextMenu(const QPoint& pos);
    void createFile(bool directory);
    void renameSelectedEntry();
    void deleteSelectedEntry();
    QString selectedPath() const;
    QString relativeProjectPath(const QString& absolutePath) const;
    QString findMainTex() const;
    bool isPathInsideProject(const QString& absolutePath) const;
    void updateEditorTitle();

    QString currentProjectId_;
    QString projectRootPath_;
    QString currentFilePath_;
    QString lastPdfPath_;
    AppSettings settings_;
    bool isDirty_;
    bool loadingFile_;
    int autoSaveDelayMs_;
    int compileTimeoutMs_;
    QLabel* projectNameLabel_;
    QLabel* fileNameLabel_;
    QLabel* statusLabel_;
    QLabel* pdfStatusLabel_;
    QFileSystemModel* fileModel_;
    QTreeView* fileTree_;
    QPlainTextEdit* editor_;
    QPlainTextEdit* compileLog_;
    QPushButton* saveButton_;
    QPushButton* compileButton_;
    QPushButton* openPdfButton_;
    QTimer* autoSaveTimer_;
    QTimer* compileTimeoutTimer_;
    QProcess* compileProcess_;
};

}  // namespace pureleaf::ui
