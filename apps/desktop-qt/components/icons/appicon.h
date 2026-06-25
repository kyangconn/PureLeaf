#pragma once

#include <QColor>
#include <QIcon>
#include <QSize>

namespace pureleaf::ui {

/// Semantic icons used by the desktop UI.
///
/// Pages depend on these meanings instead of a concrete icon set. On Linux,
/// appIcon() first asks the active desktop icon theme (for example Breeze),
/// then falls back to the bundled Lucide asset.
enum class AppIcon {
    Leaf,
    NewProject,
    OpenFolder,
    Settings,
    WindowMinimize,
    WindowMaximize,
    WindowRestore,
    WindowClose,
    More,
    GitBranch,
    Back,
    Delete,
};

QIcon appIcon(AppIcon icon, const QColor& color, const QColor& activeColor = QColor{});

QPixmap appIconPixmap(AppIcon icon, const QSize& size, const QColor& color,
                      const QColor& activeColor = QColor{});

}  // namespace pureleaf::ui
