using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;

namespace MSCC.Wpf.Converters;

/// <summary>Visible when string is non-null and non-empty; Collapsed otherwise.</summary>
public sealed class StringNullOrEmptyToVisibilityConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
    {
        string? s = value as string;
        return string.IsNullOrWhiteSpace(s) ? Visibility.Collapsed : Visibility.Visible;
    }

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => throw new NotSupportedException();
}
