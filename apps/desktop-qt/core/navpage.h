#pragma once

#include <QVariant>
#include <QWidget>

namespace pureleaf::ui {

/// Base class for all top-level pages managed by Navigator.
///
/// Subclass this and override onPageEntered() to receive navigation
/// payloads (e.g. the project id when entering the editor).
class NavPage : public QWidget {
    Q_OBJECT

public:
    explicit NavPage(QWidget *parent = nullptr) : QWidget(parent) {}

    /// Called by Navigator when this page becomes active.
    /// `payload` carries optional context (project id, file path, etc.).
    virtual void onPageEntered(const QVariant &payload) { (void)payload; }

    /// Called by Navigator when this page is about to be left.
    /// Useful for flushing unsaved state.
    virtual void onPageLeft() {}
};

}  // namespace pureleaf::ui
