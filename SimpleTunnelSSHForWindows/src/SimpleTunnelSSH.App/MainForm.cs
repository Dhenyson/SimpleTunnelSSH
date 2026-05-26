using System.Drawing;
using System.Drawing.Drawing2D;
using System.Linq;
using SimpleTunnelSSH.Core.Infrastructure;
using SimpleTunnelSSH.Core.Models;

namespace SimpleTunnelSSH.App;

public sealed class MainForm : Form
{
    private readonly TunnelApplicationContext _applicationContext;
    private readonly ListBox _connectionsListBox;
    private readonly ListView _groupsListView;
    private readonly ListView _portsListView;
    private readonly Button _connectButton;
    private readonly Button _addConnectionButton;
    private readonly Button _editConnectionButton;
    private readonly Button _deleteConnectionButton;
    private readonly Button _addGroupButton;
    private readonly Button _toggleGroupButton;
    private readonly Button _editGroupButton;
    private readonly Button _deleteGroupButton;
    private readonly Button _addPortButton;
    private readonly Button _togglePortButton;
    private readonly Button _editPortButton;
    private readonly Button _deletePortButton;
    private readonly ToolStripStatusLabel _statusLabel;
    private readonly ToolStripStatusLabel _pathLabel;
    private readonly System.Windows.Forms.Timer _connectionIndicatorTimer;

    private AppConfiguration _configuration = new();
    private TunnelRuntimeSnapshot _runtimeSnapshot = TunnelRuntimeSnapshot.Idle;
    private int _connectionIndicatorFrame;
    private bool _suppressColumnWidthPersistence;

