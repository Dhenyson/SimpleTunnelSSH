using System.Linq;

namespace SimpleTunnelSSH.Core.Models;

public sealed class AppConfiguration
{
    public int SchemaVersion { get; set; } = 2;

    public Guid? LastSelectedConnectionId { get; set; }

    public AppSettings Settings { get; set; } = new();

    public List<SshConnectionProfile> Connections { get; set; } = [];

    public AppConfiguration DeepClone()
    {
        return new AppConfiguration
        {
            SchemaVersion = SchemaVersion,
            LastSelectedConnectionId = LastSelectedConnectionId,
            Settings = (Settings ?? new AppSettings()).DeepClone(),
            Connections = (Connections ?? []).Select(static connection => connection.DeepClone()).ToList()
        };
    }
}