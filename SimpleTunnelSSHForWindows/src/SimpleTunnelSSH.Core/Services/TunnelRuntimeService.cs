using System.Diagnostics;
using System.Linq;
using System.Text;
using SimpleTunnelSSH.Core.Models;

namespace SimpleTunnelSSH.Core.Services;

public sealed class TunnelRuntimeService : IDisposable
{
    private const int MaxRuntimeLogEntries = 300;
    private const int MaxReconnectAttempts = 5;
    private static readonly TimeSpan AuthPromptPollInterval = TimeSpan.FromMilliseconds(150);

    private readonly object _syncRoot = new();
    private readonly System.Threading.Timer _reconnectTimer;
    private readonly System.Threading.Timer _authPromptMonitorTimer;
    private readonly string _sshExecutablePath;

    private Process? _process;
    private SshConnectionProfile? _activeConnection;
    private List<TunnelGroup> _activeGroups = [];
    private readonly List<string> _recentMessages = [];
    private readonly List<RuntimeLogEntry> _logEntries = [];
    private Mutex? _askPassMutex;
    private bool _stopRequested = true;
    private bool _disposed;
    private bool _isConnectionConfirmed;
    private bool _isAwaitingAuthentication;
    private bool _authenticationPromptShownForCurrentProcess;
    private Guid? _faultedConnectionId;
    private int _reconnectAttempt;
    private string? _lastError;
    private DateTimeOffset? _lastAuthenticationPromptClosedAtUtc;
    private DateTimeOffset? _connectedAtUtc;
    private DateTimeOffset? _reconnectAtUtc;

    public TunnelRuntimeService()
        : this(ResolveSshExecutablePath())
    {
    }

    internal TunnelRuntimeService(string sshExecutablePath)
    {
        _sshExecutablePath = sshExecutablePath;
        _reconnectTimer = new System.Threading.Timer(static state =>
        {
            var service = (TunnelRuntimeService?)state;
            service?.StartProcess();
        }, this, Timeout.InfiniteTimeSpan, Timeout.InfiniteTimeSpan);
        _authPromptMonitorTimer = new System.Threading.Timer(static state =>
        {
            var service = (TunnelRuntimeService?)state;
            service?.PollAuthenticationPromptState();
        }, this, Timeout.InfiniteTimeSpan, Timeout.InfiniteTimeSpan);
    }

    public event EventHandler<TunnelRuntimeSnapshot>? StateChanged;

    public event EventHandler? LogsChanged;

    public TunnelRuntimeSnapshot Snapshot
    {
        get
        {
            lock (_syncRoot)
            {
                return CreateSnapshotLocked();
            }
        }
    }

    public IReadOnlyList<RuntimeLogEntry> GetRecentLogs()
    {
        lock (_syncRoot)
        {
            return _logEntries.ToArray();
        }
    }

    public string GetRecentLogsText()
    {
        lock (_syncRoot)
        {
            if (_logEntries.Count == 0)
            {
                return string.Empty;
            }

            var builder = new StringBuilder();

            foreach (var entry in _logEntries)
            {
                if (builder.Length > 0)
                {
                    builder.AppendLine();
                }

                builder.Append(entry.TimestampUtc.ToLocalTime().ToString("yyyy-MM-dd HH:mm:ss"));
                builder.Append('\t');
                builder.Append(entry.Level);
                builder.Append('\t');
                builder.Append(entry.Message);
            }

            return builder.ToString();
        }
    }

    public void ClearLogs()
    {
        lock (_syncRoot)
        {
            _recentMessages.Clear();
            _logEntries.Clear();
        }

        RaiseLogsChanged();
    }

