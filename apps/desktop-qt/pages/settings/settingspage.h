#pragma once

#include "core/appsettings.h"
#include "core/navpage.h"

class QCheckBox;
class QComboBox;
class QFontComboBox;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace pureleaf::ui {

/// Settings page — compiler config, theme, editor preferences.
class SettingsPage : public NavPage {
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);

signals:
    void backRequested();
    void settingsApplied(AppSettings settings);

private:
    void setupUi();
    void loadSettings();
    AppSettings captureSettings() const;
    void applySettingsToControls(const AppSettings& settings);
    void markDirty(bool dirty);
    void connectDirtySignal(QWidget* widget);

    AppSettings settings_;
    bool loadingSettings_;

    QPushButton* saveButton_;
    QPushButton* applyButton_;
    QComboBox* compilerTypeCombo_;
    QLineEdit* compilerPathEdit_;
    QLineEdit* bibtexPathEdit_;
    QSpinBox* timeoutSpin_;
    QLineEdit* outputDirEdit_;
    QCheckBox* cleanBeforeCompileCheck_;
    QFontComboBox* fontCombo_;
    QCheckBox* monospaceOnlyCheck_;
    QSpinBox* fontSizeSpin_;
    QSpinBox* tabWidthSpin_;
    QCheckBox* insertSpacesCheck_;
    QCheckBox* autoSaveCheck_;
    QSpinBox* autoSaveDelaySpin_;
    QCheckBox* completionCheck_;
    QCheckBox* showLineNumbersCheck_;
    QCheckBox* wordWrapCheck_;
    QCheckBox* autoCloseBracketsCheck_;
    QComboBox* themeCombo_;
    QComboBox* colorSchemeCombo_;
    QComboBox* languageCombo_;
    QSpinBox* scaleSpin_;
};

}  // namespace pureleaf::ui
