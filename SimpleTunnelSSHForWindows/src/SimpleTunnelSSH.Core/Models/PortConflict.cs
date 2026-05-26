namespace SimpleTunnelSSH.Core.Models;

public sealed record PortConflict(
    int LocalPort,
    Guid FirstGroupId,
    string FirstGroupName,
    Guid SecondGroupId,
    string SecondGroupName);