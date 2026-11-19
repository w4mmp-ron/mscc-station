using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;

namespace MSCC.Avalonia.Views;

/// <summary>Keep Remote Audio and Debug Log inside the working area on a short screen.</summary>
internal static class ScreenFit
{
    public static void CapToWorkingArea(Window window)
    {
        var screen = window.Screens?.ScreenFromWindow(window) ?? window.Screens?.Primary;
        if (screen == null) return;
        var area = screen.WorkingArea;
        double scale = screen.Scaling > 0 ? screen.Scaling : 1;
        double workH = area.Height / scale;
        if (window.Height > workH - 40)
            window.Height = Math.Max(window.MinHeight, workH - 40);

        int heightPx = (int)Math.Ceiling(window.Height * scale);
        int widthPx = (int)Math.Ceiling(window.Width * scale);
        int x = window.Position.X;
        int y = window.Position.Y;
        if (y < area.Y) y = area.Y;
        int bottom = area.Y + area.Height;
        if (y + heightPx > bottom)
            y = Math.Max(area.Y, bottom - heightPx);
        if (x < area.X) x = area.X;
        if (x + widthPx > area.X + area.Width)
            x = Math.Max(area.X, area.X + area.Width - widthPx);
        if (x != window.Position.X || y != window.Position.Y)
            window.Position = new PixelPoint(x, y);
    }

    public static void PlaceDebugLog(Window log)
    {
        CapToWorkingArea(log);
        RemoteAfWindow? remote = null;
        if (Application.Current?.ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desk)
        {
            foreach (var w in desk.Windows)
            {
                if (w is RemoteAfWindow rw && rw.IsVisible)
                {
                    remote = rw;
                    break;
                }
            }
        }
        if (remote == null) return;

        var screen = remote.Screens?.ScreenFromWindow(remote)
            ?? log.Screens?.ScreenFromWindow(log)
            ?? log.Screens?.Primary;
        if (screen == null) return;
        var area = screen.WorkingArea;
        double scale = screen.Scaling > 0 ? screen.Scaling : 1;
        double remoteScale = remote.RenderScaling > 0 ? remote.RenderScaling : scale;
        int gap = 8;
        int myW = (int)Math.Ceiling(log.Width * scale);
        int myH = (int)Math.Ceiling(log.Height * scale);
        int rW = (int)Math.Ceiling(remote.Bounds.Width * remoteScale);
        int rH = (int)Math.Ceiling(remote.Bounds.Height * remoteScale);
        int rx = remote.Position.X;
        int ry = remote.Position.Y;

        bool Fits(int x, int y) =>
            x >= area.X && y >= area.Y &&
            x + myW <= area.X + area.Width &&
            y + myH <= area.Y + area.Height;

        int right = rx + rW + gap;
        if (Fits(right, ry))
        {
            log.Position = new PixelPoint(right, ry);
            return;
        }
        int left = rx - gap - myW;
        if (Fits(left, ry))
        {
            log.Position = new PixelPoint(left, ry);
            return;
        }
        int x = Math.Clamp(rx, area.X, Math.Max(area.X, area.X + area.Width - myW));
        int below = ry + rH + gap;
        if (Fits(x, below))
        {
            log.Position = new PixelPoint(x, below);
            return;
        }
        int above = ry - gap - myH;
        if (Fits(x, above))
        {
            log.Position = new PixelPoint(x, above);
            return;
        }
        log.Position = new PixelPoint(area.X, area.Y);
    }
}
