using System;
using System.Globalization;
using System.Windows.Data;
using System.Windows.Media;

namespace MSCC.Wpf.Converters;

public class BoolToBrushConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        if (value is bool isConnected)
        {
            return isConnected 
                ? new SolidColorBrush(Color.FromRgb(0x00, 0xC8, 0x53))   // Green
                : new SolidColorBrush(Color.FromRgb(0xE8, 0x11, 0x23)); // Red
        }
        return new SolidColorBrush(Colors.Gray);
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}
