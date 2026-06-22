#include "editorpage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

namespace pureleaf::ui {

EditorPage::EditorPage(QWidget *parent) : NavPage(parent) {
    setupUi();
}

void EditorPage::setupUi() {
    // Root is a vertical layout: top bar + main content.
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // --- Top bar ---
    auto *topBar = new QWidget(this);
    topBar->setObjectName("editorTopBar");
    topBar->setFixedHeight(48);
    auto *topLayout = new QHBoxLayout(topBar);

    auto *backBtn = new QPushButton(tr("返回"), topBar);
    auto *projectName = new QLabel(tr("未打开项目"), topBar);
    projectName->setAlignment(Qt::AlignCenter);

    topLayout->addWidget(backBtn);
    topLayout->addStretch();
    topLayout->addWidget(projectName);
    topLayout->addStretch();
    // Right-side action buttons will go here (compile, etc.)

    connect(backBtn, &QPushButton::clicked, this, &EditorPage::backRequested);

    // --- Main content: file tree | editor | pdf ---
    auto *splitter = new QSplitter(Qt::Horizontal, this);

    auto *fileTreePanel = new QWidget(splitter);
    fileTreePanel->setObjectName("fileTreePanel");
    auto *treeLayout = new QVBoxLayout(fileTreePanel);
    treeLayout->addWidget(new QLabel(tr("文件"), fileTreePanel));
    // TODO: QTreeView for project file tree.

    auto *editorPanel = new QWidget(splitter);
    auto *editorLayout = new QVBoxLayout(editorPanel);
    editorLayout->addWidget(new QLabel(tr("编辑器"), editorPanel));
    // TODO: QPlainTextEdit or embed a code editor widget.

    auto *pdfPanel = new QWidget(splitter);
    auto *pdfLayout = new QVBoxLayout(pdfPanel);
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

void EditorPage::onPageEntered(const QVariant &payload) {
    // Payload is the project id to open.
    currentProjectId_ = payload.toString();
    // TODO: load file tree, open last file, etc.
}

void EditorPage::onPageLeft() {
    // TODO: flush auto-save, stop compilation if running.
}

}  // namespace pureleaf::ui
