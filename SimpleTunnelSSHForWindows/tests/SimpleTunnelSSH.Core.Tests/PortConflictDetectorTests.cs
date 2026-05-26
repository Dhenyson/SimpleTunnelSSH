using SimpleTunnelSSH.Core.Models;
using SimpleTunnelSSH.Core.Services;

namespace SimpleTunnelSSH.Core.Tests;

public sealed class PortConflictDetectorTests
{
    [Fact]
    public void Detect_ReturnsConflict_WhenTwoGroupsUseTheSameLocalPort()
    {
        var development = new TunnelGroup
        {
            Name = "Development",
            Ports = [new TunnelPort { LocalPort = 3000, RemoteHost = "localhost", RemotePort = 3000 }]
        };

        var production = new TunnelGroup
        {
            Name = "Production",
            Ports = [new TunnelPort { LocalPort = 3000, RemoteHost = "localhost", RemotePort = 3000 }]
        };

        var conflicts = PortConflictDetector.Detect([development, production]);

        Assert.Single(conflicts);
        Assert.Equal(3000, conflicts[0].LocalPort);
    }

    [Fact]
    public void Detect_ReturnsEmpty_WhenPortsAreUnique()
    {
        var development = new TunnelGroup
        {
            Name = "Development",
            Ports = [new TunnelPort { LocalPort = 3000, RemoteHost = "localhost", RemotePort = 3000 }]
        };

        var database = new TunnelGroup
        {
            Name = "Database",
            Ports = [new TunnelPort { LocalPort = 5432, RemoteHost = "localhost", RemotePort = 5432 }]
        };

        var conflicts = PortConflictDetector.Detect([development, database]);

        Assert.Empty(conflicts);
    }

    [Fact]
    public void Detect_IgnoresDisabledPorts()
    {
        var development = new TunnelGroup
        {
            Name = "Development",
            IsEnabled = true,
            Ports = [new TunnelPort { LocalPort = 3000, RemoteHost = "localhost", RemotePort = 3000, IsEnabled = false }]
        };

        var production = new TunnelGroup
        {
            Name = "Production",
            IsEnabled = true,
            Ports = [new TunnelPort { LocalPort = 3000, RemoteHost = "localhost", RemotePort = 3000, IsEnabled = true }]
        };

        var conflicts = PortConflictDetector.Detect([development, production]);

        Assert.Empty(conflicts);
    }
}