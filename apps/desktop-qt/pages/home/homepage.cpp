#include "homepage.h"

#include <QAction>
#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include "components/icons/appicon.h"

namespace pureleaf::ui {

HomePage::HomePage(QWidget* parent)
    : NavPage(parent),
      rootLayout_(nullptr),
      actionPanel_(nullptr),
      recentPanel_(nullptr),
      leadingSpacer_(nullptr),
      trailingSpacer_(nullptr),
      recentListLayout_(nullptr) {
    setupUi();
}

void HomePage::setupUi() {
    setObjectName(QStringLiteral("homePage"));

    rootLayout_ = new QHBoxLayout(this);
    rootLayout_->setContentsMargins(0, 0, 0, 0);
    rootLayout_->setSpacing(0);

    leadingSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    trailingSpacer_ = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);

    actionPanel_ = new QWidget(this);
    actionPanel_->setObjectName(QStringLiteral("homeActionPanel"));
    actionPanel_->setMinimumWidth(320);
    actionPanel_->setMaximumWidth(440);
    actionPanel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    auto* actionLayout = new QVBoxLayout(actionPanel_);
    actionLayout->setContentsMargins(48, 48, 48, 48);
    actionLayout->setSpacing(12);
    actionLayout->addStretch();

    auto* welcomeTitle = new QLabel(tr("开始写作"), actionPanel_);
    welcomeTitle->setObjectName(QStringLiteral("homeWelcomeTitle"));

    auto* welcomeText = new QLabel(tr("创建一个空白项目，或打开已有的本地文件夹。"), actionPanel_);
    welcomeText->setObjectName(QStringLiteral("homeWelcomeText"));
    welcomeText->setWordWrap(true);

    auto* newProjectButton = new QPushButton(tr("新建空白项目"), actionPanel_);
    newProjectButton->setObjectName(QStringLiteral("newProjectButton"));
    newProjectButton->setMinimumHeight(46);
    newProjectButton->setIcon(appIcon(AppIcon::NewProject, QColor(QStringLiteral("#ffffff"))));
    newProjectButton->setIconSize(QSize(19, 19));

    auto* openFolderButton = new QPushButton(tr("打开本地文件夹"), actionPanel_);
    openFolderButton->setObjectName(QStringLiteral("openFolderButton"));
    openFolderButton->setMinimumHeight(46);
    openFolderButton->setIcon(appIcon(AppIcon::OpenFolder, QColor(QStringLiteral("#334155")),
                                      QColor(QStringLiteral("#0f172a"))));
    openFolderButton->setIconSize(QSize(19, 19));

    actionLayout->addWidget(welcomeTitle);
    actionLayout->addWidget(welcomeText);
    actionLayout->addSpacing(16);
    actionLayout->addWidget(newProjectButton);
    actionLayout->addWidget(openFolderButton);
    actionLayout->addStretch();

    recentPanel_ = new QWidget(this);
    recentPanel_->setObjectName(QStringLiteral("recentProjectsPanel"));
    auto* recentPanelLayout = new QVBoxLayout(recentPanel_);
    recentPanelLayout->setContentsMargins(48, 42, 48, 42);
    recentPanelLayout->setSpacing(8);

    auto* recentTitle = new QLabel(tr("最近项目"), recentPanel_);
    recentTitle->setObjectName(QStringLiteral("recentProjectsTitle"));
    auto* recentSubtitle = new QLabel(tr("继续上次的本地 LaTeX 工作区"), recentPanel_);
    recentSubtitle->setObjectName(QStringLiteral("recentProjectsSubtitle"));

    auto* scrollArea = new QScrollArea(recentPanel_);
    scrollArea->setObjectName(QStringLiteral("recentProjectsScrollArea"));
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* listContainer = new QWidget(scrollArea);
    listContainer->setObjectName(QStringLiteral("recentProjectsList"));
    recentListLayout_ = new QVBoxLayout(listContainer);
    recentListLayout_->setContentsMargins(0, 12, 0, 0);
    recentListLayout_->setSpacing(8);
    recentListLayout_->setAlignment(Qt::AlignTop);
    scrollArea->setWidget(listContainer);

    recentPanelLayout->addWidget(recentTitle);
    recentPanelLayout->addWidget(recentSubtitle);
    recentPanelLayout->addWidget(scrollArea, 1);

    rootLayout_->addItem(leadingSpacer_);
    rootLayout_->addWidget(actionPanel_, 2);
    rootLayout_->addWidget(recentPanel_, 5);
    rootLayout_->addItem(trailingSpacer_);

    connect(newProjectButton, &QPushButton::clicked, this, &HomePage::newProjectRequested);
    connect(openFolderButton, &QPushButton::clicked, this, &HomePage::openFolderRequested);

