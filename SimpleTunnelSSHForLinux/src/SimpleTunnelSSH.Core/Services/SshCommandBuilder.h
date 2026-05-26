#pragma once

#include <QStringList>

#include "Models/SshConnectionProfile.h"
#include "Models/TunnelGroup.h"

namespace SimpleTunnelSSH::Core::Services
{

class SshCommandBuilder
{
public:
    static QStringList buildArguments(
        const Models::SshConnectionProfile& connection,
        const QList<Models::TunnelGroup>& groups);
};

} // namespace SimpleTunnelSSH::Core::Services