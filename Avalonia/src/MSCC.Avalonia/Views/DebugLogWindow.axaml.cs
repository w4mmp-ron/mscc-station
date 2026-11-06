using Avalonia.Controls;
using Avalonia.Interactivity;

namespace MSCC.Avalonia.Views;

public partial class DebugLogWindow : Window
{
    public DebugLogWindow()
    {
        InitializeComponent();
    }

    private void Close_Click(object? sender, RoutedEventArgs e) => Close();
}
