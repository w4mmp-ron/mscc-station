using System;
using System.Globalization;
using System.Windows.Data;
using System.Windows.Media;

namespace MSCC.Wpf.Converters;

/// <summary>
/// Power-cal status lamp: true = green (calibrated), false = light gray (not calibrated).
/// </summary>
public sealed class BoolToCalStatusBrushConverter : IValueConverter
{
    private static readonly SolidColorBrush Calibrated =
        CreateFrozen(0x00, 0xFF, 0x66);
    private static readonly SolidColorBrush NotCalibrated =
        CreateFrozen(0xDD, 0xDD, 0xDD);

    private static SolidColorBrush CreateFrozen(byte r, byte g, byte b)
    {
        var brush = new SolidColorBrush(Color.FromRgb(r, g, b));
        brush.Freeze();
        return brush;
    }

    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        => value is true ? Calibrated : NotCalibrated;

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => throw new NotSupportedException();
}
