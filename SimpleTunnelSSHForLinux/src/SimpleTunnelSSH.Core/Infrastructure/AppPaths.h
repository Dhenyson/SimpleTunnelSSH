#pragma once

#include <QString>

namespace SimpleTunnelSSH::Core::Infrastructure
{

class AppPaths
{
public:
    static QString getAppDirectory();
    static QString getConfigurationFilePath();
    static QString getAutostartDirectory();
    static QString getAutostartDesktopFilePath();
};

} // namespace SimpleTunnelSSH::Core::Infrastructure