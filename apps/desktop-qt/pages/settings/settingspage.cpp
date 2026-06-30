#include "settingspage.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFont>
#include <QFontComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSize>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

#include "components/icons/appicon.h"
#include "pureleaf/version.h"

namespace pureleaf::ui {

namespace {

class SettingsComboBox : public QComboBox {
public:
    using QComboBox::QComboBox;
    ~SettingsComboBox() override = default;

    void showPopup() override {
        if (auto* popupView = view()) {
            const int popupWidth = qMax(1, width());
            popupView->setMinimumWidth(popupWidth);
            popupView->setMaximumWidth(popupWidth);
        }
        QComboBox::showPopup();
    }
};

class SettingsFontComboBox : public QFontComboBox {
public:
    using QFontComboBox::QFontComboBox;
    ~SettingsFontComboBox() override = default;

    void showPopup() override {
        if (auto* popupView = view()) {
            const int popupWidth = qMax(1, width());
            popupView->setMinimumWidth(popupWidth);
            popupView->setMaximumWidth(popupWidth);
        }
        QFontComboBox::showPopup();
    }
};

QWidget* makeSettingsTab(QWidget* parent, QFormLayout** formOut) {
    auto* scrollArea = new QScrollArea(parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* viewport = new QWidget(scrollArea);
    auto* viewportLayout = new QVBoxLayout(viewport);
    viewportLayout->setContentsMargins(0, 0, 0, 0);
    viewportLayout->setSpacing(0);

    auto* body = new QWidget(viewport);
    body->setObjectName(QStringLiteral("settingsFormBody"));
    body->setMaximumWidth(760);
    auto* outer = new QVBoxLayout(body);
    outer->setContentsMargins(28, 24, 28, 24);
    outer->setSpacing(14);

    auto* form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);
    form->setFormAlignment(Qt::AlignTop);
    form->setHorizontalSpacing(18);
    form->setVerticalSpacing(12);
    outer->addLayout(form);
    outer->addStretch();

    viewportLayout->addWidget(body, 0, Qt::AlignTop | Qt::AlignHCenter);
    viewportLayout->addStretch(1);
    scrollArea->setWidget(viewport);
    if (formOut) {
        *formOut = form;
    }
    return scrollArea;
}

QWidget* makePathPicker(QWidget* parent, const QString& placeholder, QLineEdit** editOut) {
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* edit = new QLineEdit(row);
    edit->setPlaceholderText(placeholder);
    if (editOut) {
        *editOut = edit;
    }
    auto* browse = new QPushButton(QObject::tr("浏览"), row);
    browse->setFixedWidth(72);
    layout->addWidget(edit, 1);
    layout->addWidget(browse);

    QObject::connect(browse, &QPushButton::clicked, row, [edit, row]() {
        const QString file = QFileDialog::getOpenFileName(row, QObject::tr("选择程序"));
        if (!file.isEmpty()) {
            edit->setText(file);
        }
    });

    return row;
}

}  // namespace

SettingsPage::SettingsPage(QWidget* parent)
    : NavPage(parent),
      loadingSettings_(false),
      saveButton_(nullptr),
      applyButton_(nullptr),
      compilerTypeCombo_(nullptr),
      compilerPathEdit_(nullptr),
      bibtexPathEdit_(nullptr),
      timeoutSpin_(nullptr),
      outputDirEdit_(nullptr),
      cleanBeforeCompileCheck_(nullptr),
      fontCombo_(nullptr),
      monospaceOnlyCheck_(nullptr),
      fontSizeSpin_(nullptr),
      tabWidthSpin_(nullptr),
      insertSpacesCheck_(nullptr),
      autoSaveCheck_(nullptr),
      autoSaveDelaySpin_(nullptr),
      completionCheck_(nullptr),
      showLineNumbersCheck_(nullptr),
      wordWrapCheck_(nullptr),
      autoCloseBracketsCheck_(nullptr),
      themeCombo_(nullptr),
      colorSchemeCombo_(nullptr),
      languageCombo_(nullptr),
      scaleSpin_(nullptr) {
    setupUi();
    loadSettings();
}

void SettingsPage::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* topBar = new QWidget(this);
    topBar->setObjectName(QStringLiteral("settingsTopBar"));
    topBar->setFixedHeight(48);
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(12, 0, 12, 0);
    topLayout->setSpacing(8);

