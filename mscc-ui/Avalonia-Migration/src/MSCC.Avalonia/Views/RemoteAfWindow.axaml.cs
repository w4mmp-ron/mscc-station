using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.Interactivity;
using MSCC.Avalonia.RemoteAudio;
using MSCC.Avalonia.ViewModels;

namespace MSCC.Avalonia.Views;

public partial class RemoteAfWindow : Window
{
    private MainViewModel? _vm;
    private bool _ready;

    public RemoteAfWindow()
    {
        InitializeComponent();
        Opened += OnOpened;
        Closed += OnClosed;
    }

    private void OnOpened(object? sender, EventArgs e)
    {
        _vm = DataContext as MainViewModel;
        if (_vm == null)
        {
            LogBox.Text = "No ViewModel.";
            return;
        }

        PlayDeviceCombo.Items.Clear();
        foreach (var d in RemoteAfEngine.PlayDevices)
            PlayDeviceCombo.Items.Add(new ComboBoxItem { Content = d.Name, Tag = d.Index });
        MicDeviceCombo.Items.Clear();
        foreach (var d in RemoteAfEngine.MicDevices)
            MicDeviceCombo.Items.Add(new ComboBoxItem { Content = d.Name, Tag = d.Index });

        PlayVolumeSlider.Value = _vm.RemotePlayVolume;
        MicVolumeSlider.Value = _vm.RemoteMicVolume;
        MuteCheck.IsChecked = _vm.RemotePlayMute;
        EqEnableCheck.IsChecked = _vm.RemoteEqEnabled;
        EqLowSlider.Value = _vm.RemoteEqLowDb;
        EqMidSlider.Value = _vm.RemoteEqMidDb;
        EqHighSlider.Value = _vm.RemoteEqHighDb;

        _vm.RemoteAfLog += AppendLog;
        RefreshPath();
        _ready = true;
        ApplyEq();
        StatusText.Text = _vm.RemoteAf?.Status ?? "—";
    }

    private void OnClosed(object? sender, EventArgs e)
    {
        if (_vm != null)
            _vm.RemoteAfLog -= AppendLog;
        _vm = null;
    }

    public void RefreshPath()
    {
        if (_vm == null) return;
        bool digi = _vm.IsDigitalAudio;
        Title = digi ? "Remote Digital" : "Remote Phones";
        TitleBlock.Text = digi ? "REMOTE DIGITAL" : "REMOTE PHONES";
        RxHeading.Text = digi ? "DIGITAL RX (VAC)" : "PHONES RX";
        MicHeading.Text = digi ? "DIGITAL MIC TX (VAC)" : "EQ / MIC TX";
        VacHint.Text = digi
            ? "WSJT-X should use the same Digital Speaker / Mic as mscc-init (VirtualA / VirtualB)."
            : "";
        VacHint.IsVisible = digi;
        string catPort = _vm.RemoteCat?.PortName ?? KenwoodCatPort.ResolveLinuxCatPath() ?? "(none)";
        bool catOpen = _vm.RemoteCat?.IsOpen == true;
        CatHint.Text = catOpen
            ? $"CAT: TS-2000 on {catPort}. Freq/mode/PTT → radio via 8888."
            : $"CAT: not open ({catPort}). tty0tty busy if local ms-sdr holds it.";
        EqPanel.IsVisible = !digi;
        MuteCheck.Content = digi ? "Mute VAC play" : "Mute phones";

        bool was = _ready;
        _ready = false;
        if (digi)
        {
            int play = MainViewModel.FindNamedAfDevice(RemoteAfEngine.PlayDevices, LinuxDigitalIni.DigitalSpeaker);
            int mic = MainViewModel.FindNamedAfDevice(RemoteAfEngine.MicDevices, LinuxDigitalIni.DigitalMic);
            SelectByTag(PlayDeviceCombo, play);
            SelectByTag(MicDeviceCombo, mic);
        }
        else
        {
            SelectByTag(PlayDeviceCombo, _vm.RemotePlayDeviceIndex);
            SelectByTag(MicDeviceCombo, _vm.RemoteMicDeviceIndex);
        }
        _ready = was;
    }

