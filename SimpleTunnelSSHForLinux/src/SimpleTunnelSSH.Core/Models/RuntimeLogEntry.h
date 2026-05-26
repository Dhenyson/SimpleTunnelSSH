#pragma once

#include <QDateTime>
#include <QString>

namespace SimpleTunnelSSH::Core::Models
{

class RuntimeLogEntry
{
public:
    QDateTime timestampUtc;
    QString level;
    QString message;
};

} // namespace SimpleTunnelSSH::Core::Models