    public Task ActivateAsync(
        SshConnectionProfile connection,
        IEnumerable<TunnelGroup> groups,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        var connectionCopy = connection.DeepClone();
        var groupsCopy = groups.Select(static group => group.DeepClone()).ToList();

        if (groupsCopy.Count == 0)
        {
            throw new InvalidOperationException("At least one tunnel group must be selected.");
        }

        if (!groupsCopy.Any(static group => group.IsEnabled && group.Ports.Any(static port => port.IsEnabled)))
        {
            throw new InvalidOperationException("At least one active tunnel port must be selected.");
        }

        Process? processToStop;
        Mutex? askPassMutexToDispose;

        lock (_syncRoot)
        {
            ThrowIfDisposed();
            _reconnectTimer.Change(Timeout.InfiniteTimeSpan, Timeout.InfiniteTimeSpan);
            _stopRequested = false;
            _activeConnection = connectionCopy;
            _activeGroups = groupsCopy;
            _connectedAtUtc = null;
            _isConnectionConfirmed = false;
            _reconnectAttempt = 0;
            _reconnectAtUtc = null;
            _authenticationPromptShownForCurrentProcess = false;
            _faultedConnectionId = null;
            _lastAuthenticationPromptClosedAtUtc = null;
            _recentMessages.Clear();
            _lastError = null;
            processToStop = DetachProcessLocked();
            askPassMutexToDispose = StopAskPassMonitoringLocked();
        }

        StopProcess(processToStop);
        DisposeMutex(askPassMutexToDispose);
        AddLog("Info", $"Activating SSH tunnel for {connectionCopy.GetEndpointLabel()} with {groupsCopy.Count} group(s).");
        RaiseStateChanged();
        StartProcess();
        return Task.CompletedTask;
    }

    public Task DeactivateAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        Process? processToStop;
        Mutex? askPassMutexToDispose;
        string? endpointLabel;

        lock (_syncRoot)
        {
            if (_disposed)
            {
                return Task.CompletedTask;
            }

            endpointLabel = _activeConnection?.GetEndpointLabel();
            _stopRequested = true;
            _reconnectTimer.Change(Timeout.InfiniteTimeSpan, Timeout.InfiniteTimeSpan);
            _activeConnection = null;
            _activeGroups = [];
            _connectedAtUtc = null;
            _isConnectionConfirmed = false;
            _reconnectAttempt = 0;
            _reconnectAtUtc = null;
            _authenticationPromptShownForCurrentProcess = false;
            _faultedConnectionId = null;
            _lastAuthenticationPromptClosedAtUtc = null;
            _recentMessages.Clear();
            _lastError = null;
            processToStop = DetachProcessLocked();
            askPassMutexToDispose = StopAskPassMonitoringLocked();
        }

        StopProcess(processToStop);
        DisposeMutex(askPassMutexToDispose);

        if (!string.IsNullOrWhiteSpace(endpointLabel))
        {
            AddLog("Info", $"Stopped SSH tunnel for {endpointLabel}.");
        }

        RaiseStateChanged();
        return Task.CompletedTask;
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        Process? processToStop;
        Mutex? askPassMutexToDispose;

        lock (_syncRoot)
        {
            _disposed = true;
            _stopRequested = true;
            _reconnectTimer.Change(Timeout.InfiniteTimeSpan, Timeout.InfiniteTimeSpan);
            processToStop = DetachProcessLocked();
            askPassMutexToDispose = StopAskPassMonitoringLocked();
        }