    private static void SelectByTag(ComboBox box, int tag)
    {
        for (int i = 0; i < box.Items.Count; i++)
        {
            if (box.Items[i] is ComboBoxItem it && it.Tag is int t && t == tag)
            {
                box.SelectedIndex = i;
                return;
            }
        }
        if (box.Items.Count > 0)
            box.SelectedIndex = 0;
    }

    private void AppendLog(string msg)
    {
        global::Avalonia.Threading.Dispatcher.UIThread.Post(() =>
        {
            LogBox.Text += msg + "\n";
            if (LogBox.Text.Length > 8000)
                LogBox.Text = LogBox.Text[^4000..];
        });
    }

    private void PlayDevice_Changed(object? sender, SelectionChangedEventArgs e)
    {
        if (!_ready || _vm == null) return;
        if (PlayDeviceCombo.SelectedItem is ComboBoxItem { Tag: int idx })
        {
            _vm.RemotePlayDeviceIndex = idx;
            _vm.ApplyRemoteAfDevicesAndRestart("play device");
        }
    }

    private void MicDevice_Changed(object? sender, SelectionChangedEventArgs e)
    {
        if (!_ready || _vm == null) return;
        if (MicDeviceCombo.SelectedItem is ComboBoxItem { Tag: int idx })
        {
            _vm.RemoteMicDeviceIndex = idx;
            _vm.ApplyRemoteAfDevicesAndRestart("mic device");
        }
    }

    private void PlayVolume_Changed(object? sender, RangeBaseValueChangedEventArgs e)
    {
        if (!_ready || _vm == null) return;
        int v = (int)Math.Round(e.NewValue);
        _vm.RemotePlayVolume = v;
        PlayVolumeLabel.Text = v.ToString();
        if (_vm.RemoteAf != null)
            _vm.RemoteAf.PlayVolume = v / 100f;
    }

    private void MicVolume_Changed(object? sender, RangeBaseValueChangedEventArgs e)
    {
        if (!_ready || _vm == null) return;
        int v = (int)Math.Round(e.NewValue);
        _vm.RemoteMicVolume = v;
        MicVolumeLabel.Text = v.ToString();
        if (_vm.RemoteAf != null)
            _vm.RemoteAf.MicVolume = v / 100f;
    }

    private void Mute_Changed(object? sender, RoutedEventArgs e)
    {
        if (!_ready || _vm == null) return;
        bool m = MuteCheck.IsChecked == true;
        _vm.RemotePlayMute = m;
        if (_vm.RemoteAf != null)
            _vm.RemoteAf.PlayMuted = m;
    }

    private void Eq_Changed(object? sender, RoutedEventArgs e) => ApplyEq();
    private void EqSlider_Changed(object? sender, RangeBaseValueChangedEventArgs e) => ApplyEq();

    private void ApplyEq()
    {
        if (!_ready || _vm == null) return;
        _vm.RemoteEqEnabled = EqEnableCheck.IsChecked == true;
        _vm.RemoteEqLowDb = (float)EqLowSlider.Value;
        _vm.RemoteEqMidDb = (float)EqMidSlider.Value;
        _vm.RemoteEqHighDb = (float)EqHighSlider.Value;
        _vm.RemoteAf?.ApplyEq(_vm.RemoteEqEnabled, _vm.RemoteEqLowDb, _vm.RemoteEqMidDb, _vm.RemoteEqHighDb);
    }

    private void ResetEq_Click(object? sender, RoutedEventArgs e)
    {
        EqEnableCheck.IsChecked = false;
        EqLowSlider.Value = 0;
        EqMidSlider.Value = 0;
        EqHighSlider.Value = 0;
        ApplyEq();
    }
}
