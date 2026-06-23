#include "settingspage.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "components/icons/appicon.h"

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

    root->addWidget(topBar);
    root->addWidget(formWidget, 1);
}

}  // namespace pureleaf::ui
