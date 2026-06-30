#include "editorpage.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColor>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QShortcut>
#include <QSplitter>
#include <QSyntaxHighlighter>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextFormat>
#include <QTimer>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

#include "components/icons/appicon.h"

namespace pureleaf::ui {

namespace {

constexpr qint64 kMaxEditableFileBytes = 4 * 1024 * 1024;

bool isEditableFile(const QString& path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    static const QSet<QString> editable = {
        QStringLiteral("tex"),  QStringLiteral("bib"), QStringLiteral("cls"),
        QStringLiteral("sty"),  QStringLiteral("md"),  QStringLiteral("txt"),
        QStringLiteral("tikz"), QStringLiteral("bbx"), QStringLiteral("cbx"),
    };
    return editable.contains(suffix);
}

QString latexTemplateForFile(const QString& fileName) {
    if (!fileName.endsWith(QStringLiteral(".tex"), Qt::CaseInsensitive)) {
        return QString();
    }

    return QStringLiteral(
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
        .arg(QFileInfo(fileName).completeBaseName());
}

class LineNumberArea;

class LatexEditor : public QPlainTextEdit {
public:
    explicit LatexEditor(QWidget* parent = nullptr);

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent* event);
    void setLineNumbersVisible(bool visible);
    void setTabWidthSpaces(int spaces);
    void setInsertSpaces(bool enabled);
    void setAutoCloseBrackets(bool enabled);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect& rect, int dy);

    LineNumberArea* lineNumberArea_;
    bool lineNumbersVisible_;
    bool insertSpaces_;
    bool autoCloseBrackets_;
    int tabWidthSpaces_;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(LatexEditor* editor) : QWidget(editor), editor_(editor) {}

    QSize sizeHint() const override { return QSize(editor_->lineNumberAreaWidth(), 0); }

protected:
    void paintEvent(QPaintEvent* event) override { editor_->lineNumberAreaPaintEvent(event); }

private:
    LatexEditor* editor_;
};

LatexEditor::LatexEditor(QWidget* parent)
    : QPlainTextEdit(parent),
      lineNumberArea_(new LineNumberArea(this)),
      lineNumbersVisible_(true),
      insertSpaces_(true),
      autoCloseBrackets_(true),
      tabWidthSpaces_(4) {
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabWidthSpaces(tabWidthSpaces_);

    connect(this, &QPlainTextEdit::blockCountChanged, this,
            &LatexEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &LatexEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &LatexEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int LatexEditor::lineNumberAreaWidth() const {
    if (!lineNumbersVisible_) {
        return 0;
    }

    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    return 14 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void LatexEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    if (!lineNumbersVisible_) {
        return;
    }

    QPainter painter(lineNumberArea_);
    painter.fillRect(event->rect(), QColor(QStringLiteral("#f8fafc")));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(QStringLiteral("#94a3b8")));
            painter.drawText(0, top, lineNumberArea_->width() - 6, fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void LatexEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    lineNumberArea_->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void LatexEditor::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Tab && insertSpaces_) {
        insertPlainText(QString(tabWidthSpaces_, QLatin1Char(' ')));
        return;
    }

    if (autoCloseBrackets_ && event->text().size() == 1 && !textCursor().hasSelection()) {
        const QChar open = event->text().at(0);
        QChar close;
        if (open == QLatin1Char('(')) {
            close = QLatin1Char(')');
        } else if (open == QLatin1Char('[')) {
            close = QLatin1Char(']');
        } else if (open == QLatin1Char('{')) {
            close = QLatin1Char('}');
        } else if (open == QLatin1Char('$')) {
            close = QLatin1Char('$');
        }

        if (!close.isNull()) {
            QTextCursor cursor = textCursor();
            cursor.insertText(QString(open) + close);
            cursor.movePosition(QTextCursor::Left);
            setTextCursor(cursor);
            return;
        }
    }

    QPlainTextEdit::keyPressEvent(event);
}

void LatexEditor::setLineNumbersVisible(bool visible) {
    lineNumbersVisible_ = visible;
    lineNumberArea_->setVisible(visible);
    updateLineNumberAreaWidth(0);
}

void LatexEditor::setTabWidthSpaces(int spaces) {
    tabWidthSpaces_ = qBound(2, spaces, 8);
    setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * tabWidthSpaces_);
}

void LatexEditor::setInsertSpaces(bool enabled) {
    insertSpaces_ = enabled;
}

void LatexEditor::setAutoCloseBrackets(bool enabled) {
    autoCloseBrackets_ = enabled;
}

void LatexEditor::updateLineNumberAreaWidth(int newBlockCount) {
    (void)newBlockCount;
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void LatexEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> selections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(QStringLiteral("#f1f5f9")));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        selections.append(selection);
    }
    setExtraSelections(selections);
}

