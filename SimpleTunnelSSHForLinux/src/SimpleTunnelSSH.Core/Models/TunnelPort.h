#pragma once

#include <QString>
#include <QUuid>

namespace SimpleTunnelSSH::Core::Models
{

class TunnelPort
{
public:
    QUuid id { QUuid::createUuid() };
    QString description;
    bool isEnabled { true };
    int localPort { 0 };
    QString remoteHost { QStringLiteral("localhost") };
    int remotePort { 0 };

    [[nodiscard]] TunnelPort deepClone() const
    {
        return *this;
    }
};

} // namespace SimpleTunnelSSH::Core::Models