#pragma once

#include <QList>

namespace SimpleTunnelSSH::Core::Models
{

class AppSettings
{
public:
    static constexpr int DefaultWindowWidth = 1280;
    static constexpr int DefaultWindowHeight = 720;

    bool launchAtStartup { false };
    bool startMinimizedToTray { false };
    int windowWidth { DefaultWindowWidth };
    int windowHeight { DefaultWindowHeight };
    QList<int> groupColumnWidths { createDefaultGroupColumnWidths() };
    QList<int> portColumnWidths { createDefaultPortColumnWidths() };
    QList<int> logColumnWidths { createDefaultLogColumnWidths() };

    [[nodiscard]] AppSettings deepClone() const
    {
        return *this;
    }

    static QList<int> createDefaultGroupColumnWidths()
    {
        return { 220, 72, 260 };
    }

    static QList<int> createDefaultPortColumnWidths()
    {
        return { 86, 180, 92, 260 };
    }

    static QList<int> createDefaultLogColumnWidths()
    {
        return { 170, 90, 760 };
    }
};

} // namespace SimpleTunnelSSH::Core::Models