    auto* backBtn = new QPushButton(tr("返回"), topBar);
    backBtn->setIcon(appIcon(AppIcon::Back, QColor(QStringLiteral("#475569")),
                             QColor(QStringLiteral("#0f172a"))));
    backBtn->setIconSize(QSize(18, 18));

    auto* title = new QLabel(tr("设置"), topBar);
    title->setObjectName(QStringLiteral("settingsTitle"));
    title->setAlignment(Qt::AlignCenter);

    saveButton_ = new QPushButton(tr("保存"), topBar);
    saveButton_->setEnabled(false);
    auto* cancelButton = new QPushButton(tr("取消"), topBar);
    applyButton_ = new QPushButton(tr("应用"), topBar);
    applyButton_->setEnabled(false);

    topLayout->addWidget(backBtn);
    topLayout->addWidget(title, 1);
    topLayout->addWidget(saveButton_);
    topLayout->addWidget(cancelButton);
    topLayout->addWidget(applyButton_);

    connect(backBtn, &QPushButton::clicked, this, &SettingsPage::backRequested);
    connect(cancelButton, &QPushButton::clicked, this, &SettingsPage::backRequested);
    connect(saveButton_, &QPushButton::clicked, this, [this]() {
        settings_ = captureSettings();
        saveAppSettings(settings_);
        markDirty(false);
        emit settingsApplied(settings_);
        emit backRequested();
    });
    connect(applyButton_, &QPushButton::clicked, this, [this]() {
        settings_ = captureSettings();
        saveAppSettings(settings_);
        markDirty(false);
        emit settingsApplied(settings_);
    });

    auto* tabs = new QTabWidget(this);
    tabs->setObjectName(QStringLiteral("settingsTabs"));

    QFormLayout* compileForm = nullptr;
    auto* compileTab = makeSettingsTab(tabs, &compileForm);
    compilerTypeCombo_ = [compileTab]() {
        auto* combo = new SettingsComboBox(compileTab);
        combo->addItems(
            {QStringLiteral("xelatex"), QStringLiteral("pdflatex"), QStringLiteral("lualatex")});
        return combo;
    }();
    compileForm->addRow(tr("编译器类型"), compilerTypeCombo_);
    compileForm->addRow(tr("编译器路径"),
                        makePathPicker(compileTab, tr("从 PATH 查找 xelatex"), &compilerPathEdit_));
    compileForm->addRow(tr("BibTeX 路径"),
                        makePathPicker(compileTab, tr("从 PATH 查找 bibtex"), &bibtexPathEdit_));

    timeoutSpin_ = new QSpinBox(compileTab);
    timeoutSpin_->setRange(5, 600);
    timeoutSpin_->setValue(60);
    timeoutSpin_->setSuffix(tr(" 秒"));
    compileForm->addRow(tr("编译超时"), timeoutSpin_);

    outputDirEdit_ = new QLineEdit(compileTab);
    outputDirEdit_->setText(QStringLiteral("build"));
    compileForm->addRow(tr("输出目录"), outputDirEdit_);

    cleanBeforeCompileCheck_ = new QCheckBox(tr("编译前清理辅助文件"), compileTab);
    compileForm->addRow(QString(), cleanBeforeCompileCheck_);

    QFormLayout* editorForm = nullptr;
    auto* editorTab = makeSettingsTab(tabs, &editorForm);
    fontCombo_ = new SettingsFontComboBox(editorTab);
    fontCombo_->setCurrentFont(QFont(QStringLiteral("Cascadia Code")));
    editorForm->addRow(tr("字体"), fontCombo_);

