using Avalonia.Controls;
using Avalonia.Interactivity;

namespace MSCC.Avalonia.Views;

public partial class DebugLogWindow : Window
{
    public DebugLogWindow()
    {
        InitializeComponent();
        Opened += (_, _) => ScreenFit.PlaceDebugLog(this);
    }

    private void Close_Click(object? sender, RoutedEventArgs e) => Close();
}
