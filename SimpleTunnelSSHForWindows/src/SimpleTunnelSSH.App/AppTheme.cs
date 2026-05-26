using System.Runtime.InteropServices;

namespace SimpleTunnelSSH.App;

internal static class AppTheme
{
    public static Color BackgroundColor { get; } = Color.FromArgb(24, 27, 31);

    public static Color SurfaceColor { get; } = Color.FromArgb(31, 36, 42);

    public static Color SurfaceAltColor { get; } = Color.FromArgb(40, 46, 54);

    public static Color HeaderColor { get; } = Color.FromArgb(46, 53, 62);

    public static Color BorderColor { get; } = Color.FromArgb(72, 79, 89);

    public static Color ForegroundColor { get; } = Color.FromArgb(236, 239, 244);

    public static Color MutedForegroundColor { get; } = Color.FromArgb(162, 171, 182);

    public static Color SelectionColor { get; } = Color.FromArgb(48, 74, 120);

    public static Color ConnectionConnectedColor { get; } = Color.FromArgb(66, 191, 110);

    public static Color ConnectionDisconnectedColor { get; } = Color.FromArgb(124, 132, 143);

    public static Color ConnectionAttentionColor { get; } = Color.FromArgb(214, 76, 76);

    private static readonly ToolStripRenderer ToolStripRenderer = new DarkToolStripRenderer();

    public static void Apply(Form form)
    {
        form.BackColor = BackgroundColor;
        form.ForeColor = ForegroundColor;
        form.Icon = AppIconFactory.ApplicationIcon;
        form.HandleCreated -= OnFormHandleCreated;
        form.HandleCreated += OnFormHandleCreated;

        ApplyControlTree(form);
    }

    public static void ApplyToolStripTheme(ToolStrip toolStrip)
    {
        toolStrip.Renderer = ToolStripRenderer;
        toolStrip.BackColor = SurfaceColor;
        toolStrip.ForeColor = ForegroundColor;
        ApplyToolStripItemTheme(toolStrip.Items);
    }

    private static void ApplyControlTree(Control root)
    {
        ApplyControl(root);

        foreach (Control child in root.Controls)
        {
            ApplyControlTree(child);
        }
    }

    private static void ApplyControl(Control control)
    {
        switch (control)
        {
            case Form form:
                form.BackColor = BackgroundColor;
                form.ForeColor = ForegroundColor;
                break;

            case MenuStrip menuStrip:
                ApplyToolStripTheme(menuStrip);
                break;

            case StatusStrip statusStrip:
                ApplyToolStripTheme(statusStrip);
                break;

            case ContextMenuStrip contextMenuStrip:
                ApplyToolStripTheme(contextMenuStrip);
                break;

            case ToolStrip toolStrip:
                ApplyToolStripTheme(toolStrip);
                break;

            case Button button:
                button.FlatStyle = FlatStyle.Flat;
                button.BackColor = SurfaceAltColor;
                button.ForeColor = ForegroundColor;
                button.FlatAppearance.BorderColor = BorderColor;
                button.FlatAppearance.MouseDownBackColor = SurfaceColor;
                button.FlatAppearance.MouseOverBackColor = SelectionColor;
                break;

            case TextBox textBox:
                textBox.BorderStyle = BorderStyle.FixedSingle;
                textBox.BackColor = SurfaceAltColor;
                textBox.ForeColor = ForegroundColor;
                break;

            case NumericUpDown numericUpDown:
                numericUpDown.BorderStyle = BorderStyle.FixedSingle;
                numericUpDown.BackColor = SurfaceAltColor;
                numericUpDown.ForeColor = ForegroundColor;
                break;

            case ListBox listBox:
                listBox.BackColor = SurfaceColor;
                listBox.ForeColor = ForegroundColor;
                break;

            case ListView listView:
                listView.BackColor = SurfaceColor;
                listView.ForeColor = ForegroundColor;
                listView.BorderStyle = BorderStyle.FixedSingle;
                break;

            case Label label:
                label.ForeColor = ForegroundColor;
                break;

            case CheckBox checkBox:
                checkBox.ForeColor = ForegroundColor;
                checkBox.BackColor = BackgroundColor;
                break;

            case TableLayoutPanel tableLayoutPanel:
                tableLayoutPanel.BackColor = BackgroundColor;
                tableLayoutPanel.ForeColor = ForegroundColor;
                break;

            case FlowLayoutPanel flowLayoutPanel:
                flowLayoutPanel.BackColor = BackgroundColor;
                flowLayoutPanel.ForeColor = ForegroundColor;
                break;

            case Panel panel:
                panel.BackColor = BackgroundColor;
                panel.ForeColor = ForegroundColor;
                break;
        }
    }

