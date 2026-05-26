#pragma once

#include <QList>
#include <QUuid>

#include "Models/AppSettings.h"
#include "Models/SshConnectionProfile.h"

namespace SimpleTunnelSSH::Core::Models
{

class AppConfiguration
{
public:
    int schemaVersion { 2 };
    QUuid lastSelectedConnectionId;
    AppSettings settings;
    QList<SshConnectionProfile> connections;

    [[nodiscard]] AppConfiguration deepClone() const
    {
        return *this;
    }
};

} // namespace SimpleTunnelSSH::Core::Models