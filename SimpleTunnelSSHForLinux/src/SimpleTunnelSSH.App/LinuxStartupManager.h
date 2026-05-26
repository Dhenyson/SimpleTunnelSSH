#pragma once

namespace SimpleTunnelSSH::App
{

class LinuxStartupManager
{
public:
    static void setEnabled(bool enabled);
    static bool isEnabled();
};

} // namespace SimpleTunnelSSH::App