void LatexEditor::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy != 0) {
        lineNumberArea_->scroll(0, dy);
    } else {
        lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

class LatexHighlighter : public QSyntaxHighlighter {
public:
    explicit LatexHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
        commandFormat_.setForeground(QColor(QStringLiteral("#2563eb")));
        commandFormat_.setFontWeight(QFont::DemiBold);

        environmentFormat_.setForeground(QColor(QStringLiteral("#7c3aed")));
        environmentFormat_.setFontWeight(QFont::DemiBold);

        commentFormat_.setForeground(QColor(QStringLiteral("#64748b")));
        commentFormat_.setFontItalic(true);

        mathFormat_.setForeground(QColor(QStringLiteral("#b45309")));
        bracketFormat_.setForeground(QColor(QStringLiteral("#0f766e")));
    }

protected:
    void highlightBlock(const QString& text) override {
        apply(QStringLiteral(R"(\\[A-Za-z@]+\\*?)"), commandFormat_, text);
        apply(QStringLiteral(R"(\\(begin|end)\s*\{[^}]+\})"), environmentFormat_, text);
        apply(QStringLiteral(R"([\{\}\[\]])"), bracketFormat_, text);
        apply(QStringLiteral(R"(\$[^$]*\$)"), mathFormat_, text);

        int commentStart = -1;
        for (int i = 0; i < text.size(); ++i) {
            if (text.at(i) == QLatin1Char('%') && (i == 0 || text.at(i - 1) != QLatin1Char('\\'))) {
                commentStart = i;
                break;
            }
        }
        if (commentStart >= 0) {
            setFormat(commentStart, text.size() - commentStart, commentFormat_);
        }
    }

private:
    void apply(const QString& pattern, const QTextCharFormat& format, const QString& text) {
        const QRegularExpression expression(pattern);
        auto it = expression.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), format);
        }
    }

    QTextCharFormat commandFormat_;
    QTextCharFormat environmentFormat_;
    QTextCharFormat commentFormat_;
    QTextCharFormat mathFormat_;
    QTextCharFormat bracketFormat_;
};

}  // namespace

EditorPage::EditorPage(QWidget* parent)
    : NavPage(parent),
      settings_(defaultAppSettings()),
      isDirty_(false),
      loadingFile_(false),
      autoSaveDelayMs_(2000),
      compileTimeoutMs_(60000),
      projectNameLabel_(nullptr),
      fileNameLabel_(nullptr),
      statusLabel_(nullptr),
      pdfStatusLabel_(nullptr),
      fileModel_(nullptr),
      fileTree_(nullptr),
      editor_(nullptr),
      compileLog_(nullptr),
      saveButton_(nullptr),
      compileButton_(nullptr),
      openPdfButton_(nullptr),
      autoSaveTimer_(new QTimer(this)),
      compileTimeoutTimer_(new QTimer(this)),
      compileProcess_(new QProcess(this)) {
    setupUi();
}

