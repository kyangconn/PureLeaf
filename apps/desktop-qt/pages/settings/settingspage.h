#pragma once

#include "core/navpage.h"

namespace pureleaf::ui {

/// Settings page — compiler config, theme, editor preferences.
class SettingsPage : public NavPage {
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

signals:
    void backRequested();

private:
    void setupUi();
};

}  // namespace pureleaf::ui
