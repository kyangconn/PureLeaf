#pragma once

#include <QList>
#include <QMainWindow>
#include <QSize>

#include "pages/home/homepage.h"

class QEvent;
class QCloseEvent;
class QLabel;
class QStackedWidget;
class QToolButton;
class QWidget;

namespace QWK {
class WidgetWindowAgent;
}

namespace pureleaf::ui {

class Navigator;

/// Main application window.
///
/// Owns the QWindowKit-backed window chrome, the QStackedWidget page
/// container, and the Navigator.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void applyInitialWindowSize();
    void applyHomeWindowSize();
    void applyEditorWindowSize();
    void applySettingsWindowSize();
    void applyWindowSizeProfile(const QSize& targetSize, const QSize& minimumSize);
    void setupWindowChrome();
    void setupPages();
    void wireNavigation();
    void updateWindowChrome();
    void loadRecentProjects();
    void saveRecentProjects() const;
    void rememberProject(const QString& rootPath);
    void removeRecentProject(const QString& projectKey);
    void openProject(const QString& rootPath);
    void createBlankProject();

    QWK::WidgetWindowAgent* windowAgent_;
    QWidget* titleBar_;
    QLabel* titleLabel_;
    QToolButton* maximizeButton_;

    QStackedWidget* stack_;
    Navigator* navigator_;

    // Pages (owned by stack_).
    class HomePage* homePage_;
    class EditorPage* editorPage_;
    class SettingsPage* settingsPage_;

    QList<RecentProject> recentProjects_;
};

}  // namespace pureleaf::ui
