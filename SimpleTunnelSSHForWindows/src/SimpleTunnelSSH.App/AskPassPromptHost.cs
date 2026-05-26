namespace SimpleTunnelSSH.App;

internal static class AskPassPromptHost
{
    private const string AskPassModeEnvironmentVariable = "SIMPLE_TUNNEL_ASKPASS_MODE";
    private const string AskPassMutexEnvironmentVariable = "SIMPLE_TUNNEL_ASKPASS_MUTEX";

    public static bool IsAskPassMode()
    {
        return string.Equals(
            Environment.GetEnvironmentVariable(AskPassModeEnvironmentVariable),
            "1",
            StringComparison.Ordinal);
    }

    public static int Run(string[] args)
    {
        ApplicationConfiguration.Initialize();

        var promptMutex = OpenPromptMutex();

        try
        {
            WaitForPromptMutex(promptMutex);

            var prompt = args.Length == 0
                ? "SSH authentication is required."
                : string.Join(" ", args);
            var secret = AskPassPromptForm.ShowPrompt(prompt);

            if (secret is null)
            {
                return 1;
            }

            Console.Out.WriteLine(secret);
            Console.Out.Flush();
            return 0;
        }
        finally
        {
            ReleasePromptMutex(promptMutex);
            promptMutex?.Dispose();
        }
    }

    private static Mutex? OpenPromptMutex()
    {
        var mutexName = Environment.GetEnvironmentVariable(AskPassMutexEnvironmentVariable);

        if (string.IsNullOrWhiteSpace(mutexName))
        {
            return null;
        }

        try
        {
            return Mutex.OpenExisting(mutexName);
        }
        catch (WaitHandleCannotBeOpenedException)
        {
            return null;
        }
    }

    private static void WaitForPromptMutex(Mutex? promptMutex)
    {
        if (promptMutex is null)
        {
            return;
        }

        try
        {
            promptMutex.WaitOne();
        }
        catch (AbandonedMutexException)
        {
        }
    }

    private static void ReleasePromptMutex(Mutex? promptMutex)
    {
        if (promptMutex is null)
        {
            return;
        }

        try
        {
            promptMutex.ReleaseMutex();
        }
        catch (ApplicationException)
        {
        }
    }
}