void EditorPage::setupUi() {
    autoSaveTimer_->setSingleShot(true);
    connect(autoSaveTimer_, &QTimer::timeout, this, [this]() {
        if (isDirty_) {
            saveCurrentFile();
        }
    });

    compileTimeoutTimer_->setSingleShot(true);
    connect(compileTimeoutTimer_, &QTimer::timeout, this, [this]() {
        if (compileProcess_->state() != QProcess::NotRunning) {
            appendCompileLog(tr("\n编译超时，已终止进程。\n"));
            compileProcess_->kill();
        }
    });

    connect(compileProcess_, &QProcess::readyReadStandardOutput, this, [this]() {
        appendCompileLog(QString::fromLocal8Bit(compileProcess_->readAllStandardOutput()));
    });
    connect(compileProcess_, &QProcess::readyReadStandardError, this, [this]() {
        appendCompileLog(QString::fromLocal8Bit(compileProcess_->readAllStandardError()));
    });
    connect(compileProcess_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &EditorPage::finishCompile);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* topBar = new QWidget(this);
    topBar->setObjectName(QStringLiteral("editorTopBar"));
    topBar->setFixedHeight(48);
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(12, 0, 12, 0);
    topLayout->setSpacing(8);

    auto* backBtn = new QPushButton(tr("返回"), topBar);
    backBtn->setIcon(appIcon(AppIcon::Back, QColor(QStringLiteral("#475569")),
                             QColor(QStringLiteral("#0f172a"))));
    backBtn->setIconSize(QSize(18, 18));

    projectNameLabel_ = new QLabel(tr("未打开项目"), topBar);
    projectNameLabel_->setObjectName(QStringLiteral("editorProjectName"));
    projectNameLabel_->setAlignment(Qt::AlignCenter);

    saveButton_ = new QPushButton(tr("保存"), topBar);
    saveButton_->setEnabled(false);
    compileButton_ = new QPushButton(tr("编译"), topBar);
    compileButton_->setEnabled(false);

    topLayout->addWidget(backBtn);
    topLayout->addWidget(projectNameLabel_, 1);
    topLayout->addWidget(saveButton_);
    topLayout->addWidget(compileButton_);

    connect(backBtn, &QPushButton::clicked, this, &EditorPage::backRequested);
    connect(saveButton_, &QPushButton::clicked, this, [this]() { saveCurrentFile(); });
    connect(compileButton_, &QPushButton::clicked, this, &EditorPage::compileProject);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("editorWorkspace"));

    auto* fileTreePanel = new QWidget(splitter);
    fileTreePanel->setObjectName(QStringLiteral("fileTreePanel"));
    fileTreePanel->setMinimumWidth(210);
    auto* treeLayout = new QVBoxLayout(fileTreePanel);
    treeLayout->setContentsMargins(12, 12, 8, 12);
    treeLayout->setSpacing(8);

    auto* treeHeader = new QLabel(tr("文件"), fileTreePanel);
    treeHeader->setObjectName(QStringLiteral("panelHeader"));
    fileTree_ = new QTreeView(fileTreePanel);
    fileTree_->setObjectName(QStringLiteral("projectFileTree"));
    fileTree_->setHeaderHidden(true);
    fileTree_->setContextMenuPolicy(Qt::CustomContextMenu);
    fileTree_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    fileModel_ = new QFileSystemModel(this);
    fileModel_->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    fileModel_->setReadOnly(false);
    fileTree_->setModel(fileModel_);
    for (int column = 1; column < fileModel_->columnCount(); ++column) {
        fileTree_->hideColumn(column);
    }

    treeLayout->addWidget(treeHeader);
    treeLayout->addWidget(fileTree_, 1);

    auto* editorPanel = new QWidget(splitter);
    editorPanel->setObjectName(QStringLiteral("textEditorPanel"));
    auto* editorLayout = new QVBoxLayout(editorPanel);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);

    auto* editorHeader = new QWidget(editorPanel);
    editorHeader->setObjectName(QStringLiteral("editorFileHeader"));
    editorHeader->setFixedHeight(38);
    auto* editorHeaderLayout = new QHBoxLayout(editorHeader);
    editorHeaderLayout->setContentsMargins(12, 0, 12, 0);
    fileNameLabel_ = new QLabel(tr("未选择文件"), editorHeader);
    statusLabel_ = new QLabel(tr("就绪"), editorHeader);
    statusLabel_->setObjectName(QStringLiteral("editorStatus"));
    editorHeaderLayout->addWidget(fileNameLabel_, 1);
    editorHeaderLayout->addWidget(statusLabel_);

    editor_ = new LatexEditor(editorPanel);
    editor_->setObjectName(QStringLiteral("latexEditor"));
    editor_->setEnabled(false);
    editor_->setPlaceholderText(tr("从左侧选择 .tex / .bib / .sty 文件开始编辑。"));
    new LatexHighlighter(editor_->document());

    editorLayout->addWidget(editorHeader);
    editorLayout->addWidget(editor_, 1);

    auto* outputPanel = new QWidget(splitter);
    outputPanel->setObjectName(QStringLiteral("outputPanel"));
    outputPanel->setMinimumWidth(260);
    auto* outputLayout = new QVBoxLayout(outputPanel);
    outputLayout->setContentsMargins(8, 12, 12, 12);
    outputLayout->setSpacing(8);

    auto* pdfHeader = new QLabel(tr("PDF / 日志"), outputPanel);
    pdfHeader->setObjectName(QStringLiteral("panelHeader"));
    pdfStatusLabel_ = new QLabel(tr("编译后会在这里显示 PDF 输出路径。"), outputPanel);
    pdfStatusLabel_->setObjectName(QStringLiteral("pdfStatus"));
    pdfStatusLabel_->setWordWrap(true);
    openPdfButton_ = new QPushButton(tr("打开 PDF"), outputPanel);
    openPdfButton_->setEnabled(false);

    compileLog_ = new QPlainTextEdit(outputPanel);
    compileLog_->setObjectName(QStringLiteral("compileLog"));
    compileLog_->setReadOnly(true);
    compileLog_->setPlaceholderText(tr("编译日志"));

    outputLayout->addWidget(pdfHeader);
    outputLayout->addWidget(pdfStatusLabel_);
    outputLayout->addWidget(openPdfButton_);
    outputLayout->addWidget(compileLog_, 1);

    splitter->addWidget(fileTreePanel);
    splitter->addWidget(editorPanel);
    splitter->addWidget(outputPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 4);
    splitter->setStretchFactor(2, 2);
    splitter->setSizes({260, 660, 340});

    root->addWidget(topBar);
    root->addWidget(splitter, 1);

    connect(fileTree_, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
        const QString path = fileModel_->filePath(index);
        if (QFileInfo(path).isFile()) {
            openFile(path);
        }
    });
    connect(fileTree_, &QTreeView::customContextMenuRequested, this,
            &EditorPage::showFileContextMenu);
    connect(editor_, &QPlainTextEdit::textChanged, this, [this]() {
        if (!loadingFile_) {
            markDirty(true);
            scheduleAutoSave();
        }
    });
    connect(openPdfButton_, &QPushButton::clicked, this, [this]() {
        if (!lastPdfPath_.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(lastPdfPath_));
        }
    });

    auto* saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, [this]() { saveCurrentFile(); });
    auto* compileShortcut = new QShortcut(QKeySequence(Qt::Key_F5), this);
    connect(compileShortcut, &QShortcut::activated, this, &EditorPage::compileProject);

    setStyleSheet(QStringLiteral(R"(
        QWidget#editorTopBar, QWidget#editorFileHeader {
            background: #f8fafc;
            border-bottom: 1px solid #e2e8f0;
        }

        QLabel#editorProjectName, QLabel#panelHeader {
            color: #0f172a;
            font-size: 13px;
            font-weight: 700;
        }

        QLabel#editorStatus, QLabel#pdfStatus {
            color: #64748b;
            font-size: 12px;
        }

        QWidget#fileTreePanel, QWidget#outputPanel {
            background: #f8fafc;
        }

        QWidget#fileTreePanel {
            border-right: 1px solid #e2e8f0;
        }

        QWidget#outputPanel {
            border-left: 1px solid #e2e8f0;
        }

        QTreeView#projectFileTree {
            background: transparent;
            border: none;
            color: #334155;
            outline: none;
        }

        QTreeView#projectFileTree::item {
            min-height: 24px;
            border-radius: 5px;
            padding: 2px 4px;
        }

        QTreeView#projectFileTree::item:selected {
            color: #0f172a;
            background: #dcfce7;
        }

        QPlainTextEdit#latexEditor, QPlainTextEdit#compileLog {
            border: none;
            background: white;
            color: #0f172a;
            selection-background-color: #bbf7d0;
            font-family: "Cascadia Code", "Consolas", monospace;
            font-size: 13px;
        }

        QPlainTextEdit#compileLog {
            background: #0f172a;
            color: #e2e8f0;
            border-radius: 8px;
            padding: 8px;
            font-size: 12px;
        }

        QPushButton {
            border-radius: 7px;
            padding: 6px 12px;
            border: 1px solid #cbd5e1;
            background: white;
            color: #334155;
            font-weight: 600;
        }

        QPushButton:hover {
            background: #f1f5f9;
            color: #0f172a;
        }

        QPushButton:disabled {
            color: #94a3b8;
            background: #f8fafc;
            border-color: #e2e8f0;
        }
    )"));

    applySettings(settings_);
}