    private static void ApplyToolStripItemTheme(ToolStripItemCollection items)
    {
        foreach (ToolStripItem item in items)
        {
            item.ForeColor = item.Enabled ? ForegroundColor : MutedForegroundColor;
            item.BackColor = SurfaceColor;

            if (item is ToolStripDropDownItem dropDownItem)
            {
                dropDownItem.DropDown.BackColor = SurfaceColor;
                dropDownItem.DropDown.ForeColor = ForegroundColor;
                ApplyToolStripItemTheme(dropDownItem.DropDownItems);
            }
        }
    }

    private static void OnFormHandleCreated(object? sender, EventArgs eventArgs)
    {
        if (sender is not Form form)
        {
            return;
        }

        TryEnableDarkTitleBar(form.Handle);
    }

    private static void TryEnableDarkTitleBar(IntPtr handle)
    {
        if (!OperatingSystem.IsWindows())
        {
            return;
        }

        const int DwmwaUseImmersiveDarkMode = 20;
        const int DwmwaUseImmersiveDarkModeBefore20H1 = 19;
        var enabled = 1;

        _ = DwmSetWindowAttribute(handle, DwmwaUseImmersiveDarkMode, ref enabled, sizeof(int));
        _ = DwmSetWindowAttribute(handle, DwmwaUseImmersiveDarkModeBefore20H1, ref enabled, sizeof(int));
    }

    [DllImport("dwmapi.dll")]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attribute, ref int value, int valueSize);

    private sealed class DarkToolStripRenderer : ToolStripProfessionalRenderer
    {
        public DarkToolStripRenderer()
            : base(new DarkColorTable())
        {
            RoundedEdges = false;
        }

        protected override void OnRenderItemText(ToolStripItemTextRenderEventArgs e)
        {
            e.TextColor = e.Item.Enabled ? ForegroundColor : MutedForegroundColor;
            base.OnRenderItemText(e);
        }
    }

    private sealed class DarkColorTable : ProfessionalColorTable
    {
        public override Color ToolStripDropDownBackground => SurfaceColor;

        public override Color ToolStripBorder => BorderColor;

        public override Color MenuBorder => BorderColor;

        public override Color MenuItemBorder => BorderColor;

        public override Color MenuItemSelected => SelectionColor;

        public override Color MenuItemSelectedGradientBegin => SelectionColor;

        public override Color MenuItemSelectedGradientEnd => SelectionColor;

        public override Color MenuItemPressedGradientBegin => SurfaceAltColor;

        public override Color MenuItemPressedGradientMiddle => SurfaceAltColor;

        public override Color MenuItemPressedGradientEnd => SurfaceAltColor;

        public override Color ButtonSelectedHighlight => SelectionColor;

        public override Color ButtonSelectedHighlightBorder => BorderColor;

        public override Color ButtonPressedHighlight => SurfaceAltColor;

        public override Color ButtonPressedHighlightBorder => BorderColor;

        public override Color ButtonSelectedGradientBegin => SurfaceAltColor;

        public override Color ButtonSelectedGradientMiddle => SurfaceAltColor;

        public override Color ButtonSelectedGradientEnd => SurfaceAltColor;

        public override Color ButtonPressedGradientBegin => SelectionColor;

        public override Color ButtonPressedGradientMiddle => SelectionColor;

        public override Color ButtonPressedGradientEnd => SelectionColor;

        public override Color ImageMarginGradientBegin => SurfaceColor;

        public override Color ImageMarginGradientMiddle => SurfaceColor;

        public override Color ImageMarginGradientEnd => SurfaceColor;

        public override Color SeparatorDark => BorderColor;

        public override Color SeparatorLight => BorderColor;

        public override Color StatusStripGradientBegin => SurfaceColor;

        public override Color StatusStripGradientEnd => SurfaceColor;

        public override Color ToolStripGradientBegin => BackgroundColor;

        public override Color ToolStripGradientMiddle => BackgroundColor;

        public override Color ToolStripGradientEnd => BackgroundColor;

        public override Color CheckBackground => SurfaceAltColor;

        public override Color CheckSelectedBackground => SelectionColor;

        public override Color CheckPressedBackground => SelectionColor;
    }
}