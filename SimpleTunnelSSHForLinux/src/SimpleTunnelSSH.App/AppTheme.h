#pragma once

#include <QColor>

class QApplication;
class QMenu;
class QWidget;

namespace SimpleTunnelSSH::App
{

class AppTheme
{
public:
    static QColor backgroundColor();
    static QColor surfaceColor();
    static QColor surfaceAltColor();
    static QColor headerColor();
    static QColor borderColor();
    static QColor foregroundColor();
    static QColor mutedForegroundColor();
    static QColor selectionColor();
    static QColor connectionConnectedColor();
    static QColor connectionDisconnectedColor();
    static QColor connectionAttentionColor();

    static void apply(QApplication& application);
    static void apply(QWidget* widget);
    static void applyMenu(QMenu* menu);
};

} // namespace SimpleTunnelSSH::App