using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace MSCC.Wpf.Controls;

/// <summary>
/// Compact analog meter for the MAIN top bar.
/// RX: S-meter (S1–S9 / +10…+60, value space 0–15).
/// TX / SWR fault: FWD power face (even 0–10 ticks, ×1/×10/×100 full-scale) with SWR in the digital box.
/// On fault: stay on power face; digital becomes red RESET (click → ResetRequested).
/// </summary>
public partial class AnalogSMeterControl : UserControl
{
    public static readonly DependencyProperty ValueProperty =
        DependencyProperty.Register(
            nameof(Value),
            typeof(int),
            typeof(AnalogSMeterControl),
            new PropertyMetadata(0, OnValueChanged));

    public static readonly DependencyProperty HoldEnabledProperty =
        DependencyProperty.Register(
            nameof(HoldEnabled),
            typeof(bool),
            typeof(AnalogSMeterControl),
            new PropertyMetadata(true, OnHoldPeakChanged));

    public static readonly DependencyProperty PeakEnabledProperty =
        DependencyProperty.Register(
            nameof(PeakEnabled),
            typeof(bool),
            typeof(AnalogSMeterControl),
            new PropertyMetadata(false, OnHoldPeakChanged));

    /// <summary>Peak needle hang time in seconds (original 1–3 s; default 2).</summary>
    public static readonly DependencyProperty PeakHoldSecondsProperty =
        DependencyProperty.Register(
            nameof(PeakHoldSeconds),
            typeof(double),
            typeof(AnalogSMeterControl),
            new PropertyMetadata(2.0));

    /// <summary>True = FWD power face (TX / fault); false = S-meter face.</summary>
    public static readonly DependencyProperty PowerModeProperty =
        DependencyProperty.Register(
            nameof(PowerMode),
            typeof(bool),
            typeof(AnalogSMeterControl),
            new PropertyMetadata(false, OnPowerModeChanged));

    public static readonly DependencyProperty ForwardWattsProperty =
        DependencyProperty.Register(
            nameof(ForwardWatts),
            typeof(double),
            typeof(AnalogSMeterControl),
            new PropertyMetadata(0.0, OnPowerReadingChanged));

    public static readonly DependencyProperty SwrProperty =
        DependencyProperty.Register(
            nameof(Swr),
            typeof(double),
            typeof(AnalogSMeterControl),
            new PropertyMetadata(1.0, OnPowerReadingChanged));

    public static readonly DependencyProperty FaultProperty =
        DependencyProperty.Register(
            nameof(Fault),
            typeof(bool),
            typeof(AnalogSMeterControl),
            new PropertyMetadata(false, OnFaultChanged));

    public static readonly DependencyProperty SwrThresholdProperty =
        DependencyProperty.Register(
            nameof(SwrThreshold),
            typeof(double),
            typeof(AnalogSMeterControl),
            new PropertyMetadata(2.0, OnPowerReadingChanged));

    public int Value
    {
        get => (int)GetValue(ValueProperty);
        set => SetValue(ValueProperty, value);
    }

    /// <summary>When true, main needle falls slowly after peaks (original Smeter_Hold_On).</summary>
    public bool HoldEnabled
    {
        get => (bool)GetValue(HoldEnabledProperty);
        set => SetValue(HoldEnabledProperty, value);
    }

    /// <summary>When true, show peak needle that latches to max then decays (original PeakHold).</summary>
    public bool PeakEnabled
    {
        get => (bool)GetValue(PeakEnabledProperty);
        set => SetValue(PeakEnabledProperty, value);
    }

    public double PeakHoldSeconds
    {
        get => (double)GetValue(PeakHoldSecondsProperty);
        set => SetValue(PeakHoldSecondsProperty, value);
    }

    public bool PowerMode
    {
        get => (bool)GetValue(PowerModeProperty);
        set => SetValue(PowerModeProperty, value);
    }

    public double ForwardWatts
    {
        get => (double)GetValue(ForwardWattsProperty);
        set => SetValue(ForwardWattsProperty, value);
    }

