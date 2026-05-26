namespace SimpleTunnelSSH.App;

public sealed class AskPassPromptForm : Form
{
    private readonly TextBox _secretTextBox;

    private AskPassPromptForm(string prompt)
    {
        Text = "SSH Authentication";
        StartPosition = FormStartPosition.CenterScreen;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = false;
        TopMost = true;
        Width = 520;
        AutoSize = true;
        AutoSizeMode = AutoSizeMode.GrowAndShrink;

        var promptLabel = new Label
        {
            AutoSize = true,
            MaximumSize = new Size(440, 0),
            Text = string.IsNullOrWhiteSpace(prompt)
                ? "SSH authentication is required."
                : prompt.Trim()
        };

        _secretTextBox = new TextBox
        {
            Dock = DockStyle.Fill,
            UseSystemPasswordChar = true
        };

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            Padding = new Padding(16),
            ColumnCount = 1,
            AutoSize = true
        };
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.Controls.Add(promptLabel, 0, 0);
        layout.Controls.Add(_secretTextBox, 0, 1);

        var okButton = new Button
        {
            Text = "Continue",
            DialogResult = DialogResult.OK,
            AutoSize = true
        };

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
        buttonsPanel.Controls.Add(okButton);

        Controls.Add(layout);
        Controls.Add(buttonsPanel);

        AcceptButton = okButton;
        CancelButton = cancelButton;

        Shown += (_, _) =>
        {
            _secretTextBox.SelectAll();
            _secretTextBox.Focus();
        };

        AppTheme.Apply(this);
    }

    public static string? ShowPrompt(string prompt)
    {
        using var form = new AskPassPromptForm(prompt);
        return form.ShowDialog() == DialogResult.OK
            ? form._secretTextBox.Text
            : null;
    }
}