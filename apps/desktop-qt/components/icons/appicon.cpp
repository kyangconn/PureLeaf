#include "appicon.h"

#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QHash>
#include <QIconEngine>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

#include <array>
#include <utility>

namespace pureleaf::ui {
namespace {

struct IconDefinition {
    const char* lucideName;
    std::array<const char*, 2> systemNames;
};

constexpr IconDefinition definition(AppIcon icon) {
    switch (icon) {
        case AppIcon::Leaf:
            return {"leaf", {nullptr, nullptr}};
        case AppIcon::NewProject:
            return {"file-plus", {"document-new", nullptr}};
        case AppIcon::OpenFolder:
            return {"folder-open", {"document-open", "folder-open"}};
        case AppIcon::Settings:
            return {"settings", {"configure", "preferences-system"}};
        case AppIcon::WindowMinimize:
            return {"minus", {"window-minimize", nullptr}};
        case AppIcon::WindowMaximize:
            return {"square", {"window-maximize", nullptr}};
        case AppIcon::WindowRestore:
            return {"copy", {"window-restore", nullptr}};
        case AppIcon::WindowClose:
            return {"x", {"window-close", nullptr}};
        case AppIcon::More:
            return {"ellipsis", {"view-more-symbolic", "view-more"}};
        case AppIcon::GitBranch:
            return {"git-branch", {"vcs-branch", nullptr}};
        case AppIcon::Back:
            return {"arrow-left", {"go-previous", "back"}};
        case AppIcon::Delete:
            return {"trash-2", {"edit-delete", nullptr}};
    }
    return {"square", {nullptr, nullptr}};
}

QByteArray lucideSvg(const char* name) {
    static QHash<QString, QByteArray> cache;
    const QString key = QString::fromLatin1(name);
    if (const auto it = cache.constFind(key); it != cache.cend()) {
        return it.value();
    }

    QFile file(QStringLiteral(":/pureleaf/icons/lucide/%1.svg").arg(key));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to load bundled Lucide icon" << file.fileName();
        return {};
    }

    const QByteArray data = file.readAll();
    cache.insert(key, data);
    return data;
}

class LucideIconEngine final : public QIconEngine {
public:
    LucideIconEngine(QByteArray svg, QColor color, QColor activeColor)
        : svg_(std::move(svg)), color_(std::move(color)),
          activeColor_(activeColor.isValid() ? std::move(activeColor) : color_) {}

    QIconEngine* clone() const override {
        return new LucideIconEngine(svg_, color_, activeColor_);
    }

    QString key() const override { return QStringLiteral("PureLeafLucide"); }

    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode,
               QIcon::State state) override {
        Q_UNUSED(state)

        QColor renderColor = mode == QIcon::Active ? activeColor_ : color_;
        if (mode == QIcon::Disabled) {
            renderColor.setAlphaF(renderColor.alphaF() * 0.45);
        }

        QByteArray renderedSvg = svg_;
        renderedSvg.replace("currentColor", renderColor.name(QColor::HexRgb).toUtf8());
        if (mode == QIcon::Disabled) {
            renderedSvg.replace("<svg", "<svg opacity=\"0.45\"");
        }
        QSvgRenderer renderer(renderedSvg);
        if (!renderer.isValid()) {
            qWarning() << "Unable to render bundled Lucide SVG";
            return;
        }
        renderer.render(painter, QRectF(rect));
    }

    QPixmap pixmap(const QSize& size, QIcon::Mode mode,
                   QIcon::State state) override {
        const qreal dpr = qApp == nullptr ? 1.0 : qApp->devicePixelRatio();
        const QSize pixelSize(qRound(size.width() * dpr), qRound(size.height() * dpr));
        QPixmap result(pixelSize);
        result.fill(Qt::transparent);
        result.setDevicePixelRatio(dpr);

        QPainter painter(&result);
        paint(&painter, QRect(QPoint{}, size), mode, state);
        return result;
    }

private:
    QByteArray svg_;
    QColor color_;
    QColor activeColor_;
};

QIcon systemThemeIcon(const IconDefinition& icon) {
#ifdef Q_OS_LINUX
    for (const char* name : icon.systemNames) {
        if (name == nullptr) {
            continue;
        }
        const QIcon themed = QIcon::fromTheme(QString::fromLatin1(name));
        if (!themed.isNull()) {
            return themed;
        }
    }
#else
    Q_UNUSED(icon)
#endif
    return {};
}

}  // namespace

QIcon appIcon(AppIcon icon, const QColor& color, const QColor& activeColor) {
    const IconDefinition iconDefinition = definition(icon);
    if (const QIcon themed = systemThemeIcon(iconDefinition); !themed.isNull()) {
        return themed;
    }

    const QByteArray svg = lucideSvg(iconDefinition.lucideName);
    if (svg.isEmpty()) {
        return {};
    }
    return QIcon(new LucideIconEngine(svg, color, activeColor));
}

QPixmap appIconPixmap(AppIcon icon, const QSize& size, const QColor& color,
                      const QColor& activeColor) {
    return appIcon(icon, color, activeColor).pixmap(size);
}

}  // namespace pureleaf::ui
