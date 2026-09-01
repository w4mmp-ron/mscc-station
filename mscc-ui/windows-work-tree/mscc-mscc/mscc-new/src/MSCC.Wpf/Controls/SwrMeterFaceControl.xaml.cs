using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace MSCC.Wpf.Controls;

/// <summary>
/// Compact SWR face for the S-meter slot on TX / fault.
/// Click main reading to cycle SWR ↔ FWD display.
/// </summary>
public partial class SwrMeterFaceControl : UserControl
{
    public static readonly DependencyProperty SwrProperty =
        DependencyProperty.Register(nameof(Swr), typeof(double), typeof(SwrMeterFaceControl),
            new PropertyMetadata(1.0, OnReadingChanged));

    public static readonly DependencyProperty ForwardWattsProperty =
        DependencyProperty.Register(nameof(ForwardWatts), typeof(double), typeof(SwrMeterFaceControl),
            new PropertyMetadata(0.0, OnReadingChanged));

    public static readonly DependencyProperty ReflectedWattsProperty =
        DependencyProperty.Register(nameof(ReflectedWatts), typeof(double), typeof(SwrMeterFaceControl),
            new PropertyMetadata(0.0, OnReadingChanged));

    public static readonly DependencyProperty FaultProperty =
        DependencyProperty.Register(nameof(Fault), typeof(bool), typeof(SwrMeterFaceControl),
            new PropertyMetadata(false, OnFaultChanged));

    public static readonly DependencyProperty SwrThresholdProperty =
        DependencyProperty.Register(nameof(SwrThreshold), typeof(double), typeof(SwrMeterFaceControl),
            new PropertyMetadata(2.0, OnReadingChanged));

    public static readonly DependencyProperty ShowFwdModeProperty =
        DependencyProperty.Register(nameof(ShowFwdMode), typeof(bool), typeof(SwrMeterFaceControl),
            new PropertyMetadata(false, OnReadingChanged));

    public double Swr
    {
        get => (double)GetValue(SwrProperty);
        set => SetValue(SwrProperty, value);
    }

    public double ForwardWatts
    {
        get => (double)GetValue(ForwardWattsProperty);
        set => SetValue(ForwardWattsProperty, value);
    }

    public double ReflectedWatts
    {
        get => (double)GetValue(ReflectedWattsProperty);
        set => SetValue(ReflectedWattsProperty, value);
    }

    public bool Fault
    {
        get => (bool)GetValue(FaultProperty);
        set => SetValue(FaultProperty, value);
    }

    public double SwrThreshold
    {
        get => (double)GetValue(SwrThresholdProperty);
        set => SetValue(SwrThresholdProperty, value);
    }

    /// <summary>False = show SWR as main; true = show FWD watts as main.</summary>
    public bool ShowFwdMode
    {
        get => (bool)GetValue(ShowFwdModeProperty);
        set => SetValue(ShowFwdModeProperty, value);
    }

    public event RoutedEventHandler? ResetRequested;

    public SwrMeterFaceControl()
    {
        InitializeComponent();
        UpdateVisuals();
    }

    private static void OnReadingChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is SwrMeterFaceControl c) c.UpdateVisuals();
    }

    private static void OnFaultChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is SwrMeterFaceControl c) c.UpdateVisuals();
    }

    private void UpdateVisuals()
    {
        if (ResetButton == null || ReadingPanel == null || MainReading == null) return;

        if (Fault)
        {
            ResetButton.Visibility = Visibility.Visible;
            ReadingPanel.Visibility = Visibility.Collapsed;
            if (TitleText != null) TitleText.Text = "SWR FAULT";
            if (TitleText != null) TitleText.Foreground = new SolidColorBrush(Color.FromRgb(0xFF, 0x52, 0x52));
            if (StatusLine != null) StatusLine.Text = "Amp PTT open — reset when RF off";
            return;
        }

        ResetButton.Visibility = Visibility.Collapsed;
        ReadingPanel.Visibility = Visibility.Visible;
        if (TitleText != null)
        {
            TitleText.Text = ShowFwdMode ? "FWD" : "SWR";
            TitleText.Foreground = new SolidColorBrush(Color.FromRgb(0x88, 0x88, 0x88));
        }

        if (ShowFwdMode)
        {
            MainReading.Text = $"{ForwardWatts:0.0} W";
            MainReading.Foreground = new SolidColorBrush(Color.FromRgb(0x00, 0xFF, 0xAA));
        }
        else
        {
            MainReading.Text = $"{Swr:0.00}";
            bool hot = Swr >= SwrThreshold && SwrThreshold > 0;
            MainReading.Foreground = new SolidColorBrush(
                hot ? Color.FromRgb(0xFF, 0x52, 0x52) : Color.FromRgb(0xFF, 0xB7, 0x4D));
        }

        if (SubReading != null)
            SubReading.Text = $"FWD {ForwardWatts:0.0} W  REF {ReflectedWatts:0.0} W";
        if (StatusLine != null)
            StatusLine.Text = ShowFwdMode ? "Click for SWR" : "Click for FWD";
    }

    private void OnReadingClick(object sender, MouseButtonEventArgs e)
    {
        if (Fault) return;
        ShowFwdMode = !ShowFwdMode;
        e.Handled = true;
    }

    private void OnResetClick(object sender, RoutedEventArgs e)
    {
        ResetRequested?.Invoke(this, e);
    }
}
