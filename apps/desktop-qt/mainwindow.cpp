#include "mainwindow.h"

#include <QWKWidgets/widgetwindowagent.h>

#include <QColor>
#include <QEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPoint>
#include <QStackedWidget>
#include <QStyle>
#include <QToolButton>

#include "components/icons/appicon.h"
#include "core/navigator.h"
#include "pages/editor/editorpage.h"
#include "pages/home/homepage.h"
#include "pages/settings/settingspage.h"

namespace pureleaf::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      windowAgent_(new QWK::WidgetWindowAgent(this)),
      titleBar_(nullptr),
      titleLabel_(nullptr),
      maximizeButton_(nullptr),
      stack_(new QStackedWidget(this)),
      navigator_(new Navigator(stack_, this)),
      homePage_(new HomePage),
      editorPage_(new EditorPage),
      settingsPage_(new SettingsPage) {
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setObjectName(QStringLiteral("mainWindow"));
    stack_->setObjectName(QStringLiteral("pageStack"));

    const bool windowAgentReady = windowAgent_->setup(this);
    if (windowAgentReady) {
        setupWindowChrome();
    }

    setCentralWidget(stack_);
    setWindowTitle(QStringLiteral("PureLeaf"));
    setMinimumSize(900, 600);
    resize(1280, 800);

    setupPages();
    wireNavigation();

    navigator_->navigateTo(PageId::Home);
}

MainWindow::~MainWindow() = default;

void MainWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::ActivationChange || event->type() == QEvent::WindowStateChange) {
        updateWindowChrome();
    }
}

void MainWindow::setupWindowChrome() {
    titleBar_ = new QWidget(this);
    titleBar_->setObjectName(QStringLiteral("windowTitleBar"));
    titleBar_->setFixedHeight(46);

    auto* layout = new QHBoxLayout(titleBar_);
    layout->setContentsMargins(10, 0, 0, 0);
    layout->setSpacing(0);

    auto* iconButton = new QToolButton(titleBar_);
    iconButton->setObjectName(QStringLiteral("windowIconButton"));
    iconButton->setIcon(appIcon(AppIcon::Leaf, QColor(QStringLiteral("#ffffff"))));
    iconButton->setIconSize(QSize(18, 18));
    iconButton->setToolTip(tr("系统菜单"));
    iconButton->setFixedSize(30, 30);
    iconButton->setFocusPolicy(Qt::NoFocus);

    titleLabel_ = new QLabel(windowTitle(), titleBar_);
    titleLabel_->setObjectName(QStringLiteral("windowTitleLabel"));
    titleLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    auto createWindowButton = [this](const QString& name, AppIcon icon,
                                     const QString& toolTip) {
        auto* button = new QToolButton(titleBar_);
        button->setObjectName(name);
        button->setProperty("windowControl", true);
        const QColor activeColor = icon == AppIcon::WindowClose
                                       ? QColor(QStringLiteral("#ffffff"))
                                       : QColor(QStringLiteral("#0f172a"));
        button->setIcon(appIcon(icon, QColor(QStringLiteral("#334155")), activeColor));
        button->setIconSize(QSize(17, 17));
        button->setToolTip(toolTip);
        button->setFixedSize(46, 46);
        button->setFocusPolicy(Qt::NoFocus);
        return button;
    };

    auto* minimizeButton = createWindowButton(QStringLiteral("minimizeButton"),
                                              AppIcon::WindowMinimize, tr("最小化"));
    maximizeButton_ = createWindowButton(QStringLiteral("maximizeButton"),
                                         AppIcon::WindowMaximize, tr("最大化"));
    auto* closeButton = createWindowButton(QStringLiteral("closeButton"), AppIcon::WindowClose,
                                           tr("关闭"));

    auto* settingsButton = new QToolButton(titleBar_);
    settingsButton->setObjectName(QStringLiteral("titleBarSettingsButton"));
    settingsButton->setProperty("appAction", true);
    settingsButton->setIcon(appIcon(AppIcon::Settings, QColor(QStringLiteral("#475569")),
                                    QColor(QStringLiteral("#0f172a"))));
    settingsButton->setIconSize(QSize(18, 18));
    settingsButton->setToolTip(tr("设置"));
    settingsButton->setFixedSize(40, 40);
    settingsButton->setFocusPolicy(Qt::NoFocus);

    layout->addWidget(iconButton);
    layout->addWidget(titleLabel_, 1);
    layout->addWidget(settingsButton);
    layout->addWidget(minimizeButton);
    layout->addWidget(maximizeButton_);
    layout->addWidget(closeButton);

    setMenuWidget(titleBar_);
    windowAgent_->setTitleBar(titleBar_);
    windowAgent_->setSystemButton(QWK::WindowAgentBase::WindowIcon, iconButton);
    windowAgent_->setSystemButton(QWK::WindowAgentBase::Minimize, minimizeButton);
    windowAgent_->setSystemButton(QWK::WindowAgentBase::Maximize, maximizeButton_);
    windowAgent_->setSystemButton(QWK::WindowAgentBase::Close, closeButton);
    windowAgent_->setHitTestVisible(settingsButton, true);

    connect(iconButton, &QToolButton::clicked, this, [this, iconButton]() {
        windowAgent_->showSystemMenu(iconButton->mapToGlobal(QPoint(0, iconButton->height())));
    });
    connect(minimizeButton, &QToolButton::clicked, this, &QWidget::showMinimized);
    connect(maximizeButton_, &QToolButton::clicked, this,
            [this]() { isMaximized() ? showNormal() : showMaximized(); });
    connect(closeButton, &QToolButton::clicked, this, &QWidget::close);
    connect(settingsButton, &QToolButton::clicked, this,
            [this]() { navigator_->navigateTo(PageId::Settings); });
    connect(this, &QWidget::windowTitleChanged, titleLabel_, &QLabel::setText);

#ifdef Q_OS_WIN
    // Keep the native DWM frame so Windows 11 supplies rounded corners,
    // shadow, resize borders, and Snap Layout behavior.
    windowAgent_->setWindowAttribute(QStringLiteral("dark-mode"), false);
    windowAgent_->setWindowAttribute(QStringLiteral("dwm-border-color"),
                                     QColor(QStringLiteral("#cbd5e1")));
#endif

    setStyleSheet(QStringLiteral(R"(
        QMainWindow#mainWindow, QStackedWidget#pageStack {
            background: #f8fafc;
        }

        QWidget#windowTitleBar {
            background: #f8fafc;
            border-bottom: 1px solid #e2e8f0;
        }

        QWidget#windowTitleBar[active="false"] {
            background: #f1f5f9;
        }

        QToolButton#windowIconButton {
            color: white;
            background: #16a34a;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-weight: 700;
        }

        QToolButton#windowIconButton:hover {
            background: #15803d;
        }

        QLabel#windowTitleLabel {
            color: #0f172a;
            padding-left: 10px;
            font-size: 13px;
            font-weight: 600;
        }

        QWidget#windowTitleBar[active="false"] QLabel#windowTitleLabel {
            color: #64748b;
        }

        QToolButton[windowControl="true"] {
            color: #334155;
            background: transparent;
            border: none;
            border-radius: 0;
            font-size: 17px;
        }

        QToolButton[windowControl="true"]:hover {
            color: #0f172a;
            background: #e2e8f0;
        }

        QToolButton#closeButton:hover {
            color: white;
            background: #e81123;
        }

        QToolButton[appAction="true"] {
            color: #475569;
            background: transparent;
            border: none;
            border-radius: 7px;
            font-size: 17px;
        }

        QToolButton[appAction="true"]:hover {
            color: #0f172a;
            background: #e2e8f0;
        }
    )"));

    updateWindowChrome();
}

