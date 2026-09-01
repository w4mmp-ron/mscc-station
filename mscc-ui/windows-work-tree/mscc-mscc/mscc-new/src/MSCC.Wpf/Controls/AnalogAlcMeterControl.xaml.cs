using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace MSCC.Wpf.Controls;

/// <summary>
/// Compact analog ALC meter for the right MAIN panel (below analog S-meter).
/// Arc scale 0–100 with main needle + optional peak needle.
/// Client-side Hold (slow fall) and Peak (peak needle with hold time), same pattern as
/// <see cref="AnalogSMeterControl"/>.
/// </summary>
public partial class AnalogAlcMeterControl : UserControl
{
    public static readonly DependencyProperty ValueProperty =
        DependencyProperty.Register(
            nameof(Value),
            typeof(int),
            typeof(AnalogAlcMeterControl),
            new PropertyMetadata(0, OnValueChanged));

    public static readonly DependencyProperty HoldEnabledProperty =
        DependencyProperty.Register(
            nameof(HoldEnabled),
            typeof(bool),
            typeof(AnalogAlcMeterControl),
            new PropertyMetadata(true, OnHoldPeakChanged));

    public static readonly DependencyProperty PeakEnabledProperty =
        DependencyProperty.Register(
            nameof(PeakEnabled),
            typeof(bool),
            typeof(AnalogAlcMeterControl),
            new PropertyMetadata(false, OnHoldPeakChanged));

    public static readonly DependencyProperty PeakHoldSecondsProperty =
        DependencyProperty.Register(
            nameof(PeakHoldSeconds),
            typeof(double),
            typeof(AnalogAlcMeterControl),
            new PropertyMetadata(2.0));

    public int Value
    {
        get => (int)GetValue(ValueProperty);
        set => SetValue(ValueProperty, value);
    }

    public bool HoldEnabled
    {
        get => (bool)GetValue(HoldEnabledProperty);
        set => SetValue(HoldEnabledProperty, value);
    }

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

    private const double StartAngleDeg = -58;
    private const double EndAngleDeg = 58;
    private const double SweepDeg = EndAngleDeg - StartAngleDeg;
    private const double MaxValue = 100.0;

    private Line? _needle;
    private Line? _peakNeedle;
    private bool _faceBuilt;
    private double _cx;
    private double _cy;
    private double _radius;

    private double _displayLevel;
    private double _peakLevel;
    private DateTime _peakLastRiseUtc = DateTime.MinValue;
    private DispatcherTimer? _ballisticsTimer;

    public AnalogAlcMeterControl()
    {
        InitializeComponent();
        Loaded += (_, _) =>
        {
            WireCheckBoxes();
            EnsureBallisticsTimer();
            BuildFace();
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
        if (d is AnalogAlcMeterControl c)
            c.ApplyRawSample((int)e.NewValue, force: false);
    }

    private static void OnHoldPeakChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not AnalogAlcMeterControl c) return;
        c.EnsureBallisticsTimer();
        if (!c.HoldEnabled)
            c._displayLevel = Math.Clamp(c.Value, 0, MaxValue);
        if (!c.PeakEnabled)
            c._peakLevel = 0;
        if (c.HoldCheckBox != null && c.HoldCheckBox.IsChecked != c.HoldEnabled)
            c.HoldCheckBox.IsChecked = c.HoldEnabled;
        if (c.PeakCheckBox != null && c.PeakCheckBox.IsChecked != c.PeakEnabled)
            c.PeakCheckBox.IsChecked = c.PeakEnabled;
        c.UpdateNeedles();
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

    private void ApplyRawSample(int raw, bool force)
    {
        double r = Math.Clamp(raw, 0, MaxValue);

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
        {
            _peakLevel = 0;
        }

        UpdateNeedles();
    }

