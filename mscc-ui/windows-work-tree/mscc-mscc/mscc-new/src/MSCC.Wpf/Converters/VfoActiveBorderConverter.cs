using System;
using System.Globalization;
using System.Windows.Data;
using System.Windows.Media;

namespace MSCC.Wpf.Converters;

/// <summary>
/// Placeholder. Not currently used.
/// </summary>
public class VfoActiveBorderConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        return new SolidColorBrush(Colors.Transparent);
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
    {
        throw new NotImplementedException();
    }
}
