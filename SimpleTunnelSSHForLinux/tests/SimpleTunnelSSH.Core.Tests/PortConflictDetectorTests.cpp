#include "TestRegistry.h"

#include <stdexcept>

#include "Models/TunnelGroup.h"
#include "Models/TunnelPort.h"
#include "Services/PortConflictDetector.h"

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

int runPortConflictDetectorTests()
{
    using namespace SimpleTunnelSSH::Core::Models;
    using namespace SimpleTunnelSSH::Core::Services;

    {
        TunnelGroup development;
        development.name = QStringLiteral("Development");
        development.ports = { TunnelPort { QUuid::createUuid(), {}, true, 3000, QStringLiteral("localhost"), 3000 } };

        TunnelGroup production;
        production.name = QStringLiteral("Production");
        production.ports = { TunnelPort { QUuid::createUuid(), {}, true, 3000, QStringLiteral("localhost"), 3000 } };

        const auto conflicts = PortConflictDetector::detect({ development, production });
        expect(conflicts.size() == 1, "Expected exactly one port conflict.");
        expect(conflicts.first().localPort == 3000, "Expected conflicting local port 3000.");
    }

    {
        TunnelGroup development;
        development.name = QStringLiteral("Development");
        development.ports = { TunnelPort { QUuid::createUuid(), {}, true, 3000, QStringLiteral("localhost"), 3000 } };

        TunnelGroup database;
        database.name = QStringLiteral("Database");
        database.ports = { TunnelPort { QUuid::createUuid(), {}, true, 5432, QStringLiteral("localhost"), 5432 } };

        const auto conflicts = PortConflictDetector::detect({ development, database });
        expect(conflicts.isEmpty(), "Expected no conflicts when local ports are unique.");
    }

    {
        TunnelGroup development;
        development.name = QStringLiteral("Development");
        development.ports = { TunnelPort { QUuid::createUuid(), {}, false, 3000, QStringLiteral("localhost"), 3000 } };

        TunnelGroup production;
        production.name = QStringLiteral("Production");
        production.ports = { TunnelPort { QUuid::createUuid(), {}, true, 3000, QStringLiteral("localhost"), 3000 } };

        const auto conflicts = PortConflictDetector::detect({ development, production });
        expect(conflicts.isEmpty(), "Expected disabled ports to be ignored.");
    }

    return 0;
}

} // namespace SimpleTunnelSSH::Core::Tests