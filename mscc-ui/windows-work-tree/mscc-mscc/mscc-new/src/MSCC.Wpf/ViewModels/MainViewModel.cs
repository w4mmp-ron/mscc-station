using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Threading;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using MSCC.Core.Display;
using MSCC.Core.Domain;
using MSCC.Core.Logging;
using MSCC.Core.Protocol;
using MSCC.Core.Services;
using MSCC.Wpf;
using MSCC.Wpf.Controls;
using MSCC.Wpf.Favorites;
using MSCC.Wpf.PowerCal;

namespace MSCC.Wpf.ViewModels;

/// <summary>
/// ViewModel for the main application window.
/// </summary>
public partial class MainViewModel : ObservableObject, IDisposable
{
    private IRadioService _radioService;
    private readonly SwrMeterService _swrMeter = new();
    private bool _swrFaultLatched;
    private bool _swrTxInhibited;

    /// <summary>True when UI radio model is Geminus (LF SWR profile). Set from MainWindow.</summary>
    public bool IsGeminusRadioModel { get; set; }

    internal IRadioService RadioService => _radioService;
    internal SwrMeterService SwrMeter => _swrMeter;

    [ObservableProperty]
    private RadioState _radioState = new();

    [ObservableProperty]
    private SpectrumUpdate? _currentSpectrum;

    private int _spectrumFrameCounter;

    [ObservableProperty]
    private int _sMeter;

    // ── External WiFi SWR meter ─────────────────────────────────────────
    [ObservableProperty]
    private double _swrValue = 1.0;

    [ObservableProperty]
    private double _swrForwardWatts;

    [ObservableProperty]
    private double _swrReflectedWatts;

    [ObservableProperty]
    private bool _swrFault;

    [ObservableProperty]
    private double _swrThreshold = 2.0;

    [ObservableProperty]
    private bool _swrTxRf;

    [ObservableProperty]
    private string _swrStatusText = "";

    /// <summary>
    /// TX / fault power face on the analog meter (FWD 0–10 + SWR digital).
    /// True when SWR enabled and (PTT/TUN/RF or latched fault) — stays true on fault so RESET remains available.
    /// </summary>
    [ObservableProperty]
    private bool _showExternalSwrFace;

    /// <summary>
    /// Spectrum/waterfall display zoom (1–4×). Client viewport only — same 72 kHz pan data.
    /// </summary>
    [ObservableProperty]
    private double _spectrumZoom = 1.0;

    partial void OnSpectrumZoomChanged(double value)
    {
        // Keep slider sane; renderer clamps again.
        double z = Math.Clamp(value, 1.0, 4.0);
        if (Math.Abs(z - value) > 0.001)
            SpectrumZoom = z;
    }

    /// <summary>
    /// Analog S-meter peak needle (client-side). Default off. Persisted SMETER_PEAK.
    /// </summary>
    [ObservableProperty]
    private bool _peakNeedleOn;

    /// <summary>
    /// Analog S-meter hold / slow fall (client-side). Default on. Persisted SMETER_HOLD.
    /// </summary>
    [ObservableProperty]
    private bool _smeterHold = true;

    /// <summary>
    /// Analog ALC peak needle (client-side). Default off. Persisted ALC_PEAK.
    /// </summary>
    [ObservableProperty]
    private bool _alcPeakNeedleOn;

    /// <summary>
    /// Analog ALC hold / slow fall (client-side). Default on. Persisted ALC_HOLD.
    /// </summary>
    [ObservableProperty]
    private bool _alcHold = true;

    partial void OnPeakNeedleOnChanged(bool value)
    {
        SpectrumWaterfallSettings.SmeterPeak = value;
        SpectrumWaterfallSettings.Save();
    }

    partial void OnSmeterHoldChanged(bool value)
    {
        SpectrumWaterfallSettings.SmeterHold = value;
        SpectrumWaterfallSettings.Save();
    }

    partial void OnAlcPeakNeedleOnChanged(bool value)
    {
        SpectrumWaterfallSettings.AlcPeak = value;
        SpectrumWaterfallSettings.Save();
    }

    partial void OnAlcHoldChanged(bool value)
    {
        SpectrumWaterfallSettings.AlcHold = value;
        SpectrumWaterfallSettings.Save();
    }

    [ObservableProperty]
    private int _powerOut;

    [ObservableProperty]
    private byte _lastReceivedOpcode;

    [ObservableProperty]
    private string _backendIp = "127.0.0.1";

    [ObservableProperty]
    private int _backendPort = 8888;

    /// <summary>When true, call Start automatically on main window load (persisted AUTO_START_SERVERS).</summary>
    [ObservableProperty]
    private bool _autoStartServers;

    /// <summary>
    /// When true (default), Start also launches ms-sdr / recv / trans.
    /// When false, Start only connects (use with external Start-MsccServers.bat). Persisted LAUNCH_SERVERS.
    /// </summary>
    [ObservableProperty]
    private bool _launchServersOnStart = true;

    // Favorites tab (client-side only — never sent to ms-sdr).
    // Master list holds all bands; UI shows one band at a time (same name OK on different bands).
    public ObservableCollection<FavoriteEntry> Favorites { get; } = new();

    /// <summary>Favorites for the currently selected band filter (bound to the DataGrid).</summary>
    public ObservableCollection<FavoriteEntry> FavoritesForBand { get; } = new();

    /// <summary>Band picker options for the Favorites tab.</summary>
    public ObservableCollection<string> FavoriteBandChoices { get; } = new()
    {
        "2200m", "630m", "160m", "80m", "60m", "40m", "30m", "20m", "17m", "15m", "12m", "10m", "gen"
    };

    [ObservableProperty]
    private string _favoriteBandFilter = "40m";

    [ObservableProperty]
    private string _favoriteNameInput = "";

    [ObservableProperty]
    private FavoriteEntry? _selectedFavorite;

    // ----- Power Cal tab (layout / client status; protocol wiring later) -----
    /// <summary>Per-band calibrated flags for CALIBRATION STATUS indicators (client-side file later).</summary>
    public ObservableCollection<PowerCalBandItem> PowerCalBandStatuses { get; } = new();

    /// <summary>Currently selected band number for Pwr Cal (160, 80, … 10). 0 = none.</summary>
    [ObservableProperty]
    private int _powerCalSelectedBand;

    [ObservableProperty]
    private int _powerCalSliderValue;

    [ObservableProperty]
    private string _powerCalStepLabel = "CALIBRATION STEP: —";

    [ObservableProperty]
    private bool _powerCalTxOn;

    /// <summary>True while manual CALIBRATE session is active (slider enabled, accept dialog on stop).</summary>
    [ObservableProperty]
    private bool _powerCalCalibrating;

    /// <summary>Last step from server 0xB4 (for cancel restore of previous value).</summary>
    private int _powerCalPreviousReceivedStep;

    // Rx/Tx tab specific
    [ObservableProperty]
    private int _tunePowerPercent = 10;

    [ObservableProperty]
    private int _cwPowerPercent = 50;

    [ObservableProperty]
    private int _ssbPowerPercent = 50;

    [ObservableProperty]
    private int _amCarrierPercent = 30;

    /// <summary>
    /// MAIN operate panel: which power bank the quick slider targets.
    /// TUN (or mode TUNE) → tune; CW → CW; AM → AM carrier; else → SSB (USB/LSB / future DIG).
    /// </summary>
    private enum OperatePowerBank
    {
        Tune,
        Cw,
        Ssb,
        Am
    }

    [ObservableProperty]
    private bool _fullPower;

    [ObservableProperty]
    private bool _alcOn = true;

    [ObservableProperty]
    private bool _qrpMode;

    [ObservableProperty]
    private bool _autoTune;

    [ObservableProperty]
    private int _txBandwidthIndex = 1; // 0=2.4, 1=2.7, 2=3.0, 3=5.5 kHz

    [ObservableProperty]
    private bool _nbOn;

    /// <summary>NB pulse width (µs). Original range ~10–510; default 200.</summary>
    [ObservableProperty]
    private int _nbPulse = 200;

    /// <summary>NB threshold raw (original 1–1009). Display % ≈ value/10.</summary>
    [ObservableProperty]
    private int _nbThreshold = 20;

    private bool _suppressNbCommand;

    [ObservableProperty]
    private bool _nrOn;

    [ObservableProperty]
    private int _nrLevel = 50;

    /// <summary>Suppress NR send while applying appliance → UI reports (0xA3).</summary>
    private bool _suppressNrCommand;

    /// <summary>Auto notch (AN). CMD_GET_SET_AUTO_NOTCH 0x8E — enable only.</summary>
    [ObservableProperty]
    private bool _anOn;

    /// <summary>Suppress AN send while applying appliance → UI reports (0x8E).</summary>
    private bool _suppressAnCommand;

    // AGC (level/mode) and AGC FAST RELEASE (time in ms)
    public ObservableCollection<string> AgcOptions { get; } = new ObservableCollection<string> { "SLOW", "MED", "FAST" };

    [ObservableProperty]
    private int _agcLevel;

    [ObservableProperty]
    private string _agcButtonText = "SLO";

    [ObservableProperty]
    private int _agcFastRelease = 500; // ms

    // Default Filters (Rx/Tx tab)
    public ObservableCollection<string> LowCutOptions { get; } = new ObservableCollection<string> { "500Hz", "300Hz", "200Hz", "100Hz", "75Hz" };
    public ObservableCollection<string> TxOptions { get; } = new ObservableCollection<string> { "2.4KHz", "2.7KHz", "3.0KHz", "5.5KHz" };
    public ObservableCollection<string> HighCutOptions { get; } = new ObservableCollection<string> { "5.5KHz", "4.0KHz", "3.0KHz", "2.7KHz", "2.4KHz" };
    public ObservableCollection<string> CwFilterOptions { get; } = new ObservableCollection<string> { "1.8KHz", "400Hz", "200Hz" };

    // CW tab (from original CW tab in mscc)
    public ObservableCollection<string> CwKeyerModeOptions { get; } = new ObservableCollection<string> { "STRAIGHT", "IAMBIC-A", "IAMBIC-B" };
    public ObservableCollection<string> CwSpacingOptions { get; } = new ObservableCollection<string> { "ELEMENT", "LETTER" };
    public ObservableCollection<string> CwPaddleOptions { get; } = new ObservableCollection<string> { "NORMAL", "REVERSE" };
    public ObservableCollection<string> CwWeightOptions { get; } = new ObservableCollection<string> { "25", "50", "75" };
    public ObservableCollection<string> CwPitchOptions { get; } = new ObservableCollection<string> { "400Hz", "600Hz", "800Hz", "1000Hz" };

    [ObservableProperty]
    private int _lowCutDefaultIndex;

    [ObservableProperty]
    private int _txDefaultIndex;

    [ObservableProperty]
    private int _highCutDefaultIndex;

    [ObservableProperty]
    private int _cwFilterDefaultIndex;

    // Main tab active filter/step buttons (cycling indices like original CW_Filter etc buttons)
    // Indices map to the options lists; Hz set to filter for spectrum etc; send index to backend
    [ObservableProperty] private int _lowCutIndex;
    [ObservableProperty] private string _lowCutLabel = "75Hz";
    [ObservableProperty] private int _highCutIndex;
    [ObservableProperty] private string _highCutLabel = "4.0KHz";
    [ObservableProperty] private int _cwFilterIndex;
    [ObservableProperty] private string _cwFilterLabel = "200Hz";
    [ObservableProperty] private int _stepIndex = 5;
    [ObservableProperty] private string _stepLabel = "1Hz";

    // Add this field at the class level (outside any method)
    private static int last_smeter = 0;
    
    /*private static int Db_to_Smeter(int dbm)
    {
        int smeter_value = 0;
        if (dbm <= -130)
        {
            smeter_value = 0;
        }
        else if (dbm <= -73)
        {
            // S1 to S9: 6 dB per S-unit, S9 = -73 dBm
            smeter_value = 9 + (dbm + 73) / 6;
            if (smeter_value < 1)
            {
                smeter_value = 1;
            }
        }
        else
        {
            // Above S9: 10 dB steps (S9+10, S9+20, S9+30+)
            int db_over_s9 = dbm + 73;
            int over_value = (db_over_s9 + 5) / 10; // round nearest
            if (over_value <= 0)
            {
                smeter_value = 9;
            }
            else if (over_value >= 3)
            {
                smeter_value = 12; // Firmware limit
            }
            else
            {
                smeter_value = 9 + over_value;
            }
        }

        // Asymmetric hysteresis (quick rise, slower fall)
        int hysteresis_threshold;

        if (smeter_value > last_smeter)
        {
            hysteresis_threshold = 1;   // Rise quickly
        }
        else
        {
            hysteresis_threshold = 2;   // Fall more slowly
        }

        if (Math.Abs(smeter_value - last_smeter) <= hysteresis_threshold)
        {
            smeter_value = last_smeter;
        }

        last_smeter = smeter_value;

        return smeter_value;
    }*/

    private static int Db_to_Smeter(int dbm)
    {
            int smeter_value = 0;

            if (dbm <= -130)
            {
                smeter_value = 0;
            }
            else if (dbm <= -73)
            {
                // S1 to S9: 6 dB per S-unit, S9 = -73 dBm
                smeter_value = 9 + (dbm + 73) / 6;
                if (smeter_value < 1)
                {
                    smeter_value = 1;
                }
            }
            else
            {
                // Above S9: 10 dB steps (S9+10, S9+20, S9+30+)
                int db_over_s9 = dbm + 73;
                int over_value = (db_over_s9 + 5) / 10;   // round nearest

                if (over_value <= 0)
                {
                    smeter_value = 9;
                }
                else if (over_value >= 3)
                {
                    smeter_value = 12;                    // Firmware limit
                }
                else
                {
                    smeter_value = 9 + over_value;
                }
            }

            return smeter_value;
    }


    /*private static int Db_to_Smeter(int db)
    {
        if (db <= -130) return 0;
        if (db <= -121) return 1;
        if (db <= -115) return 2;
        if (db <= -109) return 3;
        if (db <= -103) return 4;
        if (db <= -97) return 5;
        if (db <= -91) return 6;
        if (db <= -85) return 7;
        if (db <= -79) return 8;
        if (db <= -73) return 9;
        if (db <= -63) return 10;
        if (db <= -53) return 11;
        if (db <= -43) return 12;
        if (db <= -33) return 13;
        if (db <= -23) return 14;
        if (db <= -13) return 15;
        return 0;
    }*/

