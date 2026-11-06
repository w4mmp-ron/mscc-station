using Avalonia;
using Avalonia.Controls;
using Avalonia.Data;
using Avalonia.Interactivity;
using Avalonia.Media;
using Avalonia.Threading;
using AvLine = Avalonia.Controls.Shapes.Line;
using AvEllipse = Avalonia.Controls.Shapes.Ellipse;
using AvPath = Avalonia.Controls.Shapes.Path;

namespace MSCC.Avalonia.Controls;

/// <summary>
/// Analog meter for the MAIN top bar. WPF AnalogSMeterControl geometry, wider sweep (±75°).
/// RX: S-meter, value space 0–15 (S1…S9, then +10…+60). HOLD slow fall, Peak orange needle.
/// TX / SWR fault (WPF 11.x port): FWD power face (even 0–10 ticks, ×1/×10/×100 full scale) with
/// SWR in the digital box. On fault the face stays on power and the digital box becomes a red
/// RESET (click → ResetRequested).
/// </summary>
public partial class AnalogSMeterControl : UserControl
{
    private const double StartAngleDeg = -75;
    private const double EndAngleDeg = 75;
    private const double SweepDeg = EndAngleDeg - StartAngleDeg;
    private const double MaxSUnit = 15.0;
    private const double MaxPowerUnit = 10.0;

    public static readonly StyledProperty<double> ValueProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, double>(nameof(Value), 0.0);

    public static readonly StyledProperty<bool> HoldEnabledProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, bool>(
            nameof(HoldEnabled), true, defaultBindingMode: BindingMode.TwoWay);

    public static readonly StyledProperty<bool> PeakEnabledProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, bool>(
            nameof(PeakEnabled), false, defaultBindingMode: BindingMode.TwoWay);

    public static readonly StyledProperty<double> PeakHoldSecondsProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, double>(nameof(PeakHoldSeconds), 2.0);

    /// <summary>cmd-070: last S-meter report in whole dBm.</summary>
    public static readonly StyledProperty<int> DbmProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, int>(nameof(Dbm), -130);

    /// <summary>cmd-070: readout in dBm (true) or S units (false). Click toggles (RX face only).</summary>
    public static readonly StyledProperty<bool> ShowDbmProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, bool>(
            nameof(ShowDbm), false, defaultBindingMode: BindingMode.TwoWay);

    /// <summary>True = FWD power face (TX / fault); false = S-meter face.</summary>
    public static readonly StyledProperty<bool> PowerModeProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, bool>(nameof(PowerMode), false);

    public static readonly StyledProperty<double> ForwardWattsProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, double>(nameof(ForwardWatts), 0.0);

    public static readonly StyledProperty<double> SwrProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, double>(nameof(Swr), 1.0);

    public static readonly StyledProperty<bool> FaultProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, bool>(nameof(Fault), false);

    public static readonly StyledProperty<double> SwrThresholdProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, double>(nameof(SwrThreshold), 2.0);

    public double Value { get => GetValue(ValueProperty); set => SetValue(ValueProperty, value); }
    public bool HoldEnabled { get => GetValue(HoldEnabledProperty); set => SetValue(HoldEnabledProperty, value); }
    public bool PeakEnabled { get => GetValue(PeakEnabledProperty); set => SetValue(PeakEnabledProperty, value); }
    public double PeakHoldSeconds { get => GetValue(PeakHoldSecondsProperty); set => SetValue(PeakHoldSecondsProperty, value); }
    public int Dbm { get => GetValue(DbmProperty); set => SetValue(DbmProperty, value); }
    public bool ShowDbm { get => GetValue(ShowDbmProperty); set => SetValue(ShowDbmProperty, value); }
    public bool PowerMode { get => GetValue(PowerModeProperty); set => SetValue(PowerModeProperty, value); }
    public double ForwardWatts { get => GetValue(ForwardWattsProperty); set => SetValue(ForwardWattsProperty, value); }
    public double Swr { get => GetValue(SwrProperty); set => SetValue(SwrProperty, value); }
    public bool Fault { get => GetValue(FaultProperty); set => SetValue(FaultProperty, value); }
    public double SwrThreshold { get => GetValue(SwrThresholdProperty); set => SetValue(SwrThresholdProperty, value); }

