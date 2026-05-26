using System.Drawing;
using System.Linq;
using System.Threading;
using SimpleTunnelSSH.Core.Infrastructure;
using SimpleTunnelSSH.Core.Models;
using SimpleTunnelSSH.Core.Services;

namespace SimpleTunnelSSH.App;

public sealed class TunnelApplicationContext : ApplicationContext
{
    private const int CurrentSchemaVersion = 2;

    private readonly SynchronizationContext _synchronizationContext;
    private readonly JsonConfigurationStore _configurationStore;
    private readonly TunnelRuntimeService _runtimeService;
    private readonly NotifyIcon _notifyIcon;
    private readonly ContextMenuStrip _trayMenu;

    private AppConfiguration _configuration = new();
    private TunnelRuntimeSnapshot _runtimeSnapshot = TunnelRuntimeSnapshot.Idle;
    private MainForm? _mainForm;
    private LogViewerForm? _logViewerForm;
    private bool _exitRequested;
    private bool _minimizeHintShown;
    private Guid? _transitionConnectionId;
    private int _transitionDepth;

    public TunnelApplicationContext()
    {
        _synchronizationContext = SynchronizationContext.Current ?? new WindowsFormsSynchronizationContext();
        _configurationStore = new JsonConfigurationStore(AppPaths.GetConfigurationFilePath());
        _runtimeService = new TunnelRuntimeService();
        _runtimeService.StateChanged += OnRuntimeStateChanged;
        _runtimeService.LogsChanged += OnRuntimeLogsChanged;

        _trayMenu = new ContextMenuStrip();
        AppTheme.ApplyToolStripTheme(_trayMenu);
        _notifyIcon = new NotifyIcon
        {
            Icon = AppIconFactory.InactiveTrayIcon,
            Visible = true,
            Text = "Simple Tunnel SSH",
            ContextMenuStrip = _trayMenu
        };
        _notifyIcon.DoubleClick += (_, _) => ShowMainWindow();

        LoadConfiguration();
        _ = TryApplyWindowsStartupPreference(_configuration.Settings.LaunchAtWindowsStartup, showError: false);
        UpdateTrayMenu();

        if (!_configuration.Settings.StartMinimizedToTray)
        {
            ShowMainWindow();
        }
    }

    public AppConfiguration Configuration => _configuration;

    public TunnelRuntimeSnapshot RuntimeSnapshot => _runtimeSnapshot;

    public IReadOnlyCollection<Guid> GetDraftSelection(Guid connectionId)
    {
        return FindConnection(connectionId)?.Groups
            .Where(static group => group.IsEnabled)
            .Select(static group => group.Id)
            .ToArray() ?? Array.Empty<Guid>();
    }

    public bool IsConnectionTransitioning(Guid connectionId)
    {
        return (_transitionDepth > 0 && _transitionConnectionId == connectionId)
            || (_runtimeSnapshot.IsConnecting && !_runtimeSnapshot.IsAwaitingAuthentication && _runtimeSnapshot.ConnectionId == connectionId);
    }

    public void SetLastSelectedConnection(Guid? connectionId)
    {
        _configuration.LastSelectedConnectionId = connectionId;
    }

    public void UpdateDraftSelection(Guid connectionId, IEnumerable<Guid> groupIds)
    {
        var connection = FindConnection(connectionId);

        if (connection is null)
        {
            return;
        }

        ApplyGroupSelection(connection, groupIds.ToHashSet());
        RefreshUi();
    }

    public void ShowMainWindow()
    {
        if (_mainForm is null || _mainForm.IsDisposed)
        {
            _mainForm = new MainForm(this, _configuration.Settings.DeepClone());
        }

        _mainForm.Render(_configuration, _runtimeSnapshot);
        _mainForm.Show();
        _mainForm.WindowState = FormWindowState.Normal;
        _mainForm.Activate();
    }

    public void ShowLogViewer()
    {
        if (_logViewerForm is null || _logViewerForm.IsDisposed)
        {
            _logViewerForm = new LogViewerForm(this);
            _logViewerForm.FormClosed += (_, _) => _logViewerForm = null;
        }

        _logViewerForm.Render(_runtimeService.GetRecentLogs(), _configuration.Settings.LogColumnWidths);
        _logViewerForm.Show();
        _logViewerForm.WindowState = FormWindowState.Normal;
        _logViewerForm.Activate();
    }

    public void HandleMainFormClosing(FormClosingEventArgs eventArgs)
    {
        if (_exitRequested)
        {
            return;
        }

        eventArgs.Cancel = true;
        _mainForm?.Hide();

        if (_minimizeHintShown)
        {
            return;
        }

        _notifyIcon.ShowBalloonTip(
            2500,
            "Simple Tunnel SSH",
            "The window was hidden to the tray and the tunnel service is still running.",
            ToolTipIcon.Info);
        _minimizeHintShown = true;
    }

