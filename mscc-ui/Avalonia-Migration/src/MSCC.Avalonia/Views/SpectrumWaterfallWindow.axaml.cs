using Avalonia.Controls;
using Avalonia.Interactivity;
using MSCC.Avalonia.ViewModels;

namespace MSCC.Avalonia.Views;

public partial class SpectrumWaterfallWindow : Window
{
    public SpectrumWaterfallWindow()
    {
        InitializeComponent();
        DataContext = new SpectrumWaterfallViewModel();
    }

    private void Close_Click(object? sender, RoutedEventArgs e) => Close();
}
