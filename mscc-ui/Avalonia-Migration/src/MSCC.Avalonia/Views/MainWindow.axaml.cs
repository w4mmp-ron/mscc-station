using System.Globalization;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Media;
using MSCC.Avalonia.ViewModels;

namespace MSCC.Avalonia.Views;

public partial class MainWindow : Window
{
    private DebugLogWindow? _logWindow;
    private SpectrumWaterfallWindow? _swWindow;

    public MainWindow()
    {
        InitializeComponent();
        Closed += OnClosed;
        Opened += OnOpened;
    }

    private void OnOpened(object? sender, EventArgs e)
    {
        SpectrumDisplay.FrequencyClicked += OnSpectrumFrequencyClicked;
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

    private void VfoAFreqText_OnPointerMoved(object? sender, PointerEventArgs e)
    {
        UpdateHoverStep(sender, e);
    }

    private void VfoBFreqText_OnPointerMoved(object? sender, PointerEventArgs e)
    {
        UpdateHoverStep(sender, e);
    }

    private void VfoFreqText_OnPointerExited(object? sender, PointerEventArgs e)
    {
        if (DataContext is MainViewModel vm)
            vm.SetHoverTuneStep(0);
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
        vm.SetHoverTuneStep(stepHz);
        e.Handled = true;
    }

    private void UpdateHoverStep(object? sender, PointerEventArgs e)
    {
        if (sender is not TextBlock freqTb || DataContext is not MainViewModel vm)
            return;

        Point pos = e.GetPosition(freqTb);
        if (TryGetDigitStepHz(freqTb, pos, out long stepHz))
            vm.SetHoverTuneStep(stepHz);
        else
            vm.SetHoverTuneStep(0);
    }

    private static int WheelDirection(PointerWheelEventArgs e)
    {
        if (e.Delta.Y > 0) return 1;
        if (e.Delta.Y < 0) return -1;
        return 0;
    }

    /// <summary>
    /// WPF-style digit hit-test: step = 10^(digits to the right of hovered digit).
    /// Display is MHz F6 e.g. "7.000000" → over 1 kHz place → step 1000 Hz.
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

        _swWindow = new SpectrumWaterfallWindow();
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
