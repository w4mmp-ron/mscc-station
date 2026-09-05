using System.Collections.ObjectModel;
using System.Globalization;
using Avalonia.Threading;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using MSCC.Avalonia.Controls;
using MSCC.Avalonia.Models;
using MSCC.Avalonia.Services;
using MSCC.Core.Display;
using MSCC.Core.Logging;
using MSCC.Core.Protocol;
using MSCC.Core.Services;

namespace MSCC.Avalonia.ViewModels;

/// <summary>
/// Avalonia MSCC client — Windows-like shell.
/// Operate essentials: connect, spectrum, band/mode, power, PTT/TUN, AGC, AMP, CMP, MON, NB/NR/AN, filters.
/// </summary>
public partial class MainViewModel : ViewModelBase, IDisposable
{
    private static readonly long[] StepChoicesHz = { 10, 100, 1_000, 10_000, 100_000 };
    // Match WPF / Core index tables (button labels are short; list options match WPF wording)
    private static readonly string[] LowCutLabels = { "500", "300", "200", "100", "75" };
    private static readonly int[] LowCutHzValues = { 500, 300, 200, 100, 75 };
    private static readonly string[] HighCutLabels = { "5.5k", "4.0k", "3.0k", "2.7k", "2.4k" };
    private static readonly int[] HighCutHzValues = { 5500, 4000, 3000, 2700, 2400 };
    private static readonly string[] CwFilterLabels = { "1.8k", "400", "200" };
    private static readonly int[] CwFilterHzValues = { 1800, 400, 200 };
    private static readonly int[] CwWeightValues = { 25, 50, 75 };
    private static readonly int[] CwPitchValues = { 400, 600, 800, 1000 };

    private UdpRadioService? _radio;
    private bool _disposed;
    private int _packetsReceived;
    private int _panPacketsReceived;
    private int _keepAlivesReceived;
    private int _spectrumFrames;
    private int _spectrumFrameCounter;
    private long _frequencyHz = 7_000_000;
    private long _vfoBFrequencyHz = 14_200_000;
    private int _stepIndex = 2;
    private int _lowCutIndex;
    private int _highCutIndex = 2;
    private int _cwFilterIndex;
    private bool _suppressPowerSend;
    private bool _syncingRfMirror;
    private bool _suppressTransmitCommands;
    /// <summary>True when keyer memory Play asserted PTT and should auto-release it.</summary>
    private bool _keyerPlayOwnsPtt;
    private CancellationTokenSource? _keyerPlayPttReleaseCts;
    private bool _suppressAmpCommand;
    private bool _suppressNbCommand;
    private bool _suppressNrCommand;
    private bool _suppressAnCommand;
    private bool _suppressCompressionCommand;
    private bool _suppressAgcCommand;
    private bool _suppressMonitorCommand;
    private bool _suppressDefaultFilterSend;
    private bool _suppressAudioSend;
    private bool _suppressCwSend;
    private bool _suppressRitSend;
    private bool _suppressPowerCalSlider;
    private bool _suppressAmpCalSlider;
    private bool _suppressTxIqOffset;
    private bool _suppressRxIqOffset;
    private bool _suppressRxIqFreqTune;
    private int _powerCalPreviousReceivedStep;
    private int _powerCalPendingBand;
    private int _ampCalPendingBand;
    private long _rxIqBaseFreqHz;
    private int _rxIqBandMeters;
    private string _modeBeforeAmpCal = "USB";
    private string _modeBeforeTxIq = "USB";
    private bool _sessionCompressionOn;
    private string _modeBeforeTune = "USB";
    /// <summary>Mode before FREQ CAL forced CW (restored when cal ends).</summary>
    private string? _modeBeforeFreqCal;
    private bool _onGenBand;
    private int _genIndexProficio = 7; // USER
    private int _genIndexGeminus;
    private bool _suppressSettingsSave;
    private bool _suppressLastUsedSave;
    private bool _loadingAppearance;
    private DispatcherTimer? _settingsSaveTimer;
    private DispatcherTimer? _lastUsedSaveTimer;

    // FREQ CAL
    private bool _freqCalIsAuto;
    private int _lastCalDelta;
    private int _freqCalManualPpmLastSent = int.MinValue;
    private DateTime _freqCalManualPpmLastSendUtc = DateTime.MinValue;
    private DispatcherTimer? _freqCalManualPpmTimer;
    private const int FreqCalManualPpmMin = -100;
    private const int FreqCalManualPpmMax = 100;
    private const int FreqCalManualPpmMinIntervalMs = 300;

    private static readonly int[] CalBandNumbers = { 2200, 630, 160, 80, 60, 40, 30, 20, 17, 15, 12, 10 };
    // Same set/order as QRP CAL / AMP CAL (no radio-model filter).
    private static readonly int[] TxIqBandNumbers = { 2200, 630, 160, 80, 60, 40, 30, 20, 17, 15, 12, 10 };

    /// <summary>Proficio GEN: HF time/freq standards + USER.</summary>
    private static readonly (string Label, long Freq)[] GenOptionsProficio =
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

    /// <summary>Geminus GEN: LF frequency-cal carriers.</summary>
    private static readonly (string Label, long Freq)[] GenOptionsGeminus =
    {
        ("198", 198_000L),
        ("660", 660_000L),
        ("880", 880_000L),
    };

    // ALC meter smoothing (match WPF rolling mean + idle zero)
    private readonly int[] _alcSampleRing = new int[8];
    private int _alcSampleIndex;
    private int _alcSampleCount;
    private DispatcherTimer? _alcIdleTimer;
    private const double AlcIdleTimeoutSeconds = 3.0;

    private const int SpectrumRefreshDivisor = 3;

    public MainViewModel()
    {
        string logDir = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "MSCC-Avalonia", "logs");
        DebugMonitor.Initialize(logDir);
        LogDirectory = logDir;
        LogFilePath = Path.Combine(logDir, "mscc.log");

        DebugMonitor.LogMessage += OnDebugLogMessage;

