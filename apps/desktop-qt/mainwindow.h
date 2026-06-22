#pragma once

#include <QMainWindow>

class QEvent;
class QLabel;
class QStackedWidget;
class QToolButton;
class QWidget;

namespace QWK {
class WidgetWindowAgent;
}

namespace pureleaf::ui {

class Navigator;

/// Main application window.
///
/// Owns the QWindowKit-backed window chrome, the QStackedWidget page
/// container, and the Navigator.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void changeEvent(QEvent* event) override;

private:
    void setupWindowChrome();
    void setupPages();
    void wireNavigation();
    void updateWindowChrome();

    QWK::WidgetWindowAgent* windowAgent_;
    QWidget* titleBar_;
    QLabel* titleLabel_;
    QToolButton* maximizeButton_;

    QStackedWidget* stack_;
    Navigator* navigator_;

    // Pages (owned by stack_).
    class HomePage* homePage_;
    class EditorPage* editorPage_;
    class SettingsPage* settingsPage_;
};

}  // namespace pureleaf::ui
