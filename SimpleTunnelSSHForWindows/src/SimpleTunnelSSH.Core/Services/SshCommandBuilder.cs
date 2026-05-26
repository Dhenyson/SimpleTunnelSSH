using System.Diagnostics;
using System.Globalization;
using System.Linq;
using SimpleTunnelSSH.Core.Models;

namespace SimpleTunnelSSH.Core.Services;

public static class SshCommandBuilder
{
    public static IReadOnlyList<string> BuildArguments(
        SshConnectionProfile connection,
        IEnumerable<TunnelGroup> groups)
    {
        var arguments = new List<string>
        {
            "-N",
            "-T",
            "-p",
            connection.Port.ToString(CultureInfo.InvariantCulture)
        };

        foreach (var port in groups
            .Where(static group => group.IsEnabled)
            .SelectMany(static group => group.Ports.Where(static port => port.IsEnabled))
            .OrderBy(static port => port.LocalPort)
            .ThenBy(static port => port.RemoteHost, StringComparer.OrdinalIgnoreCase)
            .ThenBy(static port => port.RemotePort))
        {
            arguments.Add("-L");
            arguments.Add($"{port.LocalPort}:{port.RemoteHost}:{port.RemotePort}");
        }

        arguments.AddRange(
        [
            "-o", "ExitOnForwardFailure=yes",
            "-o", "ServerAliveInterval=60",
            "-o", "ServerAliveCountMax=3",
            "-o", "TCPKeepAlive=yes",
            "-o", "ConnectTimeout=10",
            "-o", "StrictHostKeyChecking=accept-new",
            "-o", "LogLevel=DEBUG1"
        ]);

        if (!string.IsNullOrWhiteSpace(connection.IdentityFilePath))
        {
            arguments.Add("-i");
            arguments.Add(connection.IdentityFilePath);
        }

        arguments.Add($"{connection.UserName}@{connection.Host}");
        return arguments;
    }

    public static ProcessStartInfo BuildStartInfo(
        string sshExecutablePath,
        SshConnectionProfile connection,
        IEnumerable<TunnelGroup> groups)
    {
        var startInfo = new ProcessStartInfo
        {
            FileName = sshExecutablePath,
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };

        foreach (var argument in BuildArguments(connection, groups))
        {
            startInfo.ArgumentList.Add(argument);
        }

        return startInfo;
    }
}