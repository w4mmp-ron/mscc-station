using System.Globalization;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Media;
using Avalonia.Threading;
using MSCC.Avalonia.ViewModels;
using MsccDialog = MSCC.Avalonia.MsccDialog;

namespace MSCC.Avalonia.Views;

public partial class MainWindow : Window
{
    private DebugLogWindow? _logWindow;
    private SpectrumWaterfallWindow? _swWindow;
    private bool _closeReady;
    private bool _tabChangeGuard;

    public MainWindow()
    {
        InitializeComponent();
        Closed += OnClosed;
        Closing += OnClosing;
        Opened += OnOpened;
        DataContextChanged += (_, _) => RestoreWindowPlacement();
        PositionChanged += (_, _) => SaveWindowPlacement();
        PropertyChanged += (_, e) =>
        {
            if (e.Property == ClientSizeProperty || e.Property == WindowStateProperty)
                SaveWindowPlacement();
        };
    }

    private bool _placementRestored;

    /// <summary>Drag a window-edge grip: the window manager resizes (works without WM side handles).</summary>
    private void ResizeGrip_OnPointerPressed(object? sender, PointerPressedEventArgs e)
    {
        if (!e.GetCurrentPoint(this).Properties.IsLeftButtonPressed) return;
        if (WindowState != WindowState.Normal) return;
        WindowEdge edge = sender == ResizeGripWest ? WindowEdge.West
            : sender == ResizeGripSouthEast ? WindowEdge.SouthEast
            : WindowEdge.East;
        BeginResizeDrag(edge, e);
        e.Handled = true;
    }

    /// <summary>Ron 2026-11-05: put the window back where it was (only if that spot is on a screen).</summary>
    private void RestoreWindowPlacement()
    {
        if (_placementRestored || DataContext is not MainViewModel vm) return;
        _placementRestored = true;
        var (x, y, w, h, max) = vm.SavedWindowPlacement;
        if (w >= MinWidth && h >= MinHeight)
        {
            Width = w;
            Height = h;
            var pos = new PixelPoint(x, y);
            bool onScreen = Screens.All.Any(sc => sc.WorkingArea.Contains(new PixelPoint(x + 40, y + 20)));
            if (onScreen)
            {
                WindowStartupLocation = WindowStartupLocation.Manual;
                Position = pos;
            }
        }
        if (max)
            WindowState = WindowState.Maximized;
    }

    private void SaveWindowPlacement()
    {
        if (!_placementRestored || !IsVisible || DataContext is not MainViewModel vm) return;
        if (WindowState == WindowState.Minimized) return;
        bool max = WindowState == WindowState.Maximized;
        vm.RememberWindowPlacement(Position.X, Position.Y,
            (int)Math.Round(ClientSize.Width), (int)Math.Round(ClientSize.Height), max);
    }

    private async void OnOpened(object? sender, EventArgs e)
    {
        SpectrumDisplay.FrequencyClicked += OnSpectrumFrequencyClicked;
        SMeterControl.ResetRequested += OnSwrResetRequested;
        if (DataContext is MainViewModel vm && vm.AutoStart)
        {
            await Task.Delay(500);
            vm.AppendLog($"Auto: Connect on start (Host {vm.Host})");
            if (vm.ConnectCommand.CanExecute(null))
                await vm.ConnectCommand.ExecuteAsync(null);
        }
    }

    private async void OnClosing(object? sender, WindowClosingEventArgs e)
    {
        if (_closeReady) return;
        e.Cancel = true;
        if (DataContext is MainViewModel vm)
            vm.KickOwnedServerStop();
        try
        {
            if (DataContext is MainViewModel vm2)
                await vm2.PrepareForCloseAsync();
        }
        catch { /* ignore */ }
        _closeReady = true;
        Dispatcher.UIThread.Post(() => Close());
    }

