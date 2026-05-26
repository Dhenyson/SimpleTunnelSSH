#pragma once

#include <QList>
#include <QString>
#include <QUuid>

#include "Models/TunnelGroup.h"

namespace SimpleTunnelSSH::Core::Models
{

class SshConnectionProfile
{
public:
    QUuid id { QUuid::createUuid() };
    QString name;
    QString host;
    int port { 22 };
    QString userName;
    QString identityFilePath;
    QList<TunnelGroup> groups;

    [[nodiscard]] SshConnectionProfile deepClone() const
    {
        return *this;
    }

    [[nodiscard]] QString getEndpointLabel() const
    {
        return userName.trimmed().isEmpty()
            ? QStringLiteral("%1:%2").arg(host, QString::number(port))
            : QStringLiteral("%1@%2:%3").arg(userName, host, QString::number(port));
    }
};

} // namespace SimpleTunnelSSH::Core::Models