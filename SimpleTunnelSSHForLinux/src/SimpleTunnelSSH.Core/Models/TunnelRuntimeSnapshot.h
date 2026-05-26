#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <QUuid>

namespace SimpleTunnelSSH::Core::Models
{

class TunnelRuntimeSnapshot
{
public:
    bool isActive { false };
    bool isConnecting { false };
    bool isAwaitingAuthentication { false };
    bool isFaulted { false };
    QUuid connectionId;
    QUuid faultedConnectionId;
    QList<QUuid> activeGroupIds;
    QString statusText { QStringLiteral("Idle") };
    QDateTime connectedAtUtc;
    QString lastError;

    [[nodiscard]] static TunnelRuntimeSnapshot idle()
    {
        TunnelRuntimeSnapshot snapshot;
        snapshot.statusText = QStringLiteral("Idle");
        return snapshot;
    }
};

} // namespace SimpleTunnelSSH::Core::Models