#include "TestRegistry.h"

#include <stdexcept>

#include "Models/SshConnectionProfile.h"
#include "Models/TunnelGroup.h"
#include "Models/TunnelPort.h"
#include "Services/SshCommandBuilder.h"

namespace SimpleTunnelSSH::Core::Tests
{

namespace
{

void expect(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

} // namespace

int runSshCommandBuilderTests()
{
    using namespace SimpleTunnelSSH::Core::Models;
    using namespace SimpleTunnelSSH::Core::Services;

    {
        SshConnectionProfile connection;
        connection.name = QStringLiteral("Development");
        connection.host = QStringLiteral("dev.dhenyson.com");
        connection.port = 22;
        connection.userName = QStringLiteral("root");

        TunnelGroup group;
        group.name = QStringLiteral("Web");
        group.ports = {
            TunnelPort { QUuid::createUuid(), {}, true, 3000, QStringLiteral("localhost"), 3000 },
            TunnelPort { QUuid::createUuid(), {}, true, 5432, QStringLiteral("localhost"), 5432 }
        };

        const auto arguments = SshCommandBuilder::buildArguments(connection, { group });
        expect(arguments.contains(QStringLiteral("-N")), "Expected -N argument.");
        expect(arguments.contains(QStringLiteral("-T")), "Expected -T argument.");
        expect(arguments.contains(QStringLiteral("3000:localhost:3000")), "Expected local port 3000 forward.");
        expect(arguments.contains(QStringLiteral("5432:localhost:5432")), "Expected local port 5432 forward.");
        expect(arguments.contains(QStringLiteral("ServerAliveInterval=60")), "Expected ServerAliveInterval option.");
        expect(arguments.contains(QStringLiteral("ServerAliveCountMax=3")), "Expected ServerAliveCountMax option.");
        expect(arguments.last() == QStringLiteral("root@dev.dhenyson.com"), "Expected user@endpoint at the end of the argument list.");
    }

    {
        SshConnectionProfile connection;
        connection.host = QStringLiteral("dev.dhenyson.com");
        connection.userName = QStringLiteral("root");

        TunnelGroup group;
        group.name = QStringLiteral("Web");
        group.ports = {
            TunnelPort { QUuid::createUuid(), {}, true, 3000, QStringLiteral("localhost"), 3000 },
            TunnelPort { QUuid::createUuid(), {}, false, 4017, QStringLiteral("localhost"), 4017 }
        };

        const auto arguments = SshCommandBuilder::buildArguments(connection, { group });
        expect(arguments.contains(QStringLiteral("3000:localhost:3000")), "Expected enabled port to be included.");
        expect(!arguments.contains(QStringLiteral("4017:localhost:4017")), "Expected disabled port to be ignored.");
    }

    return 0;
}

} // namespace SimpleTunnelSSH::Core::Tests