    // CW tab properties (keyer, speed, pitch, hold, qsk, phones)
    [ObservableProperty] private int _cwSpeed = 20;
    /// <summary>Farnsworth memory-play text WPM (0x76). 0=Off; 5–60.</summary>
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(CwMemTextWpmLabel))]
    private int _cwMemTextWpm;
    [ObservableProperty] private int _cwKeyerMode = 1; // IAMBIC-A
    [ObservableProperty] private int _cwSpacing = 0;
    [ObservableProperty] private int _cwPaddle = 0;
    [ObservableProperty] private int _cwWeightIndex = 1; // 50
    [ObservableProperty] private int _cwPitchIndex = 1; // 600Hz
    [ObservableProperty] private int _cwHold = 100;
    [ObservableProperty] private bool _cwQsk = false;
    [ObservableProperty] private bool _cwPhones = false;

    /// <summary>CW tab display: "Off" or numeric text WPM.</summary>
    public string CwMemTextWpmLabel =>
        CwMemTextWpm <= 0 ? "Off" : CwMemTextWpm.ToString();

    /// <summary>
    /// Settings: external electronic keyer / legacy radio.
    /// Maps to mscc.ini PROFICIO-MKII=0 when true, =1 when false (default).
    /// ms-sdr reads this only at process start — mid-session flip needs Stop/Start.
    /// </summary>
    [ObservableProperty] private bool _externalElectronicKeyer;

    /// <summary>PIC keyer CW controls (mode/speed/spacing/paddle/weight/CQ mem). Hold stays live.</summary>
    public bool PicKeyerControlsEnabled => !ExternalElectronicKeyer;

    private static readonly int[] LowCutHzValues = { 500, 300, 200, 100, 75 };
    private static readonly int[] HighCutHzValues = { 5500, 4000, 3000, 2700, 2400 };
    private static readonly int[] CwFilterHzValues = { 1800, 400, 200 };
    private static readonly int[] CwWeightValues = { 25, 50, 75 };
    private static readonly int[] CwPitchValues = { 400, 600, 800, 1000 };
    private static readonly string[] StepLabelsArr = { "100KHz", "10KHz", "1KHz", "100Hz", "10Hz", "1Hz" };
    private static readonly long[] StepHzValues = { 100000, 10000, 1000, 100, 10, 1 };

    [ObservableProperty]
    private int _forwardPower;

    [ObservableProperty]
    private int _reversePower;

    [ObservableProperty]
    private int _swr;

    // Aud/Sys tab - detailed Audio controls (0-24 level, bools)
    [ObservableProperty] private bool _compressionOn;

    /// <summary>
    /// Session-preferred CMP state (from server report or user while on P).
    /// Used to restore CMP when switching D → P. Not overwritten when CMP is forced off for D.
    /// </summary>
    private bool _sessionCompressionOn;
    private bool _suppressCompressionCommand;

    partial void OnCompressionOnChanged(bool value)
    {
        if (!_suppressCompressionCommand)
        {
            _ = _radioService.SetCompressionStateAsync(value);
            // Remember preferred state only while on phones (P), not while forced off for digital (D)
            if (!IsDigitalAudio)
                _sessionCompressionOn = value;
        }
        else if (!IsDigitalAudio)
        {
            // Server report while on P: remember for this session
            _sessionCompressionOn = value;
        }
        MonitorTextBoxText(
            $" CompressionOn set: {value}{(_suppressCompressionCommand ? " (from server)" : "")} sessionPreferred={_sessionCompressionOn}");
    }

    [ObservableProperty] private int _compressionLevel; // 0-24 db
    partial void OnCompressionLevelChanged(int value) { _ = _radioService.SetCompressionLevelAsync(value); MonitorTextBoxText($" CompressionLevel set: {value}"); }

    [ObservableProperty] private bool _monitorOn;
    partial void OnMonitorOnChanged(bool value) { _ = _radioService.SetMonitorAsync(value); MonitorTextBoxText($" MonitorOn set: {value}"); }

    /// <summary>
    /// Audio digital mode (P=phones/operator false, D=digital true). Toggles Audio_Digital_button on Main tab.
    /// P→D: if CMP on, force OFF and send. D→P: restore session CMP and send.
    /// </summary>
    [ObservableProperty]
    private bool _isDigitalAudio;

    /// <summary>
    /// With Phones selected: send CMD_SET_AUDIO_DEVICE=2 (remote mic via MsccRemotePhones).
    /// Ignored while Digital is selected (always 0). Sticky REMOTE_AUDIO in MSCC_Client.ini.
    /// </summary>
    [ObservableProperty]
    private bool _remoteAudio;

    /// <summary>Remote Audio checkbox enabled only on Phones path.</summary>
    public bool RemoteAudioCheckboxEnabled => !IsDigitalAudio;

    private bool _suppressAudioDeviceSend;

    /// <summary>0=Digital, 1=Phones local, 2=Remote (Phones + Remote Audio).</summary>
    private byte ResolveAudioDeviceOpcode()
    {
        if (IsDigitalAudio)
            return Opcodes.DIGITAL_SOUND_DEVICE;
        if (RemoteAudio)
            return Opcodes.REMOTE_SOUND_DEVICE;
        return Opcodes.PHONES_SOUND_DEVICE;
    }

    private void SendResolvedAudioDevice(string reason)
    {
        if (_suppressAudioDeviceSend) return;
        byte device = ResolveAudioDeviceOpcode();
        _ = _radioService.SetAudioDeviceAsync(device);
        string label = device switch
        {
            Opcodes.DIGITAL_SOUND_DEVICE => "Digital (0)",
            Opcodes.REMOTE_SOUND_DEVICE => "Remote (2)",
            _ => "Phones (1)",
        };
        MonitorTextBoxText($" Audio device → {label} ({reason})");
    }

    partial void OnIsDigitalAudioChanged(bool value)
    {
        OnPropertyChanged(nameof(RemoteAudioCheckboxEnabled));
        SendResolvedAudioDevice(value ? "path→D" : "path→P");

        if (_suppressAudioDeviceSend) return;

        if (value)
        {
            // P → D: force compression off and notify server (session preferred kept in _sessionCompressionOn)
            if (CompressionOn)
            {
                CompressionOn = false; // OnCompressionOnChanged sends OFF; session not cleared (IsDigitalAudio already D)
                MonitorTextBoxText(" CMP forced OFF for digital audio (D); session preferred preserved");
            }
        }
        else
        {
            // D → P: restore session CMP state and send to server
            if (CompressionOn != _sessionCompressionOn)
            {
                CompressionOn = _sessionCompressionOn; // sends restore value
            }
            else
            {
                // UI already matches; still push so server is in sync
                _ = _radioService.SetCompressionStateAsync(_sessionCompressionOn);
            }
            MonitorTextBoxText($" CMP restored for phones (P): {_sessionCompressionOn} → sent to server");
        }
    }

    partial void OnRemoteAudioChanged(bool value)
    {
        if (!_suppressAudioDeviceSend)
        {
            SpectrumWaterfallSettings.RemoteAudio = value;
            SpectrumWaterfallSettings.Save();
        }
        if (IsDigitalAudio)
        {
            if (!_suppressAudioDeviceSend)
                MonitorTextBoxText(" Remote Audio sticky saved (inactive while Audio=Digital)");
            return;
        }
        SendResolvedAudioDevice(value ? "Remote Audio ON" : "Remote Audio OFF");
    }

    /// <summary>
    /// TUN button on main tab: activates TUNE mode (carrier for antenna tuning) using rig tune command and TUNE mode.
    /// </summary>
    [ObservableProperty] private bool _tuneMode;
    partial void OnTuneModeChanged(bool value)
    {
        if (value)
        {
            _previousMode = RadioState.ActiveVfo.Mode;
            RadioState.ActiveVfo.Mode = RadioMode.TUNE;
            if (!_suppressTransmitCommands && !TxSetByServer)
            {
                _ = _radioService.SetModeAsync("TUNE");
                _ = _radioService.SetAutoTuneAsync(true);
                // Use the tune power from Rx/Tx tab
                _ = _radioService.SetTunePowerAsync(TunePowerPercent);
            }
        }
        else
        {
            RadioState.ActiveVfo.Mode = _previousMode;
            if (!_suppressTransmitCommands && !TxSetByServer)
            {
                _ = _radioService.SetModeAsync(FormatModeDisplay(_previousMode));
                _ = _radioService.SetAutoTuneAsync(false);
            }
        }
        MonitorTextBoxText($" TuneMode set: {value}");
        // Returning to RX: clear ALC if neither PTT nor TUN is keyed
        MaybeZeroAlcMeterOnRx();
        NotifyMainOperatePower();
    }

    /// <summary>
    /// PTT button: keys the transmitter (PTT on/off). Uses CMD_SET_TX_ON.
    /// </summary>
    [ObservableProperty] private bool _pttOn;
    partial void OnPttOnChanged(bool value)
    {
        if (_swrTxInhibited && value)
        {
            MonitorTextBoxText(" PTT blocked — SWR fault active (RESET meter first)");
            Application.Current?.Dispatcher.BeginInvoke(() =>
            {
                if (_swrTxInhibited && PttOn)
                    PttOn = false;
            });
            return;
        }

        RadioState.IsTransmitting = value;
        if (!_suppressTransmitCommands && !TxSetByServer)
            _ = _radioService.SetTransmitAsync(value);
        MonitorTextBoxText($" PttOn set: {value}");
        // Returning to RX: clear ALC if neither PTT nor TUN is keyed
        MaybeZeroAlcMeterOnRx();
        UpdateShowExternalSwrFace();
    }

    /// <summary>
    /// True when ms-sdr server is controlling TX state (via 0xBC). Disables user PTT/TUN controls.
    /// </summary>
    [ObservableProperty] private bool _txSetByServer;

    [ObservableProperty] private bool _canUserControlTransmit = true;

    partial void OnTxSetByServerChanged(bool value)
    {
        CanUserControlTransmit = !value;
    }

    [ObservableProperty] private bool _ampOn;
    private bool _suppressAmpCommand;

    /// <summary>
    /// QRP CAL tab is only available when AMP is inactive (not highlighted).
    /// Matches original: transceiver power cal when not in PA/QRO path.
    /// </summary>
    public bool IsPowerCalTabEnabled => !AmpOn;

    /// <summary>
    /// AMP CAL tab is only available when AMP is active (original AMP_groupBox3.Enabled with PA on).
    /// </summary>
    public bool IsAmpCalTabEnabled => AmpOn;

    /// <summary>
    /// TX IQ tab is only available in QRP (AMP off). TX IQ balance must not run with PA/QRO path.
    /// </summary>
    public bool IsTxIqTabEnabled => !AmpOn;

    // ----- TX IQ tab (manual TX I/Q balance; original IQBD_TX_groupBox; no auto cal) -----
    public ObservableCollection<PowerCalBandItem> TxIqBandItems { get; } = new();

    [ObservableProperty] private int _txIqSelectedBand;
    [ObservableProperty] private int _txIqOffset;          // −200…+200
    [ObservableProperty] private int _txIqPower = 100;     // cal tune power 0–100
    [ObservableProperty] private bool _txIqTxOn;
    [ObservableProperty] private string _txIqStatus = "Select a band (QRP only).";
    [ObservableProperty] private bool _txIqCommitting;

    private bool _txIqTabActive;
    private OperatingStateSnapshot? _preTxIqState;
    private bool _suppressTxIqOffset;
    private bool _suppressTxIqPower;

    // ----- RX IQ tab (original Freq Cal IQ_groupBox3). Full manual path: 0x58/0x55/0x52/0x57/0x8D. -----
    /// <summary>True after successful START (band resolved + 0x58 + 0x55 RX sent).</summary>
    [ObservableProperty] private bool _rxIqSessionActive;

    /// <summary>I/Q balance offset (−200…+200; original LefthScrollBar1 → 0x52).</summary>
    [ObservableProperty] private int _rxIqOffset;

    /// <summary>Fine LO offset in Hz (−1000…+2009 original IQ_Freq_hScrollBar1).</summary>
    [ObservableProperty] private int _rxIqFreqOffsetHz;

    /// <summary>Add +24 kHz to LO (image nulling aid).</summary>
    [ObservableProperty] private bool _rxIqUp24k;

    /// <summary>Band label e.g. "40m".</summary>
    [ObservableProperty] private string _rxIqBandLabel = "—";

    /// <summary>Displayed tune frequency MHz.KKK.HHH (base [+24k] [+fine]).</summary>
    [ObservableProperty] private string _rxIqFreqDisplay = "—.—.—";

    [ObservableProperty] private string _rxIqStatus =
        "Select an amateur band on MAIN, then START. BAND/FREQ use last-used for that band.";

    [ObservableProperty] private bool _rxIqCommitting;

    /// <summary>Base IQ_RX_Freq (Hz) for current session — last-used on selected band.</summary>
    private long _rxIqBaseFreqHz;

    /// <summary>Band meters (160…10) for 0x58.</summary>
    private int _rxIqBandMeters;

    private bool _rxIqTabActive;
    private bool _suppressRxIqFreqTune;
    private bool _suppressRxIqOffset;

    // ----- AMP CAL tab (POWER AMPLIFIER; layout matches TRANS CAL status + band select) -----
    /// <summary>Per-band amp calibrated flags + selection for AMP CAL UI.</summary>
    public ObservableCollection<PowerCalBandItem> AmpCalBandStatuses { get; } = new();

    /// <summary>Currently selected amp-cal band number (160…10). 0 = none.</summary>
    [ObservableProperty]
    private int _ampCalSelectedBand;

    [ObservableProperty]
    private int _ampCalSliderValue = -99;

    [ObservableProperty]
    private string _ampCalStepLabel = "STEP: 0";

    /// <summary>AMP CAL TX (tune carrier) active — original PA_TX_button8 / Tuning_Mode.</summary>
    [ObservableProperty]
    private bool _ampCalTxOn;

    /// <summary>AMP CAL manual calibrate session active (slider enabled) — original PA_Manual_Calibrate.</summary>
    [ObservableProperty]
    private bool _ampCalCalibrating;

    private bool _suppressAmpCalSlider;

    partial void OnAmpOnChanged(bool value)
    {
        // Skip PA_BYPASS send when applying a server report (bidirectional 0xF7).
        if (!_suppressAmpCommand)
            _ = _radioService.SetPaBypassAsync(value);

        // Dual Tune Power: AMP on and AMP off each have their own stored value.
        // On AMP change, apply the matching value and send it to ms_sdr (0xE9).
        ApplyTunePowerForAmpState(value);

        OnPropertyChanged(nameof(IsPowerCalTabEnabled));
        OnPropertyChanged(nameof(IsAmpCalTabEnabled));
        OnPropertyChanged(nameof(IsTxIqTabEnabled));
        // If AMP turns on while TX IQ was active, force leave path is handled by tab IsEnabled;
        // also stop TX IQ carrier if user somehow still on tab.
        if (value && _txIqTabActive)
            ForceStopTxIqSession("AMP on — TX IQ requires QRP");
        MonitorTextBoxText($" AmpOn set: {value}{( _suppressAmpCommand ? " (from server)" : "")}");
    }

    /// <summary>
    /// Load the Tune Power value stored for the given AMP state into the slider and send to ms_sdr.
    /// </summary>
    private void ApplyTunePowerForAmpState(bool ampOn)
    {
        int target = SpectrumWaterfallSettings.GetTunePowerForAmp(ampOn);
        if (TunePowerPercent != target)
        {
            // Property change sends 0xE9 and persists into the AMP-state store.
            TunePowerPercent = target;
        }
        else
        {
            // Same percent as before AMP flip — still push to ms_sdr for the new path.
            _ = _radioService.SetTunePowerAsync(target);
            SpectrumWaterfallSettings.TunePower = target;
            MonitorTextBoxText($" Tune power % re-sent for AmpOn={ampOn}: {target}");
        }
    }

    private RadioMode _previousMode = RadioMode.USB;

    [ObservableProperty] private bool _transverterOn;
    partial void OnTransverterOnChanged(bool value) { _ = _radioService.SetTransverterAsync(value); MonitorTextBoxText($" TransverterOn set: {value}"); }

    [ObservableProperty] private bool _timeDisplayOn;
    partial void OnTimeDisplayOnChanged(bool value)
    {
        _ = _radioService.SetTimeDisplayAsync(value);
        MonitorTextBoxText($" TimeDisplayOn set: {value}");
        SpectrumWaterfallSettings.TimeDisplayOn = value;
        SpectrumWaterfallSettings.Save();
        if (value)
            StartClockTimer();
        else
            StopClockTimer();
    }

    [ObservableProperty] private string _localTimeDisplay = "00:00:00";
    [ObservableProperty] private string _localDateDisplay = "01.01.24";
    [ObservableProperty] private string _utcTimeDisplay = "00:00:00";
    [ObservableProperty] private string _utcDateDisplay = "01.01.24";

    [ObservableProperty] private int _mainTabIndex = 0;

    [ObservableProperty] private bool _specWfVisible;

    /// <summary>True while the debug log popup is open (drives LOG button active look).</summary>
    [ObservableProperty] private bool _debugLogVisible;

    /// <summary>Only one Spectrum/Waterfall controls window allowed at a time.</summary>
    private PanadapterControlsWindow? _specWaterfallWindow;

    /// <summary>Only one debug log window allowed at a time.</summary>
    private DebugLogWindow? _debugLogWindow;

    private DispatcherTimer? _clockTimer;

    [ObservableProperty] private bool _betaTestOn;

    // Display info from Aud/Sys
    [ObservableProperty] private double _proficioTempC;
    [ObservableProperty] private double _ampTempC;
    [ObservableProperty] private int _ampCurrentMa;
    [ObservableProperty] private int _alcValue;  // ALC meter value (0–100, averaged for display)

    /// <summary>Rolling average of recent ALC samples so the meter does not thrash on fast server reports.</summary>
    private readonly int[] _alcSampleRing = new int[20];
    private int _alcSampleCount;
    private int _alcSampleIndex;

    /// <summary>
    /// If no ALC samples arrive for this long, zero the meter (server may not send 0 on return to RX).
    /// Also zeroed immediately when both PTT and TUN are off.
    /// </summary>
    private DispatcherTimer? _alcIdleTimer;
    private const int AlcIdleTimeoutSeconds = 3;

    /// <summary>ms-sdr (CMD_GET_SET_MSSDR_VERSION 0xB3). UI: Core:</summary>
    [ObservableProperty] private string _coreVersion = "--";
    /// <summary>This client build stamp. UI: MSCC:</summary>
    [ObservableProperty] private string _displayVersion = "--";
    /// <summary>Radio firmware (CMD_GET_SET_FIRMWARE_VERSION 0xB2). UI: FW:</summary>
    [ObservableProperty] private string _firmwareVersion = "--";

    /// <summary>Window title bar: product name + live version trio (was bottom-right VERSIONS panel).</summary>
    public string WindowTitle =>
        $"MSCC WPF   ·   MSCC: {DisplayVersion}   Core: {CoreVersion}   FW: {FirmwareVersion}";

    partial void OnDisplayVersionChanged(string value) => OnPropertyChanged(nameof(WindowTitle));
    partial void OnCoreVersionChanged(string value) => OnPropertyChanged(nameof(WindowTitle));
    partial void OnFirmwareVersionChanged(string value) => OnPropertyChanged(nameof(WindowTitle));

    [ObservableProperty] private string _preferredMeter = "P";

    [ObservableProperty] private string _lastServerMessage = "SERVER MESSAGES WILL BE DISPLAYED HERE";

    /// <summary>
    /// Debug log lines (populated via MonitorTextBoxText, matching original behavior).
    /// UI can bind a TextBox or ListBox to this.
    /// </summary>
    public ObservableCollection<string> DebugLog { get; } = new();

    private string _debugLogText = string.Empty;
    public string DebugLogText
    {
        get => _debugLogText;
        private set => SetProperty(ref _debugLogText, value);
    }

    [ObservableProperty]
    private bool _monitorSuspend;

    private bool _resetLogFile;

    public bool IsConnected => _radioService.IsConnected;

    /// <summary>
    /// True while the radio session is active (Start succeeded; cleared on Stop).
    /// Drives Start/Stop button label and green "running" style.
    /// </summary>
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(StartStopButtonText))]
    [NotifyPropertyChangedFor(nameof(SetupStatusLine))]
    [NotifyPropertyChangedFor(nameof(CanStartLocalRadioSession))]
    private bool _isRadioRunning;

    /// <summary>Start button when idle; Stop when session active.</summary>
    public string StartStopButtonText => IsRadioRunning ? "Stop" : "Start";

    /// <summary>
    /// Soft status under Start: empty when OK, or "Setup needed: …" when local Launch Servers
    /// cannot start until COM / operator speaker are set in Settings.
    /// </summary>
    [ObservableProperty]
    private string _setupStatusLine = "";

    /// <summary>
    /// True when Start/Stop is allowed: Stop while running; Start when Launch Servers is off
    /// or local COM + operator audio setup is complete. Bound to Start button IsEnabled.
    /// </summary>
    public bool CanStartLocalRadioSession
    {
        get
        {
            if (IsRadioRunning)
                return true; // Stop always allowed
            if (!LaunchServersOnStart)
                return true; // remote / connect-only
            return ConfigBootstrap.EvaluateLocalSetup(launchServers: true).IsComplete;
        }
    }

    /// <summary>Refresh setup banner after Settings apply or Launch Servers toggle.</summary>
    public void RefreshSetupStatus()
    {
        var status = ConfigBootstrap.EvaluateLocalSetup(LaunchServersOnStart);
        SetupStatusLine = IsRadioRunning ? "" : status.SummaryLine;
        OnPropertyChanged(nameof(CanStartLocalRadioSession));
    }

    /// <summary>
    /// Indicates whether the real UdpRadioService (UDP backend) is in use.
    /// (Mock simulator has been removed; always real now.)
    /// </summary>
    public bool UsingRealBackend => true;

    /// <summary>
    /// Human-friendly label for the current backend.
    /// </summary>
    public string BackendMode => "real (UDP)";

    public bool IsVfoAActive => RadioState.ActiveVfo == RadioState.VfoA;
    public bool IsVfoBActive => RadioState.ActiveVfo == RadioState.VfoB;

    public string ActiveMode
    {
        get => FormatModeDisplay(RadioState.ActiveVfo.Mode);
        set
        {
            var newMode = ParseMode(value);
            var oldMode = RadioState.ActiveVfo.Mode;
            if (oldMode != newMode)
            {
                // Persist filter bag for the mode we're leaving (phone ↔ digi without reconfig).
                if (!_suppressModeProfileSwap)
                    SaveModeFilterProfile(oldMode);

                RadioState.ActiveVfo.Mode = newMode;

                if (!_suppressModeProfileSwap)
                {
                    ApplyModeFilterProfile(newMode);
                    ApplyDigUAudioPolicy(oldMode, newMode);
                }
                // Recompute also via Vfo.Mode PropertyChanged when cuts unchanged
            }

            _ = _radioService.SetModeAsync(FormatModeDisplay(newMode));
            MonitorTextBoxText($" ActiveMode set to {FormatModeDisplay(newMode)}");
            OnPropertyChanged();
            NotifyMainOperatePower();
            SaveLastUsedForCurrentBand();
        }
    }

    /// <summary>When true, skip save/load of per-mode filter profiles (band last-used load applies its own cuts).</summary>
    private bool _suppressModeProfileSwap;

    /// <summary>Audio P/D state before entering DIG-U (restored when leaving).</summary>
    private bool? _audioBeforeDigU;

    /// <summary>
    /// Label for the MAIN operate power slider (context: TUN / CW / SSB / AM).
    /// </summary>
    public string MainOperatePowerLabel => ResolveOperatePowerBank() switch
    {
        OperatePowerBank.Tune => "TUNE POWER",
        OperatePowerBank.Cw => "CW POWER",
        OperatePowerBank.Am => "AM CARRIER",
        _ => "SSB POWER"
    };

    /// <summary>
    /// MAIN operate power % — two-way shortcut into the same stores as RX/TX tab sliders.
    /// </summary>
    public int MainOperatePowerPercent
    {
        get => ResolveOperatePowerBank() switch
        {
            OperatePowerBank.Tune => TunePowerPercent,
            OperatePowerBank.Cw => CwPowerPercent,
            OperatePowerBank.Am => AmCarrierPercent,
            _ => SsbPowerPercent
        };
        set
        {
            int v = Math.Clamp(value, 0, 100);
            switch (ResolveOperatePowerBank())
            {
                case OperatePowerBank.Tune:
                    if (TunePowerPercent != v) TunePowerPercent = v;
                    break;
                case OperatePowerBank.Cw:
                    if (CwPowerPercent != v) CwPowerPercent = v;
                    break;
                case OperatePowerBank.Am:
                    if (AmCarrierPercent != v) AmCarrierPercent = v;
                    break;
                default:
                    if (SsbPowerPercent != v) SsbPowerPercent = v;
                    break;
            }
            OnPropertyChanged();
            OnPropertyChanged(nameof(MainOperatePowerLabel));
        }
    }

    private OperatePowerBank ResolveOperatePowerBank()
    {
        // TUN button (and TUNE mode it forces) always targets tune drive — amp testing case.
        if (TuneMode || RadioState.ActiveVfo.Mode == RadioMode.TUNE)
            return OperatePowerBank.Tune;

        return RadioState.ActiveVfo.Mode switch
        {
            RadioMode.CW => OperatePowerBank.Cw,
            RadioMode.AM => OperatePowerBank.Am,
            _ => OperatePowerBank.Ssb // USB, LSB, DigU
        };
    }

    /// <summary>Refresh MAIN power slider label/value after mode, TUN, or bank change.</summary>
    private void NotifyMainOperatePower()
    {
        OnPropertyChanged(nameof(MainOperatePowerPercent));
        OnPropertyChanged(nameof(MainOperatePowerLabel));
    }

    /// <summary>
    /// Creates the ViewModel connected to the real UdpRadioService (UDP backend) using settings from MSCC_Client.ini (via ConnectionSettings.Default).
    /// This is the primary (and now only) way to use the application.
    /// </summary>
    public MainViewModel() : this(ConnectionSettings.Default)
    {
    }

    /// <summary>
    /// Creates the ViewModel connected to a *real* backend via UDP using the supplied settings.
    /// This is the primary way to make the real UdpRadioService active in the application.
    /// </summary>
    public MainViewModel(ConnectionSettings settings)
        : this(new UdpRadioService(settings.RemoteIp, settings.RemotePort, settings.LocalPort))
    {
        // Set backing fields directly to avoid triggering OnBackend*Changed (and popup) on initial load from INI.
        _backendIp = settings.RemoteIp;
        _backendPort = settings.RemotePort;
        // Client pref may already be loaded via SpectrumWaterfallSettings static/init Load.
        _autoStartServers = SpectrumWaterfallSettings.AutoStartServers;
        _launchServersOnStart = SpectrumWaterfallSettings.LaunchServersOnStart;
    }

    public MainViewModel(IRadioService radioService)
    {
        _radioService = radioService;

        // Initialize the debug monitor / log (replicates original MonitorTextBoxText + file logging)
        DebugMonitor.Initialize();
        DebugMonitor.LogMessage += OnDebugLogMessage;

        // External SWR meter (UDP) — start if enabled in MSCC_Client.ini
        try
        {
            SwrMeterSettings.Load();
            _swrMeter.ReadingReceived += OnSwrReading;
            _swrMeter.StatusChanged += s =>
            {
                RunOnUi(() => SwrStatusText = s);
            };
            ApplySwrMeterSettings("startup");
        }
        catch (Exception ex)
        {
            MonitorTextBoxText($" SWR meter init: {ex.Message}");
        }

        // Client version stamped at build (1.0.N, auto-bumped each build) → AUD/SYS "MSCC:" label
        DisplayVersion = GetClientVersionString();
        MonitorTextBoxText($" MainViewModel initialized (backend: real UDP) client={DisplayVersion}");

        ToggleVfoCommand = new RelayCommand(ToggleVfo);
        SelectVfoACommand = new RelayCommand(() => SelectVfo(useVfoB: false));
        SelectVfoBCommand = new RelayCommand(() => SelectVfo(useVfoB: true));
        TuneToFrequencyCommand = new RelayCommand<long>(f => TuneToFrequency(f));

        ToggleQrpCommand = new RelayCommand(() => QrpMode = !QrpMode);
        ToggleAutoTuneCommand = new RelayCommand(() => AutoTune = !AutoTune);
        ToggleAlcCommand = new RelayCommand(() => AlcOn = !AlcOn);
        ToggleNbCommand = new RelayCommand(() => NbOn = !NbOn);
        ToggleNrCommand = new RelayCommand(() => NrOn = !NrOn);
        // Match legacy Freqbutton3: send 0x8E payload 1 when turning ON, 0 when OFF
        ToggleAnCommand = new RelayCommand(ToggleAn);
        ToggleFullPowerCommand = new RelayCommand(() => FullPower = !FullPower);

        // init main tab filter/step buttons to first preset
        LowCutIndex = 4;   // 75Hz
        HighCutIndex = 1;  // 4.0KHz
        CwFilterIndex = 2; // 200Hz
        // StepIndex default is now from field initializer (=5) and restored from settings after Load()

        TuneMode = false;

        // CW tab defaults (from original CW tab)
        CwSpeed = 20;
        CwMemTextWpm = SpectrumWaterfallSettings.ClampCwMemTextWpm(SpectrumWaterfallSettings.CwMemTextWpm);
        CwKeyerMode = 1; // IAMBIC-A
        CwSpacing = 0;
        CwPaddle = 0;
        CwWeightIndex = 1; // 50
        CwPitchIndex = 1; // 600Hz
        CwHold = 100;
        CwQsk = false;
        CwPhones = false;

        // CQ / keyer memory (client sticky — KEYER_MEM0..3 in MSCC_Client.ini)
        _suppressKeyerMemSave = true;
        KeyerMem0 = SpectrumWaterfallSettings.KeyerMem0 ?? "";
        KeyerMem1 = SpectrumWaterfallSettings.KeyerMem1 ?? "";
        KeyerMem2 = SpectrumWaterfallSettings.KeyerMem2 ?? "";
        KeyerMem3 = SpectrumWaterfallSettings.KeyerMem3 ?? "";
        _suppressKeyerMemSave = false;
        KeyerMemStatus = "";

        // External electronic keyer / legacy (sticky client + mscc.ini PROFICIO-MKII)
        _externalElectronicKeyer = SpectrumWaterfallSettings.ExternalElectronicKeyer;
        _remoteAudio = SpectrumWaterfallSettings.RemoteAudio;
        // Keep mscc.ini aligned so next ms-sdr Start sees the sticky choice.
        try
        {
            ConfigBootstrap.WriteProficioMkii(mkii: !ExternalElectronicKeyer);
        }
        catch { /* best-effort */ }

        // Seed per-VFO band memory from defaults (VFO A 7.1 → 40m, VFO B 7.2 → 40m)
        _bandForVfoA = GetBandNameForFrequency(RadioState.VfoA.FrequencyHz);
        _bandForVfoB = GetBandNameForFrequency(RadioState.VfoB.FrequencyHz);
        if (_bandForVfoA == "?") _bandForVfoA = RadioState.CurrentBand ?? "40m";
        if (_bandForVfoB == "?") _bandForVfoB = "40m";

        RadioState.PropertyChanged += (s, e) =>
        {
            if (e.PropertyName == nameof(RadioState.ActiveVfo))
            {
                OnPropertyChanged(nameof(IsVfoAActive));
                OnPropertyChanged(nameof(IsVfoBActive));
                OnPropertyChanged(nameof(ActiveMode));

                // CMD_SET_VFO (0xF2) first, then frequency/mode — order required for split prep.
                _ = ApplyActiveVfoToRadioAsync();

                // Subscribe to filter sub-property changes for sending to service
                if (RadioState.ActiveVfo?.Filter != null)
                {
                    RadioState.ActiveVfo.Filter.PropertyChanged += OnActiveFilterPropertyChanged;
                }

                if (RadioState.ActiveVfo != null)
                {
                    RadioState.ActiveVfo.PropertyChanged += OnActiveVfoPropertyChanged;
                }
            }

            // Keep each VFO's last band in sync whenever CurrentBand changes (band buttons, GEN, server report).
            if (e.PropertyName == nameof(RadioState.CurrentBand))
            {
                StoreBandForActiveVfo(RadioState.CurrentBand);
                // Keep Favorites tab band filter aligned with radio band when it changes.
                string nb = NormalizeFavoriteBand(RadioState.CurrentBand, RadioState.ActiveVfo.FrequencyHz);
                if (FavoriteBandChoices.Any(b => string.Equals(b, nb, StringComparison.OrdinalIgnoreCase)) &&
                    !string.Equals(FavoriteBandFilter, nb, StringComparison.OrdinalIgnoreCase))
                {
                    FavoriteBandFilter = nb;
                }
            }

            if (e.PropertyName == nameof(RadioState.RfPowerPercent))
            {
                _ = _radioService.SetRfPowerAsync(RadioState.RfPowerPercent);
            }

            if (e.PropertyName == nameof(RadioState.PVolume))
            {
                _ = _radioService.SetPhonesVolumeLevelAsync(RadioState.PVolume);
                MonitorTextBoxText($" PhonesVolumeLevel set: {RadioState.PVolume}");
            }

            if (e.PropertyName == nameof(RadioState.DVolume))
            {
                _ = _radioService.SetDigitalVolumeLevelAsync(RadioState.DVolume);
                MonitorTextBoxText($" DigitalVolumeLevel set: {RadioState.DVolume}");
            }

            if (e.PropertyName == nameof(RadioState.PMicGain))
            {
                _ = _radioService.SetPhonesMicGainLevelAsync(RadioState.PMicGain);
                MonitorTextBoxText($" PhonesMicGainLevel set: {RadioState.PMicGain}");
            }

            if (e.PropertyName == nameof(RadioState.DMicGain))
            {
                _ = _radioService.SetDigitalMicGainLevelAsync(RadioState.DMicGain);
                MonitorTextBoxText($" DigitalMicGainLevel set: {RadioState.DMicGain}");
            }

            // Wire filter changes (on active VFO) to service
            if (e.PropertyName == nameof(RadioState.ActiveVfo.Filter) && RadioState.ActiveVfo != null)
            {
                var f = RadioState.ActiveVfo.Filter;
                _ = _radioService.SetFilterLowAsync(f.LowHz);
                _ = _radioService.SetFilterHighAsync(f.HighHz);
                _ = _radioService.SetCwPitchAsync(f.CwPitchHz);
            }
        };

        _radioService.SpectrumUpdated += update =>
        {
            // Throttle display updates based on SpectrumRefresh (user setting in S/W popup).
            // E.g. 4 means render 1 out of every 4 frames to reduce CPU/GPU load.
            int div = Math.Max(1, SpectrumWaterfallSettings.SpectrumRefresh);
            _spectrumFrameCounter = (_spectrumFrameCounter + 1) % div;
            if (_spectrumFrameCounter != 0)
            {
                // Drop this frame for display (still process UDP, just skip render)
                return;
            }

            var enriched = EnrichSpectrumUpdate(update);
            // Spectrum frames arrive on background receive thread; must marshal to UI thread
            // so that the ObservableProperty change notification + binding to SpectrumDisplayControl works reliably.
            var dispatcher = System.Windows.Application.Current?.Dispatcher;
            if (dispatcher != null && !dispatcher.CheckAccess())
            {
                dispatcher.BeginInvoke(() =>
                {
                    CurrentSpectrum = enriched;
                    PowerOut = (int)(RadioState.RfPowerPercent * 0.85 + (SMeter > 8 ? 12 : 0));
                    ForwardPower = PowerOut;
                    ReversePower = Math.Max(0, PowerOut / 5);
                    Swr = (ReversePower > 0) ? Math.Clamp(ReversePower * 3 / 10 + 1, 1, 10) : 1;
                });
            }
            else
            {
                CurrentSpectrum = enriched;
                PowerOut = (int)(RadioState.RfPowerPercent * 0.85 + (SMeter > 8 ? 12 : 0));
                ForwardPower = PowerOut;
                ReversePower = Math.Max(0, PowerOut / 5);
                Swr = (ReversePower > 0) ? Math.Clamp(ReversePower * 3 / 10 + 1, 1, 10) : 1;
            }
            // No per-update log here (spectrum frames are very high volume; would flood the log).
        };

        // All server report handlers (PacketReceived, FrequencyReported, BandReported, ModeReported, etc.)
        // are wired exclusively inside WireService() (called later in ctor).
        // This prevents duplicate subscriptions that previously caused double logs and double side-effects.
        // RULE: Report handlers (pushes FROM ms_sdr) must be read-only for state. Never send anything back.

        WireService(_radioService);

        // Wire listeners for the initial ActiveVfo (RadioState ctor sets _activeVfo directly without raising
        // PropertyChanged("ActiveVfo"), so the attachment inside the RadioState handler never runs for startup).
        // This ensures freq changes, mode changes (for bandpass), etc. are wired from the start.
        if (RadioState.ActiveVfo != null)
        {
            RadioState.ActiveVfo.PropertyChanged += OnActiveVfoPropertyChanged;
            if (RadioState.ActiveVfo.Filter != null)
            {
                RadioState.ActiveVfo.Filter.PropertyChanged += OnActiveFilterPropertyChanged;
            }
        }

        // Client-side favorites (MSCC_Favorites.ini) — no ms-sdr traffic
        LoadFavoritesFromStore();

        // Power Cal / Amp Cal band status + selection UI
        InitPowerCalBandStatuses();
        InitAmpCalBandStatuses();

        // Note: StartAsync (which launches subsystems, sends CMD_CHECK_GUI_STATUS (0xFE) with data=1 to signal
        // GUI ready/initialized (ms-sdr then sends versions, startup freq/band, status etc.), and starts periodic 0xF4
        // CMD_SET_KEEP_ALIVE "I'm Alive" messages) is now only triggered explicitly by the Start button.
        // The keep-alive tells ms-sdr the UI is still running (original Master_State_Machine behavior).
        // IsConnected will update when the button is used.
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
        // Default filter to radio's current band when possible
        string radioBand = NormalizeFavoriteBand(RadioState.CurrentBand, RadioState.ActiveVfo.FrequencyHz);
        if (FavoriteBandChoices.Any(b => string.Equals(b, radioBand, StringComparison.OrdinalIgnoreCase)))
            FavoriteBandFilter = radioBand;
        RefreshFavoritesForBand();
        MonitorTextBoxText($" Favorites loaded: {Favorites.Count} from {FavoritesStore.StorePath}");
    }

    private void InitPowerCalBandStatuses()
    {
        try
        {
            PowerCalStatusStore.EnsureFileExists();
            var flags = PowerCalStatusStore.Load();

            PowerCalBandStatuses.Clear();
            foreach (int n in PowerCalStatusStore.BandNumbers)
            {
                bool cal = flags.TryGetValue(n, out bool v) && v;
                // Create then set IsCalibrated so ObservableProperty notifies bindings
                var item = new PowerCalBandItem
                {
                    BandNumber = n,
                    BandLabel = n.ToString() // no "M" — shorter cal band buttons
                };
                item.IsCalibrated = cal;
                PowerCalBandStatuses.Add(item);
                string keyHint = n is 2200 or 630 ? $"GEMINUS_B{n}" : $"PROFICIO_B{n}";
                MonitorTextBoxText(
                    $" QRP cal status lamp: {keyHint}={(cal ? 1 : 0)} → IsCalibrated={cal}");
            }
            PowerCalSelectedBand = 0;
            PowerCalSliderValue = 0;
            PowerCalStepLabel = "CALIBRATION STEP: —";
            bool exists = File.Exists(PowerCalStatusStore.StorePath);
            MonitorTextBoxText(
                $" QRP cal status: file={(exists ? "OK" : "MISSING")} path={PowerCalStatusStore.StorePath} " +
                $"(calibrated: {PowerCalBandStatuses.Count(b => b.IsCalibrated)}/{PowerCalBandStatuses.Count})");
        }
        catch (Exception ex)
        {
            MonitorTextBoxText($" QRP cal status init FAILED: {ex.Message}");
            // Still populate UI lamps so the tab is usable
            PowerCalBandStatuses.Clear();
            foreach (int n in PowerCalStatusStore.BandNumbers)
            {
                PowerCalBandStatuses.Add(new PowerCalBandItem
                {
                    BandNumber = n,
                    BandLabel = n.ToString(),
                    IsCalibrated = false
                });
            }
        }
    }

    /// <summary>
    /// Persist current CALIBRATION STATUS lamps to client-settings.ini (MSCC-owned only).
    /// </summary>
    public void SavePowerCalStatus()
    {
        var map = PowerCalBandStatuses.ToDictionary(b => b.BandNumber, b => b.IsCalibrated);
        PowerCalStatusStore.Save(map);
        MonitorTextBoxText($" QRP cal status saved → {PowerCalStatusStore.StorePath}");
    }

    private void InitAmpCalBandStatuses()
    {
        try
        {
            AmpCalStatusStore.EnsureFileExists();
            var flags = AmpCalStatusStore.Load();

            AmpCalBandStatuses.Clear();
            foreach (int n in AmpCalStatusStore.BandNumbers)
            {
                bool cal = flags.TryGetValue(n, out bool v) && v;
                var item = new PowerCalBandItem
                {
                    BandNumber = n,
                    BandLabel = n.ToString()
                };
                item.IsCalibrated = cal;
                AmpCalBandStatuses.Add(item);
                MonitorTextBoxText(
                    $" Amp cal status lamp: AMP_B{n}={(cal ? 1 : 0)} → IsCalibrated={cal}");
            }
            AmpCalSelectedBand = 0;
            AmpCalSliderValue = -99;
            AmpCalStepLabel = "STEP: 0";
            bool exists = File.Exists(AmpCalStatusStore.StorePath);
            MonitorTextBoxText(
                $" Amp cal status: file={(exists ? "OK" : "MISSING")} path={AmpCalStatusStore.StorePath} " +
                $"(calibrated: {AmpCalBandStatuses.Count(b => b.IsCalibrated)}/{AmpCalBandStatuses.Count})");
        }
        catch (Exception ex)
        {
            MonitorTextBoxText($" Amp cal status init FAILED: {ex.Message}");
            AmpCalBandStatuses.Clear();
            foreach (int n in AmpCalStatusStore.BandNumbers)
            {
                AmpCalBandStatuses.Add(new PowerCalBandItem
                {
                    BandNumber = n,
                    BandLabel = n.ToString(),
                    IsCalibrated = false
                });
            }
        }
    }

    public void SaveAmpCalStatus()
    {
        var map = AmpCalBandStatuses.ToDictionary(b => b.BandNumber, b => b.IsCalibrated);
        AmpCalStatusStore.Save(map);
        MonitorTextBoxText($" Amp cal status saved → {AmpCalStatusStore.StorePath}");
    }

    public void SetAmpCalBandCalibrated(int bandNumber, bool calibrated)
    {
        var item = AmpCalBandStatuses.FirstOrDefault(b => b.BandNumber == bandNumber);
        if (item == null) return;
        if (item.IsCalibrated == calibrated) return;
        item.IsCalibrated = calibrated;
        SaveAmpCalStatus();
    }

    /// <summary>
    /// Amp cal / TX IQ band frequencies from original IQ_Controls.iq_calibration_freqs
    /// (B10…B160, then B630=475000, B2200=135750).
    /// </summary>
    private static long GetAmpCalFrequencyHz(int bandNumber) => bandNumber switch
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

    private static long GetTxIqFrequencyHz(int bandNumber) => GetAmpCalFrequencyHz(bandNumber);

    private void EnsureTxIqBandItems()
    {
        if (TxIqBandItems.Count > 0) return;
        // Same band set and order as QRP / AMP CAL (2200/630 + 160–10); no radio-model gray-out.
        foreach (int n in PowerCalStatusStore.BandNumbers)
        {
            TxIqBandItems.Add(new PowerCalBandItem
            {
                BandNumber = n,
                BandLabel = n.ToString(), // no "M" — match QRP/AMP CAL band buttons
                IsCalibrated = false,
                IsSelected = false
            });
        }
    }

    /// <summary>Enter TX IQ tab: QRP only; snapshot; TX off; tune power 100.</summary>
    public void EnterTxIqTab()
    {
        EnsureTxIqBandItems();
        if (_txIqTabActive) return;
        _txIqTabActive = true;

        if (AmpOn)
        {
            TxIqStatus = "TX IQ requires QRP (turn AMP off).";
            MonitorTextBoxText(" TX IQ ENTER blocked: AMP is on");
            return;
        }

        _preTxIqState = CaptureOperatingStateSnapshot();
        if (PttOn) PttOn = false;
        if (TuneMode) TuneMode = false;
        _ = _radioService.SetTransmitAsync(false);
        _ = _radioService.SetAutoTuneAsync(false);
        _ = _radioService.SetTunePowerAsync(100);
        _suppressTxIqPower = true;
        try { TxIqPower = 100; }
        finally { _suppressTxIqPower = false; }

        TxIqTxOn = false;
        TxIqCommitting = false;
        TxIqStatus = "Select a band, then TX ON. Adjust OFFSET (use external RX for image). APPLY to commit.";
        MonitorTextBoxText(
            $" TX IQ ENTER: snapshot saved, TUNE_POWER 100, QRP only (AmpOn={AmpOn})");
    }

    /// <summary>Leave TX IQ: stop carrier, restore snapshot.</summary>
    public void LeaveTxIqTab()
    {
        if (!_txIqTabActive) return;
        _txIqTabActive = false;

        ForceStopTxIqSession("leave tab");

        TxIqSelectedBand = 0;
        foreach (var item in TxIqBandItems)
            item.IsSelected = false;
        _suppressTxIqOffset = true;
        try { TxIqOffset = 0; }
        finally { _suppressTxIqOffset = false; }
        TxIqCommitting = false;
        TxIqStatus = "Select a band (QRP only).";

        var snap = _preTxIqState;
        _preTxIqState = null;
        if (snap == null)
        {
            MonitorTextBoxText(" TX IQ LEAVE: no snapshot");
            return;
        }
        _ = RestoreOperatingStateAfterCalTabAsync(snap, "TX IQ");
    }

    private void ForceStopTxIqSession(string reason)
    {
        if (TxIqTxOn || RadioState.IsTransmitting)
        {
            _ = _radioService.SetAutoTuneAsync(false);
            _ = _radioService.SetIqCalibrationTuneAsync(false);
            _ = _radioService.SetTransmitAsync(false);
            RadioState.IsTransmitting = false;
        }
        TxIqTxOn = false;
        MonitorTextBoxText($" TX IQ session stop ({reason})");
    }

    [RelayCommand]
    private void SelectTxIqBand(object? param)
    {
        if (AmpOn)
        {
            MessageBox.Show("TX IQ balance requires QRP mode (AMP off).", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }
        if (TxIqTxOn)
        {
            MessageBox.Show("Turn TX OFF before changing band.", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }

        int band = 0;
        if (param is int i) band = i;
        else if (param is string s) int.TryParse(s, out band);
        if (band <= 0) return;

        EnsureTxIqBandItems();
        foreach (var item in TxIqBandItems)
            item.IsSelected = item.BandNumber == band;
        TxIqSelectedBand = band;

        long freq = GetTxIqFrequencyHz(band);
        if (freq > 0)
        {
            RadioState.ActiveVfo.FrequencyHz = freq;
            _ = _radioService.SetFrequencyAsync(freq);
        }
        // Original band cycle only sets freq; IQ_BAND is sent when TX turns on via IQBD_Set_Band.
        // Also pre-send IQ band so path is selected early.
        _ = _radioService.SetIqBandAsync(band);

        _suppressTxIqOffset = true;
        try { TxIqOffset = 0; }
        finally { _suppressTxIqOffset = false; }
        TxIqStatus = $"{band}M selected. Set power, then TX ON.";
        MonitorTextBoxText($" TX IQ band select: {band}m freq={freq}");
    }

    partial void OnTxIqOffsetChanged(int value)
    {
        if (_suppressTxIqOffset) return;
        if (!TxIqTxOn || TxIqSelectedBand <= 0) return;
        int v = Math.Clamp(value, -200, 200);
        if (v != value)
        {
            _suppressTxIqOffset = true;
            try { TxIqOffset = v; }
            finally { _suppressTxIqOffset = false; }
        }
        _ = _radioService.SetIqOffsetAsync(v);
        MonitorTextBoxText($" TX IQ offset: {v}");
    }

    partial void OnTxIqPowerChanged(int value)
    {
        if (_suppressTxIqPower) return;
        if (TxIqSelectedBand <= 0) return;
        int p = Math.Clamp(value, 0, 100);
        _ = _radioService.SetTunePowerAsync(p);
        MonitorTextBoxText($" TX IQ power: {p}%");
    }

    [RelayCommand]
    private void ToggleTxIqTx()
    {
        if (AmpOn)
        {
            MessageBox.Show("TX IQ balance requires QRP mode (AMP off).", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }
        if (TxIqSelectedBand <= 0)
        {
            MessageBox.Show("SELECT A BAND", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }

        if (!TxIqTxOn)
        {
            // Original IQBD_TX_Tune(true):
            // IQ_CALIBRATION_RX_TX TX, Set_Band, mute, TUNE mode, RIG_TUNE 1
            _ = _radioService.SetIqCalibrationRxTxAsync(true);
            _ = _radioService.SetIqBandAsync(TxIqSelectedBand);
            _ = _radioService.SetTunePowerAsync(TxIqPower);
            RadioState.ActiveVfo.Mode = RadioMode.TUNE;
            OnPropertyChanged(nameof(ActiveMode));
            _ = _radioService.SetModeAsync("TUNE");
            _ = _radioService.SetAutoTuneAsync(true);
            TxIqTxOn = true;
            RadioState.IsTransmitting = true;
            TxIqStatus = "TX ON — adjust OFFSET (external RX), then APPLY or TX OFF.";
            MonitorTextBoxText(
                $" TX IQ TX ON band={TxIqSelectedBand} power={TxIqPower} → 0x55 TX, 0x58, TUNE, RIG_TUNE");
        }
        else
        {
            // TX OFF only — commit is explicit via APPLY button (no dialog here)
            StopTxIqCarrier();
            TxIqStatus = "TX OFF. Use APPLY to commit the current I/Q value if desired.";
        }
    }

    private void StopTxIqCarrier()
    {
        _ = _radioService.SetAutoTuneAsync(false);
        _ = _radioService.SetIqCalibrationTuneAsync(false);
        TxIqTxOn = false;
        RadioState.IsTransmitting = false;
        // Restore mode from enter snapshot if available
        if (_preTxIqState != null)
        {
            RadioMode mode = _preTxIqState.IsVfoB ? _preTxIqState.VfoBMode : _preTxIqState.VfoAMode;
            RadioState.ActiveVfo.Mode = mode;
            OnPropertyChanged(nameof(ActiveMode));
            _ = _radioService.SetModeAsync(FormatModeDisplay(mode));
        }
        MonitorTextBoxText(" TX IQ TX OFF → RIG_TUNE 0");
    }

    [RelayCommand]
    private void ApplyTxIq()
    {
        if (AmpOn)
        {
            MessageBox.Show("TX IQ balance requires QRP mode (AMP off).", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }
        if (TxIqSelectedBand <= 0)
        {
            MessageBox.Show("Select a Band", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }

        var ret = MessageBox.Show(
            "APPLY THE CURRENT I/Q VALUE?",
            "MSCC",
            MessageBoxButton.YesNo,
            MessageBoxImage.Question,
            MessageBoxResult.No);
        if (ret != MessageBoxResult.Yes) return;
        CommitTxIqValue();
    }

    private void CommitTxIqValue()
    {
        TxIqCommitting = true;
        TxIqStatus = "APPLYING…";
        _ = _radioService.CommitIqAsync();
        MonitorTextBoxText($" TX IQ COMMIT (0x57) band={TxIqSelectedBand} offset={TxIqOffset}");
    }

    [RelayCommand]
    private void ResetAllTxIq()
    {
        if (AmpOn)
        {
            MessageBox.Show("TX IQ balance requires QRP mode (AMP off).", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }
        if (TxIqTxOn)
        {
            MessageBox.Show("Turn TX OFF before reset.", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }

        var ret = MessageBox.Show(
            "THIS RESETS ALL I/Q VALUES TO FACTORY VALUES.\r\nARE YOU SURE YOU WANT TO CONTINUE?",
            "MSCC",
            MessageBoxButton.YesNo,
            MessageBoxImage.Warning,
            MessageBoxResult.No);
        if (ret != MessageBoxResult.Yes) return;

        _ = _radioService.ResetAllIqBandsAsync();
        _suppressTxIqOffset = true;
        try { TxIqOffset = 0; }
        finally { _suppressTxIqOffset = false; }
        TxIqStatus = "Factory reset of all I/Q bands requested.";
        MonitorTextBoxText(" TX IQ RESET ALL BANDS (0x8D)");
    }

    /// <summary>
    /// Select band for amp power cal (original Calibration_Band_button8_Click):
    /// set cal frequency, CMD_SET_AMPLIFIER_INITIALIZE (0xF9), CMD_SET_AMPLIFIER_POWER (0xFA)=100,
    /// reset cal slider to -99.
    /// </summary>
    [RelayCommand]
    private void SelectAmpCalBand(object? param)
    {
        int band = param switch
        {
            int i => i,
            string s when int.TryParse(s, out int n) => n,
            _ => 0
        };
        if (band <= 0) return;

        // Original: band select disabled while PA TX/calibrating
        if (AmpCalTxOn || AmpCalCalibrating)
        {
            MessageBox.Show(
                "TX ON. BAND CHANGE NOT PERMITTED\r\nSET TX OFF",
                "MSCC",
                MessageBoxButton.OK,
                MessageBoxImage.Asterisk);
            return;
        }

        long freq = GetAmpCalFrequencyHz(band);
        if (freq <= 0)
        {
            MonitorTextBoxText($" Amp Cal band {band}: no calibration frequency");
            return;
        }

        // Same band again: still re-send init/power like cycling could re-hit same band; skip only if identical UI re-click spam
        if (AmpCalSelectedBand == band)
            return;

        AmpCalSelectedBand = band;
        foreach (var item in AmpCalBandStatuses)
            item.IsSelected = item.BandNumber == band;

        // Client radio state (display)
        RadioState.ActiveVfo.FrequencyHz = freq;
        RadioState.CurrentBand = band + "m";
        StoreBandForActiveVfo(RadioState.CurrentBand);

        // Original: CMD_SET_MAIN_FREQ to iq_calibration_freqs[band]
        _ = _radioService.SetFrequencyAsync(freq);
        // Original: CMD_SET_AMPLIFIER_INITIALIZE (short)Band
        _ = _radioService.SetAmplifierInitializeAsync(band);
        // Original: CMD_SET_AMPLIFIER_POWER Power_Value=100
        _ = _radioService.SetAmplifierPowerAsync(100);

        // Original: PA_Calibration_Increment path / PA_hScrollBar.Value = -99
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

        MonitorTextBoxText(
            $" Amp Cal band selected: {band}m f={freq} → 0xB6, 0xF9 band={band}, 0xFA 100, slider=-99");
    }

    /// <summary>Original Step_Display = 100 + PA_Calibration_Increment (−99…0 → STEP 1…100).</summary>
    private static int AmpCalStepFromSlider(int sliderValue) => 100 + Math.Clamp(sliderValue, -99, 0);

    private void RestoreModeAfterAmpCalTx()
    {
        RadioMode restoreMode = RadioMode.USB;
        if (_preAmpCalState != null)
        {
            restoreMode = _preAmpCalState.IsVfoB
                ? _preAmpCalState.VfoBMode
                : _preAmpCalState.VfoAMode;
        }
        if (restoreMode == RadioMode.TUNE)
            restoreMode = RadioMode.USB;

        RadioState.ActiveVfo.Mode = restoreMode;
        OnPropertyChanged(nameof(ActiveMode));
        _ = _radioService.SetModeAsync(FormatModeDisplay(restoreMode));
    }

    private void StopAmpCalTuneCarrier(string reason)
    {
        _ = _radioService.SetAutoTuneAsync(false);
        AmpCalTxOn = false;
        AmpCalCalibrating = false;
        RadioState.IsTransmitting = false;
        RestoreModeAfterAmpCalTx();
        MonitorTextBoxText($" Amp Cal tune/TX OFF ({reason}) → RIG_TUNE 0");
    }

    /// <summary>
    /// AMP CAL TX toggle (original PA_TX_button8_Click).
    /// On: TUNE_POWER 100, mode TUNE, RIG_TUNE 1. Off: RIG_TUNE 0, restore mode from enter snapshot.
    /// Disabled while manual CALIBRATE session is active (use CALIBRATE to end).
    /// </summary>
    [RelayCommand]
    private void ToggleAmpCalTx()
    {
        if (AmpCalCalibrating)
        {
            MessageBox.Show("Finish or cancel CALIBRATE first.", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }

        if (AmpCalSelectedBand <= 0)
        {
            MessageBox.Show("Select a Band", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }

        if (!AmpCalTxOn)
        {
            // Original: CMD_SET_TUNE_POWER 100, CMD_SET_MAIN_MODE TUNE, CMD_SET_RIG_TUNE 1
            _ = _radioService.SetTunePowerAsync(100);
            RadioState.ActiveVfo.Mode = RadioMode.TUNE;
            OnPropertyChanged(nameof(ActiveMode));
            _ = _radioService.SetModeAsync("TUNE");
            _ = _radioService.SetAutoTuneAsync(true);
            AmpCalTxOn = true;
            RadioState.IsTransmitting = true;
            MonitorTextBoxText(
                $" Amp Cal TX ON band={AmpCalSelectedBand} → TUNE_POWER 100, mode TUNE, RIG_TUNE 1 (0xA6)");
        }
        else
        {
            StopAmpCalTuneCarrier("TX button");
        }
    }

    /// <summary>
    /// AMP CAL manual calibrate toggle (original PA_Manual_Calibrate via PA_Calibrate_button).
    /// Start: requires band + transceiver cal for band; TUNE + RIG_TUNE; potentiia −99; enable slider.
    /// Stop: RIG_TUNE off, restore mode, disable slider.
    /// </summary>
    [RelayCommand]
    private void ToggleAmpCalCalibrate()
    {
        if (AmpCalSelectedBand <= 0)
        {
            MessageBox.Show("SELECT A BAND", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }

        // Original shares Tuning_Mode: if TX/cal already on, button ends the session
        if (AmpCalCalibrating || AmpCalTxOn)
        {
            bool wasCal = AmpCalCalibrating;
            int band = AmpCalSelectedBand;
            StopAmpCalTuneCarrier(wasCal ? "CALIBRATE stop" : "CALIBRATE (was TX)");
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

            // After a manual calibrate session: accept → green status lamp (client amp-cal-status.ini)
            if (wasCal && band > 0)
            {
                var ret = MessageBox.Show(
                    "ACCEPT THIS CALIBRATION ?",
                    "MSCC",
                    MessageBoxButton.YesNoCancel,
                    MessageBoxImage.Question,
                    MessageBoxResult.Cancel);

                if (ret == MessageBoxResult.Yes)
                {
                    SetAmpCalBandCalibrated(band, true);
                    MonitorTextBoxText($" Amp Cal ACCEPT band={band}m calibrated (status lamp)");
                }
                else if (ret == MessageBoxResult.No)
                {
                    SetAmpCalBandCalibrated(band, false);
                    MessageBox.Show("CALIBRATION NOT SET", "MSCC",
                        MessageBoxButton.OK, MessageBoxImage.Information);
                    MonitorTextBoxText($" Amp Cal REJECT band={band}m not calibrated");
                }
                else
                {
                    // Cancel: leave lamp unchanged (value already sent during slider; no restore of 0x08 prior)
                    MessageBox.Show("CALIBRATION CANCELLED", "MSCC",
                        MessageBoxButton.OK, MessageBoxImage.Information);
                    MonitorTextBoxText($" Amp Cal CANCEL band={band}m status lamp unchanged");
                }
            }
            return;
        }

        // Original PA_Check_Proficio_Calibration — transceiver must be calibrated for this band
        var xcvCal = PowerCalBandStatuses.FirstOrDefault(b => b.BandNumber == AmpCalSelectedBand);
        if (xcvCal == null || !xcvCal.IsCalibrated)
        {
            MessageBox.Show(
                "QRP / TRANSCEIVER HAS NOT BEEN CALIBRATED FOR THIS BAND",
                "MSCC",
                MessageBoxButton.OK,
                MessageBoxImage.Asterisk);
            return;
        }

        // Start manual calibrate (original PA_Manual_Calibrate start path)
        _ = _radioService.SetTunePowerAsync(100);
        RadioState.ActiveVfo.Mode = RadioMode.TUNE;
        OnPropertyChanged(nameof(ActiveMode));
        _ = _radioService.SetModeAsync("TUNE");
        _ = _radioService.SetAutoTuneAsync(true);

        AmpCalCalibrating = true;
        AmpCalTxOn = true; // radio is transmitting; TX button shows ON
        RadioState.IsTransmitting = true;

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

        // Original: CMD_SET_POTENTIA_CALIBRATION −99
        _ = _radioService.SetPotentiaCalibrationAsync(-99);

        MonitorTextBoxText(
            $" Amp Cal CALIBRATE START band={AmpCalSelectedBand} → TUNE + RIG_TUNE 1, 0x08 −99, slider enabled");
    }

    partial void OnAmpCalSliderValueChanged(int value)
    {
        value = Math.Clamp(value, -99, 0);
        int step = AmpCalStepFromSlider(value);
        if (!_suppressAmpCalSlider)
            AmpCalStepLabel = $"STEP: {step}";

        // Original PA_hScrollBar_Scroll → CMD_SET_POTENTIA_CALIBRATION (int32)
        if (!_suppressAmpCalSlider && AmpCalCalibrating)
        {
            _ = _radioService.SetPotentiaCalibrationAsync(value);
            MonitorTextBoxText($" Amp Cal slider → 0x08: {value} (STEP {step})");
        }
    }

    /// <summary>
    /// Set calibrated flag for a QRP CAL band and save client-settings.ini.
    /// </summary>
    public void SetPowerCalBandCalibrated(int bandNumber, bool calibrated)
    {
        var item = PowerCalBandStatuses.FirstOrDefault(b => b.BandNumber == bandNumber);
        if (item == null) return;
        if (item.IsCalibrated == calibrated) return;
        item.IsCalibrated = calibrated;
        SavePowerCalStatus();
    }

    private bool _suppressPowerCalSlider;

    /// <summary>
    /// Select band for QRP power cal: UI highlight + CMD_SET_BAND_POWER_BAND (0xA1).
    /// Band number is meters (160…10, 2200, 630) — same as original Geminus handlers.
    /// Server replies with CMD_GET_BAND_POWER (0xB4) → updates slider / step label.
    /// </summary>
    [RelayCommand]
    private void SelectPowerCalBand(object? param)
    {
        int band = param switch
        {
            int i => i,
            string s when int.TryParse(s, out int n) => n,
            _ => 0
        };
        if (band <= 0) return;

        // Original: no band change while TX (calibration tune) is on
        if (PowerCalTxOn || PowerCalCalibrating)
        {
            MessageBox.Show(
                "TX ON. BAND CHANGE NOT PERMITTED\r\nSET TX OFF",
                "MSCC",
                MessageBoxButton.OK,
                MessageBoxImage.Asterisk);
            return;
        }

        if (PowerCalSelectedBand == band)
            return;

        PowerCalSelectedBand = band;
        foreach (var item in PowerCalBandStatuses)
            item.IsSelected = item.BandNumber == band;

        // Keep VFO/band label in sync (esp. LF); server table select is still 0xA1 only.
        RadioState.CurrentBand = band + "m";
        StoreBandForActiveVfo(RadioState.CurrentBand);
        long calFreq = GetAmpCalFrequencyHz(band);
        if (calFreq > 0)
        {
            RadioState.ActiveVfo.FrequencyHz = calFreq;
            _ = _radioService.SetFrequencyAsync(calFreq);
        }

        MonitorTextBoxText($" QRP Cal band selected: {band}m f={calFreq} → 0xA1");
        _ = _radioService.SetBandPowerBandAsync(band);
    }

    partial void OnPowerCalSliderValueChanged(int value)
    {
        value = Math.Clamp(value, 0, 100);
        if (!_suppressPowerCalSlider)
            PowerCalStepLabel = $"CALIBRATION STEP: {value}";

        // Original: Proficio_Calibrate_Power_hScrollBar_Scroll → CMD_SET_BAND_POWER_POWER (0xA2)
        // Only when user moves the slider during an active calibrate session (not server 0xB4 apply).
        if (!_suppressPowerCalSlider && PowerCalCalibrating)
        {
            _ = _radioService.SetBandPowerPowerAsync(value);
            MonitorTextBoxText($" Pwr Cal power slider → 0xA2: {value}");
        }
    }

    /// <summary>Apply server calibration step (0xB4) to slider and label without echoing 0xA2.</summary>
    private void ApplyBandPowerReport(int step)
    {
        step = Math.Clamp(step, 0, 100);
        _powerCalPreviousReceivedStep = step;
        // Do not move the slider while user is actively calibrating (they are driving 0xA2).
        if (PowerCalCalibrating)
        {
            PowerCalStepLabel = $"CALIBRATION STEP: {PowerCalSliderValue}";
            MonitorTextBoxText($" Pwr Cal step from server (0xB4): {step} (ignored while calibrating)");
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
        MonitorTextBoxText($" Pwr Cal step from server (0xB4): {step}");
    }

    /// <summary>Once per session: user confirmed antenna/dummy load warning before cal TX.</summary>
    private bool _powerCalWarningAccepted;

    /// <summary>
    /// Ensure antenna/dummy-load warning accepted once per session. Returns false if user declines.
    /// </summary>
    private bool EnsurePowerCalWarningAccepted()
    {
        if (_powerCalWarningAccepted)
            return true;

        var ret = MessageBox.Show(
            "IS A WELL MATCHED ANTENNA OR DUMMY LOAD ATTACHED?\n\n" +
            "A 50Ω 5W OR BETTER DUMMY LOAD IS PREFERRED",
            "MSCC",
            MessageBoxButton.YesNo,
            MessageBoxImage.Asterisk);
        if (ret != MessageBoxResult.Yes)
            return false;
        _powerCalWarningAccepted = true;
        return true;
    }

    /// <summary>
    /// Pwr Cal TX toggle → CMD_CALIBRATION_TUNE (0xAC). Requires band selected; first use shows antenna warning.
    /// Disabled while manual CALIBRATE session is active (original mutual exclusion).
    /// </summary>
    [RelayCommand]
    private void TogglePowerCalTx()
    {
        if (PowerCalCalibrating)
        {
            MessageBox.Show("Finish or cancel CALIBRATE first.", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }

        if (PowerCalSelectedBand <= 0)
        {
            MessageBox.Show("Select a Band", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }

        if (!PowerCalTxOn && !EnsurePowerCalWarningAccepted())
            return;

        bool turnOn = !PowerCalTxOn;
        PowerCalTxOn = turnOn;
        _ = _radioService.SetCalibrationTuneAsync(turnOn);
        MonitorTextBoxText($" Pwr Cal TX: {(turnOn ? "ON" : "OFF")} → 0xAC {(turnOn ? 1 : 0)} band={PowerCalSelectedBand}");
    }

    /// <summary>
    /// Manual CALIBRATE toggle (original Proficio_Manual_Calibrate_button_Click).
    /// Start: slider=0 → 0xA2 0, TX on (0xAC 1). Stop: TX off, accept Yes/No/Cancel for status lamps.
    /// </summary>
    [RelayCommand]
    private void TogglePowerCalCalibrate()
    {
        if (PowerCalSelectedBand <= 0)
        {
            MessageBox.Show("Select a Band", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }

        // TX-only path is active — must turn TX off before calibrate session (original disables CALIBRATE while TX).
        if (PowerCalTxOn && !PowerCalCalibrating)
        {
            MessageBox.Show("TX ON. SET TX OFF BEFORE CALIBRATE", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }

        if (!PowerCalCalibrating)
        {
            if (!EnsurePowerCalWarningAccepted())
                return;

            // Start calibrate session (original: enable scrollbar, value=0, 0xA2=0, 0xAC=1)
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

            _ = _radioService.SetBandPowerPowerAsync(0);
            _ = _radioService.SetCalibrationTuneAsync(true);
            MonitorTextBoxText(
                $" Pwr Cal CALIBRATE START band={PowerCalSelectedBand} → 0xA2 0, 0xAC 1");
        }
        else
        {
            // End calibrate session
            PowerCalCalibrating = false;
            PowerCalTxOn = false;
            _ = _radioService.SetCalibrationTuneAsync(false);

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

            MonitorTextBoxText(
                $" Pwr Cal CALIBRATE STOP band={PowerCalSelectedBand} → 0xAC 0");

            var ret = MessageBox.Show(
                "ACCEPT THIS CALIBRATION ?",
                "MSCC",
                MessageBoxButton.YesNoCancel,
                MessageBoxImage.Question,
                MessageBoxResult.Cancel);

            int band = PowerCalSelectedBand;
            if (ret == MessageBoxResult.Yes)
            {
                SetPowerCalBandCalibrated(band, true);
                MonitorTextBoxText($" Pwr Cal ACCEPT band={band}m calibrated");
            }
            else if (ret == MessageBoxResult.No)
            {
                SetPowerCalBandCalibrated(band, false);
                MessageBox.Show("CALIBRATION NOT SET", "MSCC",
                    MessageBoxButton.OK, MessageBoxImage.Information);
                MonitorTextBoxText($" Pwr Cal REJECT band={band}m not calibrated");
            }
            else
            {
                // Cancel: restore previous server step via 0xA2
                int restore = Math.Clamp(_powerCalPreviousReceivedStep, 0, 100);
                _ = _radioService.SetBandPowerPowerAsync(restore);
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
                MessageBox.Show("CALIBRATION CANCELLED", "MSCC",
                    MessageBoxButton.OK, MessageBoxImage.Information);
                MonitorTextBoxText(
                    $" Pwr Cal CANCEL band={band}m restore 0xA2={restore}");
            }
        }
    }

    /// <summary>Snapshot taken when entering TRANS CAL or AMP CAL (restored + sent on leave).</summary>
    private OperatingStateSnapshot? _prePowerCalState;
    private bool _powerCalTabActive;
    private OperatingStateSnapshot? _preAmpCalState;
    private bool _ampCalTabActive;

    private sealed class OperatingStateSnapshot
    {
        public bool IsVfoB;
        public long VfoAFrequencyHz;
        public long VfoBFrequencyHz;
        public RadioMode VfoAMode;
        public RadioMode VfoBMode;
        public string CurrentBand = "40m";
        public int LowCutIndex;
        public int HighCutIndex;
        public int CwFilterIndex;
        public int TunePowerPercent;
    }

    private OperatingStateSnapshot CaptureOperatingStateSnapshot() => new()
    {
        IsVfoB = RadioState.ActiveVfo == RadioState.VfoB,
        VfoAFrequencyHz = RadioState.VfoA.FrequencyHz,
        VfoBFrequencyHz = RadioState.VfoB.FrequencyHz,
        VfoAMode = RadioState.VfoA.Mode,
        VfoBMode = RadioState.VfoB.Mode,
        CurrentBand = RadioState.CurrentBand ?? GetBandNameForFrequency(RadioState.ActiveVfo.FrequencyHz),
        LowCutIndex = LowCutIndex,
        HighCutIndex = HighCutIndex,
        CwFilterIndex = CwFilterIndex,
        TunePowerPercent = TunePowerPercent
    };

    /// <summary>
    /// Enter Pwr Cal: snapshot operating state (incl. Tune power), then set tune power to 100%
    /// so the server can run calibration at full power (original powertabPage1_Enter).
    /// </summary>
    public void EnterPowerCalTab()
    {
        if (_powerCalTabActive) return;
        _powerCalTabActive = true;

        _prePowerCalState = CaptureOperatingStateSnapshot();

        // Original: Previous_Tune_power = TUNE_Power; SendCommand(CMD_SET_TUNE_POWER, 100)
        _ = _radioService.SetTunePowerAsync(100);
        MonitorTextBoxText(
            $" Pwr Cal ENTER: snapshot VFO={(_prePowerCalState.IsVfoB ? "B" : "A")} " +
            $"A={_prePowerCalState.VfoAFrequencyHz} B={_prePowerCalState.VfoBFrequencyHz} " +
            $"band={_prePowerCalState.CurrentBand} tunePwr saved={_prePowerCalState.TunePowerPercent}% → send TUNE_POWER 100");
    }

    /// <summary>
    /// Leave Pwr Cal: restore snapshot locally and push full state to ms-sdr (server only knows last cal state).
    /// </summary>
    public void LeavePowerCalTab()
    {
        if (!_powerCalTabActive) return;
        _powerCalTabActive = false;

        // End cal TX / calibrate session if still on (must tell server — it only knows last state)
        if (PowerCalTxOn || PowerCalCalibrating)
        {
            PowerCalTxOn = false;
            PowerCalCalibrating = false;
            _ = _radioService.SetCalibrationTuneAsync(false);
            MonitorTextBoxText(" Pwr Cal LEAVE: TX off → 0xAC 0");
        }

        PowerCalSelectedBand = 0;
        foreach (var item in PowerCalBandStatuses)
            item.IsSelected = false;
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

        var snap = _prePowerCalState;
        _prePowerCalState = null;
        if (snap == null)
        {
            MonitorTextBoxText(" Pwr Cal LEAVE: no snapshot (nothing to restore)");
            return;
        }

        _ = RestoreOperatingStateAfterCalTabAsync(snap, "Pwr Cal");
    }

    /// <summary>
    /// Enter AMP CAL: snapshot operating state (original MFC_Enter saves Previous_Tune_power,
    /// forces TX/tune off, sets TUNE_POWER 100 for amp cal drive).
    /// </summary>
    public void EnterAmpCalTab()
    {
        if (_ampCalTabActive) return;
        _ampCalTabActive = true;

        _preAmpCalState = CaptureOperatingStateSnapshot();

        // Original MFC_Enter: ensure TX/tune off, then TUNE_POWER 100
        if (PttOn) PttOn = false;
        if (TuneMode) TuneMode = false;
        _ = _radioService.SetTransmitAsync(false);
        _ = _radioService.SetAutoTuneAsync(false);
        _ = _radioService.SetTunePowerAsync(100);

        MonitorTextBoxText(
            $" Amp Cal ENTER: snapshot VFO={(_preAmpCalState.IsVfoB ? "B" : "A")} " +
            $"A={_preAmpCalState.VfoAFrequencyHz} B={_preAmpCalState.VfoBFrequencyHz} " +
            $"band={_preAmpCalState.CurrentBand} tunePwr saved={_preAmpCalState.TunePowerPercent}% → TX/TUNE off, TUNE_POWER 100");
    }

    /// <summary>
    /// Leave AMP CAL: clear cal UI, stop TX/tune, restore snapshot + send to ms-sdr
    /// (original MFC_Leave: RIG_TUNE 0, TX 0, restore TUNE_POWER, re-select previous band).
    /// </summary>
    public void LeaveAmpCalTab()
    {
        if (!_ampCalTabActive) return;
        _ampCalTabActive = false;

        // Original MFC_Leave: RIG_TUNE 0, TX 0 (end amp cal TX / calibrate if active)
        if (AmpCalTxOn || AmpCalCalibrating)
        {
            AmpCalTxOn = false;
            AmpCalCalibrating = false;
            RadioState.IsTransmitting = false;
            MonitorTextBoxText(" Amp Cal LEAVE: TX/cal was on → RIG_TUNE off");
        }
        if (PttOn) PttOn = false;
        if (TuneMode) TuneMode = false;
        _ = _radioService.SetTransmitAsync(false);
        _ = _radioService.SetAutoTuneAsync(false);

        // Clear amp cal UI (band was for cal freqs only)
        AmpCalSelectedBand = 0;
        foreach (var item in AmpCalBandStatuses)
            item.IsSelected = false;
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

        var snap = _preAmpCalState;
        _preAmpCalState = null;
        if (snap == null)
        {
            MonitorTextBoxText(" Amp Cal LEAVE: no snapshot (nothing to restore)");
            return;
        }

        _ = RestoreOperatingStateAfterCalTabAsync(snap, "Amp Cal");
    }

    private async Task RestoreOperatingStateAfterCalTabAsync(OperatingStateSnapshot snap, string logTag)
    {
        try
        {
            MonitorTextBoxText(
                $" {logTag} LEAVE: restoring VFO={(snap.IsVfoB ? "B" : "A")} " +
                $"freq={(snap.IsVfoB ? snap.VfoBFrequencyHz : snap.VfoAFrequencyHz)} band={snap.CurrentBand}");

            // Restore client-side VFO memories first
            bool prevSuppress = SuppressLastUsedSave;
            SuppressLastUsedSave = true;
            try
            {
                RadioState.VfoA.FrequencyHz = snap.VfoAFrequencyHz;
                RadioState.VfoB.FrequencyHz = snap.VfoBFrequencyHz;
                RadioState.VfoA.Mode = snap.VfoAMode;
                RadioState.VfoB.Mode = snap.VfoBMode;

                // Active VFO (raises ActiveVfo → ApplyActiveVfoToRadioAsync with VFO/freq/mode)
                bool wantB = snap.IsVfoB;
                bool isB = RadioState.ActiveVfo == RadioState.VfoB;
                if (wantB != isB)
                {
                    StoreBandForActiveVfo(RadioState.CurrentBand);
                    RadioState.ToggleActiveVfo();
                }

                // Ensure freqs/modes match snapshot after any ActiveVfo side effects
                RadioState.VfoA.FrequencyHz = snap.VfoAFrequencyHz;
                RadioState.VfoB.FrequencyHz = snap.VfoBFrequencyHz;
                RadioState.VfoA.Mode = snap.VfoAMode;
                RadioState.VfoB.Mode = snap.VfoBMode;

                if (!string.IsNullOrWhiteSpace(snap.CurrentBand) && snap.CurrentBand != "?")
                    RadioState.CurrentBand = snap.CurrentBand;

                LowCutIndex = snap.LowCutIndex;
                HighCutIndex = snap.HighCutIndex;
                CwFilterIndex = snap.CwFilterIndex;
                TunePowerPercent = snap.TunePowerPercent;

                OnPropertyChanged(nameof(ActiveMode));
                OnPropertyChanged(nameof(IsVfoAActive));
                OnPropertyChanged(nameof(IsVfoBActive));
            }
            finally
            {
                SuppressLastUsedSave = prevSuppress;
            }

            // Explicit full push to server (do not rely only on PropertyChanged race)
            byte vfo = snap.IsVfoB ? Opcodes.VFO_B : Opcodes.VFO_A;
            long freq = snap.IsVfoB ? snap.VfoBFrequencyHz : snap.VfoAFrequencyHz;
            string mode = FormatModeDisplay(snap.IsVfoB ? snap.VfoBMode : snap.VfoAMode);

            await _radioService.SetActiveVfoAsync(vfo).ConfigureAwait(false);
            await Task.Delay(10).ConfigureAwait(false);
            await _radioService.SetFrequencyAsync(freq).ConfigureAwait(false);
            await _radioService.SetModeAsync(mode).ConfigureAwait(false);

            // Filters: send Hz via service (same path as OnActiveFilterPropertyChanged)
            var f = RadioState.ActiveVfo.Filter;
            if (f != null)
            {
                await _radioService.SetFilterLowAsync(f.LowHz).ConfigureAwait(false);
                await _radioService.SetFilterHighAsync(f.HighHz).ConfigureAwait(false);
            }
            await _radioService.SetCwFilterAsync(snap.CwFilterIndex).ConfigureAwait(false);
            await _radioService.SetTunePowerAsync(snap.TunePowerPercent).ConfigureAwait(false);

            MonitorTextBoxText(
                $" {logTag} LEAVE: restored+sent VFO={(snap.IsVfoB ? "B" : "A")} f={freq} mode={mode} band={snap.CurrentBand} tunePwr={snap.TunePowerPercent}%");
        }
        catch (Exception ex)
        {
            MonitorTextBoxText($" {logTag} LEAVE restore FAILED: {ex.Message}");
        }
    }

    private void ApplyFavoriteLabels(FavoriteEntry e)
    {
        e.LowCutLabel = IndexLabel(LowCutOptions, e.LowCutIndex);
        e.HighCutLabel = IndexLabel(HighCutOptions, e.HighCutIndex);
        e.CwFilterLabel = IndexLabel(CwFilterOptions, e.CwFilterIndex);
    }

    private static string IndexLabel(ObservableCollection<string> options, int index)
    {
        if (options == null || options.Count == 0) return index.ToString();
        int i = Math.Clamp(index, 0, options.Count - 1);
        return options[i];
    }

    /// <summary>Normalize band keys so "20M" / "20m" match for per-band lists.</summary>
    private static string NormalizeFavoriteBand(string? band, long frequencyHz = 0)
    {
        string b = (band ?? "").Trim().ToLowerInvariant();
        if (b is "gen" or "general") return "gen";
        if (!string.IsNullOrEmpty(b) && b != "?")
            return b;
        string fromFreq = GetBandNameForFrequency(frequencyHz);
        return fromFreq == "?" ? "40m" : fromFreq;
    }

    partial void OnFavoriteBandFilterChanged(string value)
    {
        RefreshFavoritesForBand();
    }

    /// <summary>
    /// Rebuild the visible list for <see cref="FavoriteBandFilter"/> only, sorted by name then freq.
    /// Same name may exist on other bands (e.g. FT-8 on 20m and 15m).
    /// </summary>
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

    /// <summary>
    /// Sort master list by band, then name, then frequency (for stable file order).
    /// </summary>
    private void SortFavoritesMaster()
    {
        if (Favorites.Count <= 1) return;

        var ordered = Favorites
            .OrderBy(f => NormalizeFavoriteBand(f.Band, f.FrequencyHz), StringComparer.OrdinalIgnoreCase)
            .ThenBy(f => f.Name ?? "", StringComparer.OrdinalIgnoreCase)
            .ThenBy(f => f.FrequencyHz)
            .ToList();

        for (int i = 0; i < ordered.Count; i++)
        {
            int current = Favorites.IndexOf(ordered[i]);
            if (current != i && current >= 0)
                Favorites.Move(current, i);
        }
    }

    private void PersistFavorites()
    {
        SortFavoritesMaster();
        FavoritesStore.Save(Favorites);
        MonitorTextBoxText($" Favorites saved: {Favorites.Count} → {FavoritesStore.StorePath}");
    }

    /// <summary>
    /// Save current operating conditions under <see cref="FavoriteNameInput"/> for the
    /// <em>current radio band</em> (client-only). Name uniqueness is per-band.
    /// </summary>
    [RelayCommand]
    private void SaveFavorite()
    {
        string name = (FavoriteNameInput ?? "").Trim();
        if (string.IsNullOrEmpty(name) ||
            string.Equals(name, "NAME", StringComparison.OrdinalIgnoreCase))
        {
            MessageBox.Show("Enter a name for this favorite.", "Favorites",
                MessageBoxButton.OK, MessageBoxImage.Information);
            return;
        }

        if (name.Length > 32)
            name = name.Substring(0, 32);

        // Always store under the radio's current band (not the filter alone)
        string band = NormalizeFavoriteBand(
            RadioState.CurrentBand,
            RadioState.ActiveVfo.FrequencyHz);

        var entry = new FavoriteEntry
        {
            Name = name,
            Band = band,
            FrequencyHz = RadioState.ActiveVfo.FrequencyHz,
            Mode = ActiveMode ?? FormatModeDisplay(RadioState.ActiveVfo.Mode),
            LowCutIndex = LowCutIndex,
            HighCutIndex = HighCutIndex,
            CwFilterIndex = CwFilterIndex,
            Vfo = UseVfoBLastUsedFile ? "B" : "A"
        };
        ApplyFavoriteLabels(entry);

        // Replace only same name on the same band (FT-8 on 20m ≠ FT-8 on 15m)
        var existing = Favorites.FirstOrDefault(f =>
            string.Equals(NormalizeFavoriteBand(f.Band, f.FrequencyHz), band, StringComparison.OrdinalIgnoreCase) &&
            string.Equals(f.Name, name, StringComparison.OrdinalIgnoreCase));
        if (existing != null)
        {
            var answer = MessageBox.Show(
                $"A favorite named \"{name}\" already exists on {band}.\n\nUpdate it with the current operating conditions?",
                "Update Favorite?",
                MessageBoxButton.YesNo,
                MessageBoxImage.Question);
            if (answer != MessageBoxResult.Yes)
            {
                MonitorTextBoxText($" Favorite update cancelled: [{band}] {name}");
                return;
            }

            int idx = Favorites.IndexOf(existing);
            Favorites[idx] = entry;
            MonitorTextBoxText($" Favorite updated: [{band}] {name}");
        }
        else
        {
            Favorites.Add(entry);
            MonitorTextBoxText($" Favorite added: [{band}] {name} @ {entry.FrequencyHz}");
        }

        // Show the band we just saved into
        FavoriteBandFilter = band;
        PersistFavorites();
        RefreshFavoritesForBand();
        SelectedFavorite = entry;
    }

    /// <summary>
    /// Apply the selected favorite to the radio UI (freq/mode/band/filters/VFO). Client-only apply.
    /// </summary>
    [RelayCommand]
    private void RecallFavorite()
    {
        if (SelectedFavorite == null)
        {
            MessageBox.Show("Select a favorite to recall.", "Favorites",
                MessageBoxButton.OK, MessageBoxImage.Information);
            return;
        }

        var fav = SelectedFavorite;
        bool wantB = string.Equals(fav.Vfo, "B", StringComparison.OrdinalIgnoreCase);
        bool isB = UseVfoBLastUsedFile;

        // Switch VFO if needed (sends CMD_SET_VFO then freq/mode via ApplyActiveVfoToRadioAsync)
        if (wantB != isB)
            ToggleVfo();

        string band = NormalizeFavoriteBand(fav.Band, fav.FrequencyHz);

        bool prev = SuppressLastUsedSave;
        SuppressLastUsedSave = true;
        try
        {
            if (!string.IsNullOrWhiteSpace(band) && band != "?")
                RadioState.CurrentBand = band;

            TuneToFrequency(fav.FrequencyHz);
            ActiveMode = fav.Mode;
            if (fav.LowCutIndex >= 0) LowCutIndex = fav.LowCutIndex;
            if (fav.HighCutIndex >= 0) HighCutIndex = fav.HighCutIndex;
            if (fav.CwFilterIndex >= 0) CwFilterIndex = fav.CwFilterIndex;
        }
        finally
        {
            SuppressLastUsedSave = prev;
        }

        // Persist as last-used for this VFO/band after apply
        SaveLastUsedForCurrentBand();
        FavoriteNameInput = fav.Name;
        FavoriteBandFilter = band;
        MonitorTextBoxText(
            $" Favorite recalled: [{band}] {fav.Name} VFO{fav.Vfo} {fav.FrequencyHz} {fav.Mode}");
    }

    [RelayCommand]
    private void DeleteFavorite()
    {
        if (SelectedFavorite == null)
        {
            MessageBox.Show("Select a favorite to delete.", "Favorites",
                MessageBoxButton.OK, MessageBoxImage.Information);
            return;
        }

        var fav = SelectedFavorite;
        string band = NormalizeFavoriteBand(fav.Band, fav.FrequencyHz);
        var result = MessageBox.Show(
            $"Delete favorite \"{fav.Name}\" on {band}?",
            "Favorites",
            MessageBoxButton.YesNo,
            MessageBoxImage.Question);
        if (result != MessageBoxResult.Yes)
            return;

        Favorites.Remove(fav);
        SelectedFavorite = null;
        PersistFavorites();
        RefreshFavoritesForBand();
        MonitorTextBoxText($" Favorite deleted: [{band}] {fav.Name}");
    }

    public IRelayCommand ToggleVfoCommand { get; }
    /// <summary>Click VFO A box to select (no separate VFO toggle button).</summary>
    public IRelayCommand SelectVfoACommand { get; }
    /// <summary>Click VFO B box to select.</summary>
    public IRelayCommand SelectVfoBCommand { get; }

    public IRelayCommand<long> TuneToFrequencyCommand { get; }

    // Rx/Tx tab commands
    public IRelayCommand ToggleQrpCommand { get; }
    public IRelayCommand ToggleAutoTuneCommand { get; }
    public IRelayCommand ToggleAlcCommand { get; }
    public IRelayCommand ToggleNbCommand { get; }
    public IRelayCommand ToggleNrCommand { get; }
    public IRelayCommand ToggleAnCommand { get; }
    public IRelayCommand ToggleFullPowerCommand { get; }

    [RelayCommand] private void ToggleAudioDigital() => IsDigitalAudio = !IsDigitalAudio;

    [RelayCommand(CanExecute = nameof(CanToggleTune))]
    private void ToggleTune() => TuneMode = !TuneMode;

    private bool CanToggleTune() => !TxSetByServer;

    [RelayCommand(CanExecute = nameof(CanTogglePtt))]
    private void TogglePtt() => PttOn = !PttOn;

    private bool CanTogglePtt() => !TxSetByServer;

    [RelayCommand] private void CycleAgc() => AgcLevel = (AgcLevel + 1) % 3;

    [RelayCommand] private void ToggleAmp() => AmpOn = !AmpOn;

    [RelayCommand] private void ToggleCompression() => CompressionOn = !CompressionOn;

    [RelayCommand] private void ToggleMonitor() => MonitorOn = !MonitorOn;

    /// <summary>Cycle TX bandwidth presets (2.4 / 2.7 / 3.0 / 5.5 kHz) — same index as RX/TX list.</summary>
    [RelayCommand]
    private void CycleTxBandwidth()
    {
        if (TxOptions.Count == 0) return;
        TxBandwidthIndex = (TxBandwidthIndex + 1) % TxOptions.Count;
    }

    /// <summary>Cycle CW pitch presets (400/600/800/1000 Hz) — same index as CW tab list.</summary>
    [RelayCommand]
    private void CycleCwPitch()
    {
        if (CwPitchOptions.Count == 0) return;
        CwPitchIndex = (CwPitchIndex + 1) % CwPitchOptions.Count;
    }

    // ----- RX IQ tab: BAND/FREQ wired like original Set_IQ_RX_Band / Display_IQ_freq -----

    /// <summary>Amateur HF band meters from "40m" / "40M" / etc. Null if GEN or unknown.</summary>
    private static int? TryParseAmateurBandMeters(string? band)
    {
        if (string.IsNullOrWhiteSpace(band)) return null;
        string b = band.Trim().ToLowerInvariant();
        if (b is "gen" or "user" or "?") return null;
        if (b.EndsWith("m", StringComparison.Ordinal))
            b = b[..^1];
        if (!int.TryParse(b, out int meters)) return null;
        return meters is 160 or 80 or 60 or 40 or 30 or 20 or 17 or 15 or 12 or 10
            ? meters
            : null;
    }

    /// <summary>Original Display_IQ_freq: MHz.KKK.HHH</summary>
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
        if (RxIqUp24k) f += 24_000; // IQ_RX_Up_24K_freq
        f += RxIqFreqOffsetHz;
        return f;
    }

    private void RefreshRxIqFreqDisplay()
    {
        RxIqFreqDisplay = FormatIqFreqDisplay(ComputeRxIqTuneFreqHz());
    }

    private void ApplyRxIqTuneFrequency()
    {
        if (!RxIqSessionActive || _suppressRxIqFreqTune) return;
        long total = ComputeRxIqTuneFreqHz();
        if (total <= 0) return;
        RefreshRxIqFreqDisplay();
        RadioState.ActiveVfo.FrequencyHz = total;
        _ = _radioService.SetFrequencyAsync(total);
        MonitorTextBoxText($" RX IQ LO freq → {total} (base={_rxIqBaseFreqHz} +24k={RxIqUp24k} fine={RxIqFreqOffsetHz})");
    }

    partial void OnRxIqUp24kChanged(bool value)
    {
        if (!RxIqSessionActive || _suppressRxIqFreqTune) return;
        ApplyRxIqTuneFrequency();
        RxIqStatus = value
            ? "UP 24 kHz ON — LO shifted +24 000 Hz."
            : "UP 24 kHz OFF.";
    }

    partial void OnRxIqFreqOffsetHzChanged(int value)
    {
        int clamped = Math.Clamp(value, -1000, 2009);
        if (clamped != value)
        {
            _rxIqFreqOffsetHz = clamped;
            OnPropertyChanged(nameof(RxIqFreqOffsetHz));
        }
        if (!RxIqSessionActive || _suppressRxIqFreqTune) return;
        ApplyRxIqTuneFrequency();
    }

    partial void OnRxIqOffsetChanged(int value)
    {
        if (_suppressRxIqOffset) return;
        if (!RxIqSessionActive) return;
        int v = Math.Clamp(value, -200, 200);
        if (v != value)
        {
            _suppressRxIqOffset = true;
            try { RxIqOffset = v; }
            finally { _suppressRxIqOffset = false; }
            return;
        }
        // Original LefthScrollBar1_Scroll when IQ_RX_MODE_ACTIVE: CMD_SET_IQ_OFFSET 0x52
        _ = _radioService.SetIqOffsetAsync(v);
        MonitorTextBoxText($" RX IQ offset → {v} (0x52)");
    }

    /// <summary>Enter RX IQ tab (spectrum already in XAML; status hint only).</summary>
    public void EnterRxIqTab()
    {
        if (_rxIqTabActive) return;
        _rxIqTabActive = true;
        if (!RxIqSessionActive)
        {
            RxIqStatus =
                "Select an amateur band on MAIN, then START. BAND/FREQ use last-used for that band.";
        }
        MonitorTextBoxText(" RX IQ ENTER tab");
    }

    /// <summary>Leave RX IQ tab: end active session if any.</summary>
    public void LeaveRxIqTab()
    {
        if (!_rxIqTabActive) return;
        _rxIqTabActive = false;
        LeaveRxIqSession("leave tab");
        MonitorTextBoxText(" RX IQ LEAVE tab");
    }

    /// <summary>Leave RX IQ session (tab leave or START off). Clears active flag; LO left as last tuned.</summary>
    public void LeaveRxIqSession(string reason = "leave")
    {
        if (!RxIqSessionActive && !RxIqCommitting)
        {
            MonitorTextBoxText($" RX IQ LEAVE ({reason}): already idle");
            return;
        }
        RxIqCommitting = false;
        RxIqSessionActive = false;
        _suppressRxIqOffset = true;
        _suppressRxIqFreqTune = true;
        try
        {
            RxIqOffset = 0;
            // Keep band/freq display labels for context; zero I/Q offset UI like original leave.
        }
        finally
        {
            _suppressRxIqOffset = false;
            _suppressRxIqFreqTune = false;
        }
        RxIqStatus = $"Session ended ({reason}). Press START to re-enter.";
        MonitorTextBoxText($" RX IQ LEAVE ({reason})");
    }

    [RelayCommand]
    private void StartRxIq()
    {
        // Toggle off
        if (RxIqSessionActive)
        {
            LeaveRxIqSession("START off");
            return;
        }

        // Original: band from current main band; GEN invalid
        string bandKey = NormalizeFavoriteBand(RadioState.CurrentBand, RadioState.ActiveVfo.FrequencyHz);
        int? meters = TryParseAmateurBandMeters(bandKey);
        if (meters is null)
        {
            MessageBox.Show(
                "INVALID BAND (GENERAL).\r\nRETURN TO THE MAIN TAB AND SELECT AN AMATEUR BAND.",
                "MSCC",
                MessageBoxButton.OK,
                MessageBoxImage.Asterisk);
            MonitorTextBoxText($" RX IQ START blocked: band={bandKey}");
            return;
        }

        // Original Set_IQ_RX_Band: last-used freq for that band (active VFO's last-used file)
        var (lastFreq, _, _, _, _) = SpectrumWaterfallSettings.LoadLastUsedForBand(bandKey, UseVfoBLastUsedFile);
        long baseFreq = lastFreq;
        if (baseFreq <= 0)
        {
            string activeBand = NormalizeFavoriteBand(null, RadioState.ActiveVfo.FrequencyHz);
            if (string.Equals(activeBand, bandKey, StringComparison.OrdinalIgnoreCase))
                baseFreq = RadioState.ActiveVfo.FrequencyHz;
            else
                baseFreq = GetAmpCalFrequencyHz(meters.Value);
        }
        if (baseFreq <= 0)
        {
            MessageBox.Show("No frequency available for this band.", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Warning);
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

        // Original order after band resolve: 0x58 band, then 0x55 RX_IQBD (enter path)
        _ = _radioService.SetIqBandAsync(meters.Value);
        _ = _radioService.SetIqCalibrationRxTxAsync(txIq: false); // RX_IQBD
        _ = _radioService.SetFrequencyAsync(_rxIqBaseFreqHz);
        RadioState.ActiveVfo.FrequencyHz = _rxIqBaseFreqHz;

        RxIqSessionActive = true;
        RxIqStatus = $"ACTIVE — {RxIqBandLabel} {RxIqFreqDisplay}. Adjust I/Q OFFSET (spectrum), then APPLY.";
        MonitorTextBoxText(
            $" RX IQ START: band={meters.Value}m base={_rxIqBaseFreqHz} → 0x58 + 0x55 RX + SET_MAIN_FREQ");
    }

    [RelayCommand]
    private void ZeroRxIqOffset()
    {
        if (!RxIqSessionActive)
        {
            MessageBox.Show("Start an RX I/Q session first.", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }
        // Original LeftResetbutton2: offset 0, 0x52, then COMMIT 0x57
        _suppressRxIqOffset = true;
        try { RxIqOffset = 0; }
        finally { _suppressRxIqOffset = false; }
        _ = _radioService.SetIqOffsetAsync(0);
        _ = _radioService.CommitIqAsync();
        RxIqCommitting = true;
        RxIqStatus = "Offset ZERO + COMMIT sent (0x52, 0x57)…";
        MonitorTextBoxText(" RX IQ ZERO offset + COMMIT (0x52, 0x57)");
    }

    [RelayCommand]
    private void ResetRxIqFreq()
    {
        // Original Reset_Freq_button3: fine offset 0, retune base [+24k]
        _suppressRxIqFreqTune = true;
        try
        {
            RxIqFreqOffsetHz = 0;
        }
        finally
        {
            _suppressRxIqFreqTune = false;
        }
        if (RxIqSessionActive)
        {
            ApplyRxIqTuneFrequency();
            RxIqStatus = $"LO fine cleared — freq {RxIqFreqDisplay}.";
        }
        else
        {
            RefreshRxIqFreqDisplay();
            RxIqStatus = "LO fine offset cleared.";
        }
        MonitorTextBoxText(" RX IQ RESET FREQ");
    }

    [RelayCommand]
    private void ApplyRxIq()
    {
        if (!RxIqSessionActive)
        {
            MessageBox.Show("Start an RX I/Q session first.", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }
        var ret = MessageBox.Show(
            "APPLY THE CURRENT I/Q VALUE?",
            "MSCC",
            MessageBoxButton.YesNo,
            MessageBoxImage.Question,
            MessageBoxResult.No);
        if (ret != MessageBoxResult.Yes)
        {
            MonitorTextBoxText(" RX IQ APPLY cancelled");
            return;
        }
        // Original IQ_Commit when RX active: CMD_SET_COMMIT_IQ 0x57, wait 0x56
        RxIqCommitting = true;
        RxIqStatus = "APPLYING… (0x57)";
        _ = _radioService.CommitIqAsync();
        MonitorTextBoxText($" RX IQ APPLY (0x57) band={_rxIqBandMeters} offset={RxIqOffset}");
    }

    [RelayCommand]
    private void ResetAllRxIq()
    {
        if (!RxIqSessionActive)
        {
            MessageBox.Show("Start an RX I/Q session first.", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            return;
        }
        // Original IQ_Reset_All_button2_Click
        var ret = MessageBox.Show(
            "THIS APPLIES I/Q SLIDER BAR VALUE TO ALL BANDS.\r\nARE YOU SURE YOU WANT TO CONTINUE?",
            "MSCC",
            MessageBoxButton.YesNo,
            MessageBoxImage.Warning,
            MessageBoxResult.No);
        if (ret != MessageBoxResult.Yes)
        {
            MessageBox.Show("IQ VALUES NOT SET.", "MSCC",
                MessageBoxButton.OK, MessageBoxImage.Asterisk);
            MonitorTextBoxText(" RX IQ RESET ALL cancelled");
            return;
        }
        // Ensure current offset is on radio, then 0x55 RX + 0x8D
        _ = _radioService.SetIqOffsetAsync(RxIqOffset);
        _ = _radioService.ResetAllIqBandsAsync(rxIq: true);
        RxIqStatus = "RESET ALL sent — all bands set to current I/Q offset (0x8D).";
        MonitorTextBoxText($" RX IQ RESET ALL (0x55 RX + 0x8D) offset={RxIqOffset}");
        MessageBox.Show("ALL BANDS HAVE BEEN SET TO I/Q SLIDER BAR VALUE.", "MSCC",
            MessageBoxButton.OK, MessageBoxImage.Asterisk);
    }

    [RelayCommand]
    private void ShowSpecWaterfall()
    {
        // Single instance only — if already open, bring it to front
        if (_specWaterfallWindow != null)
        {
            if (_specWaterfallWindow.WindowState == WindowState.Minimized)
                _specWaterfallWindow.WindowState = WindowState.Normal;
            _specWaterfallWindow.Activate();
            _specWaterfallWindow.Focus();
            SpecWfVisible = true;
            MonitorTextBoxText(" S/W button pressed - Spectrum/Waterfall controls already open (activated)");
            return;
        }

        var window = new PanadapterControlsWindow();
        window.Owner = Application.Current.MainWindow;
        window.Closed += (_, _) =>
        {
            if (ReferenceEquals(_specWaterfallWindow, window))
                _specWaterfallWindow = null;
            SpecWfVisible = false;
            MonitorTextBoxText(" S/W Spectrum/Waterfall controls window closed");
        };
        _specWaterfallWindow = window;
        window.Show();
        SpecWfVisible = true;
        MonitorTextBoxText(" S/W button pressed - opened Spectrum/Waterfall controls window");
    }

    /// <summary>Open or activate the debug log popup (RESET LOGS + Pause + list).</summary>
    [RelayCommand]
    private void ShowDebugLog()
    {
        if (_debugLogWindow != null)
        {
            if (_debugLogWindow.WindowState == WindowState.Minimized)
                _debugLogWindow.WindowState = WindowState.Normal;
            _debugLogWindow.Activate();
            _debugLogWindow.Focus();
            DebugLogVisible = true;
            MonitorTextBoxText(" LOG button pressed - debug log already open (activated)");
            return;
        }

        var window = new DebugLogWindow
        {
            Owner = Application.Current.MainWindow,
            DataContext = this
        };
        window.Closed += (_, _) =>
        {
            if (ReferenceEquals(_debugLogWindow, window))
                _debugLogWindow = null;
            DebugLogVisible = false;
            MonitorTextBoxText(" Debug log window closed");
        };
        _debugLogWindow = window;
        window.Show();
        DebugLogVisible = true;
        MonitorTextBoxText(" LOG button pressed - opened debug log window");
    }

    // Cycle commands for main tab filter/step buttons (decrement to match original)
    [RelayCommand] private void CycleLowCut() => LowCutIndex = (LowCutIndex - 1 + 5) % 5;
    [RelayCommand] private void CycleHighCut() => HighCutIndex = (HighCutIndex - 1 + 5) % 5;
    [RelayCommand] private void CycleCwFilter() => CwFilterIndex = (CwFilterIndex - 1 + 3) % 3;
    [RelayCommand] private void CycleStep() => StepIndex = (StepIndex - 1 + 6) % 6;

    // CW tab inc/dec for speed and hold (numeric like original)
    [RelayCommand] private void IncCwSpeed() => CwSpeed = Math.Clamp(CwSpeed + 1, 5, 60);
    [RelayCommand] private void DecCwSpeed() => CwSpeed = Math.Clamp(CwSpeed - 1, 5, 60);

    /// <summary>Farnsworth text WPM: Off→5→…→60.</summary>
    [RelayCommand]
    private void IncCwMemTextWpm()
    {
        if (CwMemTextWpm <= 0)
            CwMemTextWpm = 5;
        else
            CwMemTextWpm = Math.Clamp(CwMemTextWpm + 1, 5, 60);
    }

    /// <summary>Farnsworth text WPM: …→5→Off.</summary>
    [RelayCommand]
    private void DecCwMemTextWpm()
    {
        if (CwMemTextWpm <= 0)
            CwMemTextWpm = 0;
        else if (CwMemTextWpm <= 5)
            CwMemTextWpm = 0;
        else
            CwMemTextWpm = Math.Clamp(CwMemTextWpm - 1, 5, 60);
    }
    [RelayCommand] private void IncCwHold() => CwHold = Math.Clamp(CwHold + 10, 1, 500);
    [RelayCommand] private void DecCwHold() => CwHold = Math.Clamp(CwHold - 10, 1, 500);

    // Mode push button (legacy cycle): LSB → USB → DIG-U → CW → AM → LSB…
    // Setting ActiveMode triggers profile swap, VFO update, filter recompute, service send, logging.
    [RelayCommand]
    private void CycleMode()
    {
        var modes = new[] { "LSB", "USB", "DIG-U", "CW", "AM" };
        string current = ActiveMode ?? "LSB";
        int idx = Array.FindIndex(modes, m => string.Equals(m, current, StringComparison.OrdinalIgnoreCase));
        if (idx < 0) idx = 0;
        else idx = (idx + 1) % modes.Length;
        ActiveMode = modes[idx];
    }

    /// <summary>
    /// Direct mode select from operate-panel buttons (LSB/USB/DIG-U/CW/AM). FM is UI-disabled for now.
    /// </summary>
    [RelayCommand]
    private void SelectMode(string? mode)
    {
        if (string.IsNullOrWhiteSpace(mode))
            return;
        if (string.Equals(mode, "FM", StringComparison.OrdinalIgnoreCase))
        {
            MonitorTextBoxText(" FM mode not available yet");
            return;
        }
        ActiveMode = mode.Trim();
    }

    [RelayCommand]
    private void ResetDebugLogs()
    {
        DebugLog.Clear();
        DebugLogText = string.Empty;
        DebugMonitor.ResetLogFile();
        LastServerMessage = "Debug logs reset.";
    }

    /// <summary>
    /// Tune active VFO. When <paramref name="fromSpectrumClick"/> and AUTO SNAP is on (and not CW),
    /// snap frequency like original Set_Spectrum_Frequency (1 kHz / 500 Hz / 100 Hz).
    /// </summary>
    public void TuneToFrequency(long frequencyHz, bool fromSpectrumClick = false)
    {
        long freq = frequencyHz;
        if (fromSpectrumClick &&
            SpectrumWaterfallSettings.SpectrumAutoSnap &&
            RadioState.ActiveVfo.Mode != RadioMode.CW)
        {
            long snapped = SpectrumWaterfallSettings.ApplyAutoSnap(freq);
            if (snapped != freq)
            {
                MonitorTextBoxText(
                    $" AUTO SNAP: {freq} → {snapped} (step={SpectrumWaterfallSettings.GetAutoSnapStepHz()} Hz)");
                freq = snapped;
            }
        }

        RadioState.ActiveVfo.FrequencyHz = freq;
        MonitorTextBoxText($" TuneToFrequency: {freq}");
        _ = _radioService.SetFrequencyAsync(freq);
        SaveLastUsedForCurrentBand();
    }

    /// <summary>
    /// Changes frequency for a specific VFO (A or B). Sends to service only if it's the active VFO.
    /// Supports mouse wheel tuning on the VFO boxes.
    /// </summary>
    public void SetVfoFrequency(VfoState vfo, long frequencyHz)
    {
        vfo.FrequencyHz = frequencyHz;
        MonitorTextBoxText($" SetVfoFrequency: vfo={(vfo == RadioState.VfoA ? "A" : "B")} freq={frequencyHz} (active={(vfo == RadioState.ActiveVfo)})");
        if (vfo == RadioState.ActiveVfo)
        {
            _ = _radioService.SetFrequencyAsync(frequencyHz);
            SaveLastUsedForCurrentBand();
        }
    }

    /// <summary>
    /// Returns the frequency increment in Hz for the current Step setting.
    /// Used for global mouse wheel tuning (when not hovering a control) on the Main tab only.
    /// </summary>
    public long GetCurrentStepHz()
    {
        int i = Math.Clamp(StepIndex, 0, 5);
        return StepHzValues[i];
    }

    /// <summary>
    /// Entry point for debug logging. Matches the original MonitorTextBoxText procedure:
    /// prefixes with incrementing line count, writes to persistent log file (always),
    /// and (if not suspended) appends to the in-memory DebugLog for UI display.
    /// Safe to call from any thread.
    /// </summary>
    public void MonitorTextBoxText(string text)
    {
        DebugMonitor.MonitorTextBoxText(text);
    }

    private void OnDebugLogMessage(string message)
    {
        // UI thread affinity for collection updates
        if (System.Windows.Application.Current?.Dispatcher is { } dispatcher)
        {
            dispatcher.BeginInvoke(() =>
            {
                if (!MonitorSuspend)
                {
                    DebugLog.Add(message);
                    // Trim to keep memory reasonable (original keeps in textbox until restart)
                    while (DebugLog.Count > 1000)
                        DebugLog.RemoveAt(0);

                    DebugLogText = string.Join(Environment.NewLine, DebugLog);
                }
            });
        }
        else
        {
            // Fallback (e.g. during shutdown)
            if (!MonitorSuspend)
            {
                DebugLog.Add(message);
                DebugLogText = string.Join(Environment.NewLine, DebugLog);
            }
        }
    }

    private void ToggleVfo() =>
        SelectVfo(useVfoB: RadioState.ActiveVfo == RadioState.VfoA);

    /// <summary>
    /// Activate VFO A or B (same path as the old VFO toggle). No-op if already active.
    /// Used by click-on-VFO-box and any remaining ToggleVfo callers.
    /// </summary>
    private void SelectVfo(bool useVfoB)
    {
        bool already = useVfoB
            ? RadioState.ActiveVfo == RadioState.VfoB
            : RadioState.ActiveVfo == RadioState.VfoA;
        if (already)
            return;

        // Remember the band for the VFO we are leaving (band buttons share one CurrentBand).
        StoreBandForActiveVfo(RadioState.CurrentBand);

        string leaving = UseVfoBLastUsedFile ? "B" : "A";
        RadioState.ToggleActiveVfo();
        // ActiveVfo PropertyChanged → ApplyActiveVfoToRadioAsync:
        //   1) CMD_SET_VFO 0xF2 (0=A / 1=B)  2) frequency  3) mode

        // Restore this VFO's last band so the band bar highlight matches (e.g. A@20m, B@10m).
        string restore = UseVfoBLastUsedFile ? _bandForVfoB : _bandForVfoA;
        if (string.IsNullOrWhiteSpace(restore) || restore == "?")
            restore = GetBandNameForFrequency(RadioState.ActiveVfo.FrequencyHz);

        if (!string.IsNullOrWhiteSpace(restore) && restore != "?")
        {
            if (!string.Equals(RadioState.CurrentBand, restore, StringComparison.OrdinalIgnoreCase))
                RadioState.CurrentBand = restore;
            else
                StoreBandForActiveVfo(restore);
        }

        OnPropertyChanged(nameof(ActiveMode));
        NotifyMainOperatePower();
        MonitorTextBoxText(
            $" SelectVFO {leaving}->{(UseVfoBLastUsedFile ? "B" : "A")} band={RadioState.CurrentBand} f={RadioState.ActiveVfo.FrequencyHz}");
    }

    /// <summary>
    /// After ActiveVfo changes: tell ms-sdr which VFO is active (CMD_SET_VFO), then push freq/mode.
    /// Order is mandatory — VFO select must precede any frequency or other VFO-specific data.
    /// </summary>
    private async Task ApplyActiveVfoToRadioAsync()
    {
        try
        {
            byte vfo = RadioState.ActiveVfo == RadioState.VfoB ? Opcodes.VFO_B : Opcodes.VFO_A;
            await _radioService.SetActiveVfoAsync(vfo).ConfigureAwait(false);
            // Brief pause matches original Thread.Sleep(10) after CMD_SET_VFO before band/freq traffic.
            await Task.Delay(10).ConfigureAwait(false);
            await _radioService.SetFrequencyAsync(RadioState.ActiveVfo.FrequencyHz).ConfigureAwait(false);
            await _radioService.SetModeAsync(FormatModeDisplay(RadioState.ActiveVfo.Mode)).ConfigureAwait(false);
        }
        catch (Exception ex)
        {
            MonitorTextBoxText($" ApplyActiveVfoToRadioAsync error: {ex.Message}");
        }
    }

    private static RadioMode ParseMode(string mode)
    {
        // Accept canonical names, single char, or numeric strings (defensive for wire reports)
        string u = (mode ?? "").Trim().ToUpperInvariant().Replace('_', '-');
        return u switch
        {
            "USB" or "U" or "2" => RadioMode.USB,
            "LSB" or "L" or "1" => RadioMode.LSB,
            "AM" or "A" or "0" => RadioMode.AM,
            "CW" or "C" or "3" => RadioMode.CW,
            "TUNE" or "T" or "4" => RadioMode.TUNE,
            "DIG-U" or "DIGU" or "DIG" => RadioMode.DigU,
            _ => RadioMode.USB
        };
    }

    /// <summary>UI / last-used / favorites display name (DIG-U, not DigU).</summary>
    public static string FormatModeDisplay(RadioMode mode) => mode switch
    {
        RadioMode.DigU => "DIG-U",
        _ => mode.ToString().ToUpperInvariant()
    };

    private void SaveModeFilterProfile(RadioMode mode)
    {
        if (mode is RadioMode.TUNE) return;
        SpectrumWaterfallSettings.SaveModeFilterProfile(
            FormatModeDisplay(mode), LowCutIndex, HighCutIndex, CwFilterIndex);
        SpectrumWaterfallSettings.Save();
    }

    private void ApplyModeFilterProfile(RadioMode mode)
    {
        if (mode is RadioMode.TUNE) return;
        var (l, h, c) = SpectrumWaterfallSettings.LoadModeFilterProfile(FormatModeDisplay(mode));
        if (l < 0 && h < 0) return;

        bool prev = SuppressLastUsedSave;
        SuppressLastUsedSave = true;
        try
        {
            // Force UI + Hz even when an index matches the previous mode (ObservableProperty
            // would skip On*Changed and leave wrong Lo/Hi labels or RF edges).
            if (l >= 0)
            {
                if (LowCutIndex == l)
                {
                    LowCutLabel = LowCutOptions[(l % 5 + 5) % 5];
                }
                else
                    LowCutIndex = l;
            }
            if (h >= 0)
            {
                if (HighCutIndex == h)
                    HighCutLabel = HighCutOptions[(h % 5 + 5) % 5];
                else
                    HighCutIndex = h;
            }
            if (mode == RadioMode.CW && c >= 0)
            {
                if (CwFilterIndex == c)
                    CwFilterLabel = CwFilterOptions[(c % 3 + 3) % 3];
                else
                    CwFilterIndex = c;
            }

            RecomputeFilterHzForCurrentCutsAndMode();
            // Push cuts to radio (Recompute unhooks filter PropertyChanged)
            var f = RadioState.ActiveVfo.Filter;
            if (f != null)
            {
                _ = _radioService.SetFilterLowAsync(f.LowHz);
                _ = _radioService.SetFilterHighAsync(f.HighHz);
            }
            if (mode == RadioMode.CW && c >= 0)
                _ = _radioService.SetCwFilterAsync(c);
            UpdateCurrentSpectrumFilters();
        }
        finally
        {
            SuppressLastUsedSave = prev;
        }
        MonitorTextBoxText(
            $" Mode profile applied: {FormatModeDisplay(mode)} Lo={LowCutIndex}({LowCutLabel}) Hi={HighCutIndex}({HighCutLabel})" +
            (mode == RadioMode.CW ? $" CW={CwFilterIndex}({CwFilterLabel})" : ""));
    }

    /// <summary>
    /// DIG-U: force Audio D (and CMP off via digital path). Leaving DIG-U restores prior P/D.
    /// </summary>
    private void ApplyDigUAudioPolicy(RadioMode oldMode, RadioMode newMode)
    {
        if (newMode == RadioMode.DigU && oldMode != RadioMode.DigU)
        {
            _audioBeforeDigU = IsDigitalAudio;
            if (!IsDigitalAudio)
            {
                IsDigitalAudio = true;
                MonitorTextBoxText(" DIG-U: Audio → D (digital); CMP forced off if it was on");
            }
        }
        else if (oldMode == RadioMode.DigU && newMode != RadioMode.DigU)
        {
            if (_audioBeforeDigU is bool prev && IsDigitalAudio != prev)
            {
                IsDigitalAudio = prev;
                MonitorTextBoxText($" Left DIG-U: Audio restored to {(prev ? "D" : "P")}");
            }
            _audioBeforeDigU = null;
        }
    }

    /// <summary>
    /// MSCC.Wpf assembly version from build stamp (InformationalVersion).
    /// Format: month.day.iteration (e.g. 7.16.0) — iteration resets when calendar month/day changes.
    /// </summary>
    private static string GetClientVersionString()
    {
        try
        {
            var asm = Assembly.GetExecutingAssembly();
            var info = asm.GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion;
            if (!string.IsNullOrWhiteSpace(info))
            {
                // Strip any +git suffix if present
                int plus = info.IndexOf('+');
                return plus > 0 ? info[..plus] : info.Trim();
            }
            var v = asm.GetName().Version;
            if (v != null)
                return $"{v.Major}.{v.Minor}.{v.Build}"; // month.day.iteration
        }
        catch { /* fall through */ }
        return "0.0.0";
    }

    /// <summary>
    /// Applies a "reasonable" filter bandwidth for the given mode to the active VFO's filter (for immediate spectrum bandpass display).
    /// Unhooks the filter listener temporarily to avoid sending filter commands to the backend (mode change doesn't change the actual cut presets).
    /// This makes the shaded bandpass on spectrum follow the mode (AM wide, CW narrow, SSB default).
    /// Called from ActiveMode setter (mode push button / band buttons) and from Vfo mode changes (reports).
    /// </summary>
    private void RecomputeFilterHzForCurrentCutsAndMode()
    {
        var filter = RadioState.ActiveVfo.Filter;
        if (filter == null) return;

        var mode = RadioState.ActiveVfo.Mode;

        // Unhook to avoid sending filter changes to backend (we are only remapping for display based on current indices + mode)
        filter.PropertyChanged -= OnActiveFilterPropertyChanged;

        if (mode == RadioMode.AM)
        {
            // Original AM markers: both sidebands, center ± high-cut (Filter_size from band_marker_high).
            // Low cut does not set the RF edges — DSB is always symmetric about the carrier.
            int highHz = HighCutHzValues[(HighCutIndex % HighCutHzValues.Length + HighCutHzValues.Length) % HighCutHzValues.Length];
            filter.LowHz = -highHz;
            filter.HighHz = +highHz;
        }
        else if (mode == RadioMode.CW)
        {
            // CW bandwidth is controlled by the CW filter button (not pitch, which is deferred)
            int cwIdx = CwFilterIndex;
            int cwHz = CwFilterHzValues[cwIdx];
            filter.LowHz = -cwHz / 2;
            filter.HighHz = +cwHz / 2;
            // CwPitchHz left for later implementation of CW pitch
        }
        else if (mode == RadioMode.TUNE)
        {
            // TUN mode: narrow carrier for antenna tuning
            filter.LowHz = -100;
            filter.HighHz = 100;
        }
        else
        {
            // USB / DIG-U or LSB: re-map cut indices with correct sideband signs.
            // DIG-U uses USB LO and the same Lo/Hi mapping as USB.
            int lowIdx = LowCutIndex;
            int lowHz = LowCutHzValues[lowIdx];
            if (mode == RadioMode.LSB)
            {
                filter.HighHz = -lowHz;   // low audio cut → inner edge for LSB
            }
            else // USB, DigU
            {
                filter.LowHz = +lowHz;    // low audio cut → inner edge for USB
            }

            int highIdx = HighCutIndex;
            int highHz = HighCutHzValues[highIdx];
            if (mode == RadioMode.LSB)
            {
                filter.LowHz = -highHz;   // high audio cut → outer edge for LSB
            }
            else // USB, DigU
            {
                filter.HighHz = +highHz;  // high audio cut → outer edge for USB
            }
        }

        filter.PropertyChanged += OnActiveFilterPropertyChanged;

        UpdateCurrentSpectrumFilters();
    }

    private void OnActiveFilterPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        var f = RadioState.ActiveVfo.Filter;
        if (e.PropertyName == nameof(FilterSettings.LowHz))
        {
            _ = _radioService.SetFilterLowAsync(f.LowHz);
            MonitorTextBoxText($" Filter low set: {f.LowHz}");
        }
        else if (e.PropertyName == nameof(FilterSettings.HighHz))
        {
            _ = _radioService.SetFilterHighAsync(f.HighHz);
            MonitorTextBoxText($" Filter high set: {f.HighHz}");
        }
        else if (e.PropertyName == nameof(FilterSettings.CwPitchHz))
        {
            _ = _radioService.SetCwPitchAsync(f.CwPitchHz);
        }

        UpdateCurrentSpectrumFilters();
    }

    private void UpdateCurrentSpectrumFilters()
    {
        if (CurrentSpectrum != null)
        {
            CurrentSpectrum = EnrichSpectrumUpdate(CurrentSpectrum);
        }
    }

    private void OnActiveVfoPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(VfoState.Mode))
        {
            // Mode changed on the VFO (e.g. via report from backend, dropdown, or band button).
            // Recompute the filter Hz offsets from the *current* cut indices using the correct signs for the new mode.
            // This preserves the user's selected Lo/Hi cut values in the UI boxes while updating the spectrum shading correctly.
            RecomputeFilterHzForCurrentCutsAndMode();
            NotifyMainOperatePower();
        }
        else if (e.PropertyName == nameof(VfoState.RitOn) || e.PropertyName == nameof(VfoState.RitOffsetHz))
        {
            var v = RadioState.ActiveVfo!;
            _ = _radioService.SetRitAsync(v.RitOn, v.RitOffsetHz);
        }
        else if (e.PropertyName == nameof(VfoState.FrequencyHz))
        {
            UpdateCurrentSpectrumFilters();
        }
    }

    // Rx/Tx tab property change handlers - send to service
    partial void OnTunePowerPercentChanged(int value)
    {
        _ = _radioService.SetTunePowerAsync(value);
        // Persist into the store for the current AMP state (dual Tune Power).
        SpectrumWaterfallSettings.SetTunePowerForAmp(AmpOn, value);
        SpectrumWaterfallSettings.Save();
        MonitorTextBoxText($" Tune power % set: {value} (AmpOn={AmpOn})");
        NotifyMainOperatePower();
    }
    partial void OnCwPowerPercentChanged(int value)
    {
        _ = _radioService.SetCwPowerAsync(value);
        SpectrumWaterfallSettings.CwPower = value;
        SpectrumWaterfallSettings.Save();
        MonitorTextBoxText($" CW power % set: {value}");
        NotifyMainOperatePower();
    }
    partial void OnSsbPowerPercentChanged(int value)
    {
        _ = _radioService.SetSsbPowerAsync(value);
        SpectrumWaterfallSettings.SsbPower = value;
        SpectrumWaterfallSettings.Save();
        MonitorTextBoxText($" SSB power % set: {value}");
        NotifyMainOperatePower();
    }
    partial void OnAmCarrierPercentChanged(int value)
    {
        _ = _radioService.SetAmCarrierAsync(value);
        SpectrumWaterfallSettings.AmCarrier = value;
        SpectrumWaterfallSettings.Save();
        MonitorTextBoxText($" AM carrier % set: {value}");
        NotifyMainOperatePower();
    }
    partial void OnFullPowerChanged(bool value) { _ = _radioService.SetFullPowerAsync(value); MonitorTextBoxText($" FullPower set: {value}"); }
    partial void OnAlcOnChanged(bool value) { _ = _radioService.SetAlcOnAsync(value); MonitorTextBoxText($" AlcOn set: {value}"); }
    partial void OnAutoTuneChanged(bool value)
    {
        _ = _radioService.SetAutoTuneAsync(value);
        MonitorTextBoxText($" AutoTune set: {value}");
        UpdateShowExternalSwrFace();
    }
    partial void OnQrpModeChanged(bool value) { _ = _radioService.SetQrpModeAsync(value); MonitorTextBoxText($" QrpMode set: {value}"); }
    partial void OnTxBandwidthIndexChanged(int value)
    {
        // No caution dialog: one-button cycle must pass through wide TX BW options freely.
        _ = _radioService.SetTxBandwidthAsync(value);
        OnPropertyChanged(nameof(TxBwButtonText));
        MonitorTextBoxText($" TxBandwidthIndex set: {value} ({TxBwButtonText})");
    }

    /// <summary>MAIN-tab TX BW cycle button label (matches TxOptions list on RX/TX tab).</summary>
    public string TxBwButtonText =>
        TxBandwidthIndex >= 0 && TxBandwidthIndex < TxOptions.Count
            ? TxOptions[TxBandwidthIndex]
            : "TX BW";

    /// <summary>MAIN-tab CW pitch cycle button label (matches CwPitchOptions on CW tab).</summary>
    public string CwPitchButtonText =>
        CwPitchIndex >= 0 && CwPitchIndex < CwPitchOptions.Count
            ? CwPitchOptions[CwPitchIndex]
            : "PITCH";
    partial void OnNbOnChanged(bool value)
    {
        if (!_suppressNbCommand)
            _ = _radioService.SetNbOnAsync(value);
        MonitorTextBoxText($" NbOn set: {value}{(_suppressNbCommand ? " (from server)" : "")}");
    }

    partial void OnNbPulseChanged(int value)
    {
        value = Math.Clamp(value, 10, 510);
        if (!_suppressNbCommand)
            _ = _radioService.SetNbPulseWidthAsync(value);
        MonitorTextBoxText($" NbPulse set: {value} uS{(_suppressNbCommand ? " (from server)" : "")}");
    }

    partial void OnNbThresholdChanged(int value)
    {
        value = Math.Clamp(value, 1, 1009);
        if (!_suppressNbCommand)
            _ = _radioService.SetNbThresholdAsync(value);
        MonitorTextBoxText($" NbThreshold set: {value}{(_suppressNbCommand ? " (from server)" : "")}");
    }
    partial void OnNrOnChanged(bool value)
    {
        // CMD_SET_NR (0xA3): ON → current slider value; OFF → 0
        // Skip send when applying appliance report (same pattern as NB).
        if (!_suppressNrCommand)
            _ = _radioService.SetNrOnAsync(value, NrLevel);
        MonitorTextBoxText(
            $" NrOn set: {value}{(_suppressNrCommand ? " (from server)" : $" → CMD_SET_NR {(value ? NrLevel : 0)}")}");
    }

    partial void OnNrLevelChanged(int value)
    {
        value = Math.Clamp(value, 0, 100);
        // While NR is on, live-update level via same 0xA3 opcode
        if (!_suppressNrCommand && NrOn)
            _ = _radioService.SetNrLevelAsync(value);
        MonitorTextBoxText(
            $" NrLevel set: {value}{(_suppressNrCommand ? " (from server)" : (NrOn ? " (sent 0xA3)" : " (NR off, not sent)"))}");
    }

    /// <summary>
    /// AN toggle — same select semantics as NB/NR:
    /// selected (AnOn=true, darker gold)  → 0x8E payload 1
    /// unselected (AnOn=false, bright gold) → 0x8E payload 0
    /// </summary>
    private void ToggleAn()
    {
        // Flip via property so UI updates; OnAnOnChanged sends when not suppressed.
        AnOn = !AnOn;
    }

    partial void OnAnOnChanged(bool value)
    {
        // User toggle: send 0x8E. Server report: suppress so we do not echo.
        if (!_suppressAnCommand)
            _ = _radioService.SetAutoNotchOnAsync(value);
        MonitorTextBoxText(
            value
                ? $" AN selected/ON{(_suppressAnCommand ? " (from server)" : " → 0x8E payload=1")}"
                : $" AN unselected/OFF{(_suppressAnCommand ? " (from server)" : " → 0x8E payload=0")}");
    }

    /// <summary>
    /// Apply appliance NR_VALUE (0xA3): 0 = off; non-zero = on with that level.
    /// Does not re-send to the server.
    /// </summary>
    private void ApplyNrValueFromServer(int nrValue)
    {
        nrValue = Math.Clamp(nrValue, 0, 100);
        _suppressNrCommand = true;
        try
        {
            if (nrValue == 0)
            {
                NrOn = false;
                // Keep slider at last non-zero preference when server reports off.
            }
            else
            {
                NrLevel = nrValue;
                NrOn = true;
            }
        }
        finally
        {
            _suppressNrCommand = false;
        }
        MonitorTextBoxText(
            nrValue == 0
                ? " NR from server: OFF (NR_VALUE=0)"
                : $" NR from server: ON level={nrValue}");
    }
    partial void OnAgcLevelChanged(int value)
    {
        _ = _radioService.SetAgcLevelAsync(value);
        AgcButtonText = value switch
        {
            0 => "SLO",
            1 => "MED",
            2 => "FST",
            _ => "SLO"
        };
        MonitorTextBoxText($" AgcLevel set: {value} ({AgcButtonText})");
    }
    partial void OnAgcFastReleaseChanged(int value) { _ = _radioService.SetAgcFastReleaseAsync(value); MonitorTextBoxText($" AgcFastRelease set: {value}"); }

    partial void OnLowCutDefaultIndexChanged(int value) { _ = _radioService.SetDefaultLowCutAsync(value); MonitorTextBoxText($" LowCutDefaultIndex set: {value}"); }
    partial void OnTxDefaultIndexChanged(int value) { _ = _radioService.SetDefaultTxAsync(value); MonitorTextBoxText($" TxDefaultIndex set: {value}"); }
    partial void OnHighCutDefaultIndexChanged(int value) { _ = _radioService.SetDefaultHighCutAsync(value); MonitorTextBoxText($" HighCutDefaultIndex set: {value}"); }
    partial void OnCwFilterDefaultIndexChanged(int value) { _ = _radioService.SetDefaultCwFilterAsync(value); MonitorTextBoxText($" CwFilterDefaultIndex set: {value}"); }

    // Main tab button partials (decrement like original, wrap, set label + Hz + send index)
    partial void OnLowCutIndexChanged(int value)
    {
        value = (value % 5 + 5) % 5;
        LowCutLabel = LowCutOptions[value];
        int hz = LowCutHzValues[value];
        var mode = RadioState.ActiveVfo.Mode;
        if (mode == RadioMode.LSB)
        {
            // LSB: low audio cut maps to inner edge (less negative HighHz)
            RadioState.ActiveVfo.Filter.HighHz = -hz;
        }
        else if (mode is RadioMode.USB or RadioMode.DigU)
        {
            // USB / DIG-U: low audio cut maps to inner edge (smaller positive LowHz)
            RadioState.ActiveVfo.Filter.LowHz = +hz;
        }
        else if (mode == RadioMode.AM)
        {
            // AM DSB shading is set by Hi cut only (symmetric). Lo still sent for audio path if needed.
            // Keep RF display edges symmetric from current high cut.
            RecomputeFilterHzForCurrentCutsAndMode();
        }
        else
        {
            // CW: low side (negative) of narrow passband — recompute full CW width from CW filter
            RecomputeFilterHzForCurrentCutsAndMode();
        }
        // Filter Hz mutation above triggers OnActiveFilterPropertyChanged which sends the correct index via service.
        // (Do not call SetFilterLowAsync here with index value -- it expects Hz.)
        MonitorTextBoxText($" LowCutIndex set: {value} ({LowCutLabel})");
        SaveModeFilterProfile(mode);
        SaveLastUsedForCurrentBand();
    }
    partial void OnHighCutIndexChanged(int value)
    {
        value = (value % 5 + 5) % 5;
        HighCutLabel = HighCutOptions[value];
        int hz = HighCutHzValues[value];
        var mode = RadioState.ActiveVfo.Mode;
        if (mode == RadioMode.LSB)
        {
            // LSB: high audio cut maps to outer edge (more negative LowHz)
            RadioState.ActiveVfo.Filter.LowHz = -hz;
        }
        else if (mode is RadioMode.USB or RadioMode.DigU)
        {
            // USB / DIG-U: high audio cut maps to outer edge (larger positive HighHz)
            RadioState.ActiveVfo.Filter.HighHz = +hz;
        }
        else if (mode == RadioMode.AM)
        {
            // AM: both RF edges = ± high cut (original AM markers)
            RadioState.ActiveVfo.Filter.LowHz = -hz;
            RadioState.ActiveVfo.Filter.HighHz = +hz;
            UpdateCurrentSpectrumFilters();
        }
        else
        {
            // CW: recompute full width from CW filter button (Hi is not the AM-style edge)
            RecomputeFilterHzForCurrentCutsAndMode();
        }
        // Filter Hz mutation above triggers OnActiveFilterPropertyChanged which sends the correct index via service.
        // (Do not call SetFilterHighAsync here with index value -- it expects Hz.)
        MonitorTextBoxText($" HighCutIndex set: {value} ({HighCutLabel})");
        SaveModeFilterProfile(mode);
        SaveLastUsedForCurrentBand();
    }
    partial void OnCwFilterIndexChanged(int value)
    {
        value = (value % 3 + 3) % 3;
        CwFilterLabel = CwFilterOptions[value];
        // This CW value is the bandwidth filter selection (index sent as CW_BW).
        // The actual bandpass width is applied via Recompute when in CW mode.
        // (CwPitchHz is not touched here; CW pitch feature is deferred.)
        if (RadioState.ActiveVfo.Mode == RadioMode.CW)
        {
            RecomputeFilterHzForCurrentCutsAndMode();
        }
        _ = _radioService.SetCwFilterAsync(value);
        MonitorTextBoxText($" CwFilterIndex set: {value} ({CwFilterLabel})");
        SaveModeFilterProfile(RadioState.ActiveVfo.Mode);
        SaveLastUsedForCurrentBand();
    }

    /// <summary>
    /// True when this client owns local backends: Launch Servers ON and backend IP is loopback.
    /// Only then is auto Stop/Start of ms-sdr safe for PROFICIO-MKII changes.
    /// </summary>
    public bool IsLocalBackendOwner
    {
        get
        {
            if (!LaunchServersOnStart)
                return false;
            string ip = (BackendIp ?? "").Trim();
            if (string.IsNullOrEmpty(ip))
                return true; // default local
            return ip.Equals("127.0.0.1", StringComparison.OrdinalIgnoreCase)
                || ip.Equals("localhost", StringComparison.OrdinalIgnoreCase)
                || ip.Equals("::1", StringComparison.OrdinalIgnoreCase);
        }
    }

    partial void OnExternalElectronicKeyerChanged(bool value)
    {
        SpectrumWaterfallSettings.ExternalElectronicKeyer = value;
        SpectrumWaterfallSettings.Save();
        OnPropertyChanged(nameof(PicKeyerControlsEnabled));
        OnPropertyChanged(nameof(KeyerMemPanelEnabled));

        // Host mscc.ini on this PC only when we own local backends (Launch Servers + loopback).
        // Remote radio PC uses Start-MsccServers.bat / mscc-init for its own mscc.ini.
        bool localOwner = IsLocalBackendOwner;
        bool wrote = false;
        if (localOwner)
            wrote = ConfigBootstrap.WriteProficioMkii(mkii: !value);

        string mode = value ? "legacy / external keyer (PROFICIO-MKII=0)" : "MKII internal keyer (PROFICIO-MKII=1)";
        MonitorTextBoxText($" External electronic keyer: {(value ? "ON" : "OFF")} → {mode}" +
                           (wrote ? " (local mscc.ini updated)" : " (client sticky; host mscc.ini is separate when remote)"));

        if (!(IsRadioRunning || _radioService.IsConnected))
        {
            if (!localOwner)
            {
                MonitorTextBoxText(
                    " Remote/connect-only: set PROFICIO-MKII on the radio PC before Start " +
                    "(Windows: Start-MsccServers.bat legacy|mkii; Linux: mscc-init).");
            }
            return;
        }

        if (localOwner)
        {
            // Majority path: same PC owns backends — auto Stop/Start so ms-sdr re-reads mscc.ini.
            var ret = MessageBox.Show(
                "External electronic keyer is applied when local ms-sdr starts.\n\n" +
                "Stop and Start now so backends re-read mscc.ini?",
                "MSCC — Restart local backends?",
                MessageBoxButton.YesNo,
                MessageBoxImage.Question,
                MessageBoxResult.Yes);
            if (ret == MessageBoxResult.Yes)
            {
                StopRadioService("external-keyer change");
                StartRadioService("external-keyer change");
            }
            else
            {
                MonitorTextBoxText(" External keyer change deferred — Stop/Start when ready.");
            }
            return;
        }

        // Remote / Launch Servers off: disconnect only; host ms-sdr keeps running until
        // operator restarts it on the radio PC. Do not auto-Start (host may still be old mode).
        string hostMode = value ? "legacy (PROFICIO-MKII=0)" : "MKII (PROFICIO-MKII=1)";
        MessageBox.Show(
            "This client is connect-only or pointed at a remote server.\n\n" +
            "Session will Stop (disconnect). Backends on the radio PC are not restarted from here.\n\n" +
            "On the radio PC, ensure mscc.ini matches " + hostMode + " and restart ms-sdr:\n" +
            "  • Windows:  Start-MsccServers.bat legacy   or   Start-MsccServers.bat mkii\n" +
            "              (writes mscc.ini and restarts backends)\n" +
            "  • Linux:    mscc-init → PROFICIO-MKII, then restart ms-sdr\n\n" +
            "If the host is already in that mode, skip the host step.\n" +
            "Then press Start on this client.",
            "MSCC — Remote / connect-only",
            MessageBoxButton.OK,
            MessageBoxImage.Information);
        StopRadioService("external-keyer change (remote)");
        MonitorTextBoxText(
            " External keyer: session stopped. Restart host backends if needed, then press Start.");
    }

    partial void OnCwSpeedChanged(int value)
    {
        value = Math.Clamp(value, 5, 60);
        if (ExternalElectronicKeyer) return;
        _ = _radioService.SetCwWpmAsync(value);
        MonitorTextBoxText($" CW Speed set: {value} WPM");
    }

    partial void OnCwMemTextWpmChanged(int value)
    {
        int clamped = SpectrumWaterfallSettings.ClampCwMemTextWpm(value);
        if (clamped != value)
        {
            CwMemTextWpm = clamped;
            return;
        }
        SpectrumWaterfallSettings.CwMemTextWpm = clamped;
        try { SpectrumWaterfallSettings.Save(); } catch { /* best-effort sticky */ }
        if (ExternalElectronicKeyer) return;
        _ = _radioService.SetKeyerMemTextWpmAsync(clamped);
        MonitorTextBoxText(
            clamped <= 0
                ? " CW Farnsworth (memory text WPM): Off"
                : $" CW Farnsworth (memory text WPM): {clamped}");
    }

    partial void OnCwKeyerModeChanged(int value)
    {
        if (ExternalElectronicKeyer) return;
        _ = _radioService.SetCwKeyerModeAsync(value);
        MonitorTextBoxText($" CW Keyer Mode set: {CwKeyerModeOptions[value]}");
    }

    partial void OnCwSpacingChanged(int value)
    {
        if (ExternalElectronicKeyer) return;
        _ = _radioService.SetCwSpacingAsync(value);
        MonitorTextBoxText($" CW Spacing set: {CwSpacingOptions[value]}");
    }

    partial void OnCwPaddleChanged(int value)
    {
        if (ExternalElectronicKeyer) return;
        _ = _radioService.SetCwPaddleAsync(value);
        MonitorTextBoxText($" CW Paddle set: {CwPaddleOptions[value]}");
    }

    partial void OnCwWeightIndexChanged(int value)
    {
        value = (value % 3 + 3) % 3;
        if (ExternalElectronicKeyer) return;
        int weight = CwWeightValues[value];
        _ = _radioService.SetCwWeightAsync(weight);
        MonitorTextBoxText($" CW Weight set: {weight}");
    }

    partial void OnCwPitchIndexChanged(int value)
    {
        value = (value % 4 + 4) % 4;
        // To match original ms-sdr expectation for CMD_SET_CW_PITCH, send the INDEX (0-3), not the Hz.
        // Also send current CW filter (BW) first, as original does before PITCH.
        _ = _radioService.SetCwFilterAsync(CwFilterIndex);
        _ = _radioService.SetCwPitchAsync(value);
        OnPropertyChanged(nameof(CwPitchButtonText));
        MonitorTextBoxText($" CW Pitch set: {CwPitchOptions[value]} (index {value})");
    }

    partial void OnCwHoldChanged(int value)
    {
        value = Math.Clamp(value, 1, 500);
        _ = _radioService.SetCwTxHoldAsync(value);
        MonitorTextBoxText($" CW TX Hold set: {value} ms");
    }

    partial void OnCwQskChanged(bool value)
    {
        _ = _radioService.SetCwQskAsync(value);
        MonitorTextBoxText($" CW QSK/Potentia set: {value}");
    }

    partial void OnCwPhonesChanged(bool value)
    {
        _ = _radioService.SetCwPhonesAsync(value);
        MonitorTextBoxText($" CW Phones set: {value}");
    }

    // ── CQ / keyer memory (0x9C) — Avalonia-compatible; see KEYER-MEMORY-GUI-UDP-BEHAVIOR.md ──

    [ObservableProperty] private string _keyerMem0 = "";
    [ObservableProperty] private string _keyerMem1 = "";
    [ObservableProperty] private string _keyerMem2 = "";
    [ObservableProperty] private string _keyerMem3 = "";
    [ObservableProperty] private string _keyerMemStatus = "";
    [ObservableProperty] private bool _keyerMemBusy;

    private bool _suppressKeyerMemSave;

    /// <summary>False while a store is in progress or external keyer/legacy mode is on.</summary>
    public bool KeyerMemPanelEnabled => !KeyerMemBusy && !ExternalElectronicKeyer;

    public string KeyerMem0Count => $"{SanitizeKeyerMem(KeyerMem0).Length}/48";
    public string KeyerMem1Count => $"{SanitizeKeyerMem(KeyerMem1).Length}/48";
    public string KeyerMem2Count => $"{SanitizeKeyerMem(KeyerMem2).Length}/48";
    public string KeyerMem3Count => $"{SanitizeKeyerMem(KeyerMem3).Length}/48";

    partial void OnKeyerMemBusyChanged(bool value) => OnPropertyChanged(nameof(KeyerMemPanelEnabled));

    partial void OnKeyerMem0Changed(string value)
    {
        OnPropertyChanged(nameof(KeyerMem0Count));
        PersistKeyerMemToSettings();
    }

    partial void OnKeyerMem1Changed(string value)
    {
        OnPropertyChanged(nameof(KeyerMem1Count));
        PersistKeyerMemToSettings();
    }

    partial void OnKeyerMem2Changed(string value)
    {
        OnPropertyChanged(nameof(KeyerMem2Count));
        PersistKeyerMemToSettings();
    }

    partial void OnKeyerMem3Changed(string value)
    {
        OnPropertyChanged(nameof(KeyerMem3Count));
        PersistKeyerMemToSettings();
    }

    private void PersistKeyerMemToSettings()
    {
        if (_suppressKeyerMemSave) return;
        SpectrumWaterfallSettings.KeyerMem0 = SanitizeKeyerMem(KeyerMem0);
        SpectrumWaterfallSettings.KeyerMem1 = SanitizeKeyerMem(KeyerMem1);
        SpectrumWaterfallSettings.KeyerMem2 = SanitizeKeyerMem(KeyerMem2);
        SpectrumWaterfallSettings.KeyerMem3 = SanitizeKeyerMem(KeyerMem3);
        SpectrumWaterfallSettings.Save();
    }

    private static string SanitizeKeyerMem(string? text) =>
        SpectrumWaterfallSettings.ClampKeyerMemText(text);

    private string GetKeyerMemText(int slot) => slot switch
    {
        0 => KeyerMem0,
        1 => KeyerMem1,
        2 => KeyerMem2,
        3 => KeyerMem3,
        _ => ""
    };

    private static bool TryParseKeyerSlot(object? parameter, out int slot)
    {
        slot = 0;
        if (parameter is int i)
        {
            slot = Math.Clamp(i, 0, 3);
            return true;
        }
        if (parameter is string s && int.TryParse(s, out int parsed))
        {
            slot = Math.Clamp(parsed, 0, 3);
            return true;
        }
        return false;
    }

    /// <summary>R — store text box for slot 0..3 to keyer EEPROM via 0x9C (no auto-play).</summary>
    [RelayCommand]
    private async Task RecordKeyerMem(object? parameter)
    {
        if (!TryParseKeyerSlot(parameter, out int slot)) return;
        if (ExternalElectronicKeyer)
        {
            KeyerMemStatus = "Disabled — external electronic keyer (legacy)";
            return;
        }
        if (!IsRadioRunning || !_radioService.IsConnected)
        {
            KeyerMemStatus = "Not connected — Start session first";
            MonitorTextBoxText(" Keyer mem R: not connected");
            return;
        }
        if (KeyerMemBusy) return;

        string text = SanitizeKeyerMem(GetKeyerMemText(slot));
        KeyerMemBusy = true;
        KeyerMemStatus = $"Storing slot {slot}…";
        MonitorTextBoxText($" Keyer mem R slot {slot}: \"{text}\" ({text.Length} chars)");
        try
        {
            await _radioService.KeyerMemoryStoreAsync(slot, text).ConfigureAwait(true);
            PersistKeyerMemToSettings();
            KeyerMemStatus = $"Stored slot {slot} ({text.Length} chars) — UDP sequence sent";
            MonitorTextBoxText($" Keyer mem R slot {slot}: store sequence sent OK");
        }
        catch (Exception ex)
        {
            KeyerMemStatus = $"Store slot {slot} failed";
            MonitorTextBoxText($" Keyer mem R error: {ex.Message}");
        }
        finally
        {
            KeyerMemBusy = false;
        }
    }

    /// <summary>
    /// P — play slot once (0x9C SELECT+PLAY). In CW mode does not assert host PTT
    /// (Proficio keys CW PA via keyer line only). Paddle aborts on radio.
    /// </summary>
    [RelayCommand]
    private async Task PlayKeyerMem(object? parameter)
    {
        if (!TryParseKeyerSlot(parameter, out int slot)) return;
        if (ExternalElectronicKeyer)
        {
            KeyerMemStatus = "Disabled — external electronic keyer (legacy)";
            return;
        }
        if (!IsRadioRunning || !_radioService.IsConnected)
        {
            KeyerMemStatus = "Not connected — Start session first";
            MonitorTextBoxText(" Keyer mem P: not connected");
            return;
        }
        if (KeyerMemBusy) return;

        string text = SanitizeKeyerMem(GetKeyerMemText(slot));
        bool modeIsCw = string.Equals((ActiveMode ?? "").Trim(), "CW", StringComparison.OrdinalIgnoreCase);
        if (modeIsCw)
            MonitorTextBoxText(" Keyer mem P: CW mode — host PTT not used; play uses keyer line");
        else
            MonitorTextBoxText(" Keyer mem P: non-CW — play still 0x9C only (no host PTT latch)");

        KeyerMemStatus = modeIsCw
            ? $"Play slot {slot} (CW — keyer keys TX)…"
            : $"Play slot {slot}…";
        MonitorTextBoxText($" Keyer mem P slot {slot}: SELECT + PLAY (0x9C) text=\"{text}\"");
        try
        {
            await _radioService.KeyerMemoryPlayAsync(slot).ConfigureAwait(true);
            KeyerMemStatus = modeIsCw
                ? $"Play sent slot {slot} — listen for keyer CW (paddle aborts)"
                : $"Play sequence sent for slot {slot}";
            MonitorTextBoxText($" Keyer mem P slot {slot}: play sequence sent OK");
        }
        catch (Exception ex)
        {
            KeyerMemStatus = $"Play slot {slot} failed";
            MonitorTextBoxText($" Keyer mem P error: {ex.Message}");
        }
    }

    partial void OnStepIndexChanged(int value)
    {
        value = (value % 6 + 6) % 6;
        StepLabel = StepLabelsArr[value];
        _ = _radioService.SetStepAsync(value);
        SpectrumWaterfallSettings.StepIndex = value;
        SpectrumWaterfallSettings.Save();
        MonitorTextBoxText($" StepIndex set: {value} ({StepLabel})");
    }

    // Server address (IP or domain) on Main tab. Changes are persisted to MSCC_Client.ini.
    // NO re-initialization or service restart here. User must stop and restart MSCC.
    partial void OnBackendIpChanged(string value)
    {
        if (string.IsNullOrWhiteSpace(value)) return;
        SpectrumWaterfallSettings.UpdateServerAddress(value, BackendPort);
        ShowServerChangePopup();
    }

    partial void OnBackendPortChanged(int value)
    {
        SpectrumWaterfallSettings.UpdateServerAddress(BackendIp, value);
        ShowServerChangePopup();
    }

    partial void OnAutoStartServersChanged(bool value)
    {
        SpectrumWaterfallSettings.AutoStartServers = value;
        SpectrumWaterfallSettings.Save();
        MonitorTextBoxText($" Auto-start (window load): {(value ? "ON" : "OFF")} (saved)");
    }

    partial void OnLaunchServersOnStartChanged(bool value)
    {
        RefreshSetupStatus();
        SpectrumWaterfallSettings.LaunchServersOnStart = value;
        SpectrumWaterfallSettings.Save();
        MonitorTextBoxText(
            $" Launch servers on Start: {(value ? "ON (spawn backends)" : "OFF (connect only)")} (saved)");
    }

    /// <summary>
    /// Starts the radio service using the configuration loaded at construction time (from MSCC_Client.ini).
    /// Safe to call when already running (no-op). Honors <see cref="LaunchServersOnStart"/>.
    /// </summary>
    public async void StartRadioService(string reason = "manual")
    {
        if (IsRadioRunning || _radioService.IsConnected)
        {
            MonitorTextBoxText($" Start ({reason}): already running");
            IsRadioRunning = true;
            return;
        }

        bool launch = LaunchServersOnStart;
        if (launch)
        {
            var setup = ConfigBootstrap.EvaluateLocalSetup(launchServers: true);
            if (!setup.IsComplete)
            {
                RefreshSetupStatus();
                MonitorTextBoxText(
                    $" Start ({reason}) blocked — setup incomplete: {string.Join(", ", setup.Missing)}");
                return; // UI shows dialog on manual Start; auto-start only logs
            }

            // ms-sdr gethostbyname(MSCC_IP) for GUI UDP — seed templates may still say Ron-PC.
            int hostFixed = ConfigBootstrap.EnsureLocalBackendHostnames();
            if (hostFixed > 0)
                MonitorTextBoxText(
                    $" Local backend host: MSCC_IP/PROFICIO_DLL_IP → 127.0.0.1 ({hostFixed} file(s))");

            // PROFICIO-MKII is read once at ms-sdr start — ensure sticky choice is on disk first.
            ConfigBootstrap.WriteProficioMkii(mkii: !ExternalElectronicKeyer);
            MonitorTextBoxText(
                ExternalElectronicKeyer
                    ? " PROFICIO-MKII=0 (legacy / external electronic keyer)"
                    : " PROFICIO-MKII=1 (MKII internal keyer)");
        }

        MonitorTextBoxText(
            $" Start ({reason}): {(launch ? "connect + launch backends" : "connect only (no backend spawn)")}");
        try
        {
            await _radioService.StartAsync(launchSubsystems: launch);
            IsRadioRunning = _radioService.IsConnected;
            if (IsRadioRunning)
                ApplyPanResolution("start");
            RefreshSetupStatus();
            MonitorTextBoxText(
                IsRadioRunning
                    ? " Start complete — session running (button = Stop)"
                    : " Start finished but IsConnected is false");
        }
        catch (Exception ex)
        {
            IsRadioRunning = false;
            MonitorTextBoxText($" Start failed: {ex.Message}");
        }
    }

    /// <summary>
    /// Apply S/W pan resolution (800/1600/3200) to radio service + SDRcore.
    /// </summary>
    public void ApplyPanResolution(string reason = "settings")
    {
        int bins = SpectrumWaterfallSettings.PanResolutionBins;
        try
        {
            _ = _radioService.SetPanResolutionAsync(bins);
            // Bin count change invalidates waterfall history lengths — clear every spectrum control.
            Application.Current?.Dispatcher.BeginInvoke(() =>
            {
                try
                {
                    if (Application.Current?.MainWindow is MainWindow mw)
                        ClearAllWaterfallHistories(mw);
                }
                catch { /* ignore */ }
            });
            MonitorTextBoxText(
                $" Pan resolution ({reason}): {SpectrumWaterfallSettings.PanResolutionLabel} → {bins} bins");
        }
        catch (Exception ex)
        {
            MonitorTextBoxText($" Pan resolution apply error: {ex.Message}");
        }
    }

    private static void ClearAllWaterfallHistories(DependencyObject parent)
    {
        if (parent == null) return;
        if (parent is Controls.SpectrumDisplayControl spec)
            spec.ClearWaterfallHistory();
        int n = System.Windows.Media.VisualTreeHelper.GetChildrenCount(parent);
        for (int i = 0; i < n; i++)
            ClearAllWaterfallHistories(System.Windows.Media.VisualTreeHelper.GetChild(parent, i));
    }

    /// <summary>
    /// Stops the session: CMD_SET_STOP if this client launched backends, then tears down UDP.
    /// After Stop, <see cref="StartRadioService"/> can start again (restart / new COM).
    /// </summary>
    public void StopRadioService(string reason = "manual")
    {
        MonitorTextBoxText($" Stop ({reason}): ending radio session...");
        try
        {
            _radioService.Stop();
        }
        catch (Exception ex)
        {
            MonitorTextBoxText($" Stop error: {ex.Message}");
        }
        IsRadioRunning = false;
        MonitorTextBoxText(" Stop complete — press Start to connect again (or restart servers)");
    }

    public bool SuppressLastUsedSave { get; set; } = false;
    private bool _suppressTransmitCommands;

    public string CurrentGenSub { get; set; } = "USER";

    // Each VFO remembers its own last band so Toggle VFO restores the correct band button highlight
    // (shared RadioState.CurrentBand alone would stick on the last band selected on either VFO).
    private string _bandForVfoA = "40m";
    private string _bandForVfoB = "40m";

    /// <summary>
    /// True when ActiveVfo is VFO B — last-used load/save use MSCC_LastUsed_VFOB.ini.
    /// VFO A continues to use MSCC_LastUsed.ini.
    /// </summary>
    public bool UseVfoBLastUsedFile => RadioState.ActiveVfo == RadioState.VfoB;

    private void StoreBandForActiveVfo(string? band)
    {
        if (string.IsNullOrWhiteSpace(band)) return;
        band = band.Trim();
        if (band == "?") return;
        if (RadioState.ActiveVfo == RadioState.VfoB)
            _bandForVfoB = band;
        else
            _bandForVfoA = band;
    }

    /// <summary>
    /// Maps a frequency (Hz) to the band button name used by CurrentBand / last-used keys.
    /// Shared with GEN / WWV special frequencies treated as "gen".
    /// LF: 2200m (default 136.0 kHz), 630m (default 474.2 kHz digital).
    /// </summary>
    public static string GetBandNameForFrequency(long freq)
    {
        // HF beacons / general + Geminus LF frequency-cal carriers (GEN button)
        if (freq == 5_000_000 || freq == 10_000_000 || freq == 15_000_000 || freq == 20_000_000 ||
            freq == 3_330_000 || freq == 7_850_000 || freq == 9_996_000 ||
            freq == 198_000 || freq == 660_000 || freq == 880_000)
            return "gen";

        return freq switch
        {
            // 2200m amateur (~135.7–137.8 kHz); default digital 136.000 kHz
            >= 130_000 and < 150_000 => "2200m",
            // 630m amateur (~472–479 kHz); default digital 474.200 kHz
            >= 470_000 and < 480_000 => "630m",
            >= 1_800_000 and < 2_000_000 => "160m",
            >= 3_500_000 and < 4_000_000 => "80m",
            >= 5_330_000 and < 5_405_000 => "60m",
            >= 7_000_000 and < 7_300_000 => "40m",
            >= 10_100_000 and < 10_150_000 => "30m",
            >= 14_000_000 and < 14_350_000 => "20m",
            >= 18_068_000 and < 18_168_000 => "17m",
            >= 21_000_000 and < 21_450_000 => "15m",
            >= 24_890_000 and < 24_990_000 => "12m",
            >= 28_000_000 and < 30_000_000 => "10m",
            _ => "?"
        };
    }

    /// <summary>Default mode when no last-used entry: USB on LF (digital), LSB below 10 MHz, USB above.</summary>
    public static string DefaultModeForFrequency(long freqHz)
    {
        if (freqHz < 1_000_000) return "USB";
        if (freqHz < 10_000_000) return "LSB";
        return "USB";
    }

    public void SaveLastUsedForCurrentBand()
    {
        if (SuppressLastUsedSave) return;
        string band = RadioState.CurrentBand;
        if (string.IsNullOrEmpty(band)) return;
        if (band == "gen" && CurrentGenSub != "USER") return;
        long f = RadioState.ActiveVfo.FrequencyHz;
        string m = ActiveMode ?? "USB";
        int l = LowCutIndex;
        int h = HighCutIndex;
        int c = CwFilterIndex;
        bool forVfoB = UseVfoBLastUsedFile;
        SpectrumWaterfallSettings.SaveLastUsedForBand(band, f, m, l, h, c, forVfoB);
        MonitorTextBoxText($" SaveLastUsed: {(forVfoB ? "VFOB" : "VFOA")} band={band} f={f}");
    }

    public void LoadLastUsedForBand(string band, long defaultFreq)
    {
        if (string.IsNullOrEmpty(band)) return;
        bool forVfoB = UseVfoBLastUsedFile;
        var (f, m, l, h, c) = SpectrumWaterfallSettings.LoadLastUsedForBand(band, forVfoB);
        long useF = f > 0 ? f : defaultFreq;
        string useM = !string.IsNullOrEmpty(m) ? m : DefaultModeForFrequency(useF);

        // Suppress intermediate saves: TuneToFrequency / ActiveMode / *CutIndex each call
        // SaveLastUsedForCurrentBand. Writing mid-load with partial state was corrupting last-used
        // (especially after the old substring key match loaded the wrong band's freq).
        // Also suppress mode-profile swap so band last-used cuts win over global mode defaults.
        bool prevSuppress = SuppressLastUsedSave;
        bool prevProfile = _suppressModeProfileSwap;
        SuppressLastUsedSave = true;
        _suppressModeProfileSwap = true;
        try
        {
            // This path (Tune + ActiveMode=) sends to server. Only call from user-initiated band selection
            // (e.g. band buttons). NEVER call from server report handlers (BandReported etc.).
            TuneToFrequency(useF);
            ActiveMode = useM;
            if (l >= 0) LowCutIndex = l;
            if (h >= 0) HighCutIndex = h;
            if (c >= 0) CwFilterIndex = c;
        }
        finally
        {
            SuppressLastUsedSave = prevSuppress;
            _suppressModeProfileSwap = prevProfile;
        }

        MonitorTextBoxText($" LoadLastUsed: {(forVfoB ? "VFOB" : "VFOA")} band={band} f={useF}");
    }

    private void ShowServerChangePopup()
    {
        try
        {
            var msg = "You have entered a new server address (IP address or domain name).\n\n" +
                      "You must stop MSCC and then start MSCC for the change to take effect.";
            System.Windows.MessageBox.Show(msg, "MSCC Server Address",
                System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Information);
        }
        catch { }
        MonitorTextBoxText(" Server address changed in UI - popup shown (restart MSCC required)");
    }

    private void WireService(IRadioService svc)
    {
        // SpectrumUpdated is already wired in the main ctor (with proper UI dispatch).
        // We do not re-subscribe here to avoid double-processing every frame.

        svc.PacketReceived += packet =>
        {
            LastReceivedOpcode = packet.Opcode;
        };

        svc.FrequencyReported += freq =>
        {
            RadioState.ActiveVfo.FrequencyHz = freq;
            // FrequencyDisplay will update via VfoState
            // Do NOT save last-used or send anything here: this is a server report (ms_sdr push).
            MonitorTextBoxText($" Frequency reported from backend: {freq}");
        };

        svc.ModeReported += mode =>
        {
            var parsed = ParseMode(mode);
            // Radio only has USB; if client is on DIG-U and server echoes USB, keep DIG-U.
            if (RadioState.ActiveVfo.Mode == RadioMode.DigU && parsed == RadioMode.USB)
            {
                OnPropertyChanged(nameof(ActiveMode));
                return;
            }
            RadioState.ActiveVfo.Mode = parsed;
            OnPropertyChanged(nameof(ActiveMode));
            NotifyMainOperatePower();
            // Do not SaveLastUsedForCurrentBand on server report.
        };

        svc.SmeterReported += dbm =>
        {
            SMeter = Db_to_Smeter(dbm);
            //MonitorTextBoxText($" Smeter reported: {dbm} dBm -> S{SMeter}");
        };

        svc.PowerReported += p =>
        {
            PowerOut = Math.Clamp(p, 0, 100);
        };

        // Bidirectional power reports for Rx/Tx sliders (from ms-sdr at startup)
        svc.TunePowerReported += v => { TunePowerPercent = v; MonitorTextBoxText($" Tune power % reported: {v}"); };
        svc.CwPowerReported += v => { CwPowerPercent = v; MonitorTextBoxText($" CW power % reported: {v}"); };
        svc.SsbPowerReported += v => { SsbPowerPercent = v; MonitorTextBoxText($" SSB power % reported: {v}"); };
        svc.AmCarrierReported += v => { AmCarrierPercent = v; MonitorTextBoxText($" AM carrier % reported: {v}"); };

        svc.DefaultLowCutIndexReported += idx =>
        {
            LowCutDefaultIndex = idx; // will send echo, harmless for UDP
            MonitorTextBoxText($" LowCutDefaultIndex reported: {idx}");
        };

        svc.DefaultTxIndexReported += idx =>
        {
            TxDefaultIndex = idx;
            MonitorTextBoxText($" TxDefaultIndex reported: {idx}");
        };

        svc.DefaultHighCutIndexReported += idx =>
        {
            HighCutDefaultIndex = idx;
            MonitorTextBoxText($" HighCutDefaultIndex reported: {idx}");
        };

        svc.DefaultCwFilterIndexReported += idx =>
        {
            CwFilterDefaultIndex = idx;
            MonitorTextBoxText($" CwFilterDefaultIndex reported: {idx}");
        };

        svc.AgcLevelReported += level =>
        {
            AgcLevel = level;
            MonitorTextBoxText($" AgcLevel reported: {level}");
        };

        svc.AgcFastReleaseReported += ms =>
        {
            AgcFastRelease = ms;
            MonitorTextBoxText($" AgcFastRelease reported: {ms}");
        };

        // Aud/Sys reports wiring
        svc.SpeakerVolumeReported += v =>
        {
            if (IsDigitalAudio) RadioState.DVolume = v; else RadioState.PVolume = v;
            MonitorTextBoxText($" SpeakerVolume reported: {v} (mode={(IsDigitalAudio ? "D" : "P")})");
        };
        svc.MicVolumeReported += v =>
        {
            if (IsDigitalAudio) RadioState.DMicGain = v; else RadioState.PMicGain = v;
            MonitorTextBoxText($" MicVolume reported: {v} (mode={(IsDigitalAudio ? "D" : "P")})");
        };
        svc.BandReported += b => 
        { 
            RadioState.CurrentBand = b; 
            MonitorTextBoxText($" Band reported from startup: {b}"); 
            // Do NOT call LoadLastUsedForBand here. Server push -- no outgoing commands allowed.
            // Server will push the actual freq (e.g. 0xBB), mode, etc. via separate reports.
        };
        svc.CompressionStateReported += b =>
        {
            // Bidirectional: adopt server state for UI; remember for session when on P
            _suppressCompressionCommand = true;
            try
            {
                CompressionOn = b;
                if (!IsDigitalAudio)
                    _sessionCompressionOn = b;
            }
            finally
            {
                _suppressCompressionCommand = false;
            }
            MonitorTextBoxText(
                $" CompressionState reported: {b} (sessionPreferred={_sessionCompressionOn}, audio={(IsDigitalAudio ? "D" : "P")})");
        };
        svc.CompressionLevelReported += v => { CompressionLevel = v; MonitorTextBoxText($" CompressionLevel reported: {v}"); };
        svc.MonitorReported += b => { MonitorOn = b; MonitorTextBoxText($" Monitor reported: {b}"); };
        svc.TransverterReported += b => { TransverterOn = b; MonitorTextBoxText($" Transverter reported: {b}"); };
        svc.AudioDigitalModeReported += b => { IsDigitalAudio = b; MonitorTextBoxText($" AudioDigitalMode reported: {(b ? "D" : "P")}"); };

        svc.PhonesVolumeLevelReported += v => { RadioState.PVolume = v; MonitorTextBoxText($" PhonesVolumeLevel reported: {v}"); };
        svc.PhonesMicGainLevelReported += v => { RadioState.PMicGain = v; MonitorTextBoxText($" PhonesMicGainLevel reported: {v}"); };
        svc.DigitalVolumeLevelReported += v => { RadioState.DVolume = v; MonitorTextBoxText($" DigitalVolumeLevel reported: {v}"); };
        svc.DigitalMicGainLevelReported += v => { RadioState.DMicGain = v; MonitorTextBoxText($" DigitalMicGainLevel reported: {v}"); };
        svc.AudioDeviceReported += dev =>
        {
            _suppressAudioDeviceSend = true;
            try
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
            }
            finally { _suppressAudioDeviceSend = false; }
            string label = dev switch
            {
                Opcodes.DIGITAL_SOUND_DEVICE => "D",
                Opcodes.REMOTE_SOUND_DEVICE => "R",
                _ => "P",
            };
            MonitorTextBoxText($" AudioDevice reported: {dev} ({label})");
        };

        svc.TxSetByServerReported += v =>
        {
            _suppressTransmitCommands = true;
            TxSetByServer = v;
            PttOn = v;
            TuneMode = v;
            _suppressTransmitCommands = false;
            MonitorTextBoxText($" TxSetByServer reported: {v} (server controls transmit)");
        };

        // AMP button bidirectional: ms-sdr pushes CMD_SET_PA_BYPASS (0xF7) at start / on change.
        svc.PaBypassReported += ampOn =>
        {
            _suppressAmpCommand = true;
            try
            {
                AmpOn = ampOn;
            }
            finally
            {
                _suppressAmpCommand = false;
            }
            MonitorTextBoxText($" PaBypass/AMP reported: AmpOn={ampOn}");
        };

        // Noise blanker bidirectional (0x80 / 0x81 / 0x82)
        svc.NbEnableReported += on =>
        {
            _suppressNbCommand = true;
            try { NbOn = on; }
            finally { _suppressNbCommand = false; }
        };
        svc.NbPulseWidthReported += us =>
        {
            _suppressNbCommand = true;
            try { NbPulse = Math.Clamp(us, 10, 510); }
            finally { _suppressNbCommand = false; }
        };
        svc.NbThresholdReported += thr =>
        {
            _suppressNbCommand = true;
            try { NbThreshold = Math.Clamp(thr, 1, 1009); }
            finally { _suppressNbCommand = false; }
        };

        // NR bidirectional (0xA3) — NR_VALUE 0=off, non-zero=on+level (Ron punch list)
        svc.NrValueReported += nrValue =>
        {
            void apply() => ApplyNrValueFromServer(nrValue);
            if (System.Windows.Application.Current?.Dispatcher is { } dispatcher &&
                !dispatcher.CheckAccess())
                dispatcher.BeginInvoke(apply);
            else
                apply();
        };

        // Auto notch bidirectional (0x8E) — enable only; no echo on report
        svc.AutoNotchReported += on =>
        {
            void apply()
            {
                _suppressAnCommand = true;
                try { AnOn = on; }
                finally { _suppressAnCommand = false; }
            }
            if (System.Windows.Application.Current?.Dispatcher is { } dispatcher &&
                !dispatcher.CheckAccess())
                dispatcher.BeginInvoke(apply);
            else
                apply();
        };

        // Power cal: after band select (0xA1), server sends calibration step (0xB4)
        svc.BandPowerReported += step =>
        {
            if (System.Windows.Application.Current?.Dispatcher is { } dispatcher &&
                !dispatcher.CheckAccess())
            {
                dispatcher.BeginInvoke(() => ApplyBandPowerReport(step));
            }
            else
            {
                ApplyBandPowerReport(step);
            }
        };

        // CW tab bidirectional startup values
        svc.CwKeyerModeReported += v => { CwKeyerMode = v; MonitorTextBoxText($" CwKeyerMode reported: {v}"); };
        svc.CwSpacingReported += v => { CwSpacing = v; MonitorTextBoxText($" CwSpacing reported: {v}"); };
        svc.CwPaddleReported += v => { CwPaddle = v; MonitorTextBoxText($" CwPaddle reported: {v}"); };
        svc.CwWeightReported += v =>
        {
            int idx = Array.IndexOf(CwWeightValues, v);
            CwWeightIndex = (idx >= 0) ? idx : CwWeightIndex;
            MonitorTextBoxText($" CwWeight reported: {v}");
        };
        svc.CwWpmReported += v => { CwSpeed = v; MonitorTextBoxText($" CwWpm reported: {v}"); };
        svc.CwTxHoldReported += v => { CwHold = v; MonitorTextBoxText($" CwTxHold reported: {v}"); };
        svc.CwMemTextWpmReported += v =>
        {
            CwMemTextWpm = SpectrumWaterfallSettings.ClampCwMemTextWpm(v);
            MonitorTextBoxText($" CwMemTextWpm (Farnsworth) reported: {(CwMemTextWpm <= 0 ? "Off" : CwMemTextWpm.ToString())}");
        };

        svc.ProficioTempReported += t => { ProficioTempC = t; };
        svc.AmpTempReported += t => { AmpTempC = t; };
        svc.AmpCurrentReported += ma => { AmpCurrentMa = ma; MonitorTextBoxText($" AmpCurrent reported: {ma}"); };
        svc.AlcReported += alc => ApplyAlcMeterSample(alc);
        svc.CoreVersionReported += v =>
        {
            CoreVersion = v;
            MonitorTextBoxText($" Core (MSSDR) version: {v}");
        };
        svc.FirmwareVersionReported += v =>
        {
            FirmwareVersion = v;
            MonitorTextBoxText($" FW (firmware) version: {v}");
        };

        svc.ServerKeepAliveLost += () =>
        {
            var dispatcher = System.Windows.Application.Current?.Dispatcher;
            if (dispatcher != null && !dispatcher.CheckAccess())
                dispatcher.BeginInvoke(OnServerKeepAliveLost);
            else
                OnServerKeepAliveLost();
        };

        // I/Q calibration reports (0x56 / 0x8B) — shared by RX IQ and TX IQ
        svc.IqOperationCompleteReported += op =>
        {
            // Prefer RX path when RX is committing (APPLY / ZERO); else TX
            if (RxIqCommitting)
            {
                RxIqCommitting = false;
                if (op == 1)
                {
                    RxIqStatus = "APPLY/ZERO succeeded (IQ_OPERATION_COMPLETE).";
                    MonitorTextBoxText(" RX IQ APPLY SUCCESS (0x56=1)");
                }
                else if (op == 0)
                {
                    RxIqStatus = "APPLY/ZERO failed (IQ_OPERATION_COMPLETE).";
                    MonitorTextBoxText(" RX IQ APPLY FAILED (0x56=0)");
                }
                else
                {
                    RxIqStatus = $"APPLY complete (operand={op}).";
                    MonitorTextBoxText($" RX IQ APPLY COMPLETE operand={op}");
                }
                return;
            }

            if (!TxIqCommitting && !_txIqTabActive)
                return;

            TxIqCommitting = false;
            if (op == 1)
            {
                TxIqStatus = "APPLY succeeded (IQ_OPERATION_COMPLETE).";
                MonitorTextBoxText(" TX IQ APPLY SUCCESS (0x56=1)");
            }
            else if (op == 0)
            {
                TxIqStatus = "APPLY failed (IQ_OPERATION_COMPLETE).";
                MonitorTextBoxText(" TX IQ APPLY FAILED (0x56=0)");
            }
            else
            {
                TxIqStatus = $"APPLY complete (operand={op}).";
                MonitorTextBoxText($" TX IQ APPLY COMPLETE operand={op}");
            }
        };
        svc.IqValueReported += v =>
        {
            int clamped = Math.Clamp(v, -200, 200);
            if (RxIqSessionActive || _rxIqTabActive)
            {
                _suppressRxIqOffset = true;
                try { RxIqOffset = clamped; }
                finally { _suppressRxIqOffset = false; }
                MonitorTextBoxText($" RX IQ value from server: {v}");
                return;
            }
            if (!_txIqTabActive) return;
            _suppressTxIqOffset = true;
            try { TxIqOffset = clamped; }
            finally { _suppressTxIqOffset = false; }
            MonitorTextBoxText($" TX IQ value from server: {v}");
        };

        // Note: the RadioState.PropertyChanged and filter/vfo subs are already global on the state object
    }

    /// <summary>
    /// Average ALC meter samples (0–100) before updating the UI.
    /// Server may send rapid values; rolling mean of last N samples smooths the bar.
    /// Restarts the idle timeout (zero meter if feed stops ~3s — server may not send 0 in RX).
    /// </summary>
    private void ApplyAlcMeterSample(int raw)
    {
        // Meter is 0–100. Accept 0–100 directly; map legacy 0–1000 reports down by 10.
        int sample = raw > 100 ? Math.Clamp(raw / 10, 0, 100) : Math.Clamp(raw, 0, 100);

        _alcSampleRing[_alcSampleIndex] = sample;
        _alcSampleIndex = (_alcSampleIndex + 1) % _alcSampleRing.Length;
        if (_alcSampleCount < _alcSampleRing.Length)
            _alcSampleCount++;

        long sum = 0;
        for (int i = 0; i < _alcSampleCount; i++)
            sum += _alcSampleRing[i];

        AlcValue = (int)((sum + _alcSampleCount / 2) / _alcSampleCount); // rounded mean
        KickAlcIdleTimer();
    }

    /// <summary>Zero ALC meter when both PTT and TUN are off (return to RX).</summary>
    private void MaybeZeroAlcMeterOnRx()
    {
        if (PttOn || TuneMode)
            return;
        _alcIdleTimer?.Stop();
        if (AlcValue != 0 || _alcSampleCount != 0)
        {
            ResetAlcMeter();
            MonitorTextBoxText(" ALC meter zeroed (PTT/TUN off → RX)");
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
                MonitorTextBoxText($" ALC meter zeroed (no samples for {AlcIdleTimeoutSeconds}s)");
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
    }

    private SpectrumUpdate EnrichSpectrumUpdate(SpectrumUpdate update)
    {
        var vfo = RadioState?.ActiveVfo;
        if (vfo == null) return update;

        int cwPitch = 0;
        if (vfo.Mode == RadioMode.CW)
        {
            // Map index to Hz for display shift. CwPitchIndex 0-3 -> 400,600,800,1000
            cwPitch = CwPitchValues[CwPitchIndex];
        }

        // PowerSDR-style dBm window (after dB CAL). Grass ~−116 needs min ≈ −125.
        // Includes BASELINE floor-tune so packet Min/Max match the drawn pane.
        SpectrumColorSettings.GetDisplayGridWindow(out float gridMin, out float gridMax);
        if (gridMax - gridMin < 20f)
            gridMin = gridMax - 20f;

        return update with
        {
            CenterFrequencyHz = vfo.FrequencyHz,
            FilterLowHz = vfo.Filter?.LowHz ?? update.FilterLowHz,
            FilterHighHz = vfo.Filter?.HighHz ?? update.FilterHighHz,
            CwPitchHz = cwPitch,
            MinDb = gridMin,
            MaxDb = gridMax
        };
    }

    private void StartClockTimer()
    {
        if (_clockTimer != null) return;
        _clockTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(1) };
        _clockTimer.Tick += (s, e) => UpdateTimeDisplays();
        UpdateTimeDisplays();
        _clockTimer.Start();
        MonitorTextBoxText(" Clock timer started (1s for time + midnight log reset check)");
    }

    private void StopClockTimer()
    {
        if (_clockTimer != null)
        {
            _clockTimer.Stop();
            _clockTimer = null;
            MonitorTextBoxText(" Clock timer stopped");
        }
    }

    private void UpdateTimeDisplays()
    {
        var now = DateTime.Now;
        var utc = DateTime.UtcNow;
        LocalTimeDisplay = now.ToString("HH:mm:ss");
        LocalDateDisplay = now.ToString("dd.MM.yy");
        UtcTimeDisplay = utc.ToString("HH:mm:ss");
        UtcDateDisplay = utc.ToString("dd.MM.yy");

        // Daily midnight log reset (ported from original Master_State_Machine case 14 + Manage_Log_File)
        int hour = now.Hour;
        if (hour == 0 && !_resetLogFile)
        {
            DebugMonitor.ResetLogFile();
            _resetLogFile = true;
            MonitorTextBoxText(" Daily log file reset at midnight");
        }
        if (hour != 0 && _resetLogFile)
        {
            _resetLogFile = false;
        }
    }

    /// <summary>
    /// Server stopped answering I'm Alive (0xF4). Offer Continue (keep waiting) or Stop session.
    /// </summary>
    private void OnServerKeepAliveLost()
    {
        MonitorTextBoxText(" Server keep-alive LOST — showing warning");

        var result = MessageBox.Show(
            "No \"I'm Alive\" (keep-alive) messages have been received from the server for several seconds.\r\n\r\n" +
            "The server may have stopped, crashed, or the network link may be down.\r\n\r\n" +
            "Yes = Continue running (wait for the server again)\r\n" +
            "No  = Close MSCC",
            "MSCC — Server Not Responding",
            MessageBoxButton.YesNo,
            MessageBoxImage.Warning,
            MessageBoxResult.Yes);

        if (result == MessageBoxResult.Yes)
        {
            MonitorTextBoxText(" Keep-alive warning: user chose Continue");
            _radioService.ResetKeepAliveWatch();
        }
        else
        {
            // Original Manage_Keep_Alive: SERVER DID NOT SEND KEEP ALIVE → Application.Exit()
            MonitorTextBoxText(" Keep-alive warning: user chose Stop — closing MSCC client");
            LastServerMessage = "Server keep-alive lost — closing client.";
            try
            {
                // Prefer closing main window so Closing handler runs (STOP if we launched backends, save placement)
                var main = System.Windows.Application.Current?.MainWindow;
                if (main != null)
                    main.Close();
                else
                    System.Windows.Application.Current?.Shutdown();
            }
            catch (Exception ex)
            {
                MonitorTextBoxText($" Close after keep-alive loss error: {ex.Message}");
                try { System.Windows.Application.Current?.Shutdown(); } catch { }
            }
        }
    }

    /// <summary>Apply Settings → SWR enable/port (call after UI saves).</summary>
    public void ApplySwrMeterSettings(string reason = "settings")
    {
        try
        {
            if (SwrMeterSettings.Enabled)
            {
                _swrMeter.Start(SwrMeterSettings.UdpListenPort);
                MonitorTextBoxText($" SWR meter ({reason}): listening UDP {SwrMeterSettings.UdpListenPort}");
            }
            else
            {
                _swrMeter.Stop();
                ShowExternalSwrFace = false;
                MonitorTextBoxText($" SWR meter ({reason}): disabled");
            }
            UpdateShowExternalSwrFace();
        }
        catch (Exception ex)
        {
            MonitorTextBoxText($" SWR meter apply error: {ex.Message}");
        }
    }

    private void OnSwrReading(SwrMeterReading r)
    {
        RunOnUi(() =>
        {
            SwrValue = r.Swr;
            SwrForwardWatts = r.ForwardWatts;
            SwrReflectedWatts = r.ReflectedWatts;
            SwrThreshold = r.SwrThreshold > 0 ? r.SwrThreshold : 2.0;
            SwrTxRf = r.Tx;

            // Auto IP into active profile if empty
            if (!string.IsNullOrWhiteSpace(r.SourceIp))
            {
                string cur = SwrMeterSettings.ActiveMeterIp(IsGeminusRadioModel);
                if (string.IsNullOrWhiteSpace(cur))
                    SwrMeterSettings.SetActiveMeterIp(IsGeminusRadioModel, r.SourceIp);
            }

            bool wasFault = SwrFault;
            SwrFault = r.Fault;

            if (r.Fault && !wasFault)
            {
                // Rising edge: force RX + inhibit re-key
                _swrFaultLatched = true;
                _swrTxInhibited = true;
                try
                {
                    if (PttOn) PttOn = false;
                    if (AutoTune) AutoTune = false;
                    _ = _radioService.SetTransmitAsync(false);
                }
                catch { /* ignore */ }
                MonitorTextBoxText(" SWR FAULT — forced RX / PTT off (reset meter when RF is low)");
            }
            else if (!r.Fault && wasFault)
            {
                _swrFaultLatched = false;
                _swrTxInhibited = false;
                MonitorTextBoxText(" SWR fault cleared — TX re-enabled (stay RX until you key)");
            }

            UpdateShowExternalSwrFace();
        });
    }

    private void UpdateShowExternalSwrFace()
    {
        if (!SwrMeterSettings.Enabled)
        {
            ShowExternalSwrFace = false;
            return;
        }
        // Keep power face while fault latched even after force-RX (so RESET digital stays usable).
        bool radioTx = PttOn || AutoTune || RadioState.IsTransmitting;
        ShowExternalSwrFace = SwrFault || _swrFaultLatched || SwrTxRf || radioTx;
    }

    /// <summary>HTTP reset to active profile meter IP (or last UDP source).</summary>
    public async Task ResetSwrFaultAsync()
    {
        string ip = SwrMeterSettings.ActiveMeterIp(IsGeminusRadioModel);
        if (string.IsNullOrWhiteSpace(ip))
            ip = _swrMeter.LastSourceIp ?? "";
        MonitorTextBoxText($" SWR RESET → {ip}");
        var (ok, msg) = await _swrMeter.ResetFaultAsync(ip);
        MonitorTextBoxText($" SWR RESET: {(ok ? "OK" : "FAIL")} — {msg}");
        SwrStatusText = msg;
    }

    private void RunOnUi(Action a)
    {
        var d = Application.Current?.Dispatcher;
        if (d == null || d.CheckAccess())
            a();
        else
            d.BeginInvoke(a);
    }

    public void Dispose()
    {
        StopClockTimer();
        if (_alcIdleTimer != null)
        {
            _alcIdleTimer.Stop();
            _alcIdleTimer = null;
        }
        try { _swrMeter.Dispose(); } catch { /* ignore */ }
        _radioService.Stop();
    }
}

/// <summary>
/// One band row for Pwr Cal CALIBRATION STATUS / BAND SELECTION UI.
/// </summary>
public partial class PowerCalBandItem : ObservableObject
{
    public int BandNumber { get; set; }
    public string BandLabel { get; set; } = "";

    /// <summary>True when client-side status file marks this band calibrated.</summary>
    [ObservableProperty]
    private bool _isCalibrated;

    /// <summary>True when this band is selected in BAND SELECTION.</summary>
    [ObservableProperty]
    private bool _isSelected;
}