    monospaceOnlyCheck_ = new QCheckBox(tr("只显示等宽字体"), editorTab);
    monospaceOnlyCheck_->setChecked(true);
    editorForm->addRow(QString(), monospaceOnlyCheck_);

    fontSizeSpin_ = new QSpinBox(editorTab);
    fontSizeSpin_->setRange(8, 36);
    fontSizeSpin_->setValue(13);
    editorForm->addRow(tr("字号"), fontSizeSpin_);

    tabWidthSpin_ = new QSpinBox(editorTab);
    tabWidthSpin_->setRange(2, 8);
    tabWidthSpin_->setValue(4);
    editorForm->addRow(tr("Tab 宽度"), tabWidthSpin_);

    insertSpacesCheck_ = new QCheckBox(tr("Tab 转空格"), editorTab);
    insertSpacesCheck_->setChecked(true);
    editorForm->addRow(QString(), insertSpacesCheck_);

    autoSaveCheck_ = new QCheckBox(tr("启用自动保存"), editorTab);
    autoSaveCheck_->setChecked(true);
    editorForm->addRow(QString(), autoSaveCheck_);

    autoSaveDelaySpin_ = new QSpinBox(editorTab);
    autoSaveDelaySpin_->setRange(500, 10000);
    autoSaveDelaySpin_->setSingleStep(500);
    autoSaveDelaySpin_->setValue(2000);
    autoSaveDelaySpin_->setSuffix(tr(" ms"));
    editorForm->addRow(tr("自动保存延迟"), autoSaveDelaySpin_);

    completionCheck_ = new QCheckBox(tr("自动补全"), editorTab);
    editorForm->addRow(QString(), completionCheck_);

    showLineNumbersCheck_ = new QCheckBox(tr("显示行号"), editorTab);
    showLineNumbersCheck_->setChecked(true);
    editorForm->addRow(QString(), showLineNumbersCheck_);

    wordWrapCheck_ = new QCheckBox(tr("自动换行"), editorTab);
    editorForm->addRow(QString(), wordWrapCheck_);

    autoCloseBracketsCheck_ = new QCheckBox(tr("括号自动闭合"), editorTab);
    autoCloseBracketsCheck_->setChecked(true);
    editorForm->addRow(QString(), autoCloseBracketsCheck_);

    QFormLayout* appearanceForm = nullptr;
    auto* appearanceTab = makeSettingsTab(tabs, &appearanceForm);
    themeCombo_ = new SettingsComboBox(appearanceTab);
    themeCombo_->addItem(tr("跟随系统"), QStringLiteral("system"));
    themeCombo_->addItem(tr("亮色"), QStringLiteral("light"));
    themeCombo_->addItem(tr("暗色"), QStringLiteral("dark"));
    appearanceForm->addRow(tr("应用主题"), themeCombo_);

    colorSchemeCombo_ = new SettingsComboBox(appearanceTab);
    colorSchemeCombo_->addItems({QStringLiteral("PureLeaf Light"), QStringLiteral("VS Code Dark"),
                                 QStringLiteral("Monokai"), QStringLiteral("Solarized")});
    appearanceForm->addRow(tr("编辑器配色"), colorSchemeCombo_);

    languageCombo_ = new SettingsComboBox(appearanceTab);
    languageCombo_->addItem(QStringLiteral("中文"), QStringLiteral("zh_CN"));
    languageCombo_->addItem(QStringLiteral("English"), QStringLiteral("en_US"));
    appearanceForm->addRow(tr("界面语言"), languageCombo_);

    scaleSpin_ = new QSpinBox(appearanceTab);
    scaleSpin_->setRange(80, 160);
    scaleSpin_->setValue(100);
    scaleSpin_->setSuffix(QStringLiteral(" %"));
    appearanceForm->addRow(tr("界面缩放"), scaleSpin_);

    auto* shortcutsTab = new QWidget(tabs);
    auto* shortcutsLayout = new QVBoxLayout(shortcutsTab);
    shortcutsLayout->setContentsMargins(28, 24, 28, 24);
    shortcutsLayout->setSpacing(8);

