using System.Linq;

namespace SimpleTunnelSSH.Core.Models;

public sealed class SshConnectionProfile
{
    public Guid Id { get; set; } = Guid.NewGuid();

    public string Name { get; set; } = string.Empty;

    public string Host { get; set; } = string.Empty;

    public int Port { get; set; } = 22;

    public string UserName { get; set; } = string.Empty;

    public string IdentityFilePath { get; set; } = string.Empty;

    public List<TunnelGroup> Groups { get; set; } = [];

    public SshConnectionProfile DeepClone()
    {
        return new SshConnectionProfile
        {
            Id = Id,
            Name = Name,
            Host = Host,
            Port = Port,
            UserName = UserName,
            IdentityFilePath = IdentityFilePath,
            Groups = (Groups ?? []).Select(static group => group.DeepClone()).ToList()
        };
    }

    public string GetEndpointLabel()
    {
        return string.IsNullOrWhiteSpace(UserName)
            ? $"{Host}:{Port}"
            : $"{UserName}@{Host}:{Port}";
    }
}