    public async Task EditSettingsAsync(IWin32Window owner)
    {
        var editedSettings = SettingsForm.ShowDialog(owner, _configuration.Settings);

        if (editedSettings is null)
        {
            return;
        }

        NormalizeSettings(editedSettings);

        if (!TryApplyWindowsStartupPreference(editedSettings.LaunchAtWindowsStartup, owner))
        {
            return;
        }

        _configuration.Settings = editedSettings;
        await PersistConfigurationAsync().ConfigureAwait(true);
        ApplyWindowSettingsToOpenForms();
        RefreshUi();
    }

    public async Task SaveGroupColumnWidthsAsync(IReadOnlyList<int> widths)
    {
        if (!UpdateColumnWidths(_configuration.Settings.GroupColumnWidths, widths))
        {
            return;
        }

        await PersistConfigurationAsync().ConfigureAwait(true);
    }

    public async Task SavePortColumnWidthsAsync(IReadOnlyList<int> widths)
    {
        if (!UpdateColumnWidths(_configuration.Settings.PortColumnWidths, widths))
        {
            return;
        }

        await PersistConfigurationAsync().ConfigureAwait(true);
    }

    public async Task SaveLogColumnWidthsAsync(IReadOnlyList<int> widths)
    {
        if (!UpdateColumnWidths(_configuration.Settings.LogColumnWidths, widths))
        {
            return;
        }

        await PersistConfigurationAsync().ConfigureAwait(true);
    }

    public void ClearRuntimeLogs()
    {
        _runtimeService.ClearLogs();
    }

    public string GetRuntimeLogsText()
    {
        return _runtimeService.GetRecentLogsText();
    }

    public async Task AddConnectionAsync(IWin32Window owner)
    {
        var connection = ConnectionEditorForm.ShowDialog(owner, null);

        if (connection is null)
        {
            return;
        }

        _configuration.Connections.Add(connection);
        _configuration.LastSelectedConnectionId = connection.Id;

        await PersistConfigurationAsync().ConfigureAwait(true);
        RefreshUi();
    }

    public async Task EditConnectionAsync(Guid connectionId, IWin32Window owner)
    {
        var index = _configuration.Connections.FindIndex(connection => connection.Id == connectionId);

        if (index < 0)
        {
            return;
        }

        var editedConnection = ConnectionEditorForm.ShowDialog(owner, _configuration.Connections[index]);

        if (editedConnection is null)
        {
            return;
        }

        _configuration.Connections[index] = editedConnection;
        await PersistConfigurationAsync().ConfigureAwait(true);

        if (_runtimeSnapshot.ConnectionId == editedConnection.Id)
        {
            await ReapplyRuntimeAsync(owner).ConfigureAwait(true);
        }

        RefreshUi();
    }

    public async Task DeleteConnectionAsync(Guid connectionId, IWin32Window owner)
    {
        var connection = FindConnection(connectionId);

        if (connection is null)
        {
            return;
        }

        var result = MessageBox.Show(
            owner,
            $"Delete the connection '{connection.Name}' and all of its groups?",
            "Delete connection",
            MessageBoxButtons.YesNo,
            MessageBoxIcon.Warning);

        if (result != DialogResult.Yes)
        {
            return;
        }

        if (_runtimeSnapshot.ConnectionId == connectionId)
        {
            await RunConnectionTransitionAsync(connectionId, () => _runtimeService.DeactivateAsync()).ConfigureAwait(true);
        }

        _configuration.Connections.RemoveAll(currentConnection => currentConnection.Id == connectionId);

        if (_configuration.LastSelectedConnectionId == connectionId)
        {
            _configuration.LastSelectedConnectionId = _configuration.Connections.FirstOrDefault()?.Id;
        }

        await PersistConfigurationAsync().ConfigureAwait(true);
        RefreshUi();
    }

    public async Task AddGroupAsync(Guid connectionId, IWin32Window owner)
    {
        var connection = FindConnection(connectionId);

        if (connection is null)
        {
            return;
        }

        var group = GroupEditorForm.ShowDialog(owner, null);

        if (group is null)
        {
            return;
        }

        group.IsEnabled = group.Ports.Any(static port => port.IsEnabled);
        connection.Groups.Add(group);

        await PersistConfigurationAsync().ConfigureAwait(true);
        RefreshUi();
    }

    public async Task EditGroupAsync(Guid connectionId, Guid groupId, IWin32Window owner)
    {
        var group = FindGroup(connectionId, groupId);

        if (group is null)
        {
            return;
        }

        var editedGroup = GroupEditorForm.ShowDialog(owner, group);

        if (editedGroup is null)
        {
            return;
        }

        group.Name = editedGroup.Name;
        await PersistConfigurationAsync().ConfigureAwait(true);
        RefreshUi();
    }

