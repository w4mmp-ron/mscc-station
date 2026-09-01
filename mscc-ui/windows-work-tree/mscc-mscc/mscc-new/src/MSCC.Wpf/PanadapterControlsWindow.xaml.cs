using System;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using MSCC.Wpf.Controls;

namespace MSCC.Wpf;

public partial class PanadapterControlsWindow : Window
{
    // Session-only placement (not written to INI). Same pattern as DebugLogWindow.
    private static double? s_left;
    private static double? s_top;
    private static double? s_width;
    private static double? s_height;

    public PanadapterControlsWindow()
    {
        InitializeComponent();
        SourceInitialized += OnSourceInitialized;
        Closed += OnWindowClosed;

        // For now, simple data context with some demo values.
        // In future, bind to MainViewModel or a dedicated SpectrumWaterfallSettings.
        DataContext = new PanadapterControlsViewModel();

        // Apply the currently persisted values (loaded at app startup or previous S/W changes)
        // so the renderer and the lists reflect what was saved in the INI. This makes FILL (and
        // the other Spectrum settings) sticky across restarts.
        // dB CAL / GRID: do NOT re-seed from INI here — live SpectrumColorSettings is source of
        // truth this session (user may have just calibrated). Sliders mirror live below.
        SpectrumColorSettings.SetFill(SpectrumWaterfallSettings.SpectrumFill);
        SpectrumColorSettings.SetLine(SpectrumWaterfallSettings.SpectrumLine);
        SpectrumWaterfallSettings.ApplySpectrumBackgroundToRenderer();
        SpectrumColorSettings.SetCursor(SpectrumWaterfallSettings.SpectrumCursor);
        SpectrumColorSettings.SetBaseline(SpectrumWaterfallSettings.SpectrumBaseline);
        SpectrumColorSettings.SetWaterfallPalette(SpectrumWaterfallSettings.WaterfallPalette);
        SpectrumColorSettings.SetWaterfallGain(SpectrumWaterfallSettings.WaterfallGain);
        SpectrumColorSettings.SetWaterfallZero(SpectrumWaterfallSettings.WaterfallZero);
        SpectrumColorSettings.SetWaterfallTimeMarker(SpectrumWaterfallSettings.WaterfallTimeMarker);
        SpectrumColorSettings.SetWaterfallDirectionNormal(SpectrumWaterfallSettings.WaterfallDirectionNormal);
        SpectrumColorSettings.SetViewGrid(SpectrumWaterfallSettings.SpectrumViewGrid);
        SpectrumColorSettings.SetViewDbLabels(SpectrumWaterfallSettings.SpectrumViewDbLabels);
        SpectrumColorSettings.SetPeakMarker(SpectrumWaterfallSettings.SpectrumPeakMarker);

        // Select the correct items in the lists based on the persisted settings values.
        // Detach handlers first: selecting BACKGROUND=CUSTOM must NOT open the color picker
        // during ctor (window not shown yet → Owner crash).
        SuppressListSelectionHandlers(true);
        try
        {
            SelectItemByContent(FillList, SpectrumWaterfallSettings.SpectrumFill);
            SelectItemByContent(BackgroundList, SpectrumWaterfallSettings.SpectrumBackground);
            SelectItemByContent(LineList, SpectrumWaterfallSettings.SpectrumLine);
            SelectItemByContent(CursorList, SpectrumWaterfallSettings.SpectrumCursor);
            SelectItemByContent(RefreshList, SpectrumWaterfallSettings.SpectrumRefresh.ToString());
            SelectItemByContent(WaterfallPaletteList, SpectrumColorSettings.WaterfallPaletteName);
            SelectPanResolutionList();
        }
        finally
        {
            SuppressListSelectionHandlers(false);
        }

        // AUTO SNAP (checkbox + 1KHz / 500Hz / 100Hz) — restore without firing save on open
        if (AutoSnapCheckBox != null)
        {
            AutoSnapCheckBox.Checked -= OnAutoSnapCheckedChanged;
            AutoSnapCheckBox.Unchecked -= OnAutoSnapCheckedChanged;
            AutoSnapCheckBox.IsChecked = SpectrumWaterfallSettings.SpectrumAutoSnap;
            AutoSnapCheckBox.Checked += OnAutoSnapCheckedChanged;
            AutoSnapCheckBox.Unchecked += OnAutoSnapCheckedChanged;
        }
        if (AutoSnapFreqList != null)
        {
            AutoSnapFreqList.SelectionChanged -= OnAutoSnapFreqSelectionChanged;
            NormalizeAndSelectAutoSnapFreq();
            AutoSnapFreqList.SelectionChanged += OnAutoSnapFreqSelectionChanged;
        }

        // Restore sliders from sticky settings, THEN attach ValueChanged.
        // Handlers must NOT be in XAML: WPF clamps default Value to Maximum during
        // InitializeComponent and was writing SPECTRUM_DB_OFFSET=-20 / GRID_MIN=-100 to INI
        // on every S/W open (wiping cal and dropping the −120 tick).
        if (BaselineSlider != null)
            BaselineSlider.Value = SpectrumWaterfallSettings.SpectrumBaseline;

        if (DbCalSlider != null)
        {
            // Slider is relative trim around SpectrumDbCalCenter (−91.3 = UI 0).
            DbCalSlider.Value = SpectrumColorSettings.GetSpectrumDbCalRelative();
            UpdateDbCalValueText();
        }

        if (GridMaxSlider != null)
            GridMaxSlider.Value = SpectrumColorSettings.SpectrumGridMax;
        if (GridMinSlider != null)
            GridMinSlider.Value = SpectrumColorSettings.SpectrumGridMin;
        UpdateGridRangeValueText();

        if (WaterfallHighSlider != null)
            WaterfallHighSlider.Value = SpectrumColorSettings.WaterfallHighDb;
        if (WaterfallLowSlider != null)
            WaterfallLowSlider.Value = SpectrumColorSettings.WaterfallLowDb;
        UpdateWaterfallRangeValueText();

        if (WaterfallGainSlider != null)
        {
            WaterfallGainSlider.Value = SpectrumColorSettings.WaterfallGain;
            if (WaterfallGainValueText != null)
                WaterfallGainValueText.Text = SpectrumColorSettings.WaterfallGain.ToString();
        }
        if (WaterfallZeroSlider != null)
        {
            WaterfallZeroSlider.Value = SpectrumColorSettings.WaterfallZero;
            if (WaterfallZeroValueText != null)
                WaterfallZeroValueText.Text = SpectrumColorSettings.WaterfallZero.ToString();
        }

        AttachSliderHandlers();

        if (WaterfallTimeMarkerButton != null)
            WaterfallTimeMarkerButton.Content = SpectrumColorSettings.WaterfallTimeMarker.ToString();

        if (WaterfallDirectionCheckBox != null)
        {
            WaterfallDirectionCheckBox.Checked -= OnWaterfallDirectionChanged;
            WaterfallDirectionCheckBox.Unchecked -= OnWaterfallDirectionChanged;
            WaterfallDirectionCheckBox.IsChecked = SpectrumColorSettings.WaterfallDirectionNormal;
            UpdateWaterfallDirectionCheckboxText();
            WaterfallDirectionCheckBox.Checked += OnWaterfallDirectionChanged;
            WaterfallDirectionCheckBox.Unchecked += OnWaterfallDirectionChanged;
        }

        if (ViewGridCheckBox != null)
        {
            ViewGridCheckBox.Checked -= OnViewGridCheckedChanged;
            ViewGridCheckBox.Unchecked -= OnViewGridCheckedChanged;
            ViewGridCheckBox.IsChecked = SpectrumWaterfallSettings.SpectrumViewGrid;
            ViewGridCheckBox.Checked += OnViewGridCheckedChanged;
            ViewGridCheckBox.Unchecked += OnViewGridCheckedChanged;
        }

        if (ViewDbLabelsCheckBox != null)
        {
            ViewDbLabelsCheckBox.Checked -= OnViewDbLabelsCheckedChanged;
            ViewDbLabelsCheckBox.Unchecked -= OnViewDbLabelsCheckedChanged;
            ViewDbLabelsCheckBox.IsChecked = SpectrumWaterfallSettings.SpectrumViewDbLabels;
            ViewDbLabelsCheckBox.Checked += OnViewDbLabelsCheckedChanged;
            ViewDbLabelsCheckBox.Unchecked += OnViewDbLabelsCheckedChanged;
        }

        if (PeakMarkerCheckBox != null)
        {
            PeakMarkerCheckBox.Checked -= OnPeakMarkerCheckedChanged;
            PeakMarkerCheckBox.Unchecked -= OnPeakMarkerCheckedChanged;
            PeakMarkerCheckBox.IsChecked = SpectrumWaterfallSettings.SpectrumPeakMarker;
            PeakMarkerCheckBox.Checked += OnPeakMarkerCheckedChanged;
            PeakMarkerCheckBox.Unchecked += OnPeakMarkerCheckedChanged;
        }

        if (TimeDisplayCheckBox != null)
        {
            TimeDisplayCheckBox.Checked -= OnTimeDisplayCheckedChanged;
            TimeDisplayCheckBox.Unchecked -= OnTimeDisplayCheckedChanged;
            // Prefer live VM value if main window is open; else INI
            bool timeOn = SpectrumWaterfallSettings.TimeDisplayOn;
            if (Application.Current?.MainWindow?.DataContext is ViewModels.MainViewModel vm)
                timeOn = vm.TimeDisplayOn;
            TimeDisplayCheckBox.IsChecked = timeOn;
            TimeDisplayCheckBox.Checked += OnTimeDisplayCheckedChanged;
            TimeDisplayCheckBox.Unchecked += OnTimeDisplayCheckedChanged;
        }

        // UI Appearance lives on Settings tab (UiAppearancePanel).
    }

