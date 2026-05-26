using SimpleTunnelSSH.Core.Models;

namespace SimpleTunnelSSH.Core.Services;

public static class PortConflictDetector
{
    public static IReadOnlyList<PortConflict> Detect(IEnumerable<TunnelGroup> groups)
    {
        var conflicts = new List<PortConflict>();
        var seenPorts = new Dictionary<int, (Guid GroupId, string GroupName)>();

        foreach (var group in groups.Where(static currentGroup => currentGroup.IsEnabled))
        {
            foreach (var port in group.Ports.Where(static currentPort => currentPort.IsEnabled))
            {
                if (seenPorts.TryGetValue(port.LocalPort, out var previous))
                {
                    if (previous.GroupId != group.Id)
                    {
                        conflicts.Add(new PortConflict(
                            port.LocalPort,
                            previous.GroupId,
                            previous.GroupName,
                            group.Id,
                            group.Name));
                    }

                    continue;
                }

                seenPorts[port.LocalPort] = (group.Id, group.Name);
            }
        }

        return conflicts;
    }
}