void EditorPage::onPageEntered(const QVariant& payload) {
    loadProject(payload.toString());
}

void EditorPage::onPageLeft() {
    if (isDirty_) {
        saveCurrentFile();
    }
    autoSaveTimer_->stop();
    if (compileProcess_->state() != QProcess::NotRunning) {
        compileProcess_->kill();
        compileProcess_->waitForFinished(300);
    }
}

void EditorPage::applySettings(AppSettings settings) {
    settings_ = settings;
    autoSaveDelayMs_ = settings_.autoSaveEnabled ? settings_.autoSaveDelayMs : 0;
    compileTimeoutMs_ = settings_.compileTimeoutSeconds * 1000;

    QFont editorFont(settings_.editorFontFamily);
    editorFont.setPointSize(settings_.editorFontSize);
    editor_->setFont(editorFont);
    compileLog_->setFont(editorFont);
    editor_->setLineWrapMode(settings_.wordWrap ? QPlainTextEdit::WidgetWidth
                                                : QPlainTextEdit::NoWrap);

    if (auto* latexEditor = dynamic_cast<LatexEditor*>(editor_)) {
        latexEditor->setLineNumbersVisible(settings_.showLineNumbers);
        latexEditor->setTabWidthSpaces(settings_.tabWidth);
        latexEditor->setInsertSpaces(settings_.insertSpaces);
        latexEditor->setAutoCloseBrackets(settings_.autoCloseBrackets);
    }

    if (!settings_.autoSaveEnabled) {
        autoSaveTimer_->stop();
    }
}

