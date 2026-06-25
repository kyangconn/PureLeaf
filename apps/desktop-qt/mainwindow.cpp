#include "mainwindow.h"

#include <QWKWidgets/widgetwindowagent.h>

#include <QColor>
#include <QDir>
#include <QDirIterator>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPoint>
#include <QProcess>
#include <QScreen>
#include <QSet>
#include <QSettings>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStyle>
#include <QToolButton>

#include "components/icons/appicon.h"
#include "core/navigator.h"
#include "pages/editor/editorpage.h"
#include "pages/home/homepage.h"
#include "pages/settings/settingspage.h"

namespace pureleaf::ui {

namespace {

constexpr int kMaxRecentProjects = 12;
constexpr qint64 kMaxCountedFileBytes = 2 * 1024 * 1024;
constexpr qint64 kMaxTotalCountedBytes = 20 * 1024 * 1024;

QString normalizedProjectPath(const QString& path) {
    const QFileInfo info(path);
    QString normalized = info.exists() ? info.canonicalFilePath() : info.absoluteFilePath();
    if (normalized.isEmpty()) {
        normalized = path;
    }
    return QDir::toNativeSeparators(QDir::cleanPath(normalized));
}

QString projectIdentity(const QString& path) {
#ifdef Q_OS_WIN
    return QDir::cleanPath(path).toCaseFolded();
#else
    return QDir::cleanPath(path);
#endif
}

QString defaultProjectDialogPath() {
    const QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return documents.isEmpty() ? QDir::homePath() : documents;
}

QString displayNameForProjectRoot(const QString& rootPath) {
    const QFileInfo info(rootPath);
    const QString name = info.fileName();
    return name.isEmpty() ? QDir::toNativeSeparators(rootPath) : name;
}

qint64 estimateCharacterCount(const QString& rootPath) {
    static const QStringList filters = {
        QStringLiteral("*.tex"), QStringLiteral("*.bib"), QStringLiteral("*.cls"),
        QStringLiteral("*.sty"), QStringLiteral("*.md"),  QStringLiteral("*.txt"),
    };

    qint64 countedBytes = 0;
    qint64 characters = 0;
    QDirIterator it(rootPath, filters, QDir::Files | QDir::Readable, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString filePath = it.next();
        const QFileInfo info(filePath);
        if (info.size() <= 0 || info.size() > kMaxCountedFileBytes) {
            continue;
        }
        if (countedBytes + info.size() > kMaxTotalCountedBytes) {
            break;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        countedBytes += info.size();
        characters += QString::fromUtf8(file.readAll()).size();
    }
    return characters;
}

RecentProject::GitState detectGitState(const QString& rootPath) {
    const QFileInfo gitInfo(QDir(rootPath).filePath(QStringLiteral(".git")));
    if (!gitInfo.exists()) {
        return RecentProject::GitState::NoRepository;
    }

    QProcess git;
    git.setProgram(QStringLiteral("git"));
    git.setArguments({QStringLiteral("-C"), rootPath, QStringLiteral("status"),
                      QStringLiteral("--porcelain=v1")});
    git.start();
    if (!git.waitForStarted(750)) {
        return RecentProject::GitState::Unknown;
    }
    if (!git.waitForFinished(1500)) {
        git.kill();
        git.waitForFinished(100);
        return RecentProject::GitState::Unknown;
    }
    if (git.exitStatus() != QProcess::NormalExit || git.exitCode() != 0) {
        return RecentProject::GitState::Unknown;
    }

    const QString output = QString::fromUtf8(git.readAllStandardOutput()).trimmed();
    if (output.isEmpty()) {
        return RecentProject::GitState::Clean;
    }

    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QString code = line.left(2);
        if (code == QStringLiteral("UU") || code == QStringLiteral("AA") ||
            code == QStringLiteral("DD") || code == QStringLiteral("AU") ||
            code == QStringLiteral("UA") || code == QStringLiteral("DU") ||
            code == QStringLiteral("UD")) {
            return RecentProject::GitState::Conflict;
        }
    }
    return RecentProject::GitState::Modified;
}

RecentProject recentProjectFromRoot(const QString& rootPath) {
    const QString normalized = normalizedProjectPath(rootPath);
    RecentProject project;
    project.id = normalized;
    project.name = displayNameForProjectRoot(normalized);
    project.rootPath = normalized;
    project.characterCount = estimateCharacterCount(normalized);
    project.gitState = detectGitState(normalized);
    return project;
}

bool isValidProjectName(const QString& name) {
    return !name.isEmpty() && !name.contains(QLatin1Char('/')) &&
           !name.contains(QLatin1Char('\\')) && name != QStringLiteral(".") &&
           name != QStringLiteral("..");
}

QString latexEscaped(QString text) {
    text.replace(QStringLiteral("\\"), QStringLiteral("\\textbackslash{}"));
    text.replace(QStringLiteral("{"), QStringLiteral("\\{"));
    text.replace(QStringLiteral("}"), QStringLiteral("\\}"));
    text.replace(QStringLiteral("%"), QStringLiteral("\\%"));
    text.replace(QStringLiteral("&"), QStringLiteral("\\&"));
    text.replace(QStringLiteral("_"), QStringLiteral("\\_"));
    text.replace(QStringLiteral("#"), QStringLiteral("\\#"));
    text.replace(QStringLiteral("$"), QStringLiteral("\\$"));
    text.replace(QStringLiteral("^"), QStringLiteral("\\^{}"));
    text.replace(QStringLiteral("~"), QStringLiteral("\\~{}"));
    return text;
}

bool writeDefaultMainTex(const QString& projectRoot, const QString& projectName,
                         QString* errorMessage) {
    const QString mainTexPath = QDir(projectRoot).filePath(QStringLiteral("main.tex"));
    if (QFileInfo::exists(mainTexPath)) {
        return true;
    }

    const QString content = QStringLiteral(
                                "\\documentclass{article}\n"
                                "\\usepackage[utf8]{inputenc}\n"
                                "\\usepackage[T1]{fontenc}\n"
                                "\\usepackage{geometry}\n"
                                "\\geometry{a4paper, margin=2.5cm}\n\n"
                                "\\title{%1}\n"
                                "\\author{}\n"
                                "\\date{}\n\n"
                                "\\begin{document}\n"
                                "\\maketitle\n\n"
                                "\\end{document}\n")
                                .arg(latexEscaped(projectName));

    QFile file(mainTexPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    if (file.write(content.toUtf8()) < 0) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    return true;
}

}  // namespace

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
    setMinimumSize(820, 520);
    applyInitialWindowSize();

