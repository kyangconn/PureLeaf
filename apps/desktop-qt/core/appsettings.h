#pragma once

#include <QMetaType>
#include <QString>

namespace pureleaf::ui {

struct AppSettings {
    QString compilerType = QStringLiteral("xelatex");
    QString compilerPath;
    QString bibtexPath;
    int compileTimeoutSeconds = 60;
    QString outputDirectory = QStringLiteral("build");
    bool cleanBeforeCompile = false;

    QString editorFontFamily = QStringLiteral("Cascadia Code");
    bool editorMonospaceOnly = true;
    int editorFontSize = 13;
    int tabWidth = 4;
    bool insertSpaces = true;
    bool autoSaveEnabled = true;
    int autoSaveDelayMs = 2000;
    bool completionEnabled = false;
    bool showLineNumbers = true;
    bool wordWrap = false;
    bool autoCloseBrackets = true;

    QString appTheme = QStringLiteral("system");
    QString colorScheme = QStringLiteral("PureLeaf Light");
    QString language = QStringLiteral("zh_CN");
    int uiScalePercent = 100;
};

AppSettings defaultAppSettings();
AppSettings loadAppSettings();
void saveAppSettings(const AppSettings& settings);

}  // namespace pureleaf::ui

Q_DECLARE_METATYPE(pureleaf::ui::AppSettings)
