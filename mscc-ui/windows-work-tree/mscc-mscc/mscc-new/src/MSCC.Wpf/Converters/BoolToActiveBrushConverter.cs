using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media;

namespace MSCC.Wpf.Converters;

public class BoolToActiveBrushConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        var win = Application.Current?.MainWindow;
        bool isActive = value is bool b && b;
        string key = isActive ? "UiAccentBrush" : "UiAccentMutedBrush";
        if (win?.TryFindResource(key) is Brush brush)
            return brush;
        return isActive
            ? new SolidColorBrush(Color.FromRgb(0x00, 0xFF, 0xAA))
            : new SolidColorBrush(Color.FromRgb(0x00, 0x6B, 0x5A));
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}
