using System;
using System.Globalization;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;

namespace MSCC.Wpf;

/// <summary>
/// Simple WPF color chart (RGB sliders + hex) for UI Appearance CUSTOM colors.
/// Avoids System.Windows.Forms ColorDialog (and WPF/WinForms type clashes).
/// </summary>
public partial class ColorPickerWindow : Window
{
    private bool _suppress;

    public Color SelectedColor { get; private set; }

    public ColorPickerWindow(Color initial)
    {
        InitializeComponent();
        SelectedColor = initial;
        _suppress = true;
        SliderR.Value = initial.R;
        SliderG.Value = initial.G;
        SliderB.Value = initial.B;
        _suppress = false;
        RefreshUiFromSliders();
    }

    /// <summary>Show as modal dialog; true if OK.</summary>
    public static bool TryPick(Window? owner, Color initial, out Color result)
    {
        var dlg = new ColorPickerWindow(initial);
        // Owner must already be shown; otherwise WPF throws InvalidOperationException.
        Window? safeOwner = owner;
        if (safeOwner != null && !safeOwner.IsLoaded)
            safeOwner = Application.Current?.MainWindow;
        if (safeOwner != null && safeOwner.IsLoaded && !ReferenceEquals(safeOwner, dlg))
            dlg.Owner = safeOwner;
        bool? ok = dlg.ShowDialog();
        if (ok == true)
        {
            result = dlg.SelectedColor;
            return true;
        }
        result = initial;
        return false;
    }

    private void OnSliderChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
    {
        if (_suppress) return;
        RefreshUiFromSliders();
    }

    private void RefreshUiFromSliders()
    {
        byte r = (byte)Math.Clamp((int)SliderR.Value, 0, 255);
        byte g = (byte)Math.Clamp((int)SliderG.Value, 0, 255);
        byte b = (byte)Math.Clamp((int)SliderB.Value, 0, 255);
        SelectedColor = Color.FromRgb(r, g, b);
        TextR.Text = r.ToString(CultureInfo.InvariantCulture);
        TextG.Text = g.ToString(CultureInfo.InvariantCulture);
        TextB.Text = b.ToString(CultureInfo.InvariantCulture);
        HexBox.Text = $"#{r:X2}{g:X2}{b:X2}";
        PreviewSwatch.Background = new SolidColorBrush(SelectedColor);
    }

    private void OnHexLostFocus(object sender, RoutedEventArgs e) => ApplyHexFromBox();

    private void OnHexKeyDown(object sender, KeyEventArgs e)
    {
        if (e.Key == Key.Enter)
        {
            ApplyHexFromBox();
            e.Handled = true;
        }
    }

    private void ApplyHexFromBox()
    {
        var parsed = UiChromeTheme.TryParseHex(HexBox.Text);
        if (parsed is not Color c) return;
        _suppress = true;
        SliderR.Value = c.R;
        SliderG.Value = c.G;
        SliderB.Value = c.B;
        _suppress = false;
        RefreshUiFromSliders();
    }

    private void Ok_Click(object sender, RoutedEventArgs e)
    {
        DialogResult = true;
        Close();
    }

    private void Cancel_Click(object sender, RoutedEventArgs e)
    {
        DialogResult = false;
        Close();
    }
}
