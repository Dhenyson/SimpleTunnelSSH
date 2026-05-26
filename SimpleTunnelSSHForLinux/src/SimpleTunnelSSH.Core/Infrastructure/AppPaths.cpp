#include "Infrastructure/AppPaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace SimpleTunnelSSH::Core::Infrastructure
{

namespace
{

QString appName()
{
    const auto configuredName = QCoreApplication::applicationName().trimmed();
    return configuredName.isEmpty() ? QStringLiteral("SimpleTunnelSSH") : configuredName;
}

QString fallbackConfigDirectory()
{
    return QDir::homePath() + QStringLiteral("/.config/") + appName();
}

} // namespace

QString AppPaths::getAppDirectory()
{
    auto path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    if (path.trimmed().isEmpty())
    {
        path = fallbackConfigDirectory();
    }

    return path;
}

QString AppPaths::getConfigurationFilePath()
{
    return QDir(getAppDirectory()).filePath(QStringLiteral("config.json"));
}

QString AppPaths::getAutostartDirectory()
{
    return QDir::homePath() + QStringLiteral("/.config/autostart");
}

QString AppPaths::getAutostartDesktopFilePath()
{
    return QDir(getAutostartDirectory()).filePath(QStringLiteral("simpletunnelssh.desktop"));
}

} // namespace SimpleTunnelSSH::Core::Infrastructure