    public async Task DeleteGroupAsync(Guid connectionId, Guid groupId, IWin32Window owner)
    {
        var connection = FindConnection(connectionId);
        var group = FindGroup(connectionId, groupId);

        if (connection is null || group is null)
        {
            return;
        }

        var result = MessageBox.Show(
            owner,
            $"Delete the group '{group.Name}' and all of its ports?",
            "Delete group",
            MessageBoxButtons.YesNo,
            MessageBoxIcon.Warning);

        if (result != DialogResult.Yes)
        {
            return;
        }

        var shouldReapply = _runtimeSnapshot.ConnectionId == connection.Id && group.IsEnabled;
        connection.Groups.RemoveAll(currentGroup => currentGroup.Id == groupId);

        await PersistConfigurationAsync().ConfigureAwait(true);

        if (shouldReapply)
        {
            await ReapplyRuntimeAsync(owner, showConflictMessage: false).ConfigureAwait(true);
        }

        RefreshUi();
    }

    public async Task AddPortAsync(Guid connectionId, Guid groupId, IWin32Window owner)
    {
        var group = FindGroup(connectionId, groupId);

        if (group is null)
        {
            return;
        }

        var port = PortEditorForm.ShowDialog(owner, null);

        if (port is null || !ValidatePort(group, port, owner))
        {
            return;
        }

        group.Ports.Add(port);
        await PersistConfigurationAsync().ConfigureAwait(true);

        if (_runtimeSnapshot.ConnectionId == connectionId && group.IsEnabled)
        {
            await ReapplyRuntimeAsync(owner).ConfigureAwait(true);
        }

        RefreshUi();
    }

    public async Task EditPortAsync(Guid connectionId, Guid groupId, Guid portId, IWin32Window owner)
    {
        var group = FindGroup(connectionId, groupId);

        if (group is null)
        {
            return;
        }

        var currentPort = group.Ports.FirstOrDefault(port => port.Id == portId);

        if (currentPort is null)
        {
            return;
        }

        var editedPort = PortEditorForm.ShowDialog(owner, currentPort);

        if (editedPort is null || !ValidatePort(group, editedPort, owner))
        {
            return;
        }

        currentPort.Description = editedPort.Description;
        currentPort.IsEnabled = editedPort.IsEnabled;
        currentPort.LocalPort = editedPort.LocalPort;
        currentPort.RemoteHost = editedPort.RemoteHost;
        currentPort.RemotePort = editedPort.RemotePort;

        if (!group.Ports.Any(static port => port.IsEnabled))
        {
            group.IsEnabled = false;
        }

        await PersistConfigurationAsync().ConfigureAwait(true);

        if (_runtimeSnapshot.ConnectionId == connectionId && group.IsEnabled)
        {
            await ReapplyRuntimeAsync(owner).ConfigureAwait(true);
        }

        RefreshUi();
    }

    public async Task DeletePortAsync(Guid connectionId, Guid groupId, Guid portId, IWin32Window owner)
    {
        var group = FindGroup(connectionId, groupId);
        var port = group?.Ports.FirstOrDefault(currentPort => currentPort.Id == portId);

        if (group is null || port is null)
        {
            return;
        }

        var result = MessageBox.Show(
            owner,
            $"Delete port {port.LocalPort}:{port.RemoteHost}:{port.RemotePort}?",
            "Delete port",
            MessageBoxButtons.YesNo,
            MessageBoxIcon.Warning);

        if (result != DialogResult.Yes)
        {
            return;
        }

        var shouldReapply = _runtimeSnapshot.ConnectionId == connectionId && _runtimeSnapshot.ActiveGroupIds.Contains(groupId);
        group.Ports.RemoveAll(currentPort => currentPort.Id == portId);

        if (!group.Ports.Any(static currentPort => currentPort.IsEnabled))
        {
            group.IsEnabled = false;
        }

        await PersistConfigurationAsync().ConfigureAwait(true);

        if (shouldReapply)
        {
            await ReapplyRuntimeAsync(owner, showConflictMessage: false).ConfigureAwait(true);
        }

        RefreshUi();
    }

