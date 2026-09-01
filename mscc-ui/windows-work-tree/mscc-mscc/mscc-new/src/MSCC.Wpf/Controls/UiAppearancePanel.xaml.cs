using System;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using WpfColor = System.Windows.Media.Color;

namespace MSCC.Wpf.Controls;

/// <summary>
/// UI chrome appearance (background / panel / button). Used on Settings tab
/// (moved from S/W Spectrum/Waterfall window).
/// </summary>
public partial class UiAppearancePanel : UserControl
{
    public UiAppearancePanel()
    {
        InitializeComponent();
        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        // Restore lists without re-saving on load
        if (UiBackgroundList != null)
        {
            UiBackgroundList.SelectionChanged -= OnUiBackgroundSelectionChanged;
            SelectItemByContent(UiBackgroundList, SpectrumWaterfallSettings.UiBackground);
            UiBackgroundList.SelectionChanged += OnUiBackgroundSelectionChanged;
        }
        if (UiPanelList != null)
        {
            UiPanelList.SelectionChanged -= OnUiPanelSelectionChanged;
            SelectItemByContent(UiPanelList, SpectrumWaterfallSettings.UiPanel);
            UiPanelList.SelectionChanged += OnUiPanelSelectionChanged;
        }
        if (UiButtonList != null)
        {
            UiButtonList.SelectionChanged -= OnUiButtonSelectionChanged;
            SelectItemByContent(UiButtonList, SpectrumWaterfallSettings.UiButton);
            UiButtonList.SelectionChanged += OnUiButtonSelectionChanged;
        }

        UpdateUiChromeRgbLabels();
        UpdatePanelListEnabledState();
    }

    private void SelectItemByContent(ListBox? list, string content)
    {
        if (list == null) return;
        foreach (ListBoxItem item in list.Items.OfType<ListBoxItem>())
        {
            if (string.Equals(item.Content?.ToString(), content, StringComparison.OrdinalIgnoreCase))
            {
                list.SelectedItem = item;
                break;
            }
        }
    }

