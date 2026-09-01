using System;
using System.Globalization;
using System.Windows.Data;

namespace MSCC.Wpf.Converters;

/// <summary>Inverts a boolean (e.g. disable control when TxIqTxOn is true).</summary>
public sealed class InverseBoolConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        => value is bool b ? !b : true;

    public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        => value is bool b ? !b : false;
}
