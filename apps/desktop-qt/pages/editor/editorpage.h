#pragma once

#include <QString>

#include "core/navpage.h"

class QLabel;

namespace pureleaf::ui {

/// Editor page — the main writing surface.
///
/// Layout (from todo.md): file tree (left) | code editor (center) | PDF preview (right).
/// Top bar has: back button, project name (center), action buttons (right).
class EditorPage : public NavPage {
    Q_OBJECT

public:
    explicit EditorPage(QWidget* parent = nullptr);

    void onPageEntered(const QVariant& payload) override;
    void onPageLeft() override;

signals:
    /// Emitted when the user clicks "back" to return home.
    void backRequested();

private:
    void setupUi();

    QString currentProjectId_;
    QLabel* projectNameLabel_;
};

}  // namespace pureleaf::ui
