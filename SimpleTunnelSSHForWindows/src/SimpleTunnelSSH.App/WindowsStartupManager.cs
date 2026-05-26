using Microsoft.Win32;

namespace SimpleTunnelSSH.App;

internal static class WindowsStartupManager
{
    private const string RunKeyPath = @"Software\Microsoft\Windows\CurrentVersion\Run";
    private const string ValueName = "SimpleTunnelSSH";

    public static void SetEnabled(bool enabled)
    {
        using var runKey = Registry.CurrentUser.CreateSubKey(RunKeyPath, writable: true)
            ?? throw new InvalidOperationException("Unable to access the Windows startup registry key.");

        if (enabled)
        {
            runKey.SetValue(ValueName, BuildLaunchCommand(), RegistryValueKind.String);
            return;
        }

        runKey.DeleteValue(ValueName, false);
    }

    public static bool IsEnabled()
    {
        using var runKey = Registry.CurrentUser.OpenSubKey(RunKeyPath, writable: false);
        var value = runKey?.GetValue(ValueName) as string;
        return !string.IsNullOrWhiteSpace(value);
    }

    private static string BuildLaunchCommand()
    {
        return $"\"{Application.ExecutablePath}\"";
    }
}
