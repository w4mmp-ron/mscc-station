using Avalonia.Controls;
using Avalonia.Interactivity;
using MSCC.Avalonia.ViewModels;

namespace MSCC.Avalonia.Views;

public partial class SpectrumWaterfallWindow : Window
{
    public SpectrumWaterfallWindow() : this(null)
    {
    }

    public SpectrumWaterfallWindow(MainViewModel? main)
    {
        InitializeComponent();
        DataContext = new SpectrumWaterfallViewModel(main);
    }

    private void Close_Click(object? sender, RoutedEventArgs e) => Close();
}
