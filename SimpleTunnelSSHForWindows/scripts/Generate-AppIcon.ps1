$ErrorActionPreference = 'Stop'

$assetsDirectory = Join-Path $PSScriptRoot '..\src\SimpleTunnelSSH.App\Assets'
New-Item -ItemType Directory -Path $assetsDirectory -Force | Out-Null

if (-not ('SimpleTunnelSshIconWriter' -as [type])) {
    $referencedAssemblies = @(
        [System.Drawing.Bitmap].Assembly.Location,
        [System.Drawing.RectangleF].Assembly.Location,
        [System.Drawing.Drawing2D.GraphicsPath].Assembly.Location,
        [System.Drawing.Imaging.ImageFormat].Assembly.Location
    ) | Select-Object -Unique

Add-Type -ReferencedAssemblies $referencedAssemblies -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;

public static class SimpleTunnelSshIconWriter
{
    public static void Write(string outputPath)
    {
        const int size = 256;
        using (var bitmap = new Bitmap(size, size, PixelFormat.Format32bppArgb))
        using (var graphics = Graphics.FromImage(bitmap))
        {
            graphics.Clear(Color.Transparent);
            graphics.SmoothingMode = SmoothingMode.AntiAlias;
            graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;

            var shellBounds = new RectangleF(size * 0.02f, size * 0.03f, size * 0.90f, size * 0.90f);
            using (var shellPath = CreateRoundedRectangle(shellBounds, size * 0.18f))
            using (var shellBrush = new SolidBrush(Color.FromArgb(28, 33, 39)))
            using (var shellBorderPen = new Pen(Color.FromArgb(88, 98, 110), size * 0.045f))
            {
                graphics.FillPath(shellBrush, shellPath);
                graphics.DrawPath(shellBorderPen, shellPath);
            }

            using (var linePen = new Pen(Color.FromArgb(120, 211, 255), size * 0.11f))
            {
                linePen.StartCap = LineCap.Round;
                linePen.EndCap = LineCap.Round;

                var leftX = shellBounds.Left + (shellBounds.Width * 0.31f);
                var rightX = shellBounds.Left + (shellBounds.Width * 0.69f);
                var topY = shellBounds.Top + (shellBounds.Height * 0.29f);
                var bottomY = shellBounds.Top + (shellBounds.Height * 0.71f);
                var centerY = shellBounds.Top + (shellBounds.Height * 0.50f);

                graphics.DrawLine(linePen, leftX, topY, leftX, bottomY);
                graphics.DrawLine(linePen, rightX, topY, rightX, bottomY);
                graphics.DrawLine(linePen, leftX, centerY, rightX, centerY);

                using (var nodeBrush = new SolidBrush(Color.FromArgb(233, 239, 247)))
                {
                    var nodeSize = shellBounds.Width * 0.15f;
                    graphics.FillEllipse(nodeBrush, leftX - (nodeSize / 2f), topY - (nodeSize / 2f), nodeSize, nodeSize);
                    graphics.FillEllipse(nodeBrush, leftX - (nodeSize / 2f), bottomY - (nodeSize / 2f), nodeSize, nodeSize);
                    graphics.FillEllipse(nodeBrush, rightX - (nodeSize / 2f), topY - (nodeSize / 2f), nodeSize, nodeSize);
                    graphics.FillEllipse(nodeBrush, rightX - (nodeSize / 2f), bottomY - (nodeSize / 2f), nodeSize, nodeSize);
                }
            }

            using (var pngStream = new MemoryStream())
            {
                bitmap.Save(pngStream, ImageFormat.Png);
                var pngBytes = pngStream.ToArray();

                using (var fileStream = File.Create(outputPath))
                using (var writer = new BinaryWriter(fileStream))
                {
                    writer.Write((short)0);
                    writer.Write((short)1);
                    writer.Write((short)1);
                    writer.Write((byte)0);
                    writer.Write((byte)0);
                    writer.Write((byte)0);
                    writer.Write((byte)0);
                    writer.Write((short)1);
                    writer.Write((short)32);
                    writer.Write(pngBytes.Length);
                    writer.Write(22);
                    writer.Write(pngBytes);
                }
            }
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
}
"@
}

[SimpleTunnelSshIconWriter]::Write((Join-Path $assetsDirectory 'App.ico'))