    public double Swr
    {
        get => (double)GetValue(SwrProperty);
        set => SetValue(SwrProperty, value);
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

    /// <summary>Raised when operator clicks red RESET on a latched SWR fault.</summary>
    public event RoutedEventHandler? ResetRequested;

    // Arc: left weak → right strong (degrees from vertical up)
    private const double StartAngleDeg = -58;
    private const double EndAngleDeg = 58;
    private const double SweepDeg = EndAngleDeg - StartAngleDeg;
    private const double MaxSUnit = 15.0;
    private const double MaxPowerUnit = 10.0;

    private Line? _needle;
    private Line? _peakNeedle;
    private TextBlock? _centerLegend;
    private bool _faceBuilt;
    private bool _builtAsPowerMode;
    private double _cx;
    private double _cy;
    private double _radius;

    /// <summary>Displayed main-needle level in current scale units, may lag raw when Hold is on.</summary>
    private double _displayLevel;
    /// <summary>Peak needle level in current scale units.</summary>
    private double _peakLevel;
    private DateTime _peakLastRiseUtc = DateTime.MinValue;
    private DispatcherTimer? _ballisticsTimer;

    /// <summary>×1 → 10 W FS, ×10 → 100 W, ×100 → 1000 W. Fixed 0–10 dial.</summary>
    private int _powerMultiplier = 1;

    /// <summary>While fault latched, freeze needle at last power (RF drops after force-RX).</summary>
    private bool _powerFrozen;
    private double _frozenDisplayLevel;
    private double _frozenPeakLevel;
    private int _frozenMultiplier = 1;

    private static readonly SolidColorBrush ReadingGreen = FreezeBrush(0x00, 0xFF, 0xAA);
    private static readonly SolidColorBrush ReadingSwrOk = FreezeBrush(0xFF, 0xB7, 0x4D);
    private static readonly SolidColorBrush ReadingSwrHot = FreezeBrush(0xFF, 0x52, 0x52);
    private static readonly SolidColorBrush ReadingFaultBg = FreezeBrush(0xB7, 0x1C, 0x1C);
    private static readonly SolidColorBrush ReadingNormalBg = FreezeBrush(0x44, 0x44, 0x44);
    private static readonly SolidColorBrush ReadingNormalBorder = FreezeBrush(0x88, 0x88, 0x88);
    private static readonly SolidColorBrush ReadingFaultBorder = FreezeBrush(0xFF, 0x52, 0x52);
    private static readonly SolidColorBrush WhiteBrush = FreezeBrush(0xFF, 0xFF, 0xFF);

    private static SolidColorBrush FreezeBrush(byte r, byte g, byte b)
    {
        var br = new SolidColorBrush(Color.FromRgb(r, g, b));
        br.Freeze();
        return br;
    }

    public AnalogSMeterControl()
    {
        InitializeComponent();
        Loaded += (_, _) =>
        {
            WireCheckBoxes();
            WireReadingClick();
            EnsureBallisticsTimer();
            BuildFace();
            if (PowerMode)
                ApplyPowerSample(force: true);
            else
                ApplyRawSample(Value, force: true);
            UpdateNeedles();
        };
        Unloaded += (_, _) => StopBallisticsTimer();
        SizeChanged += (_, _) =>
        {
            BuildFace();
            UpdateNeedles();
        };
    }

    private void WireReadingClick()
    {
        if (ReadingBorder != null)
        {
            ReadingBorder.MouseLeftButtonDown -= OnReadingClick;
            ReadingBorder.MouseLeftButtonDown += OnReadingClick;
        }
        if (ReadingText != null)
        {
            ReadingText.MouseLeftButtonDown -= OnReadingClick;
            ReadingText.MouseLeftButtonDown += OnReadingClick;
        }
    }

    private void OnReadingClick(object sender, MouseButtonEventArgs e)
    {
        if (!PowerMode || !Fault) return;
        ResetRequested?.Invoke(this, new RoutedEventArgs());
        e.Handled = true;
    }

    /// <summary>
    /// Bind HOLD / Peak checkboxes to HoldEnabled / PeakEnabled DPs so they stay in sync
    /// whether set from XAML bindings (ViewModel) or local clicks.
    /// </summary>
    private void WireCheckBoxes()
    {
        if (HoldCheckBox != null)
        {
            HoldCheckBox.IsChecked = HoldEnabled;
            HoldCheckBox.Checked -= OnHoldCheckChanged;
            HoldCheckBox.Unchecked -= OnHoldCheckChanged;
            HoldCheckBox.Checked += OnHoldCheckChanged;
            HoldCheckBox.Unchecked += OnHoldCheckChanged;
        }
        if (PeakCheckBox != null)
        {
            PeakCheckBox.IsChecked = PeakEnabled;
            PeakCheckBox.Checked -= OnPeakCheckChanged;
            PeakCheckBox.Unchecked -= OnPeakCheckChanged;
            PeakCheckBox.Checked += OnPeakCheckChanged;
            PeakCheckBox.Unchecked += OnPeakCheckChanged;
        }
    }

    private void OnHoldCheckChanged(object sender, RoutedEventArgs e)
    {
        bool on = HoldCheckBox?.IsChecked == true;
        if (HoldEnabled != on)
            HoldEnabled = on;
    }

    private void OnPeakCheckChanged(object sender, RoutedEventArgs e)
    {
        bool on = PeakCheckBox?.IsChecked == true;
        if (PeakEnabled != on)
            PeakEnabled = on;
    }

    private static void OnValueChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is AnalogSMeterControl c && !c.PowerMode)
            c.ApplyRawSample((int)e.NewValue, force: false);
    }

    private static void OnHoldPeakChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not AnalogSMeterControl c) return;
        c.EnsureBallisticsTimer();
        if (!c.HoldEnabled)
            c._displayLevel = Math.Clamp(c.CurrentRawUnits(), 0, c.ScaleMax);
        if (!c.PeakEnabled)
            c._peakLevel = 0;
        if (c.HoldCheckBox != null && c.HoldCheckBox.IsChecked != c.HoldEnabled)
            c.HoldCheckBox.IsChecked = c.HoldEnabled;
        if (c.PeakCheckBox != null && c.PeakCheckBox.IsChecked != c.PeakEnabled)
            c.PeakCheckBox.IsChecked = c.PeakEnabled;
        c.UpdateNeedles();
    }

    private static void OnPowerModeChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not AnalogSMeterControl c) return;
        bool power = (bool)e.NewValue;
        if (!power)
        {
            // Leaving power face only when not faulted (VM keeps PowerMode true while fault latched).
            c._powerFrozen = false;
            c._powerMultiplier = 1;
        }
        c.BuildFace();
        if (power)
        {
            // If fault was latched before face swapped (force-RX order), keep freeze from FWD sample.
            if (c.Fault)
                c.LatchPowerFreezeFromCurrent();
            c.ApplyPowerSample(force: true);
        }
        else
            c.ApplyRawSample(c.Value, force: true);
        c.UpdateNeedles();
    }

    private static void OnPowerReadingChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is AnalogSMeterControl c && c.PowerMode)
        {
            c.ApplyPowerSample(force: false);
            c.UpdateNeedles();
        }
    }

    private static void OnFaultChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not AnalogSMeterControl c) return;
        bool fault = (bool)e.NewValue;
        if (fault)
        {
            // Latch needle so force-RX (RF→0) does not drop the face to empty.
            // Works even if PowerMode flips true a moment later in the same reading.
            c.LatchPowerFreezeFromCurrent();
        }
        else
        {
            c._powerFrozen = false;
        }
        c.UpdateNeedles();
    }

    private void LatchPowerFreezeFromCurrent()
    {
        UpdatePowerMultiplier(ForwardWatts);
        double units = PowerMode && _displayLevel > 0.05
            ? _displayLevel
            : WattsToUnits(ForwardWatts, _powerMultiplier);
        // Prefer the higher of needle vs live watts so a late-arriving zero doesn't wipe the latch.
        double live = WattsToUnits(ForwardWatts, _powerMultiplier);
        units = Math.Max(units, live);

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
        if (fullScale <= 0) return 0;
        return Math.Clamp(watts / fullScale * MaxPowerUnit, 0, MaxPowerUnit);
    }

    private void EnsureBallisticsTimer()
    {
        if (_ballisticsTimer != null) return;
        _ballisticsTimer = new DispatcherTimer
        {
            Interval = TimeSpan.FromMilliseconds(30)
        };
        _ballisticsTimer.Tick += (_, _) => BallisticsTick();
        _ballisticsTimer.Start();
    }

    private void StopBallisticsTimer()
    {
        if (_ballisticsTimer == null) return;
        _ballisticsTimer.Stop();
        _ballisticsTimer = null;
    }

    /// <summary>
    /// Apply a new raw S-unit sample (0–15).
    /// Hold: snap up immediately; fall only via timer.
    /// Peak: track max; hang then decay via timer.
    /// </summary>
    private void ApplyRawSample(int raw, bool force)
    {
        double r = Math.Clamp(raw, 0, MaxSUnit);
        ApplyLevelSample(r, force);
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
        double r = WattsToUnits(ForwardWatts, _powerMultiplier);
        ApplyLevelSample(r, force);
    }

    private void ApplyLevelSample(double r, bool force)
    {
        r = Math.Clamp(r, 0, ScaleMax);

        if (force || !HoldEnabled)
        {
            _displayLevel = r;
        }
        else if (r >= _displayLevel)
        {
            _displayLevel = r;
        }

        if (PeakEnabled)
        {
            if (r >= _peakLevel)
            {
                _peakLevel = r;
                _peakLastRiseUtc = DateTime.UtcNow;
            }
        }
        else
        {
            _peakLevel = 0;
        }
    }

    /// <summary>
    /// Auto-range with hysteresis: ×1 (10 W), ×10 (100 W), ×100 (1000 W).
    /// Only center legend changes — dial ticks stay 0…10.
    /// </summary>
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
        else // 100
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
        double max = ScaleMax;
        double fallStep = PowerMode ? 0.18 : 0.12; // power can fall a bit faster

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
        if (PowerMode && !_powerFrozen)
        {
            int before = _powerMultiplier;
            UpdatePowerMultiplier(ForwardWatts);
            if (_powerMultiplier != before)
                changed = true;
        }

        _ = max;
        if (changed)
            UpdateNeedles();
    }

    private void BuildFace()
    {
        if (FaceCanvas == null) return;

        double w = FaceCanvas.ActualWidth;
        double h = FaceCanvas.ActualHeight;
        if (w < 10 || h < 10)
        {
            w = Math.Max(10, Width - 10);
            h = Math.Max(10, Height - 28);
        }

        FaceCanvas.Children.Clear();
        _faceBuilt = false;
        _needle = null;
        _peakNeedle = null;
        _centerLegend = null;

        _cx = w * 0.5;
        _cy = h - 3;

        double maxAngleRad = DegToRad(Math.Max(Math.Abs(StartAngleDeg), Math.Abs(EndAngleDeg)));
        double sinA = Math.Sin(maxAngleRad);
        double cosA = Math.Cos(maxAngleRad);
        const double labelInset = 15;
        const double labelHalfW = 12;
        const double edgePad = 3;

        double maxRWidth = (w * 0.5 - edgePad) / Math.Max(0.01, sinA);
        double maxRLabel = ((w * 0.5 - edgePad - labelHalfW) / Math.Max(0.01, sinA)) + labelInset;
        double maxRHeight = (h - edgePad) / Math.Max(0.01, cosA);
        double maxRHeight2 = h - edgePad;

        _radius = Math.Min(Math.Min(maxRWidth, maxRLabel), Math.Min(maxRHeight, maxRHeight2)) * 0.94;
        _radius = Math.Max(32, _radius);

        // Face disc — slightly cooler tint in power mode
        Color faceInner = PowerMode ? Color.FromRgb(0x1E, 0x28, 0x2A) : Color.FromRgb(0x26, 0x26, 0x26);
        Color faceOuter = PowerMode ? Color.FromRgb(0x0C, 0x12, 0x14) : Color.FromRgb(0x10, 0x10, 0x10);
        double faceR = _radius * 1.03;
        var face = new Ellipse
        {
            Width = faceR * 2,
            Height = faceR * 2,
            Fill = new RadialGradientBrush(faceInner, faceOuter)
            {
                GradientOrigin = new Point(0.5, 0.35),
                Center = new Point(0.5, 0.4)
            },
            Stroke = new SolidColorBrush(PowerMode ? Color.FromRgb(0x40, 0x60, 0x58) : Color.FromRgb(0x50, 0x50, 0x50)),
            StrokeThickness = 1
        };
        Canvas.SetLeft(face, _cx - face.Width / 2);
        Canvas.SetTop(face, _cy - face.Height / 2);
        FaceCanvas.Children.Add(face);

        if (PowerMode)
            BuildPowerTicksAndZones(labelInset);
        else
            BuildSmeterTicksAndZones(labelInset);

        // Center legend: "S" or "×1" / "×10" / "×100"
        _centerLegend = new TextBlock
        {
            Text = PowerMode ? FormatMultiplier(_powerMultiplier) : "S",
            FontSize = PowerMode ? 9 : 8,
            FontFamily = new FontFamily("Consolas"),
            Foreground = PowerMode ? new SolidColorBrush(Color.FromRgb(0x00, 0xCC, 0x99)) : Brushes.DimGray,
            FontWeight = FontWeights.Bold
        };
        _centerLegend.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
        Canvas.SetLeft(_centerLegend, _cx - _centerLegend.DesiredSize.Width / 2);
        Canvas.SetTop(_centerLegend, _cy - _radius * 0.42);
        FaceCanvas.Children.Add(_centerLegend);

        _peakNeedle = new Line
        {
            Stroke = new SolidColorBrush(Color.FromRgb(0xFF, 0xB0, 0x20)),
            StrokeThickness = 1.2,
            StrokeStartLineCap = PenLineCap.Round,
            StrokeEndLineCap = PenLineCap.Triangle,
            Opacity = 0.95,
            Visibility = Visibility.Collapsed
        };
        FaceCanvas.Children.Add(_peakNeedle);

        _needle = new Line
        {
            Stroke = new SolidColorBrush(Color.FromRgb(0xFF, 0x44, 0x33)),
            StrokeThickness = 1.6,
            StrokeStartLineCap = PenLineCap.Round,
            StrokeEndLineCap = PenLineCap.Triangle
        };
        FaceCanvas.Children.Add(_needle);

        var hub = new Ellipse
        {
            Width = 7,
            Height = 7,
            Fill = new SolidColorBrush(Color.FromRgb(0xC8, 0xC8, 0xC8)),
            Stroke = Brushes.DimGray,
            StrokeThickness = 0.7
        };
        Canvas.SetLeft(hub, _cx - 3.5);
        Canvas.SetTop(hub, _cy - 3.5);
        FaceCanvas.Children.Add(hub);

        _builtAsPowerMode = PowerMode;
        _faceBuilt = true;
        UpdateHoldPeakTooltips();
    }

    private void BuildSmeterTicksAndZones(double labelInset)
    {
        DrawArcZone(0, 9, Color.FromRgb(0, 150, 85), 4.0);
        DrawArcZone(9, 12, Color.FromRgb(190, 150, 20), 4.0);
        DrawArcZone(12, 15, Color.FromRgb(190, 65, 48), 4.0);
        DrawArcPath(StartAngleDeg, EndAngleDeg, _radius - 1, Brushes.Gray, 0.85);

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
        DrawArcPath(StartAngleDeg, EndAngleDeg, _radius - 1, Brushes.Gray, 0.85);

        // Major every 2 units; minor halfway
        for (int u = 0; u <= 10; u++)
        {
            bool major = u % 2 == 0;
            string label = major ? u.ToString() : "";
            DrawTickLabel(u, label, major, highlight: u == 10, labelInset, smallLabel: false);
        }
    }

    private void DrawTickLabel(int unit, string label, bool major, bool highlight, double labelInset, bool smallLabel)
    {
        double ang = UnitToAngle(unit);
        double rad = DegToRad(ang);

        double outer = _radius - 1;
        double inner = major ? _radius - 8 : _radius - 5;
        FaceCanvas.Children.Add(new Line
        {
            X1 = _cx + outer * Math.Sin(rad),
            Y1 = _cy - outer * Math.Cos(rad),
            X2 = _cx + inner * Math.Sin(rad),
            Y2 = _cy - inner * Math.Cos(rad),
            Stroke = highlight ? Brushes.White : Brushes.LightGray,
            StrokeThickness = highlight ? 1.4 : (major ? 1.0 : 0.6),
            Opacity = 0.95
        });

        if (string.IsNullOrEmpty(label))
            return;

        double lr = _radius - labelInset;
        double lx = _cx + lr * Math.Sin(rad);
        double ly = _cy - lr * Math.Cos(rad);
        var tb = new TextBlock
        {
            Text = label,
            FontSize = smallLabel ? 7 : 8,
            FontFamily = new FontFamily("Consolas"),
            Foreground = highlight ? Brushes.White : Brushes.LightGray,
            FontWeight = highlight ? FontWeights.Bold : FontWeights.Normal
        };
        tb.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
        Canvas.SetLeft(tb, lx - tb.DesiredSize.Width / 2);
        Canvas.SetTop(tb, ly - tb.DesiredSize.Height / 2);
        FaceCanvas.Children.Add(tb);
    }

    private void UpdateCenterLegend()
    {
        if (_centerLegend == null) return;
        if (!PowerMode)
        {
            _centerLegend.Text = "S";
            return;
        }
        string t = FormatMultiplier(_powerMultiplier);
        if (_centerLegend.Text == t) return;
        _centerLegend.Text = t;
        _centerLegend.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
        Canvas.SetLeft(_centerLegend, _cx - _centerLegend.DesiredSize.Width / 2);
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
            HoldCheckBox.ToolTip = PowerMode
                ? "Slow fall of the FWD power needle when power drops (client-side)."
                : "Slow fall of the main S-meter needle when signal drops (client-side). Off = needle tracks instantly.";
        }
        if (PeakCheckBox != null)
        {
            PeakCheckBox.ToolTip = PowerMode
                ? "Orange peak needle: holds highest FWD power ~2 seconds, then falls."
                : "Orange peak needle: holds the highest reading ~2 seconds, then falls (client-side).";
        }
    }

    private void DrawArcZone(int unitFrom, int unitTo, Color color, double thickness)
    {
        DrawArcPath(UnitToAngle(unitFrom), UnitToAngle(unitTo), _radius - 2,
            new SolidColorBrush(color) { Opacity = 0.85 }, thickness);
    }

    private void DrawArcPath(double startDeg, double endDeg, double r, Brush stroke, double thickness)
    {
        int steps = Math.Max(8, (int)(Math.Abs(endDeg - startDeg) / 3));
        var geo = new StreamGeometry();
        using (var ctx = geo.Open())
        {
            for (int i = 0; i <= steps; i++)
            {
                double t = i / (double)steps;
                double ang = startDeg + (endDeg - startDeg) * t;
                double rad = DegToRad(ang);
                var pt = new Point(_cx + r * Math.Sin(rad), _cy - r * Math.Cos(rad));
                if (i == 0)
                    ctx.BeginFigure(pt, false, false);
                else
                    ctx.LineTo(pt, true, false);
            }
        }
        geo.Freeze();
        FaceCanvas.Children.Add(new Path
        {
            Data = geo,
            Stroke = stroke,
            StrokeThickness = thickness,
            StrokeStartLineCap = PenLineCap.Round,
            StrokeEndLineCap = PenLineCap.Round
        });
    }

    private void UpdateNeedles()
    {
        if (!_faceBuilt || _needle == null || _builtAsPowerMode != PowerMode)
        {
            if (IsLoaded)
                BuildFace();
            if (_needle == null) return;
        }

        double tipR = _radius - 11;
        double max = ScaleMax;
        double main = Math.Clamp(_displayLevel, 0, max);
        if (_powerFrozen && PowerMode)
            main = Math.Clamp(_frozenDisplayLevel, 0, max);

        double mainRad = DegToRad(UnitToAngle(main));

        _needle.X1 = _cx;
        _needle.Y1 = _cy;
        _needle.X2 = _cx + tipR * Math.Sin(mainRad);
        _needle.Y2 = _cy - tipR * Math.Cos(mainRad);

        if (_peakNeedle != null)
        {
            double peakLevel = _powerFrozen && PowerMode ? _frozenPeakLevel : _peakLevel;
            if (PeakEnabled && peakLevel > 0.05)
            {
                double peak = Math.Clamp(peakLevel, 0, max);
                double peakRad = DegToRad(UnitToAngle(peak));
                double peakTip = tipR - 2;
                _peakNeedle.Visibility = Visibility.Visible;
                _peakNeedle.X1 = _cx;
                _peakNeedle.Y1 = _cy;
                _peakNeedle.X2 = _cx + peakTip * Math.Sin(peakRad);
                _peakNeedle.Y2 = _cy - peakTip * Math.Cos(peakRad);
            }
            else
            {
                _peakNeedle.Visibility = Visibility.Collapsed;
            }
        }

        UpdateDigitalReadout(main);
        UpdateCenterLegend();
    }

    private void UpdateDigitalReadout(double main)
    {
        if (ReadingText == null) return;

        if (!PowerMode)
        {
            ReadingText.Text = FormatSmeterReading((int)Math.Round(main));
            ReadingText.Foreground = ReadingGreen;
            ReadingText.Cursor = Cursors.Arrow;
            if (ReadingBorder != null)
            {
                ReadingBorder.Background = ReadingNormalBg;
                ReadingBorder.BorderBrush = ReadingNormalBorder;
                ReadingBorder.Cursor = Cursors.Arrow;
                ReadingBorder.ToolTip = "S-meter reading";
            }
            return;
        }

        // Power / SWR face
        if (Fault)
        {
            ReadingText.Text = "RESET";
            ReadingText.Foreground = WhiteBrush;
            ReadingText.Cursor = Cursors.Hand;
            if (ReadingBorder != null)
            {
                ReadingBorder.Background = ReadingFaultBg;
                ReadingBorder.BorderBrush = ReadingFaultBorder;
                ReadingBorder.Cursor = Cursors.Hand;
                ReadingBorder.ToolTip = "SWR fault latched — press to HTTP reset meter (RF must be off)";
            }
            return;
        }

        double swr = Swr;
        ReadingText.Text = swr < 10 ? $"{swr:0.00}" : $"{swr:0.0}";
        bool hot = SwrThreshold > 0 && swr >= SwrThreshold;
        ReadingText.Foreground = hot ? ReadingSwrHot : ReadingSwrOk;
        ReadingText.Cursor = Cursors.Arrow;
        if (ReadingBorder != null)
        {
            ReadingBorder.Background = ReadingNormalBg;
            ReadingBorder.BorderBrush = ReadingNormalBorder;
            ReadingBorder.Cursor = Cursors.Arrow;
            double fs = 10.0 * _powerMultiplier;
            ReadingBorder.ToolTip =
                $"SWR {swr:0.00}  ·  FWD {ForwardWatts:0.0} W  ·  FS {fs:0} W ({FormatMultiplier(_powerMultiplier)})";
        }
    }

    private double UnitToAngle(double unit)
    {
        double max = ScaleMax;
        double t = Math.Clamp(unit, 0, max) / max;
        return StartAngleDeg + SweepDeg * t;
    }

    private double UnitToAngle(int unit) => UnitToAngle((double)unit);

    private static double DegToRad(double deg) => deg * Math.PI / 180.0;

    private static string FormatSmeterReading(int v)
    {
        if (v <= 0) return "S0";
        if (v <= 9) return $"S{v}";
        return $"S9+{(v - 9) * 10}";
    }
}
