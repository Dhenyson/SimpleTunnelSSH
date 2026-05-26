using SimpleTunnelSSH.Core.Models;

namespace SimpleTunnelSSH.App;

public sealed class ConnectionEditorForm : Form
{
    private readonly TextBox _nameTextBox;
    private readonly TextBox _hostTextBox;
    private readonly NumericUpDown _portNumericUpDown;
    private readonly TextBox _userTextBox;
    private readonly TextBox _identityFileTextBox;

    private ConnectionEditorForm(SshConnectionProfile? connection)
    {
        Text = connection is null ? "Add connection" : "Edit connection";
        StartPosition = FormStartPosition.CenterParent;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = false;
        Width = 520;
        AutoSize = true;
        AutoSizeMode = AutoSizeMode.GrowAndShrink;

        _nameTextBox = new TextBox { Text = connection?.Name ?? string.Empty, Dock = DockStyle.Fill };
        _hostTextBox = new TextBox { Text = connection?.Host ?? string.Empty, Dock = DockStyle.Fill };
        _portNumericUpDown = new NumericUpDown { Minimum = 1, Maximum = 65535, Value = connection?.Port ?? 22, Dock = DockStyle.Fill };
        _userTextBox = new TextBox { Text = connection?.UserName ?? string.Empty, Dock = DockStyle.Fill };
        _identityFileTextBox = new TextBox { Text = connection?.IdentityFilePath ?? string.Empty, Dock = DockStyle.Fill };

        var browseButton = new Button { Text = "Browse", AutoSize = true };
        browseButton.Click += (_, _) => BrowseIdentityFile();

        var identityPanel = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, AutoSize = true };
        identityPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        identityPanel.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        identityPanel.Controls.Add(_identityFileTextBox, 0, 0);
        identityPanel.Controls.Add(browseButton, 1, 0);

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            Padding = new Padding(16),
            ColumnCount = 2,
            AutoSize = true
        };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));

        AddRow(layout, "Name", _nameTextBox, 0);
        AddRow(layout, "Host", _hostTextBox, 1);
        AddRow(layout, "Port", _portNumericUpDown, 2);
        AddRow(layout, "User", _userTextBox, 3);
        AddRow(layout, "Identity file", identityPanel, 4);

        var okButton = new Button { Text = "Save", DialogResult = DialogResult.OK, AutoSize = true };
        okButton.Click += (_, eventArgs) =>
        {
            if (!ValidateInput())
            {
                DialogResult = DialogResult.None;
            }
        };

        var cancelButton = new Button { Text = "Cancel", DialogResult = DialogResult.Cancel, AutoSize = true };
        var buttonsPanel = new FlowLayoutPanel
        {
            Dock = DockStyle.Bottom,
            FlowDirection = FlowDirection.RightToLeft,
            AutoSize = true,
            Padding = new Padding(16, 0, 16, 16)
        };
        buttonsPanel.Controls.Add(cancelButton);
        buttonsPanel.Controls.Add(okButton);

        Controls.Add(layout);
        Controls.Add(buttonsPanel);

        AcceptButton = okButton;
        CancelButton = cancelButton;
        AppTheme.Apply(this);
    }

    public static SshConnectionProfile? ShowDialog(IWin32Window owner, SshConnectionProfile? connection)
    {
        using var form = new ConnectionEditorForm(connection?.DeepClone());

        if (form.ShowDialog(owner) != DialogResult.OK)
        {
            return null;
        }

        var result = connection?.DeepClone() ?? new SshConnectionProfile();
        result.Name = form._nameTextBox.Text.Trim();
        result.Host = form._hostTextBox.Text.Trim();
        result.Port = (int)form._portNumericUpDown.Value;
        result.UserName = form._userTextBox.Text.Trim();
        result.IdentityFilePath = form._identityFileTextBox.Text.Trim();
        return result;
    }

    private void BrowseIdentityFile()
    {
        using var dialog = new OpenFileDialog
        {
            Filter = "Private key files (*.pem;*.ppk;*.*)|*.pem;*.ppk;*.*",
            RestoreDirectory = true,
            CheckFileExists = true
        };

        if (dialog.ShowDialog(this) == DialogResult.OK)
        {
            _identityFileTextBox.Text = dialog.FileName;
        }
    }

    private bool ValidateInput()
    {
        if (string.IsNullOrWhiteSpace(_nameTextBox.Text) || string.IsNullOrWhiteSpace(_hostTextBox.Text) || string.IsNullOrWhiteSpace(_userTextBox.Text))
        {
            MessageBox.Show(this, "Name, host and user are required.", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return false;
        }

        return true;
    }

    private static void AddRow(TableLayoutPanel layout, string labelText, Control control, int rowIndex)
    {
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.Controls.Add(new Label
        {
            Text = labelText,
            AutoSize = true,
            Padding = new Padding(0, 6, 12, 0)
        }, 0, rowIndex);
        layout.Controls.Add(control, 1, rowIndex);
    }
}