void EditorPage::loadProject(const QString& rootPath) {
    const QFileInfo info(rootPath);
    if (!info.exists() || !info.isDir()) {
        projectRootPath_.clear();
        currentProjectId_.clear();
        projectNameLabel_->setText(tr("未打开项目"));
        return;
    }

    currentProjectId_ = QDir::cleanPath(info.absoluteFilePath());
    projectRootPath_ = info.canonicalFilePath();
    if (projectRootPath_.isEmpty()) {
        projectRootPath_ = info.absoluteFilePath();
    }
    projectRootPath_ = QDir::cleanPath(projectRootPath_);

    projectNameLabel_->setText(info.fileName().isEmpty() ? projectRootPath_ : info.fileName());
    const QModelIndex rootIndex = fileModel_->setRootPath(projectRootPath_);
    fileTree_->setRootIndex(rootIndex);
    fileTree_->sortByColumn(0, Qt::AscendingOrder);
    fileTree_->expand(rootIndex);

    compileButton_->setEnabled(true);
    compileLog_->clear();
    lastPdfPath_.clear();
    openPdfButton_->setEnabled(false);
    pdfStatusLabel_->setText(tr("编译后会在这里显示 PDF 输出路径。"));

    const QString initialFile = findMainTex();
    if (!initialFile.isEmpty()) {
        openFile(initialFile);
    } else {
        loadingFile_ = true;
        editor_->clear();
        loadingFile_ = false;
        editor_->setEnabled(false);
        currentFilePath_.clear();
        markDirty(false);
        updateEditorTitle();
    }
}

