using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;

namespace MSCC.Wpf.Controls;

/// <summary>
/// Settings → Audio: operator + digital device pairs → Multus audio INI files.
/// </summary>
public partial class AudioSettingsPanel : UserControl
{
    private bool _suppress;
    private IReadOnlyList<AudioDeviceConfig.DeviceChoice> _outputs = Array.Empty<AudioDeviceConfig.DeviceChoice>();
    private IReadOnlyList<AudioDeviceConfig.DeviceChoice> _inputs = Array.Empty<AudioDeviceConfig.DeviceChoice>();

    public AudioSettingsPanel()
    {
        InitializeComponent();
        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        ReloadAll();
    }

    private void OnRefreshClick(object sender, RoutedEventArgs e)
    {
        ReloadAll();
        SetStatus($"Refreshed: {_outputs.Count} output(s), {_inputs.Count} input(s).");
    }

    private void OnApplyClick(object sender, RoutedEventArgs e)
    {
        if (SaveFromUi())
        {
            SetStatus("Saved operator + digital audio INIs. Press Stop then Start to apply.");
            NotifyMainSetupStatus();
        }
        else
            SetStatus("Save failed — check %LocalAppData%\\MSCC-NET9\\ permissions.");
    }

    private void OnSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_suppress) return;
        // Only auto-save when the changed combo has a real selection (blank stays blank until chosen).
        if (sender is ComboBox cb && cb.SelectedIndex < 0)
            return;
        if (SaveFromUi())
        {
            SetStatus("Saved audio settings. Press Stop then Start to apply.");
            NotifyMainSetupStatus();
        }
    }

    private void ReloadAll()
    {
        _suppress = true;
        try
        {
            _outputs = AudioDeviceConfig.GetOutputDevices();
            _inputs = AudioDeviceConfig.GetInputDevices();
            var saved = AudioDeviceConfig.Load();

            FillCombo(OpSpeakerCombo, _outputs, saved.OperatorSpeaker);
            FillCombo(OpMicCombo, _inputs, saved.OperatorMic);
            FillCombo(DigSpeakerCombo, _outputs, saved.DigitalSpeaker);
            FillCombo(DigMicCombo, _inputs, saved.DigitalMic);

            int unset = CountUnsetRequired();
            if (unset > 0)
            {
                SetStatus(
                    $"Select operator Out/Mic (and digital if used), then Apply — {unset} required field(s) blank. " +
                    $"({_outputs.Count} out / {_inputs.Count} in listed)");
            }
            else
            {
                SetStatus($"Loaded from {AudioDeviceConfig.ConfigDirectory} ({_outputs.Count} out / {_inputs.Count} in)");
            }
        }
        finally
        {
            _suppress = false;
        }
    }

    /// <summary>
    /// Fill devices; leave SelectedIndex = -1 when no saved match so first-run does not look "set".
    /// </summary>
    private static void FillCombo(ComboBox? combo, IReadOnlyList<AudioDeviceConfig.DeviceChoice> devices, string savedKey)
    {
        if (combo == null) return;
        combo.Items.Clear();
        foreach (var d in devices)
            combo.Items.Add(d);

        int idx = AudioDeviceConfig.FindBestIndex(devices, savedKey);
        combo.SelectedIndex = (idx >= 0 && idx < combo.Items.Count) ? idx : -1;
    }

    private int CountUnsetRequired()
    {
        int n = 0;
        if (OpSpeakerCombo?.SelectedIndex < 0) n++;
        if (OpMicCombo?.SelectedIndex < 0) n++;
        return n;
    }

    private bool SaveFromUi()
    {
        var s = new AudioDeviceConfig.Settings
        {
            OperatorSpeaker = KeyFromCombo(OpSpeakerCombo),
            OperatorMic = KeyFromCombo(OpMicCombo),
            DigitalSpeaker = KeyFromCombo(DigSpeakerCombo),
            DigitalMic = KeyFromCombo(DigMicCombo),
        };
        return AudioDeviceConfig.Save(s);
    }

    private static string KeyFromCombo(ComboBox? combo)
    {
        if (combo?.SelectedItem is AudioDeviceConfig.DeviceChoice d)
            return d.MatchKey;
        return "";
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
