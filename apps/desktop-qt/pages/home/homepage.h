#pragma once

#include <QList>
#include <QString>

#include "core/navpage.h"

class QHBoxLayout;
class QSpacerItem;
class QVBoxLayout;
class QWidget;

namespace pureleaf::ui {

struct RecentProject {
    enum class GitState {
        Unknown,
        NoRepository,
        Clean,
        Modified,
        Conflict,
    };

    QString id;
    QString name;
    QString rootPath;
    qint64 characterCount = 0;
    GitState gitState = GitState::Unknown;
};

/// Home page — project list / launcher.
/// Shows primary workspace actions and the recent-project list.
class HomePage : public NavPage {
    Q_OBJECT

public:
    explicit HomePage(QWidget* parent = nullptr);

    void onPageEntered(const QVariant& payload) override;
    void setRecentProjects(const QList<RecentProject>& projects);

signals:
    /// Emitted when the user wants to open a project.
    void projectOpenRequested(const QString& projectKey);
    void newProjectRequested();
    void openFolderRequested();
    void recentProjectRemoved(const QString& projectKey);

private:
    void setupUi();
    void rebuildRecentList();
    void updateRecentPanelVisibility();
    QWidget* createRecentProjectRow(const RecentProject& project);
    static QString projectKey(const RecentProject& project);

    QList<RecentProject> recentProjects_;
    QHBoxLayout* rootLayout_;
    QWidget* actionPanel_;
    QWidget* recentPanel_;
    QSpacerItem* leadingSpacer_;
    QSpacerItem* trailingSpacer_;
    QVBoxLayout* recentListLayout_;
};

}  // namespace pureleaf::ui