    /// <summary>Raised when the operator clicks the red RESET on a latched SWR fault.</summary>
    public event EventHandler? ResetRequested;

    private AvLine? _needle;
    private AvLine? _peakNeedle;
    private AvEllipse? _hub;
    private TextBlock? _centerLegend;
    private bool _faceBuilt;
    private bool _builtAsPowerMode;
    private double _cx;
    private double _cy;
    private double _radius;

    private double _displayLevel;
    private double _peakLevel;
    private DateTime _peakLastRiseUtc = DateTime.MinValue;
    private DispatcherTimer? _ballisticsTimer;
    private bool _firstSample = true;

    /// <summary>×1 → 10 W FS, ×10 → 100 W, ×100 → 1000 W. Fixed 0–10 dial.</summary>
    private int _powerMultiplier = 1;

    /// <summary>While a fault is latched, freeze the needle at the last power (RF drops after force-RX).</summary>
    private bool _powerFrozen;
    private double _frozenDisplayLevel;
    private double _frozenPeakLevel;
    private int _frozenMultiplier = 1;

    private static readonly IBrush ReadingGreen = new SolidColorBrush(Color.FromRgb(0x00, 0xFF, 0xAA));
    private static readonly IBrush ReadingSwrOk = new SolidColorBrush(Color.FromRgb(0xFF, 0xB7, 0x4D));
    private static readonly IBrush ReadingSwrHot = new SolidColorBrush(Color.FromRgb(0xFF, 0x52, 0x52));
    private static readonly IBrush ReadingFaultBg = new SolidColorBrush(Color.FromRgb(0xB7, 0x1C, 0x1C));
    private static readonly IBrush ReadingNormalBg = new SolidColorBrush(Color.FromRgb(0x44, 0x44, 0x44));
    private static readonly IBrush ReadingNormalBorder = new SolidColorBrush(Color.FromRgb(0x88, 0x88, 0x88));
    private static readonly IBrush ReadingFaultBorder = new SolidColorBrush(Color.FromRgb(0xFF, 0x52, 0x52));

    public AnalogSMeterControl()
    {
        InitializeComponent();
        AttachedToVisualTree += OnAttached;
        DetachedFromVisualTree += OnDetached;
        SizeChanged += (_, _) => QueueBuildFace(forceSample: false);
        PropertyChanged += OnPropChanged;
    }

    private void OnAttached(object? sender, VisualTreeAttachmentEventArgs e)
    {
        WireCheckBoxes();
        EnsureBallisticsTimer();
        _firstSample = true;
        QueueBuildFace(forceSample: true);
    }

    private void OnDetached(object? sender, VisualTreeAttachmentEventArgs e) =>
        StopBallisticsTimer();

    private void OnPropChanged(object? sender, AvaloniaPropertyChangedEventArgs e)
    {
        if (e.Property == ValueProperty)
        {
            if (!PowerMode)
                ApplyRawSample(Value, force: false);
        }
        else if (e.Property == HoldEnabledProperty || e.Property == PeakEnabledProperty)
            OnHoldPeakChanged();
        else if (e.Property == DbmProperty || e.Property == ShowDbmProperty)
        {
            if (!PowerMode) UpdateNeedles();
        }
        else if (e.Property == PowerModeProperty)
            OnPowerModeChanged(PowerMode);
        else if (e.Property == ForwardWattsProperty || e.Property == SwrProperty
                 || e.Property == SwrThresholdProperty)
        {
            if (PowerMode)
            {
                ApplyPowerSample(force: false);
                UpdateNeedles();
            }
        }
        else if (e.Property == FaultProperty)
            OnFaultChanged(Fault);
    }

