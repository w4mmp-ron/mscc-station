using System;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

namespace MSCC.Wpf.Controls;

/// <summary>
/// Settings → COM Port: enumerate serial ports and write ms-sdr comm-port.ini.
/// </summary>
public partial class CommPortSettingsPanel : UserControl
{
    private bool _suppressSave;
    private CommPortConfig.Settings _settings = new();

    public CommPortSettingsPanel()
    {
        InitializeComponent();
        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        _suppressSave = true;
        try
        {
            if (BaudCombo != null && BaudCombo.Items.Count == 0)
            {
                foreach (int b in CommPortConfig.BaudRates)
                    BaudCombo.Items.Add(b.ToString());
            }

            _settings = CommPortConfig.Load();
            RefreshPortList(selectPort: _settings.PortName);
            if (BaudCombo != null)
                BaudCombo.SelectedIndex = Math.Clamp(_settings.BaudRateIndex, 0, CommPortConfig.BaudRates.Length - 1);
            if (PinCheck != null)
                PinCheck.IsChecked = _settings.Pin != 0;

            SetStatus($"Loaded {CommPortConfig.ConfigPath}");
        }
        finally
        {
            _suppressSave = false;
        }
    }

    private void OnRefreshClick(object sender, RoutedEventArgs e)
    {
        string? keep = PortCombo?.SelectedItem as string ?? _settings.PortName;
        RefreshPortList(selectPort: keep);
        SetStatus($"Found {(PortCombo?.Items.Count ?? 0)} port(s).");
    }

    private void OnApplyClick(object sender, RoutedEventArgs e)
    {
        if (SaveNow())
        {
            SetStatus($"Saved {_settings.PortName} @ {CommPortConfig.BaudRates[_settings.BaudRateIndex]}. Press Stop then Start to reload COM.");
            NotifyMainSetupStatus();
        }
        else
            SetStatus("Save failed — check folder permissions for %LocalAppData%\\MSCC-NET9\\");
    }

    private void OnPortSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_suppressSave || PortCombo?.SelectedItem is not string port)
            return;
        _settings.PortName = CommPortConfig.NormalizePortName(port);
        // Auto-save on change (user can also hit Apply)
        SaveNow();
        SetStatus($"Selected {_settings.PortName} (saved).");
    }

    private void OnBaudSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_suppressSave || BaudCombo == null || BaudCombo.SelectedIndex < 0)
            return;
        _settings.BaudRateIndex = BaudCombo.SelectedIndex;
        SaveNow();
    }

    private void OnPinChanged(object sender, RoutedEventArgs e)
    {
        if (_suppressSave || PinCheck == null)
            return;
        _settings.Pin = PinCheck.IsChecked == true ? 1 : 0;
        SaveNow();
    }

    private void RefreshPortList(string? selectPort)
    {
        if (PortCombo == null) return;

        _suppressSave = true;
        try
        {
            var ports = CommPortConfig.GetAvailablePorts().ToList();
            string want = CommPortConfig.NormalizePortName(selectPort ?? "");

            // Keep configured port visible even if not currently present (USB unplugged)
            if (!string.IsNullOrEmpty(want) &&
                !ports.Contains(want, StringComparer.OrdinalIgnoreCase) &&
                want.StartsWith("COM", StringComparison.OrdinalIgnoreCase))
            {
                ports.Add(want + " (not detected)");
            }

            PortCombo.Items.Clear();
            foreach (string p in ports)
                PortCombo.Items.Add(p.Contains("(not detected)", StringComparison.Ordinal)
                    ? p
                    : p);

            // Select matching port
            int sel = -1;
            for (int i = 0; i < PortCombo.Items.Count; i++)
            {
                string item = PortCombo.Items[i]?.ToString() ?? "";
                string baseName = item.Replace(" (not detected)", "", StringComparison.OrdinalIgnoreCase).Trim();
                if (string.Equals(baseName, want, StringComparison.OrdinalIgnoreCase))
                {
                    sel = i;
                    break;
                }
            }

            if (sel < 0 && PortCombo.Items.Count > 0)
                sel = 0;
            PortCombo.SelectedIndex = sel;

            if (PortCombo.SelectedItem is string chosen)
            {
                _settings.PortName = CommPortConfig.NormalizePortName(
                    chosen.Replace(" (not detected)", "", StringComparison.OrdinalIgnoreCase).Trim());
            }
        }
        finally
        {
            _suppressSave = false;
        }
    }

    private bool SaveNow()
    {
        if (PortCombo?.SelectedItem is string chosen)
        {
            _settings.PortName = CommPortConfig.NormalizePortName(
                chosen.Replace(" (not detected)", "", StringComparison.OrdinalIgnoreCase).Trim());
        }
        if (BaudCombo != null && BaudCombo.SelectedIndex >= 0)
            _settings.BaudRateIndex = BaudCombo.SelectedIndex;
        if (PinCheck != null)
            _settings.Pin = PinCheck.IsChecked == true ? 1 : 0;

        bool ok = CommPortConfig.Save(_settings);
        if (ok)
            NotifyMainSetupStatus();
        return ok;
    }

    private static void NotifyMainSetupStatus()
    {
        try
        {
            if (Application.Current?.MainWindow is MainWindow mw)
                mw.ViewModel?.RefreshSetupStatus();
        }
        catch { /* ignore */ }
    }

    private void SetStatus(string msg)
    {
        if (StatusText != null)
            StatusText.Text = msg;
    }
}
