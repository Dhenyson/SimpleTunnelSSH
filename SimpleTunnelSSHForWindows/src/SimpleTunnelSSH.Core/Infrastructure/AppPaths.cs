namespace SimpleTunnelSSH.Core.Infrastructure;

public static class AppPaths
{
    public static string GetAppDirectory()
    {
        return Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "SimpleTunnelSSH");
    }

    public static string GetConfigurationFilePath()
    {
        return Path.Combine(GetAppDirectory(), "config.json");
    }
}