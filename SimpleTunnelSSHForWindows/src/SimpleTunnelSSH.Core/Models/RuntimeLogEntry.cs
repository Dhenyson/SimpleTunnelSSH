namespace SimpleTunnelSSH.Core.Models;

public sealed class RuntimeLogEntry
{
    public DateTimeOffset TimestampUtc { get; init; }

    public string Level { get; init; } = "Info";

    public string Message { get; init; } = string.Empty;
}