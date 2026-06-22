#pragma once

#include "core/navpage.h"

namespace pureleaf::ui {

/// Home page — project list / launcher.
/// Shows existing projects and a "new project" entry point.
class HomePage : public NavPage {
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);

    void onPageEntered(const QVariant &payload) override;

signals:
    /// Emitted when the user wants to open a project.
    void projectOpenRequested(const QString &projectId);
    /// Emitted when the user wants to go to settings.
    void settingsRequested();
    /// Emitted when the user wants to create a new project.
    void newProjectRequested();

private:
    void setupUi();
};

}  // namespace pureleaf::ui
