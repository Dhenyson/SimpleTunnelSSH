using SimpleTunnelSSH.Core.Models;

namespace SimpleTunnelSSH.App;

public sealed class SettingsForm : Form
{
    private readonly CheckBox _launchAtStartupCheckBox;
    private readonly CheckBox _startMinimizedCheckBox;
    private readonly NumericUpDown _windowWidthNumericUpDown;
    private readonly NumericUpDown _windowHeightNumericUpDown;

    private SettingsForm(AppSettings settings)
    {
        Text = "Settings";
        StartPosition = FormStartPosition.CenterParent;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = false;
        Width = 480;
        AutoSize = true;
        AutoSizeMode = AutoSizeMode.GrowAndShrink;

        _launchAtStartupCheckBox = new CheckBox
        {
            AutoSize = true,
            Text = "Launch with Windows",
            Checked = settings.LaunchAtWindowsStartup,
            Margin = new Padding(0, 4, 0, 4)
        };

        _startMinimizedCheckBox = new CheckBox
        {
            AutoSize = true,
            Text = "Start minimized in the tray",
            Checked = settings.StartMinimizedToTray,
            Margin = new Padding(0, 4, 0, 4)
        };

        _windowWidthNumericUpDown = new NumericUpDown
        {
            Minimum = 760,
            Maximum = 4096,
            Value = settings.WindowWidth,
            Dock = DockStyle.Fill
        };

        _windowHeightNumericUpDown = new NumericUpDown
        {
            Minimum = 520,
            Maximum = 2160,
            Value = settings.WindowHeight,
            Dock = DockStyle.Fill
        };

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            Padding = new Padding(16),
            ColumnCount = 2,
            AutoSize = true
        };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));

        var introLabel = new Label
        {
            AutoSize = true,
            Text = "General preferences for the main window and startup behavior.",
            Margin = new Padding(0, 0, 0, 10)
        };
        layout.Controls.Add(introLabel, 0, 0);
        layout.SetColumnSpan(introLabel, 2);

        layout.Controls.Add(new Label { AutoSize = true, Text = "Default window width", Padding = new Padding(0, 6, 12, 0) }, 0, 1);
        layout.Controls.Add(_windowWidthNumericUpDown, 1, 1);
        layout.Controls.Add(new Label { AutoSize = true, Text = "Default window height", Padding = new Padding(0, 6, 12, 0) }, 0, 2);
        layout.Controls.Add(_windowHeightNumericUpDown, 1, 2);
        layout.Controls.Add(_launchAtStartupCheckBox, 0, 3);
        layout.SetColumnSpan(_launchAtStartupCheckBox, 2);
        layout.Controls.Add(_startMinimizedCheckBox, 0, 4);
        layout.SetColumnSpan(_startMinimizedCheckBox, 2);

        var saveButton = new Button { Text = "Save", DialogResult = DialogResult.OK, AutoSize = true };
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

    public static AppSettings? ShowDialog(IWin32Window owner, AppSettings settings)
    {
        using var form = new SettingsForm(settings.DeepClone());

        if (form.ShowDialog(owner) != DialogResult.OK)
        {
            return null;
        }

        return new AppSettings
        {
            LaunchAtWindowsStartup = form._launchAtStartupCheckBox.Checked,
            StartMinimizedToTray = form._startMinimizedCheckBox.Checked,
            WindowWidth = (int)form._windowWidthNumericUpDown.Value,
            WindowHeight = (int)form._windowHeightNumericUpDown.Value,
            GroupColumnWidths = [.. settings.GroupColumnWidths],
            PortColumnWidths = [.. settings.PortColumnWidths],
            LogColumnWidths = [.. settings.LogColumnWidths]
        };
    }
}
