using SimpleTunnelSSH.Core.Models;
using SimpleTunnelSSH.Core.Services;

namespace SimpleTunnelSSH.Core.Tests;

public sealed class SshCommandBuilderTests
{
    [Fact]
    public void BuildArguments_IncludesExpectedPortForwardsAndKeepAliveOptions()
    {
        var connection = new SshConnectionProfile
        {
            Name = "Development",
            Host = "dev.dhenyson.com",
            Port = 22,
            UserName = "root"
        };
        var group = new TunnelGroup
        {
            Name = "Web",
            Ports =
            [
                new TunnelPort { LocalPort = 3000, RemoteHost = "localhost", RemotePort = 3000 },
                new TunnelPort { LocalPort = 5432, RemoteHost = "localhost", RemotePort = 5432 }
            ]
        };

        var arguments = SshCommandBuilder.BuildArguments(connection, [group]);

        Assert.Contains("-N", arguments);
        Assert.Contains("-T", arguments);
        Assert.Contains("3000:localhost:3000", arguments);
        Assert.Contains("5432:localhost:5432", arguments);
        Assert.Contains("ServerAliveInterval=60", arguments);
        Assert.Contains("ServerAliveCountMax=3", arguments);
        Assert.Equal("root@dev.dhenyson.com", arguments[^1]);
    }

    [Fact]
    public void BuildArguments_IgnoresDisabledPorts()
    {
        var connection = new SshConnectionProfile
        {
            Name = "Development",
            Host = "dev.dhenyson.com",
            Port = 22,
            UserName = "root"
        };
        var group = new TunnelGroup
        {
            Name = "Web",
            IsEnabled = true,
            Ports =
            [
                new TunnelPort { LocalPort = 3000, RemoteHost = "localhost", RemotePort = 3000, IsEnabled = true },
                new TunnelPort { LocalPort = 4017, RemoteHost = "localhost", RemotePort = 4017, IsEnabled = false }
            ]
        };

        var arguments = SshCommandBuilder.BuildArguments(connection, [group]);

        Assert.Contains("3000:localhost:3000", arguments);
        Assert.DoesNotContain("4017:localhost:4017", arguments);
    }
}