    private void BallisticsTick()
    {
        bool changed = false;
        double raw = Math.Clamp(Value, 0, MaxValue);

        // Scale decay for 0–100 (S-meter uses ~0.12 on 0–15 ≈ 0.8/tick here)
        if (HoldEnabled && _displayLevel > raw + 0.01)
        {
            _displayLevel = Math.Max(raw, _displayLevel - 0.80);
            changed = true;
        }
        else if (!HoldEnabled && Math.Abs(_displayLevel - raw) > 0.01)
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
                if (_peakLevel > floor + 0.01)
                {
                    _peakLevel = Math.Max(floor, _peakLevel - 1.3);
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

        double faceR = _radius * 1.03;
        var face = new Ellipse
        {
            Width = faceR * 2,
            Height = faceR * 2,
            Fill = new RadialGradientBrush(
                Color.FromRgb(0x26, 0x26, 0x26),
                Color.FromRgb(0x10, 0x10, 0x10))
            {
                GradientOrigin = new Point(0.5, 0.35),
                Center = new Point(0.5, 0.4)
            },
            Stroke = new SolidColorBrush(Color.FromRgb(0x50, 0x50, 0x50)),
            StrokeThickness = 1
        };
        Canvas.SetLeft(face, _cx - face.Width / 2);
        Canvas.SetTop(face, _cy - face.Height / 2);
        FaceCanvas.Children.Add(face);

        DrawArcZone(0, 20, Color.FromRgb(0, 150, 85), 4.0);
        DrawArcZone(20, 50, Color.FromRgb(190, 150, 20), 4.0);
        DrawArcZone(50, 100, Color.FromRgb(190, 65, 48), 4.0);
        DrawArcPath(StartAngleDeg, EndAngleDeg, _radius - 1, Brushes.Gray, 0.85);

        var points = new (int unit, string label, bool major)[]
        {
            (0, "0", true),
            (10, "", false),
            (20, "", false),
            (25, "25", true),
            (30, "", false),
            (40, "", false),
            (50, "50", true),
            (60, "", false),
            (70, "", false),
            (75, "75", true),
            (80, "", false),
            (90, "", false),
            (100, "100", true),
        };

        foreach (var (unit, label, major) in points)
        {
            double ang = UnitToAngle(unit);
            double rad = DegToRad(ang);

            double outer = _radius - 1;
            double inner = major ? _radius - 8 : _radius - 5;
            bool highlight = unit is 50 or 20 or 0 or 100;
            FaceCanvas.Children.Add(new Line
            {
                X1 = _cx + outer * Math.Sin(rad),
                Y1 = _cy - outer * Math.Cos(rad),
                X2 = _cx + inner * Math.Sin(rad),
                Y2 = _cy - inner * Math.Cos(rad),
                Stroke = highlight ? Brushes.White : Brushes.LightGray,
                StrokeThickness = unit == 50 ? 1.4 : (major ? 1.0 : 0.6),
                Opacity = 0.95
            });

            if (string.IsNullOrEmpty(label))
                continue;

            double lr = _radius - labelInset;
            double lx = _cx + lr * Math.Sin(rad);
            double ly = _cy - lr * Math.Cos(rad);
            var tb = new TextBlock
            {
                Text = label,
                FontSize = unit == 100 ? 7 : 8,
                FontFamily = new FontFamily("Consolas"),
                Foreground = unit == 50 ? Brushes.White : Brushes.LightGray,
                FontWeight = unit == 50 ? FontWeights.Bold : FontWeights.Normal
            };
            tb.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
            Canvas.SetLeft(tb, lx - tb.DesiredSize.Width / 2);
            Canvas.SetTop(tb, ly - tb.DesiredSize.Height / 2);
            FaceCanvas.Children.Add(tb);
        }

        var legend = new TextBlock
        {
            Text = "ALC",
            FontSize = 8,
            FontFamily = new FontFamily("Consolas"),
            Foreground = Brushes.DimGray,
            FontWeight = FontWeights.Bold
        };
        legend.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
        Canvas.SetLeft(legend, _cx - legend.DesiredSize.Width / 2);
        Canvas.SetTop(legend, _cy - _radius * 0.42);
        FaceCanvas.Children.Add(legend);

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

        _faceBuilt = true;
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
        if (!_faceBuilt || _needle == null)
        {
            if (IsLoaded)
                BuildFace();
            if (_needle == null) return;
        }

        double tipR = _radius - 11;
        double main = Math.Clamp(_displayLevel, 0, MaxValue);
        double mainRad = DegToRad(UnitToAngle(main));

        _needle.X1 = _cx;
        _needle.Y1 = _cy;
        _needle.X2 = _cx + tipR * Math.Sin(mainRad);
        _needle.Y2 = _cy - tipR * Math.Cos(mainRad);

        if (_peakNeedle != null)
        {
            if (PeakEnabled && _peakLevel > 0.3)
            {
                double peak = Math.Clamp(_peakLevel, 0, MaxValue);
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

        if (ReadingText != null)
            ReadingText.Text = ((int)Math.Round(main)).ToString();
    }

    private static double UnitToAngle(double unit)
    {
        double t = Math.Clamp(unit, 0, MaxValue) / MaxValue;
        return StartAngleDeg + SweepDeg * t;
    }

    private static double DegToRad(double deg) => deg * Math.PI / 180.0;
}