void EditorPage::openFile(const QString& filePath) {
    if (!isPathInsideProject(filePath)) {
        QMessageBox::warning(this, tr("无法打开文件"), tr("只能打开当前项目目录下的文件。"));
        return;
    }

    const QFileInfo info(filePath);
    if (!info.isFile() || !isEditableFile(filePath)) {
        statusLabel_->setText(tr("此文件类型暂不支持编辑"));
        return;
    }
    if (info.size() > kMaxEditableFileBytes) {
        QMessageBox::warning(this, tr("文件过大"), tr("暂不打开超过 4 MB 的文本文件。"));
        return;
    }

    if (isDirty_ && !saveCurrentFile()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("无法打开文件"), file.errorString());
        return;
    }

    loadingFile_ = true;
    editor_->setPlainText(QString::fromUtf8(file.readAll()));
    loadingFile_ = false;
    editor_->setEnabled(true);
    currentFilePath_ = QDir::cleanPath(
        info.canonicalFilePath().isEmpty() ? info.absoluteFilePath() : info.canonicalFilePath());
    markDirty(false);
    updateEditorTitle();
}

bool EditorPage::saveCurrentFile() {
    if (currentFilePath_.isEmpty() || !editor_->isEnabled()) {
        return false;
    }
    if (!isPathInsideProject(currentFilePath_)) {
        QMessageBox::warning(this, tr("无法保存"), tr("目标文件不在当前项目目录内。"));
        return false;
    }

    QSaveFile file(currentFilePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("保存失败"), file.errorString());
        return false;
    }

    file.write(editor_->toPlainText().toUtf8());
    if (!file.commit()) {
        QMessageBox::warning(this, tr("保存失败"), file.errorString());
        return false;
    }

    markDirty(false);
    statusLabel_->setText(tr("已保存"));
    return true;
}

void EditorPage::markDirty(bool dirty) {
    isDirty_ = dirty;
    saveButton_->setEnabled(dirty && editor_->isEnabled());
    updateEditorTitle();
}

void EditorPage::scheduleAutoSave() {
    if (autoSaveDelayMs_ > 0 && editor_->isEnabled()) {
        autoSaveTimer_->start(autoSaveDelayMs_);
    }
}

