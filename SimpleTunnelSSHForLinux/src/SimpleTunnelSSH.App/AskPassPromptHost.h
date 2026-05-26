#pragma once

#include <QStringList>

namespace SimpleTunnelSSH::App
{

class AskPassPromptHost
{
public:
    static bool isAskPassMode();
    static int run(const QStringList& args);
};

} // namespace SimpleTunnelSSH::App