    setStyleSheet(QStringLiteral(R"(
        QWidget#homePage {
            background: #f8fafc;
        }

        QWidget#homeActionPanel {
            background: #f1f5f9;
            border-right: 1px solid #e2e8f0;
        }

        QWidget#homeActionPanel[centered="true"] {
            border: 1px solid #e2e8f0;
            border-radius: 18px;
        }

        QLabel#homeWelcomeTitle, QLabel#recentProjectsTitle {
            color: #0f172a;
            font-size: 24px;
            font-weight: 700;
        }

        QLabel#homeWelcomeText, QLabel#recentProjectsSubtitle {
            color: #64748b;
            font-size: 13px;
        }

        QPushButton#newProjectButton, QPushButton#openFolderButton {
            border-radius: 8px;
            padding: 0 18px;
            text-align: left;
            font-size: 14px;
            font-weight: 600;
        }

        QPushButton#newProjectButton {
            color: white;
            background: #16a34a;
            border: 1px solid #16a34a;
        }

        QPushButton#newProjectButton:hover {
            background: #15803d;
            border-color: #15803d;
        }

        QPushButton#openFolderButton {
            color: #334155;
            background: white;
            border: 1px solid #cbd5e1;
        }

        QPushButton#openFolderButton:hover {
            color: #0f172a;
            background: #f8fafc;
            border-color: #94a3b8;
        }

        QScrollArea#recentProjectsScrollArea, QWidget#recentProjectsList {
            background: transparent;
        }

        QFrame#recentProjectRow {
            background: white;
            border: 1px solid #e2e8f0;
            border-radius: 10px;
        }

        QFrame#recentProjectRow:hover {
            border-color: #94a3b8;
        }

        QPushButton#recentProjectName {
            color: #0f172a;
            background: transparent;
            border: none;
            padding: 0;
            text-align: left;
            font-size: 14px;
            font-weight: 600;
        }

        QPushButton#recentProjectName:hover {
            color: #15803d;
        }

        QLabel#recentProjectPath, QLabel#recentProjectMeta {
            color: #64748b;
            font-size: 12px;
        }

        QWidget#recentProjectGit QLabel { font-size: 12px; }
        QWidget#recentProjectGit[gitState="clean"] QLabel { color: #15803d; }
        QWidget#recentProjectGit[gitState="modified"] QLabel { color: #b45309; }
        QWidget#recentProjectGit[gitState="conflict"] QLabel { color: #dc2626; }
        QWidget#recentProjectGit[gitState="neutral"] QLabel { color: #64748b; }

        QToolButton#recentProjectMore {
            color: #475569;
            background: transparent;
            border: none;
            border-radius: 6px;
            font-size: 20px;
        }

        QToolButton#recentProjectMore:hover {
            color: #0f172a;
            background: #e2e8f0;
        }

    )"));

    rebuildRecentList();
}

void HomePage::onPageEntered(const QVariant& payload) {
    (void)payload;
    // TODO: refresh recent projects from the workspace repository.
}

void HomePage::setRecentProjects(const QList<RecentProject>& projects) {
    recentProjects_ = projects;
    rebuildRecentList();
}