        Host = "127.0.0.1";
        RemotePortText = "8888";
        LocalPortText = "8889";
        StatusText = "Disconnected — MSCC Start, then Connect.";
        FrequencyMhzEdit = FormatMhz(_frequencyHz);
        FrequencyDisplayMhz = FormatMhz(_frequencyHz);
        VfoBDisplayMhz = FormatMhz(_vfoBFrequencyHz);
        StepLabel = FormatStep(StepChoicesHz[_stepIndex]);
        LowCutLabel = LowCutLabels[_lowCutIndex];
        HighCutLabel = HighCutLabels[_highCutIndex];
        CwFilterLabel = CwFilterLabels[_cwFilterIndex];
        ModeText = "USB";
        AppendLog("MSCC Avalonia 0.6.39 — spectrum + keep-alive hardening; CQ memory, Remote Audio, legacy keyer.");
        AppendLog("PTT = TX (voice modes); TUN = TUNE + carrier. S/W opens pan settings.");
        AppendLog($"Log: {LogFilePath}");
        CwPitchLabel = CwPitchOptions[Math.Clamp(CwPitchIndex, 0, CwPitchOptions.Count - 1)];
        FavoriteBandFilter = "40m";
        InitPowerCalBandStatuses();
        InitAmpCalBandStatuses();
        EnsureTxIqBandItems();
        LoadClientSettings();
        LoadFavoritesFromStore();
        AppendLog($"Settings: {ClientSettingsStore.StorePath}");
        AppendLog($"Favorites: {FavoritesStore.StorePath}");
        CoreVersionText = "—";
        FirmwareText = "—";
        AgcButtonText = "SLO";
        SpectrumZoom = SpectrumDisplaySettings.Instance.ZoomFactor;
        SpectrumDisplaySettings.Instance.Changed += OnSpectrumSettingsChanged;
        AppearanceSettings.Instance.Changed += OnAppearanceSettingsChanged;
        SyncAppearanceUiFromSettings();
        SyncRfPowerFromMode();
        NotifyModeFlags();
    }

    private void OnSpectrumSettingsChanged()
    {
        double z = SpectrumDisplaySettings.Instance.ZoomFactor;
        if (Math.Abs(SpectrumZoom - z) > 0.01)
            SpectrumZoom = z;
        ScheduleSaveClientSettings();
    }

    private void OnAppearanceSettingsChanged()
    {
        if (_suppressSettingsSave) return;
        SyncAppearanceUiFromSettings();
        ScheduleSaveClientSettings();
    }

    private void SyncAppearanceUiFromSettings()
    {
        var a = AppearanceSettings.Instance;
        _loadingAppearance = true;
        SelectedUiBackground = a.UiBackground;
        SelectedUiPanel = a.UiPanel;
        SelectedUiButton = a.UiButton;
        UiBackgroundRgbText = a.UiBackgroundRgb;
        UiButtonRgbText = a.UiButtonRgb;
        UiPanelRgbText = UiChromeTheme.ToHex(a.ResolvePanel());
        if (UiChromeTheme.TryParseHex(a.UiBackgroundRgb, out byte br, out byte bg, out byte bb))
        {
            UiBgR = br; UiBgG = bg; UiBgB = bb;
        }
        if (UiChromeTheme.TryParseHex(a.UiButtonRgb, out byte fr, out byte fg, out byte fb))
        {
            UiBtnR = fr; UiBtnG = fg; UiBtnB = fb;
        }
        UiPanelListEnabled = !UiChromeTheme.IsCustom(a.UiBackground);
        ShowUiBackgroundRgb = UiChromeTheme.IsCustom(a.UiBackground);
        ShowUiButtonRgb = UiChromeTheme.IsCustom(a.UiButton);
        _loadingAppearance = false;
    }

    partial void OnSpectrumZoomChanged(double value)
    {
        SpectrumDisplaySettings.Instance.SetZoomFactor(value);
    }

    // ----- Connection -----

    [ObservableProperty] private string _host = "127.0.0.1";
    /// <summary>Remote UDP port as plain text (no spinner).</summary>
    [ObservableProperty] private string _remotePortText = "8888";
    /// <summary>Local pan RX port as plain text.</summary>
    [ObservableProperty] private string _localPortText = "8889";
    [ObservableProperty] private string _statusText = "Disconnected";
    [ObservableProperty] private bool _isConnected;
    [ObservableProperty] private bool _isBusy;
    [ObservableProperty] private bool _autoStart; // placeholder only
    [ObservableProperty] private bool _launchServers; // always false for Pi connect-only

    // ----- Live radio display -----

    [ObservableProperty] private string _frequencyText = "—";
    [ObservableProperty] private string _frequencyMhzEdit = "7.000000";
    [ObservableProperty] private string _frequencyDisplayMhz = "7.000000";
    [ObservableProperty] private string _modeText = "USB";
    [ObservableProperty] private string _smeterText = "—";
    /// <summary>S-meter units 0–15 (WPF Db_to_Smeter). Drives analog face.</summary>
    [ObservableProperty] private double _sMeter;
    [ObservableProperty] private string _alcText = "—";
    /// <summary>ALC meter 0–100 (smoothed). Drives analog face.</summary>
    [ObservableProperty] private double _alcValue;
    [ObservableProperty] private string _coreVersionText = "—";
    [ObservableProperty] private string _firmwareText = "—";
    [ObservableProperty] private string _bandText = "—";
    [ObservableProperty] private string _stepLabel = "1 kHz";
    [ObservableProperty] private string _lowCutLabel = "500";
    [ObservableProperty] private string _highCutLabel = "2.7k";
    [ObservableProperty] private string _cwFilterLabel = "1.8k";
    [ObservableProperty] private string _packetStatsText = "Pkts 0 | KA 0 | Spec 0";
    [ObservableProperty] private string _logDirectory = "";
    [ObservableProperty] private string _logFilePath = "";
    [ObservableProperty] private SpectrumUpdate? _currentSpectrum;
    [ObservableProperty] private string _vfoBDisplayMhz = "14.200000";
    [ObservableProperty] private string _vfoBModeText = "USB";
    [ObservableProperty] private bool _useVfoA = true;

    // ----- Audio (phones / digital paths) -----

    [ObservableProperty] private int _pVolume = 50;
    [ObservableProperty] private int _pMicGain = 40;
    [ObservableProperty] private int _dVolume = 50;
    [ObservableProperty] private int _dMicGain = 40;
    /// <summary>false = Phones/operator (P), true = Digital/VAC (D).</summary>
    [ObservableProperty] private bool _isDigitalAudio;
    /// <summary>With Phones: CMD_SET_AUDIO_DEVICE=2 (remote mic). Sticky; ignored on Digital.</summary>
    [ObservableProperty] private bool _remoteAudio;
    [ObservableProperty] private int _ritOffset;
    [ObservableProperty] private bool _ritOn;
    [ObservableProperty] private double _spectrumZoom = 1;

    // ----- CW tab -----
    [ObservableProperty] private int _cwKeyerMode = 1; // IAMBIC-A
    [ObservableProperty] private int _cwSpacing;
    [ObservableProperty] private int _cwPaddle;
    [ObservableProperty] private int _cwWeightIndex = 1; // 50
    [ObservableProperty] private int _cwPitchIndex = 1; // 600Hz
    [ObservableProperty] private int _cwHold = 100;
    [ObservableProperty] private bool _cwQsk;
    [ObservableProperty] private bool _cwPhones;
    // Keyer CQ memory (4 slots × 48 chars) — text is client sticky; radio after R
    [ObservableProperty] private string _keyerMem0 = "";
    [ObservableProperty] private string _keyerMem1 = "";
    [ObservableProperty] private string _keyerMem2 = "";
    [ObservableProperty] private string _keyerMem3 = "";
    [ObservableProperty] private bool _keyerMemBusy;
    [ObservableProperty] private string _keyerMemStatus = "";

    /// <summary>
    /// External electronic keyer / legacy radio → mscc.ini PROFICIO-MKII=0.
    /// Default false (MKII). Applied when ms-sdr starts; flip while connected needs reconnect/restart.
    /// </summary>
    [ObservableProperty] private bool _externalElectronicKeyer;

    /// <summary>PIC keyer CW controls enabled when not in external/legacy mode.</summary>
    public bool PicKeyerControlsEnabled => !ExternalElectronicKeyer;

    /// <summary>CQ memory panel: not busy and not legacy external keyer.</summary>
    public bool KeyerMemPanelEnabled => !KeyerMemBusy && !ExternalElectronicKeyer;

    partial void OnKeyerMemBusyChanged(bool value) => OnPropertyChanged(nameof(KeyerMemPanelEnabled));

    // ----- RX/TX power banks (wired to Core when connected) -----
    [ObservableProperty] private int _tunePowerPercent = 25;
    [ObservableProperty] private int _cwPowerPercent = 40;
    [ObservableProperty] private int _ssbPowerPercent = 50;
    [ObservableProperty] private int _amCarrierPercent = 30;
    /// <summary>Right-rail mirror of the active mode's power bank.</summary>
    [ObservableProperty] private int _rfPower = 50;

    [ObservableProperty] private int _compression;
    [ObservableProperty] private bool _compressionOn;
    [ObservableProperty] private int _agcFastRelease = 50;
    /// <summary>0=SLOW, 1=MED, 2=FAST (WPF AgcLevel).</summary>
    [ObservableProperty] private int _agcLevel;
    [ObservableProperty] private string _agcButtonText = "SLO";
    /// <summary>When true, new Core log lines are not added to the UI list (file still logs).</summary>
    [ObservableProperty] private bool _logUiPaused;
    [ObservableProperty] private bool _nbOn;
    [ObservableProperty] private int _nbPulse = 10;
    [ObservableProperty] private int _nbThreshold = 20;
    [ObservableProperty] private bool _nrOn;
    [ObservableProperty] private int _nrLevel = 40;
    [ObservableProperty] private bool _anOn;
    [ObservableProperty] private bool _monitorOn;
    [ObservableProperty] private int _cwSpeed = 20;
    [ObservableProperty] private string _cwPitchLabel = "600Hz";
    [ObservableProperty] private string _proficioTempText = "— °C";
    [ObservableProperty] private string _paTempText = "— °C";
    [ObservableProperty] private string _paCurrentText = "— mA";
    [ObservableProperty] private string _clientVersionText = "0.6.39";
    [ObservableProperty] private bool _qrpMode = true;
    [ObservableProperty] private bool _fullPower;
    [ObservableProperty] private bool _alcOn;
    /// <summary>AMP / QRO path (PA bypass). Red when on (WPF).</summary>
    [ObservableProperty] private bool _ampOn;

    /// <summary>PTT latched (CMD_SET_TX_ON).</summary>
    [ObservableProperty] private bool _pttOn;

    /// <summary>TUN latched — TUNE mode + rig tune (antenna tune carrier).</summary>
    [ObservableProperty] private bool _tuneMode;

    /// <summary>Server owns TX via 0xBC — disables user PTT/TUN.</summary>
    [ObservableProperty] private bool _txSetByServer;

    // Mode latch flags for UI
    [ObservableProperty] private bool _modeIsUsb;
    [ObservableProperty] private bool _modeIsLsb;
    [ObservableProperty] private bool _modeIsCw;
    [ObservableProperty] private bool _modeIsAm;
    [ObservableProperty] private bool _modeIsDigU;

    // Main band-bar latch (selected under pointer stays highlighted)
    [ObservableProperty] private bool _bandIs2200;
    [ObservableProperty] private bool _bandIs630;
    [ObservableProperty] private bool _bandIs160;
    [ObservableProperty] private bool _bandIs80;
    [ObservableProperty] private bool _bandIs60;
    [ObservableProperty] private bool _bandIs40;
    [ObservableProperty] private bool _bandIs30;
    [ObservableProperty] private bool _bandIs20;
    [ObservableProperty] private bool _bandIs17;
    [ObservableProperty] private bool _bandIs15;
    [ObservableProperty] private bool _bandIs12;
    [ObservableProperty] private bool _bandIs10;
    [ObservableProperty] private bool _bandIsGen;

    public ObservableCollection<string> LogLines { get; } = new();

    // RX/TX tab default filter lists (WPF wording)
    public ObservableCollection<string> LowCutOptions { get; } = new()
        { "500Hz", "300Hz", "200Hz", "100Hz", "75Hz" };
    public ObservableCollection<string> HighCutOptions { get; } = new()
        { "5.5KHz", "4.0KHz", "3.0KHz", "2.7KHz", "2.4KHz" };
    public ObservableCollection<string> CwFilterOptions { get; } = new()
        { "1.8KHz", "400Hz", "200Hz" };
    public ObservableCollection<string> TxOptions { get; } = new()
        { "2.4KHz", "2.7KHz", "3.0KHz", "5.5KHz" };

    public ObservableCollection<string> CwKeyerModeOptions { get; } = new()
        { "STRAIGHT", "IAMBIC-A", "IAMBIC-B" };
    public ObservableCollection<string> CwSpacingOptions { get; } = new()
        { "ELEMENT", "LETTER" };
    public ObservableCollection<string> CwPaddleOptions { get; } = new()
        { "NORMAL", "REVERSE" };
    public ObservableCollection<string> CwWeightOptions { get; } = new()
        { "25", "50", "75" };
    public ObservableCollection<string> CwPitchOptions { get; } = new()
        { "400Hz", "600Hz", "800Hz", "1000Hz" };

    // ----- Favorites (disk: mscc-favorites.ini) -----
    public ObservableCollection<FavoriteEntry> Favorites { get; } = new();
    public ObservableCollection<FavoriteEntry> FavoritesForBand { get; } = new();
    public ObservableCollection<string> FavoriteBandChoices { get; } = new()
    {
        "2200m", "630m", "160m", "80m", "60m", "40m", "30m", "20m", "17m", "15m", "12m", "10m", "gen"
    };

    [ObservableProperty] private string _favoriteBandFilter = "40m";
    [ObservableProperty] private string _favoriteNameInput = "";
    [ObservableProperty] private FavoriteEntry? _selectedFavorite;

    // ----- QRP CAL -----
    public ObservableCollection<PowerCalBandItem> PowerCalBandStatuses { get; } = new();
    [ObservableProperty] private int _powerCalSelectedBand;
    [ObservableProperty] private int _powerCalSliderValue;
    [ObservableProperty] private string _powerCalStepLabel = "CALIBRATION STEP: —";
    [ObservableProperty] private bool _powerCalTxOn;
    [ObservableProperty] private bool _powerCalCalibrating;
    [ObservableProperty] private bool _powerCalLoadConfirmed;
    [ObservableProperty] private bool _powerCalAcceptPrompt;

    // ----- AMP CAL -----
    public ObservableCollection<PowerCalBandItem> AmpCalBandStatuses { get; } = new();
    [ObservableProperty] private int _ampCalSelectedBand;
    [ObservableProperty] private int _ampCalSliderValue = -99;
    [ObservableProperty] private string _ampCalStepLabel = "STEP: 0";
    [ObservableProperty] private bool _ampCalTxOn;
    [ObservableProperty] private bool _ampCalCalibrating;
    [ObservableProperty] private bool _ampCalAcceptPrompt;

    // ----- RX IQ -----
    [ObservableProperty] private bool _rxIqSessionActive;
    [ObservableProperty] private int _rxIqOffset;
    [ObservableProperty] private int _rxIqFreqOffsetHz;
    [ObservableProperty] private bool _rxIqUp24k;
    [ObservableProperty] private string _rxIqBandLabel = "—";
    [ObservableProperty] private string _rxIqFreqDisplay = "—.—.—";
    [ObservableProperty] private string _rxIqStatus =
        "Select an amateur band (MAIN / band bar), then START.";
    [ObservableProperty] private bool _rxIqCommitting;
    [ObservableProperty] private bool _rxIqResetAllPrompt;

    // ----- TX IQ (QRP only) -----
    public ObservableCollection<PowerCalBandItem> TxIqBandItems { get; } = new();
    [ObservableProperty] private int _txIqSelectedBand;
    [ObservableProperty] private int _txIqOffset;
    [ObservableProperty] private int _txIqPower = 100;
    [ObservableProperty] private bool _txIqTxOn;
    [ObservableProperty] private string _txIqStatus = "Select a band (QRP only).";
    [ObservableProperty] private bool _txIqCommitting;
    [ObservableProperty] private bool _txIqResetAllPrompt;

    // ----- FREQ CAL -----
    [ObservableProperty] private bool _freqCalLoose = true;
    [ObservableProperty] private bool _freqCalManualMode;
    [ObservableProperty] private bool _freqCalInProgress;
    [ObservableProperty] private int _freqCalManualPpm;
    [ObservableProperty] private int _freqCalProgress;
    [ObservableProperty] private string _freqCalStatus = "OK";
    [ObservableProperty] private bool _freqCalAutoModePrompt;
    [ObservableProperty] private bool _freqCalManualAcceptPrompt;
    [ObservableProperty] private bool _freqCalResetPrompt;

    public string FreqCalLooseButtonText => FreqCalLoose ? "LOOSE" : "TIGHT";
    public string FreqCalManualButtonText => FreqCalManualMode ? "MANUAL ON" : "MANUAL";
    public bool FreqCalActionsEnabled => !FreqCalInProgress && !FreqCalManualMode && !FreqCalAutoModePrompt && !FreqCalManualAcceptPrompt;
    public bool FreqCalManualButtonEnabled => !FreqCalInProgress && !FreqCalAutoModePrompt;
    public bool FreqCalPpmEnabled => FreqCalManualMode && !FreqCalInProgress && !FreqCalManualAcceptPrompt;

    /// <summary>QRP CAL tab enabled when AMP path is off.</summary>
    public bool IsPowerCalTabEnabled => !AmpOn;
    /// <summary>AMP CAL tab enabled when AMP path is on.</summary>
    public bool IsAmpCalTabEnabled => AmpOn;
    /// <summary>TX IQ only when AMP is off (QRP).</summary>
    public bool IsTxIqTabEnabled => !AmpOn;
    public string PowerCalTabHint => AmpOn
        ? "Turn AMP off (right rail) to use QRP CAL."
        : "Select band → confirm dummy load → CALIBRATE, adjust POWER, then stop and Accept.";
    public string AmpCalTabHint => AmpOn
        ? "Select band (needs green QRP lamp) → CALIBRATE, adjust POWER, then Accept."
        : "Turn AMP on (right rail) to use AMP CAL.";
    public string TxIqTabHint => AmpOn
        ? "Turn AMP off to use TX IQ (QRP only)."
        : "Select band → set power → TX ON → adjust OFFSET (external RX) → APPLY.";
    public string PowerCalTxButtonText => PowerCalTxOn ? "TX ON" : "TX";
    public string PowerCalCalibrateButtonText => PowerCalCalibrating ? "CALIBRATING" : "CALIBRATE";
    public string AmpCalTxButtonText => AmpCalTxOn ? "TX ON" : "TX";
    public string AmpCalCalibrateButtonText => AmpCalCalibrating ? "CALIBRATING" : "CALIBRATE";
    public string RxIqStartButtonText => RxIqSessionActive ? "ACTIVE" : "START";
    public string TxIqTxButtonText => TxIqTxOn ? "TX ON" : "TX";
    public bool PowerCalTxButtonEnabled => !PowerCalCalibrating;
    public bool AmpCalTxButtonEnabled => !AmpCalCalibrating;
    public bool TxIqBandSelectEnabled => !TxIqTxOn;

    // ----- Radio model + GEN -----
    [ObservableProperty] private bool _isGeminusRadioModel;
    [ObservableProperty] private string _genButtonText = "USER";

    public string RadioModelButtonText => IsGeminusRadioModel ? "Geminus" : "Proficio";
    public bool HfBandsEnabled => !IsGeminusRadioModel;
    public bool LfBandsEnabled => IsGeminusRadioModel;
    public string GenButtonTip => IsGeminusRadioModel
        ? "GEN (Geminus LF): 198 / 660 / 880 kHz cal carriers. Press again to rotate."
        : "GEN (Proficio): WWV / CHU / RWM / USER. Press again to rotate.";

    public string SettingsFilePath => ClientSettingsStore.StorePath;

    /// <summary>True when VFO B is the active radio VFO.</summary>
    public bool UseVfoB => !UseVfoA;

    /// <summary>Default low-cut index (RX/TX tab → radio).</summary>
    [ObservableProperty] private int _lowCutDefaultIndex;
    [ObservableProperty] private int _highCutDefaultIndex;
    [ObservableProperty] private int _cwFilterDefaultIndex;
    [ObservableProperty] private int _txDefaultIndex;
    /// <summary>Active TX bandwidth index (RX/TX tab TX BW list).</summary>
    [ObservableProperty] private int _txBandwidthIndex;

    /// <summary>Title bar like WPF: product + MSCC / Core / FW versions.</summary>
    public string WindowTitle =>
        $"MSCC Avalonia   ·   MSCC: {ClientVersionText}   Core: {CoreVersionText}   FW: {FirmwareText}";

    public string ConnectButtonText => IsConnected ? "Disconnect" : "Connect";
    public string StubTip => "Layout placeholder — not wired yet";
    /// <summary>Left-rail Audio path button: Phones or Digital.</summary>
    public string AudioPathButtonText => IsDigitalAudio ? "Digital" : "Phones";

    /// <summary>Remote Audio checkbox enabled only on Phones path.</summary>
    public bool RemoteAudioCheckboxEnabled => !IsDigitalAudio;

    /// <summary>User may press PTT/TUN only when connected and server is not locking TX.</summary>
    public bool CanUserControlTransmit => IsConnected && !IsBusy && !TxSetByServer && _radio != null;

    partial void OnIsConnectedChanged(bool value)
    {
        OnPropertyChanged(nameof(ConnectButtonText));
        OnPropertyChanged(nameof(CanUserControlTransmit));
        NotifyOperateCommands();
        if (!value)
        {
            CancelKeyerPlayPttRelease(releasePtt: false);
            _keyerPlayOwnsPtt = false;
            // UI state only after radio disposed / about to dispose
            _suppressTransmitCommands = true;
            PttOn = false;
            TuneMode = false;
            _suppressTransmitCommands = false;
            TxSetByServer = false;
        }
    }

    partial void OnTxSetByServerChanged(bool value)
    {
        OnPropertyChanged(nameof(CanUserControlTransmit));
        TogglePttCommand.NotifyCanExecuteChanged();
        ToggleTuneCommand.NotifyCanExecuteChanged();
    }

    partial void OnIsBusyChanged(bool value)
    {
        OnPropertyChanged(nameof(CanUserControlTransmit));
        TogglePttCommand.NotifyCanExecuteChanged();
        ToggleTuneCommand.NotifyCanExecuteChanged();
    }

    partial void OnClientVersionTextChanged(string value) => OnPropertyChanged(nameof(WindowTitle));
    partial void OnCoreVersionTextChanged(string value) => OnPropertyChanged(nameof(WindowTitle));
    partial void OnFirmwareTextChanged(string value) => OnPropertyChanged(nameof(WindowTitle));

    partial void OnModeTextChanged(string value)
    {
        SyncRfPowerFromMode();
        NotifyModeFlags();
    }

    private void NotifyModeFlags()
    {
        string m = (ModeText ?? "").Trim().ToUpperInvariant();
        ModeIsUsb = m is "USB";
        ModeIsLsb = m is "LSB";
        ModeIsCw = m is "CW";
        ModeIsAm = m is "AM";
        ModeIsDigU = m is "DIG-U" or "DIGU" or "DIG";
    }

    partial void OnBandTextChanged(string value) => NotifyBandFlags();

    private void NotifyBandFlags()
    {
        string b = NormalizeBandLabel(BandText);
        BandIs2200 = b is "2200m";
        BandIs630 = b is "630m";
        BandIs160 = b is "160m";
        BandIs80 = b is "80m";
        BandIs60 = b is "60m";
        BandIs40 = b is "40m";
        BandIs30 = b is "30m";
        BandIs20 = b is "20m";
        BandIs17 = b is "17m";
        BandIs15 = b is "15m";
        BandIs12 = b is "12m";
        BandIs10 = b is "10m";
        BandIsGen = b is "gen";
    }

    partial void OnTunePowerPercentChanged(int value)
    {
        if (IsTuneBankActive()) SyncRfPowerFromMode(force: true);
        ScheduleSaveClientSettings();
        if (_suppressPowerSend || !CanOperate()) return;
        _ = SendTunePowerAsync(Math.Clamp(value, 0, 100));
    }

    partial void OnCwPowerPercentChanged(int value)
    {
        if (IsCwBankActive()) SyncRfPowerFromMode(force: true);
        ScheduleSaveClientSettings();
        if (_suppressPowerSend || !CanOperate()) return;
        _ = SendCwPowerAsync(Math.Clamp(value, 0, 100));
    }

    partial void OnSsbPowerPercentChanged(int value)
    {
        if (IsSsbBankActive()) SyncRfPowerFromMode(force: true);
        ScheduleSaveClientSettings();
        if (_suppressPowerSend || !CanOperate()) return;
        _ = SendSsbPowerAsync(Math.Clamp(value, 0, 100));
    }

    partial void OnAmCarrierPercentChanged(int value)
    {
        if (IsAmBankActive()) SyncRfPowerFromMode(force: true);
        ScheduleSaveClientSettings();
        if (_suppressPowerSend || !CanOperate()) return;
        _ = SendAmCarrierAsync(Math.Clamp(value, 0, 100));
    }

    /// <summary>Right-rail RF slider writes into the active mode's power bank.</summary>
    partial void OnRfPowerChanged(int value)
    {
        if (_syncingRfMirror) return;
        value = Math.Clamp(value, 0, 100);
        string m = (ModeText ?? "").Trim().ToUpperInvariant();
        switch (m)
        {
            case "TUNE":
                if (TunePowerPercent != value) TunePowerPercent = value;
                break;
            case "CW":
                if (CwPowerPercent != value) CwPowerPercent = value;
                break;
            case "AM":
                if (AmCarrierPercent != value) AmCarrierPercent = value;
                break;
            default:
                // USB / LSB / DIG-U / other → SSB bank
                if (SsbPowerPercent != value) SsbPowerPercent = value;
                break;
        }
    }

    private void NotifyOperateCommands()
    {
        ConnectCommand.NotifyCanExecuteChanged();
        DisconnectCommand.NotifyCanExecuteChanged();
        ToggleConnectCommand.NotifyCanExecuteChanged();
        SetFrequencyCommand.NotifyCanExecuteChanged();
        TuneUpCommand.NotifyCanExecuteChanged();
        TuneDownCommand.NotifyCanExecuteChanged();
        CycleStepCommand.NotifyCanExecuteChanged();
        SetModeCommand.NotifyCanExecuteChanged();
        SelectBandCommand.NotifyCanExecuteChanged();
        TogglePttCommand.NotifyCanExecuteChanged();
        ToggleTuneCommand.NotifyCanExecuteChanged();
        CycleAgcCommand.NotifyCanExecuteChanged();
        ToggleAmpCommand.NotifyCanExecuteChanged();
        ToggleCompressionCommand.NotifyCanExecuteChanged();
        ToggleMonitorCommand.NotifyCanExecuteChanged();
        ToggleNbCommand.NotifyCanExecuteChanged();
        ToggleNrCommand.NotifyCanExecuteChanged();
        ToggleAnCommand.NotifyCanExecuteChanged();
        OnPropertyChanged(nameof(CanUserControlTransmit));
    }

    private bool CanOperate() => IsConnected && !IsBusy && _radio != null;

    private bool CanTogglePtt() => CanUserControlTransmit;
    private bool CanToggleTune() => CanUserControlTransmit;

    [RelayCommand(CanExecute = nameof(CanTogglePtt))]
    private void TogglePtt() => PttOn = !PttOn;

    [RelayCommand(CanExecute = nameof(CanToggleTune))]
    private void ToggleTune() => TuneMode = !TuneMode;

    partial void OnPttOnChanged(bool value)
    {
        TogglePttCommand.NotifyCanExecuteChanged();
        // User (or forced) drop of PTT cancels auto-release ownership
        if (!value && _keyerPlayOwnsPtt)
        {
            _keyerPlayOwnsPtt = false;
            try { _keyerPlayPttReleaseCts?.Cancel(); } catch { /* ignore */ }
        }
        if (_suppressTransmitCommands) return;
        if (!CanOperate() || TxSetByServer) return;

        _ = SendPttAsync(value);
        MaybeZeroAlcMeterOnRx();
    }

    partial void OnTuneModeChanged(bool value)
    {
        ToggleTuneCommand.NotifyCanExecuteChanged();
        if (_suppressTransmitCommands)
        {
            if (value)
                ModeText = "TUNE";
            return;
        }

        if (value)
        {
            // Remember mode to restore when TUN released
            string cur = (ModeText ?? "USB").Trim();
            if (!string.Equals(cur, "TUNE", StringComparison.OrdinalIgnoreCase) && cur.Length > 0)
                _modeBeforeTune = cur;
            ModeText = "TUNE";
            if (CanOperate() && !TxSetByServer)
                _ = ApplyTuneOnAsync();
        }
        else
        {
            string restore = string.IsNullOrWhiteSpace(_modeBeforeTune) ? "USB" : _modeBeforeTune;
            if (string.Equals(ModeText, "TUNE", StringComparison.OrdinalIgnoreCase))
                ModeText = restore;
            if (CanOperate() && !TxSetByServer)
                _ = ApplyTuneOffAsync(restore);
        }

        MaybeZeroAlcMeterOnRx();
    }

    private async Task SendPttAsync(bool on)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetTransmitAsync(on).ConfigureAwait(true);
            bool cw = string.Equals((ModeText ?? "").Trim(), "CW", StringComparison.OrdinalIgnoreCase);
            if (on && cw)
            {
                StatusText = "PTT latched (CW: PA still needs keyer/paddle)";
                AppendLog("PTT ON sent (0xBA) — in CW mode Proficio keys PA from keyer line, not host PTT");
            }
            else
            {
                StatusText = on ? "PTT ON" : "PTT OFF";
                AppendLog($"PTT {(on ? "ON" : "OFF")} (CMD_SET_TX_ON 0xBA)");
            }
        }
        catch (Exception ex)
        {
            AppendLog($"PTT error: {ex.Message}");
            _suppressTransmitCommands = true;
            PttOn = false;
            _suppressTransmitCommands = false;
        }
    }

    private async Task ApplyTuneOnAsync()
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetModeAsync("TUNE").ConfigureAwait(true);
            await _radio.SetTunePowerAsync(TunePowerPercent).ConfigureAwait(true);
            await _radio.SetAutoTuneAsync(true).ConfigureAwait(true);
            StatusText = $"TUN ON (Tune Power {TunePowerPercent}%)";
            AppendLog($"TUN ON — mode TUNE, rig tune, power {TunePowerPercent}%");
        }
        catch (Exception ex)
        {
            AppendLog($"TUN on error: {ex.Message}");
            _suppressTransmitCommands = true;
            TuneMode = false;
            _suppressTransmitCommands = false;
        }
    }

    private async Task ApplyTuneOffAsync(string restoreMode)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetAutoTuneAsync(false).ConfigureAwait(true);
            await _radio.SetModeAsync(restoreMode).ConfigureAwait(true);
            StatusText = $"TUN OFF → {restoreMode}";
            AppendLog($"TUN OFF — restored mode {restoreMode}");
        }
        catch (Exception ex)
        {
            AppendLog($"TUN off error: {ex.Message}");
        }
    }

    /// <summary>Force RX: PTT and TUN off (disconnect / safety).</summary>
    private async Task ForceTxOffAsync()
    {
        if (_radio == null) return;
        try
        {
            if (PttOn || TuneMode)
                AppendLog("Forcing PTT/TUN off…");
            await _radio.SetTransmitAsync(false).ConfigureAwait(true);
            await _radio.SetAutoTuneAsync(false).ConfigureAwait(true);
        }
        catch (Exception ex)
        {
            AppendLog($"Force TX off: {ex.Message}");
        }
    }

    private string ActiveModeString =>
        UseVfoA
            ? (ModeText ?? "USB")
            : (VfoBModeText ?? "USB");

    private bool IsTuneBankActive() =>
        string.Equals(ActiveModeString, "TUNE", StringComparison.OrdinalIgnoreCase);

    private bool IsCwBankActive() =>
        string.Equals(ActiveModeString, "CW", StringComparison.OrdinalIgnoreCase);

    private bool IsAmBankActive() =>
        string.Equals(ActiveModeString, "AM", StringComparison.OrdinalIgnoreCase);

    private bool IsSsbBankActive() =>
        !IsTuneBankActive() && !IsCwBankActive() && !IsAmBankActive();

    private void SyncRfPowerFromMode(bool force = false)
    {
        int target = IsTuneBankActive() ? TunePowerPercent
            : IsCwBankActive() ? CwPowerPercent
            : IsAmBankActive() ? AmCarrierPercent
            : SsbPowerPercent;

        if (!force && RfPower == target) return;
        _syncingRfMirror = true;
        try { RfPower = target; }
        finally { _syncingRfMirror = false; }
    }

    private async Task SendTunePowerAsync(int percent)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetTunePowerAsync(percent).ConfigureAwait(true);
            AppendLog($"Sent Tune Power {percent}%");
        }
        catch (Exception ex) { AppendLog($"Tune power error: {ex.Message}"); }
    }

    private async Task SendCwPowerAsync(int percent)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetCwPowerAsync(percent).ConfigureAwait(true);
            AppendLog($"Sent CW Power {percent}%");
        }
        catch (Exception ex) { AppendLog($"CW power error: {ex.Message}"); }
    }

    private async Task SendSsbPowerAsync(int percent)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetSsbPowerAsync(percent).ConfigureAwait(true);
            AppendLog($"Sent SSB Power {percent}%");
        }
        catch (Exception ex) { AppendLog($"SSB power error: {ex.Message}"); }
    }

    private async Task SendAmCarrierAsync(int percent)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetAmCarrierAsync(percent).ConfigureAwait(true);
            AppendLog($"Sent AM Carrier {percent}%");
        }
        catch (Exception ex) { AppendLog($"AM carrier error: {ex.Message}"); }
    }

    private void ApplyReportedPower(Action apply)
    {
        _suppressPowerSend = true;
        try { apply(); }
        finally { _suppressPowerSend = false; }
        SyncRfPowerFromMode();
    }

    // ----- Connect -----

    [RelayCommand]
    private async Task ToggleConnectAsync()
    {
        if (IsConnected)
            Disconnect();
        else
            await ConnectAsync().ConfigureAwait(true);
    }

    [RelayCommand(CanExecute = nameof(CanConnect))]
    private async Task ConnectAsync()
    {
        if (IsConnected || IsBusy) return;

        IsBusy = true;
        StatusText = "Connecting…";
        NotifyOperateCommands();

        try
        {
            // Ensure PROFICIO-MKII is on disk before a local ms-sdr (re)start reads mscc.ini.
            MsccIniProficio.WriteProficioMkii(mkii: !ExternalElectronicKeyer);
            AppendLog(ExternalElectronicKeyer
                ? "PROFICIO-MKII=0 (legacy / external electronic keyer)"
                : "PROFICIO-MKII=1 (MKII internal keyer)");

            Host = Host.Trim();
            if (string.IsNullOrWhiteSpace(Host))
            {
                StatusText = "Host required.";
                return;
            }

            if (!TryParsePort(RemotePortText, out int remotePort))
            {
                StatusText = "Port must be 1–65535.";
                return;
            }

            if (!TryParsePort(LocalPortText, out int localPort))
            {
                StatusText = "Local RX port must be 1–65535.";
                return;
            }

            // Normalize displayed text after parse
            RemotePortText = remotePort.ToString(CultureInfo.InvariantCulture);
            LocalPortText = localPort.ToString(CultureInfo.InvariantCulture);

            DisposeRadio();
            _packetsReceived = 0;
            _keepAlivesReceived = 0;
            _panPacketsReceived = 0;
            _spectrumFrames = 0;
            _spectrumFrameCounter = 0;
            CurrentSpectrum = null;
            UpdatePacketStats();

            AppendLog($"Connect → {Host}:{remotePort} (RX {localPort})");
            _radio = new UdpRadioService(Host, remotePort, localPort);
            WireRadioEvents(_radio);
            await _radio.StartAsync(launchSubsystems: false).ConfigureAwait(true);

            IsConnected = true;
            StatusText = $"Connected {Host}:{remotePort}";
            AppendLog("Connected (connect-only).");
            // Ensure pan assembly + heal Linux pan refresh (Blocks≥1). Without this,
            // a prior client that sent 0x5F=0 leaves the Pi with silent no-spectrum.
            try
            {
                await _radio.SetPanResolutionAsync(800).ConfigureAwait(true);
                AppendLog("Pan resolution: 800 bins (refresh healed).");
            }
            catch (Exception ex)
            {
                AppendLog($"Pan resolution apply warning: {ex.Message}");
            }
            // Push selected VFO + freq/mode so dual-VFO state matches UI
            await PushActiveVfoToRadioAsync(force: true).ConfigureAwait(true);
            // Restore sticky operate settings to the radio (server-backed)
            await PushStickyOperateToRadioAsync().ConfigureAwait(true);
        }
        catch (Exception ex)
        {
            IsConnected = false;
            StatusText = $"Connect failed: {ex.Message}";
            AppendLog($"ERROR: {ex.Message}");
            DisposeRadio();
        }
        finally
        {
            IsBusy = false;
            NotifyOperateCommands();
            OnPropertyChanged(nameof(ConnectButtonText));
        }
    }

    private bool CanConnect() => !IsConnected && !IsBusy;

    [RelayCommand(CanExecute = nameof(CanDisconnect))]
    private void Disconnect()
    {
        if (IsBusy) return;
        IsBusy = true;
        try
        {
            AppendLog("Disconnect (servers left running).");
            // Best-effort clear TX before dropping UDP session
            try
            {
                ForceTxOffAsync().GetAwaiter().GetResult();
            }
            catch
            {
                // ignore
            }

            _suppressTransmitCommands = true;
            PttOn = false;
            TuneMode = false;
            _suppressTransmitCommands = false;
            TxSetByServer = false;

            ForceStopPowerCal("disconnect");
            ForceStopAmpCal("disconnect");
            LeaveRxIqSession("disconnect");
            ForceStopTxIqSession("disconnect");
            ForceStopFreqCal("disconnect");
            DisposeRadio();
            IsConnected = false;
            StatusText = "Disconnected";
            FrequencyText = "—";
            SmeterText = "—";
            SMeter = 0;
            AlcText = "—";
            ResetAlcMeter();
            BandText = "—";
            CurrentSpectrum = null;
            _suppressRitSend = true;
            RitOn = false;
            RitOffset = 0;
            _suppressRitSend = false;
        }
        finally
        {
            IsBusy = false;
            NotifyOperateCommands();
            OnPropertyChanged(nameof(ConnectButtonText));
        }
    }

    private bool CanDisconnect() => IsConnected && !IsBusy;

    /// <summary>
    /// Smooth ALC for the needle (WPF-style rolling mean). Maps 0–1000 legacy → 0–100.
    /// </summary>
    private void ApplyAlcMeterSample(int raw)
    {
        int sample = raw > 100 ? Math.Clamp(raw / 10, 0, 100) : Math.Clamp(raw, 0, 100);

        _alcSampleRing[_alcSampleIndex] = sample;
        _alcSampleIndex = (_alcSampleIndex + 1) % _alcSampleRing.Length;
        if (_alcSampleCount < _alcSampleRing.Length)
            _alcSampleCount++;

        long sum = 0;
        for (int i = 0; i < _alcSampleCount; i++)
            sum += _alcSampleRing[i];

        AlcValue = (sum + _alcSampleCount / 2.0) / _alcSampleCount;
        AlcText = ((int)Math.Round(AlcValue)).ToString(CultureInfo.InvariantCulture);
        KickAlcIdleTimer();
    }

    private void MaybeZeroAlcMeterOnRx()
    {
        if (PttOn || TuneMode)
            return;
        _alcIdleTimer?.Stop();
        if (AlcValue != 0 || _alcSampleCount != 0)
        {
            ResetAlcMeter();
            AppendLog("ALC meter zeroed (PTT/TUN off → RX)");
        }
    }

    private void EnsureAlcIdleTimer()
    {
        if (_alcIdleTimer != null) return;
        _alcIdleTimer = new DispatcherTimer
        {
            Interval = TimeSpan.FromSeconds(AlcIdleTimeoutSeconds)
        };
        _alcIdleTimer.Tick += (_, _) =>
        {
            _alcIdleTimer.Stop();
            if (AlcValue != 0 || _alcSampleCount != 0)
            {
                ResetAlcMeter();
                AppendLog($"ALC meter zeroed (no samples for {AlcIdleTimeoutSeconds}s)");
            }
        };
    }

    private void KickAlcIdleTimer()
    {
        EnsureAlcIdleTimer();
        _alcIdleTimer!.Stop();
        _alcIdleTimer.Start();
    }

    private void ResetAlcMeter()
    {
        Array.Clear(_alcSampleRing);
        _alcSampleCount = 0;
        _alcSampleIndex = 0;
        AlcValue = 0;
        AlcText = "0";
    }

    /// <summary>
    /// Map radio dBm (CMD_GET_SET_SMETER) to analog units 0–15 (WPF Db_to_Smeter).
    /// S9 = −73 dBm; S1–S9 at 6 dB/S-unit; above S9 in 10 dB steps (capped at 12 / S9+30).
    /// </summary>
    private static int DbToSmeter(int dbm)
    {
        if (dbm <= -130)
            return 0;

        if (dbm <= -73)
        {
            // S1 to S9: 6 dB per S-unit, S9 = -73 dBm
            int smeterValue = 9 + (dbm + 73) / 6;
            return smeterValue < 1 ? 1 : smeterValue;
        }

        // Above S9: 10 dB steps (S9+10, S9+20, S9+30)
        int dbOverS9 = dbm + 73;
        int overValue = (dbOverS9 + 5) / 10; // round nearest
        if (overValue <= 0)
            return 9;
        if (overValue >= 3)
            return 12; // firmware limit
        return 9 + overValue;
    }

    private static string FormatSmeterReading(int units)
    {
        if (units <= 0) return "S0";
        if (units <= 9) return $"S{units}";
        return $"S9+{(units - 9) * 10}";
    }

    private void ApplySmeterSample(int dbm)
    {
        int units = DbToSmeter(dbm);
        SMeter = units;
        SmeterText = FormatSmeterReading(units);
    }

    public Task TuneFromSpectrumAsync(long frequencyHz) =>
        ApplyFrequencyAsync(frequencyHz, "click-to-tune");

    /// <summary>
    /// VFO A mouse wheel with left-rail Step (fallback when not over a digit).
    /// </summary>
    public Task NudgeFrequencyAsync(int direction) =>
        NudgeFrequencyByDigitAsync(direction, GetCurrentStepHz(), quantize: false, vfoA: UseVfoA);

    public long GetCurrentStepHz() => StepChoicesHz[_stepIndex];

    /// <summary>
    /// Digit-position wheel: step = 10^n from hovered digit; quantize zeros lower digits (WPF style).
    /// Tunes the VFO under the pointer and selects it if needed.
    /// </summary>
    public async Task NudgeFrequencyByDigitAsync(int direction, long stepHz, bool quantize, bool vfoA)
    {
        if (direction == 0 || stepHz <= 0)
            return;

        // Select the VFO being tuned
        if (vfoA && !UseVfoA)
            await SelectVfoAsync(useVfoA: true).ConfigureAwait(true);
        else if (!vfoA && UseVfoA)
            await SelectVfoAsync(useVfoA: false).ConfigureAwait(true);

        long current = vfoA ? _frequencyHz : _vfoBFrequencyHz;
        long baseFreq = quantize ? current - (current % stepHz) : current;
        long hz = Math.Clamp(baseFreq + direction * stepHz, 10_000, 60_000_000);
        await ApplyFrequencyAsync(hz, quantize ? $"digit±{stepHz}" : $"wheel±{stepHz}").ConfigureAwait(true);
    }

    /// <summary>Optional hover readout under VFO (e.g. "1 kHz").</summary>
    public void SetHoverTuneStep(long stepHz)
    {
        HoverTuneStepLabel = stepHz > 0 ? FormatStep(stepHz) : "";
        OnPropertyChanged(nameof(VfoTuneStepDisplay));
    }

    [ObservableProperty] private string _hoverTuneStepLabel = "";

    /// <summary>Under VFO A: hovered digit step, or left-rail Step fallback.</summary>
    public string VfoTuneStepDisplay =>
        string.IsNullOrEmpty(HoverTuneStepLabel)
            ? $"step {StepLabel}"
            : HoverTuneStepLabel;

    // ----- Frequency (live) -----

    [RelayCommand(CanExecute = nameof(CanOperate))]
    private async Task SetFrequencyAsync()
    {
        if (!TryParseMhz(FrequencyMhzEdit, out long hz))
        {
            StatusText = "Invalid MHz";
            return;
        }
        await ApplyFrequencyAsync(hz, "Set").ConfigureAwait(true);
    }

    [RelayCommand(CanExecute = nameof(CanOperate))]
    private async Task TuneUpAsync()
    {
        long step = StepChoicesHz[_stepIndex];
        long hz = Math.Clamp(_frequencyHz + step, 10_000, 60_000_000);
        await ApplyFrequencyAsync(hz, $"+{FormatStep(step)}").ConfigureAwait(true);
    }

    [RelayCommand(CanExecute = nameof(CanOperate))]
    private async Task TuneDownAsync()
    {
        long step = StepChoicesHz[_stepIndex];
        long hz = Math.Clamp(_frequencyHz - step, 10_000, 60_000_000);
        await ApplyFrequencyAsync(hz, $"-{FormatStep(step)}").ConfigureAwait(true);
    }

    [RelayCommand(CanExecute = nameof(CanOperate))]
    private void CycleStep()
    {
        _stepIndex = (_stepIndex + 1) % StepChoicesHz.Length;
        StepLabel = FormatStep(StepChoicesHz[_stepIndex]);
        OnPropertyChanged(nameof(VfoTuneStepDisplay));
        AppendLog($"Step → {StepLabel}");
        ScheduleSaveClientSettings();
    }

    partial void OnStepLabelChanged(string value) => OnPropertyChanged(nameof(VfoTuneStepDisplay));

    /// <summary>Cycle active low cut; send when connected; refresh spectrum passband marker.</summary>
    [RelayCommand]
    private void CycleLowCut()
    {
        _lowCutIndex = (_lowCutIndex + 1) % LowCutLabels.Length;
        LowCutLabel = LowCutLabels[_lowCutIndex];
        int hz = LowCutHzValues[_lowCutIndex];
        RefreshSpectrumFilterOverlay();
        ScheduleSaveClientSettings();
        if (CanOperate())
            _ = SendFilterLowAsync(hz);
        else
            AppendLog($"Lo cut → {LowCutLabel} (not connected)");
    }

    [RelayCommand]
    private void CycleHighCut()
    {
        _highCutIndex = (_highCutIndex + 1) % HighCutLabels.Length;
        HighCutLabel = HighCutLabels[_highCutIndex];
        int hz = HighCutHzValues[_highCutIndex];
        RefreshSpectrumFilterOverlay();
        ScheduleSaveClientSettings();
        if (CanOperate())
            _ = SendFilterHighAsync(hz);
        else
            AppendLog($"Hi cut → {HighCutLabel} (not connected)");
    }

    [RelayCommand]
    private void CycleCwFilter()
    {
        _cwFilterIndex = (_cwFilterIndex + 1) % CwFilterLabels.Length;
        CwFilterLabel = CwFilterLabels[_cwFilterIndex];
        RefreshSpectrumFilterOverlay();
        ScheduleSaveClientSettings();
        if (CanOperate())
            _ = SendCwFilterAsync(_cwFilterIndex);
        else
            AppendLog($"CW filter → {CwFilterLabel} (not connected)");
    }

    private async Task SendFilterLowAsync(int hz)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetFilterLowAsync(hz).ConfigureAwait(true);
            AppendLog($"Sent Lo cut {hz} Hz ({LowCutLabel})");
        }
        catch (Exception ex) { AppendLog($"Lo cut error: {ex.Message}"); }
    }

    private async Task SendFilterHighAsync(int hz)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetFilterHighAsync(hz).ConfigureAwait(true);
            AppendLog($"Sent Hi cut {hz} Hz ({HighCutLabel})");
        }
        catch (Exception ex) { AppendLog($"Hi cut error: {ex.Message}"); }
    }

    private async Task SendCwFilterAsync(int index)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetCwFilterAsync(index).ConfigureAwait(true);
            AppendLog($"Sent CW filter index {index} ({CwFilterLabel})");
        }
        catch (Exception ex) { AppendLog($"CW filter error: {ex.Message}"); }
    }

    // ----- RX/TX tab default filters + TX BW -----

    partial void OnLowCutDefaultIndexChanged(int value)
    {
        if (_suppressDefaultFilterSend || !CanOperate() || _radio == null) return;
        value = Math.Clamp(value, 0, LowCutOptions.Count - 1);
        _ = SendDefaultFilterAsync(
            () => _radio.SetDefaultLowCutAsync(value),
            $"Default Lo cut index {value} ({LowCutOptions[value]})");
    }

    partial void OnHighCutDefaultIndexChanged(int value)
    {
        if (_suppressDefaultFilterSend || !CanOperate() || _radio == null) return;
        value = Math.Clamp(value, 0, HighCutOptions.Count - 1);
        _ = SendDefaultFilterAsync(
            () => _radio.SetDefaultHighCutAsync(value),
            $"Default Hi cut index {value} ({HighCutOptions[value]})");
    }

    partial void OnCwFilterDefaultIndexChanged(int value)
    {
        if (_suppressDefaultFilterSend || !CanOperate() || _radio == null) return;
        value = Math.Clamp(value, 0, CwFilterOptions.Count - 1);
        _ = SendDefaultFilterAsync(
            () => _radio.SetDefaultCwFilterAsync(value),
            $"Default CW filter index {value} ({CwFilterOptions[value]})");
    }

    partial void OnTxDefaultIndexChanged(int value)
    {
        if (_suppressDefaultFilterSend || !CanOperate() || _radio == null) return;
        value = Math.Clamp(value, 0, TxOptions.Count - 1);
        _ = SendDefaultFilterAsync(
            () => _radio.SetDefaultTxAsync(value),
            $"Default TX index {value} ({TxOptions[value]})");
    }

    partial void OnTxBandwidthIndexChanged(int value)
    {
        if (!CanOperate() || _radio == null) return;
        value = Math.Clamp(value, 0, TxOptions.Count - 1);
        _ = SendDefaultFilterAsync(
            () => _radio.SetTxBandwidthAsync(value),
            $"TX BW index {value} ({TxOptions[value]})");
    }

    private async Task SendDefaultFilterAsync(Func<Task> send, string okMsg)
    {
        try
        {
            await send().ConfigureAwait(true);
            AppendLog($"Sent {okMsg}");
        }
        catch (Exception ex)
        {
            AppendLog($"Filter default error: {ex.Message}");
        }
    }

    private void ApplyReportedDefaultIndex(Action apply)
    {
        _suppressDefaultFilterSend = true;
        try { apply(); }
        finally { _suppressDefaultFilterSend = false; }
    }

    private static bool TryParsePort(string? text, out int port)
    {
        port = 0;
        if (string.IsNullOrWhiteSpace(text))
            return false;
        if (!int.TryParse(text.Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out port))
            return false;
        return port is >= 1 and <= 65535;
    }

    // ----- AGC / AMP / CMP / MON / NB / NR / AN -----

    [RelayCommand(CanExecute = nameof(CanOperate))]
    private void CycleAgc() => AgcLevel = (AgcLevel + 1) % 3;

    [RelayCommand(CanExecute = nameof(CanOperate))]
    private void ToggleAmp() => AmpOn = !AmpOn;

    [RelayCommand(CanExecute = nameof(CanOperate))]
    private void ToggleCompression() => CompressionOn = !CompressionOn;

    [RelayCommand(CanExecute = nameof(CanOperate))]
    private void ToggleMonitor() => MonitorOn = !MonitorOn;

    [RelayCommand(CanExecute = nameof(CanOperate))]
    private void ToggleNb() => NbOn = !NbOn;

    [RelayCommand(CanExecute = nameof(CanOperate))]
    private void ToggleNr() => NrOn = !NrOn;

    [RelayCommand(CanExecute = nameof(CanOperate))]
    private void ToggleAn() => AnOn = !AnOn;

    partial void OnAgcLevelChanged(int value)
    {
        value = Math.Clamp(value, 0, 2);
        AgcButtonText = value switch
        {
            1 => "MED",
            2 => "FST",
            _ => "SLO"
        };
        ScheduleSaveClientSettings();
        if (_suppressAgcCommand || !CanOperate()) return;
        _ = SendAgcLevelAsync(value);
    }

    partial void OnAgcFastReleaseChanged(int value)
    {
        ScheduleSaveClientSettings();
        if (_suppressAgcCommand || !CanOperate()) return;
        _ = SendAgcFastReleaseAsync(Math.Clamp(value, 0, 1000));
    }

    partial void OnAmpOnChanged(bool value)
    {
        OnPropertyChanged(nameof(IsPowerCalTabEnabled));
        OnPropertyChanged(nameof(IsAmpCalTabEnabled));
        OnPropertyChanged(nameof(IsTxIqTabEnabled));
        OnPropertyChanged(nameof(PowerCalTabHint));
        OnPropertyChanged(nameof(AmpCalTabHint));
        OnPropertyChanged(nameof(TxIqTabHint));
        ScheduleSaveClientSettings();

        // Mutual exclusion with cal sessions
        if (value)
        {
            ForceStopPowerCal("AMP on");
            ForceStopTxIqSession("AMP on — TX IQ requires QRP");
        }
        else
            ForceStopAmpCal("AMP off");

        if (_suppressAmpCommand || !CanOperate()) return;
        _ = SendAmpAsync(value);
    }

    partial void OnCompressionOnChanged(bool value)
    {
        // Remember preferred CMP only while on phones (P); digital forces CMP off.
        if (!_suppressCompressionCommand && !IsDigitalAudio)
            _sessionCompressionOn = value;
        ScheduleSaveClientSettings();

        if (_suppressCompressionCommand || !CanOperate()) return;
        _ = SendCompressionStateAsync(value);
    }

    // ----- Audio levels + path -----

    partial void OnPVolumeChanged(int value)
    {
        ScheduleSaveClientSettings();
        if (_suppressAudioSend || !CanOperate()) return;
        _ = SendAudioAsync(
            () => _radio!.SetPhonesVolumeLevelAsync(Math.Clamp(value, 0, 100)),
            $"Phones Vol {value}");
    }

    partial void OnPMicGainChanged(int value)
    {
        ScheduleSaveClientSettings();
        if (_suppressAudioSend || !CanOperate()) return;
        _ = SendAudioAsync(
            () => _radio!.SetPhonesMicGainLevelAsync(Math.Clamp(value, 0, 100)),
            $"Phones Mic {value}");
    }

    partial void OnDVolumeChanged(int value)
    {
        ScheduleSaveClientSettings();
        if (_suppressAudioSend || !CanOperate()) return;
        _ = SendAudioAsync(
            () => _radio!.SetDigitalVolumeLevelAsync(Math.Clamp(value, 0, 100)),
            $"Digital Vol {value}");
    }

    partial void OnDMicGainChanged(int value)
    {
        ScheduleSaveClientSettings();
        if (_suppressAudioSend || !CanOperate()) return;
        _ = SendAudioAsync(
            () => _radio!.SetDigitalMicGainLevelAsync(Math.Clamp(value, 0, 100)),
            $"Digital Mic {value}");
    }

    private byte ResolveAudioDeviceOpcode()
    {
        if (IsDigitalAudio)
            return Opcodes.DIGITAL_SOUND_DEVICE;
        if (RemoteAudio)
            return Opcodes.REMOTE_SOUND_DEVICE;
        return Opcodes.PHONES_SOUND_DEVICE;
    }

    partial void OnIsDigitalAudioChanged(bool value)
    {
        OnPropertyChanged(nameof(AudioPathButtonText));
        OnPropertyChanged(nameof(RemoteAudioCheckboxEnabled));
        ScheduleSaveClientSettings();

        if (_suppressAudioSend) return;

        byte device = ResolveAudioDeviceOpcode();
        string label = device switch
        {
            Opcodes.DIGITAL_SOUND_DEVICE => "Digital (0)",
            Opcodes.REMOTE_SOUND_DEVICE => "Remote (2)",
            _ => "Phones (1)",
        };

        if (CanOperate() && _radio != null)
        {
            _ = SendAudioAsync(
                () => _radio.SetAudioDeviceAsync(device),
                $"Audio device → {label}");
        }
        else
        {
            AppendLog($"Audio device → {label} (not connected)");
        }

        // Match WPF: P→D forces CMP off; D→P restores session preferred CMP.
        if (value)
        {
            if (CompressionOn)
            {
                CompressionOn = false;
                AppendLog("CMP forced OFF for digital audio (D)");
            }
        }
        else if (CanOperate())
        {
            if (CompressionOn != _sessionCompressionOn)
                CompressionOn = _sessionCompressionOn;
            else if (!_suppressCompressionCommand)
                _ = SendCompressionStateAsync(_sessionCompressionOn);
            AppendLog($"CMP restored for phones (P): {_sessionCompressionOn}");
        }
    }

    partial void OnRemoteAudioChanged(bool value)
    {
        ScheduleSaveClientSettings();
        if (_suppressAudioSend) return;

        if (IsDigitalAudio)
        {
            AppendLog("Remote Audio sticky saved (inactive while Audio=Digital)");
            return;
        }

        byte device = ResolveAudioDeviceOpcode();
        string label = device == Opcodes.REMOTE_SOUND_DEVICE ? "Remote (2)" : "Phones (1)";
        if (CanOperate() && _radio != null)
        {
            _ = SendAudioAsync(
                () => _radio.SetAudioDeviceAsync(device),
                $"Audio device → {label}");
        }
        else
        {
            AppendLog($"Audio device → {label} (not connected)");
        }
    }

    [RelayCommand]
    private void ToggleAudioDigital() => IsDigitalAudio = !IsDigitalAudio;

    private async Task SendAudioAsync(Func<Task> send, string okMsg)
    {
        if (_radio == null) return;
        try
        {
            await send().ConfigureAwait(true);
            AppendLog($"Sent {okMsg}");
        }
        catch (Exception ex)
        {
            AppendLog($"Audio error: {ex.Message}");
        }
    }

    private void ApplyReportedAudio(Action apply)
    {
        _suppressAudioSend = true;
        try { apply(); }
        finally { _suppressAudioSend = false; }
    }

    // ----- RIT (left rail) -----

    partial void OnRitOnChanged(bool value)
    {
        ScheduleSaveClientSettings();
        if (_suppressRitSend) return;
        if (CanOperate())
            _ = SendRitAsync();
        else
            AppendLog($"RIT {(value ? "on" : "off")} (not connected)");
    }

    partial void OnRitOffsetChanged(int value)
    {
        if (_suppressRitSend) return;
        // Clamp to slider range used in UI
        int clamped = Math.Clamp(value, -500, 500);
        if (clamped != value)
        {
            _suppressRitSend = true;
            RitOffset = clamped;
            _suppressRitSend = false;
        }

        ScheduleSaveClientSettings();
        if (CanOperate())
            _ = SendRitAsync();
    }

    [RelayCommand]
    private void ClearRit()
    {
        _suppressRitSend = true;
        RitOn = false;
        RitOffset = 0;
        _suppressRitSend = false;
        if (CanOperate())
            _ = SendRitAsync();
        AppendLog("RIT cleared");
    }

    private async Task SendRitAsync()
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetRitAsync(RitOn, RitOffset).ConfigureAwait(true);
            AppendLog($"Sent RIT {(RitOn ? "on" : "off")} offset {RitOffset} Hz");
        }
        catch (Exception ex)
        {
            AppendLog($"RIT error: {ex.Message}");
        }
    }

    // ----- CW tab + right-rail speed/pitch -----

    [RelayCommand]
    private void IncCwSpeed() => CwSpeed = Math.Clamp(CwSpeed + 1, 5, 60);

    [RelayCommand]
    private void DecCwSpeed() => CwSpeed = Math.Clamp(CwSpeed - 1, 5, 60);

    [RelayCommand]
    private void IncCwHold() => CwHold = Math.Clamp(CwHold + 10, 1, 500);

    [RelayCommand]
    private void DecCwHold() => CwHold = Math.Clamp(CwHold - 10, 1, 500);

    [RelayCommand]
    private void CycleCwPitch()
    {
        CwPitchIndex = (CwPitchIndex + 1) % CwPitchOptions.Count;
    }

    /// <summary>
    /// Avalonia is connect-only (does not spawn backends). "Local host" means loopback —
    /// we still write this machine's mscc.ini, but the operator must restart ms-sdr here.
    /// </summary>
    private bool IsLoopbackHost
    {
        get
        {
            string h = (Host ?? "").Trim();
            if (string.IsNullOrEmpty(h)) return true;
            return h.Equals("127.0.0.1", StringComparison.OrdinalIgnoreCase)
                || h.Equals("localhost", StringComparison.OrdinalIgnoreCase)
                || h.Equals("::1", StringComparison.OrdinalIgnoreCase);
        }
    }

    partial void OnExternalElectronicKeyerChanged(bool value)
    {
        if (_suppressCwSend || _suppressSettingsSave) return;
        ScheduleSaveClientSettings();
        OnPropertyChanged(nameof(PicKeyerControlsEnabled));
        OnPropertyChanged(nameof(KeyerMemPanelEnabled));

        // Write mscc.ini on this machine when Host is loopback (UI + ms-sdr co-located).
        bool wrote = false;
        if (IsLoopbackHost)
            wrote = MsccIniProficio.WriteProficioMkii(mkii: !value);

        string mode = value ? "legacy / external (PROFICIO-MKII=0)" : "MKII internal (PROFICIO-MKII=1)";
        AppendLog($"External electronic keyer: {(value ? "ON" : "OFF")} → {mode}" +
                  (wrote ? " (local mscc.ini updated)" : " (client sticky; host mscc.ini when remote)"));

        if (!IsConnected)
        {
            if (!IsLoopbackHost)
            {
                AppendLog(
                    "Remote host: set PROFICIO-MKII on radio PC (Windows: Start-MsccServers.bat legacy|mkii; " +
                    "Linux: mscc-init) before Connect.");
            }
            else
            {
                AppendLog("Local host: restart ms-sdr after this change so PROFICIO-MKII is re-read.");
            }
            return;
        }

        // Connect-only: disconnect; never auto-spawn backends.
        string hostMode = value ? "legacy (PROFICIO-MKII=0)" : "MKII (PROFICIO-MKII=1)";
        if (IsLoopbackHost)
        {
            AppendLog(
                $"Session will disconnect. Restart local backends for {hostMode}, then Connect " +
                "(Windows: Start-MsccServers.bat legacy|mkii or restart services).");
            StatusText = "Disconnected — restart local ms-sdr, then Connect";
        }
        else
        {
            AppendLog(
                $"Session will disconnect. On radio PC set {hostMode} and restart ms-sdr, then Connect. " +
                "Windows: Start-MsccServers.bat legacy|mkii. Linux: mscc-init. " +
                "If host already matches, just Connect again.");
            StatusText = "Disconnected — restart host backends if needed, then Connect";
        }
        Disconnect();
    }

    partial void OnCwSpeedChanged(int value)
    {
        int wpm = Math.Clamp(value, 5, 60);
        if (wpm != value)
        {
            _suppressCwSend = true;
            CwSpeed = wpm;
            _suppressCwSend = false;
            return;
        }

        ScheduleSaveClientSettings();
        if (ExternalElectronicKeyer) return;
        if (_suppressCwSend || !CanOperate() || _radio == null) return;
        _ = SendCwAsync(() => _radio.SetCwWpmAsync(wpm), $"CW speed {wpm} WPM");
    }

    partial void OnCwKeyerModeChanged(int value)
    {
        ScheduleSaveClientSettings();
        if (ExternalElectronicKeyer) return;
        if (_suppressCwSend || !CanOperate() || _radio == null) return;
        value = Math.Clamp(value, 0, CwKeyerModeOptions.Count - 1);
        _ = SendCwAsync(() => _radio.SetCwKeyerModeAsync(value),
            $"CW keyer {CwKeyerModeOptions[value]}");
    }

    partial void OnCwSpacingChanged(int value)
    {
        ScheduleSaveClientSettings();
        if (ExternalElectronicKeyer) return;
        if (_suppressCwSend || !CanOperate() || _radio == null) return;
        value = Math.Clamp(value, 0, CwSpacingOptions.Count - 1);
        _ = SendCwAsync(() => _radio.SetCwSpacingAsync(value),
            $"CW spacing {CwSpacingOptions[value]}");
    }

    partial void OnCwPaddleChanged(int value)
    {
        ScheduleSaveClientSettings();
        if (ExternalElectronicKeyer) return;
        if (_suppressCwSend || !CanOperate() || _radio == null) return;
        value = Math.Clamp(value, 0, CwPaddleOptions.Count - 1);
        _ = SendCwAsync(() => _radio.SetCwPaddleAsync(value),
            $"CW paddle {CwPaddleOptions[value]}");
    }

    partial void OnCwWeightIndexChanged(int value)
    {
        ScheduleSaveClientSettings();
        if (ExternalElectronicKeyer) return;
        if (_suppressCwSend || !CanOperate() || _radio == null) return;
        value = Math.Clamp(value, 0, CwWeightValues.Length - 1);
        int weight = CwWeightValues[value];
        _ = SendCwAsync(() => _radio.SetCwWeightAsync(weight), $"CW weight {weight}");
    }

    partial void OnCwPitchIndexChanged(int value)
    {
        value = Math.Clamp(value, 0, CwPitchOptions.Count - 1);
        CwPitchLabel = CwPitchOptions[value];
        RefreshSpectrumFilterOverlay();
        ScheduleSaveClientSettings();

        if (_suppressCwSend || !CanOperate() || _radio == null) return;
        // WPF: send CW filter BW first, then pitch INDEX (0–3), not Hz
        _ = SendCwPitchAsync(value);
    }

    partial void OnCwHoldChanged(int value)
    {
        int hold = Math.Clamp(value, 1, 500);
        if (hold != value)
        {
            _suppressCwSend = true;
            CwHold = hold;
            _suppressCwSend = false;
            return;
        }

        ScheduleSaveClientSettings();
        if (_suppressCwSend || !CanOperate() || _radio == null) return;
        _ = SendCwAsync(() => _radio.SetCwTxHoldAsync(hold), $"CW hold {hold} ms");
    }

    partial void OnCwQskChanged(bool value)
    {
        ScheduleSaveClientSettings();
        if (_suppressCwSend || !CanOperate() || _radio == null) return;
        _ = SendCwAsync(() => _radio.SetCwQskAsync(value), $"CW QSK {(value ? "on" : "off")}");
    }

    partial void OnCwPhonesChanged(bool value)
    {
        ScheduleSaveClientSettings();
        if (_suppressCwSend || !CanOperate() || _radio == null) return;
        _ = SendCwAsync(() => _radio.SetCwPhonesAsync(value), $"CW phones {(value ? "on" : "off")}");
    }

    // ----- Keyer CQ memory (R = store, P = play) -----

    public string KeyerMem0Count => $"{SanitizeKeyerMem(KeyerMem0).Length}/48";
    public string KeyerMem1Count => $"{SanitizeKeyerMem(KeyerMem1).Length}/48";
    public string KeyerMem2Count => $"{SanitizeKeyerMem(KeyerMem2).Length}/48";
    public string KeyerMem3Count => $"{SanitizeKeyerMem(KeyerMem3).Length}/48";

    partial void OnKeyerMem0Changed(string value)
    {
        OnPropertyChanged(nameof(KeyerMem0Count));
        if (!_suppressCwSend) ScheduleSaveClientSettings();
    }

    partial void OnKeyerMem1Changed(string value)
    {
        OnPropertyChanged(nameof(KeyerMem1Count));
        if (!_suppressCwSend) ScheduleSaveClientSettings();
    }

    partial void OnKeyerMem2Changed(string value)
    {
        OnPropertyChanged(nameof(KeyerMem2Count));
        if (!_suppressCwSend) ScheduleSaveClientSettings();
    }

    partial void OnKeyerMem3Changed(string value)
    {
        OnPropertyChanged(nameof(KeyerMem3Count));
        if (!_suppressCwSend) ScheduleSaveClientSettings();
    }

    private static string SanitizeKeyerMem(string? text)
    {
        if (string.IsNullOrEmpty(text)) return "";
        var sb = new System.Text.StringBuilder(Math.Min(text.Length, Opcodes.KEYER_MEM_MAX_CHARS));
        foreach (char c in text)
        {
            if (c is < (char)0x20 or > (char)0x7E) continue;
            sb.Append(c);
            if (sb.Length >= Opcodes.KEYER_MEM_MAX_CHARS) break;
        }
        return sb.ToString();
    }

    private string GetKeyerMemText(int slot) => slot switch
    {
        0 => KeyerMem0,
        1 => KeyerMem1,
        2 => KeyerMem2,
        3 => KeyerMem3,
        _ => ""
    };

    /// <summary>R — store text box for slot 0..3 to keyer EEPROM (no auto-play).</summary>
    [RelayCommand]
    private async Task RecordKeyerMem(object? parameter)
    {
        if (!TryParseKeyerSlot(parameter, out int slot)) return;
        if (ExternalElectronicKeyer)
        {
            KeyerMemStatus = "Disabled — external electronic keyer (legacy)";
            return;
        }
        if (!CanOperate() || _radio == null)
        {
            KeyerMemStatus = "Not connected";
            AppendLog("Keyer mem R: not connected");
            return;
        }
        if (KeyerMemBusy) return;

        string text = SanitizeKeyerMem(GetKeyerMemText(slot));
        KeyerMemBusy = true;
        KeyerMemStatus = $"Storing slot {slot}…";
        AppendLog($"Keyer mem R slot {slot}: \"{text}\" ({text.Length} chars)");
        try
        {
            await _radio.KeyerMemoryStoreAsync(slot, text).ConfigureAwait(true);
            ScheduleSaveClientSettings();
            KeyerMemStatus = $"Stored slot {slot} ({text.Length} chars)";
            AppendLog($"Keyer mem R slot {slot}: store sequence sent OK");
        }
        catch (Exception ex)
        {
            KeyerMemStatus = $"Store slot {slot} failed";
            AppendLog($"Keyer mem R error: {ex.Message}");
        }
        finally
        {
            KeyerMemBusy = false;
        }
    }

    /// <summary>P — assert host PTT (if free), select slot, play once. Paddle aborts on radio.</summary>
    [RelayCommand]
    private async Task PlayKeyerMem(object? parameter)
    {
        if (!TryParseKeyerSlot(parameter, out int slot)) return;
        if (ExternalElectronicKeyer)
        {
            KeyerMemStatus = "Disabled — external electronic keyer (legacy)";
            return;
        }
        if (!CanOperate() || _radio == null)
        {
            KeyerMemStatus = "Not connected";
            AppendLog("Keyer mem P: not connected");
            return;
        }
        if (KeyerMemBusy) return;

        // Cancel any previous auto-PTT release from an earlier play
        CancelKeyerPlayPttRelease(releasePtt: false);

        string text = SanitizeKeyerMem(GetKeyerMemText(slot));
        bool assertedPtt = false;
        // Proficio only runs TX_Main (host TX_Request / software PTT → PA) when mode is NOT CW.
        // In CW, RF is keyed only by the PIC keyer line (paddle / memory play). Host PTT would
        // only light the UI red and confuse operators — skip it when already in CW.
        bool modeIsCw = string.Equals((ModeText ?? "").Trim(), "CW", StringComparison.OrdinalIgnoreCase);

        if (TxSetByServer)
        {
            AppendLog("Keyer mem P: server owns TX — not asserting PTT");
        }
        else if (TuneMode)
        {
            AppendLog("Keyer mem P: releasing TUN before play");
            TuneMode = false;
        }

        if (modeIsCw)
        {
            AppendLog("Keyer mem P: CW mode — host PTT does not key PA; play uses keyer line");
            _keyerPlayOwnsPtt = false;
        }
        else if (!TxSetByServer && CanUserControlTransmit)
        {
            // Non-CW: assert host PTT so voice-path TX_Request is set (same as PTT button).
            if (!PttOn)
            {
                try
                {
                    _suppressTransmitCommands = true;
                    PttOn = true;
                    _suppressTransmitCommands = false;
                    await SendPttAsync(true).ConfigureAwait(true);
                    assertedPtt = true;
                    _keyerPlayOwnsPtt = true;
                    AppendLog("Keyer mem P: PTT ON for memory play (non-CW mode)");
                }
                catch (Exception ex)
                {
                    AppendLog($"Keyer mem P: PTT on failed: {ex.Message}");
                }
            }
            else
            {
                _keyerPlayOwnsPtt = false;
                AppendLog("Keyer mem P: PTT already on (user latch)");
            }
        }

        KeyerMemStatus = modeIsCw
            ? $"Play slot {slot} (CW — keyer keys TX)…"
            : assertedPtt
                ? $"PTT on — play slot {slot}…"
                : $"Play slot {slot}…";
        AppendLog($"Keyer mem P slot {slot}: SELECT + PLAY (0x9C)");
        try
        {
            await _radio.KeyerMemoryPlayAsync(slot).ConfigureAwait(true);
            int playMs = EstimateKeyerPlayDurationMs(text, CwSpeed);
            KeyerMemStatus = modeIsCw
                ? $"Play sent slot {slot} — listen for keyer CW (paddle aborts)"
                : assertedPtt
                    ? $"Playing slot {slot} (~{playMs / 1000.0:0.0}s) — PTT auto-off"
                    : $"Play sent for slot {slot}";
            AppendLog($"Keyer mem P slot {slot}: play sequence sent OK (est. {playMs} ms)");

            if (_keyerPlayOwnsPtt)
                ScheduleKeyerPlayPttRelease(playMs);
        }
        catch (Exception ex)
        {
            KeyerMemStatus = $"Play slot {slot} failed";
            AppendLog($"Keyer mem P error: {ex.Message}");
            if (_keyerPlayOwnsPtt)
                await ReleaseKeyerPlayPttAsync().ConfigureAwait(true);
        }
    }

    /// <summary>
    /// Rough PARIS-style duration so PTT can drop after memory play finishes.
    /// ~12 element units per character, 7 for space; unit = 1200/WPM ms. +0.5s pad.
    /// </summary>
    private static int EstimateKeyerPlayDurationMs(string text, int wpm)
    {
        wpm = Math.Clamp(wpm, 5, 60);
        int units = 0;
        foreach (char c in text)
            units += c is ' ' or '\t' ? 7 : 12;
        if (units < 12) units = 12; // empty / short — still leave a second of TX
        int ms = units * 1200 / wpm + 500;
        return Math.Clamp(ms, 1000, 180_000);
    }

    private void CancelKeyerPlayPttRelease(bool releasePtt)
    {
        try { _keyerPlayPttReleaseCts?.Cancel(); } catch { /* ignore */ }
        try { _keyerPlayPttReleaseCts?.Dispose(); } catch { /* ignore */ }
        _keyerPlayPttReleaseCts = null;
        if (releasePtt && _keyerPlayOwnsPtt)
            _ = ReleaseKeyerPlayPttAsync();
    }

    private void ScheduleKeyerPlayPttRelease(int delayMs)
    {
        CancelKeyerPlayPttRelease(releasePtt: false);
        var cts = new CancellationTokenSource();
        _keyerPlayPttReleaseCts = cts;
        _ = ReleaseKeyerPlayPttAfterDelayAsync(delayMs, cts.Token);
    }

    private async Task ReleaseKeyerPlayPttAfterDelayAsync(int delayMs, CancellationToken ct)
    {
        try
        {
            await Task.Delay(delayMs, ct).ConfigureAwait(true);
        }
        catch (OperationCanceledException)
        {
            return;
        }

        if (ct.IsCancellationRequested || !_keyerPlayOwnsPtt) return;
        await ReleaseKeyerPlayPttAsync().ConfigureAwait(true);
    }

    private async Task ReleaseKeyerPlayPttAsync()
    {
        if (!_keyerPlayOwnsPtt) return;
        _keyerPlayOwnsPtt = false;
        if (!PttOn || TxSetByServer || _radio == null) return;
        try
        {
            _suppressTransmitCommands = true;
            PttOn = false;
            _suppressTransmitCommands = false;
            await SendPttAsync(false).ConfigureAwait(true);
            AppendLog("Keyer mem P: PTT OFF (play complete estimate)");
            if (KeyerMemStatus.StartsWith("Playing", StringComparison.OrdinalIgnoreCase)
                || KeyerMemStatus.Contains("auto-off", StringComparison.OrdinalIgnoreCase))
                KeyerMemStatus = "Play done — PTT released";
        }
        catch (Exception ex)
        {
            AppendLog($"Keyer mem P: PTT off failed: {ex.Message}");
        }
    }

    private static bool TryParseKeyerSlot(object? parameter, out int slot)
    {
        slot = 0;
        switch (parameter)
        {
            case int i:
                slot = i;
                break;
            case string s when int.TryParse(s, NumberStyles.Integer, CultureInfo.InvariantCulture, out int parsed):
                slot = parsed;
                break;
            default:
                return false;
        }
        return slot is >= 0 and <= 3;
    }

    private async Task SendCwPitchAsync(int pitchIndex)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetCwFilterAsync(_cwFilterIndex).ConfigureAwait(true);
            await _radio.SetCwPitchAsync(pitchIndex).ConfigureAwait(true);
            AppendLog($"Sent CW pitch {CwPitchLabel} (index {pitchIndex})");
        }
        catch (Exception ex)
        {
            AppendLog($"CW pitch error: {ex.Message}");
        }
    }

    private async Task SendCwAsync(Func<Task> send, string okMsg)
    {
        if (_radio == null) return;
        try
        {
            await send().ConfigureAwait(true);
            AppendLog($"Sent {okMsg}");
        }
        catch (Exception ex)
        {
            AppendLog($"CW error: {ex.Message}");
        }
    }

    private void ApplyReportedCw(Action apply)
    {
        _suppressCwSend = true;
        try { apply(); }
        finally { _suppressCwSend = false; }
    }

    // ----- Favorites tab (client-side session memory) -----

    partial void OnFavoriteBandFilterChanged(string value) => RefreshFavoritesForBand();

    partial void OnSelectedFavoriteChanged(FavoriteEntry? value)
    {
        if (value != null && !string.IsNullOrWhiteSpace(value.Name))
            FavoriteNameInput = value.Name;
    }

    private void ApplyFavoriteLabels(FavoriteEntry e)
    {
        e.LowCutLabel = IndexLabel(LowCutLabels, e.LowCutIndex);
        e.HighCutLabel = IndexLabel(HighCutLabels, e.HighCutIndex);
        e.CwFilterLabel = IndexLabel(CwFilterLabels, e.CwFilterIndex);
    }

    private static string IndexLabel(string[] options, int index)
    {
        if (options.Length == 0) return index.ToString(CultureInfo.InvariantCulture);
        return options[Math.Clamp(index, 0, options.Length - 1)];
    }

    private static string NormalizeFavoriteBand(string? band, long frequencyHz = 0)
    {
        string b = (band ?? "").Trim().ToLowerInvariant();
        if (b is "gen" or "general") return "gen";
        if (!string.IsNullOrEmpty(b) && b is not ("?" or "—" or "-"))
            return b;
        string fromFreq = BandNameForFrequency(frequencyHz);
        return fromFreq is "?" or "—" ? "40m" : fromFreq;
    }

    private void RefreshFavoritesForBand()
    {
        string band = NormalizeFavoriteBand(FavoriteBandFilter);
        var selected = SelectedFavorite;
        var ordered = Favorites
            .Where(f => string.Equals(NormalizeFavoriteBand(f.Band, f.FrequencyHz), band, StringComparison.OrdinalIgnoreCase))
            .OrderBy(f => f.Name ?? "", StringComparer.OrdinalIgnoreCase)
            .ThenBy(f => f.FrequencyHz)
            .ToList();

        FavoritesForBand.Clear();
        foreach (var e in ordered)
            FavoritesForBand.Add(e);

        if (selected != null && FavoritesForBand.Contains(selected))
            SelectedFavorite = selected;
        else
            SelectedFavorite = null;
    }

    private void SyncFavoriteBandFilterFromRadio()
    {
        string nb = NormalizeFavoriteBand(BandText, _frequencyHz);
        if (FavoriteBandChoices.Any(b => string.Equals(b, nb, StringComparison.OrdinalIgnoreCase)) &&
            !string.Equals(FavoriteBandFilter, nb, StringComparison.OrdinalIgnoreCase))
        {
            FavoriteBandFilter = nb;
        }
    }

    /// <summary>Apply Lo/Hi/CW indices locally and send when connected.</summary>
    private void ApplyFilterIndices(int lowIdx, int highIdx, int cwIdx, bool send)
    {
        _lowCutIndex = Math.Clamp(lowIdx, 0, LowCutLabels.Length - 1);
        _highCutIndex = Math.Clamp(highIdx, 0, HighCutLabels.Length - 1);
        _cwFilterIndex = Math.Clamp(cwIdx, 0, CwFilterLabels.Length - 1);
        LowCutLabel = LowCutLabels[_lowCutIndex];
        HighCutLabel = HighCutLabels[_highCutIndex];
        CwFilterLabel = CwFilterLabels[_cwFilterIndex];
        RefreshSpectrumFilterOverlay();

        if (!send || !CanOperate()) return;
        _ = SendFilterLowAsync(LowCutHzValues[_lowCutIndex]);
        _ = SendFilterHighAsync(HighCutHzValues[_highCutIndex]);
        _ = SendCwFilterAsync(_cwFilterIndex);
    }

    [RelayCommand]
    private void SaveFavorite()
    {
        string name = (FavoriteNameInput ?? "").Trim();
        if (string.IsNullOrEmpty(name) ||
            string.Equals(name, "NAME", StringComparison.OrdinalIgnoreCase))
        {
            StatusText = "Enter a favorite name.";
            AppendLog("Favorite SAVE: name required");
            return;
        }

        if (name.Length > 32)
            name = name[..32];

        string band = NormalizeFavoriteBand(BandText, _frequencyHz);
        var entry = new FavoriteEntry
        {
            Name = name,
            Band = band,
            FrequencyHz = _frequencyHz,
            Mode = string.IsNullOrWhiteSpace(ModeText) ? "USB" : ModeText,
            LowCutIndex = _lowCutIndex,
            HighCutIndex = _highCutIndex,
            CwFilterIndex = _cwFilterIndex,
            Vfo = UseVfoA ? "A" : "B"
        };
        ApplyFavoriteLabels(entry);

        var existing = Favorites.FirstOrDefault(f =>
            string.Equals(NormalizeFavoriteBand(f.Band, f.FrequencyHz), band, StringComparison.OrdinalIgnoreCase) &&
            string.Equals(f.Name, name, StringComparison.OrdinalIgnoreCase));

        if (existing != null)
        {
            int idx = Favorites.IndexOf(existing);
            Favorites[idx] = entry;
            AppendLog($"Favorite updated: [{band}] {name}");
        }
        else
        {
            Favorites.Add(entry);
            AppendLog($"Favorite added: [{band}] {name} @ {entry.FrequencyDisplay} MHz {entry.Mode}");
        }

        FavoriteBandFilter = band;
        RefreshFavoritesForBand();
        SelectedFavorite = entry;
        PersistFavorites();
        StatusText = $"Saved favorite [{band}] {name}";
    }

    [RelayCommand]
    private async Task RecallFavoriteAsync()
    {
        if (SelectedFavorite == null)
        {
            StatusText = "Select a favorite to recall.";
            AppendLog("Favorite RECALL: nothing selected");
            return;
        }

        var fav = SelectedFavorite;
        string band = NormalizeFavoriteBand(fav.Band, fav.FrequencyHz);

        // Local UI first
        _frequencyHz = fav.FrequencyHz;
        UpdateFrequencyUi(fav.FrequencyHz);
        ModeText = fav.Mode;
        NotifyModeFlags();
        BandText = band is "—" or "?" ? BandNameForFrequency(fav.FrequencyHz) : band;
        ApplyFilterIndices(fav.LowCutIndex, fav.HighCutIndex, fav.CwFilterIndex, send: CanOperate());
        FavoriteNameInput = fav.Name;
        FavoriteBandFilter = NormalizeFavoriteBand(BandText, fav.FrequencyHz);

        if (string.Equals(fav.Vfo, "B", StringComparison.OrdinalIgnoreCase) && UseVfoA)
            AppendLog("Favorite VFO B noted — dual-VFO switch not wired yet; applying on VFO A");

        if (CanOperate())
        {
            await ApplyFrequencyAsync(fav.FrequencyHz, $"fav {fav.Name}").ConfigureAwait(true);
            await SetModeAsync(fav.Mode).ConfigureAwait(true);
            // Filters already sent by ApplyFilterIndices
        }
        else
        {
            AppendLog($"Favorite recalled locally (not connected): [{band}] {fav.Name}");
        }

        StatusText = $"Recalled [{band}] {fav.Name}";
        AppendLog($"Favorite recalled: [{band}] {fav.Name} VFO{fav.Vfo} {fav.FrequencyDisplay} {fav.Mode}");
    }

    [RelayCommand]
    private void DeleteFavorite()
    {
        if (SelectedFavorite == null)
        {
            StatusText = "Select a favorite to delete.";
            AppendLog("Favorite DELETE: nothing selected");
            return;
        }

        var fav = SelectedFavorite;
        string band = NormalizeFavoriteBand(fav.Band, fav.FrequencyHz);
        Favorites.Remove(fav);
        SelectedFavorite = null;
        RefreshFavoritesForBand();
        PersistFavorites();
        StatusText = $"Deleted [{band}] {fav.Name}";
        AppendLog($"Favorite deleted: [{band}] {fav.Name}");
    }

    // ----- QRP CAL + AMP CAL -----

    private void InitPowerCalBandStatuses()
    {
        PowerCalBandStatuses.Clear();
        foreach (int n in CalBandNumbers)
        {
            PowerCalBandStatuses.Add(new PowerCalBandItem
            {
                BandNumber = n,
                BandLabel = n.ToString(CultureInfo.InvariantCulture),
                IsCalibrated = false,
                IsSelected = false
            });
        }

        PowerCalSelectedBand = 0;
        PowerCalSliderValue = 0;
        PowerCalStepLabel = "CALIBRATION STEP: —";
        PowerCalTxOn = false;
        PowerCalCalibrating = false;
        PowerCalAcceptPrompt = false;
    }

    private void InitAmpCalBandStatuses()
    {
        AmpCalBandStatuses.Clear();
        foreach (int n in CalBandNumbers)
        {
            AmpCalBandStatuses.Add(new PowerCalBandItem
            {
                BandNumber = n,
                BandLabel = n.ToString(CultureInfo.InvariantCulture),
                IsCalibrated = false,
                IsSelected = false
            });
        }

        AmpCalSelectedBand = 0;
        AmpCalSliderValue = -99;
        AmpCalStepLabel = "STEP: 0";
        AmpCalTxOn = false;
        AmpCalCalibrating = false;
        AmpCalAcceptPrompt = false;
    }

    private static long GetCalFrequencyHz(int bandNumber) => bandNumber switch
    {
        2200 => 135_750,
        630 => 475_000,
        160 => 1_810_000,
        80 => 3_510_000,
        60 => 5_330_500,
        40 => 7_010_000,
        30 => 10_110_000,
        20 => 14_150_000,
        17 => 18_110_000,
        15 => 21_200_000,
        12 => 24_900_000,
        10 => 28_010_000,
        _ => 0
    };

    private static int AmpCalStepFromSlider(int sliderValue) => 100 + Math.Clamp(sliderValue, -99, 0);

    partial void OnPowerCalTxOnChanged(bool value)
    {
        OnPropertyChanged(nameof(PowerCalTxButtonText));
        OnPropertyChanged(nameof(PowerCalTxButtonEnabled));
    }

    partial void OnPowerCalCalibratingChanged(bool value)
    {
        OnPropertyChanged(nameof(PowerCalCalibrateButtonText));
        OnPropertyChanged(nameof(PowerCalTxButtonEnabled));
    }

    partial void OnAmpCalTxOnChanged(bool value)
    {
        OnPropertyChanged(nameof(AmpCalTxButtonText));
        OnPropertyChanged(nameof(AmpCalTxButtonEnabled));
    }

    partial void OnAmpCalCalibratingChanged(bool value)
    {
        OnPropertyChanged(nameof(AmpCalCalibrateButtonText));
        OnPropertyChanged(nameof(AmpCalTxButtonEnabled));
    }

    private void SetPowerCalBandCalibrated(int bandNumber, bool calibrated)
    {
        var item = PowerCalBandStatuses.FirstOrDefault(b => b.BandNumber == bandNumber);
        if (item == null) return;
        item.IsCalibrated = calibrated;
        AppendLog($"QRP cal status: {bandNumber}m → {(calibrated ? "calibrated" : "not calibrated")}");
    }

    private void SetAmpCalBandCalibrated(int bandNumber, bool calibrated)
    {
        var item = AmpCalBandStatuses.FirstOrDefault(b => b.BandNumber == bandNumber);
        if (item == null) return;
        item.IsCalibrated = calibrated;
        AppendLog($"AMP cal status: {bandNumber}m → {(calibrated ? "calibrated" : "not calibrated")}");
    }

    [RelayCommand]
    private async Task SelectPowerCalBandAsync(int band)
    {
        if (band <= 0) return;
        if (PowerCalTxOn || PowerCalCalibrating)
        {
            StatusText = "TX ON — set TX off before band change";
            AppendLog("QRP cal: band change blocked (TX on)");
            return;
        }

        if (PowerCalSelectedBand == band) return;

        PowerCalSelectedBand = band;
        foreach (var item in PowerCalBandStatuses)
            item.IsSelected = item.BandNumber == band;

        long calFreq = GetCalFrequencyHz(band);
        if (calFreq > 0 && CanOperate())
            await ApplyFrequencyAsync(calFreq, $"qrp-cal {band}m").ConfigureAwait(true);
        else if (calFreq > 0)
        {
            _frequencyHz = calFreq;
            UpdateFrequencyUi(calFreq);
            BandText = band + "m";
        }

        if (CanOperate() && _radio != null)
        {
            try
            {
                await _radio.SetBandPowerBandAsync(band).ConfigureAwait(true);
                AppendLog($"QRP cal band {band}m → 0xA1");
            }
            catch (Exception ex)
            {
                AppendLog($"QRP cal band error: {ex.Message}");
            }
        }
        else
            AppendLog($"QRP cal band {band}m selected (not connected)");
    }

    partial void OnPowerCalSliderValueChanged(int value)
    {
        value = Math.Clamp(value, 0, 100);
        if (!_suppressPowerCalSlider)
            PowerCalStepLabel = $"CALIBRATION STEP: {value}";

        if (_suppressPowerCalSlider || !PowerCalCalibrating || !CanOperate() || _radio == null)
            return;

        _ = SendCalAsync(
            () => _radio.SetBandPowerPowerAsync(value),
            $"QRP cal power 0xA2={value}");
    }

    private void ApplyBandPowerReport(int step)
    {
        step = Math.Clamp(step, 0, 100);
        _powerCalPreviousReceivedStep = step;
        if (PowerCalCalibrating)
        {
            PowerCalStepLabel = $"CALIBRATION STEP: {PowerCalSliderValue}";
            AppendLog($"QRP cal step 0xB4={step} (ignored while calibrating)");
            return;
        }

        _suppressPowerCalSlider = true;
        try
        {
            PowerCalSliderValue = step;
            PowerCalStepLabel = $"CALIBRATION STEP: {step}";
        }
        finally
        {
            _suppressPowerCalSlider = false;
        }
        AppendLog($"QRP cal step from server 0xB4={step}");
    }

    [RelayCommand]
    private async Task TogglePowerCalTxAsync()
    {
        if (PowerCalCalibrating)
        {
            StatusText = "Finish CALIBRATE first";
            return;
        }

        if (PowerCalSelectedBand <= 0)
        {
            StatusText = "Select a band for QRP CAL";
            return;
        }

        if (!PowerCalTxOn && !PowerCalLoadConfirmed)
        {
            StatusText = "Confirm dummy load / antenna first";
            return;
        }

        if (!CanOperate() || _radio == null)
        {
            StatusText = "Connect first";
            return;
        }

        bool turnOn = !PowerCalTxOn;
        try
        {
            await _radio.SetCalibrationTuneAsync(turnOn).ConfigureAwait(true);
            PowerCalTxOn = turnOn;
            AppendLog($"QRP cal TX {(turnOn ? "ON" : "OFF")} → 0xAC band={PowerCalSelectedBand}");
        }
        catch (Exception ex)
        {
            AppendLog($"QRP cal TX error: {ex.Message}");
        }
    }

    [RelayCommand]
    private async Task TogglePowerCalCalibrateAsync()
    {
        if (PowerCalSelectedBand <= 0)
        {
            StatusText = "Select a band for QRP CAL";
            return;
        }

        if (PowerCalTxOn && !PowerCalCalibrating)
        {
            StatusText = "TX ON — set TX off before CALIBRATE";
            return;
        }

        if (!CanOperate() || _radio == null)
        {
            StatusText = "Connect first";
            return;
        }

        if (!PowerCalCalibrating)
        {
            if (!PowerCalLoadConfirmed)
            {
                StatusText = "Confirm dummy load / antenna first";
                return;
            }

            PowerCalAcceptPrompt = false;
            PowerCalCalibrating = true;
            PowerCalTxOn = true;

            _suppressPowerCalSlider = true;
            try
            {
                PowerCalSliderValue = 0;
                PowerCalStepLabel = "CALIBRATION STEP: 0";
            }
            finally
            {
                _suppressPowerCalSlider = false;
            }

            try
            {
                await _radio.SetBandPowerPowerAsync(0).ConfigureAwait(true);
                await _radio.SetCalibrationTuneAsync(true).ConfigureAwait(true);
                AppendLog($"QRP cal CALIBRATE START band={PowerCalSelectedBand} → 0xA2 0, 0xAC 1");
            }
            catch (Exception ex)
            {
                AppendLog($"QRP cal start error: {ex.Message}");
            }
        }
        else
        {
            try
            {
                await _radio.SetCalibrationTuneAsync(false).ConfigureAwait(true);
            }
            catch (Exception ex)
            {
                AppendLog($"QRP cal stop TX error: {ex.Message}");
            }

            PowerCalCalibrating = false;
            PowerCalTxOn = false;
            _powerCalPendingBand = PowerCalSelectedBand;

            _suppressPowerCalSlider = true;
            try
            {
                PowerCalSliderValue = 0;
                PowerCalStepLabel = "CALIBRATION STEP: —";
            }
            finally
            {
                _suppressPowerCalSlider = false;
            }

            PowerCalAcceptPrompt = true;
            StatusText = "Accept this QRP calibration?";
            AppendLog($"QRP cal CALIBRATE STOP band={PowerCalSelectedBand} → 0xAC 0");
        }
    }

    [RelayCommand]
    private void AcceptPowerCal()
    {
        if (_powerCalPendingBand > 0)
            SetPowerCalBandCalibrated(_powerCalPendingBand, true);
        PowerCalAcceptPrompt = false;
        StatusText = $"QRP cal accepted: {_powerCalPendingBand}m";
        _powerCalPendingBand = 0;
    }

    [RelayCommand]
    private void RejectPowerCal()
    {
        if (_powerCalPendingBand > 0)
            SetPowerCalBandCalibrated(_powerCalPendingBand, false);
        PowerCalAcceptPrompt = false;
        StatusText = $"QRP cal rejected: {_powerCalPendingBand}m";
        _powerCalPendingBand = 0;
    }

    [RelayCommand]
    private async Task CancelPowerCalAsync()
    {
        int band = _powerCalPendingBand;
        PowerCalAcceptPrompt = false;
        _powerCalPendingBand = 0;
        int restore = Math.Clamp(_powerCalPreviousReceivedStep, 0, 100);
        if (CanOperate() && _radio != null)
        {
            try
            {
                await _radio.SetBandPowerPowerAsync(restore).ConfigureAwait(true);
            }
            catch (Exception ex)
            {
                AppendLog($"QRP cal cancel restore error: {ex.Message}");
            }
        }

        _suppressPowerCalSlider = true;
        try
        {
            PowerCalSliderValue = restore;
            PowerCalStepLabel = $"CALIBRATION STEP: {restore}";
        }
        finally
        {
            _suppressPowerCalSlider = false;
        }

        StatusText = "QRP cal cancelled (restored step)";
        AppendLog($"QRP cal CANCEL band={band}m restore 0xA2={restore}");
    }

    private void ForceStopPowerCal(string reason)
    {
        if (!PowerCalTxOn && !PowerCalCalibrating && !PowerCalAcceptPrompt) return;
        if (_radio != null && IsConnected)
        {
            try { _ = _radio.SetCalibrationTuneAsync(false); }
            catch { /* best effort */ }
        }

        PowerCalTxOn = false;
        PowerCalCalibrating = false;
        PowerCalAcceptPrompt = false;
        _powerCalPendingBand = 0;
        AppendLog($"QRP cal forced stop ({reason})");
    }

    [RelayCommand]
    private async Task SelectAmpCalBandAsync(int band)
    {
        if (band <= 0) return;
        if (AmpCalTxOn || AmpCalCalibrating)
        {
            StatusText = "TX ON — set TX off before band change";
            AppendLog("AMP cal: band change blocked (TX on)");
            return;
        }

        if (AmpCalSelectedBand == band) return;

        AmpCalSelectedBand = band;
        foreach (var item in AmpCalBandStatuses)
            item.IsSelected = item.BandNumber == band;

        long freq = GetCalFrequencyHz(band);
        if (freq <= 0)
        {
            AppendLog($"AMP cal band {band}: no cal frequency");
            return;
        }

        if (CanOperate() && _radio != null)
        {
            try
            {
                await ApplyFrequencyAsync(freq, $"amp-cal {band}m").ConfigureAwait(true);
                await _radio.SetAmplifierInitializeAsync(band).ConfigureAwait(true);
                await _radio.SetAmplifierPowerAsync(100).ConfigureAwait(true);
                AppendLog($"AMP cal band {band}m → 0xF9, 0xFA 100, f={freq}");
            }
            catch (Exception ex)
            {
                AppendLog($"AMP cal band error: {ex.Message}");
            }
        }
        else
        {
            _frequencyHz = freq;
            UpdateFrequencyUi(freq);
            BandText = band + "m";
            AppendLog($"AMP cal band {band}m selected (not connected)");
        }

        _suppressAmpCalSlider = true;
        try
        {
            AmpCalSliderValue = -99;
            AmpCalStepLabel = "STEP: 0";
        }
        finally
        {
            _suppressAmpCalSlider = false;
        }
    }

    partial void OnAmpCalSliderValueChanged(int value)
    {
        value = Math.Clamp(value, -99, 0);
        int step = AmpCalStepFromSlider(value);
        if (!_suppressAmpCalSlider)
            AmpCalStepLabel = $"STEP: {step}";

        if (_suppressAmpCalSlider || !AmpCalCalibrating || !CanOperate() || _radio == null)
            return;

        _ = SendCalAsync(
            () => _radio.SetPotentiaCalibrationAsync(value),
            $"AMP cal 0x08={value} (STEP {step})");
    }

    [RelayCommand]
    private async Task ToggleAmpCalTxAsync()
    {
        if (AmpCalCalibrating)
        {
            StatusText = "Finish CALIBRATE first";
            return;
        }

        if (AmpCalSelectedBand <= 0)
        {
            StatusText = "Select a band for AMP CAL";
            return;
        }

        if (!CanOperate() || _radio == null)
        {
            StatusText = "Connect first";
            return;
        }

        if (!AmpCalTxOn)
        {
            try
            {
                _modeBeforeAmpCal = string.IsNullOrWhiteSpace(ModeText) ? "USB" : ModeText;
                await _radio.SetTunePowerAsync(100).ConfigureAwait(true);
                await _radio.SetModeAsync("TUNE").ConfigureAwait(true);
                ModeText = "TUNE";
                await _radio.SetAutoTuneAsync(true).ConfigureAwait(true);
                AmpCalTxOn = true;
                AppendLog($"AMP cal TX ON band={AmpCalSelectedBand}");
            }
            catch (Exception ex)
            {
                AppendLog($"AMP cal TX error: {ex.Message}");
            }
        }
        else
        {
            await StopAmpCalTuneCarrierAsync("TX button").ConfigureAwait(true);
        }
    }

    [RelayCommand]
    private async Task ToggleAmpCalCalibrateAsync()
    {
        if (AmpCalSelectedBand <= 0)
        {
            StatusText = "Select a band for AMP CAL";
            return;
        }

        if (!CanOperate() || _radio == null)
        {
            StatusText = "Connect first";
            return;
        }

        // Stop path
        if (AmpCalCalibrating || AmpCalTxOn)
        {
            bool wasCal = AmpCalCalibrating;
            int band = AmpCalSelectedBand;
            await StopAmpCalTuneCarrierAsync(wasCal ? "CALIBRATE stop" : "CALIBRATE (was TX)").ConfigureAwait(true);

            _suppressAmpCalSlider = true;
            try
            {
                AmpCalSliderValue = -99;
                AmpCalStepLabel = "STEP: 0";
            }
            finally
            {
                _suppressAmpCalSlider = false;
            }

            if (wasCal && band > 0)
            {
                _ampCalPendingBand = band;
                AmpCalAcceptPrompt = true;
                StatusText = "Accept this AMP calibration?";
            }
            return;
        }

        // Start requires QRP cal green for band
        var xcvCal = PowerCalBandStatuses.FirstOrDefault(b => b.BandNumber == AmpCalSelectedBand);
        if (xcvCal == null || !xcvCal.IsCalibrated)
        {
            StatusText = "QRP CAL not done for this band";
            AppendLog("AMP cal blocked: QRP cal lamp not green for band");
            return;
        }

        try
        {
            _modeBeforeAmpCal = string.IsNullOrWhiteSpace(ModeText) ? "USB" : ModeText;
            await _radio.SetTunePowerAsync(100).ConfigureAwait(true);
            await _radio.SetModeAsync("TUNE").ConfigureAwait(true);
            ModeText = "TUNE";
            await _radio.SetAutoTuneAsync(true).ConfigureAwait(true);

            AmpCalCalibrating = true;
            AmpCalTxOn = true;
            AmpCalAcceptPrompt = false;

            _suppressAmpCalSlider = true;
            try
            {
                AmpCalSliderValue = -99;
                AmpCalStepLabel = $"STEP: {AmpCalStepFromSlider(-99)}";
            }
            finally
            {
                _suppressAmpCalSlider = false;
            }

            await _radio.SetPotentiaCalibrationAsync(-99).ConfigureAwait(true);
            AppendLog($"AMP cal CALIBRATE START band={AmpCalSelectedBand}");
        }
        catch (Exception ex)
        {
            AppendLog($"AMP cal start error: {ex.Message}");
        }
    }

    private async Task StopAmpCalTuneCarrierAsync(string reason)
    {
        if (_radio != null && IsConnected)
        {
            try
            {
                await _radio.SetAutoTuneAsync(false).ConfigureAwait(true);
                string restore = string.IsNullOrWhiteSpace(_modeBeforeAmpCal) ||
                                 string.Equals(_modeBeforeAmpCal, "TUNE", StringComparison.OrdinalIgnoreCase)
                    ? "USB"
                    : _modeBeforeAmpCal;
                await _radio.SetModeAsync(restore).ConfigureAwait(true);
                ModeText = restore;
            }
            catch (Exception ex)
            {
                AppendLog($"AMP cal stop error: {ex.Message}");
            }
        }

        AmpCalTxOn = false;
        AmpCalCalibrating = false;
        AppendLog($"AMP cal tune/TX OFF ({reason})");
    }

    [RelayCommand]
    private void AcceptAmpCal()
    {
        if (_ampCalPendingBand > 0)
            SetAmpCalBandCalibrated(_ampCalPendingBand, true);
        AmpCalAcceptPrompt = false;
        StatusText = $"AMP cal accepted: {_ampCalPendingBand}m";
        _ampCalPendingBand = 0;
    }

    [RelayCommand]
    private void RejectAmpCal()
    {
        if (_ampCalPendingBand > 0)
            SetAmpCalBandCalibrated(_ampCalPendingBand, false);
        AmpCalAcceptPrompt = false;
        StatusText = $"AMP cal rejected: {_ampCalPendingBand}m";
        _ampCalPendingBand = 0;
    }

    [RelayCommand]
    private void CancelAmpCal()
    {
        AmpCalAcceptPrompt = false;
        StatusText = "AMP cal cancelled (lamp unchanged)";
        AppendLog($"AMP cal CANCEL band={_ampCalPendingBand}m");
        _ampCalPendingBand = 0;
    }

    private void ForceStopAmpCal(string reason)
    {
        if (!AmpCalTxOn && !AmpCalCalibrating && !AmpCalAcceptPrompt) return;
        if (_radio != null && IsConnected)
        {
            try
            {
                _ = _radio.SetAutoTuneAsync(false);
            }
            catch { /* best effort */ }
        }

        AmpCalTxOn = false;
        AmpCalCalibrating = false;
        AmpCalAcceptPrompt = false;
        _ampCalPendingBand = 0;
        AppendLog($"AMP cal forced stop ({reason})");
    }

    private async Task SendCalAsync(Func<Task> send, string okMsg)
    {
        if (_radio == null) return;
        try
        {
            await send().ConfigureAwait(true);
            AppendLog($"Sent {okMsg}");
        }
        catch (Exception ex)
        {
            AppendLog($"Cal error: {ex.Message}");
        }
    }

    // ----- RX IQ + TX IQ -----

    partial void OnRxIqSessionActiveChanged(bool value) =>
        OnPropertyChanged(nameof(RxIqStartButtonText));

    partial void OnTxIqTxOnChanged(bool value)
    {
        OnPropertyChanged(nameof(TxIqTxButtonText));
        OnPropertyChanged(nameof(TxIqBandSelectEnabled));
    }

    private static int? TryParseAmateurBandMeters(string? band)
    {
        if (string.IsNullOrWhiteSpace(band)) return null;
        string b = band.Trim().ToLowerInvariant();
        if (b is "gen" or "user" or "?" or "—") return null;
        if (b.EndsWith('m'))
            b = b[..^1];
        if (!int.TryParse(b, NumberStyles.Integer, CultureInfo.InvariantCulture, out int meters))
            return null;
        return meters is 160 or 80 or 60 or 40 or 30 or 20 or 17 or 15 or 12 or 10
            ? meters
            : null;
    }

    private static string FormatIqFreqDisplay(long freqHz)
    {
        if (freqHz <= 0) return "—.—.—";
        long mhz = freqHz / 1_000_000;
        long khz = (freqHz - mhz * 1_000_000) / 1000;
        long hz = freqHz - mhz * 1_000_000 - khz * 1000;
        return $"{mhz}.{khz:000}.{hz:000}";
    }

    private long ComputeRxIqTuneFreqHz()
    {
        long f = _rxIqBaseFreqHz;
        if (RxIqUp24k) f += 24_000;
        f += RxIqFreqOffsetHz;
        return f;
    }

    private void RefreshRxIqFreqDisplay() =>
        RxIqFreqDisplay = FormatIqFreqDisplay(ComputeRxIqTuneFreqHz());

    private async Task ApplyRxIqTuneFrequencyAsync()
    {
        if (!RxIqSessionActive || _suppressRxIqFreqTune) return;
        long total = ComputeRxIqTuneFreqHz();
        if (total <= 0) return;
        RefreshRxIqFreqDisplay();
        if (CanOperate() && _radio != null)
        {
            try
            {
                await _radio.SetFrequencyAsync(total).ConfigureAwait(true);
                _frequencyHz = total;
                UpdateFrequencyUi(total);
                AppendLog($"RX IQ LO → {total}");
            }
            catch (Exception ex)
            {
                AppendLog($"RX IQ tune error: {ex.Message}");
            }
        }
        else
        {
            _frequencyHz = total;
            UpdateFrequencyUi(total);
        }
    }

    partial void OnRxIqUp24kChanged(bool value)
    {
        if (!RxIqSessionActive || _suppressRxIqFreqTune) return;
        _ = ApplyRxIqTuneFrequencyAsync();
        RxIqStatus = value ? "UP 24 kHz ON — LO +24 000 Hz." : "UP 24 kHz OFF.";
    }

    partial void OnRxIqFreqOffsetHzChanged(int value)
    {
        int clamped = Math.Clamp(value, -1000, 2009);
        if (clamped != value)
        {
            _suppressRxIqFreqTune = true;
            RxIqFreqOffsetHz = clamped;
            _suppressRxIqFreqTune = false;
            return;
        }

        if (!RxIqSessionActive || _suppressRxIqFreqTune) return;
        _ = ApplyRxIqTuneFrequencyAsync();
    }

    partial void OnRxIqOffsetChanged(int value)
    {
        if (_suppressRxIqOffset || !RxIqSessionActive) return;
        int v = Math.Clamp(value, -200, 200);
        if (v != value)
        {
            _suppressRxIqOffset = true;
            RxIqOffset = v;
            _suppressRxIqOffset = false;
            return;
        }

        if (!CanOperate() || _radio == null) return;
        _ = SendCalAsync(() => _radio.SetIqOffsetAsync(v), $"RX IQ offset 0x52={v}");
    }

    [RelayCommand]
    private async Task StartRxIqAsync()
    {
        if (RxIqSessionActive)
        {
            LeaveRxIqSession("START off");
            return;
        }

        if (!CanOperate() || _radio == null)
        {
            StatusText = "Connect first";
            return;
        }

        string bandKey = NormalizeFavoriteBand(BandText, _frequencyHz);
        int? meters = TryParseAmateurBandMeters(bandKey);
        if (meters is null)
        {
            StatusText = "Invalid band — select amateur band first";
            RxIqStatus = "INVALID BAND (general). Return to MAIN and select an amateur band.";
            AppendLog($"RX IQ START blocked: band={bandKey}");
            return;
        }

        long baseFreq = _frequencyHz > 0 ? _frequencyHz : GetCalFrequencyHz(meters.Value);
        if (baseFreq <= 0)
            baseFreq = GetCalFrequencyHz(meters.Value);
        if (baseFreq <= 0)
        {
            StatusText = "No frequency for this band";
            return;
        }

        _rxIqBandMeters = meters.Value;
        _rxIqBaseFreqHz = baseFreq;

        _suppressRxIqFreqTune = true;
        _suppressRxIqOffset = true;
        try
        {
            RxIqBandLabel = meters.Value + "m";
            RxIqFreqOffsetHz = 0;
            RxIqUp24k = false;
            RxIqOffset = 0;
            RefreshRxIqFreqDisplay();
        }
        finally
        {
            _suppressRxIqFreqTune = false;
            _suppressRxIqOffset = false;
        }

        try
        {
            await _radio.SetIqBandAsync(meters.Value).ConfigureAwait(true);
            await _radio.SetIqCalibrationRxTxAsync(txIq: false).ConfigureAwait(true);
            await _radio.SetFrequencyAsync(_rxIqBaseFreqHz).ConfigureAwait(true);
            _frequencyHz = _rxIqBaseFreqHz;
            UpdateFrequencyUi(_rxIqBaseFreqHz);
            BandText = RxIqBandLabel;

            RxIqSessionActive = true;
            RxIqResetAllPrompt = false;
            RxIqStatus = $"ACTIVE — {RxIqBandLabel} {RxIqFreqDisplay}. Adjust I/Q OFFSET, then APPLY.";
            AppendLog($"RX IQ START: band={meters.Value}m base={_rxIqBaseFreqHz} → 0x58 + 0x55 RX");
        }
        catch (Exception ex)
        {
            AppendLog($"RX IQ start error: {ex.Message}");
            StatusText = $"RX IQ failed: {ex.Message}";
        }
    }

    private void LeaveRxIqSession(string reason)
    {
        if (!RxIqSessionActive && !RxIqCommitting)
            return;

        RxIqCommitting = false;
        RxIqSessionActive = false;
        RxIqResetAllPrompt = false;
        _suppressRxIqOffset = true;
        _suppressRxIqFreqTune = true;
        try
        {
            RxIqOffset = 0;
        }
        finally
        {
            _suppressRxIqOffset = false;
            _suppressRxIqFreqTune = false;
        }

        RxIqStatus = $"Session ended ({reason}). Press START to re-enter.";
        AppendLog($"RX IQ LEAVE ({reason})");
    }

    [RelayCommand]
    private async Task ZeroRxIqOffsetAsync()
    {
        if (!RxIqSessionActive || _radio == null)
        {
            StatusText = "Start RX IQ first";
            return;
        }

        _suppressRxIqOffset = true;
        try { RxIqOffset = 0; }
        finally { _suppressRxIqOffset = false; }

        try
        {
            await _radio.SetIqOffsetAsync(0).ConfigureAwait(true);
            await _radio.CommitIqAsync().ConfigureAwait(true);
            RxIqCommitting = true;
            RxIqStatus = "Offset ZERO + COMMIT sent (0x52, 0x57)…";
            AppendLog("RX IQ ZERO + COMMIT");
        }
        catch (Exception ex)
        {
            AppendLog($"RX IQ zero error: {ex.Message}");
        }
    }

    [RelayCommand]
    private void ResetRxIqFreq()
    {
        _suppressRxIqFreqTune = true;
        try { RxIqFreqOffsetHz = 0; }
        finally { _suppressRxIqFreqTune = false; }

        if (RxIqSessionActive)
        {
            _ = ApplyRxIqTuneFrequencyAsync();
            RxIqStatus = $"LO fine cleared — freq {RxIqFreqDisplay}.";
        }
        else
        {
            RefreshRxIqFreqDisplay();
            RxIqStatus = "LO fine offset cleared.";
        }
        AppendLog("RX IQ RESET FREQ");
    }

    [RelayCommand]
    private async Task ApplyRxIqAsync()
    {
        if (!RxIqSessionActive || _radio == null)
        {
            StatusText = "Start RX IQ first";
            return;
        }

        try
        {
            RxIqCommitting = true;
            RxIqStatus = "APPLYING… (0x57)";
            await _radio.CommitIqAsync().ConfigureAwait(true);
            AppendLog($"RX IQ APPLY (0x57) band={_rxIqBandMeters} offset={RxIqOffset}");
        }
        catch (Exception ex)
        {
            RxIqCommitting = false;
            AppendLog($"RX IQ apply error: {ex.Message}");
        }
    }

    [RelayCommand]
    private void ResetAllRxIq()
    {
        if (!RxIqSessionActive)
        {
            StatusText = "Start RX IQ first";
            return;
        }

        RxIqResetAllPrompt = true;
        RxIqStatus = "Confirm RESET ALL (applies current I/Q to every band)?";
    }

    [RelayCommand]
    private async Task ConfirmResetAllRxIqAsync()
    {
        RxIqResetAllPrompt = false;
        if (!RxIqSessionActive || _radio == null) return;
        try
        {
            await _radio.SetIqOffsetAsync(RxIqOffset).ConfigureAwait(true);
            await _radio.ResetAllIqBandsAsync(rxIq: true).ConfigureAwait(true);
            RxIqStatus = "RESET ALL sent — all bands set to current I/Q offset (0x8D).";
            AppendLog($"RX IQ RESET ALL offset={RxIqOffset}");
        }
        catch (Exception ex)
        {
            AppendLog($"RX IQ reset-all error: {ex.Message}");
        }
    }

    [RelayCommand]
    private void CancelResetAllRxIq()
    {
        RxIqResetAllPrompt = false;
        RxIqStatus = "RESET ALL cancelled.";
    }

    private void EnsureTxIqBandItems()
    {
        if (TxIqBandItems.Count > 0) return;
        foreach (int n in TxIqBandNumbers)
        {
            TxIqBandItems.Add(new PowerCalBandItem
            {
                BandNumber = n,
                BandLabel = n.ToString(CultureInfo.InvariantCulture),
                IsCalibrated = false,
                IsSelected = false
            });
        }
    }

    private void ForceStopTxIqSession(string reason)
    {
        if (!TxIqTxOn && !TxIqCommitting && !TxIqResetAllPrompt) return;
        if (_radio != null && IsConnected)
        {
            try
            {
                _ = _radio.SetAutoTuneAsync(false);
                _ = _radio.SetIqCalibrationTuneAsync(false);
            }
            catch { /* best effort */ }
        }

        TxIqTxOn = false;
        TxIqCommitting = false;
        TxIqResetAllPrompt = false;
        AppendLog($"TX IQ session stop ({reason})");
    }

    [RelayCommand]
    private async Task SelectTxIqBandAsync(int band)
    {
        if (AmpOn)
        {
            StatusText = "TX IQ requires QRP (AMP off)";
            return;
        }

        if (TxIqTxOn)
        {
            StatusText = "Turn TX OFF before band change";
            return;
        }

        if (band <= 0) return;
        EnsureTxIqBandItems();
        foreach (var item in TxIqBandItems)
            item.IsSelected = item.BandNumber == band;
        TxIqSelectedBand = band;

        long freq = GetCalFrequencyHz(band);
        _suppressTxIqOffset = true;
        try { TxIqOffset = 0; }
        finally { _suppressTxIqOffset = false; }

        if (CanOperate() && _radio != null)
        {
            try
            {
                if (freq > 0)
                    await ApplyFrequencyAsync(freq, $"tx-iq {band}m").ConfigureAwait(true);
                await _radio.SetIqBandAsync(band).ConfigureAwait(true);
                TxIqStatus = $"{band}M selected. Set power, then TX ON.";
                AppendLog($"TX IQ band {band}m freq={freq}");
            }
            catch (Exception ex)
            {
                AppendLog($"TX IQ band error: {ex.Message}");
            }
        }
        else
        {
            if (freq > 0)
            {
                _frequencyHz = freq;
                UpdateFrequencyUi(freq);
                BandText = band + "m";
            }
            TxIqStatus = $"{band}M selected (not connected).";
        }
    }

    partial void OnTxIqOffsetChanged(int value)
    {
        if (_suppressTxIqOffset || !TxIqTxOn || TxIqSelectedBand <= 0) return;
        int v = Math.Clamp(value, -200, 200);
        if (v != value)
        {
            _suppressTxIqOffset = true;
            TxIqOffset = v;
            _suppressTxIqOffset = false;
            return;
        }

        if (!CanOperate() || _radio == null) return;
        _ = SendCalAsync(() => _radio.SetIqOffsetAsync(v), $"TX IQ offset 0x52={v}");
    }

    partial void OnTxIqPowerChanged(int value)
    {
        if (TxIqSelectedBand <= 0) return;
        if (!CanOperate() || _radio == null) return;
        int p = Math.Clamp(value, 0, 100);
        _ = SendCalAsync(() => _radio.SetTunePowerAsync(p), $"TX IQ power {p}%");
    }

    [RelayCommand]
    private async Task ToggleTxIqTxAsync()
    {
        if (AmpOn)
        {
            StatusText = "TX IQ requires QRP (AMP off)";
            return;
        }

        if (TxIqSelectedBand <= 0)
        {
            StatusText = "Select a band for TX IQ";
            return;
        }

        if (!CanOperate() || _radio == null)
        {
            StatusText = "Connect first";
            return;
        }

        if (!TxIqTxOn)
        {
            try
            {
                _modeBeforeTxIq = string.IsNullOrWhiteSpace(ModeText) ? "USB" : ModeText;
                await _radio.SetIqCalibrationRxTxAsync(true).ConfigureAwait(true);
                await _radio.SetIqBandAsync(TxIqSelectedBand).ConfigureAwait(true);
                await _radio.SetTunePowerAsync(TxIqPower).ConfigureAwait(true);
                await _radio.SetModeAsync("TUNE").ConfigureAwait(true);
                ModeText = "TUNE";
                await _radio.SetAutoTuneAsync(true).ConfigureAwait(true);
                TxIqTxOn = true;
                TxIqStatus = "TX ON — adjust OFFSET (external RX), then APPLY or TX OFF.";
                AppendLog($"TX IQ TX ON band={TxIqSelectedBand} power={TxIqPower}");
            }
            catch (Exception ex)
            {
                AppendLog($"TX IQ TX error: {ex.Message}");
            }
        }
        else
        {
            await StopTxIqCarrierAsync().ConfigureAwait(true);
            TxIqStatus = "TX OFF. Use APPLY to commit if desired.";
        }
    }

    private async Task StopTxIqCarrierAsync()
    {
        if (_radio != null && IsConnected)
        {
            try
            {
                await _radio.SetAutoTuneAsync(false).ConfigureAwait(true);
                await _radio.SetIqCalibrationTuneAsync(false).ConfigureAwait(true);
                string restore = string.IsNullOrWhiteSpace(_modeBeforeTxIq) ||
                                 string.Equals(_modeBeforeTxIq, "TUNE", StringComparison.OrdinalIgnoreCase)
                    ? "USB"
                    : _modeBeforeTxIq;
                await _radio.SetModeAsync(restore).ConfigureAwait(true);
                ModeText = restore;
            }
            catch (Exception ex)
            {
                AppendLog($"TX IQ stop error: {ex.Message}");
            }
        }

        TxIqTxOn = false;
        AppendLog("TX IQ TX OFF");
    }

    [RelayCommand]
    private async Task ApplyTxIqAsync()
    {
        if (AmpOn)
        {
            StatusText = "TX IQ requires QRP (AMP off)";
            return;
        }

        if (TxIqSelectedBand <= 0)
        {
            StatusText = "Select a band for TX IQ";
            return;
        }

        if (!CanOperate() || _radio == null)
        {
            StatusText = "Connect first";
            return;
        }

        try
        {
            TxIqCommitting = true;
            TxIqStatus = "APPLYING…";
            await _radio.CommitIqAsync().ConfigureAwait(true);
            AppendLog($"TX IQ COMMIT (0x57) band={TxIqSelectedBand} offset={TxIqOffset}");
        }
        catch (Exception ex)
        {
            TxIqCommitting = false;
            AppendLog($"TX IQ apply error: {ex.Message}");
        }
    }

    [RelayCommand]
    private void ResetAllTxIq()
    {
        if (AmpOn)
        {
            StatusText = "TX IQ requires QRP (AMP off)";
            return;
        }

        if (TxIqTxOn)
        {
            StatusText = "Turn TX OFF before reset";
            return;
        }

        TxIqResetAllPrompt = true;
        TxIqStatus = "Confirm factory RESET ALL I/Q bands?";
    }

    [RelayCommand]
    private async Task ConfirmResetAllTxIqAsync()
    {
        TxIqResetAllPrompt = false;
        if (!CanOperate() || _radio == null) return;
        try
        {
            await _radio.ResetAllIqBandsAsync(rxIq: false).ConfigureAwait(true);
            _suppressTxIqOffset = true;
            try { TxIqOffset = 0; }
            finally { _suppressTxIqOffset = false; }
            TxIqStatus = "Factory reset of all I/Q bands requested.";
            AppendLog("TX IQ RESET ALL (0x8D)");
        }
        catch (Exception ex)
        {
            AppendLog($"TX IQ reset-all error: {ex.Message}");
        }
    }

    [RelayCommand]
    private void CancelResetAllTxIq()
    {
        TxIqResetAllPrompt = false;
        TxIqStatus = "RESET ALL cancelled.";
    }

    private void OnIqOperationComplete(int op)
    {
        if (RxIqCommitting)
        {
            RxIqCommitting = false;
            RxIqStatus = op switch
            {
                1 => "APPLY/ZERO succeeded (IQ_OPERATION_COMPLETE).",
                0 => "APPLY/ZERO failed (IQ_OPERATION_COMPLETE).",
                _ => $"APPLY complete (operand={op})."
            };
            AppendLog($"RX IQ 0x56 op={op}");
            return;
        }

        if (!TxIqCommitting) return;
        TxIqCommitting = false;
        TxIqStatus = op switch
        {
            1 => "APPLY succeeded (IQ_OPERATION_COMPLETE).",
            0 => "APPLY failed (IQ_OPERATION_COMPLETE).",
            _ => $"APPLY complete (operand={op})."
        };
        AppendLog($"TX IQ 0x56 op={op}");
    }

    private void OnIqValueReported(int v)
    {
        int clamped = Math.Clamp(v, -200, 200);
        if (RxIqSessionActive)
        {
            _suppressRxIqOffset = true;
            try { RxIqOffset = clamped; }
            finally { _suppressRxIqOffset = false; }
            AppendLog($"RX IQ value from server: {v}");
            return;
        }

        if (TxIqSelectedBand <= 0 && !TxIqTxOn) return;
        _suppressTxIqOffset = true;
        try { TxIqOffset = clamped; }
        finally { _suppressTxIqOffset = false; }
        AppendLog($"TX IQ value from server: {v}");
    }

    // ----- FREQ CAL -----

    partial void OnFreqCalLooseChanged(bool value) =>
        OnPropertyChanged(nameof(FreqCalLooseButtonText));

    partial void OnFreqCalManualModeChanged(bool value)
    {
        OnPropertyChanged(nameof(FreqCalManualButtonText));
        OnPropertyChanged(nameof(FreqCalActionsEnabled));
        OnPropertyChanged(nameof(FreqCalManualButtonEnabled));
        OnPropertyChanged(nameof(FreqCalPpmEnabled));
    }

    partial void OnFreqCalInProgressChanged(bool value)
    {
        OnPropertyChanged(nameof(FreqCalActionsEnabled));
        OnPropertyChanged(nameof(FreqCalManualButtonEnabled));
        OnPropertyChanged(nameof(FreqCalPpmEnabled));
    }

    partial void OnFreqCalAutoModePromptChanged(bool value)
    {
        OnPropertyChanged(nameof(FreqCalActionsEnabled));
        OnPropertyChanged(nameof(FreqCalManualButtonEnabled));
    }

    partial void OnFreqCalManualAcceptPromptChanged(bool value)
    {
        OnPropertyChanged(nameof(FreqCalActionsEnabled));
        OnPropertyChanged(nameof(FreqCalPpmEnabled));
    }

    [RelayCommand]
    private async Task ToggleFreqCalLooseAsync()
    {
        FreqCalLoose = !FreqCalLoose;
        if (!CanOperate() || _radio == null) return;
        try
        {
            await _radio.SetCalLooseAsync(FreqCalLoose).ConfigureAwait(true);
            AppendLog($"Freq Cal: {(FreqCalLoose ? "LOOSE" : "TIGHT")}");
        }
        catch (Exception ex)
        {
            AppendLog($"Freq Cal loose error: {ex.Message}");
        }
    }

    [RelayCommand]
    private void FreqCalAuto()
    {
        if (FreqCalManualMode)
        {
            StatusText = "Exit MANUAL before AUTO";
            return;
        }

        if (FreqCalInProgress)
        {
            StatusText = "Calibration already in progress";
            return;
        }

        if (!CanOperate())
        {
            StatusText = "Connect first";
            return;
        }

        FreqCalResetPrompt = false;
        FreqCalAutoModePrompt = true;
        FreqCalStatus = "Choose COARSE or FINE…";
    }

    [RelayCommand]
    private Task FreqCalAutoCoarseAsync() => StartFreqCalAutoAsync(coarse: true);

    [RelayCommand]
    private Task FreqCalAutoFineAsync() => StartFreqCalAutoAsync(coarse: false);

    [RelayCommand]
    private void FreqCalAutoCancel()
    {
        FreqCalAutoModePrompt = false;
        FreqCalStatus = "AUTO cancelled";
        AppendLog("Freq Cal: AUTO cancelled");
    }

    private async Task StartFreqCalAutoAsync(bool coarse)
    {
        FreqCalAutoModePrompt = false;
        if (!CanOperate() || _radio == null)
        {
            StatusText = "Connect first";
            return;
        }

        await ForceCwForFreqCalAsync().ConfigureAwait(true);

        int freqHz = 0;
        if (_frequencyHz > 0 && _frequencyHz <= int.MaxValue)
            freqHz = (int)_frequencyHz;

        FreqCalProgress = 0;
        FreqCalInProgress = true;
        _freqCalIsAuto = true;
        _lastCalDelta = 0;
        FreqCalStatus = coarse ? "RUNNING COARSE — WAIT" : "RUNNING FINE — WAIT";

        try
        {
            await _radio.SetCalLooseAsync(FreqCalLoose).ConfigureAwait(true);
            await _radio.SetCalCheckAsync(false).ConfigureAwait(true);
            await _radio.SetCalModeAsync(coarse ? 0 : 1).ConfigureAwait(true);
            await _radio.StartCalibrateAsync(freqHz).ConfigureAwait(true);
            AppendLog($"Freq Cal: AUTO start ({(coarse ? "COARSE" : "FINE")}, loose={FreqCalLoose}, f={freqHz})");
        }
        catch (Exception ex)
        {
            FreqCalInProgress = false;
            _freqCalIsAuto = false;
            FreqCalStatus = "AUTO FAILED";
            AppendLog($"Freq Cal: AUTO start error: {ex.Message}");
        }
    }

    [RelayCommand]
    private async Task ToggleFreqCalManualAsync()
    {
        if (FreqCalInProgress)
        {
            StatusText = "Calibration in progress";
            return;
        }

        if (!CanOperate() || _radio == null)
        {
            StatusText = "Connect first";
            return;
        }

        if (!FreqCalManualMode)
        {
            try
            {
                ResetFreqCalManualPpmUi(sendToRadio: false);
                await _radio.SetForceCalibrationAsync(true).ConfigureAwait(true);
                FreqCalManualMode = true;
                FreqCalStatus = "MANUAL CALIBRATION";
                AppendLog("Freq Cal: entered MANUAL");
            }
            catch (Exception ex)
            {
                AppendLog($"Freq Cal manual start error: {ex.Message}");
            }
        }
        else
        {
            FlushFreqCalManualPpmPending(force: true);
            FreqCalManualAcceptPrompt = true;
            FreqCalStatus = "Accept this MANUAL calibration?";
        }
    }

    [RelayCommand]
    private async Task AcceptFreqCalManualAsync()
    {
        FreqCalManualAcceptPrompt = false;
        if (_radio != null && IsConnected)
        {
            try
            {
                await _radio.SetForceCalibrationAsync(false).ConfigureAwait(true);
                await _radio.SetCalibrationFinishedAsync(true).ConfigureAwait(true);
            }
            catch (Exception ex)
            {
                AppendLog($"Freq Cal manual accept error: {ex.Message}");
            }
        }

        FreqCalManualMode = false;
        ResetFreqCalManualPpmUi(sendToRadio: false);
        FreqCalStatus = "MANUAL CALIBRATED";
        AppendLog("Freq Cal: MANUAL accepted");
    }

    [RelayCommand]
    private async Task RejectFreqCalManualAsync()
    {
        FreqCalManualAcceptPrompt = false;
        if (_radio != null && IsConnected)
        {
            try
            {
                await _radio.SetForceCalibrationAsync(false).ConfigureAwait(true);
                await _radio.SetCalibrationFinishedAsync(false).ConfigureAwait(true);
            }
            catch (Exception ex)
            {
                AppendLog($"Freq Cal manual reject error: {ex.Message}");
            }
        }

        FreqCalManualMode = false;
        ResetFreqCalManualPpmUi(sendToRadio: false);
        FreqCalStatus = "NOT CALIBRATED";
        AppendLog("Freq Cal: MANUAL rejected");
    }

    [RelayCommand]
    private void FreqCalPpmMinus() => StepFreqCalManualPpm(-1);

    [RelayCommand]
    private void FreqCalPpmPlus() => StepFreqCalManualPpm(+1);

    private void StepFreqCalManualPpm(int delta)
    {
        if (!FreqCalManualMode) return;
        int next = Math.Clamp(FreqCalManualPpm + delta, FreqCalManualPpmMin, FreqCalManualPpmMax);
        if (next == FreqCalManualPpm) return;
        FreqCalManualPpm = next;
        ScheduleFreqCalManualPpmSend();
    }

    private void ResetFreqCalManualPpmUi(bool sendToRadio)
    {
        FreqCalManualPpm = 0;
        _freqCalManualPpmLastSent = int.MinValue;
        StopFreqCalManualPpmTimer();
        if (sendToRadio && _radio != null && IsConnected)
            _ = SendCalAsync(() => _radio.SetCalSetCoarseAsync(0), "Freq Cal PPM 0");
    }

    private void ScheduleFreqCalManualPpmSend()
    {
        var elapsed = (DateTime.UtcNow - _freqCalManualPpmLastSendUtc).TotalMilliseconds;
        if (elapsed >= FreqCalManualPpmMinIntervalMs)
        {
            FlushFreqCalManualPpmPending(force: true);
            return;
        }

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
        if (!FreqCalManualMode || _radio == null || !IsConnected) return;
        if (FreqCalManualPpm == _freqCalManualPpmLastSent) return;

        int val = FreqCalManualPpm;
        _freqCalManualPpmLastSent = val;
        _freqCalManualPpmLastSendUtc = DateTime.UtcNow;
        _ = SendCalAsync(() => _radio.SetCalSetCoarseAsync(val), $"Freq Cal PPM {val}");
    }

    private void StopFreqCalManualPpmTimer()
    {
        if (_freqCalManualPpmTimer is { IsEnabled: true })
            _freqCalManualPpmTimer.Stop();
    }

    [RelayCommand]
    private async Task FreqCalCheckAsync()
    {
        if (FreqCalInProgress)
        {
            StatusText = "Calibration already in progress";
            return;
        }

        if (!CanOperate() || _radio == null)
        {
            StatusText = "Connect first";
            return;
        }

        await ForceCwForFreqCalAsync().ConfigureAwait(true);

        FreqCalProgress = 0;
        FreqCalInProgress = true;
        _freqCalIsAuto = false;
        _lastCalDelta = 0;
        FreqCalStatus = "CHECKING — WAIT";

        try
        {
            await _radio.SetCalCheckAsync(true).ConfigureAwait(true);
            AppendLog("Freq Cal: CHECK started");
        }
        catch (Exception ex)
        {
            FreqCalInProgress = false;
            FreqCalStatus = "CHECK FAILED";
            AppendLog($"Freq Cal CHECK error: {ex.Message}");
        }
    }

    [RelayCommand]
    private void FreqCalReset()
    {
        if (FreqCalInProgress || FreqCalManualMode)
        {
            StatusText = "Finish cal session first";
            return;
        }

        FreqCalResetPrompt = true;
        FreqCalStatus = "Confirm RESET?";
    }

    [RelayCommand]
    private async Task ConfirmFreqCalResetAsync()
    {
        FreqCalResetPrompt = false;
        if (!CanOperate() || _radio == null)
        {
            StatusText = "Connect first";
            return;
        }

        try
        {
            await _radio.SetCalResetAsync(true).ConfigureAwait(true);
            FreqCalStatus = "RESET";
            AppendLog("Freq Cal: RESET");
        }
        catch (Exception ex)
        {
            AppendLog($"Freq Cal reset error: {ex.Message}");
        }
    }

    [RelayCommand]
    private void CancelFreqCalReset()
    {
        FreqCalResetPrompt = false;
        FreqCalStatus = "OK";
    }

    /// <summary>
    /// AUTO/CHECK need CW demod + 200 Hz filter + 600 Hz pitch (Goertzel listens at ~600 Hz).
    /// Also re-sends VFO so LO matches pitch-offset path after mode change.
    /// </summary>
    private async Task ForceCwForFreqCalAsync()
    {
        if (_radio == null || !IsConnected)
            return;

        string currentMode = (ModeText ?? "").Trim();
        if (!string.Equals(currentMode, "CW", StringComparison.OrdinalIgnoreCase))
        {
            _modeBeforeFreqCal ??= string.IsNullOrWhiteSpace(currentMode) ? "USB" : currentMode;
            try
            {
                await _radio.SetModeAsync("CW").ConfigureAwait(true);
                ModeText = "CW";
                NotifyModeFlags();
                RefreshSpectrumFilterOverlay();
                AppendLog("Freq Cal: mode → CW (for Goertzel / 600 Hz pitch path)");
            }
            catch (Exception ex)
            {
                AppendLog($"Freq Cal mode CW error: {ex.Message}");
            }
        }

        // Labels: 1.8k=0, 400=1, 200=2
        if (_cwFilterIndex != 2)
        {
            _cwFilterIndex = 2;
            CwFilterLabel = CwFilterLabels[2];
            RefreshSpectrumFilterOverlay();
            await SendCwFilterAsync(2).ConfigureAwait(true);
        }

        // Pitch index 1 = 600 Hz (must be index, not Hz)
        if (CwPitchIndex != 1)
        {
            // Set without double-send: property change sends pitch
            CwPitchIndex = 1;
        }
        else
        {
            // Ensure radio has 600 Hz even if UI already showed it
            try
            {
                await _radio.SetCwPitchAsync(1).ConfigureAwait(true);
            }
            catch (Exception ex)
            {
                AppendLog($"Freq Cal pitch error: {ex.Message}");
            }
        }

        // Re-assert frequency after CW mode so LO/pitch path and G_tune_freq agree
        if (_frequencyHz > 0)
        {
            try
            {
                await _radio.SetFrequencyAsync(_frequencyHz).ConfigureAwait(true);
                AppendLog($"Freq Cal: re-sent freq {_frequencyHz} after CW setup");
            }
            catch (Exception ex)
            {
                AppendLog($"Freq Cal retune error: {ex.Message}");
            }
        }

        RefreshSpectrumFilterOverlay();
    }

    private async Task RestoreModeAfterFreqCalAsync()
    {
        if (_modeBeforeFreqCal == null || _radio == null || !IsConnected)
        {
            _modeBeforeFreqCal = null;
            return;
        }

        string restore = _modeBeforeFreqCal;
        _modeBeforeFreqCal = null;
        if (string.Equals(restore, "CW", StringComparison.OrdinalIgnoreCase))
            return;

        try
        {
            await _radio.SetModeAsync(restore).ConfigureAwait(true);
            ModeText = restore;
            NotifyModeFlags();
            RefreshSpectrumFilterOverlay();
            // Keep VFO where user left it (carrier); re-send after mode change
            if (_frequencyHz > 0)
                await _radio.SetFrequencyAsync(_frequencyHz).ConfigureAwait(true);
            AppendLog($"Freq Cal: restored mode {restore}");
        }
        catch (Exception ex)
        {
            AppendLog($"Freq Cal restore mode error: {ex.Message}");
        }
    }

    private void OnFreqCalStatusReported(int value)
    {
        bool wasInProgress = FreqCalInProgress;
        bool wasAuto = _freqCalIsAuto;
        FreqCalInProgress = false;
        _freqCalIsAuto = false;

        if (!wasInProgress)
        {
            AppendLog($"Freq Cal: status {value}");
            return;
        }

        string statusText = wasAuto
            ? (value == 1 ? "AUTO COMPLETED" : "AUTO FAILED")
            : (value == 1 ? "CHECK COMPLETED" : "CHECK FAILED");

        if (_lastCalDelta != 0 && Math.Abs(_lastCalDelta) < 10_000)
        {
            statusText += $"  {_lastCalDelta} Hz";
            _lastCalDelta = 0;
        }

        FreqCalStatus = statusText;
        AppendLog($"Freq Cal: {(wasAuto ? "AUTO" : "CHECK")} status={value}");
        _ = RestoreModeAfterFreqCalAsync();
    }

    private void OnFreqCalDeltaReported(int value)
    {
        _lastCalDelta = value;
        string st = FreqCalStatus ?? "";
        if ((st.StartsWith("CHECK COMPLETED", StringComparison.Ordinal) ||
             st.StartsWith("CHECK FAILED", StringComparison.Ordinal) ||
             st.StartsWith("AUTO COMPLETED", StringComparison.Ordinal) ||
             st.StartsWith("AUTO FAILED", StringComparison.Ordinal)) &&
            Math.Abs(value) < 10_000 &&
            !st.Contains("Hz", StringComparison.Ordinal))
        {
            FreqCalStatus = st + $"  {value} Hz";
            _lastCalDelta = 0;
        }

        AppendLog($"Freq Cal: Delta {value} Hz");
    }

    private void ForceStopFreqCal(string reason)
    {
        if (!FreqCalInProgress && !FreqCalManualMode && !FreqCalAutoModePrompt &&
            !FreqCalManualAcceptPrompt && !FreqCalResetPrompt && _modeBeforeFreqCal == null)
            return;

        StopFreqCalManualPpmTimer();
        if (_radio != null && IsConnected && FreqCalManualMode)
        {
            try
            {
                _ = _radio.SetForceCalibrationAsync(false);
                _ = _radio.SetCalibrationFinishedAsync(false);
            }
            catch { /* best effort */ }
        }

        FreqCalInProgress = false;
        _freqCalIsAuto = false;
        FreqCalManualMode = false;
        FreqCalAutoModePrompt = false;
        FreqCalManualAcceptPrompt = false;
        FreqCalResetPrompt = false;
        ResetFreqCalManualPpmUi(sendToRadio: false);
        FreqCalStatus = "OK";
        _ = RestoreModeAfterFreqCalAsync();
        AppendLog($"Freq Cal forced stop ({reason})");
    }

    partial void OnCompressionChanged(int value)
    {
        ScheduleSaveClientSettings();
        if (_suppressCompressionCommand || !CanOperate()) return;
        _ = SendCompressionLevelAsync(Math.Clamp(value, 0, 24));
    }

    partial void OnMonitorOnChanged(bool value)
    {
        ScheduleSaveClientSettings();
        if (_suppressMonitorCommand || !CanOperate()) return;
        _ = SendMonitorAsync(value);
    }

    partial void OnNbOnChanged(bool value)
    {
        ScheduleSaveClientSettings();
        if (_suppressNbCommand || !CanOperate()) return;
        _ = SendNbOnAsync(value);
    }

    partial void OnNbPulseChanged(int value)
    {
        ScheduleSaveClientSettings();
        if (_suppressNbCommand || !CanOperate()) return;
        _ = SendNbPulseAsync(Math.Clamp(value, 10, 510));
    }

    partial void OnNbThresholdChanged(int value)
    {
        ScheduleSaveClientSettings();
        if (_suppressNbCommand || !CanOperate()) return;
        _ = SendNbThresholdAsync(Math.Clamp(value, 1, 1009));
    }

    partial void OnNrOnChanged(bool value)
    {
        ScheduleSaveClientSettings();
        if (_suppressNrCommand || !CanOperate()) return;
        _ = SendNrOnAsync(value);
    }

    partial void OnNrLevelChanged(int value)
    {
        ScheduleSaveClientSettings();
        if (_suppressNrCommand || !CanOperate() || !NrOn) return;
        _ = SendNrLevelAsync(Math.Clamp(value, 0, 100));
    }

    partial void OnAnOnChanged(bool value)
    {
        ScheduleSaveClientSettings();
        if (_suppressAnCommand || !CanOperate()) return;
        _ = SendAnOnAsync(value);
    }

    private async Task SendAgcLevelAsync(int level)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetAgcLevelAsync(level).ConfigureAwait(true);
            AppendLog($"AGC → {AgcButtonText} ({level})");
        }
        catch (Exception ex) { AppendLog($"AGC error: {ex.Message}"); }
    }

    private async Task SendAgcFastReleaseAsync(int ms)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetAgcFastReleaseAsync(ms).ConfigureAwait(true);
            AppendLog($"AGC Fast Release {ms} ms");
        }
        catch (Exception ex) { AppendLog($"AGC fast error: {ex.Message}"); }
    }

    private async Task SendAmpAsync(bool on)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetPaBypassAsync(on).ConfigureAwait(true);
            AppendLog($"AMP {(on ? "ON (QRO)" : "OFF (QRP)")}");
        }
        catch (Exception ex) { AppendLog($"AMP error: {ex.Message}"); }
    }

    private async Task SendCompressionStateAsync(bool on)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetCompressionStateAsync(on).ConfigureAwait(true);
            AppendLog($"CMP {(on ? "ON" : "OFF")}");
        }
        catch (Exception ex) { AppendLog($"CMP error: {ex.Message}"); }
    }

    private async Task SendCompressionLevelAsync(int level)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetCompressionLevelAsync(level).ConfigureAwait(true);
            AppendLog($"CMP level {level}");
        }
        catch (Exception ex) { AppendLog($"CMP level error: {ex.Message}"); }
    }

    private async Task SendMonitorAsync(bool on)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetMonitorAsync(on).ConfigureAwait(true);
            AppendLog($"MON {(on ? "ON" : "OFF")}");
        }
        catch (Exception ex) { AppendLog($"MON error: {ex.Message}"); }
    }

    private async Task SendNbOnAsync(bool on)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetNbOnAsync(on).ConfigureAwait(true);
            AppendLog($"NB {(on ? "ON" : "OFF")}");
        }
        catch (Exception ex) { AppendLog($"NB error: {ex.Message}"); }
    }

    private async Task SendNbPulseAsync(int us)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetNbPulseWidthAsync(us).ConfigureAwait(true);
            AppendLog($"NB pulse {us} µs");
        }
        catch (Exception ex) { AppendLog($"NB pulse error: {ex.Message}"); }
    }

    private async Task SendNbThresholdAsync(int thr)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetNbThresholdAsync(thr).ConfigureAwait(true);
            AppendLog($"NB thr {thr}");
        }
        catch (Exception ex) { AppendLog($"NB thr error: {ex.Message}"); }
    }

    private async Task SendNrOnAsync(bool on)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetNrOnAsync(on, NrLevel).ConfigureAwait(true);
            AppendLog($"NR {(on ? $"ON level={NrLevel}" : "OFF")}");
        }
        catch (Exception ex) { AppendLog($"NR error: {ex.Message}"); }
    }

    private async Task SendNrLevelAsync(int level)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetNrLevelAsync(level).ConfigureAwait(true);
            AppendLog($"NR level {level}");
        }
        catch (Exception ex) { AppendLog($"NR level error: {ex.Message}"); }
    }

    private async Task SendAnOnAsync(bool on)
    {
        if (_radio == null) return;
        try
        {
            await _radio.SetAutoNotchOnAsync(on).ConfigureAwait(true);
            AppendLog($"AN {(on ? "ON" : "OFF")}");
        }
        catch (Exception ex) { AppendLog($"AN error: {ex.Message}"); }
    }

    private async Task ApplyFrequencyAsync(long hz, string reason)
    {
        // Update local VFO state even when offline
        if (UseVfoA)
        {
            _frequencyHz = hz;
            UpdateFrequencyUi(hz);
        }
        else
        {
            _vfoBFrequencyHz = hz;
            VfoBDisplayMhz = FormatMhz(hz);
        }

        BandText = BandNameForFrequency(hz);
        if (!string.Equals(BandText, "gen", StringComparison.OrdinalIgnoreCase) &&
            !reason.StartsWith("gen ", StringComparison.OrdinalIgnoreCase))
            _onGenBand = false;

        // Debounced last-used for on-air tuning only (not band switch / cal / IQ sessions)
        if (ShouldUpdateLastUsed(reason))
            ScheduleSaveLastUsed();

        if (_radio == null || !IsConnected)
        {
            ScheduleSaveClientSettings();
            return;
        }

        try
        {
            await _radio.SetFrequencyAsync(hz).ConfigureAwait(true);
            SyncFavoriteBandFilterFromRadio();
            StatusText = $"Freq {(UseVfoA ? "A" : "B")} {FormatMhz(hz)}";
            AppendLog($"Sent VFO{(UseVfoA ? "A" : "B")} freq {hz} [{reason}]");
            ScheduleSaveClientSettings();
        }
        catch (Exception ex)
        {
            StatusText = $"Freq failed: {ex.Message}";
            AppendLog($"ERROR: {ex.Message}");
        }
    }

    /// <summary>
    /// Last-used band memory is for MAIN on-air use only — not QRP/AMP cal or RX/TX IQ.
    /// </summary>
    private bool ShouldUpdateLastUsed(string reason)
    {
        if (_suppressLastUsedSave) return false;
        if (IsCalibrationOrIqSessionActive()) return false;
        if (reason.StartsWith("band ", StringComparison.OrdinalIgnoreCase)) return false;
        if (reason.StartsWith("qrp-cal", StringComparison.OrdinalIgnoreCase)) return false;
        if (reason.StartsWith("amp-cal", StringComparison.OrdinalIgnoreCase)) return false;
        if (reason.StartsWith("tx-iq", StringComparison.OrdinalIgnoreCase)) return false;
        if (reason.StartsWith("rx-iq", StringComparison.OrdinalIgnoreCase)) return false;
        if (reason.StartsWith("gen ", StringComparison.OrdinalIgnoreCase)) return false;
        return true;
    }

    private bool IsCalibrationOrIqSessionActive() =>
        PowerCalCalibrating || PowerCalTxOn || PowerCalAcceptPrompt
        || AmpCalCalibrating || AmpCalTxOn || AmpCalAcceptPrompt
        || TxIqTxOn || RxIqSessionActive
        || FreqCalInProgress || FreqCalManualMode;

    [RelayCommand]
    private Task SelectVfoA() => SelectVfoAsync(useVfoA: true);

    [RelayCommand]
    private Task SelectVfoB() => SelectVfoAsync(useVfoA: false);

    /// <summary>Activate VFO A or B: CMD_SET_VFO then push that VFO's freq/mode.</summary>
    public async Task SelectVfoAsync(bool useVfoA)
    {
        if (UseVfoA == useVfoA)
            return;

        // Remember last-used for the VFO we're leaving (not during cal/IQ)
        if (!IsCalibrationOrIqSessionActive())
            SaveLastUsedForCurrentBand();

        UseVfoA = useVfoA;
        OnPropertyChanged(nameof(UseVfoB));
        AppendLog($"Select VFO {(useVfoA ? "A" : "B")}");
        await PushActiveVfoToRadioAsync(force: false).ConfigureAwait(true);
        ScheduleSaveClientSettings();
    }

    /// <summary>CMD_SET_VFO + freq + mode for the UI-selected VFO.</summary>
    private async Task PushActiveVfoToRadioAsync(bool force)
    {
        long hz = UseVfoA ? _frequencyHz : _vfoBFrequencyHz;
        string mode = UseVfoA
            ? (string.IsNullOrWhiteSpace(ModeText) ? "USB" : ModeText)
            : (string.IsNullOrWhiteSpace(VfoBModeText) ? "USB" : VfoBModeText);

        BandText = BandNameForFrequency(hz);
        RefreshSpectrumFilterOverlay();
        SyncRfPowerFromMode(force: true);
        if (UseVfoA)
            NotifyModeFlags();

        if (!CanOperate() || _radio == null)
            return;

        try
        {
            byte vfo = UseVfoA ? Opcodes.VFO_A : Opcodes.VFO_B;
            await _radio.SetActiveVfoAsync(vfo).ConfigureAwait(true);
            await Task.Delay(10).ConfigureAwait(true);
            await _radio.SetFrequencyAsync(hz).ConfigureAwait(true);
            await _radio.SetModeAsync(mode).ConfigureAwait(true);
            StatusText = $"VFO {(UseVfoA ? "A" : "B")} active";
            AppendLog($"VFO {(UseVfoA ? "A" : "B")} → {FormatMhz(hz)} {mode}{(force ? " (connect)" : "")}");
        }
        catch (Exception ex)
        {
            AppendLog($"Push VFO error: {ex.Message}");
        }
    }

    partial void OnUseVfoAChanged(bool value) => OnPropertyChanged(nameof(UseVfoB));

    [RelayCommand(CanExecute = nameof(CanOperate))]
    private async Task SetModeAsync(string? mode)
    {
        if (_radio == null || string.IsNullOrWhiteSpace(mode)) return;
        string m = mode.Trim();

        // If TUN is latched, exit tune without restoring old mode — apply the user's choice.
        if (TuneMode && !string.Equals(m, "TUNE", StringComparison.OrdinalIgnoreCase))
        {
            try
            {
                await _radio.SetAutoTuneAsync(false).ConfigureAwait(true);
            }
            catch (Exception ex)
            {
                AppendLog($"TUN release error: {ex.Message}");
            }

            _suppressTransmitCommands = true;
            TuneMode = false;
            _suppressTransmitCommands = false;
            _modeBeforeTune = m;
        }

        try
        {
            await _radio.SetModeAsync(m).ConfigureAwait(true);
            if (UseVfoA)
            {
                ModeText = m;
                NotifyModeFlags();
            }
            else
            {
                VfoBModeText = m;
            }

            RefreshSpectrumFilterOverlay();
            SyncRfPowerFromMode();
            StatusText = $"Mode {m} (VFO {(UseVfoA ? "A" : "B")})";
            AppendLog($"Sent mode {m} VFO{(UseVfoA ? "A" : "B")}");
            ScheduleSaveClientSettings();
        }
        catch (Exception ex)
        {
            StatusText = $"Mode failed: {ex.Message}";
            AppendLog($"ERROR: {ex.Message}");
        }
    }

    [RelayCommand]
    private async Task SelectBandAsync(string? bandKey)
    {
        if (string.IsNullOrWhiteSpace(bandKey)) return;
        if (!TryBandDefaultHz(bandKey, out long defaultHz, out string label)) return;

        // Respect radio-model gating (disabled buttons should not fire)
        string k = bandKey.Trim().ToLowerInvariant();
        if ((k is "2200" or "630") && !LfBandsEnabled) return;
        if (k is not ("2200" or "630") && !HfBandsEnabled) return;

        // Remember where we were on the band we're leaving (e.g. 14.074 on 20m) — not during cal/IQ
        if (!IsCalibrationOrIqSessionActive())
            SaveLastUsedForCurrentBand();

        bool forVfoB = !UseVfoA;
        var (lastFreq, lastMode, lastLow, lastHigh, lastCw) =
            BandLastUsedStore.Load(label, forVfoB);
        long hz = defaultHz;
        if (lastFreq is >= 10_000 and <= 60_000_000)
        {
            string recallBand = BandNameForFrequency(lastFreq);
            // Accept if still maps to this band (or unknown edge — keep last freq)
            if (string.Equals(NormalizeBandLabel(recallBand), NormalizeBandLabel(label), StringComparison.OrdinalIgnoreCase)
                || recallBand is "?" or "—")
                hz = lastFreq;
        }

        string mode = !string.IsNullOrWhiteSpace(lastMode)
            ? lastMode
            : DefaultModeForFrequency(hz);

        _onGenBand = false;
        _suppressLastUsedSave = true;
        try
        {
            await ApplyFrequencyAsync(hz, $"band {label}").ConfigureAwait(true);
            if (CanOperate())
                await SetModeAsync(mode).ConfigureAwait(true);
            else
            {
                ModeText = mode;
                NotifyModeFlags();
            }

            if (lastLow >= 0 || lastHigh >= 0 || lastCw >= 0)
            {
                int lo = lastLow >= 0 ? lastLow : _lowCutIndex;
                int hi = lastHigh >= 0 ? lastHigh : _highCutIndex;
                int cw = lastCw >= 0 ? lastCw : _cwFilterIndex;
                ApplyFilterIndices(lo, hi, cw, send: CanOperate());
            }

            BandText = label;
        }
        finally
        {
            _suppressLastUsedSave = false;
        }

        SaveLastUsedForCurrentBand();
        ScheduleSaveClientSettings();
    }

    /// <summary>Persist last-used freq/mode/filters for the active VFO's current band.</summary>
    private void SaveLastUsedForCurrentBand()
    {
        if (_suppressLastUsedSave) return;
        if (IsCalibrationOrIqSessionActive()) return;
        string band = BandText ?? "";
        if (string.IsNullOrWhiteSpace(band) || band is "—" or "?" or "-")
            return;
        // GEN beacons: do not overwrite ham-band last-used
        if (string.Equals(band, "gen", StringComparison.OrdinalIgnoreCase))
            return;

        long hz = UseVfoA ? _frequencyHz : _vfoBFrequencyHz;
        string mode = UseVfoA
            ? (string.IsNullOrWhiteSpace(ModeText) ? "USB" : ModeText)
            : (string.IsNullOrWhiteSpace(VfoBModeText) ? "USB" : VfoBModeText);

        string bandCopy = band;
        string modeCopy = mode;
        int low = _lowCutIndex, high = _highCutIndex, cw = _cwFilterIndex;
        bool vfoB = !UseVfoA;
        // Disk I/O off UI thread
        _ = Task.Run(() =>
            BandLastUsedStore.Save(bandCopy, hz, modeCopy, low, high, cw, forVfoB: vfoB));
    }

    private void ScheduleSaveLastUsed()
    {
        if (_suppressLastUsedSave || IsCalibrationOrIqSessionActive()) return;
        if (_lastUsedSaveTimer == null)
        {
            _lastUsedSaveTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(500) };
            _lastUsedSaveTimer.Tick += (_, _) =>
            {
                _lastUsedSaveTimer.Stop();
                SaveLastUsedForCurrentBand();
            };
        }

        _lastUsedSaveTimer.Stop();
        _lastUsedSaveTimer.Start();
    }

    private static string NormalizeBandLabel(string? band)
    {
        string b = (band ?? "").Trim().ToLowerInvariant();
        if (b.EndsWith('m') && b.Length > 1) return b;
        return b;
    }

    // ----- Radio model + GEN -----

    partial void OnIsGeminusRadioModelChanged(bool value)
    {
        OnPropertyChanged(nameof(RadioModelButtonText));
        OnPropertyChanged(nameof(HfBandsEnabled));
        OnPropertyChanged(nameof(LfBandsEnabled));
        OnPropertyChanged(nameof(GenButtonTip));
    }

    [RelayCommand]
    private async Task ToggleRadioModelAsync()
    {
        IsGeminusRadioModel = !IsGeminusRadioModel;
        // Swap S/W bank with radio type (capture leaving bank, load entering bank)
        SpectrumDisplaySettings.Instance.SwitchRadioModel(IsGeminusRadioModel);
        AppendLog(IsGeminusRadioModel
            ? "Radio model: Geminus — LF bands on; HF grayed; GEN=198/660/880; S/W→LF bank"
            : "Radio model: Proficio — HF bands on; LF grayed; GEN=WWV/CHU/RWM/USER; S/W→HF bank");

        SyncGenButtonForRadioModel();
        if (_onGenBand)
            await ApplyGenAsync(rotate: false).ConfigureAwait(true);

        ScheduleSaveClientSettings();
    }

    [RelayCommand]
    private Task SelectGen() => ApplyGenAsync(rotate: _onGenBand);

    /// <summary>
    /// GEN band: first press uses current slot; while on GEN, each press rotates presets.
    /// </summary>
    private async Task ApplyGenAsync(bool rotate)
    {
        var opts = IsGeminusRadioModel ? GenOptionsGeminus : GenOptionsProficio;
        int idx = IsGeminusRadioModel ? _genIndexGeminus : _genIndexProficio;
        idx = Math.Clamp(idx, 0, opts.Length - 1);

        if (rotate)
            idx = (idx + 1) % opts.Length;

        if (IsGeminusRadioModel)
            _genIndexGeminus = idx;
        else
            _genIndexProficio = idx;

        var opt = opts[idx];
        GenButtonText = opt.Label;
        _onGenBand = true;

        if (!CanOperate())
        {
            _frequencyHz = opt.Freq;
            UpdateFrequencyUi(opt.Freq);
            BandText = "gen";
            ModeText = DefaultModeForFrequency(opt.Freq);
            NotifyModeFlags();
            AppendLog($"GEN {opt.Label} @ {opt.Freq} (not connected)");
            ScheduleSaveClientSettings();
            return;
        }

        await ApplyFrequencyAsync(opt.Freq, $"gen {opt.Label}").ConfigureAwait(true);
        await SetModeAsync(DefaultModeForFrequency(opt.Freq)).ConfigureAwait(true);
        BandText = "gen";
        string model = IsGeminusRadioModel ? "Geminus" : "Proficio";
        AppendLog(rotate
            ? $"GEN ({model}) rotated → {opt.Label} @ {opt.Freq}"
            : $"GEN ({model}) → {opt.Label} @ {opt.Freq}");
        ScheduleSaveClientSettings();
    }

    private void SyncGenButtonForRadioModel()
    {
        var opts = IsGeminusRadioModel ? GenOptionsGeminus : GenOptionsProficio;
        int idx = IsGeminusRadioModel ? _genIndexGeminus : _genIndexProficio;
        idx = Math.Clamp(idx, 0, opts.Length - 1);
        if (IsGeminusRadioModel)
            _genIndexGeminus = idx;
        else
            _genIndexProficio = idx;
        GenButtonText = opts[idx].Label;
    }

    // ----- Sticky client settings -----

    private void LoadClientSettings()
    {
        _suppressSettingsSave = true;
        try
        {
            var s = ClientSettingsStore.Load();
            Host = string.IsNullOrWhiteSpace(s.Host) ? Host : s.Host;
            RemotePortText = string.IsNullOrWhiteSpace(s.RemotePortText) ? RemotePortText : s.RemotePortText;
            LocalPortText = string.IsNullOrWhiteSpace(s.LocalPortText) ? LocalPortText : s.LocalPortText;
            IsGeminusRadioModel = s.IsGeminusRadioModel;
            _genIndexProficio = Math.Clamp(s.GenIndexProficio, 0, GenOptionsProficio.Length - 1);
            _genIndexGeminus = Math.Clamp(s.GenIndexGeminus, 0, GenOptionsGeminus.Length - 1);
            SyncGenButtonForRadioModel();

            if (s.LastFrequencyHz is >= 10_000 and <= 60_000_000)
            {
                _frequencyHz = s.LastFrequencyHz;
                UpdateFrequencyUi(_frequencyHz);
            }

            if (!string.IsNullOrWhiteSpace(s.LastMode))
            {
                ModeText = s.LastMode.Trim();
                NotifyModeFlags();
            }

            if (s.LastVfoBFrequencyHz is >= 10_000 and <= 60_000_000)
            {
                _vfoBFrequencyHz = s.LastVfoBFrequencyHz;
                VfoBDisplayMhz = FormatMhz(_vfoBFrequencyHz);
            }

            if (!string.IsNullOrWhiteSpace(s.LastVfoBMode))
                VfoBModeText = s.LastVfoBMode.Trim();

            UseVfoA = s.UseVfoA;
            OnPropertyChanged(nameof(UseVfoB));

            // Operate UI state (pushed to radio on Connect)
            _stepIndex = Math.Clamp(s.StepIndex, 0, StepChoicesHz.Length - 1);
            StepLabel = FormatStep(StepChoicesHz[_stepIndex]);
            ApplyFilterIndices(s.LowCutIndex, s.HighCutIndex, s.CwFilterIndex, send: false);

            _suppressAudioSend = true;
            PVolume = Math.Clamp(s.PVolume, 0, 100);
            PMicGain = Math.Clamp(s.PMicGain, 0, 100);
            DVolume = Math.Clamp(s.DVolume, 0, 100);
            DMicGain = Math.Clamp(s.DMicGain, 0, 100);
            IsDigitalAudio = s.IsDigitalAudio;
            RemoteAudio = s.RemoteAudio;
            _suppressAudioSend = false;
            OnPropertyChanged(nameof(RemoteAudioCheckboxEnabled));

            _suppressRitSend = true;
            RitOffset = s.RitOffset;
            RitOn = s.RitOn;
            _suppressRitSend = false;

            _suppressCwSend = true;
            CwKeyerMode = Math.Clamp(s.CwKeyerMode, 0, Math.Max(0, CwKeyerModeOptions.Count - 1));
            CwSpacing = Math.Clamp(s.CwSpacing, 0, Math.Max(0, CwSpacingOptions.Count - 1));
            CwPaddle = Math.Clamp(s.CwPaddle, 0, Math.Max(0, CwPaddleOptions.Count - 1));
            CwWeightIndex = Math.Clamp(s.CwWeightIndex, 0, CwWeightValues.Length - 1);
            CwPitchIndex = Math.Clamp(s.CwPitchIndex, 0, Math.Max(0, CwPitchOptions.Count - 1));
            CwPitchLabel = CwPitchOptions[CwPitchIndex];
            CwHold = Math.Clamp(s.CwHold, 1, 500);
            CwQsk = s.CwQsk;
            CwPhones = s.CwPhones;
            CwSpeed = Math.Clamp(s.CwSpeed, 5, 60);
            KeyerMem0 = s.KeyerMem0 ?? "";
            KeyerMem1 = s.KeyerMem1 ?? "";
            KeyerMem2 = s.KeyerMem2 ?? "";
            KeyerMem3 = s.KeyerMem3 ?? "";
            ExternalElectronicKeyer = s.ExternalElectronicKeyer;
            _suppressCwSend = false;
            OnPropertyChanged(nameof(PicKeyerControlsEnabled));
            OnPropertyChanged(nameof(KeyerMemPanelEnabled));
            try
            {
                MsccIniProficio.WriteProficioMkii(mkii: !ExternalElectronicKeyer);
            }
            catch { /* best-effort */ }

            _suppressPowerSend = true;
            TunePowerPercent = Math.Clamp(s.TunePowerPercent, 0, 100);
            CwPowerPercent = Math.Clamp(s.CwPowerPercent, 0, 100);
            SsbPowerPercent = Math.Clamp(s.SsbPowerPercent, 0, 100);
            AmCarrierPercent = Math.Clamp(s.AmCarrierPercent, 0, 100);
            _suppressPowerSend = false;
            SyncRfPowerFromMode();

            _suppressCompressionCommand = true;
            Compression = Math.Clamp(s.Compression, 0, 24);
            CompressionOn = s.CompressionOn && !s.IsDigitalAudio;
            _sessionCompressionOn = s.CompressionOn;
            _suppressCompressionCommand = false;

            _suppressAgcCommand = true;
            AgcLevel = Math.Clamp(s.AgcLevel, 0, 2);
            AgcFastRelease = Math.Clamp(s.AgcFastRelease, 0, 1000);
            _suppressAgcCommand = false;

            _suppressNbCommand = true;
            NbOn = s.NbOn;
            NbPulse = Math.Clamp(s.NbPulse, 10, 510);
            NbThreshold = Math.Clamp(s.NbThreshold, 1, 1009);
            _suppressNbCommand = false;

            _suppressNrCommand = true;
            NrOn = s.NrOn;
            NrLevel = Math.Clamp(s.NrLevel, 0, 100);
            _suppressNrCommand = false;

            _suppressAnCommand = true;
            AnOn = s.AnOn;
            _suppressAnCommand = false;

            _suppressMonitorCommand = true;
            MonitorOn = s.MonitorOn;
            _suppressMonitorCommand = false;

            _suppressAmpCommand = true;
            AmpOn = s.AmpOn;
            _suppressAmpCommand = false;

            QrpMode = s.QrpMode;
            FullPower = s.FullPower;
            AlcOn = s.AlcOn;

            long activeHz = UseVfoA ? _frequencyHz : _vfoBFrequencyHz;
            BandText = BandNameForFrequency(activeHz);

            // Dual HF/LF S/W banks + global zoom / dB CAL
            SpectrumDisplaySettings.Instance.LoadBanks(
                s.HfBank,
                s.LfBank,
                s.IsGeminusRadioModel,
                s.DbCalRelative,
                s.SpectrumZoom);
            SpectrumZoom = SpectrumDisplaySettings.Instance.ZoomFactor;

            AppearanceSettings.Instance.LoadFrom(
                s.SpectrumBackground,
                s.SpectrumBackgroundRgb,
                s.SpectrumFill,
                s.SpectrumLine,
                s.UiBackground,
                s.UiBackgroundRgb,
                s.UiButton,
                s.UiButtonRgb,
                s.UiPanel);
            SyncAppearanceUiFromSettings();

            FavoriteBandFilter = NormalizeFavoriteBand(BandText, activeHz);
            AppendLog(
                $"Loaded settings (model={(IsGeminusRadioModel ? "Geminus" : "Proficio")}, " +
                $"VFO={(UseVfoA ? "A" : "B")}, S/W bank={(IsGeminusRadioModel ? "LF" : "HF")})");
        }
        finally
        {
            _suppressSettingsSave = false;
        }
    }

    private void ScheduleSaveClientSettings()
    {
        if (_suppressSettingsSave) return;
        if (_settingsSaveTimer == null)
        {
            _settingsSaveTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(400) };
            _settingsSaveTimer.Tick += (_, _) =>
            {
                _settingsSaveTimer.Stop();
                SaveClientSettingsNow();
            };
        }

        _settingsSaveTimer.Stop();
        _settingsSaveTimer.Start();
    }

    private void SaveClientSettingsNow()
    {
        if (_suppressSettingsSave) return;
        var sw = SpectrumDisplaySettings.Instance;
        var app = AppearanceSettings.Instance;
        sw.CaptureLiveToActiveBank();
        // Snapshot on UI thread, write INI off UI thread
        var s = new ClientSettings
        {
            Host = Host ?? "127.0.0.1",
            RemotePortText = RemotePortText ?? "8888",
            LocalPortText = LocalPortText ?? "8889",
            IsGeminusRadioModel = IsGeminusRadioModel,
            LastFrequencyHz = _frequencyHz,
            LastMode = string.IsNullOrWhiteSpace(ModeText) ? "USB" : ModeText,
            LastVfoBFrequencyHz = _vfoBFrequencyHz,
            LastVfoBMode = string.IsNullOrWhiteSpace(VfoBModeText) ? "USB" : VfoBModeText,
            UseVfoA = UseVfoA,
            StepIndex = _stepIndex,
            LowCutIndex = _lowCutIndex,
            HighCutIndex = _highCutIndex,
            CwFilterIndex = _cwFilterIndex,
            PVolume = PVolume,
            PMicGain = PMicGain,
            DVolume = DVolume,
            DMicGain = DMicGain,
            IsDigitalAudio = IsDigitalAudio,
            RemoteAudio = RemoteAudio,
            RitOn = RitOn,
            RitOffset = RitOffset,
            CwKeyerMode = CwKeyerMode,
            CwSpacing = CwSpacing,
            CwPaddle = CwPaddle,
            CwWeightIndex = CwWeightIndex,
            CwPitchIndex = CwPitchIndex,
            CwHold = CwHold,
            CwQsk = CwQsk,
            CwPhones = CwPhones,
            CwSpeed = CwSpeed,
            KeyerMem0 = SanitizeKeyerMem(KeyerMem0),
            KeyerMem1 = SanitizeKeyerMem(KeyerMem1),
            KeyerMem2 = SanitizeKeyerMem(KeyerMem2),
            KeyerMem3 = SanitizeKeyerMem(KeyerMem3),
            ExternalElectronicKeyer = ExternalElectronicKeyer,
            TunePowerPercent = TunePowerPercent,
            CwPowerPercent = CwPowerPercent,
            SsbPowerPercent = SsbPowerPercent,
            AmCarrierPercent = AmCarrierPercent,
            Compression = Compression,
            CompressionOn = _sessionCompressionOn || CompressionOn,
            AgcLevel = AgcLevel,
            AgcFastRelease = AgcFastRelease,
            NbOn = NbOn,
            NbPulse = NbPulse,
            NbThreshold = NbThreshold,
            NrOn = NrOn,
            NrLevel = NrLevel,
            AnOn = AnOn,
            MonitorOn = MonitorOn,
            AmpOn = AmpOn,
            QrpMode = QrpMode,
            FullPower = FullPower,
            AlcOn = AlcOn,
            SpectrumZoom = sw.ZoomFactor,
            DbCalRelative = sw.DbCalRelative,
            GridMaxDb = sw.GridMaxDb,
            GridMinDb = sw.GridMinDb,
            WaterfallHighDb = sw.WaterfallHighDb,
            WaterfallLowDb = sw.WaterfallLowDb,
            ViewGrid = sw.ViewGrid,
            ShowWaterfall = sw.ShowWaterfall,
            WaterfallDirectionNormal = sw.WaterfallDirectionNormal,
            WaterfallPalette = sw.WaterfallPalette,
            HfBank = sw.HfBank.Clone(),
            LfBank = sw.LfBank.Clone(),
            SpectrumBackground = app.SpectrumBackground,
            SpectrumBackgroundRgb = app.SpectrumBackgroundRgb,
            SpectrumFill = app.SpectrumFill,
            SpectrumLine = app.SpectrumLine,
            UiBackground = app.UiBackground,
            UiBackgroundRgb = app.UiBackgroundRgb,
            UiButton = app.UiButton,
            UiButtonRgb = app.UiButtonRgb,
            UiPanel = app.UiPanel,
            GenIndexProficio = _genIndexProficio,
            GenIndexGeminus = _genIndexGeminus,
        };
        _ = Task.Run(() => ClientSettingsStore.Save(s));
    }

    /// <summary>
    /// After connect: push sticky operate controls to ms-sdr.
    /// PTT/TUN never restored. Failures are logged but do not drop the session.
    /// </summary>
    private async Task PushStickyOperateToRadioAsync()
    {
        if (_radio == null || !IsConnected) return;
        AppendLog("Restoring sticky operate settings to radio…");

        try
        {
            // Filters + step
            ApplyFilterIndices(_lowCutIndex, _highCutIndex, _cwFilterIndex, send: true);
            try { await _radio.SetStepAsync(_stepIndex).ConfigureAwait(true); }
            catch (Exception ex) { AppendLog($"Step restore: {ex.Message}"); }

            // Audio path + levels
            try
            {
                byte device = ResolveAudioDeviceOpcode();
                await _radio.SetAudioDeviceAsync(device).ConfigureAwait(true);
                await _radio.SetPhonesVolumeLevelAsync(Math.Clamp(PVolume, 0, 100)).ConfigureAwait(true);
                await _radio.SetPhonesMicGainLevelAsync(Math.Clamp(PMicGain, 0, 100)).ConfigureAwait(true);
                await _radio.SetDigitalVolumeLevelAsync(Math.Clamp(DVolume, 0, 100)).ConfigureAwait(true);
                await _radio.SetDigitalMicGainLevelAsync(Math.Clamp(DMicGain, 0, 100)).ConfigureAwait(true);
            }
            catch (Exception ex) { AppendLog($"Audio restore: {ex.Message}"); }

            // Power banks
            try
            {
                await _radio.SetTunePowerAsync(Math.Clamp(TunePowerPercent, 0, 100)).ConfigureAwait(true);
                await _radio.SetCwPowerAsync(Math.Clamp(CwPowerPercent, 0, 100)).ConfigureAwait(true);
                await _radio.SetSsbPowerAsync(Math.Clamp(SsbPowerPercent, 0, 100)).ConfigureAwait(true);
                await _radio.SetAmCarrierAsync(Math.Clamp(AmCarrierPercent, 0, 100)).ConfigureAwait(true);
            }
            catch (Exception ex) { AppendLog($"Power restore: {ex.Message}"); }

            // CW
            try
            {
                await _radio.SetCwWpmAsync(Math.Clamp(CwSpeed, 5, 60)).ConfigureAwait(true);
                await _radio.SetCwKeyerModeAsync(Math.Clamp(CwKeyerMode, 0, 3)).ConfigureAwait(true);
                await _radio.SetCwSpacingAsync(Math.Clamp(CwSpacing, 0, 2)).ConfigureAwait(true);
                await _radio.SetCwPaddleAsync(Math.Clamp(CwPaddle, 0, 1)).ConfigureAwait(true);
                int weight = CwWeightValues[Math.Clamp(CwWeightIndex, 0, CwWeightValues.Length - 1)];
                await _radio.SetCwWeightAsync(weight).ConfigureAwait(true);
                await _radio.SetCwPitchAsync(Math.Clamp(CwPitchIndex, 0, 3)).ConfigureAwait(true);
                await _radio.SetCwTxHoldAsync(Math.Clamp(CwHold, 1, 500)).ConfigureAwait(true);
                await _radio.SetCwQskAsync(CwQsk).ConfigureAwait(true);
                await _radio.SetCwPhonesAsync(CwPhones).ConfigureAwait(true);
            }
            catch (Exception ex) { AppendLog($"CW restore: {ex.Message}"); }

            // AGC / CMP / NB / NR / AN / MON
            try
            {
                await _radio.SetAgcLevelAsync(Math.Clamp(AgcLevel, 0, 2)).ConfigureAwait(true);
                await _radio.SetAgcFastReleaseAsync(Math.Clamp(AgcFastRelease, 0, 1000)).ConfigureAwait(true);
                await _radio.SetCompressionLevelAsync(Math.Clamp(Compression, 0, 24)).ConfigureAwait(true);
                bool cmp = CompressionOn && !IsDigitalAudio;
                await _radio.SetCompressionStateAsync(cmp).ConfigureAwait(true);
                await _radio.SetNbPulseWidthAsync(Math.Clamp(NbPulse, 10, 510)).ConfigureAwait(true);
                await _radio.SetNbThresholdAsync(Math.Clamp(NbThreshold, 1, 1009)).ConfigureAwait(true);
                await _radio.SetNbOnAsync(NbOn).ConfigureAwait(true);
                await _radio.SetNrOnAsync(NrOn, Math.Clamp(NrLevel, 0, 100)).ConfigureAwait(true);
                if (NrOn)
                    await _radio.SetNrLevelAsync(Math.Clamp(NrLevel, 0, 100)).ConfigureAwait(true);
                await _radio.SetAutoNotchOnAsync(AnOn).ConfigureAwait(true);
                await _radio.SetMonitorAsync(MonitorOn).ConfigureAwait(true);
            }
            catch (Exception ex) { AppendLog($"DSP restore: {ex.Message}"); }

            // AMP path (PA bypass) — after power banks
            try
            {
                await _radio.SetPaBypassAsync(AmpOn).ConfigureAwait(true);
            }
            catch (Exception ex) { AppendLog($"AMP restore: {ex.Message}"); }

            // Optional power modes if Core accepts them
            try
            {
                await _radio.SetQrpModeAsync(QrpMode).ConfigureAwait(true);
                await _radio.SetFullPowerAsync(FullPower).ConfigureAwait(true);
                await _radio.SetAlcOnAsync(AlcOn).ConfigureAwait(true);
            }
            catch (Exception ex) { AppendLog($"Power-mode restore: {ex.Message}"); }

            // RIT last
            try
            {
                await _radio.SetRitAsync(RitOn, RitOffset).ConfigureAwait(true);
            }
            catch (Exception ex) { AppendLog($"RIT restore: {ex.Message}"); }

            AppendLog("Sticky operate settings restored.");
        }
        catch (Exception ex)
        {
            AppendLog($"Operate restore incomplete: {ex.Message}");
        }
    }

    private void LoadFavoritesFromStore()
    {
        Favorites.Clear();
        foreach (var e in FavoritesStore.Load())
        {
            e.Band = NormalizeFavoriteBand(e.Band, e.FrequencyHz);
            ApplyFavoriteLabels(e);
            Favorites.Add(e);
        }
        RefreshFavoritesForBand();
        AppendLog($"Favorites loaded: {Favorites.Count}");
    }

    private void PersistFavorites()
    {
        FavoritesStore.Save(Favorites);
    }

    partial void OnHostChanged(string value) => ScheduleSaveClientSettings();
    partial void OnRemotePortTextChanged(string value) => ScheduleSaveClientSettings();
    partial void OnLocalPortTextChanged(string value) => ScheduleSaveClientSettings();

    // ----- UI appearance (SETTINGS tab) -----
    public string[] UiBackgroundNames => UiChromeTheme.ColorNames;
    public string[] UiPanelNames => UiChromeTheme.PanelColorNames;
    public string[] UiButtonNames => UiChromeTheme.ColorNames;

    [ObservableProperty] private string _selectedUiBackground = "BLACK";
    [ObservableProperty] private string _selectedUiPanel = "AUTO";
    [ObservableProperty] private string _selectedUiButton = "YELLOW";
    [ObservableProperty] private string _uiBackgroundRgbText = "#1C1C1C";
    [ObservableProperty] private string _uiButtonRgbText = "#FFCC00";
    [ObservableProperty] private string _uiPanelRgbText = "#2A2A2A";
    [ObservableProperty] private int _uiBgR = 0x1C;
    [ObservableProperty] private int _uiBgG = 0x1C;
    [ObservableProperty] private int _uiBgB = 0x1C;
    [ObservableProperty] private int _uiBtnR = 0xFF;
    [ObservableProperty] private int _uiBtnG = 0xCC;
    [ObservableProperty] private int _uiBtnB;
    [ObservableProperty] private bool _uiPanelListEnabled = true;
    [ObservableProperty] private bool _showUiBackgroundRgb;
    [ObservableProperty] private bool _showUiButtonRgb;

    partial void OnSelectedUiBackgroundChanged(string value)
    {
        if (_loadingAppearance || string.IsNullOrWhiteSpace(value)) return;
        if (UiChromeTheme.IsCustom(value))
        {
            AppearanceSettings.Instance.SetUiBackgroundRgb((byte)UiBgR, (byte)UiBgG, (byte)UiBgB);
        }
        else
        {
            AppearanceSettings.Instance.SetUiBackground(value);
        }
    }

    partial void OnSelectedUiPanelChanged(string value)
    {
        if (_loadingAppearance || string.IsNullOrWhiteSpace(value)) return;
        AppearanceSettings.Instance.SetUiPanel(value);
    }

    partial void OnSelectedUiButtonChanged(string value)
    {
        if (_loadingAppearance || string.IsNullOrWhiteSpace(value)) return;
        if (UiChromeTheme.IsCustom(value))
        {
            AppearanceSettings.Instance.SetUiButtonRgb((byte)UiBtnR, (byte)UiBtnG, (byte)UiBtnB);
        }
        else
        {
            AppearanceSettings.Instance.SetUiButton(value);
        }
    }

    partial void OnUiBgRChanged(int value) => ApplyUiBackgroundRgbFromSliders();
    partial void OnUiBgGChanged(int value) => ApplyUiBackgroundRgbFromSliders();
    partial void OnUiBgBChanged(int value) => ApplyUiBackgroundRgbFromSliders();
    partial void OnUiBtnRChanged(int value) => ApplyUiButtonRgbFromSliders();
    partial void OnUiBtnGChanged(int value) => ApplyUiButtonRgbFromSliders();
    partial void OnUiBtnBChanged(int value) => ApplyUiButtonRgbFromSliders();

    private void ApplyUiBackgroundRgbFromSliders()
    {
        if (_loadingAppearance) return;
        if (!UiChromeTheme.IsCustom(SelectedUiBackground)) return;
        AppearanceSettings.Instance.SetUiBackgroundRgb(
            (byte)Math.Clamp(UiBgR, 0, 255),
            (byte)Math.Clamp(UiBgG, 0, 255),
            (byte)Math.Clamp(UiBgB, 0, 255));
    }

    private void ApplyUiButtonRgbFromSliders()
    {
        if (_loadingAppearance) return;
        if (!UiChromeTheme.IsCustom(SelectedUiButton)) return;
        AppearanceSettings.Instance.SetUiButtonRgb(
            (byte)Math.Clamp(UiBtnR, 0, 255),
            (byte)Math.Clamp(UiBtnG, 0, 255),
            (byte)Math.Clamp(UiBtnB, 0, 255));
    }

    [RelayCommand]
    private void ResetUiChrome()
    {
        AppearanceSettings.Instance.ResetUiChromeDefaults();
        SyncAppearanceUiFromSettings();
        ScheduleSaveClientSettings();
    }

    [RelayCommand]
    private void ClearLog()
    {
        LogLines.Clear();
        try
        {
            DebugMonitor.ResetLogFile();
        }
        catch
        {
            // ignore
        }

        // Always show the clear marker even if pause is on
        bool paused = LogUiPaused;
        LogUiPaused = false;
        AppendLog("Log cleared (RESET LOGS).");
        LogUiPaused = paused;
    }

    // ----- Radio events -----

    private void WireRadioEvents(UdpRadioService radio)
    {
        radio.PacketReceived += e =>
        {
            int pkts = Interlocked.Increment(ref _packetsReceived);
            if (e.Opcode == Opcodes.CMD_SET_KEEP_ALIVE)
                Interlocked.Increment(ref _keepAlivesReceived);
            else if (e.Opcode == Opcodes.CMD_GET_SET_PANADAPTER)
            {
                int d5 = Interlocked.Increment(ref _panPacketsReceived);
                if (d5 == 1)
                    PostToUi(() => AppendLog("Panadapter packets arriving (0xD5)…"));
            }
            // Throttle stats UI — every packet under pan flood saturates the dispatcher.
            if (pkts == 1 || (pkts % 25) == 0)
                PostToUi(UpdatePacketStats);
        };

        radio.FrequencyReported += hz =>
            PostToUi(() =>
            {
                if (UseVfoA)
                {
                    _frequencyHz = hz;
                    UpdateFrequencyUi(hz);
                }
                else
                {
                    _vfoBFrequencyHz = hz;
                    VfoBDisplayMhz = FormatMhz(hz);
                }
            });

        radio.ModeReported += mode =>
            PostToUi(() =>
            {
                if (UseVfoA)
                {
                    ModeText = mode;
                    NotifyModeFlags();
                }
                else
                {
                    VfoBModeText = mode;
                }
                RefreshSpectrumFilterOverlay();
                AppendLog($"Mode reported: {mode} (VFO {(UseVfoA ? "A" : "B")})");
            });

        radio.DefaultLowCutIndexReported += idx =>
            PostToUi(() => ApplyReportedDefaultIndex(() =>
            {
                LowCutDefaultIndex = Math.Clamp(idx, 0, LowCutOptions.Count - 1);
                AppendLog($"Default Lo cut reported: {LowCutDefaultIndex}");
            }));

        radio.DefaultHighCutIndexReported += idx =>
            PostToUi(() => ApplyReportedDefaultIndex(() =>
            {
                HighCutDefaultIndex = Math.Clamp(idx, 0, HighCutOptions.Count - 1);
                AppendLog($"Default Hi cut reported: {HighCutDefaultIndex}");
            }));

        radio.DefaultCwFilterIndexReported += idx =>
            PostToUi(() => ApplyReportedDefaultIndex(() =>
            {
                CwFilterDefaultIndex = Math.Clamp(idx, 0, CwFilterOptions.Count - 1);
                AppendLog($"Default CW filter reported: {CwFilterDefaultIndex}");
            }));

        radio.DefaultTxIndexReported += idx =>
            PostToUi(() => ApplyReportedDefaultIndex(() =>
            {
                TxDefaultIndex = Math.Clamp(idx, 0, TxOptions.Count - 1);
                AppendLog($"Default TX reported: {TxDefaultIndex}");
            }));

        radio.SmeterReported += dbm =>
            PostToUi(() => ApplySmeterSample(dbm));

        radio.AlcReported += a =>
            PostToUi(() => ApplyAlcMeterSample(a));

        radio.BandReported += band =>
            PostToUi(() =>
            {
                BandText = band;
                SyncFavoriteBandFilterFromRadio();
                AppendLog($"Band reported: {band}");
            });

        radio.BandPowerReported += step =>
            PostToUi(() => ApplyBandPowerReport(step));

        radio.IqOperationCompleteReported += op =>
            PostToUi(() => OnIqOperationComplete(op));

        radio.IqValueReported += v =>
            PostToUi(() => OnIqValueReported(v));

        radio.CalProgressReported += v =>
            PostToUi(() => FreqCalProgress = Math.Clamp(v, 0, 100));

        radio.CalStatusReported += v =>
            PostToUi(() => OnFreqCalStatusReported(v));

        radio.CalDeltaReported += v =>
            PostToUi(() => OnFreqCalDeltaReported(v));

        radio.CoreVersionReported += v =>
            PostToUi(() =>
            {
                CoreVersionText = v;
                AppendLog($"Core: {v}");
            });

        radio.FirmwareVersionReported += v =>
            PostToUi(() =>
            {
                FirmwareText = v;
                AppendLog($"FW: {v}");
            });

        radio.TunePowerReported += v =>
            PostToUi(() => ApplyReportedPower(() =>
            {
                TunePowerPercent = Math.Clamp(v, 0, 100);
                AppendLog($"Tune power reported: {v}%");
            }));

        radio.CwPowerReported += v =>
            PostToUi(() => ApplyReportedPower(() =>
            {
                CwPowerPercent = Math.Clamp(v, 0, 100);
                AppendLog($"CW power reported: {v}%");
            }));

        radio.SsbPowerReported += v =>
            PostToUi(() => ApplyReportedPower(() =>
            {
                SsbPowerPercent = Math.Clamp(v, 0, 100);
                AppendLog($"SSB power reported: {v}%");
            }));

        radio.AmCarrierReported += v =>
            PostToUi(() => ApplyReportedPower(() =>
            {
                AmCarrierPercent = Math.Clamp(v, 0, 100);
                AppendLog($"AM carrier reported: {v}%");
            }));

        radio.TxSetByServerReported += v =>
            PostToUi(() =>
            {
                _suppressTransmitCommands = true;
                TxSetByServer = v;
                PttOn = v;
                TuneMode = v;
                _suppressTransmitCommands = false;
                AppendLog($"TxSetByServer: {v} (user PTT/TUN {(v ? "locked" : "unlocked")})");
                if (v)
                    StatusText = "Server controls TX — PTT/TUN locked";
            });

        radio.PaBypassReported += ampOn =>
            PostToUi(() =>
            {
                _suppressAmpCommand = true;
                AmpOn = ampOn;
                _suppressAmpCommand = false;
                AppendLog($"AMP from server: {(ampOn ? "ON" : "OFF")}");
            });

        radio.AgcLevelReported += level =>
            PostToUi(() =>
            {
                _suppressAgcCommand = true;
                AgcLevel = Math.Clamp(level, 0, 2);
                _suppressAgcCommand = false;
                AppendLog($"AGC from server: {AgcButtonText}");
            });

        radio.AgcFastReleaseReported += ms =>
            PostToUi(() =>
            {
                _suppressAgcCommand = true;
                AgcFastRelease = ms;
                _suppressAgcCommand = false;
            });

        radio.CompressionStateReported += on =>
            PostToUi(() =>
            {
                _suppressCompressionCommand = true;
                CompressionOn = on;
                if (!IsDigitalAudio)
                    _sessionCompressionOn = on;
                _suppressCompressionCommand = false;
            });

        radio.PhonesVolumeLevelReported += v =>
            PostToUi(() => ApplyReportedAudio(() =>
            {
                PVolume = Math.Clamp(v, 0, 100);
            }));

        radio.PhonesMicGainLevelReported += v =>
            PostToUi(() => ApplyReportedAudio(() =>
            {
                PMicGain = Math.Clamp(v, 0, 100);
            }));

        radio.DigitalVolumeLevelReported += v =>
            PostToUi(() => ApplyReportedAudio(() =>
            {
                DVolume = Math.Clamp(v, 0, 100);
            }));

        radio.DigitalMicGainLevelReported += v =>
            PostToUi(() => ApplyReportedAudio(() =>
            {
                DMicGain = Math.Clamp(v, 0, 100);
            }));

        radio.SpeakerVolumeReported += v =>
            PostToUi(() => ApplyReportedAudio(() =>
            {
                if (IsDigitalAudio) DVolume = Math.Clamp(v, 0, 100);
                else PVolume = Math.Clamp(v, 0, 100);
            }));

        radio.MicVolumeReported += v =>
            PostToUi(() => ApplyReportedAudio(() =>
            {
                if (IsDigitalAudio) DMicGain = Math.Clamp(v, 0, 100);
                else PMicGain = Math.Clamp(v, 0, 100);
            }));

        radio.AudioDigitalModeReported += isDigital =>
            PostToUi(() => ApplyReportedAudio(() =>
            {
                IsDigitalAudio = isDigital;
                AppendLog($"Audio mode reported: {(isDigital ? "D" : "P")}");
            }));

        radio.AudioDeviceReported += dev =>
            PostToUi(() => ApplyReportedAudio(() =>
            {
                if (dev == Opcodes.DIGITAL_SOUND_DEVICE)
                {
                    IsDigitalAudio = true;
                }
                else
                {
                    IsDigitalAudio = false;
                    RemoteAudio = (dev == Opcodes.REMOTE_SOUND_DEVICE);
                }
                string label = dev switch
                {
                    Opcodes.DIGITAL_SOUND_DEVICE => "D",
                    Opcodes.REMOTE_SOUND_DEVICE => "R",
                    _ => "P",
                };
                AppendLog($"Audio device reported: {dev} ({label})");
            }));

        // CW tab bidirectional reports
        radio.CwKeyerModeReported += v =>
            PostToUi(() => ApplyReportedCw(() =>
            {
                CwKeyerMode = Math.Clamp(v, 0, CwKeyerModeOptions.Count - 1);
            }));

        radio.CwSpacingReported += v =>
            PostToUi(() => ApplyReportedCw(() =>
            {
                CwSpacing = Math.Clamp(v, 0, CwSpacingOptions.Count - 1);
            }));

        radio.CwPaddleReported += v =>
            PostToUi(() => ApplyReportedCw(() =>
            {
                CwPaddle = Math.Clamp(v, 0, CwPaddleOptions.Count - 1);
            }));

        radio.CwWeightReported += v =>
            PostToUi(() => ApplyReportedCw(() =>
            {
                int idx = Array.IndexOf(CwWeightValues, v);
                if (idx >= 0) CwWeightIndex = idx;
            }));

        radio.CwWpmReported += v =>
            PostToUi(() => ApplyReportedCw(() =>
            {
                CwSpeed = Math.Clamp(v, 5, 60);
            }));

        radio.CwTxHoldReported += v =>
            PostToUi(() => ApplyReportedCw(() =>
            {
                CwHold = Math.Clamp(v, 1, 500);
            }));

        radio.CompressionLevelReported += level =>
            PostToUi(() =>
            {
                _suppressCompressionCommand = true;
                Compression = Math.Clamp(level, 0, 24);
                _suppressCompressionCommand = false;
            });

        radio.MonitorReported += on =>
            PostToUi(() =>
            {
                _suppressMonitorCommand = true;
                MonitorOn = on;
                _suppressMonitorCommand = false;
            });

        radio.NbEnableReported += on =>
            PostToUi(() =>
            {
                _suppressNbCommand = true;
                NbOn = on;
                _suppressNbCommand = false;
            });

        radio.NbPulseWidthReported += us =>
            PostToUi(() =>
            {
                _suppressNbCommand = true;
                NbPulse = us;
                _suppressNbCommand = false;
            });

        radio.NbThresholdReported += thr =>
            PostToUi(() =>
            {
                _suppressNbCommand = true;
                NbThreshold = thr;
                _suppressNbCommand = false;
            });

        radio.NrValueReported += nrValue =>
            PostToUi(() =>
            {
                _suppressNrCommand = true;
                try
                {
                    if (nrValue == 0)
                        NrOn = false;
                    else
                    {
                        NrLevel = Math.Clamp(nrValue, 0, 100);
                        NrOn = true;
                    }
                }
                finally { _suppressNrCommand = false; }
            });

        radio.AutoNotchReported += on =>
            PostToUi(() =>
            {
                _suppressAnCommand = true;
                AnOn = on;
                _suppressAnCommand = false;
            });

        radio.ProficioTempReported += t =>
            PostToUi(() => ProficioTempText = $"{t:0.0} °C");

        radio.AmpTempReported += t =>
            PostToUi(() => PaTempText = $"{t:0.0} °C");

        radio.AmpCurrentReported += ma =>
            PostToUi(() => PaCurrentText = $"{ma} mA");

        radio.ServerKeepAliveLost += () =>
            PostToUi(() =>
            {
                StatusText = "WARNING: keep-alive lost";
                AppendLog("Server keep-alive lost.");
            });

        radio.SpectrumUpdated += update =>
        {
            _spectrumFrameCounter = (_spectrumFrameCounter + 1) % SpectrumRefreshDivisor;
            if (_spectrumFrameCounter != 0) return;

            var enriched = EnrichSpectrum(update);
            int n = Interlocked.Increment(ref _spectrumFrames);
            PostToUi(() =>
            {
                CurrentSpectrum = enriched;
                if (n == 1)
                {
                    AppendLog($"Spectrum: {enriched.Data.Length} bins");
                    StatusText = "Connected — spectrum OK";
                }
                if (n % 50 == 0) UpdatePacketStats();
            });
        };
    }

    private SpectrumUpdate EnrichSpectrum(SpectrumUpdate update)
    {
        string m = UseVfoA
            ? (ModeText ?? "").Trim().ToUpperInvariant()
            : (VfoBModeText ?? "").Trim().ToUpperInvariant();
        GetFilterOffsets(m, out int low, out int high);
        int pitchHz = m is "CW"
            ? CwPitchValues[Math.Clamp(CwPitchIndex, 0, CwPitchValues.Length - 1)]
            : 0;
        long center = UseVfoA ? _frequencyHz : _vfoBFrequencyHz;
        return update with
        {
            CenterFrequencyHz = center > 0 ? center : update.CenterFrequencyHz,
            SpanHz = update.SpanHz > 0 ? update.SpanHz : SpectrumUpdate.DefaultPanadapterSpanHz,
            FilterLowHz = low,
            FilterHighHz = high,
            CwPitchHz = pitchHz,
            MinDb = -140f,
            MaxDb = 0f
        };
    }

    /// <summary>Push current Lo/Hi/CW into the spectrum passband marker (WPF sideband mapping).</summary>
    private void RefreshSpectrumFilterOverlay()
    {
        if (CurrentSpectrum != null)
            CurrentSpectrum = EnrichSpectrum(CurrentSpectrum);
    }

    private void GetFilterOffsets(string mode, out int lowHz, out int highHz)
    {
        int lo = LowCutHzValues[Math.Clamp(_lowCutIndex, 0, LowCutHzValues.Length - 1)];
        int hi = HighCutHzValues[Math.Clamp(_highCutIndex, 0, HighCutHzValues.Length - 1)];
        int cw = CwFilterHzValues[Math.Clamp(_cwFilterIndex, 0, CwFilterHzValues.Length - 1)];
        string m = (mode ?? "").Trim().ToUpperInvariant();

        switch (m)
        {
            case "LSB":
                // LSB: high audio cut → outer (more negative); low audio cut → inner
                lowHz = -hi;
                highHz = -lo;
                break;
            case "CW":
                lowHz = -cw / 2;
                highHz = +cw / 2;
                break;
            case "AM":
                lowHz = -hi;
                highHz = +hi;
                break;
            case "TUNE":
                lowHz = -100;
                highHz = 100;
                break;
            default:
                // USB / DIG-U / other
                lowHz = +lo;
                highHz = +hi;
                break;
        }
    }

    private void UpdateFrequencyUi(long hz)
    {
        FrequencyText = FormatFrequency(hz);
        FrequencyMhzEdit = FormatMhz(hz);
        FrequencyDisplayMhz = FormatMhz(hz);
    }

    private void UpdatePacketStats()
    {
        PacketStatsText = $"Pkts {_packetsReceived} | KA {_keepAlivesReceived} | D5 {_panPacketsReceived} | Spec {_spectrumFrames}";
    }

    private void OnDebugLogMessage(string message) =>
        PostToUi(() => AppendLog(message, fromCore: true));

    private static void PostToUi(Action action)
    {
        if (Dispatcher.UIThread.CheckAccess()) action();
        else Dispatcher.UIThread.Post(action);
    }

    private void AppendLog(string line, bool fromCore = false)
    {
        if (LogUiPaused && fromCore)
            return;

        string stamp = DateTime.Now.ToString("HH:mm:ss", CultureInfo.InvariantCulture);
        LogLines.Add($"[{stamp}] {line}");
        while (LogLines.Count > 800)
            LogLines.RemoveAt(0);
    }

    private static string FormatFrequency(long hz)
    {
        if (hz >= 1_000_000)
            return (hz / 1_000_000.0).ToString("0.000000", CultureInfo.InvariantCulture) + " MHz";
        if (hz >= 1_000)
            return (hz / 1_000.0).ToString("0.000", CultureInfo.InvariantCulture) + " kHz";
        return hz + " Hz";
    }

    private static string FormatMhz(long hz) =>
        (hz / 1_000_000.0).ToString("0.000000", CultureInfo.InvariantCulture);

    private static string FormatStep(long hz) => hz switch
    {
        10 => "10 Hz",
        100 => "100 Hz",
        1_000 => "1 kHz",
        10_000 => "10 kHz",
        100_000 => "100 kHz",
        _ => hz + " Hz"
    };

    private static bool TryParseMhz(string text, out long hz)
    {
        hz = 0;
        if (string.IsNullOrWhiteSpace(text)) return false;
        string t = text.Trim().ToLowerInvariant().Replace("mhz", "").Replace(" ", "");
        if (!double.TryParse(t, NumberStyles.Float, CultureInfo.InvariantCulture, out double mhz) &&
            !double.TryParse(t, NumberStyles.Float, CultureInfo.CurrentCulture, out mhz))
            return false;
        if (mhz <= 0 || mhz > 60) return false;
        hz = (long)Math.Round(mhz * 1_000_000.0);
        return hz >= 10_000;
    }

    private static string BandNameForFrequency(long hz) => hz switch
    {
        >= 135_000 and < 138_000 => "2200m",
        >= 470_000 and < 480_000 => "630m",
        >= 1_800_000 and < 2_000_000 => "160m",
        >= 3_500_000 and < 4_000_000 => "80m",
        >= 5_000_000 and < 5_500_000 => "60m",
        >= 7_000_000 and < 7_300_000 => "40m",
        >= 10_100_000 and < 10_150_000 => "30m",
        >= 14_000_000 and < 14_350_000 => "20m",
        >= 18_068_000 and < 18_168_000 => "17m",
        >= 21_000_000 and < 21_450_000 => "15m",
        >= 24_890_000 and < 24_990_000 => "12m",
        >= 28_000_000 and < 29_700_000 => "10m",
        _ => "—"
    };

    /// <summary>Default mode for band defaults / GEN (WPF-style).</summary>
    public static string DefaultModeForFrequency(long hz)
    {
        if (hz < 1_000_000) return "USB"; // LF cal carriers
        if (hz < 10_000_000) return "LSB"; // 160–40 typically LSB
        return "USB";
    }

    private static bool TryBandDefaultHz(string key, out long hz, out string label)
    {
        switch (key.Trim().ToLowerInvariant())
        {
            case "2200": hz = 136_000; label = "2200m"; return true;
            case "630": hz = 474_200; label = "630m"; return true;
            case "160": hz = 1_800_000; label = "160m"; return true;
            case "80": hz = 3_500_000; label = "80m"; return true;
            case "60": hz = 5_350_000; label = "60m"; return true;
            case "40": hz = 7_000_000; label = "40m"; return true;
            case "30": hz = 10_100_000; label = "30m"; return true;
            case "20": hz = 14_000_000; label = "20m"; return true;
            case "17": hz = 18_100_000; label = "17m"; return true;
            case "15": hz = 21_000_000; label = "15m"; return true;
            case "12": hz = 24_900_000; label = "12m"; return true;
            case "10": hz = 28_000_000; label = "10m"; return true;
            default: hz = 0; label = ""; return false;
        }
    }

    private void DisposeRadio()
    {
        if (_radio == null) return;
        try { _radio.Stop(); } catch (Exception ex) { AppendLog($"Stop: {ex.Message}"); }
        try { _radio.Dispose(); } catch { /* ignore */ }
        _radio = null;
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        CancelKeyerPlayPttRelease(releasePtt: false);
        _keyerPlayOwnsPtt = false;
        DebugMonitor.LogMessage -= OnDebugLogMessage;
        SpectrumDisplaySettings.Instance.Changed -= OnSpectrumSettingsChanged;
        AppearanceSettings.Instance.Changed -= OnAppearanceSettingsChanged;
        if (_alcIdleTimer != null)
        {
            _alcIdleTimer.Stop();
            _alcIdleTimer = null;
        }

        StopFreqCalManualPpmTimer();
        _freqCalManualPpmTimer = null;
        if (_settingsSaveTimer != null)
        {
            _settingsSaveTimer.Stop();
            _settingsSaveTimer = null;
        }

        if (_lastUsedSaveTimer != null)
        {
            _lastUsedSaveTimer.Stop();
            _lastUsedSaveTimer = null;
        }

        if (!IsCalibrationOrIqSessionActive())
            SaveLastUsedForCurrentBand();
        SaveClientSettingsNow();
        DisposeRadio();
    }
}
