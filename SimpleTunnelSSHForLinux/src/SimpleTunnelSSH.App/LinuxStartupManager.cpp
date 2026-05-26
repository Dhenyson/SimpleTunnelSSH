#include "LinuxStartupManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTextStream>

#include <stdexcept>

#include "Infrastructure/AppPaths.h"

namespace SimpleTunnelSSH::App
{

namespace
{

QString escapeDesktopExec(QString value)
{
    value.replace('\\', QStringLiteral("\\\\"));
    value.replace('"', QStringLiteral("\\\""));
    return QStringLiteral("\"%1\"").arg(value);
}

QString buildDesktopEntry()
{
    const auto executablePath = escapeDesktopExec(QCoreApplication::applicationFilePath());

    return QStringLiteral(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Version=1.0\n"
        "Name=Simple Tunnel SSH\n"
        "Comment=Lightweight SSH tunnel manager\n"
        "Exec=%1\n"
        "Icon=simpletunnelssh\n"
        "Terminal=false\n"
        "Categories=Network;Utility;\n"
        "X-GNOME-Autostart-enabled=true\n")
        .arg(executablePath);
}

} // namespace

void LinuxStartupManager::setEnabled(bool enabled)
{
    const auto filePath = SimpleTunnelSSH::Core::Infrastructure::AppPaths::getAutostartDesktopFilePath();

    if (!enabled)
    {
        QFile::remove(filePath);
        return;
    }

    QDir().mkpath(QFileInfo(filePath).absolutePath());

    QSaveFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        throw std::runtime_error(QStringLiteral("Unable to write %1.").arg(filePath).toStdString());
    }

    QTextStream stream(&file);
    stream << buildDesktopEntry();

    if (!file.commit())
    {
        throw std::runtime_error(QStringLiteral("Unable to commit %1.").arg(filePath).toStdString());
    }
}

bool LinuxStartupManager::isEnabled()
{
    return QFile::exists(SimpleTunnelSSH::Core::Infrastructure::AppPaths::getAutostartDesktopFilePath());
}

} // namespace SimpleTunnelSSH::App