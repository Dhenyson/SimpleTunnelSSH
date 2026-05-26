using SimpleTunnelSSH.Core.Models;

namespace SimpleTunnelSSH.App;

public sealed class GroupEditorForm : Form
{
    private readonly TextBox _nameTextBox;

    private GroupEditorForm(TunnelGroup? group)
    {
        Text = group is null ? "Add group" : "Edit group";
        StartPosition = FormStartPosition.CenterParent;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        MinimizeBox = false;
        ShowInTaskbar = false;
        Width = 420;
        AutoSize = true;
        AutoSizeMode = AutoSizeMode.GrowAndShrink;

        _nameTextBox = new TextBox { Text = group?.Name ?? string.Empty, Dock = DockStyle.Fill };

        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            Padding = new Padding(16),
            ColumnCount = 2,
            AutoSize = true
        };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        layout.Controls.Add(new Label { Text = "Name", AutoSize = true, Padding = new Padding(0, 6, 12, 0) }, 0, 0);
        layout.Controls.Add(_nameTextBox, 1, 0);

        var saveButton = new Button { Text = "Save", DialogResult = DialogResult.OK, AutoSize = true };
        saveButton.Click += (_, _) =>
        {
            if (string.IsNullOrWhiteSpace(_nameTextBox.Text))
            {
                DialogResult = DialogResult.None;
                MessageBox.Show(this, "Group name is required.", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
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

    public static TunnelGroup? ShowDialog(IWin32Window owner, TunnelGroup? group)
    {
        using var form = new GroupEditorForm(group?.DeepClone());

        if (form.ShowDialog(owner) != DialogResult.OK)
        {
            return null;
        }

        var result = group?.DeepClone() ?? new TunnelGroup();
        result.Name = form._nameTextBox.Text.Trim();
        return result;
    }
}