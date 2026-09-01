using System;
using System.Windows;
using System.Windows.Controls;

namespace MSCC.Wpf.Controls;

/// <summary>Settings → external WiFi SWR meter (UDP listen + HF/LF IP profiles).</summary>
public partial class SwrMeterSettingsPanel : UserControl
{
    private bool _suppress;

    public SwrMeterSettingsPanel()
    {
        InitializeComponent();
        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        _suppress = true;
        try
        {
            SwrMeterSettings.Load();
            if (EnableCheck != null)
                EnableCheck.IsChecked = SwrMeterSettings.Enabled;
            if (PortBox != null)
                PortBox.Text = SwrMeterSettings.UdpListenPort.ToString();
            if (HfIpBox != null)
                HfIpBox.Text = SwrMeterSettings.HfMeterIp ?? "";
            if (LfIpBox != null)
                LfIpBox.Text = SwrMeterSettings.LfMeterIp ?? "";
            SetStatus(SwrMeterSettings.Enabled
                ? $"Enabled — UDP port {SwrMeterSettings.UdpListenPort}"
                : "Disabled");
        }
        finally
        {
            _suppress = false;
        }

        // Push current UI into host so listener starts if already enabled
        RequestApplyToHost();
    }

    private void OnEnableChanged(object sender, RoutedEventArgs e)
    {
        if (_suppress) return;
        SwrMeterSettings.Enabled = EnableCheck?.IsChecked == true;
        SwrMeterSettings.Save();
        RequestApplyToHost();
        SetStatus(SwrMeterSettings.Enabled ? "Enabled (saved)" : "Disabled (saved)");
    }

    private void OnPortLostFocus(object sender, RoutedEventArgs e) => SavePortFromUi(apply: false);

    private void OnApplyListenClick(object sender, RoutedEventArgs e)
    {
        SavePortFromUi(apply: true);
        SaveIpsFromUi();
        RequestApplyToHost();
        SetStatus($"Listen apply — port {SwrMeterSettings.UdpListenPort}, enabled={SwrMeterSettings.Enabled}");
    }

    private void OnIpLostFocus(object sender, RoutedEventArgs e)
    {
        if (_suppress) return;
        SaveIpsFromUi();
    }

    private void SavePortFromUi(bool apply)
    {
        if (PortBox == null) return;
        if (int.TryParse(PortBox.Text.Trim(), out int p) && p is >= 1 and <= 65535)
            SwrMeterSettings.UdpListenPort = p;
        else
            PortBox.Text = SwrMeterSettings.UdpListenPort.ToString();
        SwrMeterSettings.Save();
        if (apply)
            RequestApplyToHost();
    }

    private void SaveIpsFromUi()
    {
        SwrMeterSettings.HfMeterIp = HfIpBox?.Text?.Trim() ?? "";
        SwrMeterSettings.LfMeterIp = LfIpBox?.Text?.Trim() ?? "";
        SwrMeterSettings.Save();
    }

    /// <summary>MainWindow / ViewModel should start/stop UDP listener.</summary>
    public event Action? ApplyRequested;

    private void RequestApplyToHost() => ApplyRequested?.Invoke();

    public void SetStatus(string msg)
    {
        if (StatusText != null)
            StatusText.Text = msg;
    }

    public void SetAutoIp(string ip, bool geminus)
    {
        if (string.IsNullOrWhiteSpace(ip)) return;
        _suppress = true;
        try
        {
            if (geminus)
            {
                if (string.IsNullOrWhiteSpace(SwrMeterSettings.LfMeterIp) || SwrMeterSettings.LfMeterIp == ip)
                {
                    SwrMeterSettings.LfMeterIp = ip;
                    if (LfIpBox != null) LfIpBox.Text = ip;
                }
            }
            else
            {
                if (string.IsNullOrWhiteSpace(SwrMeterSettings.HfMeterIp) || SwrMeterSettings.HfMeterIp == ip)
                {
                    SwrMeterSettings.HfMeterIp = ip;
                    if (HfIpBox != null) HfIpBox.Text = ip;
                }
            }
            SwrMeterSettings.Save();
        }
        finally
        {
            _suppress = false;
        }
    }
}
