#include "core/appsettings.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QtGlobal>

namespace pureleaf::ui {

namespace {

QString configFilePath() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty()) {
        configDir = QDir::home().filePath(QStringLiteral(".pureleaf"));
    }
    QDir().mkpath(configDir);
    return QDir(configDir).filePath(QStringLiteral("config.json"));
}

QJsonObject objectValue(const QJsonObject& parent, const QString& key) {
    const QJsonValue value = parent.value(key);
    return value.isObject() ? value.toObject() : QJsonObject{};
}

QString stringValue(const QJsonObject& object, const QString& key, const QString& fallback) {
    const QString value = object.value(key).toString(fallback).trimmed();
    return value.isEmpty() ? fallback : value;
}

bool boolValue(const QJsonObject& object, const QString& key, bool fallback) {
    const QJsonValue value = object.value(key);
    return value.isBool() ? value.toBool() : fallback;
}

int intValue(const QJsonObject& object, const QString& key, int fallback, int min, int max) {
    const QJsonValue value = object.value(key);
    const int parsed = value.isDouble() ? value.toInt(fallback) : fallback;
    return qBound(min, parsed, max);
}

}  // namespace

AppSettings defaultAppSettings() {
    return AppSettings{};
}

AppSettings loadAppSettings() {
    const AppSettings defaults = defaultAppSettings();
    AppSettings settings;

    QFile file(configFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return settings;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return settings;
    }

    const QJsonObject root = document.object();
    const QJsonObject compile = objectValue(root, QStringLiteral("compile"));
    settings.compilerType =
        stringValue(compile, QStringLiteral("compilerType"), defaults.compilerType);
    settings.compilerPath = compile.value(QStringLiteral("compilerPath")).toString().trimmed();
    settings.bibtexPath = compile.value(QStringLiteral("bibtexPath")).toString().trimmed();
    settings.compileTimeoutSeconds =
        intValue(compile, QStringLiteral("timeoutSeconds"), defaults.compileTimeoutSeconds, 5, 600);
    settings.outputDirectory =
        stringValue(compile, QStringLiteral("outputDirectory"), defaults.outputDirectory);
    settings.cleanBeforeCompile =
        boolValue(compile, QStringLiteral("cleanBeforeCompile"), defaults.cleanBeforeCompile);

    const QJsonObject editor = objectValue(root, QStringLiteral("editor"));
    settings.editorFontFamily =
        stringValue(editor, QStringLiteral("fontFamily"), defaults.editorFontFamily);
    settings.editorMonospaceOnly =
        boolValue(editor, QStringLiteral("monospaceOnly"), defaults.editorMonospaceOnly);
    settings.editorFontSize =
        intValue(editor, QStringLiteral("fontSize"), defaults.editorFontSize, 8, 36);
    settings.tabWidth = intValue(editor, QStringLiteral("tabWidth"), defaults.tabWidth, 2, 8);
    settings.insertSpaces =
        boolValue(editor, QStringLiteral("insertSpaces"), defaults.insertSpaces);
    settings.autoSaveEnabled =
        boolValue(editor, QStringLiteral("autoSaveEnabled"), defaults.autoSaveEnabled);
    settings.autoSaveDelayMs =
        intValue(editor, QStringLiteral("autoSaveDelayMs"), defaults.autoSaveDelayMs, 500, 10000);
    settings.completionEnabled =
        boolValue(editor, QStringLiteral("completionEnabled"), defaults.completionEnabled);
    settings.showLineNumbers =
        boolValue(editor, QStringLiteral("showLineNumbers"), defaults.showLineNumbers);
    settings.wordWrap = boolValue(editor, QStringLiteral("wordWrap"), defaults.wordWrap);
    settings.autoCloseBrackets =
        boolValue(editor, QStringLiteral("autoCloseBrackets"), defaults.autoCloseBrackets);

    const QJsonObject appearance = objectValue(root, QStringLiteral("appearance"));
    settings.appTheme = stringValue(appearance, QStringLiteral("appTheme"), defaults.appTheme);
    settings.colorScheme =
        stringValue(appearance, QStringLiteral("colorScheme"), defaults.colorScheme);
    settings.language = stringValue(appearance, QStringLiteral("language"), defaults.language);
    settings.uiScalePercent =
        intValue(appearance, QStringLiteral("uiScalePercent"), defaults.uiScalePercent, 80, 160);

    return settings;
}

void saveAppSettings(const AppSettings& settings) {
    QJsonObject compile;
    compile.insert(QStringLiteral("compilerType"), settings.compilerType);
    compile.insert(QStringLiteral("compilerPath"), settings.compilerPath.trimmed());
    compile.insert(QStringLiteral("bibtexPath"), settings.bibtexPath.trimmed());
    compile.insert(QStringLiteral("timeoutSeconds"),
                   qBound(5, settings.compileTimeoutSeconds, 600));
    compile.insert(QStringLiteral("outputDirectory"), settings.outputDirectory.trimmed().isEmpty()
                                                          ? QStringLiteral("build")
                                                          : settings.outputDirectory.trimmed());
    compile.insert(QStringLiteral("cleanBeforeCompile"), settings.cleanBeforeCompile);

    QJsonObject editor;
    editor.insert(QStringLiteral("fontFamily"), settings.editorFontFamily.trimmed());
    editor.insert(QStringLiteral("monospaceOnly"), settings.editorMonospaceOnly);
    editor.insert(QStringLiteral("fontSize"), qBound(8, settings.editorFontSize, 36));
    editor.insert(QStringLiteral("tabWidth"), qBound(2, settings.tabWidth, 8));
    editor.insert(QStringLiteral("insertSpaces"), settings.insertSpaces);
    editor.insert(QStringLiteral("autoSaveEnabled"), settings.autoSaveEnabled);
    editor.insert(QStringLiteral("autoSaveDelayMs"), qBound(500, settings.autoSaveDelayMs, 10000));
    editor.insert(QStringLiteral("completionEnabled"), settings.completionEnabled);
    editor.insert(QStringLiteral("showLineNumbers"), settings.showLineNumbers);
    editor.insert(QStringLiteral("wordWrap"), settings.wordWrap);
    editor.insert(QStringLiteral("autoCloseBrackets"), settings.autoCloseBrackets);

    QJsonObject appearance;
    appearance.insert(QStringLiteral("appTheme"), settings.appTheme);
    appearance.insert(QStringLiteral("colorScheme"), settings.colorScheme);
    appearance.insert(QStringLiteral("language"), settings.language);
    appearance.insert(QStringLiteral("uiScalePercent"), qBound(80, settings.uiScalePercent, 160));

    QJsonObject root;
    root.insert(QStringLiteral("compile"), compile);
    root.insert(QStringLiteral("editor"), editor);
    root.insert(QStringLiteral("appearance"), appearance);

    QFile file(configFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

}  // namespace pureleaf::ui
