namespace SimpleTunnelSSH.App;

static class Program
{
    [STAThread]
    static int Main(string[] args)
    {
        if (AskPassPromptHost.IsAskPassMode())
        {
            return AskPassPromptHost.Run(args);
        }

        using var singleInstanceMutex = new Mutex(true, "SimpleTunnelSSH.Singleton", out var createdNew);

        if (!createdNew)
        {
            return 0;
        }

        ApplicationConfiguration.Initialize();
        Application.Run(new TunnelApplicationContext());
        return 0;
    }
}