using System;
using System.ComponentModel;
using System.Globalization;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;
using MSCC.Core.Domain;
using MSCC.Core.Services;
using MSCC.Wpf.ViewModels;

namespace MSCC.Wpf;

public partial class MainWindow : Window
{
    public MainViewModel? ViewModel => DataContext as MainViewModel;
    private bool _pwrCalTabActive;
    private bool _ampCalTabActive;
    private bool _txIqTabActive;
    private bool _rxIqTabActive;

    public MainWindow()
    {
        try
        {
            InitializeComponent();

            // First launch / missing AppData: copy init-files templates (never overwrite existing).
            // Replaces Initialize.bat for normal installs.
            bool wasFirstRun = ConfigBootstrap.IsLikelyFirstRun();
            int seeded = ConfigBootstrap.SeedMissingConfigFiles();

            // Restore last window position/size/state from MSCC_Client.ini
            RestoreWindowPlacement();

            // Apply UI chrome (window bg + gold buttons) from Settings → UI Appearance names in INI
            ApplyUiChromeTheme();

            // Default (and only): use the real UdpRadioService.
            // It launches the subsystems (ms-sdr-MKII.exe, mscc-recv.exe, Mscc-trans.exe) from AppContext.BaseDirectory
            // (the folder containing this exe, typically C:\mscc-net9 after build copy).
            // All client settings (including connection) now load from MSCC_Client.ini.
            DataContext = new MainViewModel();

            // Load client settings (MSCC_Client.ini) at startup (spectrum, window, time display, etc.).
            SpectrumWaterfallSettings.Load();

            // Auto-scroll the debug log list when new ViewModel?.MonitorTextBoxText messages arrive.
            // Uses the ObservableCollection (virtualized ListBox in Aud/Sys tab) for performance with high-volume logging.
            if (DataContext is MainViewModel vm)
            {
                if (seeded > 0)
                    vm.MonitorTextBoxText($" Config seed: copied {seeded} template file(s) into %LocalAppData%\\MSCC-NET9");
                vm.RefreshSetupStatus();
                vm.TimeDisplayOn = SpectrumWaterfallSettings.TimeDisplayOn;
                // Re-apply after SpectrumWaterfallSettings.Load() in this ctor (VM may have read earlier static load).
                // Set backing preference without relying on property order: assign after load.
                if (vm.AutoStartServers != SpectrumWaterfallSettings.AutoStartServers)
                    vm.AutoStartServers = SpectrumWaterfallSettings.AutoStartServers;
                if (vm.LaunchServersOnStart != SpectrumWaterfallSettings.LaunchServersOnStart)
                    vm.LaunchServersOnStart = SpectrumWaterfallSettings.LaunchServersOnStart;
                // External electronic keyer sticky (PROFICIO-MKII) — re-sync after Load().
                if (vm.ExternalElectronicKeyer != SpectrumWaterfallSettings.ExternalElectronicKeyer)
                    vm.ExternalElectronicKeyer = SpectrumWaterfallSettings.ExternalElectronicKeyer;

                // Analog meter Peak/HOLD (sticky). Set fields before first meter paint if possible;
                // property set will Save again (harmless).
                vm.SmeterHold = SpectrumWaterfallSettings.SmeterHold;
                vm.PeakNeedleOn = SpectrumWaterfallSettings.SmeterPeak;
                vm.AlcHold = SpectrumWaterfallSettings.AlcHold;
                vm.AlcPeakNeedleOn = SpectrumWaterfallSettings.AlcPeak;
                // AMP defaults off at startup; use AMP-off Tune Power store.
                vm.TunePowerPercent = SpectrumWaterfallSettings.GetTunePowerForAmp(vm.AmpOn);
                vm.CwPowerPercent = SpectrumWaterfallSettings.CwPower;
                vm.SsbPowerPercent = SpectrumWaterfallSettings.SsbPower;
                vm.AmCarrierPercent = SpectrumWaterfallSettings.AmCarrier;
                vm.StepIndex = SpectrumWaterfallSettings.StepIndex;
                vm.MonitorTextBoxText(" MainWindow ctor complete (single VM, XAML DC removed)");

                // Subscribe so that band reports (CMD_GET_SET_STARTUP_BAND etc.) that set RadioState.CurrentBand
                // will update the visual "active" highlight on the matching band button (and the band label).
                vm.RadioState.PropertyChanged += (s, e) =>
                {
                    if (e.PropertyName == nameof(RadioState.CurrentBand))
                    {
                        Dispatcher.BeginInvoke(UpdateBandButtonVisuals);
                    }
                };

                // AMP gate: leave AMP CAL when AMP off; leave TRANS CAL / TX IQ when AMP on.
                vm.PropertyChanged += (_, e) =>
                {
                    if (e.PropertyName != nameof(MainViewModel.AmpOn)) return;
                    Dispatcher.BeginInvoke(LeaveCalTabsIfAmpGateClosed);
                };

                // Initial band highlight + Proficio default (LF grayed) + GEN list after layout.
                // First run / incomplete local setup: open Settings once so COM + audio can be set.
                Dispatcher.BeginInvoke(() =>
                {
                    ApplyRadioModelBandGating();
                    SyncGenButtonForRadioModel(retuneIfOnGen: false);
                    vm.RefreshSetupStatus();
                    if (wasFirstRun || (vm.LaunchServersOnStart && !vm.CanStartLocalRadioSession))
                    {
                        SelectSettingsTab();
                        vm.MonitorTextBoxText(
                            " Setup: open Settings → set COM port and operator speaker, then press Start");
                    }
                });

                // Wire calibration progress/status reports (for Freq Cal tab CHECK implementation)
                vm.RadioService.CalProgressReported += OnCalProgressReported;
                vm.RadioService.CalStatusReported += OnCalStatusReported;
                vm.RadioService.CalDeltaReported += OnCalDeltaReported;
            }

            // Hook close (X button / Alt-F4 / system close) so we can send CMD_SET_STOP (0xFF) to ms-sdr
            // (matching original Form1_Closing + Shutdown_by_System) + kill any child subsystems we launched.
            // This prevents zombie/orphaned processes (the "zombie" behavior when just letting the process die).
            this.Closing += MainWindow_Closing;

            // Additional safety: log when the window is actually laid out and visible, and force it to front.
            this.Loaded += MainWindow_Loaded;
        }
        catch (Exception ex)
        {
            // Ensure we always log the root cause to the file (even if DebugMonitor not fully wired).
            // This surfaces XAML/runtime load errors that previously caused "logs but no UI".
            try
            {
                MSCC.Core.Logging.DebugMonitor.MonitorTextBoxText($"!!! MAINWINDOW CTOR/XAML EXCEPTION: {ex.Message}");
            }
            catch { }

            try
            {
                string logDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "MSCC-NET9", "logs");
                Directory.CreateDirectory(logDir);
                File.AppendAllText(Path.Combine(logDir, "crash.log"),
                    $"{DateTime.Now:yyyy-MM-dd HH:mm:ss} MAINWINDOW LOAD CRASH:\n{ex}\n\n");
            }
            catch { }

            // Show a visible error UI instead of a blank/no window (so user sees the problem + log hint).
            var errorContent = new TextBlock
            {
                Text = "MSCC failed to initialize the main window (see logs).\n\n" +
                       "Check %LocalAppData%\\MSCC-NET9\\logs\\mscc.log and crash.log for the full exception.\n\n" +
                       ex.ToString(),
                Foreground = Brushes.OrangeRed,
                Background = Brushes.Black,
                FontFamily = new FontFamily("Consolas"),
                FontSize = 11,
                Padding = new Thickness(12),
                TextWrapping = TextWrapping.Wrap
            };
            Content = errorContent;
            // Let the window still appear with the error details.
        }
    }

    private void SpectrumControl_FrequencyClicked(object? sender, long frequency)
    {
        // Click-to-tune path: apply AUTO SNAP (S/W popup) when enabled and not CW
        ViewModel.TuneToFrequency(frequency, fromSpectrumClick: true);
    }

    private void ClearRit_Click(object sender, RoutedEventArgs e)
    {
        if (ViewModel != null)
        {
            ViewModel.RadioState.ActiveVfo.RitOn = false;
            ViewModel.RadioState.ActiveVfo.RitOffsetHz = 0;
            ViewModel.MonitorTextBoxText(" Clear RIT clicked");
        }
    }

    /// <summary>
    /// Toggles Proficio ↔ Geminus label and grays out the other radio's band buttons.
    /// Does not yet change frequency or talk to the hardware about model type.
    /// </summary>
    private void RadioModelButton_Click(object sender, RoutedEventArgs e)
    {
        if (sender is not Button btn) return;
        string current = btn.Content?.ToString() ?? "Proficio";
        bool nowGeminus = !current.Equals("Geminus", StringComparison.OrdinalIgnoreCase);
        btn.Content = nowGeminus ? "Geminus" : "Proficio";
        // Swap HF/LF waterfall banks (High/Low/Gain/Zero/Palette) before gating
        SpectrumWaterfallSettings.SwitchRadioModelWaterfall(nowGeminus);
        ApplyRadioModelBandGating();
        // GEN/USER list is model-specific (HF beacons vs LF freq-cal carriers)
        SyncGenButtonForRadioModel(retuneIfOnGen: true);
        if (ViewModel != null)
        {
            ViewModel.IsGeminusRadioModel = nowGeminus;
            ViewModel.MonitorTextBoxText(
                nowGeminus
                    ? " Radio model: Geminus — LF waterfall bank; HF grayed; 2200/630 on"
                    : " Radio model: Proficio — HF waterfall bank; LF grayed; GEN=WWV/CHU/RWM/USER");
        }
    }

    private void SwrSettingsPanel_ApplyRequested()
    {
        ViewModel?.ApplySwrMeterSettings("settings");
        if (SwrSettingsPanel != null && ViewModel != null)
            SwrSettingsPanel.SetStatus(ViewModel.SwrStatusText);
    }

    private async void SwrMeterFace_ResetRequested(object sender, RoutedEventArgs e)
    {
        if (ViewModel == null) return;
        await ViewModel.ResetSwrFaultAsync();
    }

    /// <summary>
    /// Live-apply UI chrome from <see cref="SpectrumWaterfallSettings"/> (Settings → UI Appearance).
    /// Updates DynamicResource brushes so MsccGoldButtonStyle and window background refresh immediately.
    /// </summary>
    public void ApplyUiChromeTheme()
    {
        try
        {
            Color bg = UiChromeTheme.ResolveBackground();
            Color panel = UiChromeTheme.ResolvePanel(bg);
            Color face = UiChromeTheme.ResolveButtonFace();
            Color border = UiChromeTheme.Darken(face, 0x22);
            Color hover = UiChromeTheme.Lighten(face, 0x22);
            // Selected = noticeably darker shade of the same button color
            Color selected = UiChromeTheme.Darken(face, 0x40);
            Color selectedBorder = UiChromeTheme.Darken(face, 0x55);
            // Readable text: dark on light faces, light on dark faces
            double lum = 0.299 * face.R + 0.587 * face.G + 0.114 * face.B;
            Color text = lum > 140 ? Colors.Black : Colors.White;

            // Text on panels (Volume, Temps, Versions, …): contrast against panel fill
            double panelLum = 0.299 * panel.R + 0.587 * panel.G + 0.114 * panel.B;
            Color primaryText = panelLum > 140 ? Colors.Black : Colors.White;
            // Match S/W page label gray (#D0D0D0) on dark panels; darker mute on light panels.
            Color mutedText = panelLum > 140
                ? Color.FromRgb(0x44, 0x44, 0x44)
                : Color.FromRgb(0xD0, 0xD0, 0xD0);

            SetResourceBrushColor("UiWindowBackgroundBrush", bg);
            SetResourceBrushColor("UiPanelBackgroundBrush", panel);
            SetResourceBrushColor("UiPrimaryTextBrush", primaryText);
            SetResourceBrushColor("UiMutedTextBrush", mutedText);
            SetResourceBrushColor("UiButtonFaceBrush", face);
            SetResourceBrushColor("UiButtonBorderBrush", border);
            SetResourceBrushColor("UiButtonHoverBrush", hover);
            SetResourceBrushColor("UiButtonHoverBorderBrush", face);
            SetResourceBrushColor("UiButtonPressedBrush", selected);
            SetResourceBrushColor("UiButtonTextBrush", text);
            SetResourceBrushColor("UiButtonSelectedBrush", selected);
            SetResourceBrushColor("UiButtonSelectedBorderBrush", selectedBorder);
            // Fixed alert red for PTT / TUN / AMP when latched on
            SetResourceBrushColor("UiButtonAlertBrush", Color.FromRgb(0xE5, 0x39, 0x35));
            SetResourceBrushColor("UiButtonAlertBorderBrush", Color.FromRgb(0xB7, 0x1C, 0x1C));

            // Ensure window background is bound/updated even if style missed it
            if (Resources["UiWindowBackgroundBrush"] is SolidColorBrush winBrush)
                Background = winBrush;

            // Refresh band "active" paints so selected band tracks the new palette
            UpdateBandButtonVisuals();
        }
        catch (Exception ex)
        {
            try { ViewModel?.MonitorTextBoxText($" ApplyUiChromeTheme error: {ex.Message}"); } catch { }
        }
    }

    private void SetResourceBrushColor(string key, Color color)
    {
        if (Resources[key] is SolidColorBrush brush)
        {
            // Replace brush so DynamicResource consumers pick up the change reliably
            var next = new SolidColorBrush(color);
            next.Freeze();
            Resources[key] = next;
        }
        else
        {
            var created = new SolidColorBrush(color);
            created.Freeze();
            Resources[key] = created;
        }
    }

    /// <summary>
    /// True when the UI radio-model control is set to Geminus (label text).
    /// </summary>
    private bool IsGeminusModelSelected()
    {
        var btn = this.FindName("RadioModelButton") as Button;
        string label = btn?.Content?.ToString() ?? "Proficio";
        return label.Equals("Geminus", StringComparison.OrdinalIgnoreCase);
    }

    /// <summary>
    /// Enable/disable band buttons by selected radio model:
    /// Proficio → HF (160–10) on, LF (2200/630) grayed;
    /// Geminus → LF on, HF grayed.
    /// GEN stays enabled for both (LF GEN presets later).
    /// </summary>
    private void ApplyRadioModelBandGating()
    {
        var panel = this.FindName("BandButtonsPanel") as StackPanel;
        if (panel == null) return;

        bool geminus = IsGeminusModelSelected();

        // Geminus: deeper S/W BASELINE pull-down for LF noise floor; Proficio keeps HF mapping.
        Controls.SpectrumColorSettings.SetGeminusBaselineRange(geminus);
        if (ViewModel != null)
            ViewModel.IsGeminusRadioModel = geminus;

        foreach (var child in panel.Children)
        {
            if (child is not Button btn) continue;

            if (btn.Name == "GenBandButton")
            {
                btn.IsEnabled = true;
                continue;
            }

            if (btn.Name == "Band2200Button" || btn.Name == "Band630Button")
            {
                // LF: Geminus only
                btn.IsEnabled = geminus;
                continue;
            }

            // Remaining named/unnamed band buttons in the panel are HF (160–10)
            btn.IsEnabled = !geminus;
        }

        // Re-apply active highlight so disabled buttons don't keep "active" gold/red look
        UpdateBandButtonVisuals();
    }

    private void BandButton_Click(object sender, RoutedEventArgs e)
    {
        if (sender is Button btn && long.TryParse(btn.Tag?.ToString(), out long freq) && ViewModel != null)
        {
            // Respect radio-model gating (disabled buttons should not fire, but guard anyway).
            if (!btn.IsEnabled)
                return;

            string currentBand = ViewModel.RadioState.CurrentBand;
            if (!string.IsNullOrEmpty(currentBand))
            {
                ViewModel.SaveLastUsedForCurrentBand();
            }

            string bandName = GetBandNameForFreq(freq);
            ViewModel.RadioState.CurrentBand = bandName;  // switch band first so subsequent freq/mode/filter sets save under the correct band
            ViewModel.LoadLastUsedForBand(bandName, freq);
            ViewModel.MonitorTextBoxText($" Band button clicked: {bandName} @ {freq}");

            UpdateBandButtonVisuals();
        }
    }

    private void GenBandButton_Click(object sender, RoutedEventArgs e)
    {
        if (ViewModel == null) return;
        var btn = sender as Button;
        if (btn == null) return;

        var opts = GetActiveGenOptions();
        string currentBand = ViewModel.RadioState.CurrentBand ?? "";
        bool enteringGen = currentBand != "gen";

        if (enteringGen && !string.IsNullOrEmpty(currentBand))
        {
            ViewModel.SaveLastUsedForCurrentBand();
        }

        // Determine the sub-preset to activate:
        // - If entering GEN from another band: use whatever label is currently shown on the button (no rotate).
        // - If already on GEN: advance to the next (rotate).
        string currentLabel = btn.Content?.ToString() ?? opts[0].Label;
        int currentIdx = Array.FindIndex(opts, o => o.Label == currentLabel);
        if (currentIdx < 0) currentIdx = 0;

        if (enteringGen)
        {
            _genIndex = currentIdx;   // do NOT rotate on entry click
        }
        else
        {
            _genIndex = (currentIdx + 1) % opts.Length;
        }

        var opt = opts[_genIndex];
        long freq = opt.Freq;
        btn.Content = opt.Label;
        btn.Tag = freq.ToString();
        _currentGenLabel = opt.Label;
        ViewModel.CurrentGenSub = _currentGenLabel;

        string bandName = "gen";
        _previousBand = "gen";
        ViewModel.RadioState.CurrentBand = bandName;

        long useFreq = freq;
        // Proficio USER slot last-used is per-VFO (A vs B files). Geminus has only fixed cal carriers.
        var (savedF, m, l, h, c) = SpectrumWaterfallSettings.LoadLastUsedForBand(
            bandName, forVfoB: ViewModel.UseVfoBLastUsedFile);
        if (opt.Label == "USER" && savedF > 0)
            useFreq = savedF;

        string useM = !string.IsNullOrEmpty(m) ? m : MainViewModel.DefaultModeForFrequency(useFreq);

        // Fixed presets (WWV/CHU/RWM or LF cal) do not overwrite USER last-used
        ViewModel.SuppressLastUsedSave = (opt.Label != "USER");
        ViewModel.TuneToFrequency(useFreq);
        ViewModel.ActiveMode = useM;
        if (l >= 0) ViewModel.LowCutIndex = l;
        if (h >= 0) ViewModel.HighCutIndex = h;
        if (c >= 0) ViewModel.CwFilterIndex = c;
        ViewModel.SuppressLastUsedSave = false;

        btn.Tag = useFreq.ToString();

        string model = IsGeminusModelSelected() ? "Geminus" : "Proficio";
        ViewModel.MonitorTextBoxText(
            enteringGen
                ? $" Gen ({model}) to {opt.Label} @ {useFreq}"
                : $" Gen ({model}) rotated to {opt.Label} @ {useFreq}");

        UpdateBandButtonVisuals();
    }

    /// <summary>Proficio GEN list vs Geminus LF freq-cal carriers.</summary>
    private (string Label, long Freq)[] GetActiveGenOptions() =>
        IsGeminusModelSelected() ? _genOptionsGeminus : _genOptionsProficio;

    /// <summary>
    /// After Proficio ↔ Geminus switch, put GEN button on the correct preset list.
    /// If already on "gen", retune to the selected LF/HF carrier so the radio matches the button.
    /// </summary>
    private void SyncGenButtonForRadioModel(bool retuneIfOnGen)
    {
        var opts = GetActiveGenOptions();
        var btn = this.FindName("GenBandButton") as Button;
        if (btn == null) return;

        // Prefer match by current frequency; else first entry of the active list
        long freqNow = ViewModel?.RadioState.ActiveVfo.FrequencyHz ?? 0;
        int idx = Array.FindIndex(opts, o => o.Freq == freqNow);
        if (idx < 0)
        {
            // Keep label if it exists on the new list
            string label = btn.Content?.ToString() ?? "";
            idx = Array.FindIndex(opts, o => o.Label == label);
        }
        if (idx < 0) idx = 0;

        _genIndex = idx;
        var opt = opts[_genIndex];
        btn.Content = opt.Label;
        btn.Tag = opt.Freq.ToString();
        _currentGenLabel = opt.Label;
        if (ViewModel != null)
            ViewModel.CurrentGenSub = _currentGenLabel;

        bool onGen = string.Equals(ViewModel?.RadioState.CurrentBand, "gen", StringComparison.OrdinalIgnoreCase);
        if (retuneIfOnGen && onGen && ViewModel != null)
        {
            ViewModel.SuppressLastUsedSave = true;
            try
            {
                ViewModel.TuneToFrequency(opt.Freq);
                ViewModel.ActiveMode = MainViewModel.DefaultModeForFrequency(opt.Freq);
            }
            finally
            {
                ViewModel.SuppressLastUsedSave = false;
            }
            ViewModel.MonitorTextBoxText($" Gen list switched for radio model → {opt.Label} @ {opt.Freq}");
        }

        UpdateBandButtonVisuals();
    }

    /// <summary>
    /// If AMP state invalidates the current cal tab, switch to MAIN (leave handlers run via SelectionChanged).
    /// </summary>
    private void LeaveCalTabsIfAmpGateClosed()
    {
        if (ViewModel == null || MainTabControl == null) return;
        var selected = MainTabControl.SelectedItem as TabItem;
        if (selected == null) return;

        bool onAmpCal = ReferenceEquals(selected, AmpCalTabItem);
        bool onTransCal = ReferenceEquals(selected, PwrCalTabItem);
        bool onTxIq = ReferenceEquals(selected, TxIqTabItem);

        // AMP off → AMP CAL disabled; AMP on → TRANS CAL / TX IQ disabled
        if ((onAmpCal && !ViewModel.IsAmpCalTabEnabled) ||
            (onTransCal && !ViewModel.IsPowerCalTabEnabled) ||
            (onTxIq && !ViewModel.IsTxIqTabEnabled))
        {
            // First tab is MAIN
            if (MainTabControl.Items.Count > 0)
                MainTabControl.SelectedIndex = 0;
        }
    }

    /// <summary>
    /// Snapshot operating state on enter TRANS CAL / AMP CAL; restore+send to ms-sdr on leave.
    /// </summary>
    private void MainTabControl_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        // Only our main tab strip (ignore nested ListBox/ComboBox SelectionChanged bubble)
        if (!ReferenceEquals(sender, MainTabControl) && !ReferenceEquals(e.Source, MainTabControl))
            return;
        if (ViewModel == null) return;

        var selected = MainTabControl.SelectedItem as TabItem;
        bool nowPwrCal = selected != null &&
                         (ReferenceEquals(selected, PwrCalTabItem) ||
                          string.Equals(selected.Header?.ToString(), "QRP CAL", StringComparison.Ordinal) ||
                          string.Equals(selected.Header?.ToString(), "TRANS CAL", StringComparison.Ordinal));
        bool nowAmpCal = selected != null &&
                         (ReferenceEquals(selected, AmpCalTabItem) ||
                          string.Equals(selected.Header?.ToString(), "AMP CAL", StringComparison.Ordinal));
        bool nowTxIq = selected != null &&
                       (ReferenceEquals(selected, TxIqTabItem) ||
                        string.Equals(selected.Header?.ToString(), "TX IQ", StringComparison.Ordinal));
        bool nowRxIq = selected != null &&
                       (ReferenceEquals(selected, RxIqTabItem) ||
                        string.Equals(selected.Header?.ToString(), "RX IQ", StringComparison.Ordinal));

        if (nowPwrCal && !_pwrCalTabActive)
        {
            _pwrCalTabActive = true;
            ViewModel.EnterPowerCalTab();
        }
        else if (!nowPwrCal && _pwrCalTabActive)
        {
            _pwrCalTabActive = false;
            ViewModel.LeavePowerCalTab();
            // Band button highlight may need refresh after CurrentBand restore
            Dispatcher.BeginInvoke(UpdateBandButtonVisuals);
        }

        if (nowAmpCal && !_ampCalTabActive)
        {
            _ampCalTabActive = true;
            ViewModel.EnterAmpCalTab();
        }
        else if (!nowAmpCal && _ampCalTabActive)
        {
            _ampCalTabActive = false;
            ViewModel.LeaveAmpCalTab();
            Dispatcher.BeginInvoke(UpdateBandButtonVisuals);
        }

        if (nowTxIq && !_txIqTabActive)
        {
            _txIqTabActive = true;
            ViewModel.EnterTxIqTab();
        }
        else if (!nowTxIq && _txIqTabActive)
        {
            _txIqTabActive = false;
            ViewModel.LeaveTxIqTab();
            Dispatcher.BeginInvoke(UpdateBandButtonVisuals);
        }

        if (nowRxIq && !_rxIqTabActive)
        {
            _rxIqTabActive = true;
            ViewModel.EnterRxIqTab();
        }
        else if (!nowRxIq && _rxIqTabActive)
        {
            _rxIqTabActive = false;
            ViewModel.LeaveRxIqTab();
            Dispatcher.BeginInvoke(UpdateBandButtonVisuals);
        }
    }

    private void StartStop_Click(object sender, RoutedEventArgs e)
    {
        if (ViewModel == null) return;

        // Toggle: Start when idle, Stop when running (Stop+Start = restart after COM change).
        if (ViewModel.IsRadioRunning)
        {
            ViewModel.StopRadioService("manual");
            ViewModel.RefreshSetupStatus();
            return;
        }

        if (ViewModel.LaunchServersOnStart)
        {
            var setup = ConfigBootstrap.EvaluateLocalSetup(launchServers: true);
            if (!setup.IsComplete)
            {
                ViewModel.RefreshSetupStatus();
                var result = MessageBox.Show(
                    ConfigBootstrap.FormatStartBlockedMessage(setup) + "\n\nOpen Settings now?",
                    "MSCC — setup needed",
                    MessageBoxButton.YesNo,
                    MessageBoxImage.Information,
                    MessageBoxResult.Yes);
                if (result == MessageBoxResult.Yes)
                    SelectSettingsTab();
                return;
            }
        }

        ViewModel.StartRadioService("manual");
    }

    /// <summary>Select the SETTINGS tab (first-run / setup incomplete).</summary>
    public void SelectSettingsTab()
    {
        try
        {
            if (MainTabControl == null) return;
            foreach (var item in MainTabControl.Items)
            {
                if (item is TabItem ti &&
                    string.Equals(ti.Header?.ToString(), "SETTINGS", StringComparison.OrdinalIgnoreCase))
                {
                    MainTabControl.SelectedItem = ti;
                    break;
                }
            }
        }
        catch { /* ignore */ }
    }

    private void ServerAddress_PreviewKeyDown(object sender, KeyEventArgs e)
    {
        if (e.Key == Key.Enter || e.Key == Key.Return)
        {
            if (sender is TextBox tb)
            {
                // Commit the text to the binding source (triggers OnBackendIp/PortChanged and the popup)
                var binding = BindingOperations.GetBindingExpression(tb, TextBox.TextProperty);
                binding?.UpdateSource();
            }
            e.Handled = true;
        }
    }

    private void Slider_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
    {
        if (sender is Slider slider)
        {
            double step = slider.SmallChange > 0 ? slider.SmallChange : 1.0;
            double delta = (e.Delta > 0) ? step : -step;
            slider.Value = Math.Max(slider.Minimum, Math.Min(slider.Maximum, slider.Value + delta));
            e.Handled = true;
        }
    }

    private void VfoFrequency_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
    {
        if (sender is not TextBlock freqTb || ViewModel == null) return;

        string tag = freqTb.Tag?.ToString() ?? "";
        VfoState targetVfo = tag == "A" ? ViewModel.RadioState.VfoA : ViewModel.RadioState.VfoB;

        // Frequency line is fixed F6 only (RIT is a separate line under the digits).
        string freqPart = freqTb.Text ?? "";
        if (string.IsNullOrWhiteSpace(freqPart)) return;

        Point pos = e.GetPosition(freqTb);
        if (pos.X < 0 || pos.X > freqTb.ActualWidth) return;

        // Measure with FormattedText for accurate per-character hit testing
        var typeface = new Typeface(freqTb.FontFamily, freqTb.FontStyle, freqTb.FontWeight, freqTb.FontStretch);
        double ppd = VisualTreeHelper.GetDpi(freqTb).PixelsPerDip;

        var ftFreq = new FormattedText(
            freqPart,
            CultureInfo.CurrentCulture,
            FlowDirection.LeftToRight,
            typeface,
            freqTb.FontSize,
            Brushes.Transparent,
            ppd);

        double freqPartWidth = ftFreq.Width;

        long step = 100; // fallback step
        bool isDigitPosition = false;

        if (pos.X < freqPartWidth)
        {
            // Find the char index whose rendered prefix first exceeds the mouse X
            int charIdx = 0;
            for (int i = 0; i < freqPart.Length; i++)
            {
                string prefix = freqPart.Substring(0, i + 1);
                var ftPre = new FormattedText(
                    prefix,
                    CultureInfo.CurrentCulture,
                    FlowDirection.LeftToRight,
                    typeface,
                    freqTb.FontSize,
                    Brushes.Transparent,
                    ppd);
                if (pos.X < ftPre.Width)
                {
                    charIdx = i;
                    break;
                }
                charIdx = i;
            }

            char c = freqPart[charIdx];
            if (!char.IsDigit(c))
            {
                // Over '.' or other separator: use the digit immediately to the left (if any)
                int leftDigitIdx = -1;
                for (int j = charIdx - 1; j >= 0; j--)
                {
                    if (char.IsDigit(freqPart[j]))
                    {
                        leftDigitIdx = j;
                        break;
                    }
                }
                if (leftDigitIdx >= 0)
                {
                    charIdx = leftDigitIdx;
                }
                else
                {
                    // no digit to left, use default step
                    goto applyDelta;
                }
            }

            // Count digits strictly to the right of this one (determines the 10^power)
            int digitsToRight = 0;
            for (int j = charIdx + 1; j < freqPart.Length; j++)
            {
                if (char.IsDigit(freqPart[j]))
                    digitsToRight++;
            }

            step = 1;
            for (int k = 0; k < digitsToRight; k++)
                step *= 10;
            isDigitPosition = true;
        }

    applyDelta:
        long delta = (e.Delta > 0) ? step : -step;
        long current = targetVfo.FrequencyHz;
        long baseFreq = current;
        if (isDigitPosition && step > 0)
        {
            baseFreq = current - (current % step);
        }
        long newFreq = Math.Max(0, baseFreq + delta);

        ViewModel.SetVfoFrequency(targetVfo, newFreq);

        ViewModel.MonitorTextBoxText($" VFO wheel: {tag} digit step {step} new {newFreq}");

        e.Handled = true;
    }

    /// <summary>
    /// Walks up the visual tree from the hit element to see if the mouse is over
    /// an interactive control (Slider, Button, TextBox, CheckBox, etc.).
    /// If so, the global Step wheel must give up and not change VFO A.
    /// </summary>
    private bool IsOverControlThatOwnsWheel(object source)
    {
        if (source is not DependencyObject d) return false;
        DependencyObject current = d;
        while (current != null)
        {
            if (current is Slider ||
                current is Button ||
                current is TextBox ||
                current is CheckBox ||
                current is ListBox)
            {
                return true;
            }
            current = VisualTreeHelper.GetParent(current);
        }
        return false;
    }

    /// <summary>
    /// Global mouse wheel on the Main tab content area ONLY.
    /// Applies ONLY when the mouse is NOT hovering over any control.
    /// Uses the Step value (Step control under CONTROLS) to inc/dec VFO A.
    /// The handler is attached exclusively to the Grid inside the Main TabItem.
    /// </summary>
    private void MainTab_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
    {
        if (ViewModel == null) return;

        // If over any interactive control (slider, button like Step/Lo/Hi, textbox, checkbox, etc.),
        // give up — do not apply the global step. Let the control handle (or ignore) the wheel.
        object hit = e.OriginalSource ?? Mouse.DirectlyOver;
        if (IsOverControlThatOwnsWheel(hit))
        {
            return;
        }

        long stepHz = ViewModel.GetCurrentStepHz();
        long delta = (e.Delta > 0) ? stepHz : -stepHz;

        var vfoA = ViewModel.RadioState.VfoA;
        long newFreq = Math.Max(0, vfoA.FrequencyHz + delta);

        ViewModel.SetVfoFrequency(vfoA, newFreq);

        ViewModel.MonitorTextBoxText($" Global wheel (Main tab): Step={ViewModel.StepLabel} ({stepHz}Hz) freq={newFreq}");

        e.Handled = true;
    }

    private void MainWindow_Loaded(object sender, RoutedEventArgs e)
    {
        if (DataContext is MainViewModel vm)
        {
            vm.MonitorTextBoxText($" MainWindow_Loaded: ActualWidth={ActualWidth} Height={ActualHeight} Left={Left} Top={Top} WindowState={WindowState}");
            vm.RefreshSetupStatus();

            // Optional: launch subsystems automatically (AUTO_START_SERVERS in MSCC_Client.ini).
            // Deferred slightly so the window is visible first; StartAsync is re-entrant-safe.
            // Skipped silently when local COM/audio setup is incomplete (see SetupStatusLine).
            if (vm.AutoStartServers)
            {
                if (vm.LaunchServersOnStart && !vm.CanStartLocalRadioSession)
                {
                    vm.MonitorTextBoxText(
                        " Auto-start skipped — finish COM + operator speaker in Settings, then press Start");
                }
                else
                {
                    vm.MonitorTextBoxText(" Auto-start servers enabled → starting backend");
                    Dispatcher.BeginInvoke(new Action(() =>
                    {
                        if (DataContext is MainViewModel v && v.AutoStartServers)
                            v.StartRadioService("auto");
                    }), System.Windows.Threading.DispatcherPriority.ApplicationIdle);
                }
            }
        }

        // Force visible + foreground (harmless if already done in App). Helps with multi-monitor / activation edge cases.
        // Note: we no longer force WindowState=Normal here because RestoreWindowPlacement() in the ctor
        // may have set a saved Normal/Maximized state.
        try
        {
            if (Left < -10000 || Top < -10000) { Left = 100; Top = 100; } // recover off-screen
            this.Activate();
            this.Focus();
            this.Topmost = true;
            this.Topmost = false; // flash to top without staying on top
            this.BringIntoView();
        }
        catch (Exception ex)
        {
            if (DataContext is MainViewModel vm2) vm2.MonitorTextBoxText($" Loaded force-visible error: {ex.Message}");
        }

        // Ensure band highlight + radio-model gray-out + GEN list after layout.
        // Restore Proficio/Geminus from INI (RADIO_MODEL) so HF/LF waterfall banks match.
        Dispatcher.BeginInvoke(() =>
        {
            RestoreRadioModelButtonFromSettings();
            ApplyRadioModelBandGating();
            SyncGenButtonForRadioModel(retuneIfOnGen: false);
        });
    }

    /// <summary>Set RadioModelButton label from sticky RADIO_MODEL (Proficio / Geminus).</summary>
    private void RestoreRadioModelButtonFromSettings()
    {
        var btn = this.FindName("RadioModelButton") as Button;
        if (btn == null) return;
        bool geminus = SpectrumWaterfallSettings.RadioModelIsGeminus;
        btn.Content = geminus ? "Geminus" : "Proficio";
        if (ViewModel != null)
            ViewModel.IsGeminusRadioModel = geminus;
        // Live waterfall already loaded from the matching bank in SpectrumWaterfallSettings.Load()
        SpectrumWaterfallSettings.ApplyLiveWaterfallToRenderer();
    }

    private void MainWindow_Closing(object? sender, CancelEventArgs e)
    {
        try
        {
            MSCC.Core.Logging.DebugMonitor.MonitorTextBoxText(" === MainWindow_Closing (X button / Alt+F4 / system close) ===");
            if (DataContext is MainViewModel vm)
            {
                // Unsubscribe cal handlers
                vm.RadioService.CalProgressReported -= OnCalProgressReported;
                vm.RadioService.CalStatusReported -= OnCalStatusReported;
                vm.RadioService.CalDeltaReported -= OnCalDeltaReported;

                vm.MonitorTextBoxText(
                    " MainWindow_Closing: Dispose VM (STOP only if this client launched backends; connect-only leaves servers running)");
                vm.Dispose();
                vm.MonitorTextBoxText(" MainWindow_Closing: VM Dispose returned. MSCC client exiting.");
            }
            else
            {
                MSCC.Core.Logging.DebugMonitor.MonitorTextBoxText(" MainWindow_Closing: no ViewModel in DataContext; doing direct service stop if possible");
            }

            // Persist window position/size/state so next launch restores it
            SaveWindowPlacement();
        }
        catch (Exception ex)
        {
            try
            {
                MSCC.Core.Logging.DebugMonitor.MonitorTextBoxText($" MainWindow_Closing cleanup exception (non-fatal): {ex.Message}");
            }
            catch { }
        }
        // Do not set e.Cancel. Allow normal close after our cleanup.
    }

    private void RestoreWindowPlacement()
    {
        // Access triggers static ctor Load() which populates the Window* values from MSCC_Client.ini (client settings file).
        bool hasPosition = !double.IsNaN(SpectrumWaterfallSettings.WindowLeft) && !double.IsNaN(SpectrumWaterfallSettings.WindowTop);
        bool hasSize = !double.IsNaN(SpectrumWaterfallSettings.WindowWidth) && !double.IsNaN(SpectrumWaterfallSettings.WindowHeight);

        var virtualScreen = new Rect(
            SystemParameters.VirtualScreenLeft,
            SystemParameters.VirtualScreenTop,
            SystemParameters.VirtualScreenWidth,
            SystemParameters.VirtualScreenHeight);

        bool restored = false;

        if (hasPosition || hasSize)
        {
            double left = hasPosition ? SpectrumWaterfallSettings.WindowLeft : Left;
            double top = hasPosition ? SpectrumWaterfallSettings.WindowTop : Top;
            double width = hasSize ? SpectrumWaterfallSettings.WindowWidth : Width;
            double height = hasSize ? SpectrumWaterfallSettings.WindowHeight : Height;

            var candidate = new Rect(left, top, width, height);

            if (virtualScreen.IntersectsWith(candidate))
            {
                if (hasPosition)
                {
                    Left = SpectrumWaterfallSettings.WindowLeft;
                    Top = SpectrumWaterfallSettings.WindowTop;
                }
                if (hasSize)
                {
                    Width = SpectrumWaterfallSettings.WindowWidth;
                    Height = SpectrumWaterfallSettings.WindowHeight;
                }
                restored = true;
            }
            // else: saved position is off-screen (e.g. monitor removed); fall through to center
        }

        if (!restored)
        {
            // First run (no saved or invalid) or off-screen saved: center using the (XAML or restored-size) dimensions.
            // This gives the previous "CenterScreen" feel on initial launch while still allowing saved user positions to be honored.
            double w = Width;
            double h = Height;
            Left = virtualScreen.Left + (virtualScreen.Width - w) / 2;
            Top = virtualScreen.Top + (virtualScreen.Height - h) / 2;
        }

        // Apply persisted window state (Maximized is useful; Minimized on launch is not, so map it to Normal).
        if (SpectrumWaterfallSettings.WindowState == WindowState.Maximized)
        {
            WindowState = WindowState.Maximized;
        }
        else if (SpectrumWaterfallSettings.WindowState == WindowState.Minimized)
        {
            WindowState = WindowState.Normal;
        }
        // else Normal (or unparsable) -> leave as Normal (XAML default + our size/pos already applied).
    }

    private void SaveWindowPlacement()
    {
        // Only save the normal bounds; maximized state is saved separately
        if (WindowState == WindowState.Normal)
        {
            SpectrumWaterfallSettings.WindowLeft = Left;
            SpectrumWaterfallSettings.WindowTop = Top;
            SpectrumWaterfallSettings.WindowWidth = Width;
            SpectrumWaterfallSettings.WindowHeight = Height;
        }

        SpectrumWaterfallSettings.WindowState = WindowState;
        SpectrumWaterfallSettings.Save();
    }

    /// <summary>
    /// Settings → Reset configuration: wipe config files under %LocalAppData%\MSCC-NET9
    /// (keep logs\), reseed from install init-files, restart app.
    /// </summary>
    private void SettingsConfigReset_Click(object sender, RoutedEventArgs e)
    {
        var result = MessageBox.Show(
            "This will delete MSCC configuration files under:\n\n" +
            $"  {ConfigBootstrap.ConfigDirectory}\n\n" +
            "Logs are kept. Defaults will be restored if an init-files folder is next to MSCC.Wpf.exe.\n\n" +
            "MSCC will close and you should start it again.\n\nContinue?",
            "Reset configuration to defaults",
            MessageBoxButton.YesNo,
            MessageBoxImage.Warning,
            MessageBoxResult.No);

        if (result != MessageBoxResult.Yes)
            return;

        try
        {
            string dest = ConfigBootstrap.ConfigDirectory;
            Directory.CreateDirectory(dest);

            // Remove config files and subdirs except logs
            foreach (string file in Directory.GetFiles(dest))
            {
                try { File.Delete(file); } catch { /* best effort */ }
            }
            foreach (string dir in Directory.GetDirectories(dest))
            {
                string name = Path.GetFileName(dir);
                if (string.Equals(name, "logs", StringComparison.OrdinalIgnoreCase))
                    continue;
                try { Directory.Delete(dir, recursive: true); } catch { /* best effort */ }
            }

            // Full reseed (overwrite) from install templates
            string initSrc = ConfigBootstrap.InstallInitFilesDirectory;
            if (Directory.Exists(initSrc))
            {
                foreach (string srcFile in Directory.GetFiles(initSrc, "*", SearchOption.AllDirectories))
                {
                    string rel = Path.GetRelativePath(initSrc, srcFile);
                    string dstFile = Path.Combine(dest, rel);
                    string? dstDir = Path.GetDirectoryName(dstFile);
                    if (!string.IsNullOrEmpty(dstDir))
                        Directory.CreateDirectory(dstDir);
                    File.Copy(srcFile, dstFile, overwrite: true);
                }
            }

            try
            {
                string flag = Path.Combine(dest, "MSCC_INIT_COMPLETE.flag");
                if (File.Exists(flag)) File.Delete(flag);
                string seedFlag = Path.Combine(dest, ConfigBootstrap.SeedCompleteFlag);
                if (File.Exists(seedFlag)) File.Delete(seedFlag);
            }
            catch { /* ignore */ }

            MessageBox.Show(
                "Configuration reset complete.\n\nMSCC will now close. Start MSCC again — defaults seed automatically (no Initialize.bat needed).",
                "Reset complete",
                MessageBoxButton.OK,
                MessageBoxImage.Information);

            Application.Current.Shutdown();
        }
        catch (Exception ex)
        {
            MessageBox.Show(
                "Configuration reset failed:\n\n" + ex.Message,
                "Reset failed",
                MessageBoxButton.OK,
                MessageBoxImage.Error);
        }
    }

    /// <summary>
    /// Applies (or clears) the "active/selected" visual state on the band button whose band matches
    /// RadioState.CurrentBand. Active band uses themed darker selected brushes (not TX alert red).
    /// </summary>
    private void UpdateBandButtonVisuals()
    {
        if (ViewModel == null) return;
        var panel = this.FindName("BandButtonsPanel") as StackPanel;
        if (panel == null) return;

        string current = (ViewModel.RadioState.CurrentBand ?? string.Empty).Trim().ToLowerInvariant();

        // Sync label from freq only on transition into gen (rotation while on gen manages its own label).
        if (current == "gen" && _previousBand != "gen")
            UpdateGenButtonFromCurrentFreq();

        Brush? selBg = Resources["UiButtonSelectedBrush"] as Brush;
        Brush? selBorder = Resources["UiButtonSelectedBorderBrush"] as Brush;

        foreach (var child in panel.Children)
        {
            if (child is not Button btn) continue;

            string? tagStr = btn.Tag?.ToString();
            bool isActive = false;

            if (!string.IsNullOrWhiteSpace(tagStr) && long.TryParse(tagStr, out long freq))
            {
                string bname = GetBandNameForFreq(freq); // e.g. "40m"
                if (btn.Name == "GenBandButton")
                    bname = "gen";
                isActive = bname.Equals(current, StringComparison.OrdinalIgnoreCase);
            }

            // Disabled: clear local brushes so IsEnabled=False gray style shows.
            // Active: darker themed selected face. Inactive: clear so MsccGoldButtonStyle applies.
            if (!btn.IsEnabled)
            {
                btn.ClearValue(Control.BackgroundProperty);
                btn.ClearValue(Control.BorderBrushProperty);
                btn.ClearValue(Control.ForegroundProperty);
                btn.BorderThickness = new Thickness(1);
                btn.FontWeight = FontWeights.Bold;
            }
            else if (isActive && selBg != null && selBorder != null)
            {
                btn.Background = selBg;
                btn.BorderBrush = selBorder;
                btn.BorderThickness = new Thickness(1);
                btn.Foreground = Brushes.White;
                btn.FontWeight = FontWeights.Bold;
            }
            else
            {
                btn.ClearValue(Control.BackgroundProperty);
                btn.ClearValue(Control.BorderBrushProperty);
                btn.ClearValue(Control.ForegroundProperty);
                btn.BorderThickness = new Thickness(1);
                btn.FontWeight = FontWeights.Bold;
            }
        }

        _previousBand = current;
    }

    private static string GetBandNameForFreq(long freq)
    {
        // Keep in sync with MainViewModel.GetBandNameForFrequency
        return MainViewModel.GetBandNameForFrequency(freq);
    }

    private void UpdateGenButtonFromCurrentFreq()
    {
        var btn = this.FindName("GenBandButton") as Button;
        if (btn == null || ViewModel == null) return;

        var opts = GetActiveGenOptions();
        long freq = ViewModel.RadioState.ActiveVfo.FrequencyHz;

        for (int i = 0; i < opts.Length; i++)
        {
            if (opts[i].Freq == freq)
            {
                _genIndex = i;
                btn.Content = opts[i].Label;
                btn.Tag = freq.ToString();
                _currentGenLabel = opts[i].Label;
                ViewModel.CurrentGenSub = _currentGenLabel;
                return;
            }
        }

        // Proficio: unmatched → USER. Geminus: only fixed cal carriers — leave first if no match.
        if (!IsGeminusModelSelected())
        {
            int userIdx = Array.FindIndex(opts, o => o.Label == "USER");
            if (userIdx < 0) userIdx = opts.Length - 1;
            _genIndex = userIdx;
            btn.Content = "USER";
            btn.Tag = freq.ToString();
            _currentGenLabel = "USER";
            ViewModel.CurrentGenSub = _currentGenLabel;
        }
        else
        {
            _genIndex = 0;
            btn.Content = opts[0].Label;
            btn.Tag = opts[0].Freq.ToString();
            _currentGenLabel = opts[0].Label;
            ViewModel.CurrentGenSub = _currentGenLabel;
        }
    }

    // Freq Cal (manual / auto / check)
    private bool _freqCalManualMode = false;
    private bool _freqCalInProgress = false;
    /// <summary>True while AUTO sweep is running (vs CHECK-only).</summary>
    private bool _freqCalIsAuto = false;
    private int _lastCalDelta = 0;

    // Manual PPM: each step does USB EEPROM-style cal + dual LO retune — must rate-limit.
    private const int FreqCalManualPpmMin = -100;
    private const int FreqCalManualPpmMax = 100;
    private const int FreqCalManualPpmMinIntervalMs = 300;
    private int _freqCalManualPpm;
    private int _freqCalManualPpmLastSent = int.MinValue;
    private DateTime _freqCalManualPpmLastSendUtc = DateTime.MinValue;
    private DispatcherTimer? _freqCalManualPpmTimer;

    // GEN band rotation — Proficio: WWV/CHU/RWM/USER; Geminus: LF frequency-cal carriers
    private readonly (string Label, long Freq)[] _genOptionsProficio = new[]
    {
        ("WWV1", 5_000_000L),
        ("WWV2", 10_000_000L),
        ("WWV3", 15_000_000L),
        ("WWV4", 20_000_000L),
        ("CHU1", 3_330_000L),
        ("CHU2", 7_850_000L),
        ("RWM", 9_996_000L),
        ("USER", 10_000_000L),
    };

    /// <summary>Geminus GEN: frequency-calibration reference carriers (kHz labels on the button).</summary>
    private readonly (string Label, long Freq)[] _genOptionsGeminus = new[]
    {
        ("198", 198_000L),
        ("660", 660_000L),
        ("880", 880_000L),
    };

    private int _genIndex = 7;
    private string _currentGenLabel = "USER";
    private string _previousBand = "";

    private void FreqCalManualButton_Click(object sender, RoutedEventArgs e)
    {
        if (ViewModel == null) return;

        if (!_freqCalManualMode)
        {
            // Enter manual mode, like original Set_Calibration
            FreqCalStatusLabel.Text = "MANUAL CALIBRATION";
            FreqCalStatusLabel.Foreground = new SolidColorBrush(Color.FromRgb(0x00, 0xFF, 0xAA));

            ResetFreqCalManualPpmUi(sendToRadio: false);
            SetFreqCalManualPpmButtonsEnabled(true);

            // Disable other buttons during manual
            FreqCalLooseButton.IsEnabled = false;
            FreqCalAutoButton.IsEnabled = false;
            FreqCalCheckButton.IsEnabled = false;
            FreqCalResetButton.IsEnabled = false;

            // Send force calibration (as in original)
            _ = ViewModel.RadioService.SetForceCalibrationAsync(true);

            _freqCalManualMode = true;
            ViewModel?.MonitorTextBoxText(" Freq Cal: Entered MANUAL mode");
        }
        else
        {
            // Flush any pending rate-limited PPM before accept/reject
            FlushFreqCalManualPpmPending(force: true);

            // Exit manual, ask to accept like original
            FreqCalLooseButton.IsEnabled = true;
            FreqCalAutoButton.IsEnabled = true;
            FreqCalCheckButton.IsEnabled = true;
            FreqCalResetButton.IsEnabled = true;
            SetFreqCalManualPpmButtonsEnabled(false);

            FreqCalStatusLabel.Foreground = Brushes.White;

            _ = ViewModel.RadioService.SetForceCalibrationAsync(false);

            // Simple accept dialog
            var result = MessageBox.Show("ACCEPT THIS MANUAL CALIBRATION SETTING?", "MSCC", MessageBoxButton.YesNo, MessageBoxImage.Question);
            if (result == MessageBoxResult.Yes)
            {
                _ = ViewModel.RadioService.SetCalibrationFinishedAsync(true);
                FreqCalStatusLabel.Text = "MANUAL CALIBRATED";
            }
            else
            {
                _ = ViewModel.RadioService.SetCalibrationFinishedAsync(false);
                FreqCalStatusLabel.Text = "NOT CALIBRATED";
            }

            _freqCalManualMode = false;
            ResetFreqCalManualPpmUi(sendToRadio: false);
            ViewModel?.MonitorTextBoxText(" Freq Cal: Exited MANUAL mode");
        }
    }

    private void FreqCalPPMMinus_Click(object sender, RoutedEventArgs e) => StepFreqCalManualPpm(-1);

    private void FreqCalPPMPlus_Click(object sender, RoutedEventArgs e) => StepFreqCalManualPpm(+1);

    private void StepFreqCalManualPpm(int delta)
    {
        if (!_freqCalManualMode || ViewModel == null) return;

        int next = Math.Clamp(_freqCalManualPpm + delta, FreqCalManualPpmMin, FreqCalManualPpmMax);
        if (next == _freqCalManualPpm) return;

        _freqCalManualPpm = next;
        if (FreqCalPPMValue != null)
            FreqCalPPMValue.Text = _freqCalManualPpm.ToString(CultureInfo.InvariantCulture);

        // UI updates immediately; radio is rate-limited (each step = USB cal + dual LO retune).
        ScheduleFreqCalManualPpmSend();
    }

    private void ResetFreqCalManualPpmUi(bool sendToRadio)
    {
        _freqCalManualPpm = 0;
        _freqCalManualPpmLastSent = int.MinValue;
        if (FreqCalPPMValue != null)
            FreqCalPPMValue.Text = "0";
        StopFreqCalManualPpmTimer();
        if (sendToRadio && ViewModel != null)
            _ = ViewModel.RadioService.SetCalSetCoarseAsync(0);
    }

    private void SetFreqCalManualPpmButtonsEnabled(bool enabled)
    {
        if (FreqCalPPMMinus != null) FreqCalPPMMinus.IsEnabled = enabled;
        if (FreqCalPPMPlus != null) FreqCalPPMPlus.IsEnabled = enabled;
    }

    private void ScheduleFreqCalManualPpmSend()
    {
        var elapsed = (DateTime.UtcNow - _freqCalManualPpmLastSendUtc).TotalMilliseconds;
        if (elapsed >= FreqCalManualPpmMinIntervalMs)
        {
            FlushFreqCalManualPpmPending(force: true);
            return;
        }

        // Coalesce rapid clicks: one send of the latest value after the interval.
        if (_freqCalManualPpmTimer == null)
        {
            _freqCalManualPpmTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(50) };
            _freqCalManualPpmTimer.Tick += (_, _) =>
            {
                var wait = (DateTime.UtcNow - _freqCalManualPpmLastSendUtc).TotalMilliseconds;
                if (wait >= FreqCalManualPpmMinIntervalMs)
                    FlushFreqCalManualPpmPending(force: true);
            };
        }
        if (!_freqCalManualPpmTimer.IsEnabled)
            _freqCalManualPpmTimer.Start();
    }

    private void FlushFreqCalManualPpmPending(bool force)
    {
        if (!force && (DateTime.UtcNow - _freqCalManualPpmLastSendUtc).TotalMilliseconds < FreqCalManualPpmMinIntervalMs)
            return;

        StopFreqCalManualPpmTimer();

        if (ViewModel == null || !_freqCalManualMode)
            return;
        if (_freqCalManualPpm == _freqCalManualPpmLastSent)
            return;

        int val = _freqCalManualPpm;
        _freqCalManualPpmLastSent = val;
        _freqCalManualPpmLastSendUtc = DateTime.UtcNow;
        _ = ViewModel.RadioService.SetCalSetCoarseAsync(val);
        ViewModel.MonitorTextBoxText($" Freq Cal: Coarse PPM set to {val}");
    }

    private void StopFreqCalManualPpmTimer()
    {
        if (_freqCalManualPpmTimer != null && _freqCalManualPpmTimer.IsEnabled)
            _freqCalManualPpmTimer.Stop();
    }

    private void FreqCalLooseButton_Click(object sender, RoutedEventArgs e)
    {
        // Placeholder - toggle loose/tight for cal sensitivity
        if (FreqCalLooseButton.Content.ToString() == "LOOSE")
        {
            FreqCalLooseButton.Content = "TIGHT";
            if (ViewModel != null) _ = ViewModel.RadioService.SetCalLooseAsync(false);
        }
        else
        {
            FreqCalLooseButton.Content = "LOOSE";
            if (ViewModel != null) _ = ViewModel.RadioService.SetCalLooseAsync(true);
        }
    }

    private void FreqCalAutoButton_Click(object sender, RoutedEventArgs e)
    {
        if (ViewModel == null) return;
        if (_freqCalManualMode)
        {
            MessageBox.Show("EXIT MANUAL CALIBRATION BEFORE RUNNING AUTO.", "MSCC",
                            MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }
        if (_freqCalInProgress)
        {
            MessageBox.Show("FREQUENCY CALIBRATION IN PROGRESS.", "MSCC",
                            MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }

        // Same wording pattern as original: Yes = COARSE, No = FINE, Cancel = abort
        var calibrationType = MessageBox.Show(
            "COARSE OR FINE CALIBRATION?\r\n\r\n" +
            "YES  = COARSE  (±250 Hz, 5 Hz steps)\r\n" +
            "NO   = FINE    (±50 Hz, 1 Hz steps)\r\n" +
            "CANCEL = do not start",
            "MSCC AUTO FREQUENCY CALIBRATION",
            MessageBoxButton.YesNoCancel,
            MessageBoxImage.Question,
            MessageBoxResult.Cancel);

        if (calibrationType == MessageBoxResult.Cancel)
        {
            ViewModel.MonitorTextBoxText(" Freq Cal: AUTO cancelled");
            return;
        }

        bool coarse = calibrationType == MessageBoxResult.Yes;

        // Force CW filter 200 Hz and pitch 600 Hz (Goertzel listens at ~600 Hz)
        if (ViewModel.CwFilterIndex != 2)
            ViewModel.CwFilterIndex = 2;
        if (ViewModel.CwPitchIndex != 1)
            ViewModel.CwPitchIndex = 1;

        bool loose = FreqCalLooseButton?.Content?.ToString() == "LOOSE";

        int freqHz = 0;
        try
        {
            // Best-effort: original sent Target_Calibration_Frequency; server uses G_tune_freq
            long f = ViewModel.RadioState.ActiveVfo.FrequencyHz;
            if (f > 0 && f <= int.MaxValue)
                freqHz = (int)f;
        }
        catch { /* ignore */ }

        SetFreqCalControlsEnabled(false);
        if (FreqCalProgress != null)
            FreqCalProgress.Value = 0;

        _freqCalInProgress = true;
        _freqCalIsAuto = true;
        _lastCalDelta = 0;
        FreqCalStatusLabel.Text = coarse
            ? "RUNNING COARSE\r\n!WAIT!"
            : "RUNNING FINE\r\n!WAIT!";
        FreqCalStatusLabel.Foreground = Brushes.Black;

        ViewModel.MonitorTextBoxText(
            $" Freq Cal: AUTO starting ({(coarse ? "COARSE" : "FINE")}, loose={loose}, f={freqHz})");

        // Ordered send: loose → clear check-only → mode → START (must not race)
        _ = StartFreqCalAutoAsync(loose, coarse, freqHz);
    }

    private async System.Threading.Tasks.Task StartFreqCalAutoAsync(bool loose, bool coarse, int freqHz)
    {
        if (ViewModel == null) return;
        try
        {
            await ViewModel.RadioService.SetCalLooseAsync(loose).ConfigureAwait(true);
            // Not check-only: finish path must apply PPM, not CHECK-only reporting
            await ViewModel.RadioService.SetCalCheckAsync(false).ConfigureAwait(true);
            await ViewModel.RadioService.SetCalModeAsync(coarse ? 0 : 1).ConfigureAwait(true);
            await ViewModel.RadioService.StartCalibrateAsync(freqHz).ConfigureAwait(true);
            ViewModel.MonitorTextBoxText(" Freq Cal: AUTO start commands sent");
        }
        catch (Exception ex)
        {
            _freqCalInProgress = false;
            _freqCalIsAuto = false;
            SetFreqCalControlsEnabled(true);
            if (FreqCalStatusLabel != null)
            {
                FreqCalStatusLabel.Text = "AUTO FAILED";
                FreqCalStatusLabel.Foreground = Brushes.Red;
            }
            ViewModel.MonitorTextBoxText($" Freq Cal: AUTO start error: {ex.Message}");
        }
    }

    private void FreqCalCheckButton_Click(object sender, RoutedEventArgs e)
    {
        if (ViewModel == null) return;
        if (_freqCalInProgress)
        {
            MessageBox.Show("FREQUENCY CALIBRATION IN PROGRESS.", "MSCC",
                            MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }

        // Force CW filter 200Hz and pitch 600Hz like original (for check)
        if (ViewModel.CwFilterIndex != 2) // 200Hz
        {
            ViewModel.CwFilterIndex = 2;
        }
        if (ViewModel.CwPitchIndex != 1) // 600Hz
        {
            ViewModel.CwPitchIndex = 1;
        }

        SetFreqCalControlsEnabled(false);
        if (FreqCalProgress != null)
            FreqCalProgress.Value = 0;

        _ = ViewModel.RadioService.SetCalCheckAsync(true);

        _freqCalInProgress = true;
        _freqCalIsAuto = false;
        _lastCalDelta = 0;
        FreqCalStatusLabel.Text = "CHECKING\r\n!WAIT!";
        FreqCalStatusLabel.Foreground = Brushes.Black;

        ViewModel?.MonitorTextBoxText(" Freq Cal: CHECK started");
    }

    /// <summary>Disable/enable FREQ CAL action buttons during a running sweep.</summary>
    private void SetFreqCalControlsEnabled(bool enabled)
    {
        if (FreqCalResetButton != null) FreqCalResetButton.IsEnabled = enabled;
        if (FreqCalLooseButton != null) FreqCalLooseButton.IsEnabled = enabled;
        if (FreqCalAutoButton != null) FreqCalAutoButton.IsEnabled = enabled;
        if (FreqCalCheckButton != null) FreqCalCheckButton.IsEnabled = enabled;
        if (FreqCalManualButton != null) FreqCalManualButton.IsEnabled = enabled;
        // PPM ± only when manual mode and not in an auto/check sweep
        SetFreqCalManualPpmButtonsEnabled(enabled && _freqCalManualMode);
    }

    private void FreqCalResetButton_Click(object sender, RoutedEventArgs e)
    {
        if (ViewModel != null)
        {
            var res = MessageBox.Show("RESET CALIBRATION?", "MSCC", MessageBoxButton.YesNo);
            if (res == MessageBoxResult.Yes)
            {
                _ = ViewModel.RadioService.SetCalResetAsync(true);
                FreqCalStatusLabel.Text = "RESET";
                ViewModel?.MonitorTextBoxText(" Freq Cal: RESET clicked");
            }
        }
    }

    // --- Freq Cal CHECK progress/status handlers (completes the Check button impl) ---

    private void OnCalProgressReported(int value)
    {
        if (!Dispatcher.CheckAccess())
        {
            Dispatcher.BeginInvoke(() => OnCalProgressReported(value));
            return;
        }
        if (FreqCalProgress != null)
        {
            FreqCalProgress.Value = Math.Clamp(value, 0, 100);
        }
    }

    private void OnCalStatusReported(int value)
    {
        if (!Dispatcher.CheckAccess())
        {
            Dispatcher.BeginInvoke(() => OnCalStatusReported(value));
            return;
        }

        bool wasInProgress = _freqCalInProgress;
        bool wasAuto = _freqCalIsAuto;
        _freqCalInProgress = false;
        _freqCalIsAuto = false;

        SetFreqCalControlsEnabled(true);

        if (wasInProgress)
        {
            string statusText;
            if (wasAuto)
                statusText = (value == 1) ? "AUTO COMPLETED" : "AUTO FAILED";
            else
                statusText = (value == 1) ? "CHECK COMPLETED" : "CHECK FAILED";

            var statusColor = (value == 1) ? Brushes.Green : Brushes.Red;

            if (_lastCalDelta != 0 && Math.Abs(_lastCalDelta) < 10000)
            {
                statusText += $"\r\n{_lastCalDelta} Hz";
                _lastCalDelta = 0;
            }

            if (FreqCalStatusLabel != null)
            {
                FreqCalStatusLabel.Text = statusText;
                FreqCalStatusLabel.Foreground = statusColor;
            }
            ViewModel?.MonitorTextBoxText(
                $" Freq Cal: {(wasAuto ? "AUTO" : "CHECK")} status received: {value} (1=COMPLETED)");
        }
        else
        {
            ViewModel?.MonitorTextBoxText($" Freq Cal: status received: {value}");
        }
    }

    private void OnCalDeltaReported(int value)
    {
        _lastCalDelta = value;

        if (!Dispatcher.CheckAccess())
        {
            Dispatcher.BeginInvoke(() => OnCalDeltaReported(value));
            return;
        }

        // If status is already shown, append the delta (handles report ordering)
        if (FreqCalStatusLabel != null &&
            (FreqCalStatusLabel.Text == "CHECK COMPLETED" || FreqCalStatusLabel.Text == "CHECK FAILED" ||
             FreqCalStatusLabel.Text == "AUTO COMPLETED" || FreqCalStatusLabel.Text == "AUTO FAILED") &&
            Math.Abs(value) < 10000 &&
            !FreqCalStatusLabel.Text.Contains("Hz"))
        {
            FreqCalStatusLabel.Text += $"\r\n{value} Hz";
            _lastCalDelta = 0;
        }

        ViewModel?.MonitorTextBoxText($" Freq Cal: Delta received: {value} Hz");
    }
}