    private async void MainTabs_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (_tabChangeGuard) return;
        if (DataContext is not MainViewModel vm) return;
        var added = e.AddedItems.Count > 0 ? e.AddedItems[0] as TabItem : null;
        var removed = e.RemovedItems.Count > 0 ? e.RemovedItems[0] as TabItem : null;
        if (removed == QrpCalTab)
            vm.ForceStopPowerCal("leave tab");
        if (added == FreqCalTab)
        {
            await vm.EnterFreqCalTab();
            return;
        }
        if (removed == FreqCalTab && !await TryLeaveFreqCalTabAsync(vm, added))
            return;
        if (added == QrpCalTab)
            await vm.PreparePowerCalTab();
        else if (added == AmpCalTab)
            await vm.PrepareAmpCalTab();
    }

    /// <summary>Existing Freq Cal leave guards. False when the tab change was cancelled.</summary>
    private async Task<bool> TryLeaveFreqCalTabAsync(MainViewModel vm, TabItem? added)
    {
        if (vm.FreqCalManualMode)
        {
            _tabChangeGuard = true;
            Dispatcher.UIThread.Post(() =>
            {
                MainTabs.SelectedItem = FreqCalTab;
                _tabChangeGuard = false;
            });
            await MsccDialog.AlertAsync("EXIT MANUAL CALIBRATION FIRST.");
            return false;
        }
        if (vm.FreqCalInProgress &&
            (added == QrpCalTab || added == AmpCalTab || added == TxIqTab))
        {
            _tabChangeGuard = true;
            Dispatcher.UIThread.Post(() =>
            {
                MainTabs.SelectedItem = FreqCalTab;
                _tabChangeGuard = false;
            });
            await MsccDialog.AlertAsync("FREQUENCY CALIBRATION IN PROGRESS.");
            return false;
        }
        if (vm.FreqCalInProgress)
        {
            vm.DeferFreqCalRestore();
            return false;
        }
        await vm.LeaveFreqCalTab();
        return true;
    }

    /// <summary>Avalonia int bindings often only commit on pointer-up; send 0xA2 while dragging.</summary>
    private void PowerCalSlider_OnValueChanged(object? sender, RangeBaseValueChangedEventArgs e)
    {
        if (DataContext is MainViewModel vm)
            vm.ApplyPowerCalSliderLive(e.NewValue);
    }

    private void AmpCalSlider_OnValueChanged(object? sender, RangeBaseValueChangedEventArgs e)
    {
        if (DataContext is MainViewModel vm)
            vm.ApplyAmpCalSliderLive(e.NewValue);
    }

    private void HostPick_OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (DataContext is not MainViewModel vm) return;
        if (e.AddedItems.Count == 0) return;
        if (e.AddedItems[0] is string s && !string.IsNullOrWhiteSpace(s))
            vm.Host = s;
    }

    /// <summary>Red RESET on the meter after an SWR fault: HTTP reset of the WiFi meter (WPF port).</summary>
    private async void OnSwrResetRequested(object? sender, EventArgs e)
    {
        if (DataContext is MainViewModel vm)
            await vm.ResetSwrFaultAsync();
    }

    private void OnSpectrumFrequencyClicked(object? sender, long frequencyHz)
    {
        if (DataContext is MainViewModel vm)
            _ = vm.TuneFromSpectrumAsync(frequencyHz);
    }

    private void VfoAPanel_OnPointerPressed(object? sender, PointerPressedEventArgs e)
    {
        if (DataContext is MainViewModel vm)
            _ = vm.SelectVfoAsync(useVfoA: true);
    }

    private void VfoBPanel_OnPointerPressed(object? sender, PointerPressedEventArgs e)
    {
        if (DataContext is MainViewModel vm)
            _ = vm.SelectVfoAsync(useVfoA: false);
    }

    /// <summary>Wheel over VFO A panel (not on digits) uses left-rail Step.</summary>
    private void VfoAPanel_OnPointerWheelChanged(object? sender, PointerWheelEventArgs e)
    {
        if (e.Handled) return;
        if (DataContext is not MainViewModel vm)
            return;

        int direction = WheelDirection(e);
        if (direction == 0)
            return;

        _ = vm.NudgeFrequencyByDigitAsync(direction, GetLeftRailStepHz(vm), quantize: false, vfoA: true);
        e.Handled = true;
    }

    private void VfoBPanel_OnPointerWheelChanged(object? sender, PointerWheelEventArgs e)
    {
        if (e.Handled) return;
        if (DataContext is not MainViewModel vm)
            return;

        int direction = WheelDirection(e);
        if (direction == 0)
            return;

        _ = vm.NudgeFrequencyByDigitAsync(direction, GetLeftRailStepHz(vm), quantize: false, vfoA: false);
        e.Handled = true;
    }

    private static long GetLeftRailStepHz(MainViewModel vm)
    {
        // Mirror ViewModel StepChoices default when only label is public
        return vm.GetCurrentStepHz();
    }

    private void VfoAFreqText_OnPointerWheelChanged(object? sender, PointerWheelEventArgs e)
    {
        HandleDigitWheel(sender, e, vfoA: true);
    }

    private void VfoBFreqText_OnPointerWheelChanged(object? sender, PointerWheelEventArgs e)
    {
        HandleDigitWheel(sender, e, vfoA: false);
    }

    private void HandleDigitWheel(object? sender, PointerWheelEventArgs e, bool vfoA)
    {
        if (sender is not TextBlock freqTb || DataContext is not MainViewModel vm)
            return;

        int direction = WheelDirection(e);
        if (direction == 0)
            return;

        Point pos = e.GetPosition(freqTb);
        if (!TryGetDigitStepHz(freqTb, pos, out long stepHz))
        {
            // Not over a digit — VFO A uses left-rail Step; VFO B no-op
            if (vfoA)
                _ = vm.NudgeFrequencyAsync(direction);
            e.Handled = true;
            return;
        }

        _ = vm.NudgeFrequencyByDigitAsync(direction, stepHz, quantize: true, vfoA: vfoA);
        e.Handled = true;
    }

    private static int WheelDirection(PointerWheelEventArgs e)
    {
        if (e.Delta.Y > 0) return 1;
        if (e.Delta.Y < 0) return -1;
        return 0;
    }

    /// <summary>
    /// One notch = SmallChange (or 1). Same as WPF Slider_PreviewMouseWheel.
    /// Tab sliders still use this. Left and right rail sliders use SliderWheel.OneStepPerNotch.
    /// Handled so the notch does not scroll or reach the VFO.
    /// </summary>
    private void Slider_OnPointerWheelChanged(object? sender, PointerWheelEventArgs e)
    {
        if (sender is not Slider slider)
            return;
        int direction = WheelDirection(e);
        if (direction == 0)
            return;
        double step = slider.SmallChange > 0 ? slider.SmallChange : 1.0;
        slider.Value = Math.Max(slider.Minimum, Math.Min(slider.Maximum, slider.Value + direction * step));
        e.Handled = true;
    }

    /// <summary>
    /// Digit hit-test: step = 10^(digits to the right of the hovered digit), then × 10^Tag.
    /// Big digits (Tag 3) are "14.074"; the 1 kHz place steps 1000 Hz and the MHz place steps 1 000 000 Hz.
    /// Hz digits (Tag 0) step 100 / 10 / 1 Hz. Over '.' the digit to the left is used.
    /// </summary>
    private static bool TryGetDigitStepHz(TextBlock freqTb, Point pos, out long stepHz)
    {
        stepHz = 0;
        string freqPart = freqTb.Text ?? "";
        if (string.IsNullOrWhiteSpace(freqPart))
            return false;

        double w = freqTb.Bounds.Width;
        double h = freqTb.Bounds.Height;
        if (w < 2 || h < 2)
            return false;
        if (pos.X < 0 || pos.X > w || pos.Y < -2 || pos.Y > h + 2)
            return false;

        var typeface = new Typeface(
            freqTb.FontFamily,
            freqTb.FontStyle,
            freqTb.FontWeight,
            freqTb.FontStretch);

        double fontSize = freqTb.FontSize > 0 ? freqTb.FontSize : 14;

        // Measure full string and prefixes for hit testing
        double fullWidth = MeasureTextWidth(freqPart, typeface, fontSize);
        if (fullWidth < 1)
            return false;

        // Centered text: map X into text bounds
        double textLeft = Math.Max(0, (w - fullWidth) / 2.0);
        double xInText = pos.X - textLeft;
        if (xInText < 0 || xInText > fullWidth)
            return false;

        int charIdx = 0;
        for (int i = 0; i < freqPart.Length; i++)
        {
            string prefix = freqPart.Substring(0, i + 1);
            double preW = MeasureTextWidth(prefix, typeface, fontSize);
            if (xInText < preW)
            {
                charIdx = i;
                break;
            }
            charIdx = i;
        }

        if (charIdx < 0 || charIdx >= freqPart.Length)
            return false;

        char c = freqPart[charIdx];
        if (!char.IsDigit(c))
        {
            // Over '.' — use digit immediately to the left
            int leftDigitIdx = -1;
            for (int j = charIdx - 1; j >= 0; j--)
            {
                if (char.IsDigit(freqPart[j]))
                {
                    leftDigitIdx = j;
                    break;
                }
            }
            if (leftDigitIdx < 0)
                return false;
            charIdx = leftDigitIdx;
        }

        int digitsToRight = 0;
        for (int j = charIdx + 1; j < freqPart.Length; j++)
        {
            if (char.IsDigit(freqPart[j]))
                digitsToRight++;
        }

        stepHz = 1;
        for (int k = 0; k < digitsToRight; k++)
        {
            if (stepHz > long.MaxValue / 10)
                break;
            stepHz *= 10;
        }

        // Big digits are the MHz.kHz group (Tag 3). Hz digits are Tag 0.
        int offset = int.TryParse(freqTb.Tag?.ToString(), out int tagOffset) ? tagOffset : 0;
        for (int k = 0; k < offset; k++)
        {
            if (stepHz > long.MaxValue / 10)
                break;
            stepHz *= 10;
        }

        return stepHz > 0;
    }

    private static double MeasureTextWidth(string text, Typeface typeface, double fontSize)
    {
        var ft = new FormattedText(
            text,
            CultureInfo.CurrentCulture,
            FlowDirection.LeftToRight,
            typeface,
            fontSize,
            Brushes.White);
        return ft.Width;
    }

    /// <summary>Double-click VFO cycles tune step (same as left Step button).</summary>
    private void VfoAPanel_OnDoubleTapped(object? sender, TappedEventArgs e)
    {
        if (DataContext is MainViewModel vm && vm.CycleStepCommand.CanExecute(null))
            vm.CycleStepCommand.Execute(null);
        e.Handled = true;
    }

    /// <summary>Open debug log popup (single instance), like WPF DebugLogWindow.</summary>
    private void LogButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (_logWindow != null)
        {
            _logWindow.Activate();
            return;
        }

        _logWindow = new DebugLogWindow
        {
            DataContext = DataContext,
        };
        _logWindow.Closed += (_, _) => _logWindow = null;

        try { _logWindow.Show(this); }
        catch { _logWindow.Show(); }
    }

    /// <summary>Open spectrum / waterfall (S/W) controls — single instance.</summary>
    private void SwButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (_swWindow != null)
        {
            _swWindow.Activate();
            return;
        }

        _swWindow = new SpectrumWaterfallWindow(DataContext as MainViewModel);
        _swWindow.Closed += (_, _) => _swWindow = null;

        try { _swWindow.Show(this); }
        catch { _swWindow.Show(); }
    }

    private void OnClosed(object? sender, EventArgs e)
    {
        SpectrumDisplay.FrequencyClicked -= OnSpectrumFrequencyClicked;
        if (_logWindow != null)
        {
            try { _logWindow.Close(); } catch { /* ignore */ }
            _logWindow = null;
        }

        if (_swWindow != null)
        {
            try { _swWindow.Close(); } catch { /* ignore */ }
            _swWindow = null;
        }

        if (DataContext is MainViewModel vm)
            vm.Dispose();
    }
}
