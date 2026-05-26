using System.Drawing.Drawing2D;
using System.Runtime.InteropServices;

namespace SimpleTunnelSSH.App;

internal static class AppIconFactory
{
    public static Icon ApplicationIcon { get; } = CreateIcon(64, includeIndicator: false, indicatorColor: AppTheme.ConnectionDisconnectedColor);

    public static Icon InactiveTrayIcon { get; } = CreateIcon(64, includeIndicator: true, indicatorColor: AppTheme.ConnectionDisconnectedColor);

    public static Icon ActiveTrayIcon { get; } = CreateIcon(64, includeIndicator: true, indicatorColor: AppTheme.ConnectionConnectedColor);

    public static Icon AttentionTrayIcon { get; } = CreateIcon(64, includeIndicator: true, indicatorColor: AppTheme.ConnectionAttentionColor);

    public static Icon CreateIcon(int size, bool includeIndicator, Color indicatorColor)
    {
        using var bitmap = new Bitmap(size, size, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
        using var graphics = Graphics.FromImage(bitmap);
        graphics.Clear(Color.Transparent);
        graphics.SmoothingMode = SmoothingMode.AntiAlias;
        graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;

        var shellBounds = includeIndicator
            ? new RectangleF(size * 0.03f, size * 0.04f, size * 0.84f, size * 0.84f)
            : new RectangleF(size * 0.02f, size * 0.03f, size * 0.90f, size * 0.90f);
        using var shellPath = CreateRoundedRectangle(shellBounds, size * 0.18f);
        using var shellBrush = new SolidBrush(Color.FromArgb(28, 33, 39));
        using var shellBorderPen = new Pen(Color.FromArgb(88, 98, 110), Math.Max(1.5f, size * 0.045f));
        graphics.FillPath(shellBrush, shellPath);
        graphics.DrawPath(shellBorderPen, shellPath);

        using var linePen = new Pen(Color.FromArgb(120, 211, 255), Math.Max(3.0f, size * 0.11f))
        {
            StartCap = LineCap.Round,
            EndCap = LineCap.Round
        };

        var leftX = shellBounds.Left + (shellBounds.Width * 0.31f);
        var rightX = shellBounds.Left + (shellBounds.Width * 0.69f);
        var topY = shellBounds.Top + (shellBounds.Height * 0.29f);
        var bottomY = shellBounds.Top + (shellBounds.Height * 0.71f);
        var centerY = shellBounds.Top + (shellBounds.Height * 0.50f);

        graphics.DrawLine(linePen, leftX, topY, leftX, bottomY);
        graphics.DrawLine(linePen, rightX, topY, rightX, bottomY);
        graphics.DrawLine(linePen, leftX, centerY, rightX, centerY);

        using var nodeBrush = new SolidBrush(Color.FromArgb(233, 239, 247));
        var nodeSize = shellBounds.Width * 0.15f;
        graphics.FillEllipse(nodeBrush, leftX - (nodeSize / 2f), topY - (nodeSize / 2f), nodeSize, nodeSize);
        graphics.FillEllipse(nodeBrush, leftX - (nodeSize / 2f), bottomY - (nodeSize / 2f), nodeSize, nodeSize);
        graphics.FillEllipse(nodeBrush, rightX - (nodeSize / 2f), topY - (nodeSize / 2f), nodeSize, nodeSize);
        graphics.FillEllipse(nodeBrush, rightX - (nodeSize / 2f), bottomY - (nodeSize / 2f), nodeSize, nodeSize);

        if (includeIndicator)
        {
            var indicatorSize = size * 0.30f;
            var indicatorX = size - indicatorSize - (size * 0.05f);
            var indicatorY = size - indicatorSize - (size * 0.05f);
            using var indicatorBrush = new SolidBrush(indicatorColor);
            using var indicatorBorderPen = new Pen(Color.FromArgb(245, 247, 250), Math.Max(1.5f, size * 0.035f));
            graphics.FillEllipse(indicatorBrush, indicatorX, indicatorY, indicatorSize, indicatorSize);
            graphics.DrawEllipse(indicatorBorderPen, indicatorX, indicatorY, indicatorSize, indicatorSize);
        }

        var handle = bitmap.GetHicon();

        try
        {
            using var icon = Icon.FromHandle(handle);
            return (Icon)icon.Clone();
        }
        finally
        {
            DestroyIcon(handle);
        }
    }

    private static GraphicsPath CreateRoundedRectangle(RectangleF bounds, float radius)
    {
        var diameter = radius * 2f;
        var path = new GraphicsPath();
        path.AddArc(bounds.X, bounds.Y, diameter, diameter, 180, 90);
        path.AddArc(bounds.Right - diameter, bounds.Y, diameter, diameter, 270, 90);
        path.AddArc(bounds.Right - diameter, bounds.Bottom - diameter, diameter, diameter, 0, 90);
        path.AddArc(bounds.X, bounds.Bottom - diameter, diameter, diameter, 90, 90);
        path.CloseFigure();
        return path;
    }

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool DestroyIcon(IntPtr handle);
}