    auto* shortcutTable = new QTableWidget(shortcutsTab);
    shortcutTable->setColumnCount(2);
    shortcutTable->setHorizontalHeaderLabels({tr("动作"), tr("快捷键")});
    shortcutTable->horizontalHeader()->setStretchLastSection(true);
    shortcutTable->verticalHeader()->setVisible(false);
    shortcutTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    shortcutTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    const QList<QPair<QString, QString>> shortcuts = {
        {tr("保存当前文件"), QStringLiteral("Ctrl+S")}, {tr("编译项目"), QStringLiteral("F5")},
        {tr("查找"), QStringLiteral("Ctrl+F")},         {tr("跳转到行"), QStringLiteral("Ctrl+G")},
        {tr("打开设置"), QStringLiteral("Ctrl+,")},
    };
    shortcutTable->setRowCount(shortcuts.size());
    for (int row = 0; row < shortcuts.size(); ++row) {
        shortcutTable->setItem(row, 0, new QTableWidgetItem(shortcuts.at(row).first));
        shortcutTable->setItem(row, 1, new QTableWidgetItem(shortcuts.at(row).second));
    }
    shortcutsLayout->addWidget(shortcutTable);

    auto* aboutTab = new QWidget(tabs);
    auto* aboutLayout = new QVBoxLayout(aboutTab);
    aboutLayout->setContentsMargins(28, 24, 28, 24);
    aboutLayout->setSpacing(8);

    const auto& v = pureleaf::getVersion();
    auto* versionLabel =
        new QLabel(QStringLiteral("PureLeaf %1").arg(QString::fromStdString(v.full)), aboutTab);
    versionLabel->setObjectName(QStringLiteral("settingsVersion"));

    auto* buildInfoLabel =
        new QLabel(QString::fromStdString(v.gitHash + " @ " + v.gitBranch + " | " + v.buildType +
                                          " | " + v.compiler + " | " + v.platform),
                   aboutTab);
    buildInfoLabel->setObjectName(QStringLiteral("settingsBuildInfo"));
    buildInfoLabel->setWordWrap(true);

    aboutLayout->addWidget(versionLabel);
    aboutLayout->addWidget(buildInfoLabel);
    aboutLayout->addStretch();

    tabs->addTab(compileTab, tr("编译"));
    tabs->addTab(editorTab, tr("编辑器"));
    tabs->addTab(appearanceTab, tr("外观"));
    tabs->addTab(shortcutsTab, tr("快捷键"));
    tabs->addTab(aboutTab, tr("关于"));

    root->addWidget(topBar);
    root->addWidget(tabs, 1);

