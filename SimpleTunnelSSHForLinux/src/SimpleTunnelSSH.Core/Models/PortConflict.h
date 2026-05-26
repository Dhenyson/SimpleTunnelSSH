#pragma once

#include <QString>
#include <QUuid>

namespace SimpleTunnelSSH::Core::Models
{

class PortConflict
{
public:
    QUuid firstGroupId;
    QString firstGroupName;
    QUuid secondGroupId;
    QString secondGroupName;
    int localPort { 0 };
};

} // namespace SimpleTunnelSSH::Core::Models