    /// <summary>
    /// Wire slider events only after Values are restored. Never attach in XAML.
    /// </summary>
    private void AttachSliderHandlers()
    {
        if (BaselineSlider != null)
        {
            BaselineSlider.ValueChanged -= OnBaselineChanged;
            BaselineSlider.ValueChanged += OnBaselineChanged;
        }
        if (DbCalSlider != null)
        {
            DbCalSlider.ValueChanged -= OnDbCalChanged;
            DbCalSlider.ValueChanged += OnDbCalChanged;
        }
        if (GridMaxSlider != null)
        {
            GridMaxSlider.ValueChanged -= OnGridMaxChanged;
            GridMaxSlider.ValueChanged += OnGridMaxChanged;
        }
        if (GridMinSlider != null)
        {
            GridMinSlider.ValueChanged -= OnGridMinChanged;
            GridMinSlider.ValueChanged += OnGridMinChanged;
        }
        if (WaterfallHighSlider != null)
        {
            WaterfallHighSlider.ValueChanged -= OnWaterfallHighChanged;
            WaterfallHighSlider.ValueChanged += OnWaterfallHighChanged;
        }
        if (WaterfallLowSlider != null)
        {
            WaterfallLowSlider.ValueChanged -= OnWaterfallLowChanged;
            WaterfallLowSlider.ValueChanged += OnWaterfallLowChanged;
        }
        if (WaterfallGainSlider != null)
        {
            WaterfallGainSlider.ValueChanged -= OnWaterfallGainChanged;
            WaterfallGainSlider.ValueChanged += OnWaterfallGainChanged;
        }
        if (WaterfallZeroSlider != null)
        {
            WaterfallZeroSlider.ValueChanged -= OnWaterfallZeroChanged;
            WaterfallZeroSlider.ValueChanged += OnWaterfallZeroChanged;
        }
    }

