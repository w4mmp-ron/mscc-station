using System;
using System.Globalization;
using System.Windows.Data;
using System.Windows.Media;

namespace MSCC.Wpf.Converters;

public class BoolToActiveBrushConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is bool isActive && isActive)
        {
            return new SolidColorBrush(Color.FromRgb(0x00, 0xFF, 0x88)); // Nice green
        }
        return new SolidColorBrush(Colors.Transparent);
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}
