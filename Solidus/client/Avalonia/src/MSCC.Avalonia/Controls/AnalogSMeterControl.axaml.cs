using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Threading;
using AvLine = Avalonia.Controls.Shapes.Line;
using AvEllipse = Avalonia.Controls.Shapes.Ellipse;
using AvPath = Avalonia.Controls.Shapes.Path;

namespace MSCC.Avalonia.Controls;

/// <summary>
/// Analog S-meter face (RX). WPF AnalogSMeterControl geometry, wider sweep (±75°).
/// Value space 0–15: S1…S9, then +10…+60 (units 10–15). Live via MainViewModel.SMeter.
/// </summary>
public partial class AnalogSMeterControl : UserControl
{
    private const double StartAngleDeg = -75;
    private const double EndAngleDeg = 75;
    private const double SweepDeg = EndAngleDeg - StartAngleDeg;
    private const double MaxSUnit = 15.0;

    public static readonly StyledProperty<double> ValueProperty =
        AvaloniaProperty.Register<AnalogSMeterControl, double>(nameof(Value), 0.0);

    /// <summary>S-meter units 0–15 (WPF). Needle follows when set.</summary>
    public double Value
    {
        get => GetValue(ValueProperty);
        set => SetValue(ValueProperty, value);
    }

    private AvLine? _needle;
    private AvEllipse? _hub;
    private bool _faceBuilt;
    private double _cx;
    private double _cy;
    private double _radius;

    public AnalogSMeterControl()
    {
        InitializeComponent();
        AttachedToVisualTree += (_, _) => QueueBuildFace();
        SizeChanged += (_, _) => QueueBuildFace();
        PropertyChanged += OnPropChanged;
    }

    private void OnPropChanged(object? sender, AvaloniaPropertyChangedEventArgs e)
    {
        if (e.Property == ValueProperty)
            UpdateNeedle();
    }

    private void QueueBuildFace()
    {
        Dispatcher.UIThread.Post(() =>
        {
            BuildFace();
            UpdateNeedle();
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

        // S0–S9 green, +10…+30 yellow, +40…+60 red (WPF zones on 0–15 scale)
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

    private void UpdateNeedle()
    {
        if (!_faceBuilt || _needle == null)
        {
            BuildFace();
            if (_needle == null) return;
        }

        double tipR = _radius - 11;
        double main = Math.Clamp(Value, 0, MaxSUnit);
        double mainRad = DegToRad(UnitToAngle(main));

        _needle.StartPoint = new Point(_cx, _cy);
        _needle.EndPoint = new Point(
            _cx + tipR * Math.Sin(mainRad),
            _cy - tipR * Math.Cos(mainRad));

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