    private void OnSourceInitialized(object? sender, EventArgs e)
    {
        if (s_left is double left && s_top is double top)
        {
            WindowStartupLocation = WindowStartupLocation.Manual;
            Left = left;
            Top = top;
            if (s_width is double w && w >= MinWidth)
                Width = w;
            if (s_height is double h && h >= MinHeight)
                Height = h;
        }
    }

    private void OnWindowClosed(object? sender, EventArgs e)
    {
        if (WindowState == WindowState.Normal)
        {
            s_left = Left;
            s_top = Top;
            s_width = Width;
            s_height = Height;
        }
    }

    private void UpdateWaterfallDirectionCheckboxText()
    {
        if (WaterfallDirectionCheckBox == null) return;
        WaterfallDirectionCheckBox.Content = SpectrumColorSettings.WaterfallDirectionNormal
            ? "Direction Normal"
            : "Direction Reversed";
    }

    private void SuppressListSelectionHandlers(bool suppress)
    {
        if (FillList != null)
        {
            if (suppress) FillList.SelectionChanged -= OnFillSelectionChanged;
            else FillList.SelectionChanged += OnFillSelectionChanged;
        }
        if (BackgroundList != null)
        {
            if (suppress) BackgroundList.SelectionChanged -= OnBackgroundSelectionChanged;
            else BackgroundList.SelectionChanged += OnBackgroundSelectionChanged;
        }
        if (LineList != null)
        {
            if (suppress) LineList.SelectionChanged -= OnLineSelectionChanged;
            else LineList.SelectionChanged += OnLineSelectionChanged;
        }
        if (CursorList != null)
        {
            if (suppress) CursorList.SelectionChanged -= OnCursorSelectionChanged;
            else CursorList.SelectionChanged += OnCursorSelectionChanged;
        }
        if (RefreshList != null)
        {
            if (suppress) RefreshList.SelectionChanged -= OnRefreshSelectionChanged;
            else RefreshList.SelectionChanged += OnRefreshSelectionChanged;
        }
        if (WaterfallPaletteList != null)
        {
            if (suppress) WaterfallPaletteList.SelectionChanged -= OnWaterfallPaletteSelectionChanged;
            else WaterfallPaletteList.SelectionChanged += OnWaterfallPaletteSelectionChanged;
        }
        if (PanResolutionList != null)
        {
            if (suppress) PanResolutionList.SelectionChanged -= OnPanResolutionSelectionChanged;
            else PanResolutionList.SelectionChanged += OnPanResolutionSelectionChanged;
        }
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

    private void CloseButton_Click(object sender, RoutedEventArgs e)
    {
        this.Close();
    }

    /// <summary>
    /// WPF Slider does not scroll with the mouse wheel unless focused; handle wheel while hovered.
    /// One notch = SmallChange (default 1). Ctrl = LargeChange (default 5).
    /// </summary>
    private void OnSliderPreviewMouseWheel(object sender, MouseWheelEventArgs e)
    {
        if (sender is not Slider slider || !slider.IsEnabled)
            return;

        double step = Keyboard.Modifiers.HasFlag(ModifierKeys.Control)
            ? (slider.LargeChange > 0 ? slider.LargeChange : 5)
            : (slider.SmallChange > 0 ? slider.SmallChange : 1);

        double delta = e.Delta > 0 ? step : -step;
        slider.Value = Math.Clamp(slider.Value + delta, slider.Minimum, slider.Maximum);
        e.Handled = true;
    }

    private void OnFillSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (sender is ListBox lb && lb.SelectedItem is ListBoxItem item)
        {
            string fillName = item.Content?.ToString() ?? "SCOPE";
            SpectrumColorSettings.SetFill(fillName);
            SpectrumWaterfallSettings.SpectrumFill = fillName;
            SpectrumWaterfallSettings.Save();
            // The next spectrum update in SpectrumDisplayControl will pick up the new colors from SpectrumColorSettings
        }
    }

