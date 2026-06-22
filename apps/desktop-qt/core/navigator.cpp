#include "navigator.h"
#include "navpage.h"

#include <QStackedWidget>
#include <QWidget>

namespace pureleaf::ui {

Navigator::Navigator(QStackedWidget *stack, QObject *parent)
    : QObject(parent), stack_(stack), current_(PageId::Home) {}

void Navigator::registerPage(PageId id, QWidget *page) {
    const int index = stack_->addWidget(page);
    indexMap_[id] = index;
}

void Navigator::navigateTo(PageId id, const QVariant &payload) {
    auto it = indexMap_.constFind(id);
    if (it == indexMap_.constEnd()) return;

    // Notify the page we're leaving.
    if (auto *leaving = qobject_cast<NavPage *>(stack_->currentWidget())) {
        leaving->onPageLeft();
    }

    stack_->setCurrentIndex(it.value());
    current_ = id;

    // Forward the payload to the target page.
    auto *page = stack_->widget(it.value());
    if (auto *navPage = qobject_cast<NavPage *>(page)) {
        navPage->onPageEntered(payload);
    }

    emit pageChanged(id);
}

}  // namespace pureleaf::ui
