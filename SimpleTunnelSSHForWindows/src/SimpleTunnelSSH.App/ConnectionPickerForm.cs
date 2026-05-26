using SimpleTunnelSSH.Core.Models;

namespace SimpleTunnelSSH.App;

public sealed class ConnectionPickerForm : Form
{
    private readonly ListBox _connectionsListBox;
    private readonly Button _connectButton;

    private ConnectionPickerForm(IReadOnlyList<SshConnectionProfile> connections, Guid? preselectedConnectionId)
    {
        Text = "Choose connection";
        StartPosition = FormStartPosition.CenterParent;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = false;
        Width = 460;
        Height = 340;

        _connectionsListBox = new ListBox
        {
            Dock = DockStyle.Fill,
            IntegralHeight = false
        };

        foreach (var connection in connections.OrderBy(static connection => connection.Name, StringComparer.OrdinalIgnoreCase))
        {
            _connectionsListBox.Items.Add(new ConnectionPickerItem(
                connection.Id,
                connection.Name,
                connection.GetEndpointLabel(),
                connection.Groups.Count));
        }

        if (_connectionsListBox.Items.Count > 0)
        {
            var selectedItem = _connectionsListBox.Items
                .Cast<ConnectionPickerItem>()
                .FirstOrDefault(item => item.Id == preselectedConnectionId)
                ?? (ConnectionPickerItem)_connectionsListBox.Items[0];
            _connectionsListBox.SelectedItem = selectedItem;
        }

        var hintLabel = new Label
        {
            Dock = DockStyle.Top,
            AutoSize = true,
            Padding = new Padding(16, 16, 16, 10),
            Text = "Choose the SSH connection to activate. The saved group selection for that connection will be used."
        };

        _connectButton = new Button
        {
            Text = "Connect",
            DialogResult = DialogResult.OK,
            AutoSize = true,
            Enabled = _connectionsListBox.SelectedItem is ConnectionPickerItem
        };
        _connectionsListBox.SelectedIndexChanged += (_, _) => _connectButton.Enabled = _connectionsListBox.SelectedItem is ConnectionPickerItem;
        var cancelButton = new Button
        {
            Text = "Cancel",
            DialogResult = DialogResult.Cancel,
            AutoSize = true
        };
        var buttonsPanel = new FlowLayoutPanel
        {
            Dock = DockStyle.Bottom,
            FlowDirection = FlowDirection.RightToLeft,
            AutoSize = true,
            Padding = new Padding(16, 0, 16, 16)
        };
        buttonsPanel.Controls.Add(cancelButton);
        buttonsPanel.Controls.Add(_connectButton);

        Controls.Add(_connectionsListBox);
        Controls.Add(hintLabel);
        Controls.Add(buttonsPanel);

        AcceptButton = _connectButton;
        CancelButton = cancelButton;
        AppTheme.Apply(this);
    }

    public static Guid? ShowDialog(IWin32Window owner, IReadOnlyList<SshConnectionProfile> connections, Guid? preselectedConnectionId)
    {
        using var form = new ConnectionPickerForm(connections, preselectedConnectionId);

        if (form.ShowDialog(owner) != DialogResult.OK)
        {
            return null;
        }

        return form._connectionsListBox.SelectedItem is ConnectionPickerItem item ? item.Id : null;
    }

    private sealed class ConnectionPickerItem
    {
        public ConnectionPickerItem(Guid id, string name, string endpoint, int groupCount)
        {
            Id = id;
            DisplayText = $"{name} ({endpoint}) - {groupCount} group(s)";
        }

        public Guid Id { get; }

        public string DisplayText { get; }

        public override string ToString()
        {
            return DisplayText;
        }
    }
}