    private void OnLineSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (sender is ListBox lb && lb.SelectedItem is ListBoxItem item)
        {
            string lineName = item.Content?.ToString() ?? "WHITE";
            SpectrumColorSettings.SetLine(lineName);
            SpectrumWaterfallSettings.SpectrumLine = lineName;
            SpectrumWaterfallSettings.Save();
            // The bright top line (trace) color will update on next spectrum render
        }
    }

    private void OnBackgroundSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (sender is not ListBox lb || lb.SelectedItem is not ListBoxItem item)
            return;
        string bgName = item.Content?.ToString() ?? "BLACK";
        ApplySpectrumBackgroundChoice(bgName, reopenCustom: false);
    }

    /// <summary>Re-open color picker when CUSTOM is already selected (same pattern as UI Appearance).</summary>
    private void OnBackgroundListPreviewMouseDown(object sender, MouseButtonEventArgs e)
    {
        if (!IsClickOnListItemContent(e, "CUSTOM")) return;
        if (!string.Equals(SpectrumWaterfallSettings.SpectrumBackground, "CUSTOM", StringComparison.OrdinalIgnoreCase))
            return;
        ApplySpectrumBackgroundChoice("CUSTOM", reopenCustom: true);
        e.Handled = true;
    }

    private void ApplySpectrumBackgroundChoice(string bgName, bool reopenCustom)
    {
        string previous = SpectrumWaterfallSettings.SpectrumBackground;

        if (string.Equals(bgName, "CUSTOM", StringComparison.OrdinalIgnoreCase))
        {
            var start = UiChromeTheme.TryParseHex(SpectrumWaterfallSettings.SpectrumBackgroundRgb)
                        ?? System.Windows.Media.Color.FromRgb(
                            SpectrumColorSettings.BackgroundR,
                            SpectrumColorSettings.BackgroundG,
                            SpectrumColorSettings.BackgroundB);
            if (!ColorPickerWindow.TryPick(this, start, out var picked))
            {
                if (BackgroundList != null && !reopenCustom)
                {
                    BackgroundList.SelectionChanged -= OnBackgroundSelectionChanged;
                    SelectItemByContent(BackgroundList, previous);
                    BackgroundList.SelectionChanged += OnBackgroundSelectionChanged;
                }
                return;
            }

            SpectrumWaterfallSettings.SpectrumBackground = "CUSTOM";
            SpectrumWaterfallSettings.SpectrumBackgroundRgb = UiChromeTheme.ToHex(picked);
            SpectrumColorSettings.SetBackgroundRgb(picked.R, picked.G, picked.B);
        }
        else
        {
            SpectrumWaterfallSettings.SpectrumBackground = bgName;
            SpectrumColorSettings.SetBackground(bgName);
        }

        SpectrumWaterfallSettings.Save();
    }

    private static bool IsClickOnListItemContent(MouseButtonEventArgs e, string content)
    {
        DependencyObject? dep = e.OriginalSource as DependencyObject;
        while (dep != null)
        {
            if (dep is ListBoxItem lbi)
                return string.Equals(lbi.Content?.ToString(), content, StringComparison.OrdinalIgnoreCase);
            if (dep is System.Windows.Media.Visual)
                dep = System.Windows.Media.VisualTreeHelper.GetParent(dep);
            else
                break;
        }
        return false;
    }

    private void OnCursorSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (sender is ListBox lb && lb.SelectedItem is ListBoxItem item)
        {
            string cursorName = item.Content?.ToString() ?? "WHITE";
            SpectrumColorSettings.SetCursor(cursorName);
            SpectrumWaterfallSettings.SpectrumCursor = cursorName;
            SpectrumWaterfallSettings.Save();
            // The center tuning cursor line color will update on next spectrum render
        }
    }

    private void BaselineLeft_Click(object sender, RoutedEventArgs e)
    {
        int val = SpectrumColorSettings.SpectrumBaseline - 5;
        SpectrumColorSettings.SetBaseline(val);
        SpectrumWaterfallSettings.SpectrumBaseline = val;
        SpectrumWaterfallSettings.Save();
        if (BaselineSlider != null) BaselineSlider.Value = val;
    }

    private void BaselineRight_Click(object sender, RoutedEventArgs e)
    {
        int val = SpectrumColorSettings.SpectrumBaseline + 5;
        SpectrumColorSettings.SetBaseline(val);
        SpectrumWaterfallSettings.SpectrumBaseline = val;
        SpectrumWaterfallSettings.Save();
        if (BaselineSlider != null) BaselineSlider.Value = val;
    }

    private void OnBaselineChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
    {
        int val = (int)e.NewValue;
        SpectrumColorSettings.SetBaseline(val);
        SpectrumWaterfallSettings.SpectrumBaseline = val;
        SpectrumWaterfallSettings.Save();
    }

    private void DbCalLeft_Click(object sender, RoutedEventArgs e) =>
        ApplyDbCalRelative(SpectrumColorSettings.GetSpectrumDbCalRelative() - 1f);

    private void DbCalRight_Click(object sender, RoutedEventArgs e) =>
        ApplyDbCalRelative(SpectrumColorSettings.GetSpectrumDbCalRelative() + 1f);

    private void DbCalReset_Click(object sender, RoutedEventArgs e) =>
        ApplyDbCalRelative(0f);

    private void OnDbCalChanged(object sender, RoutedPropertyChangedEventArgs<double> e) =>
        ApplyDbCalRelative((float)e.NewValue);

    /// <param name="relativeDb">UI trim: 0 = center (−91.3 absolute), ±20 span.</param>
    private void ApplyDbCalRelative(float relativeDb)
    {
        SpectrumColorSettings.SetSpectrumDbCalRelative(relativeDb);
        SpectrumWaterfallSettings.SpectrumDbOffset = SpectrumColorSettings.SpectrumDbOffset;
        SpectrumWaterfallSettings.Save();
        float rel = SpectrumColorSettings.GetSpectrumDbCalRelative();
        if (DbCalSlider != null && Math.Abs(DbCalSlider.Value - rel) > 0.01)
        {
            DbCalSlider.ValueChanged -= OnDbCalChanged;
            DbCalSlider.Value = rel;
            DbCalSlider.ValueChanged += OnDbCalChanged;
        }
        UpdateDbCalValueText();
    }

    private void UpdateDbCalValueText()
    {
        if (DbCalValueText == null) return;
        float rel = SpectrumColorSettings.GetSpectrumDbCalRelative();
        float abs = SpectrumColorSettings.SpectrumDbOffset;
        // Show relative (0 = center); signed for clarity
        DbCalValueText.Text = rel > 0.05f ? $"+{rel:0.0}" : $"{rel:0.0}";
        DbCalValueText.ToolTip = $"Trim {rel:0.0} dB  →  absolute {abs:0.0} (center {SpectrumColorSettings.SpectrumDbCalCenter:0.0})";
    }

    private void OnGridMaxChanged(object sender, RoutedPropertyChangedEventArgs<double> e) =>
        ApplySpectrumGrid((float)e.NewValue, SpectrumColorSettings.SpectrumGridMin);

    private void OnGridMinChanged(object sender, RoutedPropertyChangedEventArgs<double> e) =>
        ApplySpectrumGrid(SpectrumColorSettings.SpectrumGridMax, (float)e.NewValue);

    private void GridRangeReset_Click(object sender, RoutedEventArgs e) =>
        ApplySpectrumGrid(-20f, -125f);

    private void ApplySpectrumGrid(float maxDb, float minDb)
    {
        SpectrumColorSettings.SetSpectrumGrid(maxDb, minDb);
        SpectrumWaterfallSettings.SpectrumGridMax = SpectrumColorSettings.SpectrumGridMax;
        SpectrumWaterfallSettings.SpectrumGridMin = SpectrumColorSettings.SpectrumGridMin;
        SpectrumWaterfallSettings.Save();

        if (GridMaxSlider != null && Math.Abs(GridMaxSlider.Value - SpectrumColorSettings.SpectrumGridMax) > 0.1)
        {
            GridMaxSlider.ValueChanged -= OnGridMaxChanged;
            GridMaxSlider.Value = SpectrumColorSettings.SpectrumGridMax;
            GridMaxSlider.ValueChanged += OnGridMaxChanged;
        }
        if (GridMinSlider != null && Math.Abs(GridMinSlider.Value - SpectrumColorSettings.SpectrumGridMin) > 0.1)
        {
            GridMinSlider.ValueChanged -= OnGridMinChanged;
            GridMinSlider.Value = SpectrumColorSettings.SpectrumGridMin;
            GridMinSlider.ValueChanged += OnGridMinChanged;
        }
        UpdateGridRangeValueText();
    }

    private void UpdateGridRangeValueText()
    {
        if (GridMaxValueText != null)
            GridMaxValueText.Text = $"{SpectrumColorSettings.SpectrumGridMax:0}";
        if (GridMinValueText != null)
            GridMinValueText.Text = $"{SpectrumColorSettings.SpectrumGridMin:0}";
    }

    private void OnRefreshSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (sender is ListBox lb && lb.SelectedItem is ListBoxItem item)
        {
            string txt = item.Content?.ToString() ?? "4";
            if (int.TryParse(txt, out int refresh) && refresh > 0)
            {
                SpectrumWaterfallSettings.SpectrumRefresh = refresh;
                SpectrumWaterfallSettings.Save();
            }
        }
    }

    private void SelectPanResolutionList()
    {
        if (PanResolutionList == null) return;
        PanResolutionList.SelectionChanged -= OnPanResolutionSelectionChanged;
        int idx = Math.Clamp(SpectrumWaterfallSettings.PanResolutionIndex, 0, 2);
        if (idx < PanResolutionList.Items.Count)
            PanResolutionList.SelectedIndex = idx;
        PanResolutionList.SelectionChanged += OnPanResolutionSelectionChanged;
    }

    private void OnPanResolutionSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (sender is not ListBox lb || lb.SelectedItem is not ListBoxItem item) return;
        int idx = 0;
        if (item.Tag != null && int.TryParse(item.Tag.ToString(), out int tagIdx))
            idx = Math.Clamp(tagIdx, 0, 2);
        else
            idx = Math.Clamp(lb.SelectedIndex, 0, 2);

        SpectrumWaterfallSettings.PanResolutionIndex = idx;
        SpectrumWaterfallSettings.Save();

        // Push to SDRcore via MainViewModel / radio service when available
        try
        {
            if (Owner is MainWindow mw && mw.ViewModel != null)
                mw.ViewModel.ApplyPanResolution("S/W");
        }
        catch { /* ignore if window not owned by MainWindow */ }
    }

    private void OnWaterfallPaletteSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (sender is ListBox lb && lb.SelectedItem is ListBoxItem item)
        {
            string name = item.Content?.ToString() ?? "Red/Yellow";
            SpectrumColorSettings.SetWaterfallPalette(name);
            SpectrumWaterfallSettings.WaterfallPalette = SpectrumColorSettings.WaterfallPaletteName;
            SpectrumWaterfallSettings.SaveLiveWaterfallAndActiveBank();
            // Next waterfall render line uses SpectrumColorSettings.WaterfallScheme
        }
    }

    private void OnWaterfallHighChanged(object sender, RoutedPropertyChangedEventArgs<double> e) =>
        ApplyWaterfallRange((float)e.NewValue, SpectrumColorSettings.WaterfallLowDb, highIsPrimary: true);

    private void OnWaterfallLowChanged(object sender, RoutedPropertyChangedEventArgs<double> e) =>
        ApplyWaterfallRange(SpectrumColorSettings.WaterfallHighDb, (float)e.NewValue, highIsPrimary: false);

    private void WaterfallRangeReset_Click(object sender, RoutedEventArgs e) =>
        ApplyWaterfallRange(
            SpectrumColorSettings.WaterfallHighDefault,
            SpectrumColorSettings.WaterfallLowDefault,
            highIsPrimary: true);

    private void ApplyWaterfallRange(float highDb, float lowDb, bool highIsPrimary)
    {
        SpectrumColorSettings.SetWaterfallRange(highDb, lowDb, highIsPrimary);
        SpectrumWaterfallSettings.WaterfallHighDb = SpectrumColorSettings.WaterfallHighDb;
        SpectrumWaterfallSettings.WaterfallLowDb = SpectrumColorSettings.WaterfallLowDb;
        SpectrumWaterfallSettings.SaveLiveWaterfallAndActiveBank();

        if (WaterfallHighSlider != null &&
            Math.Abs(WaterfallHighSlider.Value - SpectrumColorSettings.WaterfallHighDb) > 0.1)
        {
            WaterfallHighSlider.ValueChanged -= OnWaterfallHighChanged;
            WaterfallHighSlider.Value = SpectrumColorSettings.WaterfallHighDb;
            WaterfallHighSlider.ValueChanged += OnWaterfallHighChanged;
        }
        if (WaterfallLowSlider != null &&
            Math.Abs(WaterfallLowSlider.Value - SpectrumColorSettings.WaterfallLowDb) > 0.1)
        {
            WaterfallLowSlider.ValueChanged -= OnWaterfallLowChanged;
            WaterfallLowSlider.Value = SpectrumColorSettings.WaterfallLowDb;
            WaterfallLowSlider.ValueChanged += OnWaterfallLowChanged;
        }
        UpdateWaterfallRangeValueText();
    }

    private void UpdateWaterfallRangeValueText()
    {
        if (WaterfallHighValueText != null)
            WaterfallHighValueText.Text = $"{SpectrumColorSettings.WaterfallHighDb:0}";
        if (WaterfallLowValueText != null)
            WaterfallLowValueText.Text = $"{SpectrumColorSettings.WaterfallLowDb:0}";
    }

    private void OnWaterfallGainChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
    {
        int val = (int)Math.Round(e.NewValue);
        SpectrumColorSettings.SetWaterfallGain(val);
        SpectrumWaterfallSettings.WaterfallGain = SpectrumColorSettings.WaterfallGain;
        SpectrumWaterfallSettings.SaveLiveWaterfallAndActiveBank();
        if (WaterfallGainValueText != null)
            WaterfallGainValueText.Text = SpectrumColorSettings.WaterfallGain.ToString();
    }

    private void OnWaterfallZeroChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
    {
        int val = (int)Math.Round(e.NewValue);
        SpectrumColorSettings.SetWaterfallZero(val);
        SpectrumWaterfallSettings.WaterfallZero = SpectrumColorSettings.WaterfallZero;
        SpectrumWaterfallSettings.SaveLiveWaterfallAndActiveBank();
        if (WaterfallZeroValueText != null)
            WaterfallZeroValueText.Text = SpectrumColorSettings.WaterfallZero.ToString();
    }

    private void OnWaterfallTimeMarkerClick(object sender, RoutedEventArgs e)
    {
        int val = SpectrumColorSettings.CycleWaterfallTimeMarker();
        SpectrumWaterfallSettings.WaterfallTimeMarker = val;
        SpectrumWaterfallSettings.Save();
        if (WaterfallTimeMarkerButton != null)
            WaterfallTimeMarkerButton.Content = val.ToString();
    }

    private void OnWaterfallDirectionChanged(object sender, RoutedEventArgs e)
    {
        bool normal = WaterfallDirectionCheckBox?.IsChecked == true;
        SpectrumColorSettings.SetWaterfallDirectionNormal(normal);
        SpectrumWaterfallSettings.WaterfallDirectionNormal = normal;
        SpectrumWaterfallSettings.SaveLiveWaterfallAndActiveBank();
        UpdateWaterfallDirectionCheckboxText();
    }

    private void NormalizeAndSelectAutoSnapFreq()
    {
        // Map any legacy labels to the three original list items
        int hz = SpectrumWaterfallSettings.GetAutoSnapStepHz();
        string label = hz switch
        {
            1000 => "1KHz",
            500 => "500Hz",
            _ => "100Hz"
        };
        SpectrumWaterfallSettings.SpectrumAutoSnapFreq = label;
        SelectItemByContent(AutoSnapFreqList, label);
    }

    private void OnAutoSnapCheckedChanged(object sender, RoutedEventArgs e)
    {
        SpectrumWaterfallSettings.SpectrumAutoSnap = AutoSnapCheckBox?.IsChecked == true;
        SpectrumWaterfallSettings.Save();
    }

    private void OnAutoSnapFreqSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (sender is ListBox lb && lb.SelectedItem is ListBoxItem item)
        {
            string label = item.Content?.ToString() ?? "100Hz";
            SpectrumWaterfallSettings.SpectrumAutoSnapFreq = label;
            SpectrumWaterfallSettings.Save();
        }
    }

    private void OnViewGridCheckedChanged(object sender, RoutedEventArgs e)
    {
        bool on = ViewGridCheckBox?.IsChecked == true;
        SpectrumWaterfallSettings.SpectrumViewGrid = on;
        SpectrumColorSettings.SetViewGrid(on);
        SpectrumWaterfallSettings.Save();
    }

    private void OnViewDbLabelsCheckedChanged(object sender, RoutedEventArgs e)
    {
        bool on = ViewDbLabelsCheckBox?.IsChecked == true;
        SpectrumWaterfallSettings.SpectrumViewDbLabels = on;
        SpectrumColorSettings.SetViewDbLabels(on);
        SpectrumWaterfallSettings.Save();
    }

    private void OnPeakMarkerCheckedChanged(object sender, RoutedEventArgs e)
    {
        bool on = PeakMarkerCheckBox?.IsChecked == true;
        SpectrumWaterfallSettings.SpectrumPeakMarker = on;
        SpectrumColorSettings.SetPeakMarker(on);
        SpectrumWaterfallSettings.Save();
    }

    /// <summary>
    /// TIME DISPLAY lives in S/W spectrum block; keeps MainViewModel clock bar in sync.
    /// </summary>
    private void OnTimeDisplayCheckedChanged(object sender, RoutedEventArgs e)
    {
        bool on = TimeDisplayCheckBox?.IsChecked == true;
        if (Application.Current?.MainWindow?.DataContext is ViewModels.MainViewModel vm)
        {
            // Property change saves INI + starts/stops clock timer
            vm.TimeDisplayOn = on;
        }
        else
        {
            SpectrumWaterfallSettings.TimeDisplayOn = on;
            SpectrumWaterfallSettings.Save();
        }
    }
}

