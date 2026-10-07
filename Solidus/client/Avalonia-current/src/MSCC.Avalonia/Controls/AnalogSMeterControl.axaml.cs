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
/// Analog S-meter face (RX). WPF AnalogSMeterControl geometry, wider sweep (±75°).
/// Value space 0–15: S1…S9, then +10…+60 (units 10–15). HOLD slow fall, Peak orange needle.
/// </summary>
public partial class AnalogSMeterControl : UserControl
{
    private const double StartAngleDeg = -75;
    private const double EndAngleDeg = 75;
    private const double SweepDeg = EndAngleDeg - StartAngleDeg;
    private const double MaxSUnit = 15.0;

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

    public double Value
    {
        get => GetValue(ValueProperty);
        set => SetValue(ValueProperty, value);
    }

    public bool HoldEnabled
    {
        get => GetValue(HoldEnabledProperty);
        set => SetValue(HoldEnabledProperty, value);
    }

    public bool PeakEnabled
    {
        get => GetValue(PeakEnabledProperty);
        set => SetValue(PeakEnabledProperty, value);
    }

    public double PeakHoldSeconds
    {
        get => GetValue(PeakHoldSecondsProperty);
        set => SetValue(PeakHoldSecondsProperty, value);
    }

    private AvLine? _needle;
    private AvLine? _peakNeedle;
    private AvEllipse? _hub;
    private bool _faceBuilt;
    private double _cx;
    private double _cy;
    private double _radius;

    private double _displayLevel;
    private double _peakLevel;
    private DateTime _peakLastRiseUtc = DateTime.MinValue;
    private DispatcherTimer? _ballisticsTimer;
    private bool _firstSample = true;

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
            ApplyRawSample(Value, force: false);
        else if (e.Property == HoldEnabledProperty || e.Property == PeakEnabledProperty)
            OnHoldPeakChanged();
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
        if (PeakCheckBox != null)
        {
            PeakCheckBox.IsCheckedChanged -= OnPeakCheckChanged;
            if (PeakCheckBox.IsChecked != PeakEnabled)
                PeakCheckBox.IsChecked = PeakEnabled;
            PeakCheckBox.IsCheckedChanged += OnPeakCheckChanged;
        }
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
            _displayLevel = Math.Clamp(Value, 0, MaxSUnit);
        if (!PeakEnabled)
            _peakLevel = 0;
        if (HoldCheckBox != null && HoldCheckBox.IsChecked != HoldEnabled)
            HoldCheckBox.IsChecked = HoldEnabled;
        if (PeakCheckBox != null && PeakCheckBox.IsChecked != PeakEnabled)
            PeakCheckBox.IsChecked = PeakEnabled;
        UpdateNeedles();
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
        double r = Math.Clamp(raw, 0, MaxSUnit);

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

        UpdateNeedles();
    }

    private void BallisticsTick()
    {
        bool changed = false;
        double raw = Math.Clamp(Value, 0, MaxSUnit);

        if (HoldEnabled && _displayLevel > raw + 0.001)
        {
            _displayLevel = Math.Max(raw, _displayLevel - 0.12);
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
                    new GradientStop(Color.FromRgb(0x26, 0x26, 0x26), 0),
                    new GradientStop(Color.FromRgb(0x10, 0x10, 0x10), 1)
                }
            },
            Stroke = new SolidColorBrush(Color.FromRgb(0x50, 0x50, 0x50)),
            StrokeThickness = 1
        };
        Canvas.SetLeft(face, _cx - face.Width / 2);
        Canvas.SetTop(face, _cy - face.Height / 2);
        FaceCanvas.Children.Add(face);

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

        var legend = new TextBlock
        {
            Text = "S",
            FontSize = 9,
            FontFamily = new FontFamily("Consolas, Courier New, monospace"),
            Foreground = new SolidColorBrush(Color.FromRgb(0x69, 0x69, 0x69)),
            FontWeight = FontWeight.Bold
        };
        Canvas.SetLeft(legend, _cx - 4);
        Canvas.SetTop(legend, _cy - _radius * 0.42);
        FaceCanvas.Children.Add(legend);

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

        _faceBuilt = true;
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
        if (!_faceBuilt || _needle == null)
        {
            BuildFace();
            if (_needle == null) return;
        }

        double tipR = _radius - 11;
        double main = Math.Clamp(_displayLevel, 0, MaxSUnit);
        double mainRad = DegToRad(UnitToAngle(main));

        _needle.StartPoint = new Point(_cx, _cy);
        _needle.EndPoint = new Point(
            _cx + tipR * Math.Sin(mainRad),
            _cy - tipR * Math.Cos(mainRad));

        if (_peakNeedle != null)
        {
            if (PeakEnabled && _peakLevel > 0.05)
            {
                double peak = Math.Clamp(_peakLevel, 0, MaxSUnit);
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

        if (ReadingText != null)
            ReadingText.Text = FormatSmeterReading((int)Math.Round(main));
    }

    private static double UnitToAngle(double unit)
    {
        double t = Math.Clamp(unit, 0, MaxSUnit) / MaxSUnit;
        return StartAngleDeg + SweepDeg * t;
    }

    private static double DegToRad(double deg) => deg * Math.PI / 180.0;

    private static string FormatSmeterReading(int v)
    {
        if (v <= 0) return "S0";
        if (v <= 9) return $"S{v}";
        return $"S9+{(v - 9) * 10}";
    }
}
