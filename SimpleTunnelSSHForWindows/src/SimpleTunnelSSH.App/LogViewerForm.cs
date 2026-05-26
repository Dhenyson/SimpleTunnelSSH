using SimpleTunnelSSH.Core.Models;

namespace SimpleTunnelSSH.App;

public sealed class LogViewerForm : Form
{
    private readonly TunnelApplicationContext _applicationContext;
    private readonly ListView _logListView;
    private readonly Label _summaryLabel;
    private bool _suppressColumnWidthPersistence;

    public LogViewerForm(TunnelApplicationContext applicationContext)
    {
        _applicationContext = applicationContext;

        Text = "Recent Logs";
        StartPosition = FormStartPosition.CenterScreen;
        MinimumSize = new Size(760, 360);
        Size = new Size(920, 520);

        _summaryLabel = new Label
        {
            Dock = DockStyle.Top,
            AutoSize = true,
            Padding = new Padding(10, 10, 10, 6),
            Text = "Showing the most recent runtime events."
        };

        var copyButton = new Button { Text = "Copy Logs", AutoSize = true };
        copyButton.Click += (_, _) => CopyLogs();

        var clearButton = new Button { Text = "Clear Logs", AutoSize = true };
        clearButton.Click += (_, _) => ClearLogs();

        var actionsPanel = new FlowLayoutPanel
        {
            Dock = DockStyle.Top,
            AutoSize = true,
            Padding = new Padding(10, 0, 10, 8)
        };
        actionsPanel.Controls.Add(copyButton);
        actionsPanel.Controls.Add(clearButton);

        _logListView = new ListView
        {
            Dock = DockStyle.Fill,
            View = View.Details,
            FullRowSelect = true,
            MultiSelect = false,
            HideSelection = false,
            HeaderStyle = ColumnHeaderStyle.Nonclickable,
            OwnerDraw = false
        };
        _logListView.Columns.Add("Time", 110);
        _logListView.Columns.Add("Level", 90);
        _logListView.Columns.Add("Message", 660);
        _logListView.ColumnWidthChanged += async (_, _) => await HandleColumnWidthsChangedAsync();

        Controls.Add(_logListView);
        Controls.Add(actionsPanel);
        Controls.Add(_summaryLabel);

        AppTheme.Apply(this);
    }

    public void Render(IReadOnlyList<RuntimeLogEntry> entries, IReadOnlyList<int> columnWidths)
    {
        ApplyColumnWidths(columnWidths);

        _logListView.BeginUpdate();
        _logListView.Items.Clear();

        foreach (var entry in entries)
        {
            var item = new ListViewItem(entry.TimestampUtc.ToLocalTime().ToString("yyyy-MM-dd HH:mm:ss"));
            item.SubItems.Add(entry.Level);
            item.SubItems.Add(entry.Message);
            _logListView.Items.Add(item);
        }

        _logListView.EndUpdate();
        _summaryLabel.Text = $"Showing the latest {entries.Count} runtime event(s).";
        Text = $"Recent Logs ({entries.Count})";

        if (_logListView.Items.Count > 0)
        {
            _logListView.EnsureVisible(_logListView.Items.Count - 1);
        }
    }

    private async Task HandleColumnWidthsChangedAsync()
    {
        if (_suppressColumnWidthPersistence)
        {
            return;
        }

        await _applicationContext.SaveLogColumnWidthsAsync(_logListView.Columns.Cast<ColumnHeader>().Select(static column => column.Width).ToList()).ConfigureAwait(true);
    }

    private void ApplyColumnWidths(IReadOnlyList<int> widths)
    {
        if (_logListView.Columns.Count != widths.Count)
        {
            return;
        }

        _suppressColumnWidthPersistence = true;

        try
        {
            for (var index = 0; index < widths.Count; index++)
            {
                if (_logListView.Columns[index].Width == widths[index])
                {
                    continue;
                }

                _logListView.Columns[index].Width = widths[index];
            }
        }
        finally
        {
            _suppressColumnWidthPersistence = false;
        }
    }

    private void CopyLogs()
    {
        var logsText = _applicationContext.GetRuntimeLogsText();

        if (string.IsNullOrWhiteSpace(logsText))
        {
            MessageBox.Show(this, "There are no logs to copy.", "Recent Logs", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        Clipboard.SetText(logsText);
    }

    private void ClearLogs()
    {
        if (_logListView.Items.Count == 0)
        {
            return;
        }

        var result = MessageBox.Show(
            this,
            "Clear all runtime logs?",
            "Recent Logs",
            MessageBoxButtons.YesNo,
            MessageBoxIcon.Warning);

        if (result != DialogResult.Yes)
        {
            return;
        }

        _applicationContext.ClearRuntimeLogs();
    }
}