void EditorPage::compileProject() {
    if (projectRootPath_.isEmpty()) {
        return;
    }
    if (isDirty_ && !saveCurrentFile()) {
        return;
    }
    if (compileProcess_->state() != QProcess::NotRunning) {
        QMessageBox::information(this, tr("正在编译"), tr("当前项目已经在编译中。"));
        return;
    }

    const QString mainTex = findMainTex();
    if (mainTex.isEmpty()) {
        QMessageBox::warning(this, tr("无法编译"), tr("没有找到 main.tex 或其他 .tex 文件。"));
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
        QMessageBox::warning(this, tr("无法编译"), tr("无法创建编译输出目录。"));
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

    compileLog_->clear();
    const QString compilerProgram = settings_.compilerPath.trimmed().isEmpty()
                                        ? settings_.compilerType
                                        : settings_.compilerPath.trimmed();
    appendCompileLog(
        tr("编译：%1\n编译器：%2\n").arg(relativeProjectPath(mainTex), compilerProgram));
    pdfStatusLabel_->setText(tr("正在编译..."));
    openPdfButton_->setEnabled(false);
    compileButton_->setEnabled(false);
    lastPdfPath_.clear();

    compileProcess_->setProgram(compilerProgram);
    compileProcess_->setWorkingDirectory(projectRootPath_);
    compileProcess_->setArguments({
        QStringLiteral("-interaction=nonstopmode"),
        QStringLiteral("-synctex=1"),
        QStringLiteral("-output-directory=%1").arg(outputDir),
        relativeProjectPath(mainTex),
    });
    compileProcess_->start();
    if (!compileProcess_->waitForStarted(500)) {
        compileButton_->setEnabled(true);
        pdfStatusLabel_->setText(tr("编译器启动失败"));
        appendCompileLog(
            tr("无法启动编译器。请确认 LaTeX 发行版已安装，或在设置中配置编译器路径。\n"));
        return;
    }
    compileTimeoutTimer_->start(compileTimeoutMs_);
}

void EditorPage::finishCompile(int exitCode, QProcess::ExitStatus exitStatus) {
    compileTimeoutTimer_->stop();
    compileButton_->setEnabled(!projectRootPath_.isEmpty());

    const QString mainTex = findMainTex();
    const QString pdfName = QFileInfo(mainTex).completeBaseName() + QStringLiteral(".pdf");
    const QString configuredOutputDir = settings_.outputDirectory.trimmed().isEmpty()
                                            ? QStringLiteral("build")
                                            : settings_.outputDirectory.trimmed();
    const QString outputDir =
        QDir::cleanPath(QFileInfo(configuredOutputDir).isAbsolute()
                            ? configuredOutputDir
                            : QDir(projectRootPath_).filePath(configuredOutputDir));
    const QString pdfPath = QDir(outputDir).filePath(pdfName);

    if (exitStatus == QProcess::NormalExit && exitCode == 0 && QFileInfo::exists(pdfPath)) {
        lastPdfPath_ = pdfPath;
        openPdfButton_->setEnabled(true);
        pdfStatusLabel_->setText(tr("已生成：%1").arg(QDir::toNativeSeparators(pdfPath)));
        appendCompileLog(tr("\n编译完成。\n"));
    } else {
        pdfStatusLabel_->setText(tr("编译失败，请查看日志"));
        appendCompileLog(tr("\n编译失败，退出码：%1\n").arg(exitCode));
    }
}

void EditorPage::appendCompileLog(const QString& text) {
    compileLog_->moveCursor(QTextCursor::End);
    compileLog_->insertPlainText(text);
    compileLog_->moveCursor(QTextCursor::End);
}

void EditorPage::showFileContextMenu(const QPoint& pos) {
    QMenu menu(this);
    QAction* newFile = menu.addAction(tr("新建文件"));
    QAction* newDirectory = menu.addAction(tr("新建文件夹"));
    menu.addSeparator();
    QAction* rename = menu.addAction(tr("重命名"));
    QAction* remove = menu.addAction(tr("删除"));
    QAction* copyPath = menu.addAction(tr("复制相对路径"));

    const QString path = selectedPath();
    const bool hasSelection = !path.isEmpty();
    rename->setEnabled(hasSelection);
    remove->setEnabled(hasSelection);
    copyPath->setEnabled(hasSelection);

    QAction* action = menu.exec(fileTree_->viewport()->mapToGlobal(pos));
    if (action == newFile) {
        createFile(false);
    } else if (action == newDirectory) {
        createFile(true);
    } else if (action == rename) {
        renameSelectedEntry();
    } else if (action == remove) {
        deleteSelectedEntry();
    } else if (action == copyPath) {
        QApplication::clipboard()->setText(relativeProjectPath(path));
    }
}

void EditorPage::createFile(bool directory) {
    QString parentPath = selectedPath();
    if (parentPath.isEmpty() || QFileInfo(parentPath).isFile()) {
        parentPath = projectRootPath_;
    }

    const QString title = directory ? tr("新建文件夹") : tr("新建文件");
    const QString label = directory ? tr("文件夹名称") : tr("文件名");
    const QString defaultName = directory ? tr("untitled") : QStringLiteral("untitled.tex");
    bool ok = false;
    const QString name =
        QInputDialog::getText(this, title, label, QLineEdit::Normal, defaultName, &ok).trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }
    if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\')) ||
        name == QStringLiteral(".") || name == QStringLiteral("..")) {
        QMessageBox::warning(this, tr("名称不可用"), tr("名称不能包含路径分隔符。"));
        return;
    }

    const QString newPath = QDir(parentPath).filePath(name);
    if (!isPathInsideProject(newPath) || QFileInfo::exists(newPath)) {
        QMessageBox::warning(this, tr("无法创建"), tr("路径不可用或文件已存在。"));
        return;
    }

    if (directory) {
        if (!QDir().mkpath(newPath)) {
            QMessageBox::warning(this, tr("无法创建"), tr("创建文件夹失败。"));
        }
        return;
    }

    QSaveFile file(newPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("无法创建"), file.errorString());
        return;
    }
    file.write(latexTemplateForFile(name).toUtf8());
    if (!file.commit()) {
        QMessageBox::warning(this, tr("无法创建"), file.errorString());
        return;
    }
    openFile(newPath);
}

