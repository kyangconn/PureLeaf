#include "editorpage.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

#include "components/icons/appicon.h"

namespace pureleaf::ui {

EditorPage::EditorPage(QWidget* parent) : NavPage(parent), projectNameLabel_(nullptr) {
    setupUi();
}

void EditorPage::setupUi() {
    // Root is a vertical layout: top bar + main content.
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // --- Top bar ---
    auto* topBar = new QWidget(this);
    topBar->setObjectName("editorTopBar");
    topBar->setFixedHeight(48);
    auto* topLayout = new QHBoxLayout(topBar);

    auto* backBtn = new QPushButton(tr("返回"), topBar);
    backBtn->setIcon(appIcon(AppIcon::Back, QColor(QStringLiteral("#475569")),
                             QColor(QStringLiteral("#0f172a"))));
    backBtn->setIconSize(QSize(18, 18));
    projectNameLabel_ = new QLabel(tr("未打开项目"), topBar);
    projectNameLabel_->setAlignment(Qt::AlignCenter);

    topLayout->addWidget(backBtn);
    topLayout->addStretch();
    topLayout->addWidget(projectNameLabel_);
    topLayout->addStretch();
    // Right-side action buttons will go here (compile, etc.)

    connect(backBtn, &QPushButton::clicked, this, &EditorPage::backRequested);

    // --- Main content: file tree | editor | pdf ---
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    auto* fileTreePanel = new QWidget(splitter);
    fileTreePanel->setObjectName("fileTreePanel");
    auto* treeLayout = new QVBoxLayout(fileTreePanel);
    treeLayout->addWidget(new QLabel(tr("文件"), fileTreePanel));
    // TODO: QTreeView for project file tree.

    auto* editorPanel = new QWidget(splitter);
    auto* editorLayout = new QVBoxLayout(editorPanel);
    editorLayout->addWidget(new QLabel(tr("编辑器"), editorPanel));
    // TODO: QPlainTextEdit or embed a code editor widget.

    auto* pdfPanel = new QWidget(splitter);
    auto* pdfLayout = new QVBoxLayout(pdfPanel);
    pdfLayout->addWidget(new QLabel(tr("PDF 预览"), pdfPanel));
    // TODO: PDF rendering widget.

    splitter->addWidget(fileTreePanel);
    splitter->addWidget(editorPanel);
    splitter->addWidget(pdfPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->setStretchFactor(2, 2);

    root->addWidget(topBar);
    root->addWidget(splitter, 1);
}

void EditorPage::onPageEntered(const QVariant& payload) {
    // Payload is the project id to open.
    currentProjectId_ = payload.toString();
    if (projectNameLabel_) {
        const QFileInfo info(currentProjectId_);
        const QString displayName = info.exists() ? info.fileName() : currentProjectId_;
        projectNameLabel_->setText(displayName.isEmpty() ? tr("未打开项目") : displayName);
    }
    // TODO: load file tree, open last file, etc.
}

void EditorPage::onPageLeft() {
    // TODO: flush auto-save, stop compilation if running.
}

}  // namespace pureleaf::ui
