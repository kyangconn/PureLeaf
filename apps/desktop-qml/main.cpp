#include <QWKQuick/qwkquickglobal.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QUrl>

#include "controller/desktopqmlcontroller.h"
#include "core/appsettings.h"

namespace {

QString platformDefaultStyle() {
#ifdef Q_OS_WIN
    return QStringLiteral("FluentWinUI3");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("Fusion");
#else
    return QStringLiteral("Fusion");
#endif
}

QString resolvedStyle(const pureleaf::ui::AppSettings& settings) {
    const QString requested = settings.controlsStyle.trimmed();
    if (requested.isEmpty() || requested == QStringLiteral("platform")) {
        return platformDefaultStyle();
    }

    if (requested.compare(QStringLiteral("breeze"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("Breeze");
    }
    if (requested.compare(QStringLiteral("fluent"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("FluentWinUI3");
    }
    if (requested.compare(QStringLiteral("fusion"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("Fusion");
    }

    return requested;
}

/// Picks a default Qt Quick Controls 2 style per platform, without overriding
/// an explicit user choice supplied via the `QT_QUICK_CONTROLS_STYLE` env var
/// or the `-style` command-line argument (todo.md §2).
void applyDefaultStyle(const pureleaf::ui::AppSettings& settings) {
    if (!qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        return;  // Respect explicit env override.
    }
    const QStringList args = QGuiApplication::arguments();
    for (const QString& arg : args) {
        if (arg == QStringLiteral("-style") || arg.startsWith(QStringLiteral("-style="))) {
            return;  // Respect explicit CLI override.
        }
    }

    QQuickStyle::setStyle(resolvedStyle(settings));
}

}  // namespace

int main(int argc, char* argv[]) {
#ifdef Q_OS_WIN
    qputenv("QT_QPA_DISABLE_REDIRECTION_SURFACE", "1");
#endif

    QGuiApplication::setApplicationName(QStringLiteral("PureLeaf"));
    QGuiApplication::setOrganizationName(QStringLiteral("PureLeaf"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("PureLeaf"));
    QQuickWindow::setDefaultAlphaBuffer(true);

    QGuiApplication app(argc, argv);
    const pureleaf::ui::AppSettings settings = pureleaf::ui::loadAppSettings();
    applyDefaultStyle(settings);

    pureleaf::qml::DesktopQmlController controller(settings);

    QQmlApplicationEngine engine;
    QWK::registerTypes(&engine);
    engine.rootContext()->setContextProperty(QStringLiteral("controller"), &controller);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("PureLeaf.Desktop", "Main");

    return QGuiApplication::exec();
}