    setupPages();
    wireNavigation();
    loadRecentProjects();

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

    auto createWindowButton = [this](const QString& name, AppIcon icon, const QString& toolTip) {
        auto* button = new QToolButton(titleBar_);
        button->setObjectName(name);
        button->setProperty("windowControl", true);
        const QColor activeColor = icon == AppIcon::WindowClose ? QColor(QStringLiteral("#ffffff"))
                                                                : QColor(QStringLiteral("#0f172a"));
        button->setIcon(appIcon(icon, QColor(QStringLiteral("#334155")), activeColor));
        button->setIconSize(QSize(17, 17));
        button->setToolTip(toolTip);
        button->setFixedSize(46, 46);
        button->setFocusPolicy(Qt::NoFocus);
        return button;
    };

    auto* minimizeButton =
        createWindowButton(QStringLiteral("minimizeButton"), AppIcon::WindowMinimize, tr("最小化"));
    maximizeButton_ =
        createWindowButton(QStringLiteral("maximizeButton"), AppIcon::WindowMaximize, tr("最大化"));
    auto* closeButton =
        createWindowButton(QStringLiteral("closeButton"), AppIcon::WindowClose, tr("关闭"));

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

void MainWindow::applyInitialWindowSize() {
    const QScreen* primaryScreen = QGuiApplication::primaryScreen();
    const QRect available =
        primaryScreen ? primaryScreen->availableGeometry() : QRect(0, 0, 1280, 800);

    const int minWidth = minimumWidth();
    const int minHeight = minimumHeight();
    const int maxWidth = qMax(minWidth, qMin(1440, static_cast<int>(available.width() * 0.92)));
    int targetWidth = qBound(minWidth, static_cast<int>(available.width() * 0.82), maxWidth);

    const int maxHeight = qMax(minHeight, static_cast<int>(available.height() * 0.78));
    int targetHeight = qMin(static_cast<int>(targetWidth * 0.75), maxHeight);
    targetHeight = qMax(minHeight, targetHeight);

    const int fourByThreeWidth = (targetHeight * 4 + 2) / 3;
    targetWidth = qBound(minWidth, qMax(targetWidth, fourByThreeWidth), maxWidth);

    resize(targetWidth, targetHeight);
    move(available.center() - rect().center());
}

void MainWindow::updateWindowChrome() {
    if (!titleBar_) return;

    titleBar_->setProperty("active", isActiveWindow());
    titleBar_->style()->unpolish(titleBar_);
    titleBar_->style()->polish(titleBar_);

    if (maximizeButton_) {
        maximizeButton_->setIcon(
            appIcon(isMaximized() ? AppIcon::WindowRestore : AppIcon::WindowMaximize,
                    QColor(QStringLiteral("#334155")), QColor(QStringLiteral("#0f172a"))));
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
    connect(homePage_, &HomePage::projectOpenRequested, this,
            [this](const QString& projectKey) { openProject(projectKey); });

    // Home -> new blank project.
    connect(homePage_, &HomePage::newProjectRequested, this, [this]() { createBlankProject(); });

    // Home -> open a local workspace folder.
    connect(homePage_, &HomePage::openFolderRequested, this, [this]() {
        const QString folder = QFileDialog::getExistingDirectory(
            this, tr("打开本地文件夹"), defaultProjectDialogPath(), QFileDialog::ShowDirsOnly);
        if (!folder.isEmpty()) {
            openProject(folder);
        }
    });

    connect(homePage_, &HomePage::recentProjectRemoved, this,
            [this](const QString& projectKey) { removeRecentProject(projectKey); });

    // Editor -> Home
    connect(editorPage_, &EditorPage::backRequested, this, [this]() {
        loadRecentProjects();
        navigator_->navigateTo(PageId::Home);
    });

    // Settings -> Home
    connect(settingsPage_, &SettingsPage::backRequested, this, [this]() {
        loadRecentProjects();
        navigator_->navigateTo(PageId::Home);
    });
}

void MainWindow::loadRecentProjects() {
    recentProjects_.clear();

    QSettings settings;
    const int size = settings.beginReadArray(QStringLiteral("recentProjects"));
    QSet<QString> seen;
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString rootPath = settings.value(QStringLiteral("rootPath")).toString();
        if (rootPath.isEmpty()) {
            rootPath = settings.value(QStringLiteral("id")).toString();
        }
        if (rootPath.isEmpty()) {
            continue;
        }

        const QString normalized = normalizedProjectPath(rootPath);
        const QFileInfo info(normalized);
        if (!info.exists() || !info.isDir()) {
            continue;
        }

        const QString identity = projectIdentity(normalized);
        if (seen.contains(identity)) {
            continue;
        }
        seen.insert(identity);

        RecentProject project = recentProjectFromRoot(normalized);
        const QString storedName = settings.value(QStringLiteral("name")).toString().trimmed();
        if (!storedName.isEmpty()) {
            project.name = storedName;
        }
        recentProjects_.append(project);
    }
    settings.endArray();

    homePage_->setRecentProjects(recentProjects_);
}

void MainWindow::saveRecentProjects() const {
    QSettings settings;
    settings.remove(QStringLiteral("recentProjects"));
    settings.beginWriteArray(QStringLiteral("recentProjects"));
    for (int i = 0; i < recentProjects_.size(); ++i) {
        settings.setArrayIndex(i);
        const RecentProject& project = recentProjects_.at(i);
        settings.setValue(QStringLiteral("id"), project.id);
        settings.setValue(QStringLiteral("name"), project.name);
        settings.setValue(QStringLiteral("rootPath"), project.rootPath);
    }
    settings.endArray();
}

void MainWindow::rememberProject(const QString& rootPath) {
    const QString normalized = normalizedProjectPath(rootPath);
    const QString identity = projectIdentity(normalized);

    for (qsizetype i = recentProjects_.size(); i-- > 0;) {
        const RecentProject& project = recentProjects_.at(i);
        if (projectIdentity(project.rootPath) == identity ||
            projectIdentity(project.id) == identity) {
            recentProjects_.removeAt(i);
        }
    }

    recentProjects_.prepend(recentProjectFromRoot(normalized));
    while (recentProjects_.size() > kMaxRecentProjects) {
        recentProjects_.removeLast();
    }

    saveRecentProjects();
    homePage_->setRecentProjects(recentProjects_);
}

void MainWindow::removeRecentProject(const QString& projectKey) {
    const QString identity = projectIdentity(projectKey);
    for (qsizetype i = recentProjects_.size(); i-- > 0;) {
        const RecentProject& project = recentProjects_.at(i);
        if (projectIdentity(project.id) == identity ||
            projectIdentity(project.rootPath) == identity) {
            recentProjects_.removeAt(i);
        }
    }

    saveRecentProjects();
    homePage_->setRecentProjects(recentProjects_);
}

void MainWindow::openProject(const QString& rootPath) {
    const QString normalized = normalizedProjectPath(rootPath);
    const QFileInfo info(normalized);
    if (!info.exists() || !info.isDir()) {
        QMessageBox::warning(
            this, tr("无法打开项目"),
            tr("这个项目路径不存在：\n%1").arg(QDir::toNativeSeparators(normalized)));
        removeRecentProject(normalized);
        return;
    }

    rememberProject(normalized);
    navigator_->navigateTo(PageId::Editor, normalized);
}

void MainWindow::createBlankProject() {
    const QString parentDir = QFileDialog::getExistingDirectory(
        this, tr("选择新项目保存位置"), defaultProjectDialogPath(), QFileDialog::ShowDirsOnly);
    if (parentDir.isEmpty()) {
        return;
    }

    bool ok = false;
    const QString projectName = QInputDialog::getText(this, tr("新建空白项目"), tr("项目名称"),
                                                      QLineEdit::Normal, tr("Untitled"), &ok)
                                    .trimmed();
    if (!ok || projectName.isEmpty()) {
        return;
    }
    if (!isValidProjectName(projectName)) {
        QMessageBox::warning(this, tr("项目名称不可用"),
                             tr("项目名称不能包含路径分隔符，也不能是 . 或 ..。"));
        return;
    }

    const QString projectRoot = QDir(parentDir).filePath(projectName);
    QDir projectDir(projectRoot);
    if (projectDir.exists()) {
        const bool isEmpty =
            projectDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty();
        if (!isEmpty) {
            QMessageBox::warning(
                this, tr("目录已存在"),
                tr("目标目录已经存在且不为空：\n%1").arg(QDir::toNativeSeparators(projectRoot)));
            return;
        }
    } else if (!QDir().mkpath(projectRoot)) {
        QMessageBox::critical(
            this, tr("创建失败"),
            tr("无法创建项目目录：\n%1").arg(QDir::toNativeSeparators(projectRoot)));
        return;
    }

    QString errorMessage;
    if (!writeDefaultMainTex(projectRoot, projectName, &errorMessage)) {
        QMessageBox::critical(this, tr("创建失败"),
                              tr("无法写入 main.tex：\n%1").arg(errorMessage));
        return;
    }

    openProject(projectRoot);
}

}  // namespace pureleaf::ui