    for (QWidget* widget : {static_cast<QWidget*>(compilerTypeCombo_),
                            static_cast<QWidget*>(compilerPathEdit_),
                            static_cast<QWidget*>(bibtexPathEdit_),
                            static_cast<QWidget*>(timeoutSpin_),
                            static_cast<QWidget*>(outputDirEdit_),
                            static_cast<QWidget*>(cleanBeforeCompileCheck_),
                            static_cast<QWidget*>(fontCombo_),
                            static_cast<QWidget*>(monospaceOnlyCheck_),
                            static_cast<QWidget*>(fontSizeSpin_),
                            static_cast<QWidget*>(tabWidthSpin_),
                            static_cast<QWidget*>(insertSpacesCheck_),
                            static_cast<QWidget*>(autoSaveCheck_),
                            static_cast<QWidget*>(autoSaveDelaySpin_),
                            static_cast<QWidget*>(completionCheck_),
                            static_cast<QWidget*>(showLineNumbersCheck_),
                            static_cast<QWidget*>(wordWrapCheck_),
                            static_cast<QWidget*>(autoCloseBracketsCheck_),
                            static_cast<QWidget*>(themeCombo_),
                            static_cast<QWidget*>(colorSchemeCombo_),
                            static_cast<QWidget*>(languageCombo_),
                            static_cast<QWidget*>(scaleSpin_)}) {
        connectDirtySignal(widget);
    }
    connect(monospaceOnlyCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        const QFont currentFont = fontCombo_->currentFont();
        fontCombo_->setFontFilters(checked ? QFontComboBox::MonospacedFonts
                                           : QFontComboBox::AllFonts);
        fontCombo_->setCurrentFont(currentFont);
    });

    setStyleSheet(QStringLiteral(R"(
        QWidget#settingsTopBar {
            background: #f8fafc;
            border-bottom: 1px solid #e2e8f0;
        }

        QLabel#settingsTitle {
            color: #0f172a;
            font-size: 13px;
            font-weight: 700;
        }

        QTabWidget#settingsTabs::pane {
            border: none;
            background: #f8fafc;
        }

        QTabBar::tab {
            min-width: 86px;
            padding: 9px 14px;
            color: #475569;
            background: #f8fafc;
            border: none;
            border-bottom: 2px solid transparent;
        }

        QTabBar::tab:selected {
            color: #15803d;
            border-bottom-color: #16a34a;
            font-weight: 700;
        }

        QScrollArea {
            background: #f8fafc;
        }

        QWidget#settingsFormBody {
            background: transparent;
        }

        QLineEdit, QSpinBox, QComboBox, QFontComboBox {
            min-height: 30px;
            border: 1px solid #cbd5e1;
            border-radius: 7px;
            padding: 2px 8px;
            background: white;
            color: #0f172a;
        }

        QSpinBox {
            padding-right: 6px;
        }

        QComboBox, QFontComboBox {
            padding-right: 34px;
        }

        QComboBox::drop-down, QFontComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 30px;
            border-left: 1px solid #e2e8f0;
            border-top-right-radius: 7px;
            border-bottom-right-radius: 7px;
            background: #f8fafc;
        }

        QComboBox::down-arrow, QFontComboBox::down-arrow {
            image: url(:/pureleaf/icons/lucide/chevron-down.svg);
            width: 14px;
            height: 14px;
        }

        QComboBox::drop-down:hover, QFontComboBox::drop-down:hover {
            background: #f1f5f9;
        }

        QComboBox QAbstractItemView, QFontComboBox QAbstractItemView {
            outline: none;
            border: 1px solid #cbd5e1;
            border-radius: 8px;
            background: white;
            color: #0f172a;
            selection-background-color: #dcfce7;
            selection-color: #0f172a;
        }

        QTableWidget {
            background: white;
            border: 1px solid #e2e8f0;
            border-radius: 8px;
            color: #0f172a;
            gridline-color: #e2e8f0;
        }

        QHeaderView::section {
            background: #f1f5f9;
            color: #334155;
            border: none;
            border-bottom: 1px solid #e2e8f0;
            padding: 7px;
            font-weight: 700;
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

        QLabel#settingsVersion {
            color: #0f172a;
            font-size: 16px;
            font-weight: 700;
        }

        QLabel#settingsBuildInfo {
            color: #64748b;
            font-size: 12px;
        }
    )"));
}

void SettingsPage::loadSettings() {
    settings_ = loadAppSettings();
    applySettingsToControls(settings_);
    markDirty(false);
}

AppSettings SettingsPage::captureSettings() const {
    AppSettings settings;
    settings.compilerType = compilerTypeCombo_->currentText();
    settings.compilerPath = compilerPathEdit_->text().trimmed();
    settings.bibtexPath = bibtexPathEdit_->text().trimmed();
    settings.compileTimeoutSeconds = timeoutSpin_->value();
    settings.outputDirectory = outputDirEdit_->text().trimmed().isEmpty()
                                   ? QStringLiteral("build")
                                   : outputDirEdit_->text().trimmed();
    settings.cleanBeforeCompile = cleanBeforeCompileCheck_->isChecked();

    settings.editorFontFamily = fontCombo_->currentFont().family();
    settings.editorMonospaceOnly = monospaceOnlyCheck_->isChecked();
    settings.editorFontSize = fontSizeSpin_->value();
    settings.tabWidth = tabWidthSpin_->value();
    settings.insertSpaces = insertSpacesCheck_->isChecked();
    settings.autoSaveEnabled = autoSaveCheck_->isChecked();
    settings.autoSaveDelayMs = autoSaveDelaySpin_->value();
    settings.completionEnabled = completionCheck_->isChecked();
    settings.showLineNumbers = showLineNumbersCheck_->isChecked();
    settings.wordWrap = wordWrapCheck_->isChecked();
    settings.autoCloseBrackets = autoCloseBracketsCheck_->isChecked();

    settings.appTheme = themeCombo_->currentData().toString();
    settings.colorScheme = colorSchemeCombo_->currentText();
    settings.language = languageCombo_->currentData().toString();
    settings.uiScalePercent = scaleSpin_->value();
    return settings;
}