void HomePage::rebuildRecentList() {
    while (QLayoutItem* item = recentListLayout_->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    updateRecentPanelVisibility();

    if (recentProjects_.isEmpty()) {
        return;
    }

    for (const RecentProject& project : recentProjects_) {
        recentListLayout_->addWidget(createRecentProjectRow(project));
    }
}

void HomePage::updateRecentPanelVisibility() {
    if (!recentPanel_ || !actionPanel_ || !leadingSpacer_ || !trailingSpacer_) {
        return;
    }

    const bool hasRecentProjects = !recentProjects_.isEmpty();
    recentPanel_->setVisible(hasRecentProjects);
    actionPanel_->setProperty("centered", !hasRecentProjects);

    if (hasRecentProjects) {
        leadingSpacer_->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
        trailingSpacer_->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
        actionPanel_->setMinimumWidth(320);
        actionPanel_->setMaximumWidth(440);
        actionPanel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    } else {
        leadingSpacer_->changeSize(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        trailingSpacer_->changeSize(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        actionPanel_->setMinimumWidth(360);
        actionPanel_->setMaximumWidth(460);
        actionPanel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }

    actionPanel_->style()->unpolish(actionPanel_);
    actionPanel_->style()->polish(actionPanel_);
    if (rootLayout_) {
        rootLayout_->invalidate();
    }
}

QWidget* HomePage::createRecentProjectRow(const RecentProject& project) {
    auto* row = new QFrame(this);
    row->setObjectName(QStringLiteral("recentProjectRow"));
    row->setMinimumHeight(74);

    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(16, 10, 10, 10);
    rowLayout->setSpacing(16);

    auto* identity = new QWidget(row);
    auto* identityLayout = new QVBoxLayout(identity);
    identityLayout->setContentsMargins(0, 0, 0, 0);
    identityLayout->setSpacing(3);

    auto* nameButton = new QPushButton(project.name, identity);
    nameButton->setObjectName(QStringLiteral("recentProjectName"));
    nameButton->setCursor(Qt::PointingHandCursor);

    auto* pathLabel = new QLabel(project.rootPath, identity);
    pathLabel->setObjectName(QStringLiteral("recentProjectPath"));
    pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    identityLayout->addWidget(nameButton);
    identityLayout->addWidget(pathLabel);

    auto* characterCount =
        new QLabel(tr("%1 字符").arg(QLocale().toString(project.characterCount)), row);
    characterCount->setObjectName(QStringLiteral("recentProjectMeta"));
    characterCount->setMinimumWidth(90);

    auto* gitStatus = new QWidget(row);
    gitStatus->setObjectName(QStringLiteral("recentProjectGit"));
    gitStatus->setMinimumWidth(104);
    auto* gitStatusLayout = new QHBoxLayout(gitStatus);
    gitStatusLayout->setContentsMargins(0, 0, 0, 0);
    gitStatusLayout->setSpacing(6);
    auto* gitStatusIcon = new QLabel(gitStatus);
    auto* gitStatusText = new QLabel(gitStatus);

    QColor gitColor(QStringLiteral("#64748b"));
    QString gitText;
    const char* gitState = "neutral";
    switch (project.gitState) {
        case RecentProject::GitState::Clean:
            gitText = tr("Git 干净");
            gitColor = QColor(QStringLiteral("#15803d"));
            gitState = "clean";
            break;
        case RecentProject::GitState::Modified:
            gitText = tr("有更改");
            gitColor = QColor(QStringLiteral("#b45309"));
            gitState = "modified";
            break;
        case RecentProject::GitState::Conflict:
            gitText = tr("有冲突");
            gitColor = QColor(QStringLiteral("#dc2626"));
            gitState = "conflict";
            break;
        case RecentProject::GitState::NoRepository:
            gitText = tr("无 Git");
            break;
        case RecentProject::GitState::Unknown:
            gitText = tr("未检测");
            break;
    }
    gitStatus->setProperty("gitState", gitState);
    gitStatusIcon->setPixmap(appIconPixmap(AppIcon::GitBranch, QSize(16, 16), gitColor));
    gitStatusText->setText(gitText);
    gitStatusLayout->addWidget(gitStatusIcon);
    gitStatusLayout->addWidget(gitStatusText);
    gitStatusLayout->addStretch();

    auto* moreButton = new QToolButton(row);
    moreButton->setObjectName(QStringLiteral("recentProjectMore"));
    moreButton->setIcon(appIcon(AppIcon::More, QColor(QStringLiteral("#475569")),
                                QColor(QStringLiteral("#0f172a"))));
    moreButton->setIconSize(QSize(18, 18));
    moreButton->setToolTip(tr("更多"));
    moreButton->setFixedSize(36, 36);
    moreButton->setPopupMode(QToolButton::InstantPopup);

    auto* menu = new QMenu(moreButton);
    QAction* openAction = menu->addAction(
        appIcon(AppIcon::OpenFolder, QColor(QStringLiteral("#475569"))), tr("打开"));
    QAction* removeAction = menu->addAction(
        appIcon(AppIcon::Delete, QColor(QStringLiteral("#dc2626"))), tr("从最近项目中移除"));
    moreButton->setMenu(menu);

    const QString key = projectKey(project);
    connect(nameButton, &QPushButton::clicked, this,
            [this, key]() { emit projectOpenRequested(key); });
    connect(openAction, &QAction::triggered, this,
            [this, key]() { emit projectOpenRequested(key); });
    connect(removeAction, &QAction::triggered, this, [this, key]() {
        for (qsizetype i = recentProjects_.size(); i-- > 0;) {
            if (projectKey(recentProjects_.at(i)) == key) {
                recentProjects_.removeAt(i);
            }
        }
        rebuildRecentList();
        emit recentProjectRemoved(key);
    });

    rowLayout->addWidget(identity, 1);
    rowLayout->addWidget(characterCount);
    rowLayout->addWidget(gitStatus);
    rowLayout->addWidget(moreButton);
    return row;
}

QString HomePage::projectKey(const RecentProject& project) {
    return project.id.isEmpty() ? project.rootPath : project.id;
}

}  // namespace pureleaf::ui