// Simple view model for the window (can be expanded or replaced with bindings to main VM)
public class PanadapterControlsViewModel
{
    // Spectrum
    public string[] SpectrumLineColors { get; } = { "Azure", "Red", "Green", "Blue", "Yellow" };
    public string SelectedSpectrumLineColor { get; set; } = "Azure";

    public string[] SpectrumFillColors { get; } = { "Azure", "Red", "Green", "Blue", "Yellow" };
    public string SelectedSpectrumFillColor { get; set; } = "Azure";

    public string[] SpectrumBackgroundColors { get; } = { "Black", "DarkBlue", "DarkGreen", "Gray" };
    public string SelectedSpectrumBackgroundColor { get; set; } = "Black";

    public bool SpectrumShowGrid { get; set; } = true;
    public double SpectrumGain { get; set; } = 50;
    public double SpectrumBaseLine { get; set; } = 50;

    // Waterfall (original MSCC palettes)
    public string[] WaterfallPalettes { get; } = { "Red/Yellow", "Enhanced", "Spectran", "BlackWhite" };
    public string SelectedWaterfallPalette { get; set; } = "Red/Yellow";

    public double WaterfallGain { get; set; } = 50;  // 0–100, 50 = neutral
    public double WaterfallZero { get; set; } = 0;   // 0–100, 0 = no offset
    public bool WaterfallDirection { get; set; } = true; // true = Direction Normal
    public double WaterfallSpeed { get; set; } = 10;
}