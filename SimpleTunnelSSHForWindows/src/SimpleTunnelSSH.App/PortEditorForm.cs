using SimpleTunnelSSH.Core.Models;

namespace SimpleTunnelSSH.App;

public sealed class PortEditorForm : Form
{
    private readonly TextBox _descriptionTextBox;
    private readonly NumericUpDown _localPortNumericUpDown;
    private readonly TextBox _remoteHostTextBox;
    private readonly NumericUpDown _remotePortNumericUpDown;

    private PortEditorForm(TunnelPort? port)
    {
        Text = port is null ? "Add port" : "Edit port";
        StartPosition = FormStartPosition.CenterParent;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = false;
        Width = 480;
        AutoSize = true;
        AutoSizeMode = AutoSizeMode.GrowAndShrink;

        _descriptionTextBox = new TextBox { Text = port?.Description ?? string.Empty, Dock = DockStyle.Fill };
        _localPortNumericUpDown = new NumericUpDown { Minimum = 1, Maximum = 65535, Value = port?.LocalPort ?? 3000, Dock = DockStyle.Fill };
        _remoteHostTextBox = new TextBox { Text = port?.RemoteHost ?? "localhost", Dock = DockStyle.Fill };
        _remotePortNumericUpDown = new NumericUpDown { Minimum = 1, Maximum = 65535, Value = port?.RemotePort ?? port?.LocalPort ?? 3000, Dock = DockStyle.Fill };

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            Padding = new Padding(16),
            ColumnCount = 2,
            AutoSize = true
        };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        AddRow(layout, "Description", _descriptionTextBox, 0);
        AddRow(layout, "Local port", _localPortNumericUpDown, 1);
        AddRow(layout, "Remote host", _remoteHostTextBox, 2);
        AddRow(layout, "Remote port", _remotePortNumericUpDown, 3);

        var saveButton = new Button { Text = "Save", DialogResult = DialogResult.OK, AutoSize = true };
        saveButton.Click += (_, _) =>
        {
            if (string.IsNullOrWhiteSpace(_remoteHostTextBox.Text))
            {
                DialogResult = DialogResult.None;
                MessageBox.Show(this, "Remote host is required.", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
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
        buttonsPanel.Controls.Add(saveButton);

        Controls.Add(layout);
        Controls.Add(buttonsPanel);

        AcceptButton = saveButton;
        CancelButton = cancelButton;
        AppTheme.Apply(this);
    }

    public static TunnelPort? ShowDialog(IWin32Window owner, TunnelPort? port)
    {
        using var form = new PortEditorForm(port?.DeepClone());

        if (form.ShowDialog(owner) != DialogResult.OK)
        {
            return null;
        }

        var result = port?.DeepClone() ?? new TunnelPort();
        result.Description = form._descriptionTextBox.Text.Trim();
        result.LocalPort = (int)form._localPortNumericUpDown.Value;
        result.RemoteHost = form._remoteHostTextBox.Text.Trim();
        result.RemotePort = (int)form._remotePortNumericUpDown.Value;
        return result;
    }

    private static void AddRow(TableLayoutPanel layout, string labelText, Control control, int rowIndex)
    {
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.Controls.Add(new Label { Text = labelText, AutoSize = true, Padding = new Padding(0, 6, 12, 0) }, 0, rowIndex);
        layout.Controls.Add(control, 1, rowIndex);
    }
}