void MainWindow::updateWindowChrome() {
    if (!titleBar_) return;

    titleBar_->setProperty("active", isActiveWindow());
    titleBar_->style()->unpolish(titleBar_);
    titleBar_->style()->polish(titleBar_);

    if (maximizeButton_) {
        maximizeButton_->setIcon(appIcon(isMaximized() ? AppIcon::WindowRestore
                                                       : AppIcon::WindowMaximize,
                                         QColor(QStringLiteral("#334155")),
                                         QColor(QStringLiteral("#0f172a"))));
        maximizeButton_->setToolTip(isMaximized() ? tr("还原") : tr("最大化"));
    }
}

void MainWindow::setupPages() {
    navigator_->registerPage(PageId::Home, homePage_);
    navigator_->registerPage(PageId::Editor, editorPage_);
    navigator_->registerPage(PageId::Settings, settingsPage_);
}

void MainWindow::wireNavigation() {
    // Home -> Editor (open project)
    connect(homePage_, &HomePage::projectOpenRequested, this, [this](const QString& projectId) {
        navigator_->navigateTo(PageId::Editor, projectId);
    });

    // Home -> new project (for now, also opens editor with empty id)
    connect(homePage_, &HomePage::newProjectRequested, this, [this]() {
        // TODO: show project creation dialog, then open editor.
        navigator_->navigateTo(PageId::Editor, QString{});
    });

    // Home -> open a local workspace folder.
    connect(homePage_, &HomePage::openFolderRequested, this, [this]() {
        const QString folder = QFileDialog::getExistingDirectory(
            this, tr("打开本地文件夹"), QString{}, QFileDialog::ShowDirsOnly);
        if (!folder.isEmpty()) {
            navigator_->navigateTo(PageId::Editor, folder);
        }
    });

    // Editor -> Home
    connect(editorPage_, &EditorPage::backRequested, this,
            [this]() { navigator_->navigateTo(PageId::Home); });

    // Settings -> Home
    connect(settingsPage_, &SettingsPage::backRequested, this,
            [this]() { navigator_->navigateTo(PageId::Home); });
}

}  // namespace pureleaf::ui
