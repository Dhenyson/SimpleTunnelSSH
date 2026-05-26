namespace SimpleTunnelSSH.Core.Models;

public sealed class TunnelPort
{
    public Guid Id { get; set; } = Guid.NewGuid();

    public string Description { get; set; } = string.Empty;

    public bool IsEnabled { get; set; } = true;

    public int LocalPort { get; set; }

    public string RemoteHost { get; set; } = "localhost";

    public int RemotePort { get; set; }

    public TunnelPort DeepClone()
    {
        return new TunnelPort
        {
            Id = Id,
            Description = Description,
            IsEnabled = IsEnabled,
            LocalPort = LocalPort,
            RemoteHost = RemoteHost,
            RemotePort = RemotePort
        };
    }
}