void SettingsPage::applySettingsToControls(const AppSettings& settings) {
    loadingSettings_ = true;

    const int compilerIndex = compilerTypeCombo_->findText(settings.compilerType);
    compilerTypeCombo_->setCurrentIndex(compilerIndex >= 0 ? compilerIndex : 0);
    compilerPathEdit_->setText(settings.compilerPath);
    bibtexPathEdit_->setText(settings.bibtexPath);
    timeoutSpin_->setValue(settings.compileTimeoutSeconds);
    outputDirEdit_->setText(settings.outputDirectory);
    cleanBeforeCompileCheck_->setChecked(settings.cleanBeforeCompile);

    monospaceOnlyCheck_->setChecked(settings.editorMonospaceOnly);
    fontCombo_->setFontFilters(settings.editorMonospaceOnly ? QFontComboBox::MonospacedFonts
                                                            : QFontComboBox::AllFonts);
    fontCombo_->setCurrentFont(QFont(settings.editorFontFamily));
    fontSizeSpin_->setValue(settings.editorFontSize);
    tabWidthSpin_->setValue(settings.tabWidth);
    insertSpacesCheck_->setChecked(settings.insertSpaces);
    autoSaveCheck_->setChecked(settings.autoSaveEnabled);
    autoSaveDelaySpin_->setValue(settings.autoSaveDelayMs);
    completionCheck_->setChecked(settings.completionEnabled);
    showLineNumbersCheck_->setChecked(settings.showLineNumbers);
    wordWrapCheck_->setChecked(settings.wordWrap);
    autoCloseBracketsCheck_->setChecked(settings.autoCloseBrackets);

    const int themeIndex = themeCombo_->findData(settings.appTheme);
    themeCombo_->setCurrentIndex(themeIndex >= 0 ? themeIndex : 0);
    const int schemeIndex = colorSchemeCombo_->findText(settings.colorScheme);
    colorSchemeCombo_->setCurrentIndex(schemeIndex >= 0 ? schemeIndex : 0);
    const int languageIndex = languageCombo_->findData(settings.language);
    languageCombo_->setCurrentIndex(languageIndex >= 0 ? languageIndex : 0);
    scaleSpin_->setValue(settings.uiScalePercent);

    loadingSettings_ = false;
}

void SettingsPage::markDirty(bool dirty) {
    if (loadingSettings_) {
        return;
    }
    saveButton_->setEnabled(dirty);
    applyButton_->setEnabled(dirty);
}

void SettingsPage::connectDirtySignal(QWidget* widget) {
    if (auto* lineEdit = qobject_cast<QLineEdit*>(widget)) {
        connect(lineEdit, &QLineEdit::textChanged, this, [this]() { markDirty(true); });
    } else if (auto* combo = qobject_cast<QComboBox*>(widget)) {
        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this]() { markDirty(true); });
    } else if (auto* spin = qobject_cast<QSpinBox*>(widget)) {
        connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this,
                [this]() { markDirty(true); });
    } else if (auto* check = qobject_cast<QCheckBox*>(widget)) {
        connect(check, &QCheckBox::toggled, this, [this]() { markDirty(true); });
    }
}

}  // namespace pureleaf::ui