    public async Task ActivateConnectionAsync(Guid connectionId, IEnumerable<Guid> groupIds, IWin32Window? owner = null)
    {
        var connection = FindConnection(connectionId);

        if (connection is null)
        {
            return;
        }

        var desiredGroupIds = NormalizeGroupSelection(connection, groupIds);
        var selectedGroups = connection.Groups
            .Where(group => desiredGroupIds.Contains(group.Id))
            .ToList();
        var runnableGroups = GetRunnableGroups(selectedGroups);

        if (selectedGroups.Count == 0)
        {
            MessageBox.Show(
                owner,
                "Activate at least one group before activating the connection.",
                "No group selected",
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
            RefreshUi();
            return;
        }

        if (runnableGroups.Count == 0)
        {
            MessageBox.Show(
                owner,
                "Activate at least one port before activating the connection.",
                "No active port",
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
            RefreshUi();
            return;
        }

        var conflicts = PortConflictDetector.Detect(runnableGroups);

        if (conflicts.Count > 0)
        {
            MessageBox.Show(
                owner,
                BuildConflictMessage(conflicts),
                "Port conflict detected",
                MessageBoxButtons.OK,
                MessageBoxIcon.Warning);
            RefreshUi();
            return;
        }

        if (_runtimeSnapshot.ConnectionId is Guid activeConnectionId && activeConnectionId != connectionId)
        {
            var result = MessageBox.Show(
                owner,
                "Another connection is already active. Switch to the selected connection?",
                "Switch connection",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Question);

            if (result != DialogResult.Yes)
            {
                RefreshUi();
                return;
            }
        }

        _configuration.LastSelectedConnectionId = connectionId;
        ApplyGroupSelection(connection, runnableGroups.Select(static group => group.Id).ToHashSet());
        await PersistConfigurationAsync().ConfigureAwait(true);
        await RunConnectionTransitionAsync(connectionId, () => _runtimeService.ActivateAsync(connection, runnableGroups)).ConfigureAwait(true);
        RefreshUi();
    }

    public async Task DeactivateConnectionAsync()
    {
        var connectionId = _runtimeSnapshot.ConnectionId;
        await RunConnectionTransitionAsync(connectionId, () => _runtimeService.DeactivateAsync()).ConfigureAwait(true);
        RefreshUi();
    }

    public async Task ToggleActiveGroupAsync(Guid connectionId, Guid groupId, bool shouldBeActive, IWin32Window? owner = null)
    {
        var connection = FindConnection(connectionId);
        var group = FindGroup(connectionId, groupId);

        if (connection is null || group is null)
        {
            return;
        }

        if (shouldBeActive && !group.Ports.Any(static port => port.IsEnabled))
        {
            MessageBox.Show(
                owner,
                $"Activate at least one port in '{group.Name}' before activating the group.",
                "No active port",
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
            RefreshUi();
            return;
        }

        var activeSelection = connection.Groups
            .Where(static currentGroup => currentGroup.IsEnabled)
            .Select(static currentGroup => currentGroup.Id)
            .ToHashSet();

        if (shouldBeActive)
        {
            activeSelection.Add(groupId);
        }
        else
        {
            activeSelection.Remove(groupId);
        }

        if (activeSelection.Count == 0)
        {
            ApplyGroupSelection(connection, new HashSet<Guid>());
            await PersistConfigurationAsync().ConfigureAwait(true);

            if (_runtimeSnapshot.ConnectionId == connectionId)
            {
                await DeactivateConnectionAsync().ConfigureAwait(true);
            }
            else
            {
                RefreshUi();
            }

            return;
        }

        var selectedGroups = connection.Groups
            .Where(currentGroup => activeSelection.Contains(currentGroup.Id))
            .ToList();
        var runnableGroups = selectedGroups
            .Where(static currentGroup => currentGroup.Ports.Any(static currentPort => currentPort.IsEnabled))
            .ToList();

        if (runnableGroups.Count == 0)
        {
            ApplyGroupSelection(connection, new HashSet<Guid>());
            await PersistConfigurationAsync().ConfigureAwait(true);

            if (_runtimeSnapshot.ConnectionId == connectionId)
            {
                await DeactivateConnectionAsync().ConfigureAwait(true);
            }
            else
            {
                RefreshUi();
            }

            return;
        }

        var conflicts = PortConflictDetector.Detect(runnableGroups);

        if (conflicts.Count > 0 && shouldBeActive)
        {
            var conflictingGroupIds = conflicts
                .Where(conflict => conflict.FirstGroupId == groupId || conflict.SecondGroupId == groupId)
                .Select(conflict => conflict.FirstGroupId == groupId ? conflict.SecondGroupId : conflict.FirstGroupId)
                .Distinct()
                .ToList();

            if (conflictingGroupIds.Count > 0)
            {
                var conflictingGroups = connection.Groups
                    .Where(currentGroup => conflictingGroupIds.Contains(currentGroup.Id))
                    .Select(static currentGroup => currentGroup.Name)
                    .ToList();

                var result = MessageBox.Show(
                    owner,
                    $"Activating this group requires disabling: {string.Join(", ", conflictingGroups)}. Continue?",
                    "Replace conflicting groups",
                    MessageBoxButtons.YesNo,
                    MessageBoxIcon.Question);

                if (result == DialogResult.Yes)
                {
                    activeSelection.ExceptWith(conflictingGroupIds);
                    selectedGroups = connection.Groups.Where(currentGroup => activeSelection.Contains(currentGroup.Id)).ToList();
                    runnableGroups = selectedGroups
                        .Where(static currentGroup => currentGroup.Ports.Any(static currentPort => currentPort.IsEnabled))
                        .ToList();
                    conflicts = PortConflictDetector.Detect(runnableGroups);
                }
            }
        }

        if (conflicts.Count > 0)
        {
            MessageBox.Show(
                owner,
                BuildConflictMessage(conflicts),
                "Port conflict detected",
                MessageBoxButtons.OK,
                MessageBoxIcon.Warning);
            RefreshUi();
            return;
        }

        ApplyGroupSelection(connection, runnableGroups.Select(static currentGroup => currentGroup.Id).ToHashSet());
        await PersistConfigurationAsync().ConfigureAwait(true);

        if (_runtimeSnapshot.ConnectionId == connectionId)
        {
            await RunConnectionTransitionAsync(connectionId, () => _runtimeService.ActivateAsync(connection, runnableGroups)).ConfigureAwait(true);
        }

        RefreshUi();
    }

    public async Task ToggleActivePortAsync(Guid connectionId, Guid groupId, Guid portId, bool shouldBeActive, IWin32Window? owner = null)
    {
        var connection = FindConnection(connectionId);
        var group = FindGroup(connectionId, groupId);
        var port = group?.Ports.FirstOrDefault(currentPort => currentPort.Id == portId);

        if (connection is null || group is null || port is null)
        {
            return;
        }

        var previousPortState = port.IsEnabled;
        var previousGroupState = group.IsEnabled;

        port.IsEnabled = shouldBeActive;

        if (!group.Ports.Any(static currentPort => currentPort.IsEnabled))
        {
            group.IsEnabled = false;
        }

        if (shouldBeActive && group.IsEnabled)
        {
            var conflicts = PortConflictDetector.Detect(connection.Groups.Where(static currentGroup => currentGroup.IsEnabled));

            if (conflicts.Count > 0)
            {
                port.IsEnabled = previousPortState;
                group.IsEnabled = previousGroupState;
                MessageBox.Show(
                    owner,
                    BuildConflictMessage(conflicts),
                    "Port conflict detected",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);
                RefreshUi();
                return;
            }
        }

        await PersistConfigurationAsync().ConfigureAwait(true);

        if (_runtimeSnapshot.ConnectionId == connectionId && _runtimeSnapshot.ActiveGroupIds.Contains(groupId))
        {
            await ReapplyRuntimeAsync(owner, showConflictMessage: false).ConfigureAwait(true);
        }

        RefreshUi();
    }

    public async Task ExportConfigurationAsync(IWin32Window owner)
    {
        using var dialog = new SaveFileDialog
        {
            Filter = "JSON files (*.json)|*.json",
            FileName = "simple-tunnel-ssh-config.json",
            RestoreDirectory = true
        };

        if (dialog.ShowDialog(owner) != DialogResult.OK)
        {
            return;
        }

        await _configurationStore.ExportAsync(_configuration, dialog.FileName).ConfigureAwait(true);
    }

    public async Task ImportConfigurationAsync(IWin32Window owner)
    {
        using var dialog = new OpenFileDialog
        {
            Filter = "JSON files (*.json)|*.json",
            RestoreDirectory = true,
            CheckFileExists = true
        };

        if (dialog.ShowDialog(owner) != DialogResult.OK)
        {
            return;
        }

        var result = MessageBox.Show(
            owner,
            "Importing a configuration replaces the current connections, groups, ports and settings. Continue?",
            "Import configuration",
            MessageBoxButtons.YesNo,
            MessageBoxIcon.Warning);

        if (result != DialogResult.Yes)
        {
            return;
        }

        await _runtimeService.DeactivateAsync().ConfigureAwait(true);

        _configuration = await _configurationStore.ImportAsync(dialog.FileName).ConfigureAwait(true);
        NormalizeConfiguration();
        _ = TryApplyWindowsStartupPreference(_configuration.Settings.LaunchAtWindowsStartup, owner);
        await PersistConfigurationAsync().ConfigureAwait(true);
        ApplyWindowSettingsToOpenForms();
        RefreshUi();
    }

    public async Task ExitApplicationAsync()
    {
        _exitRequested = true;
        await _runtimeService.DeactivateAsync().ConfigureAwait(true);
        _notifyIcon.Visible = false;

        if (_mainForm is not null && !_mainForm.IsDisposed)
        {
            _mainForm.Close();
        }

        ExitThread();
    }

    protected override void ExitThreadCore()
    {
        if (_logViewerForm is not null && !_logViewerForm.IsDisposed)
        {
            _logViewerForm.Close();
            _logViewerForm.Dispose();
        }

        _notifyIcon.Visible = false;
        _notifyIcon.Dispose();
        _trayMenu.Dispose();
        _runtimeService.Dispose();
        base.ExitThreadCore();
    }

    private void LoadConfiguration()
    {
        _configuration = _configurationStore.LoadAsync().GetAwaiter().GetResult();
        NormalizeConfiguration();
    }

    private async Task ReapplyRuntimeAsync(IWin32Window? owner, bool showConflictMessage = true)
    {
        if (_runtimeSnapshot.ConnectionId is not Guid connectionId)
        {
            return;
        }

        var connection = FindConnection(connectionId);

        if (connection is null)
        {
            await RunConnectionTransitionAsync(connectionId, () => _runtimeService.DeactivateAsync()).ConfigureAwait(true);
            return;
        }

        var runnableGroups = GetRunnableGroups(connection.Groups.Where(static group => group.IsEnabled));

        if (runnableGroups.Count == 0)
        {
            await RunConnectionTransitionAsync(connectionId, () => _runtimeService.DeactivateAsync()).ConfigureAwait(true);
            return;
        }

        var conflicts = PortConflictDetector.Detect(runnableGroups);

        if (conflicts.Count > 0)
        {
            await RunConnectionTransitionAsync(connectionId, () => _runtimeService.DeactivateAsync()).ConfigureAwait(true);

            if (showConflictMessage)
            {
                MessageBox.Show(
                    owner,
                    "The active tunnel was stopped because the updated configuration created a port conflict. Review the active groups and ports, then activate the connection again.",
                    "Active tunnel stopped",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);
            }

            return;
        }

        await RunConnectionTransitionAsync(connectionId, () => _runtimeService.ActivateAsync(connection, runnableGroups)).ConfigureAwait(true);
    }

    private void NormalizeConfiguration()
    {
        _configuration.SchemaVersion = CurrentSchemaVersion;
        _configuration.Settings ??= new AppSettings();
        NormalizeSettings(_configuration.Settings);
        _configuration.Connections ??= [];

        foreach (var connection in _configuration.Connections)
        {
            if (connection.Id == Guid.Empty)
            {
                connection.Id = Guid.NewGuid();
            }

            connection.Groups ??= [];
            connection.Host = connection.Host?.Trim() ?? string.Empty;
            connection.UserName = connection.UserName?.Trim() ?? string.Empty;
            connection.IdentityFilePath = connection.IdentityFilePath?.Trim() ?? string.Empty;
            connection.Name = string.IsNullOrWhiteSpace(connection.Name)
                ? connection.GetEndpointLabel()
                : connection.Name.Trim();

            if (connection.Port <= 0)
            {
                connection.Port = 22;
            }

            foreach (var group in connection.Groups)
            {
                if (group.Id == Guid.Empty)
                {
                    group.Id = Guid.NewGuid();
                }

                group.Ports ??= [];
                group.Name = string.IsNullOrWhiteSpace(group.Name) ? "Group" : group.Name.Trim();

                foreach (var port in group.Ports)
                {
                    if (port.Id == Guid.Empty)
                    {
                        port.Id = Guid.NewGuid();
                    }

                    port.Description = port.Description?.Trim() ?? string.Empty;
                    port.RemoteHost = string.IsNullOrWhiteSpace(port.RemoteHost) ? "localhost" : port.RemoteHost.Trim();

                    if (port.RemotePort <= 0)
                    {
                        port.RemotePort = port.LocalPort;
                    }
                }

                if (!group.Ports.Any(static port => port.IsEnabled))
                {
                    group.IsEnabled = false;
                }
            }
        }

        var validConnectionIds = _configuration.Connections.Select(static connection => connection.Id).ToHashSet();

        if (_configuration.LastSelectedConnectionId is Guid lastSelectedConnectionId && !validConnectionIds.Contains(lastSelectedConnectionId))
        {
            _configuration.LastSelectedConnectionId = _configuration.Connections.FirstOrDefault()?.Id;
        }
    }

    private static void NormalizeSettings(AppSettings settings)
    {
        settings.WindowWidth = settings.WindowWidth < 760 ? AppSettings.DefaultWindowWidth : settings.WindowWidth;
        settings.WindowHeight = settings.WindowHeight < 520 ? AppSettings.DefaultWindowHeight : settings.WindowHeight;
        settings.GroupColumnWidths = NormalizeColumnWidths(settings.GroupColumnWidths, AppSettings.CreateDefaultGroupColumnWidths());
        settings.PortColumnWidths = NormalizeColumnWidths(settings.PortColumnWidths, AppSettings.CreateDefaultPortColumnWidths());
        settings.LogColumnWidths = NormalizeColumnWidths(settings.LogColumnWidths, AppSettings.CreateDefaultLogColumnWidths());
    }

    private static List<int> NormalizeColumnWidths(List<int>? widths, List<int> defaultWidths)
    {
        if (widths is null || widths.Count != defaultWidths.Count)
        {
            return defaultWidths;
        }

        for (var index = 0; index < widths.Count; index++)
        {
            if (widths[index] < 24)
            {
                widths[index] = defaultWidths[index];
            }
        }

        return widths;
    }

    private HashSet<Guid> NormalizeGroupSelection(SshConnectionProfile connection, IEnumerable<Guid> groupIds)
    {
        var validGroupIds = connection.Groups.Select(static group => group.Id).ToHashSet();
        var selection = groupIds.Where(validGroupIds.Contains).ToHashSet();

        if (selection.Count == 0)
        {
            selection = GetDraftSelection(connection.Id).Where(validGroupIds.Contains).ToHashSet();
        }

        return selection;
    }

    private bool ValidatePort(TunnelGroup group, TunnelPort candidatePort, IWin32Window owner)
    {
        var hasDuplicateLocalPort = group.Ports.Any(port =>
            port.Id != candidatePort.Id &&
            port.LocalPort == candidatePort.LocalPort);

        if (!hasDuplicateLocalPort)
        {
            return true;
        }

        MessageBox.Show(
            owner,
            $"The group '{group.Name}' already contains a forward for local port {candidatePort.LocalPort}.",
            "Duplicate local port",
            MessageBoxButtons.OK,
            MessageBoxIcon.Warning);
        return false;
    }

    private async Task PersistConfigurationAsync()
    {
        await _configurationStore.SaveAsync(_configuration).ConfigureAwait(true);
    }

    private bool TryApplyWindowsStartupPreference(bool enabled, IWin32Window? owner = null, bool showError = true)
    {
        try
        {
            WindowsStartupManager.SetEnabled(enabled);
            return true;
        }
        catch (Exception exception)
        {
            if (showError)
            {
                MessageBox.Show(
                    owner,
                    $"Unable to update Windows startup: {exception.Message}",
                    "Startup setting",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Warning);
            }

            return false;
        }
    }

    private static bool UpdateColumnWidths(List<int> currentWidths, IReadOnlyList<int> newWidths)
    {
        if (newWidths.Count == 0)
        {
            return false;
        }

        var changed = false;

        if (currentWidths.Count != newWidths.Count)
        {
            currentWidths.Clear();

            foreach (var width in newWidths)
            {
                currentWidths.Add(Math.Max(24, width));
            }

            return true;
        }

        for (var index = 0; index < newWidths.Count; index++)
        {
            var normalizedWidth = Math.Max(24, newWidths[index]);

            if (currentWidths[index] == normalizedWidth)
            {
                continue;
            }

            currentWidths[index] = normalizedWidth;
            changed = true;
        }

        return changed;
    }

    private async Task RunConnectionTransitionAsync(Guid? connectionId, Func<Task> action)
    {
        BeginConnectionTransition(connectionId);

        try
        {
            await action().ConfigureAwait(true);
        }
        finally
        {
            EndConnectionTransition(connectionId);
        }
    }

    private void BeginConnectionTransition(Guid? connectionId)
    {
        if (connectionId is null)
        {
            return;
        }

        _transitionConnectionId = connectionId;
        _transitionDepth++;
        RefreshUi();
    }

    private void EndConnectionTransition(Guid? connectionId)
    {
        if (connectionId is null)
        {
            return;
        }

        if (_transitionDepth > 0)
        {
            _transitionDepth--;
        }

        if (_transitionDepth == 0 && _transitionConnectionId == connectionId)
        {
            _transitionConnectionId = null;
        }

        RefreshUi();
    }

    private void OnRuntimeStateChanged(object? sender, TunnelRuntimeSnapshot snapshot)
    {
        _synchronizationContext.Post(_ =>
        {
            _runtimeSnapshot = snapshot;
            RefreshUi();
        }, null);
    }

    private void OnRuntimeLogsChanged(object? sender, EventArgs eventArgs)
    {
        _synchronizationContext.Post(_ => RefreshLogViewer(), null);
    }

    private void RefreshUi()
    {
        UpdateTrayMenu();

        if (_mainForm is not null && !_mainForm.IsDisposed)
        {
            _mainForm.Render(_configuration, _runtimeSnapshot);
        }

        RefreshLogViewer();
    }

    private void RefreshLogViewer()
    {
        if (_logViewerForm is null || _logViewerForm.IsDisposed)
        {
            return;
        }

        _logViewerForm.Render(_runtimeService.GetRecentLogs(), _configuration.Settings.LogColumnWidths);
    }

    private void ApplyWindowSettingsToOpenForms()
    {
        if (_mainForm is not null && !_mainForm.IsDisposed)
        {
            _mainForm.ApplyDefaultWindowSize(_configuration.Settings);
        }
    }

    private void UpdateTrayMenu()
    {
        _trayMenu.SuspendLayout();
        _trayMenu.Items.Clear();

        _trayMenu.Items.Add(new ToolStripMenuItem("Open", null, (_, _) => ShowMainWindow()));

        if (_configuration.Connections.Count > 0)
        {
            var connectionsMenuItem = new ToolStripMenuItem("Connections");

            foreach (var connection in _configuration.Connections.OrderBy(static connection => connection.Name, StringComparer.OrdinalIgnoreCase))
            {
                var currentConnectionId = connection.Id;
                var menuItem = new ToolStripMenuItem(connection.Name)
                {
                    Checked = _runtimeSnapshot.ConnectionId == currentConnectionId
                };
                menuItem.Click += async (_, _) =>
                {
                    if (_runtimeSnapshot.ConnectionId == currentConnectionId)
                    {
                        await DeactivateConnectionAsync().ConfigureAwait(true);
                    }
                    else
                    {
                        await ActivateConnectionAsync(currentConnectionId, GetDraftSelection(currentConnectionId)).ConfigureAwait(true);
                    }
                };
                connectionsMenuItem.DropDownItems.Add(menuItem);
            }

            _trayMenu.Items.Add(connectionsMenuItem);
        }

        var activeConnection = _runtimeSnapshot.ConnectionId is Guid activeConnectionId
            ? FindConnection(activeConnectionId)
            : null;

        var groupsMenuItem = new ToolStripMenuItem("Active Groups")
        {
            Enabled = activeConnection is not null
        };

        if (activeConnection is not null)
        {
            foreach (var group in activeConnection.Groups.OrderBy(static group => group.Name, StringComparer.OrdinalIgnoreCase))
            {
                var currentGroupId = group.Id;
                var menuItem = new ToolStripMenuItem(group.Name)
                {
                    Checked = group.IsEnabled,
                    CheckOnClick = true
                };
                menuItem.CheckedChanged += async (_, _) =>
                {
                    await ToggleActiveGroupAsync(activeConnection.Id, currentGroupId, menuItem.Checked).ConfigureAwait(true);
                };
                groupsMenuItem.DropDownItems.Add(menuItem);
            }
        }

        _trayMenu.Items.Add(groupsMenuItem);
        _trayMenu.Items.Add(new ToolStripSeparator());
        _trayMenu.Items.Add(new ToolStripMenuItem("Exit", null, async (_, _) => await ExitApplicationAsync().ConfigureAwait(true)));
        _trayMenu.ResumeLayout();

        _notifyIcon.Icon = _runtimeSnapshot.IsAwaitingAuthentication || _runtimeSnapshot.IsFaulted
            ? AppIconFactory.AttentionTrayIcon
            : _runtimeSnapshot.IsActive
                ? AppIconFactory.ActiveTrayIcon
                : AppIconFactory.InactiveTrayIcon;
        var tooltip = _runtimeSnapshot.IsActive || _runtimeSnapshot.IsConnecting || _runtimeSnapshot.IsFaulted
            ? $"Simple Tunnel SSH - {_runtimeSnapshot.StatusText}"
            : "Simple Tunnel SSH";
        _notifyIcon.Text = tooltip.Length > 63 ? tooltip[..63] : tooltip;
    }

    private SshConnectionProfile? FindConnection(Guid connectionId)
    {
        return _configuration.Connections.FirstOrDefault(connection => connection.Id == connectionId);
    }

    private TunnelGroup? FindGroup(Guid connectionId, Guid groupId)
    {
        return FindConnection(connectionId)?.Groups.FirstOrDefault(group => group.Id == groupId);
    }

    private static void ApplyGroupSelection(SshConnectionProfile connection, IReadOnlySet<Guid> activeGroupIds)
    {
        foreach (var group in connection.Groups)
        {
            group.IsEnabled = activeGroupIds.Contains(group.Id);
        }
    }

    private static List<TunnelGroup> GetRunnableGroups(IEnumerable<TunnelGroup> groups)
    {
        return groups
            .Where(static group => group.IsEnabled && group.Ports.Any(static port => port.IsEnabled))
            .ToList();
    }

    private static string BuildConflictMessage(IEnumerable<PortConflict> conflicts)
    {
        return string.Join(
            Environment.NewLine,
            conflicts.Select(conflict =>
                $"Local port {conflict.LocalPort} is shared by '{conflict.FirstGroupName}' and '{conflict.SecondGroupName}'."));
    }
}
