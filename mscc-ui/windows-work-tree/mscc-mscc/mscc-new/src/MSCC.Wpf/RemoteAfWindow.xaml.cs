using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using MSCC.Wpf.ViewModels;

namespace MSCC.Wpf;

public partial class RemoteAfWindow : Window
{
    private static double? s_left, s_top, s_width, s_height;
    private MainViewModel? _vm;
    private bool _ready;

    public RemoteAfWindow()
    {
        InitializeComponent();
        SourceInitialized += OnSourceInitialized;
        Loaded += OnLoaded;
        Closed += OnClosed;
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
        CopyChromeFromOwner();
    }

    private void CopyChromeFromOwner()
    {
        if (Owner is not MainWindow mw)
            return;
        string[] keys =
        {
            "UiWindowBackgroundBrush", "UiPanelBackgroundBrush", "UiPrimaryTextBrush",
            "UiMutedTextBrush", "UiButtonFaceBrush", "UiButtonBorderBrush",
            "UiButtonHoverBrush", "UiButtonHoverBorderBrush", "UiButtonPressedBrush",
            "UiButtonTextBrush",
        };
        foreach (string key in keys)
        {
            if (mw.Resources[key] is SolidColorBrush b)
                Resources[key] = b;
        }
        if (Resources["UiWindowBackgroundBrush"] is SolidColorBrush win)
            Background = win;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        try
        {
            _vm = DataContext as MainViewModel;
            if (_vm == null)
            {
                LogBox.Text = "No ViewModel — popup still opened.";
                return;
            }

            PlayDeviceCombo.Items.Clear();
            foreach (var d in RemoteAudio.RemoteAfEngine.PlayDevices)
                PlayDeviceCombo.Items.Add(new ComboBoxItem { Content = d.Name, Tag = d.Index });
            MicDeviceCombo.Items.Clear();
            foreach (var d in RemoteAudio.RemoteAfEngine.MicDevices)
                MicDeviceCombo.Items.Add(new ComboBoxItem { Content = d.Name, Tag = d.Index });

            PlayVolumeSlider.Value = SpectrumWaterfallSettings.RemotePlayVolume;
            MicVolumeSlider.Value = SpectrumWaterfallSettings.RemoteMicVolume;
            MuteCheck.IsChecked = SpectrumWaterfallSettings.RemotePlayMute;
            EqEnableCheck.IsChecked = SpectrumWaterfallSettings.RemoteEqEnabled;
            EqLowSlider.Value = SpectrumWaterfallSettings.RemoteEqLowDb;
            EqMidSlider.Value = SpectrumWaterfallSettings.RemoteEqMidDb;
            EqHighSlider.Value = SpectrumWaterfallSettings.RemoteEqHighDb;

            _vm.RemoteAfLog += AppendLog;
            RefreshPath();
            _ready = true;
            ApplyEq();
            StatusText.Text = _vm.RemoteAf?.Status ?? "—";
        }
        catch (Exception ex)
        {
            _ready = true;
            LogBox.Text = "Popup load error: " + ex.Message;
        }
    }

    private void OnClosed(object? sender, EventArgs e)
    {
        if (WindowState == WindowState.Normal)
        {
            s_left = Left;
            s_top = Top;
            s_width = Width;
            s_height = Height;
        }
        if (_vm != null)
            _vm.RemoteAfLog -= AppendLog;
        _vm = null;
    }

