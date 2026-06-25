#include "settingspage.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSize>
#include <QSpinBox>
#include <QVBoxLayout>

#include "components/icons/appicon.h"
#include "pureleaf/version.h"

namespace pureleaf::ui {

SettingsPage::SettingsPage(QWidget *parent) : NavPage(parent) {
    setupUi();
}

void SettingsPage::setupUi() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // --- Top bar ---
    auto *topBar = new QWidget(this);
    topBar->setFixedHeight(48);
    auto *topLayout = new QHBoxLayout(topBar);
    auto *backBtn = new QPushButton(tr("返回"), topBar);
    backBtn->setIcon(appIcon(AppIcon::Back, QColor(QStringLiteral("#475569")),
                             QColor(QStringLiteral("#0f172a"))));
    backBtn->setIconSize(QSize(18, 18));
    auto *title = new QLabel(tr("设置"), topBar);
    title->setAlignment(Qt::AlignCenter);
    topLayout->addWidget(backBtn);
    topLayout->addStretch();
    topLayout->addWidget(title);
    topLayout->addStretch();

    connect(backBtn, &QPushButton::clicked, this, &SettingsPage::backRequested);

    // --- Form ---
    auto *formWidget = new QWidget(this);
    auto *form = new QFormLayout(formWidget);
    form->setLabelAlignment(Qt::AlignRight);

    // TODO: load/save via pureleaf_core config service.
    auto *compilerEdit = new QLineEdit(formWidget);
    compilerEdit->setPlaceholderText(tr("xelatex"));
    form->addRow(tr("编译器"), compilerEdit);

    auto *timeoutSpin = new QSpinBox(formWidget);
    timeoutSpin->setRange(5, 300);
    timeoutSpin->setValue(30);
    timeoutSpin->setSuffix(tr(" 秒"));
    form->addRow(tr("编译超时"), timeoutSpin);

    // --- Version info ---
    const auto& v = pureleaf::getVersion();

    auto *versionSection = new QWidget(this);
    auto *versionLayout = new QVBoxLayout(versionSection);
    versionLayout->setContentsMargins(0, 16, 0, 0);

    auto *versionLabel = new QLabel(
        QStringLiteral("PureLeaf %1").arg(QString::fromStdString(v.full)),
        versionSection);
    versionLabel->setAlignment(Qt::AlignCenter);
    versionLabel->setStyleSheet(QStringLiteral("color: #64748b; font-size: 12px;"));

    auto *buildInfoLabel = new QLabel(
        QString::fromStdString(v.gitHash + " @ " + v.gitBranch + " | " +
                               v.buildType + " | " + v.compiler + " | " +
                               v.platform),
        versionSection);
    buildInfoLabel->setAlignment(Qt::AlignCenter);
    buildInfoLabel->setStyleSheet(QStringLiteral("color: #94a3b8; font-size: 10px;"));
    buildInfoLabel->setWordWrap(true);

    versionLayout->addWidget(versionLabel);
    versionLayout->addWidget(buildInfoLabel);

    root->addWidget(topBar);
    root->addWidget(formWidget, 1);
    root->addWidget(versionSection);
}

}  // namespace pureleaf::ui
