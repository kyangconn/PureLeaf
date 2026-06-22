#pragma once

#include <QMap>
#include <QObject>
#include <QVariant>

class QStackedWidget;
class QWidget;

namespace pureleaf::ui {

/// Page identifiers for navigation.
enum class PageId {
    Home,      ///< Project list / launcher.
    Editor,    ///< LaTeX editor (file tree + code + PDF).
    Settings,  ///< Application settings.
};

/// Central navigation controller. Qt Widgets has no built-in router;
/// this wraps a QStackedWidget and exposes typed navigation calls.
///
/// Pages are registered once at startup via registerPage(). Navigation
/// is done by navigateTo() with an optional payload (e.g. the project
/// to open in the editor).
class Navigator : public QObject {
    Q_OBJECT

public:
    explicit Navigator(class QStackedWidget *stack, QObject *parent = nullptr);

    /// Registers a page widget at a given index slot.
    void registerPage(PageId id, class QWidget *page);

    /// Switches to a page. The optional payload is forwarded to the page
    /// via onPageEntered() if the page implements it.
    void navigateTo(PageId id, const QVariant &payload = {});

    PageId currentPage() const { return current_; }

signals:
    /// Emitted after a page switch. Pages can connect to this to do
    /// lazy refresh, but the convention is to override onPageEntered().
    void pageChanged(PageId id);

private:
    QStackedWidget *stack_;
    PageId current_;
    QMap<PageId, int> indexMap_;
};

}  // namespace pureleaf::ui