    /// <summary>R-Phones vs R-Digital chrome: VAC devices, hide EQ.</summary>
    public void RefreshPath()
    {
        if (_vm == null) return;
        bool digi = _vm.IsDigitalAudio;
        Title = digi ? "Remote Digital" : "Remote Phones";
        TitleBlock.Text = digi ? "REMOTE DIGITAL" : "REMOTE PHONES";
        RxHeading.Text = digi ? "DIGITAL RX (VAC)" : "PHONES RX";
        MicHeading.Text = digi ? "DIGITAL MIC TX (VAC)" : "EQ / MIC TX";
        VacHint.Text = digi
            ? "WSJT-X should use the same Digital Speaker / Digital Mic as Settings (CABLE / VB-Audio)."
            : "";
        VacHint.Visibility = digi ? Visibility.Visible : Visibility.Collapsed;
        string catPort = _vm.RemoteCat?.PortName ?? CommPortConfig.Load().PortName;
        bool catOpen = _vm.RemoteCat?.IsOpen == true;
        CatHint.Text = catOpen
            ? $"CAT: TS-2000 on {catPort} (same WSJT-X serial as local). Freq/mode/PTT → radio via 8888."
            : $"CAT: not open ({catPort}). Check Settings COM (ms-sdr side of the pair) and that local ms-sdr is not holding it.";
        EqPanel.Visibility = digi ? Visibility.Collapsed : Visibility.Visible;
        MuteCheck.Content = digi ? "Mute VAC play" : "Mute phones";

        bool wasReady = _ready;
        _ready = false;
        if (digi)
        {
            var s = AudioDeviceConfig.Load();
            int play = MainViewModel.FindNamedAfDevice(RemoteAudio.RemoteAfEngine.PlayDevices, s.DigitalSpeaker);
            int mic = MainViewModel.FindNamedAfDevice(RemoteAudio.RemoteAfEngine.MicDevices, s.DigitalMic);
            SelectByTag(PlayDeviceCombo, play);
            SelectByTag(MicDeviceCombo, mic);
        }
        else
        {
            SelectByTag(PlayDeviceCombo, SpectrumWaterfallSettings.RemotePlayDeviceIndex);
            SelectByTag(MicDeviceCombo, SpectrumWaterfallSettings.RemoteMicDeviceIndex);
        }
        _ready = wasReady;
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

    private void AppendLog(string line)
    {
        Dispatcher.BeginInvoke(() =>
        {
            LogBox.AppendText(line + Environment.NewLine);
            LogBox.ScrollToEnd();
            if (_vm?.RemoteAf != null)
                StatusText.Text = _vm.RemoteAf.Status;
        });
    }

    private void PlayDevice_Changed(object sender, SelectionChangedEventArgs e)
    {
        if (!_ready || _vm == null) return;
        if (PlayDeviceCombo.SelectedItem is ComboBoxItem it && it.Tag is int idx)
        {
            if (_vm.RemoteAf != null)
                _vm.RemoteAf.PlayDeviceIndex = idx;
            if (!_vm.IsDigitalAudio)
            {
                SpectrumWaterfallSettings.RemotePlayDeviceIndex = idx;
                SaveSettings();
            }
            _vm.RestartRemoteAfRx();
        }
    }

    private void MicDevice_Changed(object sender, SelectionChangedEventArgs e)
    {
        if (!_ready || _vm == null) return;
        if (MicDeviceCombo.SelectedItem is ComboBoxItem it && it.Tag is int idx)
        {
            if (_vm.RemoteAf != null)
                _vm.RemoteAf.MicDeviceIndex = idx;
            if (!_vm.IsDigitalAudio)
            {
                SpectrumWaterfallSettings.RemoteMicDeviceIndex = idx;
                SaveSettings();
            }
            _vm.RestartRemoteAfMic();
        }
    }

    private void PlayVolume_Changed(object sender, RoutedPropertyChangedEventArgs<double> e)
    {
        // ValueChanged also fires during InitializeComponent, before labels exist.
        if (PlayVolumeLabel != null)
            PlayVolumeLabel.Text = ((int)PlayVolumeSlider.Value).ToString();
        if (!_ready || _vm?.RemoteAf == null) return;
        int v = (int)PlayVolumeSlider.Value;
        _vm.RemoteAf.PlayVolume = v / 100f;
        SpectrumWaterfallSettings.RemotePlayVolume = v;
        SaveSettings();
    }

    private void MicVolume_Changed(object sender, RoutedPropertyChangedEventArgs<double> e)
    {
        if (MicVolumeLabel != null)
            MicVolumeLabel.Text = ((int)MicVolumeSlider.Value).ToString();
        if (!_ready || _vm?.RemoteAf == null) return;
        int v = (int)MicVolumeSlider.Value;
        _vm.RemoteAf.MicVolume = v / 100f;
        SpectrumWaterfallSettings.RemoteMicVolume = v;
        SaveSettings();
    }

    private void Mute_Changed(object sender, RoutedEventArgs e)
    {
        if (!_ready || _vm?.RemoteAf == null) return;
        bool mute = MuteCheck.IsChecked == true;
        _vm.RemoteAf.PlayMuted = mute;
        SpectrumWaterfallSettings.RemotePlayMute = mute;
        SaveSettings();
    }

    private void Eq_Changed(object sender, RoutedEventArgs e) => ApplyEq();

    private void EqSlider_Changed(object sender, RoutedPropertyChangedEventArgs<double> e) => ApplyEq();

    private void ResetEq_Click(object sender, RoutedEventArgs e)
    {
        EqEnableCheck.IsChecked = false;
        EqLowSlider.Value = 0;
        EqMidSlider.Value = 0;
        EqHighSlider.Value = 0;
        ApplyEq();
    }

    private void ApplyEq()
    {
        if (!_ready || _vm?.RemoteAf == null) return;
        bool on = EqEnableCheck.IsChecked == true;
        float lo = (float)EqLowSlider.Value;
        float mid = (float)EqMidSlider.Value;
        float hi = (float)EqHighSlider.Value;
        _vm.RemoteAf.ApplyEq(on, lo, mid, hi);
        SpectrumWaterfallSettings.RemoteEqEnabled = on;
        SpectrumWaterfallSettings.RemoteEqLowDb = lo;
        SpectrumWaterfallSettings.RemoteEqMidDb = mid;
        SpectrumWaterfallSettings.RemoteEqHighDb = hi;
        SaveSettings();
    }

    private static void SaveSettings()
    {
        try { SpectrumWaterfallSettings.Save(); } catch { /* best-effort */ }
    }
}