    private void WireCheckBoxes()
    {
        if (HoldCheckBox != null)
        {
            HoldCheckBox.IsCheckedChanged -= OnHoldCheckChanged;
            if (HoldCheckBox.IsChecked != HoldEnabled)
                HoldCheckBox.IsChecked = HoldEnabled;
            HoldCheckBox.IsCheckedChanged += OnHoldCheckChanged;
        }
        if (ReadingBorder != null)
        {
            ReadingBorder.PointerPressed -= OnReadingPressed;
            ReadingBorder.PointerPressed += OnReadingPressed;
        }
        if (PeakCheckBox != null)
        {
            PeakCheckBox.IsCheckedChanged -= OnPeakCheckChanged;
            if (PeakCheckBox.IsChecked != PeakEnabled)
                PeakCheckBox.IsChecked = PeakEnabled;
            PeakCheckBox.IsCheckedChanged += OnPeakCheckChanged;
        }
    }

    /// <summary>RX face: toggle S units / dBm. Power face with a fault: RESET.</summary>
    private void OnReadingPressed(object? sender, global::Avalonia.Input.PointerPressedEventArgs e)
    {
        if (!PowerMode)
        {
            ShowDbm = !ShowDbm;
            e.Handled = true;
            return;
        }
        if (!Fault) return;
        ResetRequested?.Invoke(this, EventArgs.Empty);
        e.Handled = true;
    }

    private void OnHoldCheckChanged(object? sender, RoutedEventArgs e)
    {
        bool on = HoldCheckBox?.IsChecked == true;
        if (HoldEnabled != on)
            HoldEnabled = on;
    }

    private void OnPeakCheckChanged(object? sender, RoutedEventArgs e)
    {
        bool on = PeakCheckBox?.IsChecked == true;
        if (PeakEnabled != on)
            PeakEnabled = on;
    }

    private void OnHoldPeakChanged()
    {
        EnsureBallisticsTimer();
        if (!HoldEnabled)
            _displayLevel = Math.Clamp(CurrentRawUnits(), 0, ScaleMax);
        if (!PeakEnabled)
            _peakLevel = 0;
        if (HoldCheckBox != null && HoldCheckBox.IsChecked != HoldEnabled)
            HoldCheckBox.IsChecked = HoldEnabled;
        if (PeakCheckBox != null && PeakCheckBox.IsChecked != PeakEnabled)
            PeakCheckBox.IsChecked = PeakEnabled;
        UpdateNeedles();
    }

    private void OnPowerModeChanged(bool power)
    {
        if (!power)
        {
            // Leaving the power face only when not faulted (the VM keeps PowerMode true while latched).
            _powerFrozen = false;
            _powerMultiplier = 1;
        }
        BuildFace();
        if (power)
        {
            // Fault latched before the face swapped (force-RX order): keep the freeze.
            if (Fault)
                LatchPowerFreezeFromCurrent();
            ApplyPowerSample(force: true);
        }
        else
            ApplyRawSample(Value, force: true);
        UpdateNeedles();
    }

    private void OnFaultChanged(bool fault)
    {
        if (fault)
            LatchPowerFreezeFromCurrent();   // force-RX (RF → 0) must not empty the face
        else
            _powerFrozen = false;
        UpdateNeedles();
    }

    private void LatchPowerFreezeFromCurrent()
    {
        UpdatePowerMultiplier(ForwardWatts);
        double live = WattsToUnits(ForwardWatts, _powerMultiplier);
        double units = PowerMode && _displayLevel > 0.05 ? _displayLevel : live;
        units = Math.Max(units, live);   // a late zero must not wipe the latch

        _powerFrozen = true;
        _frozenDisplayLevel = units;
        _frozenPeakLevel = Math.Max(_peakLevel, units);
        _frozenMultiplier = _powerMultiplier;
        _displayLevel = _frozenDisplayLevel;
        _peakLevel = _frozenPeakLevel;
    }

    private double ScaleMax => PowerMode ? MaxPowerUnit : MaxSUnit;

    private double CurrentRawUnits()
    {
        if (!PowerMode)
            return Math.Clamp(Value, 0, MaxSUnit);
        if (_powerFrozen)
            return _frozenDisplayLevel;
        return WattsToUnits(ForwardWatts, _powerMultiplier);
    }

    private static double WattsToUnits(double watts, int multiplier)
    {
        double fullScale = 10.0 * Math.Max(1, multiplier);
        return Math.Clamp(watts / fullScale * MaxPowerUnit, 0, MaxPowerUnit);
    }

