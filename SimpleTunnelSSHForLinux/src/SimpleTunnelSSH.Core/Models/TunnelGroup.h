#pragma once

#include <QList>
#include <QString>
#include <QUuid>

#include "Models/TunnelPort.h"

namespace SimpleTunnelSSH::Core::Models
{

class TunnelGroup
{
public:
    QUuid id { QUuid::createUuid() };
    QString name;
    bool isEnabled { true };
    QList<TunnelPort> ports;

    [[nodiscard]] TunnelGroup deepClone() const
    {
        return *this;
    }
};

} // namespace SimpleTunnelSSH::Core::Models