#include "Services/PortConflictDetector.h"

namespace SimpleTunnelSSH::Core::Services
{

QList<Models::PortConflict> PortConflictDetector::detect(const QList<Models::TunnelGroup>& groups)
{
    QList<Models::PortConflict> conflicts;

    for (int firstIndex = 0; firstIndex < groups.size(); ++firstIndex)
    {
        const auto& firstGroup = groups[firstIndex];

        if (!firstGroup.isEnabled)
        {
            continue;
        }

        for (int secondIndex = firstIndex + 1; secondIndex < groups.size(); ++secondIndex)
        {
            const auto& secondGroup = groups[secondIndex];

            if (!secondGroup.isEnabled)
            {
                continue;
            }

            for (const auto& firstPort : firstGroup.ports)
            {
                if (!firstPort.isEnabled)
                {
                    continue;
                }

                for (const auto& secondPort : secondGroup.ports)
                {
                    if (!secondPort.isEnabled || firstPort.localPort != secondPort.localPort)
                    {
                        continue;
                    }

                    Models::PortConflict conflict;
                    conflict.firstGroupId = firstGroup.id;
                    conflict.firstGroupName = firstGroup.name;
                    conflict.secondGroupId = secondGroup.id;
                    conflict.secondGroupName = secondGroup.name;
                    conflict.localPort = firstPort.localPort;
                    conflicts.append(conflict);
                }
            }
        }
    }

    return conflicts;
}

} // namespace SimpleTunnelSSH::Core::Services