        StopProcess(processToStop);
        DisposeMutex(askPassMutexToDispose);
        _reconnectTimer.Dispose();
        _authPromptMonitorTimer.Dispose();
    }

    private void StartProcess()
    {
        SshConnectionProfile? connection;
        List<TunnelGroup>? groups;

        lock (_syncRoot)
        {
            if (_disposed || _stopRequested || _activeConnection is null || _activeGroups.Count == 0 || _process is not null)
            {
                return;
            }

            connection = _activeConnection.DeepClone();
            groups = _activeGroups.Select(static group => group.DeepClone()).ToList();
            _reconnectAtUtc = null;
            _isConnectionConfirmed = false;
            _authenticationPromptShownForCurrentProcess = false;
            _lastAuthenticationPromptClosedAtUtc = null;
        }

        var startInfo = SshCommandBuilder.BuildStartInfo(_sshExecutablePath, connection, groups);
        ConfigureAskPass(startInfo);

        var process = new Process
        {
            StartInfo = startInfo
        };

        process.EnableRaisingEvents = true;
        process.Exited += OnProcessExited;
        process.OutputDataReceived += OnOutputDataReceived;
        process.ErrorDataReceived += OnErrorDataReceived;

        try
        {
            AddLog("Info", $"Starting ssh.exe for {connection.GetEndpointLabel()}.");

            if (!process.Start())
            {
                throw new InvalidOperationException("The OpenSSH client did not start.");
            }

            process.BeginOutputReadLine();
            process.BeginErrorReadLine();

            lock (_syncRoot)
            {
                if (_disposed || _stopRequested || _activeConnection is null)
                {
                    CleanupProcessHandlers(process);
                    StopProcess(process);
                    return;
                }

                _process = process;
                _connectedAtUtc = null;
                _isConnectionConfirmed = false;
                _lastError = null;
                _authPromptMonitorTimer.Change(AuthPromptPollInterval, AuthPromptPollInterval);
            }

            AddLog("Info", $"ssh.exe started with PID {process.Id}.");
            RaiseStateChanged();
        }
        catch (Exception exception)
        {
            Mutex? askPassMutexToDispose;

            CleanupProcessHandlers(process);
            process.Dispose();

            lock (_syncRoot)
            {
                _connectedAtUtc = null;
                _lastError = exception.Message;
                askPassMutexToDispose = StopAskPassMonitoringLocked();
            }

            DisposeMutex(askPassMutexToDispose);
            AddLog("Error", exception.Message);
            ScheduleReconnect(exception.Message);
        }
    }

    private void OnProcessExited(object? sender, EventArgs eventArgs)
    {
        var process = (Process?)sender;
        string exitMessage;
        bool shouldReconnect;
        bool shouldStopAfterPromptFailure;
        bool wasStopRequested;
        Mutex? askPassMutexToDispose;

        lock (_syncRoot)
        {
            if (ReferenceEquals(_process, process))
            {
                _process = null;
            }

            _connectedAtUtc = null;
            _isConnectionConfirmed = false;
            exitMessage = BuildExitMessageLocked(process);
            _lastError = exitMessage;
            shouldStopAfterPromptFailure = ShouldStopAfterInteractiveAuthFailureLocked(exitMessage);

            if (shouldStopAfterPromptFailure)
            {
                TransitionToFaultedStateLocked();
            }

            shouldReconnect = !_disposed && !_stopRequested && _activeConnection is not null && _activeGroups.Count > 0;
            wasStopRequested = _stopRequested;
            askPassMutexToDispose = StopAskPassMonitoringLocked();
        }

        DisposeMutex(askPassMutexToDispose);

        CleanupProcessHandlers(process);

        try
        {
            process?.Dispose();
        }
        catch
        {
        }

        if (!wasStopRequested || shouldStopAfterPromptFailure)
        {
            AddLog(shouldReconnect ? "Warning" : "Error", exitMessage);
        }

        if (shouldReconnect)
        {
            ScheduleReconnect(exitMessage);
            return;
        }

        RaiseStateChanged();
    }

    private void OnOutputDataReceived(object sender, DataReceivedEventArgs eventArgs)
    {
        AppendMessage(eventArgs.Data);
    }

    private void OnErrorDataReceived(object sender, DataReceivedEventArgs eventArgs)
    {
        AppendMessage(eventArgs.Data);
    }

    private void AppendMessage(string? message)
    {
        if (string.IsNullOrWhiteSpace(message))
        {
            return;
        }

        var trimmedMessage = message.Trim();
        string? connectedEndpointLabel = null;
        bool shouldRaiseState = false;

        lock (_syncRoot)
        {
            _recentMessages.Add(trimmedMessage);

            if (_recentMessages.Count > 12)
            {
                _recentMessages.RemoveAt(0);
            }

            if (!_isConnectionConfirmed && _activeConnection is not null && LooksLikeSuccessfulConnectionMessage(trimmedMessage))
            {
                _isConnectionConfirmed = true;
                _connectedAtUtc = DateTimeOffset.UtcNow;
                _reconnectAttempt = 0;
                _reconnectAtUtc = null;
                _faultedConnectionId = null;
                _lastError = null;
                connectedEndpointLabel = _activeConnection.GetEndpointLabel();
                shouldRaiseState = true;
            }
        }

        AddLog("SSH", trimmedMessage);

        if (!string.IsNullOrWhiteSpace(connectedEndpointLabel))
        {
            AddLog("Info", $"SSH tunnel connected to {connectedEndpointLabel}.");
        }

        if (shouldRaiseState)
        {
            RaiseStateChanged();
        }
    }

    private void ScheduleReconnect(string? reason)
    {
        TimeSpan delay;
        string? effectiveReason;
        bool waitingForManualRetry;

        lock (_syncRoot)
        {
            if (_disposed || _stopRequested || _activeConnection is null || _activeGroups.Count == 0)
            {
                return;
            }

            _reconnectAttempt++;
            _lastError = string.IsNullOrWhiteSpace(reason) ? _lastError : reason;
            effectiveReason = _lastError;

            if (_reconnectAttempt >= MaxReconnectAttempts)
            {
                TransitionToFaultedStateLocked();
                delay = TimeSpan.Zero;
                waitingForManualRetry = true;
            }
            else
            {
                delay = TimeSpan.FromSeconds(Math.Min(30, 1 << _reconnectAttempt));
                _reconnectAtUtc = DateTimeOffset.UtcNow.Add(delay);
                _reconnectTimer.Change(delay, Timeout.InfiniteTimeSpan);
                waitingForManualRetry = false;
            }
        }

        if (waitingForManualRetry)
        {
            AddLog("Error", string.IsNullOrWhiteSpace(effectiveReason)
                ? $"SSH connection failed {MaxReconnectAttempts} times. Waiting for manual retry."
                : $"SSH connection failed {MaxReconnectAttempts} times. Waiting for manual retry: {effectiveReason}");
        }
        else
        {
            AddLog("Warning", string.IsNullOrWhiteSpace(effectiveReason)
                ? $"Retrying SSH connection in {(int)delay.TotalSeconds}s."
                : $"Retrying SSH connection in {(int)delay.TotalSeconds}s: {effectiveReason}");
        }

        RaiseStateChanged();
    }

    private TunnelRuntimeSnapshot CreateSnapshotLocked()
    {
        var isRunning = _process is not null && !_process.HasExited;
        var isConfirmedConnected = isRunning && _isConnectionConfirmed;
        var hasDesiredSession = !_stopRequested && _activeConnection is not null && _activeGroups.Count > 0;
        var isAwaitingAuthentication = _isAwaitingAuthentication;
        var isFaulted = _faultedConnectionId is not null;
        string statusText;

        if (isFaulted)
        {
            statusText = string.IsNullOrWhiteSpace(_lastError)
                ? $"Connection failed after {MaxReconnectAttempts} attempts. Click Connect to retry."
                : $"Connection failed after {MaxReconnectAttempts} attempts: {_lastError}";
        }
        else if (!hasDesiredSession)
        {
            statusText = string.IsNullOrWhiteSpace(_lastError)
                ? "Idle"
                : $"Idle: {_lastError}";
        }
        else if (isAwaitingAuthentication)
        {
            statusText = $"Awaiting authentication for {_activeConnection!.GetEndpointLabel()}";
        }
        else if (isConfirmedConnected)
        {
            statusText = $"Connected to {_activeConnection!.GetEndpointLabel()}";
        }
        else if (_reconnectAtUtc is not null)
        {
            var remainingSeconds = Math.Max(0, (int)Math.Ceiling((_reconnectAtUtc.Value - DateTimeOffset.UtcNow).TotalSeconds));
            var nextAttempt = Math.Min(MaxReconnectAttempts, _reconnectAttempt + 1);
            statusText = string.IsNullOrWhiteSpace(_lastError)
                ? $"Retrying in {remainingSeconds}s (attempt {nextAttempt} of {MaxReconnectAttempts})"
                : $"Retrying in {remainingSeconds}s (attempt {nextAttempt} of {MaxReconnectAttempts}): {_lastError}";
        }
        else
        {
            var currentAttempt = Math.Min(MaxReconnectAttempts, Math.Max(1, _reconnectAttempt + 1));
            statusText = currentAttempt <= 1
                ? $"Connecting to {_activeConnection!.GetEndpointLabel()}..."
                : $"Connecting to {_activeConnection!.GetEndpointLabel()} (attempt {currentAttempt} of {MaxReconnectAttempts})...";
        }

        return new TunnelRuntimeSnapshot
        {
            IsActive = isConfirmedConnected,
            IsConnecting = hasDesiredSession && !isConfirmedConnected,
            IsAwaitingAuthentication = isAwaitingAuthentication,
            IsFaulted = isFaulted,
            ConnectionId = _activeConnection?.Id,
            FaultedConnectionId = _faultedConnectionId,
            ActiveGroupIds = _activeGroups.Select(static group => group.Id).ToArray(),
            StatusText = statusText,
            ConnectedAtUtc = _connectedAtUtc,
            LastError = _lastError
        };
    }

    private Process? DetachProcessLocked()
    {
        var currentProcess = _process;
        _process = null;

        if (currentProcess is null)
        {
            return null;
        }

        CleanupProcessHandlers(currentProcess);
        return currentProcess;
    }

    private static void StopProcess(Process? process)
    {
        if (process is null)
        {
            return;
        }

        try
        {
            if (!process.HasExited)
            {
                process.Kill(true);
                process.WaitForExit(5000);
            }
        }
        catch
        {
        }

        try
        {
            process.Dispose();
        }
        catch
        {
        }
    }

    private void ConfigureAskPass(ProcessStartInfo startInfo)
    {
        var currentProcessPath = Environment.ProcessPath;

        if (string.IsNullOrWhiteSpace(currentProcessPath) || !File.Exists(currentProcessPath))
        {
            AddLog("Warning", "Interactive SSH authentication helper is unavailable because the current executable path could not be resolved.");
            return;
        }

        var askPassMutexName = $@"Local\SimpleTunnelSSH.AskPass.{Guid.NewGuid():N}";
        var askPassMutex = new Mutex(initiallyOwned: false, askPassMutexName);
        Mutex? previousMutex;

        lock (_syncRoot)
        {
            previousMutex = StopAskPassMonitoringLocked();
            _askPassMutex = askPassMutex;
        }

        DisposeMutex(previousMutex);
        startInfo.Environment["SSH_ASKPASS"] = currentProcessPath;
        startInfo.Environment["SSH_ASKPASS_REQUIRE"] = "force";
        startInfo.Environment["DISPLAY"] = "SimpleTunnelSSH";
        startInfo.Environment["SIMPLE_TUNNEL_ASKPASS_MODE"] = "1";
        startInfo.Environment["SIMPLE_TUNNEL_ASKPASS_MUTEX"] = askPassMutexName;
    }

    private void PollAuthenticationPromptState()
    {
        bool shouldRaiseState;
        bool isAwaitingAuthentication;

        lock (_syncRoot)
        {
            if (_disposed || _askPassMutex is null || _activeConnection is null || _stopRequested)
            {
                return;
            }

            isAwaitingAuthentication = IsInteractivePromptActive(_askPassMutex);

            if (isAwaitingAuthentication == _isAwaitingAuthentication)
            {
                return;
            }

            _isAwaitingAuthentication = isAwaitingAuthentication;

            if (isAwaitingAuthentication)
            {
                _authenticationPromptShownForCurrentProcess = true;
            }
            else
            {
                _lastAuthenticationPromptClosedAtUtc = DateTimeOffset.UtcNow;
            }

            shouldRaiseState = true;
        }

        AddLog(isAwaitingAuthentication ? "Auth" : "Info",
            isAwaitingAuthentication
                ? "SSH requested interactive authentication."
                : "Authentication prompt closed.");

        if (shouldRaiseState)
        {
            RaiseStateChanged();
        }
    }

    private void RaiseStateChanged()
    {
        var handler = StateChanged;

        if (handler is null)
        {
            return;
        }

        TunnelRuntimeSnapshot snapshot;

        lock (_syncRoot)
        {
            snapshot = CreateSnapshotLocked();
        }

        handler.Invoke(this, snapshot);
    }

    private void AddLog(string level, string message)
    {
        if (string.IsNullOrWhiteSpace(message))
        {
            return;
        }

        lock (_syncRoot)
        {
            _logEntries.Add(new RuntimeLogEntry
            {
                TimestampUtc = DateTimeOffset.UtcNow,
                Level = level,
                Message = message.Trim()
            });

            if (_logEntries.Count > MaxRuntimeLogEntries)
            {
                _logEntries.RemoveRange(0, _logEntries.Count - MaxRuntimeLogEntries);
            }
        }

        RaiseLogsChanged();
    }

    private void RaiseLogsChanged()
    {
        var handler = LogsChanged;
        handler?.Invoke(this, EventArgs.Empty);
    }

    private string BuildExitMessageLocked(Process? process)
    {
        var message = _recentMessages.Count == 0 ? null : _recentMessages[^1];
        var exitCode = process?.ExitCode ?? -1;
        return string.IsNullOrWhiteSpace(message)
            ? $"ssh.exe exited with code {exitCode}."
            : $"ssh.exe exited with code {exitCode}: {message}";
    }

    private void CleanupProcessHandlers(Process? process)
    {
        if (process is null)
        {
            return;
        }

        process.Exited -= OnProcessExited;
        process.OutputDataReceived -= OnOutputDataReceived;
        process.ErrorDataReceived -= OnErrorDataReceived;
    }

    private Mutex? StopAskPassMonitoringLocked()
    {
        _authPromptMonitorTimer.Change(Timeout.InfiniteTimeSpan, Timeout.InfiniteTimeSpan);

        var currentMutex = _askPassMutex;
        _askPassMutex = null;
        _isAwaitingAuthentication = false;
        return currentMutex;
    }

    private void TransitionToFaultedStateLocked()
    {
        _faultedConnectionId = _activeConnection?.Id ?? _faultedConnectionId;
        _stopRequested = true;
        _activeConnection = null;
        _activeGroups = [];
        _connectedAtUtc = null;
        _isConnectionConfirmed = false;
        _reconnectAtUtc = null;
    }

    private bool ShouldStopAfterInteractiveAuthFailureLocked(string exitMessage)
    {
        if (!_authenticationPromptShownForCurrentProcess || _lastAuthenticationPromptClosedAtUtc is null)
        {
            return false;
        }

        if ((DateTimeOffset.UtcNow - _lastAuthenticationPromptClosedAtUtc.Value) > TimeSpan.FromSeconds(5))
        {
            return false;
        }

        return exitMessage.Contains("Permission denied", StringComparison.OrdinalIgnoreCase)
            || exitMessage.Contains("authentication", StringComparison.OrdinalIgnoreCase)
            || exitMessage.Contains("passphrase", StringComparison.OrdinalIgnoreCase)
            || exitMessage.Contains("publickey", StringComparison.OrdinalIgnoreCase)
            || exitMessage.Contains("keyboard-interactive", StringComparison.OrdinalIgnoreCase)
            || exitMessage.Contains("exited with code 255", StringComparison.OrdinalIgnoreCase);
    }

    private static bool LooksLikeSuccessfulConnectionMessage(string message)
    {
        return message.Contains("Authenticated to ", StringComparison.OrdinalIgnoreCase)
            || message.Contains("Entering interactive session.", StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsInteractivePromptActive(Mutex mutex)
    {
        try
        {
            if (!mutex.WaitOne(0))
            {
                return true;
            }

            mutex.ReleaseMutex();
            return false;
        }
        catch (AbandonedMutexException)
        {
            mutex.ReleaseMutex();
            return false;
        }
        catch (ObjectDisposedException)
        {
            return false;
        }
    }

    private static void DisposeMutex(Mutex? mutex)
    {
        try
        {
            mutex?.Dispose();
        }
        catch
        {
        }
    }

    private void ThrowIfDisposed()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
    }

    private static string ResolveSshExecutablePath()
    {
        var systemPath = Path.Combine(Environment.SystemDirectory, "OpenSSH", "ssh.exe");
        return File.Exists(systemPath) ? systemPath : "ssh.exe";
    }
}