using System.Collections.Specialized;
using System.Windows;
using MSCC.Wpf.ViewModels;

namespace MSCC.Wpf;

/// <summary>
/// Popup debug log viewer (moved from AUD/SYS). Hosts RESET LOGS + Pause and a virtualized log list.
/// Remembers position/size for the current process session only.
/// </summary>
public partial class DebugLogWindow : Window
{
    // Session-only placement (not written to INI).
    private static double? s_left;
    private static double? s_top;
    private static double? s_width;
    private static double? s_height;

    private MainViewModel? _vm;

    public DebugLogWindow()
    {
        InitializeComponent();
        SourceInitialized += OnSourceInitialized;
        Loaded += OnLoaded;
        Closed += OnClosed;
    }

    private void OnSourceInitialized(object? sender, System.EventArgs e)
    {
        if (s_left is double left && s_top is double top)
        {
            WindowStartupLocation = WindowStartupLocation.Manual;
            Left = left;
            Top = top;
            if (s_width is double w && w >= MinWidth)
                Width = w;
            if (s_height is double h && h >= MinHeight)
                Height = h;
        }
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        _vm = DataContext as MainViewModel;
        if (_vm == null) return;

        _vm.DebugLog.CollectionChanged += OnDebugLogCollectionChanged;
        ScrollToEnd();
    }

    private void OnClosed(object? sender, System.EventArgs e)
    {
        // Remember placement for reopen this session (normal state only).
        if (WindowState == WindowState.Normal)
        {
            s_left = Left;
            s_top = Top;
            s_width = Width;
            s_height = Height;
        }

        if (_vm != null)
            _vm.DebugLog.CollectionChanged -= OnDebugLogCollectionChanged;
        _vm = null;
    }

    private void OnDebugLogCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        if (e.Action != NotifyCollectionChangedAction.Add) return;
        Dispatcher.BeginInvoke(ScrollToEnd);
    }

    private void ScrollToEnd()
    {
        if (DebugLogListBox.Items.Count > 0)
            DebugLogListBox.ScrollIntoView(DebugLogListBox.Items[^1]);
    }

    private void Exit_Click(object sender, RoutedEventArgs e) => Close();
}