    public MainForm(TunnelApplicationContext applicationContext, AppSettings settings)
    {
        _applicationContext = applicationContext;

        Text = "Simple Tunnel SSH";
        StartPosition = FormStartPosition.CenterScreen;
        MinimumSize = new Size(760, 520);
        Icon = AppIconFactory.ApplicationIcon;
        ApplyDefaultWindowSize(settings);

        var menuStrip = new MenuStrip();
        var fileMenuItem = new ToolStripMenuItem("File");
        fileMenuItem.DropDownItems.Add(new ToolStripMenuItem("Import", null, async (_, _) => await _applicationContext.ImportConfigurationAsync(this)));
        fileMenuItem.DropDownItems.Add(new ToolStripMenuItem("Export", null, async (_, _) => await _applicationContext.ExportConfigurationAsync(this)));
        fileMenuItem.DropDownItems.Add(new ToolStripSeparator());
        fileMenuItem.DropDownItems.Add(new ToolStripMenuItem("Exit", null, async (_, _) => await _applicationContext.ExitApplicationAsync()));
        menuStrip.Items.Add(fileMenuItem);
        var logsMenuItem = new ToolStripMenuItem("Logs");
        logsMenuItem.DropDownItems.Add(new ToolStripMenuItem("View Logs", null, (_, _) => _applicationContext.ShowLogViewer()));
        menuStrip.Items.Add(logsMenuItem);
        var settingsMenuItem = new ToolStripMenuItem("Settings");
        settingsMenuItem.DropDownItems.Add(new ToolStripMenuItem("Open Settings", null, async (_, _) => await _applicationContext.EditSettingsAsync(this)));
        menuStrip.Items.Add(settingsMenuItem);
        MainMenuStrip = menuStrip;

        _connectionsListBox = new ListBox
        {
            Dock = DockStyle.Fill,
            IntegralHeight = false,
            DrawMode = DrawMode.OwnerDrawFixed,
            ItemHeight = 24,
            BorderStyle = BorderStyle.FixedSingle
        };
        _connectionsListBox.DrawItem += OnDrawConnectionItem;
        _connectionsListBox.SelectedIndexChanged += (_, _) =>
        {
            _applicationContext.SetLastSelectedConnection(GetSelectedConnectionId());
            PopulateGroupsAndPorts();
            UpdateButtons();
        };
        _connectionsListBox.DoubleClick += async (_, _) =>
        {
            if (GetSelectedConnectionId() is Guid connectionId)
            {
                await _applicationContext.EditConnectionAsync(connectionId, this);
            }
        };

        _groupsListView = new ListView
        {
            Dock = DockStyle.Fill,
            View = View.Details,
            FullRowSelect = true,
            MultiSelect = false,
            HideSelection = false,
            HeaderStyle = ColumnHeaderStyle.Nonclickable,
            OwnerDraw = true
        };
        _groupsListView.Columns.Add("Group", 160);
        _groupsListView.Columns.Add("Ports", 56);
        _groupsListView.Columns.Add("Port list", 180);
        _groupsListView.DrawColumnHeader += OnDrawListViewColumnHeader;
        _groupsListView.DrawItem += OnDrawListViewItem;
        _groupsListView.DrawSubItem += OnDrawGroupsSubItem;
        _groupsListView.ColumnWidthChanged += async (_, _) => await HandleGroupColumnWidthsChangedAsync();
        _groupsListView.ItemSelectionChanged += (_, _) =>
        {
            PopulatePorts();
            UpdateButtons();
        };
        _groupsListView.DoubleClick += async (_, _) =>
        {
            if (GetSelectedConnectionId() is Guid connectionId && GetSelectedGroupId() is Guid groupId)
            {
                await _applicationContext.EditGroupAsync(connectionId, groupId, this);
            }
        };

        _portsListView = new ListView
        {
            Dock = DockStyle.Fill,
            View = View.Details,
            FullRowSelect = true,
            MultiSelect = false,
            HideSelection = false,
            HeaderStyle = ColumnHeaderStyle.Nonclickable,
            OwnerDraw = true
        };
        _portsListView.Columns.Add("Local", 60);
        _portsListView.Columns.Add("Remote host", 110);
        _portsListView.Columns.Add("Remote port", 80);
        _portsListView.Columns.Add("Description", 150);
        _portsListView.DrawColumnHeader += OnDrawListViewColumnHeader;
        _portsListView.DrawItem += OnDrawListViewItem;
        _portsListView.DrawSubItem += OnDrawPortsSubItem;
        _portsListView.ColumnWidthChanged += async (_, _) => await HandlePortColumnWidthsChangedAsync();
        _portsListView.ItemSelectionChanged += (_, _) => UpdateButtons();
        _portsListView.DoubleClick += async (_, _) =>
        {
            if (GetSelectedConnectionId() is Guid connectionId && GetSelectedGroupId() is Guid groupId && GetSelectedPortId() is Guid portId)
            {
                await _applicationContext.EditPortAsync(connectionId, groupId, portId, this);
            }
        };

        _addConnectionButton = BuildButton("Add", async (_, _) => await _applicationContext.AddConnectionAsync(this));
        _editConnectionButton = BuildButton("Edit", async (_, _) =>
        {
            if (GetSelectedConnectionId() is Guid connectionId)
            {
                await _applicationContext.EditConnectionAsync(connectionId, this);
            }
        });
        _deleteConnectionButton = BuildButton("Delete", async (_, _) =>
        {
            if (GetSelectedConnectionId() is Guid connectionId)
            {
                await _applicationContext.DeleteConnectionAsync(connectionId, this);
            }
        });
        _connectButton = BuildButton("Connect", async (_, _) => await HandleConnectButtonClickAsync());

        _addGroupButton = BuildButton("Add", async (_, _) =>
        {
            if (GetSelectedConnectionId() is not Guid connectionId)
            {
                ShowSelectionMessage("Select a connection before adding a group.");
                return;
            }

            await _applicationContext.AddGroupAsync(connectionId, this);
        });
        _toggleGroupButton = BuildButton("Activate", async (_, _) => await HandleToggleGroupButtonClickAsync());
        _editGroupButton = BuildButton("Edit", async (_, _) =>
        {
            if (GetSelectedConnectionId() is Guid connectionId && GetSelectedGroupId() is Guid groupId)
            {
                await _applicationContext.EditGroupAsync(connectionId, groupId, this);
            }
        });
        _deleteGroupButton = BuildButton("Delete", async (_, _) =>
        {
            if (GetSelectedConnectionId() is Guid connectionId && GetSelectedGroupId() is Guid groupId)
            {
                await _applicationContext.DeleteGroupAsync(connectionId, groupId, this);
            }
        });

        _addPortButton = BuildButton("Add", async (_, _) =>
        {
            if (GetSelectedConnectionId() is not Guid connectionId)
            {
                ShowSelectionMessage("Select a connection before adding a port.");
                return;
            }

            if (GetSelectedGroupId() is not Guid groupId)
            {
                ShowSelectionMessage("Select a group before adding a port.");
                return;
            }

            await _applicationContext.AddPortAsync(connectionId, groupId, this);
        });
        _togglePortButton = BuildButton("Activate", async (_, _) => await HandleTogglePortButtonClickAsync());
        _editPortButton = BuildButton("Edit", async (_, _) =>
        {
            if (GetSelectedConnectionId() is Guid connectionId && GetSelectedGroupId() is Guid groupId && GetSelectedPortId() is Guid portId)
            {
                await _applicationContext.EditPortAsync(connectionId, groupId, portId, this);
            }
        });
        _deletePortButton = BuildButton("Delete", async (_, _) =>
        {
            if (GetSelectedConnectionId() is Guid connectionId && GetSelectedGroupId() is Guid groupId && GetSelectedPortId() is Guid portId)
            {
                await _applicationContext.DeletePortAsync(connectionId, groupId, portId, this);
            }
        });

        var rootLayout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 3,
            RowCount = 1,
            Padding = new Padding(8)
        };
        rootLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 31));
        rootLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 31));
        rootLayout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 38));
        rootLayout.Controls.Add(BuildPanel("Connections", _connectionsListBox, _addConnectionButton, _editConnectionButton, _deleteConnectionButton, _connectButton), 0, 0);
        rootLayout.Controls.Add(BuildPanel("Groups", _groupsListView, _addGroupButton, _toggleGroupButton, _editGroupButton, _deleteGroupButton), 1, 0);
        rootLayout.Controls.Add(BuildPanel("Ports", _portsListView, _addPortButton, _togglePortButton, _editPortButton, _deletePortButton), 2, 0);

        var statusStrip = new StatusStrip();
        _statusLabel = new ToolStripStatusLabel("Idle")
        {
            Spring = true,
            TextAlign = ContentAlignment.MiddleLeft
        };
        _pathLabel = new ToolStripStatusLabel(AppPaths.GetConfigurationFilePath());
        statusStrip.Items.Add(_statusLabel);
        statusStrip.Items.Add(_pathLabel);

        Controls.Add(rootLayout);
        Controls.Add(statusStrip);
        Controls.Add(menuStrip);
        MainMenuStrip = menuStrip;

        SetStyle(ControlStyles.OptimizedDoubleBuffer | ControlStyles.AllPaintingInWmPaint, true);

        _connectionIndicatorTimer = new System.Windows.Forms.Timer
        {
            Interval = 120
        };
        _connectionIndicatorTimer.Tick += (_, _) =>
        {
            _connectionIndicatorFrame = (_connectionIndicatorFrame + 1) % 12;
            _connectionsListBox.Invalidate();
        };

        Disposed += (_, _) => _connectionIndicatorTimer.Dispose();

        AppTheme.Apply(this);

        FormClosing += (_, eventArgs) => _applicationContext.HandleMainFormClosing(eventArgs);
    }

    public void ApplyDefaultWindowSize(AppSettings settings)
    {
        var requestedSize = new Size(settings.WindowWidth, settings.WindowHeight);
        var workingArea = Screen.PrimaryScreen?.WorkingArea ?? new Rectangle(0, 0, requestedSize.Width, requestedSize.Height);
        var maxWidth = Math.Max(320, workingArea.Width - 32);
        var maxHeight = Math.Max(240, workingArea.Height - 32);
        var width = Math.Min(Math.Max(760, requestedSize.Width), maxWidth);
        var height = Math.Min(Math.Max(520, requestedSize.Height), maxHeight);
        Size = new Size(width, height);
    }

    public void Render(AppConfiguration configuration, TunnelRuntimeSnapshot runtimeSnapshot)
    {
        _configuration = configuration;
        _runtimeSnapshot = runtimeSnapshot;

        PopulateConnections();
        PopulateGroupsAndPorts();
        UpdateConnectionIndicatorAnimation();
        ApplyConfiguredColumnWidths(configuration.Settings);
        UpdateButtons();
        UpdateStatus();
    }

    private void PopulateConnections()
    {
        var currentConnectionId = GetSelectedConnectionId() ?? _configuration.LastSelectedConnectionId;

        _connectionsListBox.BeginUpdate();
        _connectionsListBox.Items.Clear();

        foreach (var connection in _configuration.Connections.OrderBy(static connection => connection.Name, StringComparer.OrdinalIgnoreCase))
        {
            _connectionsListBox.Items.Add(new ConnectionListItem(
                connection.Id,
                $"{connection.Name} ({connection.GetEndpointLabel()})",
                _runtimeSnapshot.IsActive && _runtimeSnapshot.ConnectionId == connection.Id,
                _applicationContext.IsConnectionTransitioning(connection.Id),
                (_runtimeSnapshot.IsAwaitingAuthentication && _runtimeSnapshot.ConnectionId == connection.Id)
                    || (_runtimeSnapshot.IsFaulted && _runtimeSnapshot.FaultedConnectionId == connection.Id)));
        }

        if (_connectionsListBox.Items.Count > 0)
        {
            var selectedItem = _connectionsListBox.Items
                .Cast<ConnectionListItem>()
                .FirstOrDefault(item => item.Id == currentConnectionId)
                ?? (ConnectionListItem)_connectionsListBox.Items[0];
            _connectionsListBox.SelectedItem = selectedItem;
        }

        _connectionsListBox.EndUpdate();
    }

    private void PopulateGroupsAndPorts()
    {
        PopulateGroups();
        PopulatePorts();
    }

    private void PopulateGroups()
    {
        var currentGroupId = GetSelectedGroupId();
        _groupsListView.BeginUpdate();
        _groupsListView.Items.Clear();

        var connection = GetSelectedConnection();

        if (connection is not null)
        {
            foreach (var group in connection.Groups.OrderBy(static group => group.Name, StringComparer.OrdinalIgnoreCase))
            {
                var activePorts = group.Ports
                    .Where(static port => port.IsEnabled)
                    .OrderBy(static port => port.LocalPort)
                    .ToList();
                var portSummary = activePorts.Count == 0
                    ? "No active ports"
                    : string.Join(", ", activePorts.Select(static port => port.LocalPort.ToString()));

                var item = new ListViewItem(group.Name)
                {
                    Tag = group.Id,
                    Checked = group.IsEnabled
                };
                item.SubItems.Add(activePorts.Count.ToString());
                item.SubItems.Add(portSummary);
                _groupsListView.Items.Add(item);
            }

            if (_groupsListView.Items.Count > 0)
            {
                var selectedItem = _groupsListView.Items
                    .Cast<ListViewItem>()
                    .FirstOrDefault(item => item.Tag is Guid groupId && groupId == currentGroupId)
                    ?? _groupsListView.Items[0];
                selectedItem.Selected = true;
            }
        }

        _groupsListView.EndUpdate();
    }

    private void PopulatePorts()
    {
        var currentPortId = GetSelectedPortId();
        _portsListView.BeginUpdate();
        _portsListView.Items.Clear();

        var group = GetSelectedGroup();

        if (group is not null)
        {
            foreach (var port in group.Ports.OrderBy(static port => port.LocalPort))
            {
                var item = new ListViewItem(port.LocalPort.ToString())
                {
                    Tag = port.Id,
                    Checked = port.IsEnabled
                };
                item.SubItems.Add(port.RemoteHost);
                item.SubItems.Add(port.RemotePort.ToString());
                item.SubItems.Add(string.IsNullOrWhiteSpace(port.Description) ? "-" : port.Description);
                _portsListView.Items.Add(item);
            }

            if (_portsListView.Items.Count > 0)
            {
                var selectedItem = _portsListView.Items
                    .Cast<ListViewItem>()
                    .FirstOrDefault(item => item.Tag is Guid portId && portId == currentPortId)
                    ?? _portsListView.Items[0];
                selectedItem.Selected = true;
            }
        }

        _portsListView.EndUpdate();
    }

    private void UpdateButtons()
    {
        var hasConnection = GetSelectedConnection() is not null;
        var hasGroup = GetSelectedGroup() is not null;
        var hasPort = GetSelectedPortId() is not null;

        _editConnectionButton.Enabled = hasConnection;
        _deleteConnectionButton.Enabled = hasConnection;
        _connectButton.Enabled = true;

        _addGroupButton.Enabled = true;
        _toggleGroupButton.Enabled = hasGroup;
        _editGroupButton.Enabled = hasGroup;
        _deleteGroupButton.Enabled = hasGroup;

        _addPortButton.Enabled = true;
        _togglePortButton.Enabled = hasPort;
        _editPortButton.Enabled = hasPort;
        _deletePortButton.Enabled = hasPort;

        _connectButton.Text = _runtimeSnapshot.IsActive || _runtimeSnapshot.IsConnecting ? "Disconnect" : "Connect";
        _toggleGroupButton.Text = GetSelectedGroup()?.IsEnabled == true ? "Deactivate" : "Activate";
        _togglePortButton.Text = GetSelectedPort()?.IsEnabled == true ? "Deactivate" : "Activate";
    }

    private void UpdateStatus()
    {
        var runtimeText = _runtimeSnapshot.StatusText;
        var statusConnectionId = _runtimeSnapshot.ConnectionId ?? _runtimeSnapshot.FaultedConnectionId;

        if (statusConnectionId is Guid connectionId)
        {
            var connection = _configuration.Connections.FirstOrDefault(currentConnection => currentConnection.Id == connectionId);

            if (connection is not null)
            {
                runtimeText = $"{runtimeText} | {connection.Name}";
            }
        }

        _statusLabel.Text = runtimeText;
    }

    private SshConnectionProfile? GetSelectedConnection()
    {
        return GetSelectedConnectionId() is Guid connectionId
            ? _configuration.Connections.FirstOrDefault(connection => connection.Id == connectionId)
            : null;
    }

    private Guid? GetSelectedConnectionId()
    {
        return _connectionsListBox.SelectedItem is ConnectionListItem connectionListItem
            ? connectionListItem.Id
            : null;
    }

    private TunnelGroup? GetSelectedGroup()
    {
        var connection = GetSelectedConnection();
        return connection is null || GetSelectedGroupId() is not Guid groupId
            ? null
            : connection.Groups.FirstOrDefault(group => group.Id == groupId);
    }

    private TunnelPort? GetSelectedPort()
    {
        var group = GetSelectedGroup();
        return group is null || GetSelectedPortId() is not Guid portId
            ? null
            : group.Ports.FirstOrDefault(port => port.Id == portId);
    }

    private Guid? GetSelectedGroupId()
    {
        return _groupsListView.SelectedItems.Count == 0 || _groupsListView.SelectedItems[0].Tag is not Guid groupId
            ? null
            : groupId;
    }

    private Guid? GetSelectedPortId()
    {
        return _portsListView.SelectedItems.Count == 0 || _portsListView.SelectedItems[0].Tag is not Guid portId
            ? null
            : portId;
    }

    private static Button BuildButton(string text, EventHandler onClick)
    {
        var button = new Button
        {
            AutoSize = true,
            Text = text,
            Margin = new Padding(0, 0, 6, 6),
            Padding = new Padding(8, 4, 8, 4)
        };
        button.Click += onClick;
        return button;
    }

    private static Control BuildPanel(string title, Control body, params Button[] buttons)
    {
        var panel = new Panel
        {
            Dock = DockStyle.Fill,
            Padding = new Padding(6)
        };

        var titleLabel = new Label
        {
            Dock = DockStyle.Top,
            AutoSize = true,
            Text = title,
            Font = new Font(SystemFonts.MessageBoxFont!, FontStyle.Bold),
            Padding = new Padding(0, 0, 0, 6)
        };

        var buttonPanel = new FlowLayoutPanel
        {
            Dock = DockStyle.Top,
            AutoSize = true,
            WrapContents = true,
            Padding = new Padding(0, 0, 0, 6)
        };

        foreach (var button in buttons)
        {
            buttonPanel.Controls.Add(button);
        }

        body.Dock = DockStyle.Fill;
        panel.Controls.Add(body);
        panel.Controls.Add(buttonPanel);
        panel.Controls.Add(titleLabel);
        return panel;
    }

    private void OnDrawConnectionItem(object? sender, DrawItemEventArgs eventArgs)
    {
        eventArgs.DrawBackground();

        if (eventArgs.Index < 0 || eventArgs.Index >= _connectionsListBox.Items.Count)
        {
            return;
        }

        var item = (ConnectionListItem)_connectionsListBox.Items[eventArgs.Index];
        var isSelected = (eventArgs.State & DrawItemState.Selected) == DrawItemState.Selected;
        var backgroundColor = isSelected ? AppTheme.SelectionColor : AppTheme.SurfaceColor;

        using var backgroundBrush = new SolidBrush(backgroundColor);
        eventArgs.Graphics.FillRectangle(backgroundBrush, eventArgs.Bounds);

        var indicatorBounds = new Rectangle(
            eventArgs.Bounds.Left + 8,
            eventArgs.Bounds.Top + ((eventArgs.Bounds.Height - 12) / 2),
            12,
            12);

        DrawIndicator(
            eventArgs.Graphics,
            indicatorBounds,
            item.IsAttention
                ? IndicatorState.Attention
                : item.IsLoading
                    ? IndicatorState.Loading
                    : item.IsActive
                        ? IndicatorState.Active
                        : IndicatorState.Inactive);

        var textBounds = new Rectangle(
            indicatorBounds.Right + 8,
            eventArgs.Bounds.Top,
            eventArgs.Bounds.Width - indicatorBounds.Right - 12,
            eventArgs.Bounds.Height);

        TextRenderer.DrawText(
            eventArgs.Graphics,
            item.DisplayText,
            Font!,
            textBounds,
            AppTheme.ForegroundColor,
            TextFormatFlags.Left | TextFormatFlags.VerticalCenter | TextFormatFlags.EndEllipsis | TextFormatFlags.NoPrefix);

        eventArgs.DrawFocusRectangle();
    }

    private void OnDrawListViewItem(object? sender, DrawListViewItemEventArgs eventArgs)
    {
    }

    private void OnDrawListViewColumnHeader(object? sender, DrawListViewColumnHeaderEventArgs eventArgs)
    {
        var headerText = eventArgs.Header?.Text ?? string.Empty;
        using var backgroundBrush = new SolidBrush(AppTheme.HeaderColor);
        using var borderPen = new Pen(AppTheme.BorderColor);
        eventArgs.Graphics.FillRectangle(backgroundBrush, eventArgs.Bounds);
        eventArgs.Graphics.DrawRectangle(borderPen, eventArgs.Bounds.Left, eventArgs.Bounds.Top, eventArgs.Bounds.Width - 1, eventArgs.Bounds.Height - 1);

        var textBounds = Rectangle.Inflate(eventArgs.Bounds, -8, 0);
        TextRenderer.DrawText(
            eventArgs.Graphics,
            headerText,
            Font!,
            textBounds,
            AppTheme.ForegroundColor,
            TextFormatFlags.Left | TextFormatFlags.VerticalCenter | TextFormatFlags.EndEllipsis | TextFormatFlags.NoPrefix);
    }

    private void OnDrawGroupsSubItem(object? sender, DrawListViewSubItemEventArgs eventArgs)
    {
        DrawListViewSubItem(
            eventArgs,
            indicatorState: eventArgs.ColumnIndex == 0
                ? eventArgs.Item?.Checked == true ? IndicatorState.Active : IndicatorState.Inactive
                : null,
            text: eventArgs.SubItem?.Text ?? string.Empty);
    }

    private void OnDrawPortsSubItem(object? sender, DrawListViewSubItemEventArgs eventArgs)
    {
        DrawListViewSubItem(
            eventArgs,
            indicatorState: eventArgs.ColumnIndex == 0
                ? eventArgs.Item?.Checked == true ? IndicatorState.Active : IndicatorState.Inactive
                : null,
            text: eventArgs.SubItem?.Text ?? string.Empty);
    }

    private void DrawListViewSubItem(DrawListViewSubItemEventArgs eventArgs, IndicatorState? indicatorState, string text)
    {
        var isSelected = eventArgs.Item?.Selected ?? false;
        using var backgroundBrush = new SolidBrush(isSelected ? AppTheme.SelectionColor : AppTheme.SurfaceColor);
        using var borderPen = new Pen(AppTheme.BorderColor);

        eventArgs.Graphics.FillRectangle(backgroundBrush, eventArgs.Bounds);
        eventArgs.Graphics.DrawLine(borderPen, eventArgs.Bounds.Left, eventArgs.Bounds.Bottom - 1, eventArgs.Bounds.Right, eventArgs.Bounds.Bottom - 1);
        eventArgs.Graphics.DrawLine(borderPen, eventArgs.Bounds.Right - 1, eventArgs.Bounds.Top, eventArgs.Bounds.Right - 1, eventArgs.Bounds.Bottom);

        var textBounds = Rectangle.Inflate(eventArgs.Bounds, -8, 0);

        if (indicatorState is IndicatorState state)
        {
            var indicatorBounds = new Rectangle(
                eventArgs.Bounds.Left + 6,
                eventArgs.Bounds.Top + ((eventArgs.Bounds.Height - 10) / 2),
                10,
                10);
            DrawIndicator(eventArgs.Graphics, indicatorBounds, state);
            textBounds = new Rectangle(indicatorBounds.Right + 8, eventArgs.Bounds.Top, eventArgs.Bounds.Width - (indicatorBounds.Right - eventArgs.Bounds.Left) - 14, eventArgs.Bounds.Height);
        }

        TextRenderer.DrawText(
            eventArgs.Graphics,
            text,
            Font!,
            textBounds,
            AppTheme.ForegroundColor,
            TextFormatFlags.Left | TextFormatFlags.VerticalCenter | TextFormatFlags.EndEllipsis | TextFormatFlags.NoPrefix);
    }

    private void DrawIndicator(Graphics graphics, Rectangle bounds, IndicatorState state)
    {
        graphics.SmoothingMode = SmoothingMode.AntiAlias;

        if (state == IndicatorState.Loading)
        {
            using var trackPen = new Pen(AppTheme.ConnectionDisconnectedColor, 2f);
            using var arcPen = new Pen(AppTheme.ConnectionConnectedColor, 2.5f)
            {
                StartCap = LineCap.Round,
                EndCap = LineCap.Round
            };
            graphics.DrawEllipse(trackPen, bounds);
            graphics.DrawArc(arcPen, bounds, _connectionIndicatorFrame * 30, 220);
            return;
        }

        using var indicatorBrush = new SolidBrush(state switch
        {
            IndicatorState.Active => AppTheme.ConnectionConnectedColor,
            IndicatorState.Attention => AppTheme.ConnectionAttentionColor,
            _ => AppTheme.ConnectionDisconnectedColor
        });
        graphics.FillEllipse(indicatorBrush, bounds);
    }

    private async Task HandleConnectButtonClickAsync()
    {
        if (_runtimeSnapshot.IsActive || _runtimeSnapshot.IsConnecting)
        {
            await _applicationContext.DeactivateConnectionAsync();
            return;
        }

        if (_configuration.Connections.Count == 0)
        {
            ShowSelectionMessage("Create a connection before connecting.");
            return;
        }

        Guid? connectionId = GetSelectedConnectionId();
        IReadOnlyCollection<Guid> groupIds;

        if (connectionId is null)
        {
            var chosenConnectionId = ConnectionPickerForm.ShowDialog(this, _configuration.Connections, _configuration.LastSelectedConnectionId);

            if (chosenConnectionId is not Guid chosenId)
            {
                return;
            }

            connectionId = chosenId;
            _applicationContext.SetLastSelectedConnection(chosenId);
            groupIds = _applicationContext.GetDraftSelection(chosenId);
        }
        else
        {
            groupIds = _applicationContext.GetDraftSelection(connectionId.Value);
        }

        await _applicationContext.ActivateConnectionAsync(connectionId.Value, groupIds, this);
    }

    private async Task HandleToggleGroupButtonClickAsync()
    {
        if (GetSelectedConnectionId() is Guid connectionId && GetSelectedGroupId() is Guid groupId && GetSelectedGroup() is TunnelGroup group)
        {
            await _applicationContext.ToggleActiveGroupAsync(connectionId, groupId, !group.IsEnabled, this);
        }
    }

    private async Task HandleTogglePortButtonClickAsync()
    {
        if (GetSelectedConnectionId() is Guid connectionId &&
            GetSelectedGroupId() is Guid groupId &&
            GetSelectedPortId() is Guid portId &&
            GetSelectedPort() is TunnelPort port)
        {
            await _applicationContext.ToggleActivePortAsync(connectionId, groupId, portId, !port.IsEnabled, this);
        }
    }

    private void UpdateConnectionIndicatorAnimation()
    {
        var shouldAnimate = _connectionsListBox.Items
            .Cast<ConnectionListItem>()
            .Any(static item => item.IsLoading);

        if (shouldAnimate)
        {
            if (!_connectionIndicatorTimer.Enabled)
            {
                _connectionIndicatorTimer.Start();
            }

            return;
        }

        if (_connectionIndicatorTimer.Enabled)
        {
            _connectionIndicatorTimer.Stop();
            _connectionIndicatorFrame = 0;
            _connectionsListBox.Invalidate();
        }
    }

    private void ApplyConfiguredColumnWidths(AppSettings settings)
    {
        ApplyColumnWidths(_groupsListView, settings.GroupColumnWidths);
        ApplyColumnWidths(_portsListView, settings.PortColumnWidths);
    }

    private async Task HandleGroupColumnWidthsChangedAsync()
    {
        if (_suppressColumnWidthPersistence)
        {
            return;
        }

        await _applicationContext.SaveGroupColumnWidthsAsync(GetColumnWidths(_groupsListView)).ConfigureAwait(true);
    }

    private async Task HandlePortColumnWidthsChangedAsync()
    {
        if (_suppressColumnWidthPersistence)
        {
            return;
        }

        await _applicationContext.SavePortColumnWidthsAsync(GetColumnWidths(_portsListView)).ConfigureAwait(true);
    }

    private void ApplyColumnWidths(ListView listView, IReadOnlyList<int> widths)
    {
        if (listView.Columns.Count != widths.Count)
        {
            return;
        }

        _suppressColumnWidthPersistence = true;

        try
        {
            for (var index = 0; index < widths.Count; index++)
            {
                if (listView.Columns[index].Width == widths[index])
                {
                    continue;
                }

                listView.Columns[index].Width = widths[index];
            }
        }
        finally
        {
            _suppressColumnWidthPersistence = false;
        }
    }

    private static List<int> GetColumnWidths(ListView listView)
    {
        return listView.Columns.Cast<ColumnHeader>().Select(static column => column.Width).ToList();
    }

    private void ShowSelectionMessage(string message)
    {
        MessageBox.Show(this, message, "Selection required", MessageBoxButtons.OK, MessageBoxIcon.Information);
    }

    private sealed class ConnectionListItem
    {
        public ConnectionListItem(Guid id, string displayText, bool isActive, bool isLoading, bool isAttention)
        {
            Id = id;
            DisplayText = displayText;
            IsActive = isActive;
            IsLoading = isLoading;
            IsAttention = isAttention;
        }

        public Guid Id { get; }

        public string DisplayText { get; }

        public bool IsActive { get; }

        public bool IsLoading { get; }

        public bool IsAttention { get; }

        public override string ToString()
        {
            return DisplayText;
        }
    }

    private enum IndicatorState
    {
        Inactive,
        Active,
        Loading,
        Attention
    }
}