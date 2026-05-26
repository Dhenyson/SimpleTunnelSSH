namespace SimpleTunnelSSH.Core.Models;

public sealed class TunnelRuntimeSnapshot
{
    public static TunnelRuntimeSnapshot Idle { get; } = new()
    {
        StatusText = "Idle",
        ActiveGroupIds = Array.Empty<Guid>()
    };

    public bool IsActive { get; init; }

    public bool IsConnecting { get; init; }

    public bool IsAwaitingAuthentication { get; init; }

    public bool IsFaulted { get; init; }

    public Guid? ConnectionId { get; init; }

    public Guid? FaultedConnectionId { get; init; }

    public IReadOnlyCollection<Guid> ActiveGroupIds { get; init; } = Array.Empty<Guid>();

    public string StatusText { get; init; } = "Idle";

    public DateTimeOffset? ConnectedAtUtc { get; init; }

    public string? LastError { get; init; }
}