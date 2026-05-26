using System.Linq;

namespace SimpleTunnelSSH.Core.Models;

public sealed class TunnelGroup
{
    public Guid Id { get; set; } = Guid.NewGuid();

    public string Name { get; set; } = string.Empty;

    public bool IsEnabled { get; set; } = true;

    public List<TunnelPort> Ports { get; set; } = [];

    public TunnelGroup DeepClone()
    {
        return new TunnelGroup
        {
            Id = Id,
            Name = Name,
            IsEnabled = IsEnabled,
            Ports = (Ports ?? []).Select(static port => port.DeepClone()).ToList()
        };
    }
}