    private void EnsureBallisticsTimer()
    {
        if (_ballisticsTimer != null) return;
        _ballisticsTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(30) };
        _ballisticsTimer.Tick += (_, _) => BallisticsTick();
        _ballisticsTimer.Start();
    }

    private void StopBallisticsTimer()
    {
        if (_ballisticsTimer == null) return;
        _ballisticsTimer.Stop();
        _ballisticsTimer = null;
    }

    private void ApplyRawSample(double raw, bool force)
    {
        ApplyLevelSample(Math.Clamp(raw, 0, MaxSUnit), force);
        UpdateNeedles();
    }

    private void ApplyPowerSample(bool force)
    {
        if (_powerFrozen)
        {
            _displayLevel = _frozenDisplayLevel;
            _peakLevel = _frozenPeakLevel;
            _powerMultiplier = _frozenMultiplier;
            UpdateCenterLegend();
            return;
        }

        UpdatePowerMultiplier(ForwardWatts);
        ApplyLevelSample(WattsToUnits(ForwardWatts, _powerMultiplier), force);
    }

    private void ApplyLevelSample(double r, bool force)
    {
        r = Math.Clamp(r, 0, ScaleMax);

        if (force || !HoldEnabled)
            _displayLevel = r;
        else if (r >= _displayLevel)
            _displayLevel = r;

        if (PeakEnabled)
        {
            if (r >= _peakLevel)
            {
                _peakLevel = r;
                _peakLastRiseUtc = DateTime.UtcNow;
            }
        }
        else
            _peakLevel = 0;
    }

    /// <summary>Auto-range with hysteresis: ×1 (10 W), ×10 (100 W), ×100 (1000 W). Dial stays 0…10.</summary>
    private void UpdatePowerMultiplier(double watts)
    {
        int prev = _powerMultiplier;
        if (_powerMultiplier <= 1)
        {
            if (watts >= 9.5)
                _powerMultiplier = 10;
        }
        else if (_powerMultiplier == 10)
        {
            if (watts >= 95)
                _powerMultiplier = 100;
            else if (watts < 3.0)
                _powerMultiplier = 1;
        }
        else
        {
            if (watts < 30)
                _powerMultiplier = 10;
        }

        if (_powerMultiplier != prev)
            UpdateCenterLegend();
    }

    private void BallisticsTick()
    {
        if (_powerFrozen)
            return;

        bool changed = false;
        double raw = CurrentRawUnits();
        double fallStep = PowerMode ? 0.18 : 0.12;   // power falls a bit faster

        if (HoldEnabled && _displayLevel > raw + 0.001)
        {
            _displayLevel = Math.Max(raw, _displayLevel - fallStep);
            changed = true;
        }
        else if (!HoldEnabled && Math.Abs(_displayLevel - raw) > 0.001)
        {
            _displayLevel = raw;
            changed = true;
        }

        if (PeakEnabled)
        {
            double hang = Math.Clamp(PeakHoldSeconds, 0.5, 5.0);
            if ((DateTime.UtcNow - _peakLastRiseUtc).TotalSeconds >= hang)
            {
                double floor = Math.Max(raw, _displayLevel);
                if (_peakLevel > floor + 0.001)
                {
                    _peakLevel = Math.Max(floor, _peakLevel - 0.20);
                    changed = true;
                }
            }
            if (_peakLevel < _displayLevel)
            {
                _peakLevel = _displayLevel;
                changed = true;
            }
        }
        else if (_peakLevel > 0)
        {
            _peakLevel = 0;
            changed = true;
        }

        // Keep auto-range alive between samples (UDP rate)
        if (PowerMode)
        {
            int before = _powerMultiplier;
            UpdatePowerMultiplier(ForwardWatts);
            if (_powerMultiplier != before)
                changed = true;
        }

        if (changed)
            UpdateNeedles();
    }

    private void QueueBuildFace(bool forceSample)
    {
        Dispatcher.UIThread.Post(() =>
        {
            BuildFace();
            if (forceSample || _firstSample)
            {
                _firstSample = false;
                if (PowerMode)
                {
                    ApplyPowerSample(force: true);
                    UpdateNeedles();
                }
                else
                    ApplyRawSample(Value, force: true);
            }
            else
                UpdateNeedles();
        }, DispatcherPriority.Loaded);
    }

    private void BuildFace()
    {
        if (FaceCanvas == null) return;

        double w = FaceCanvas.Bounds.Width;
        double h = FaceCanvas.Bounds.Height;
        if (w < 10 || h < 10)
        {
            w = Math.Max(10, Bounds.Width - 10);
            h = Math.Max(10, Bounds.Height - 28);
        }

        FaceCanvas.Children.Clear();
        _needle = null;
        _peakNeedle = null;
        _hub = null;
        _centerLegend = null;
        _faceBuilt = false;

        _cx = w * 0.5;
        _cy = h - 3;

        double maxAngleRad = DegToRad(Math.Max(Math.Abs(StartAngleDeg), Math.Abs(EndAngleDeg)));
        double sinA = Math.Sin(maxAngleRad);
        double cosA = Math.Cos(maxAngleRad);
        const double labelInset = 15;
        const double labelHalfW = 14;
        const double edgePad = 3;

        double maxRWidth = (w * 0.5 - edgePad) / Math.Max(0.01, sinA);
        double maxRLabel = ((w * 0.5 - edgePad - labelHalfW) / Math.Max(0.01, sinA)) + labelInset;
        double maxRHeight = (h - edgePad) / Math.Max(0.01, cosA);
        double maxRHeight2 = h - edgePad;

        _radius = Math.Min(Math.Min(maxRWidth, maxRLabel), Math.Min(maxRHeight, maxRHeight2)) * 0.98;
        _radius = Math.Max(32, _radius);

        bool power = PowerMode;
        // Face disc — slightly cooler tint in power mode (WPF)
        Color faceInner = power ? Color.FromRgb(0x1E, 0x28, 0x2A) : Color.FromRgb(0x26, 0x26, 0x26);
        Color faceOuter = power ? Color.FromRgb(0x0C, 0x12, 0x14) : Color.FromRgb(0x10, 0x10, 0x10);
        double faceR = _radius * 1.03;
        var face = new AvEllipse
        {
            Width = faceR * 2,
            Height = faceR * 2,
            Fill = new RadialGradientBrush
            {
                GradientOrigin = new RelativePoint(0.5, 0.35, RelativeUnit.Relative),
                Center = new RelativePoint(0.5, 0.4, RelativeUnit.Relative),
                GradientStops =
                {
                    new GradientStop(faceInner, 0),
                    new GradientStop(faceOuter, 1)
                }
            },
            Stroke = new SolidColorBrush(power ? Color.FromRgb(0x40, 0x60, 0x58) : Color.FromRgb(0x50, 0x50, 0x50)),
            StrokeThickness = 1
        };
        Canvas.SetLeft(face, _cx - face.Width / 2);
        Canvas.SetTop(face, _cy - face.Height / 2);
        FaceCanvas.Children.Add(face);

        if (power)
            BuildPowerTicksAndZones(labelInset);
        else
            BuildSmeterTicksAndZones(labelInset);

        // Center legend: "S" or "×1" / "×10" / "×100"
        _centerLegend = new TextBlock
        {
            Text = power ? FormatMultiplier(_powerMultiplier) : "S",
            FontSize = 9,
            FontFamily = new FontFamily("Consolas, Courier New, monospace"),
            Foreground = power
                ? new SolidColorBrush(Color.FromRgb(0x00, 0xCC, 0x99))
                : new SolidColorBrush(Color.FromRgb(0x69, 0x69, 0x69)),
            FontWeight = FontWeight.Bold
        };
        FaceCanvas.Children.Add(_centerLegend);
        PlaceCenterLegend();

        _peakNeedle = new AvLine
        {
            Stroke = new SolidColorBrush(Color.FromRgb(0xFF, 0xB0, 0x20)),
            StrokeThickness = 1.2,
            StrokeLineCap = PenLineCap.Round,
            Opacity = 0.95,
            IsVisible = false
        };
        FaceCanvas.Children.Add(_peakNeedle);

        _needle = new AvLine
        {
            Stroke = new SolidColorBrush(Color.FromRgb(0xFF, 0x44, 0x33)),
            StrokeThickness = 1.6,
            StrokeLineCap = PenLineCap.Round
        };
        FaceCanvas.Children.Add(_needle);

        _hub = new AvEllipse
        {
            Width = 7,
            Height = 7,
            Fill = new SolidColorBrush(Color.FromRgb(0xC8, 0xC8, 0xC8)),
            Stroke = new SolidColorBrush(Color.FromRgb(0x69, 0x69, 0x69)),
            StrokeThickness = 0.7
        };
        Canvas.SetLeft(_hub, _cx - 3.5);
        Canvas.SetTop(_hub, _cy - 3.5);
        FaceCanvas.Children.Add(_hub);

        _builtAsPowerMode = power;
        _faceBuilt = true;
        UpdateHoldPeakTooltips();
    }

    private void BuildSmeterTicksAndZones(double labelInset)
    {
        DrawArcZone(0, 9, Color.FromRgb(0, 150, 85), 4.0);
        DrawArcZone(9, 12, Color.FromRgb(190, 150, 20), 4.0);
        DrawArcZone(12, 15, Color.FromRgb(190, 65, 48), 4.0);
        DrawArcPath(StartAngleDeg, EndAngleDeg, _radius - 1,
            new SolidColorBrush(Colors.Gray) { Opacity = 0.85 }, 0.85);

        var points = new (int unit, string label, bool major)[]
        {
            (1, "1", true),
            (3, "3", true),
            (5, "5", true),
            (7, "7", true),
            (9, "9", true),
            (10, "+10", true),
            (11, "", false),
            (12, "+30", true),
            (13, "", false),
            (14, "", false),
            (15, "+60", true),
        };

        foreach (var (unit, label, major) in points)
            DrawTickLabel(unit, label, major, highlight: unit == 9, labelInset, smallLabel: unit >= 10);
    }

    private void BuildPowerTicksAndZones(double labelInset)
    {
        // Even 0–10 scale: green low, yellow mid, red high
        DrawArcZone(0, 6, Color.FromRgb(0, 150, 85), 4.0);
        DrawArcZone(6, 8, Color.FromRgb(190, 150, 20), 4.0);
        DrawArcZone(8, 10, Color.FromRgb(190, 65, 48), 4.0);
        DrawArcPath(StartAngleDeg, EndAngleDeg, _radius - 1,
            new SolidColorBrush(Colors.Gray) { Opacity = 0.85 }, 0.85);

        // Major every 2 units; minor halfway
        for (int u = 0; u <= 10; u++)
        {
            bool major = u % 2 == 0;
            DrawTickLabel(u, major ? u.ToString() : "", major, highlight: u == 10, labelInset, smallLabel: false);
        }
    }

    private void DrawTickLabel(int unit, string label, bool major, bool highlight, double labelInset, bool smallLabel)
    {
        double ang = UnitToAngle(unit);
        double rad = DegToRad(ang);
        double outer = _radius - 1;
        double inner = major ? _radius - 8 : _radius - 5;

        FaceCanvas.Children.Add(new AvLine
        {
            StartPoint = new Point(_cx + outer * Math.Sin(rad), _cy - outer * Math.Cos(rad)),
            EndPoint = new Point(_cx + inner * Math.Sin(rad), _cy - inner * Math.Cos(rad)),
            Stroke = highlight
                ? Brushes.White
                : new SolidColorBrush(Color.FromRgb(0xD0, 0xD0, 0xD0)),
            StrokeThickness = highlight ? 1.4 : (major ? 1.0 : 0.6),
            Opacity = 0.95
        });

        if (string.IsNullOrEmpty(label))
            return;

        double lr = _radius - labelInset;
        double lx = _cx + lr * Math.Sin(rad);
        double ly = _cy - lr * Math.Cos(rad);
        double fontSize = smallLabel ? 7 : 8;
        var tb = new TextBlock
        {
            Text = label,
            FontSize = fontSize,
            FontFamily = new FontFamily("Consolas, Courier New, monospace"),
            Foreground = highlight ? Brushes.White : new SolidColorBrush(Color.FromRgb(0xD0, 0xD0, 0xD0)),
            FontWeight = highlight ? FontWeight.Bold : FontWeight.Normal
        };
        double tw = label.Length * (smallLabel ? 4.0 : 4.8);
        double th = 10;
        Canvas.SetLeft(tb, lx - tw / 2);
        Canvas.SetTop(tb, ly - th / 2);
        FaceCanvas.Children.Add(tb);
    }

    private void PlaceCenterLegend()
    {
        if (_centerLegend == null) return;
        double tw = (_centerLegend.Text?.Length ?? 1) * 5.0;
        Canvas.SetLeft(_centerLegend, _cx - tw / 2);
        Canvas.SetTop(_centerLegend, _cy - _radius * 0.42);
    }

    private void UpdateCenterLegend()
    {
        if (_centerLegend == null) return;
        string t = PowerMode ? FormatMultiplier(_powerMultiplier) : "S";
        if (_centerLegend.Text == t) return;
        _centerLegend.Text = t;
        PlaceCenterLegend();
    }

    private static string FormatMultiplier(int m) => m switch
    {
        100 => "×100",
        10 => "×10",
        _ => "×1"
    };

    private void UpdateHoldPeakTooltips()
    {
        if (HoldCheckBox != null)
        {
            ToolTip.SetTip(HoldCheckBox, PowerMode
                ? "Slow fall of the FWD power needle when power drops (client-side)."
                : "Slow fall of the main S-meter needle when signal drops (client-side). Off = needle tracks instantly.");
        }
        if (PeakCheckBox != null)
        {
            ToolTip.SetTip(PeakCheckBox, PowerMode
                ? "Orange peak needle: holds highest FWD power ~2 seconds, then falls."
                : "Orange peak needle: holds the highest reading ~2 seconds, then falls (client-side).");
        }
    }

    private void DrawArcZone(int unitFrom, int unitTo, Color color, double thickness)
    {
        DrawArcPath(UnitToAngle(unitFrom), UnitToAngle(unitTo), _radius - 2,
            new SolidColorBrush(color) { Opacity = 0.85 }, thickness);
    }

    private void DrawArcPath(double startDeg, double endDeg, double r, IBrush stroke, double thickness)
    {
        int steps = Math.Max(8, (int)(Math.Abs(endDeg - startDeg) / 3));
        var points = new List<Point>();
        for (int i = 0; i <= steps; i++)
        {
            double t = i / (double)steps;
            double ang = startDeg + (endDeg - startDeg) * t;
            double rad = DegToRad(ang);
            points.Add(new Point(_cx + r * Math.Sin(rad), _cy - r * Math.Cos(rad)));
        }

        if (points.Count < 2) return;

        var fig = new PathFigure
        {
            StartPoint = points[0],
            IsClosed = false,
            IsFilled = false
        };
        var poly = new PolyLineSegment();
        for (int i = 1; i < points.Count; i++)
            poly.Points.Add(points[i]);
        fig.Segments!.Add(poly);

        var geo = new PathGeometry();
        geo.Figures!.Add(fig);

        FaceCanvas.Children.Add(new AvPath
        {
            Data = geo,
            Stroke = stroke,
            StrokeThickness = thickness,
            StrokeLineCap = PenLineCap.Round,
            StrokeJoin = PenLineJoin.Round
        });
    }

    private void UpdateNeedles()
    {
        if (!_faceBuilt || _needle == null || _builtAsPowerMode != PowerMode)
        {
            BuildFace();
            if (_needle == null) return;
        }

        double tipR = _radius - 11;
        double max = ScaleMax;
        double main = Math.Clamp(_displayLevel, 0, max);
        if (_powerFrozen && PowerMode)
            main = Math.Clamp(_frozenDisplayLevel, 0, max);
        double mainRad = DegToRad(UnitToAngle(main));

        _needle.StartPoint = new Point(_cx, _cy);
        _needle.EndPoint = new Point(
            _cx + tipR * Math.Sin(mainRad),
            _cy - tipR * Math.Cos(mainRad));

        if (_peakNeedle != null)
        {
            double peakLevel = _powerFrozen && PowerMode ? _frozenPeakLevel : _peakLevel;
            if (PeakEnabled && peakLevel > 0.05)
            {
                double peak = Math.Clamp(peakLevel, 0, max);
                double peakRad = DegToRad(UnitToAngle(peak));
                double peakTip = tipR - 2;
                _peakNeedle.IsVisible = true;
                _peakNeedle.StartPoint = new Point(_cx, _cy);
                _peakNeedle.EndPoint = new Point(
                    _cx + peakTip * Math.Sin(peakRad),
                    _cy - peakTip * Math.Cos(peakRad));
            }
            else
                _peakNeedle.IsVisible = false;
        }

        UpdateDigitalReadout(main);
        UpdateCenterLegend();
    }

    private void UpdateDigitalReadout(double main)
    {
        if (ReadingText == null) return;

        if (!PowerMode)
        {
            ReadingText.Text = ShowDbm ? $"{Dbm} dBm" : FormatSmeterReading(ReadoutUnits(main));
            ReadingText.Foreground = ReadingGreen;
            if (ReadingBorder != null)
            {
                ReadingBorder.Background = ReadingNormalBg;
                ReadingBorder.BorderBrush = ReadingNormalBorder;
                ReadingBorder.Cursor = new global::Avalonia.Input.Cursor(global::Avalonia.Input.StandardCursorType.Hand);
                ToolTip.SetTip(ReadingBorder, ShowDbm
                    ? "S-meter in dBm (click for S units)"
                    : "S-meter reading (click for dBm)");
            }
            return;
        }

        // Power / SWR face
        if (Fault)
        {
            ReadingText.Text = "RESET";
            ReadingText.Foreground = Brushes.White;
            if (ReadingBorder != null)
            {
                ReadingBorder.Background = ReadingFaultBg;
                ReadingBorder.BorderBrush = ReadingFaultBorder;
                ReadingBorder.Cursor = new global::Avalonia.Input.Cursor(global::Avalonia.Input.StandardCursorType.Hand);
                ToolTip.SetTip(ReadingBorder, "SWR fault latched — press to HTTP reset meter (RF must be off)");
            }
            return;
        }

        double swr = Swr;
        ReadingText.Text = swr < 10 ? $"{swr:0.00}" : $"{swr:0.0}";
        bool hot = SwrThreshold > 0 && swr >= SwrThreshold;
        ReadingText.Foreground = hot ? ReadingSwrHot : ReadingSwrOk;
        if (ReadingBorder != null)
        {
            ReadingBorder.Background = ReadingNormalBg;
            ReadingBorder.BorderBrush = ReadingNormalBorder;
            ReadingBorder.Cursor = new global::Avalonia.Input.Cursor(global::Avalonia.Input.StandardCursorType.Arrow);
            double fs = 10.0 * _powerMultiplier;
            ToolTip.SetTip(ReadingBorder,
                $"SWR {swr:0.00}  ·  FWD {ForwardWatts:0.0} W  ·  FS {fs:0} W ({FormatMultiplier(_powerMultiplier)})");
        }
    }

    private double UnitToAngle(double unit)
    {
        double max = ScaleMax;
        double t = Math.Clamp(unit, 0, max) / max;
        return StartAngleDeg + SweepDeg * t;
    }

    private static double DegToRad(double deg) => deg * Math.PI / 180.0;

    private static string FormatSmeterReading(int v)
    {
        if (v <= 0) return "S0";
        if (v <= 9) return $"S{v}";
        return $"S9+{(v - 9) * 10}";
    }

    /// <summary>
    /// cmd-073: whole S unit for the readout from the fractional needle. Nearest unit, a tie
    /// goes up (same as DbToSmeter): −76…−73 = S9, −77 = S8; above S9 nearest 10 dB (−68 = S9+10).
    /// </summary>
    private static int ReadoutUnits(double main)
    {
        int u = (int)Math.Floor(main + 0.5);
        if (main > 0.0 && u < 1) u = 1;
        return Math.Clamp(u, 0, (int)MaxSUnit);
    }
}
