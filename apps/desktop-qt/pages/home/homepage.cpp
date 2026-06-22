#include "homepage.h"

#include <QFont>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace pureleaf::ui {

HomePage::HomePage(QWidget *parent) : NavPage(parent) {
    setupUi();
}

void HomePage::setupUi() {
    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel(tr("PureLeaf"), this);
    title->setObjectName("homeTitle");
    QFont titleFont = title->font();
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto *subtitle = new QLabel(tr("本地 LaTeX 写作环境"), this);

    auto *newBtn = new QPushButton(tr("新建项目"), this);
    auto *settingsBtn = new QPushButton(tr("设置"), this);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(20);
    layout->addWidget(newBtn);
    layout->addWidget(settingsBtn);
    layout->addStretch();

    connect(newBtn, &QPushButton::clicked, this, &HomePage::newProjectRequested);
    connect(settingsBtn, &QPushButton::clicked, this, &HomePage::settingsRequested);
}

void HomePage::onPageEntered(const QVariant &payload) {
    (void)payload;
    // TODO: refresh project list from storage.
}

}  // namespace pureleaf::ui
