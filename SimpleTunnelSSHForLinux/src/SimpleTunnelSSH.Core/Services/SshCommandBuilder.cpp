#include "Services/SshCommandBuilder.h"

#include <algorithm>

namespace SimpleTunnelSSH::Core::Services
{

namespace
{

struct FlattenedPort
{
    int localPort { 0 };
    QString remoteHost;
    int remotePort { 0 };
};

} // namespace

QStringList SshCommandBuilder::buildArguments(
    const Models::SshConnectionProfile& connection,
    const QList<Models::TunnelGroup>& groups)
{
    QStringList arguments;
    arguments
        << QStringLiteral("-N")
        << QStringLiteral("-T")
        << QStringLiteral("-p")
        << QString::number(connection.port);

    QList<FlattenedPort> flattenedPorts;

    for (const auto& group : groups)
    {
        if (!group.isEnabled)
        {
            continue;
        }

        for (const auto& port : group.ports)
        {
            if (!port.isEnabled)
            {
                continue;
            }

            flattenedPorts.append(FlattenedPort {
                port.localPort,
                port.remoteHost,
                port.remotePort
            });
        }
    }

    std::sort(flattenedPorts.begin(), flattenedPorts.end(), [](const auto& left, const auto& right)
    {
        if (left.localPort != right.localPort)
        {
            return left.localPort < right.localPort;
        }

        const auto hostComparison = QString::compare(left.remoteHost, right.remoteHost, Qt::CaseInsensitive);

        if (hostComparison != 0)
        {
            return hostComparison < 0;
        }

        return left.remotePort < right.remotePort;
    });

    for (const auto& port : flattenedPorts)
    {
        arguments
            << QStringLiteral("-L")
            << QStringLiteral("%1:%2:%3")
                   .arg(QString::number(port.localPort), port.remoteHost, QString::number(port.remotePort));
    }

    arguments
        << QStringLiteral("-o") << QStringLiteral("ExitOnForwardFailure=yes")
        << QStringLiteral("-o") << QStringLiteral("ServerAliveInterval=60")
        << QStringLiteral("-o") << QStringLiteral("ServerAliveCountMax=3")
        << QStringLiteral("-o") << QStringLiteral("TCPKeepAlive=yes")
        << QStringLiteral("-o") << QStringLiteral("ConnectTimeout=10")
        << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=accept-new")
        << QStringLiteral("-o") << QStringLiteral("LogLevel=DEBUG1");

    if (!connection.identityFilePath.trimmed().isEmpty())
    {
        arguments
            << QStringLiteral("-i")
            << connection.identityFilePath.trimmed();
    }

    arguments << QStringLiteral("%1@%2").arg(connection.userName.trimmed(), connection.host.trimmed());
    return arguments;
}

} // namespace SimpleTunnelSSH::Core::Services