    private void OnUiBackgroundSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (sender is not ListBox lb || lb.SelectedItem is not ListBoxItem item)
            return;
        string name = item.Content?.ToString() ?? "BLACK";
        ApplyUiBackgroundChoice(name, reopenCustom: false);
    }

    private void OnUiBackgroundListPreviewMouseDown(object sender, MouseButtonEventArgs e)
    {
        if (!IsClickOnListItemContent(e, "CUSTOM")) return;
        if (!UiChromeTheme.IsCustom(SpectrumWaterfallSettings.UiBackground)) return;
        ApplyUiBackgroundChoice("CUSTOM", reopenCustom: true);
        e.Handled = true;
    }

    private void ApplyUiBackgroundChoice(string name, bool reopenCustom)
    {
        string previous = SpectrumWaterfallSettings.UiBackground;

        if (UiChromeTheme.IsCustom(name))
        {
            WpfColor start = UiChromeTheme.ResolveBackground();
            if (!TryPickColor(start, out WpfColor picked))
            {
                if (UiBackgroundList != null && !reopenCustom)
                {
                    UiBackgroundList.SelectionChanged -= OnUiBackgroundSelectionChanged;
                    SelectItemByContent(UiBackgroundList, previous);
                    UiBackgroundList.SelectionChanged += OnUiBackgroundSelectionChanged;
                }
                return;
            }

            SpectrumWaterfallSettings.UiBackground = "CUSTOM";
            SpectrumWaterfallSettings.UiBackgroundRgb = UiChromeTheme.ToHex(picked);
            SpectrumWaterfallSettings.UiPanel = "AUTO";
            if (UiPanelList != null)
            {
                UiPanelList.SelectionChanged -= OnUiPanelSelectionChanged;
                SelectItemByContent(UiPanelList, "AUTO");
                UiPanelList.SelectionChanged += OnUiPanelSelectionChanged;
            }
        }
        else
        {
            SpectrumWaterfallSettings.UiBackground = name;
        }

        SpectrumWaterfallSettings.Save();
        UiChromeTheme.ApplyToMainWindow();
        UpdatePanelListEnabledState();
        UpdateUiChromeRgbLabels();
    }

    private void OnUiButtonSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (sender is not ListBox lb || lb.SelectedItem is not ListBoxItem item)
            return;
        string name = item.Content?.ToString() ?? "YELLOW";
        ApplyUiButtonChoice(name, reopenCustom: false);
    }

    private void OnUiButtonListPreviewMouseDown(object sender, MouseButtonEventArgs e)
    {
        if (!IsClickOnListItemContent(e, "CUSTOM")) return;
        if (!UiChromeTheme.IsCustom(SpectrumWaterfallSettings.UiButton)) return;
        ApplyUiButtonChoice("CUSTOM", reopenCustom: true);
        e.Handled = true;
    }

    private void ApplyUiButtonChoice(string name, bool reopenCustom)
    {
        string previous = SpectrumWaterfallSettings.UiButton;

        if (UiChromeTheme.IsCustom(name))
        {
            WpfColor start = UiChromeTheme.ResolveButtonFace();
            if (!TryPickColor(start, out WpfColor picked))
            {
                if (UiButtonList != null && !reopenCustom)
                {
                    UiButtonList.SelectionChanged -= OnUiButtonSelectionChanged;
                    SelectItemByContent(UiButtonList, previous);
                    UiButtonList.SelectionChanged += OnUiButtonSelectionChanged;
                }
                return;
            }

            SpectrumWaterfallSettings.UiButton = "CUSTOM";
            SpectrumWaterfallSettings.UiButtonRgb = UiChromeTheme.ToHex(picked);
        }
        else
        {
            SpectrumWaterfallSettings.UiButton = name;
        }

        SpectrumWaterfallSettings.Save();
        UiChromeTheme.ApplyToMainWindow();
        UpdateUiChromeRgbLabels();
    }

    private void OnUiPanelSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (UiChromeTheme.IsCustom(SpectrumWaterfallSettings.UiBackground))
            return;
        if (sender is not ListBox lb || lb.SelectedItem is not ListBoxItem item)
            return;
        string name = item.Content?.ToString() ?? "AUTO";
        SpectrumWaterfallSettings.UiPanel = name;
        SpectrumWaterfallSettings.Save();
        UiChromeTheme.ApplyToMainWindow();
        UpdateUiChromeRgbLabels();
    }

    private void OnUiChromeResetClick(object sender, RoutedEventArgs e)
    {
        SpectrumWaterfallSettings.UiBackground = "BLACK";
        SpectrumWaterfallSettings.UiBackgroundRgb = "#1C1C1C";
        SpectrumWaterfallSettings.UiPanel = "AUTO";
        SpectrumWaterfallSettings.UiButton = "YELLOW";
        SpectrumWaterfallSettings.UiButtonRgb = "#FFCC00";
        SpectrumWaterfallSettings.Save();

        if (UiBackgroundList != null)
        {
            UiBackgroundList.SelectionChanged -= OnUiBackgroundSelectionChanged;
            SelectItemByContent(UiBackgroundList, "BLACK");
            UiBackgroundList.SelectionChanged += OnUiBackgroundSelectionChanged;
        }
        if (UiPanelList != null)
        {
            UiPanelList.SelectionChanged -= OnUiPanelSelectionChanged;
            SelectItemByContent(UiPanelList, "AUTO");
            UiPanelList.SelectionChanged += OnUiPanelSelectionChanged;
        }
        if (UiButtonList != null)
        {
            UiButtonList.SelectionChanged -= OnUiButtonSelectionChanged;
            SelectItemByContent(UiButtonList, "YELLOW");
            UiButtonList.SelectionChanged += OnUiButtonSelectionChanged;
        }

        UiChromeTheme.ApplyToMainWindow();
        UpdatePanelListEnabledState();
        UpdateUiChromeRgbLabels();
    }

    private void UpdatePanelListEnabledState()
    {
        bool customBg = UiChromeTheme.IsCustom(SpectrumWaterfallSettings.UiBackground);
        if (UiPanelList != null)
            UiPanelList.IsEnabled = !customBg;
    }

    private void UpdateUiChromeRgbLabels()
    {
        WpfColor bg = UiChromeTheme.ResolveBackground();
        WpfColor panel = UiChromeTheme.ResolvePanel(bg);
        WpfColor btn = UiChromeTheme.ResolveButtonFace();

        string bgHex = UiChromeTheme.ToHex(bg);
        string panelHex = UiChromeTheme.ToHex(panel);
        string btnHex = UiChromeTheme.ToHex(btn);

        if (UiBackgroundRgbText != null) UiBackgroundRgbText.Text = bgHex;
        if (UiPanelRgbText != null) UiPanelRgbText.Text = panelHex;
        if (UiButtonRgbText != null) UiButtonRgbText.Text = btnHex;

        if (UiBackgroundSwatch != null) UiBackgroundSwatch.Background = new SolidColorBrush(bg);
        if (UiPanelSwatch != null) UiPanelSwatch.Background = new SolidColorBrush(panel);
        if (UiButtonSwatch != null) UiButtonSwatch.Background = new SolidColorBrush(btn);
    }

    private bool TryPickColor(WpfColor start, out WpfColor result)
    {
        Window? owner = Window.GetWindow(this) ?? Application.Current?.MainWindow;
        return ColorPickerWindow.TryPick(owner, start, out result);
    }

    private static bool IsClickOnListItemContent(MouseButtonEventArgs e, string content)
    {
        DependencyObject? dep = e.OriginalSource as DependencyObject;
        while (dep != null)
        {
            if (dep is ListBoxItem lbi)
                return string.Equals(lbi.Content?.ToString(), content, StringComparison.OrdinalIgnoreCase);
            if (dep is Visual)
                dep = VisualTreeHelper.GetParent(dep);
            else
                break;
        }
        return false;
    }
}
