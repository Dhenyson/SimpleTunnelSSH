namespace SimpleTunnelSSH.Core.Models;

public sealed class AppSettings
{
    public const int DefaultWindowWidth = 1280;
    public const int DefaultWindowHeight = 720;

    public bool LaunchAtWindowsStartup { get; set; }

    public bool StartMinimizedToTray { get; set; }

    public int WindowWidth { get; set; } = DefaultWindowWidth;

    public int WindowHeight { get; set; } = DefaultWindowHeight;

    public List<int> GroupColumnWidths { get; set; } = CreateDefaultGroupColumnWidths();

    public List<int> PortColumnWidths { get; set; } = CreateDefaultPortColumnWidths();

    public List<int> LogColumnWidths { get; set; } = CreateDefaultLogColumnWidths();

    public AppSettings DeepClone()
    {
        return new AppSettings
        {
            LaunchAtWindowsStartup = LaunchAtWindowsStartup,
            StartMinimizedToTray = StartMinimizedToTray,
            WindowWidth = WindowWidth,
            WindowHeight = WindowHeight,
            GroupColumnWidths = [.. GroupColumnWidths],
            PortColumnWidths = [.. PortColumnWidths],
            LogColumnWidths = [.. LogColumnWidths]
        };
    }

    public static List<int> CreateDefaultGroupColumnWidths()
    {
        return [220, 72, 260];
    }

    public static List<int> CreateDefaultPortColumnWidths()
    {
        return [86, 180, 92, 260];
    }

    public static List<int> CreateDefaultLogColumnWidths()
    {
        return [170, 90, 760];
    }
}