void EditorPage::renameSelectedEntry() {
    const QString path = selectedPath();
    if (path.isEmpty() || path == projectRootPath_) {
        return;
    }

    const QFileInfo info(path);
    bool ok = false;
    const QString newName = QInputDialog::getText(this, tr("重命名"), tr("新名称"),
                                                  QLineEdit::Normal, info.fileName(), &ok)
                                .trimmed();
    if (!ok || newName.isEmpty() || newName == info.fileName()) {
        return;
    }
    if (newName.contains(QLatin1Char('/')) || newName.contains(QLatin1Char('\\'))) {
        QMessageBox::warning(this, tr("名称不可用"), tr("名称不能包含路径分隔符。"));
        return;
    }

    const QString newPath = QDir(info.absolutePath()).filePath(newName);
    if (!isPathInsideProject(newPath) || QFileInfo::exists(newPath)) {
        QMessageBox::warning(this, tr("无法重命名"), tr("目标路径不可用或已经存在。"));
        return;
    }
    if (!QFile::rename(path, newPath)) {
        QMessageBox::warning(this, tr("无法重命名"), tr("重命名失败。"));
        return;
    }
    if (currentFilePath_ == path) {
        currentFilePath_ = newPath;
        updateEditorTitle();
    }
}

void EditorPage::deleteSelectedEntry() {
    const QString path = selectedPath();
    if (path.isEmpty() || path == projectRootPath_) {
        return;
    }

    const QFileInfo info(path);
    const QMessageBox::StandardButton result =
        QMessageBox::question(this, tr("删除"), tr("确定删除 %1 吗？").arg(info.fileName()),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (result != QMessageBox::Yes) {
        return;
    }

    bool ok = false;
    if (info.isDir()) {
        ok = QDir(path).removeRecursively();
    } else {
        ok = QFile::remove(path);
    }
    if (!ok) {
        QMessageBox::warning(this, tr("删除失败"), tr("无法删除所选项目。"));
        return;
    }
    if (currentFilePath_ == path || currentFilePath_.startsWith(path + QLatin1Char('/'))) {
        loadingFile_ = true;
        editor_->clear();
        loadingFile_ = false;
        currentFilePath_.clear();
        editor_->setEnabled(false);
        markDirty(false);
    }
}

QString EditorPage::selectedPath() const {
    const QModelIndex index = fileTree_->currentIndex();
    return index.isValid() ? QDir::cleanPath(fileModel_->filePath(index)) : QString();
}

QString EditorPage::relativeProjectPath(const QString& absolutePath) const {
    return QDir(projectRootPath_)
        .relativeFilePath(absolutePath)
        .replace(QLatin1Char('\\'), QLatin1Char('/'));
}

QString EditorPage::findMainTex() const {
    if (projectRootPath_.isEmpty()) {
        return QString();
    }

    const QString mainTex = QDir(projectRootPath_).filePath(QStringLiteral("main.tex"));
    if (QFileInfo::exists(mainTex)) {
        return mainTex;
    }

    QDirIterator it(projectRootPath_, {QStringLiteral("*.tex")}, QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        if (!relativeProjectPath(path).startsWith(QStringLiteral("build/"))) {
            return path;
        }
    }
    return QString();
}

bool EditorPage::isPathInsideProject(const QString& absolutePath) const {
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

void EditorPage::updateEditorTitle() {
    if (currentFilePath_.isEmpty()) {
        fileNameLabel_->setText(tr("未选择文件"));
        statusLabel_->setText(tr("就绪"));
        return;
    }

    const QString marker = isDirty_ ? QStringLiteral("● ") : QString();
    fileNameLabel_->setText(marker + relativeProjectPath(currentFilePath_));
    if (isDirty_) {
        statusLabel_->setText(tr("未保存，自动保存等待中"));
    } else if (statusLabel_->text().isEmpty() || statusLabel_->text().contains(tr("未保存"))) {
        statusLabel_->setText(tr("已保存"));
    }
}

}  // namespace pureleaf::ui
