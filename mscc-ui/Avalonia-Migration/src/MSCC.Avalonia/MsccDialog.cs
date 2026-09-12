using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Layout;
using Avalonia.Media;

namespace MSCC.Avalonia;

/// <summary>Simple owner-centered Yes/No/Cancel dialogs (WPF MessageBox stand-in).</summary>
internal static class MsccDialog
{
    public enum Result
    {
        Cancel = 0,
        No = 1,
        Yes = 2,
        Ok = 3
    }

    public static Task AlertAsync(string message, string title = "MSCC")
        => ShowAsync(title, message, ok: true).ContinueWith(_ => { });

    public static async Task<bool> ConfirmAsync(string message, string title = "MSCC")
        => await ShowAsync(title, message, yesNo: true) == Result.Yes;

    public static Task<Result> YesNoCancelAsync(string message, string title = "MSCC")
        => ShowAsync(title, message, yesNoCancel: true);

    private static async Task<Result> ShowAsync(
        string title,
        string message,
        bool ok = false,
        bool yesNo = false,
        bool yesNoCancel = false)
    {
        var owner = Owner();
        var chosen = Result.Cancel;

        var win = new Window
        {
            Title = title,
            Width = 460,
            SizeToContent = SizeToContent.Height,
            WindowStartupLocation = WindowStartupLocation.CenterOwner,
            CanResize = false,
            ShowInTaskbar = false,
            Background = new SolidColorBrush(Color.Parse("#FF2A2A2A")),
        };

        var text = new TextBlock
        {
            Text = message,
            TextWrapping = TextWrapping.Wrap,
            Foreground = Brushes.White,
            FontSize = 13,
            Margin = new Thickness(16, 16, 16, 8),
        };

        var buttons = new StackPanel
        {
            Orientation = Orientation.Horizontal,
            HorizontalAlignment = HorizontalAlignment.Right,
            Spacing = 8,
            Margin = new Thickness(16, 8, 16, 16),
        };

        Button Add(string caption, Result result, bool isDefault = false, bool isCancel = false)
        {
            var b = new Button
            {
                Content = caption,
                MinWidth = 80,
                Height = 28,
                IsDefault = isDefault,
                IsCancel = isCancel,
            };
            b.Click += (_, _) =>
            {
                chosen = result;
                win.Close();
            };
            buttons.Children.Add(b);
            return b;
        }

        if (ok)
            Add("OK", Result.Ok, isDefault: true, isCancel: true);
        else if (yesNoCancel)
        {
            Add("Yes", Result.Yes, isDefault: true);
            Add("No", Result.No);
            Add("Cancel", Result.Cancel, isCancel: true);
        }
        else
        {
            Add("Yes", Result.Yes, isDefault: true);
            Add("No", Result.No, isCancel: true);
        }

        win.Content = new StackPanel { Children = { text, buttons } };

        if (owner != null)
            await win.ShowDialog(owner);
        else
        {
            win.WindowStartupLocation = WindowStartupLocation.CenterScreen;
            win.Show();
            var tcs = new TaskCompletionSource();
            win.Closed += (_, _) => tcs.TrySetResult();
            await tcs.Task;
        }

        return chosen;
    }

    private static Window? Owner()
    {
        if (Application.Current?.ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desk)
            return desk.MainWindow;
        return null;
    }
}
