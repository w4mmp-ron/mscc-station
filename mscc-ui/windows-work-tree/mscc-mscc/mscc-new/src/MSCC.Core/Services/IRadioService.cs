using System;
using System.Threading;
using System.Threading.Tasks;
using MSCC.Core.Display;
using MSCC.Core.Protocol;

namespace MSCC.Core.Services;

/// <summary>
/// Abstraction for the radio hardware / DSP layer.
/// The UI talks to this, not directly to low-level communication.
/// </summary>
public interface IRadioService : IDisposable
{
    /// <summary>
    /// Fired whenever new spectrum data is available.
    /// </summary>
    event Action<SpectrumUpdate> SpectrumUpdated;

    /// <summary>
    /// Fired for every raw packet received from the backend (useful for debugging and future parsing).
    /// </summary>
    event Action<RadioPacketReceivedEventArgs> PacketReceived;

    /// <summary>
    /// Fired when the backend reports the current tuned frequency (e.g. confirmation or server-driven change).
    /// </summary>
    event Action<long> FrequencyReported;

    /// <summary>
    /// Fired when the backend reports the current mode.
    /// </summary>
    event Action<string> ModeReported;

    /// <summary>
    /// Fired when the backend reports S-meter value (0-15 typical).
    /// </summary>
    event Action<int> SmeterReported;

    /// <summary>
    /// Fired when the backend reports power level (e.g. forward power % or value).
    /// </summary>
    event Action<int> PowerReported;

    // Default filters reports
    event Action<int> DefaultLowCutIndexReported;
    event Action<int> DefaultTxIndexReported;
    event Action<int> DefaultCwFilterIndexReported;
    event Action<int> DefaultHighCutIndexReported;

    // AGC reports
    event Action<int> AgcLevelReported;
    event Action<int> AgcFastReleaseReported;

    /// <summary>
    /// Indicates whether the service is currently connected to a backend (real or simulated).
    /// </summary>
    bool IsConnected { get; }

    /// <summary>
    /// Starts the radio connection (real or simulated).
    /// </summary>
    /// <param name="launchSubsystems">
    /// When true (default), spawn ms-sdr / recv / trans from the app directory.
    /// When false, only open UDP and signal GUI ready — use when backends are already running.
    /// </param>
    Task StartAsync(bool launchSubsystems = true, CancellationToken cancellationToken = default);

    /// <summary>
    /// Stops the radio connection. Sends CMD_SET_STOP only if this client launched
    /// the backends (Launch Servers on at Start); connect-only sessions leave ms-sdr running.
    /// </summary>
    void Stop();

    // Note: Keep-alive ("I'm Alive") sending to tell ms-sdr the UI is running (CMD_SET_KEEP_ALIVE 0xF4)
    // is handled internally by UdpRadioService once started (periodic, matching original state machine).
    // Initial GUI_RUNNING (0xFE) is also sent at StartAsync.

    /// <summary>
    /// Fired when no CMD_SET_KEEP_ALIVE has been received from the server for the watchdog timeout
    /// (after Start). UI should warn the user (Continue resets the watch; Stop ends the session).
    /// </summary>
    event Action ServerKeepAliveLost;

    /// <summary>Reset keep-alive watchdog after user chooses Continue (clears timeout latch).</summary>
    void ResetKeepAliveWatch();

    /// <summary>
    /// Selects the active VFO on the server (CMD_SET_VFO 0xF2).
    /// Pass Opcodes.VFO_A (0) or Opcodes.VFO_B (1).
    /// Must be sent before frequency/mode when switching VFOs.
    /// </summary>
    Task SetActiveVfoAsync(byte vfo, CancellationToken cancellationToken = default);

    /// <summary>
    /// Selects which band to power-calibrate (CMD_SET_BAND_POWER_BAND 0xA1).
    /// Band number is 160, 80, 60, … 10 (same as original Power_Calibration_Controls.Band).
    /// Server replies with CMD_GET_BAND_POWER (0xB4) calibration step.
    /// </summary>
    Task SetBandPowerBandAsync(int bandNumber, CancellationToken cancellationToken = default);

    /// <summary>
    /// Sets power-cal step / drive level for the selected band (CMD_SET_BAND_POWER_POWER 0xA2).
    /// Value is 0–100 (original Proficio_Calibrate_Power_hScrollBar).
    /// </summary>
    Task SetBandPowerPowerAsync(int percent, CancellationToken cancellationToken = default);

    /// <summary>
    /// Power-cal TX on/off (CMD_CALIBRATION_TUNE 0xAC). true=1, false=0.
    /// </summary>
    Task SetCalibrationTuneAsync(bool on, CancellationToken cancellationToken = default);

    /// <summary>
    /// Amplifier cal: select band path (CMD_SET_AMPLIFIER_INITIALIZE 0xF9). Band = 160, 80, … 10.
    /// </summary>
    Task SetAmplifierInitializeAsync(int bandNumber, CancellationToken cancellationToken = default);

    /// <summary>
    /// Amplifier cal power (CMD_SET_AMPLIFIER_POWER 0xFA). Original band select sends 100.
    /// </summary>
    Task SetAmplifierPowerAsync(int power, CancellationToken cancellationToken = default);

    /// <summary>
    /// Amplifier manual cal step (CMD_SET_POTENTIA_CALIBRATION 0x08).
    /// Original SendCommand32: int32 payload, typically −99…0 (slider).
    /// </summary>
    Task SetPotentiaCalibrationAsync(int calibrationValue, CancellationToken cancellationToken = default);

    /// <summary>
    /// Fired when ms-sdr reports the calibration step for the selected power-cal band (0xB4).
    /// </summary>
    event Action<int> BandPowerReported;

    /// <summary>
    /// Sets the current tuned frequency (in Hz).
    /// </summary>
    Task SetFrequencyAsync(long frequencyHz, CancellationToken cancellationToken = default);

    /// <summary>
    /// Sets the current RF output power (0-100%).
    /// </summary>
    Task SetRfPowerAsync(int percent, CancellationToken cancellationToken = default);

    /// <summary>
    /// Sets the current operating mode.
    /// </summary>
    Task SetModeAsync(string mode, CancellationToken cancellationToken = default);

    /// <summary>
    /// Panadapter resolution: 800, 1600, or 3200 bins across 72 kHz (Normal / High / Max).
    /// Sent as CMD_GET_SET_PANADAPTER_REFRESH (0x5F) index 0/1/2.
    /// </summary>
    Task SetPanResolutionAsync(int bins, CancellationToken cancellationToken = default);

    /// <summary>
    /// Sets filter low cut (Hz).
    /// </summary>
    Task SetFilterLowAsync(int lowHz, CancellationToken cancellationToken = default);

    /// <summary>
    /// Sets filter high cut (Hz).
    /// </summary>
    Task SetFilterHighAsync(int highHz, CancellationToken cancellationToken = default);

    /// <summary>
    /// Sets RIT on/off and offset.
    /// </summary>
    Task SetRitAsync(bool on, long offsetHz, CancellationToken cancellationToken = default);

    /// <summary>
    /// Sets CW pitch. For the CW tab pitch list, this is the INDEX (0-3) to match original behavior.
    /// (Actual Hz mapping done in ms-sdr; full pitch feature deferred.)
    /// </summary>
    Task SetCwPitchAsync(int pitch, CancellationToken cancellationToken = default);

    /// <summary>
    /// Sets the CW filter bandwidth index (0=1.8k,1=400,2=200) - from CW_Filter_button on main.
    /// </summary>
    Task SetCwFilterAsync(int index, CancellationToken cancellationToken = default);

    // CW tab specific (from original CW tab)
    Task SetCwWpmAsync(int wpm, CancellationToken cancellationToken = default);
    Task SetCwKeyerModeAsync(int mode, CancellationToken cancellationToken = default);
    Task SetCwSpacingAsync(int spacing, CancellationToken cancellationToken = default);
    Task SetCwPaddleAsync(int paddle, CancellationToken cancellationToken = default);
    Task SetCwWeightAsync(int weight, CancellationToken cancellationToken = default);
    Task SetCwTxHoldAsync(int holdMs, CancellationToken cancellationToken = default);
    Task SetCwQskAsync(bool on, CancellationToken cancellationToken = default);
    Task SetCwPhonesAsync(bool phones, CancellationToken cancellationToken = default);

    /// <summary>Select keyer CQ memory slot 0..3 (sticky on PIC). CMD_SET_KEYER_MEMORY 0x9C.</summary>
    Task KeyerMemorySelectAsync(int slot, CancellationToken cancellationToken = default);
    /// <summary>Select slot then play once. Paddle aborts on radio.</summary>
    Task KeyerMemoryPlayAsync(int slot, CancellationToken cancellationToken = default);
    /// <summary>Store text to slot (SELECT + BEGIN + chars + END). Max 48 printable ASCII.</summary>
    Task KeyerMemoryStoreAsync(int slot, string text, CancellationToken cancellationToken = default);
    /// <summary>Optional Farnsworth text WPM for memory play (0x76). 0=off; 5–60.</summary>
    Task SetKeyerMemTextWpmAsync(int textWpm, CancellationToken cancellationToken = default);

    /// <summary>
    /// Sets the tuning step index (0=100kHz ... 5=1Hz) from Step button on main tab.
    /// </summary>
    Task SetStepAsync(int index, CancellationToken cancellationToken = default);

    // TX / RxTx tab specific
    Task SetMainPowerAsync(int percent, CancellationToken cancellationToken = default);
    Task SetTunePowerAsync(int percent, CancellationToken cancellationToken = default);
    Task SetCwPowerAsync(int percent, CancellationToken cancellationToken = default);
    Task SetSsbPowerAsync(int percent, CancellationToken cancellationToken = default);
    Task SetAmCarrierAsync(int percent, CancellationToken cancellationToken = default);
    Task SetFullPowerAsync(bool full, CancellationToken cancellationToken = default);

    // Bidirectional reports for Rx/Tx power sliders (Tune, CW, SSB, AM)
    event Action<int> TunePowerReported;
    event Action<int> CwPowerReported;
    event Action<int> SsbPowerReported;
    event Action<int> AmCarrierReported;
    /// <summary>ALC button on/off via CMD_SET_ALC_MULTIPLIER (0x23): 1=on, 0=off.</summary>
    Task SetAlcOnAsync(bool on, CancellationToken cancellationToken = default);
    Task SetAutoTuneAsync(bool on, CancellationToken cancellationToken = default);
    Task SetQrpModeAsync(bool qrp, CancellationToken cancellationToken = default);
    Task SetTransmitAsync(bool on, CancellationToken cancellationToken = default);
    Task SetPaBypassAsync(bool on, CancellationToken cancellationToken = default);

    /// <summary>
    /// Fired when ms-sdr reports PA / AMP path via CMD_SET_PA_BYPASS (0xF7).
    /// true = AMP/QRO (payload QRO_MODE=1), false = QRP (payload QRP_MODE=0).
    /// Pushed at startup and whenever the server changes the state.
    /// </summary>
    event Action<bool> PaBypassReported;

    /// <summary>
    /// Fired when server takes/releases control of TX state via 0xBC.
    /// true = server controls TX (disable user PTT/TUN), false = user control enabled.
    /// </summary>
    event Action<bool> TxSetByServerReported;
    Task SetTxBandwidthAsync(int index, CancellationToken cancellationToken = default);

    /// <summary>Noise blanker on/off (CMD_GET_SET_NB_ENABLE 0x80). Bidirectional.</summary>
    Task SetNbOnAsync(bool on, CancellationToken cancellationToken = default);
    event Action<bool> NbEnableReported;

    /// <summary>NB pulse width µs (CMD_GET_SET_NB_PULSE_WIDTH 0x81). Int32 payload. Bidirectional.</summary>
    Task SetNbPulseWidthAsync(int pulseWidthUs, CancellationToken cancellationToken = default);
    event Action<int> NbPulseWidthReported;

    /// <summary>NB threshold (CMD_GET_SET_NB_THRESHOLD 0x82). Int32 payload. Bidirectional.</summary>
    Task SetNbThresholdAsync(int threshold, CancellationToken cancellationToken = default);
    event Action<int> NbThresholdReported;

    /// <summary>
    /// Noise reduction via CMD_SET_NR (0xA3).
    /// ON: send current NR slider level (non-zero). OFF: send 0.
    /// </summary>
    Task SetNrOnAsync(bool on, int levelWhenOn, CancellationToken cancellationToken = default);

    /// <summary>
    /// Send NR level while NR is on (CMD_SET_NR 0xA3). Use 0 only to force off.
    /// </summary>
    Task SetNrLevelAsync(int level, CancellationToken cancellationToken = default);

    /// <summary>
    /// Appliance → client: CMD_SET_NR (0xA3). Payload is NR_VALUE:
    /// 0 = off; non-zero = on with that level.
    /// </summary>
    event Action<int> NrValueReported;

    /// <summary>Auto notch on/off (CMD_GET_SET_AUTO_NOTCH 0x8E). Bidirectional. Enable only — no level.</summary>
    Task SetAutoNotchOnAsync(bool on, CancellationToken cancellationToken = default);
    event Action<bool> AutoNotchReported;

    // Default filters (Rx/Tx tab)
    Task SetDefaultLowCutAsync(int index, CancellationToken cancellationToken = default);
    Task SetDefaultTxAsync(int index, CancellationToken cancellationToken = default);
    Task SetDefaultCwFilterAsync(int index, CancellationToken cancellationToken = default);
    Task SetDefaultHighCutAsync(int index, CancellationToken cancellationToken = default);

    // AGC controls
    Task SetAgcLevelAsync(int level, CancellationToken cancellationToken = default);
    Task SetAgcFastReleaseAsync(int ms, CancellationToken cancellationToken = default);

    // Aud/Sys tab - Audio detailed (pre-gains, attns, compression, monitor)
    Task SetMicPreGainAsync(int index, CancellationToken cancellationToken = default); // 0-5 phone
    Task SetDigitalMicPreGainAsync(int index, CancellationToken cancellationToken = default); // 0-5 digital
    Task SetVolumeAttnAsync(int index, CancellationToken cancellationToken = default); // 0-5 phone attn
    Task SetDigitalVolumeAttnAsync(int index, CancellationToken cancellationToken = default); // 0-5 digital attn
    Task SetSpeakerVolumeAsync(int value, CancellationToken cancellationToken = default); // 0-100 main speaker volume

    // Frequency Calibration (manual / auto / check)
    Task SetForceCalibrationAsync(bool on, CancellationToken cancellationToken = default);
    Task SetCalSetCoarseAsync(int value, CancellationToken cancellationToken = default);
    Task SetCalSetFineAsync(int value, CancellationToken cancellationToken = default);
    Task SetCalLooseAsync(bool loose, CancellationToken cancellationToken = default);
    Task SetCalResetAsync(bool reset, CancellationToken cancellationToken = default);
    Task SetCalCheckAsync(bool check, CancellationToken cancellationToken = default);
    /// <summary>0 = coarse (±250 Hz / 5 Hz), 1 = fine (±50 Hz / 1 Hz). Must be sent before StartCalibrate.</summary>
    Task SetCalModeAsync(int mode, CancellationToken cancellationToken = default);
    /// <summary>Start auto Si5351 calibration sweep (server uses current tune frequency).</summary>
    Task StartCalibrateAsync(int frequencyHz = 0, CancellationToken cancellationToken = default);
    Task SetCalibrationFinishedAsync(bool accept, CancellationToken cancellationToken = default);

    event Action<int> CalProgressReported;
    event Action<int> CalStatusReported; // 1 = success, 0 = fail for check etc.
    event Action<int> CalDeltaReported; // frequency offset in Hz reported after check/cal
    Task SetMicVolumeAsync(int value, CancellationToken cancellationToken = default); // 0-100 main mic volume

    Task SetPhonesVolumeLevelAsync(int level, CancellationToken cancellationToken = default); // 0-100 phones
    Task SetPhonesMicGainLevelAsync(int level, CancellationToken cancellationToken = default); // 0-100 phones
    Task SetDigitalVolumeLevelAsync(int level, CancellationToken cancellationToken = default); // 0-100 digital
    Task SetDigitalMicGainLevelAsync(int level, CancellationToken cancellationToken = default); // 0-100 digital
    Task SetAudioDeviceAsync(byte device, CancellationToken cancellationToken = default); // 0=Digital, 1=Phones, 2=Remote

    event Action<int> SpeakerVolumeReported;
    event Action<int> MicVolumeReported;

    event Action<int> PhonesVolumeLevelReported;
    event Action<int> PhonesMicGainLevelReported;
    event Action<int> DigitalVolumeLevelReported;
    event Action<int> DigitalMicGainLevelReported;
    event Action<byte> AudioDeviceReported;

    event Action<string> BandReported;
    Task SetCompressionStateAsync(bool on, CancellationToken cancellationToken = default);
    Task SetCompressionLevelAsync(int level, CancellationToken cancellationToken = default); // 0-24
    Task SetMonitorAsync(bool on, CancellationToken cancellationToken = default);

    /// <summary>
    /// Sets the audio mode: false = P (phones/operator audio), true = D (digital audio).
    /// Corresponds to Audio_Digital_button in original.
    /// </summary>
    Task SetAudioDigitalModeAsync(bool isDigital, CancellationToken cancellationToken = default);

    // System
    Task SetTransverterAsync(bool on, CancellationToken cancellationToken = default);
    Task SetTimeDisplayAsync(bool on, CancellationToken cancellationToken = default);
    Task SetBetaTestAsync(bool on, CancellationToken cancellationToken = default);

    // Reports for Aud/Sys
    event Action<int> MicPreGainReported;
    event Action<int> DigitalMicPreGainReported;
    event Action<int> VolumeAttnReported;
    event Action<int> DigitalVolumeAttnReported;
    event Action<bool> CompressionStateReported;
    event Action<int> CompressionLevelReported;
    event Action<bool> MonitorReported;
    event Action<bool> TransverterReported;
    event Action<bool> AudioDigitalModeReported;  // for Audio_Digital toggle feedback

    // CW tab startup reports (bidirectional)
    event Action<int> CwKeyerModeReported;
    event Action<int> CwSpacingReported;
    event Action<int> CwPaddleReported;
    event Action<int> CwWeightReported;
    event Action<int> CwWpmReported;
    event Action<int> CwTxHoldReported;

    event Action<double> ProficioTempReported; // °C
    event Action<double> AmpTempReported;
    event Action<int> AmpCurrentReported; // mA
    /// <summary>Proficio/radio firmware (CMD_GET_SET_FIRMWARE_VERSION 0xB2) → UI FW:</summary>
    event Action<string> FirmwareVersionReported;
    /// <summary>ms-sdr core version (CMD_GET_SET_MSSDR_VERSION 0xB3) → UI Core:</summary>
    event Action<string> CoreVersionReported;
    event Action<int> AlcReported;  // ALC meter value

    // ----- TX I/Q balance calibration (original IQ_Controls; manual only) -----
    /// <summary>Select IQ cal band path (CMD_SET_IQ_BAND 0x58). Band meters: 2200, 630, 160…10.</summary>
    Task SetIqBandAsync(int bandNumber, CancellationToken cancellationToken = default);

    /// <summary>Enter/leave TX IQ cal mode (IQ_CALIBRATION_RX_TX 0x55, payload TX_IQBD=1).</summary>
    Task SetIqCalibrationRxTxAsync(bool txIq, CancellationToken cancellationToken = default);

    /// <summary>IQ cal tune carrier (IQ_CALIBRATION_TUNE 0x54).</summary>
    Task SetIqCalibrationTuneAsync(bool on, CancellationToken cancellationToken = default);

    /// <summary>TX IQ offset (CMD_SET_IQ_OFFSET 0x52). Int32, typically −200…+200.</summary>
    Task SetIqOffsetAsync(int offset, CancellationToken cancellationToken = default);

    /// <summary>Commit current IQ value (CMD_SET_COMMIT_IQ 0x57).</summary>
    Task CommitIqAsync(CancellationToken cancellationToken = default);

    /// <summary>
    /// Reset all IQ bands (CMD_SET_IQ_ALL_BANDS 0x8D, payload 1).
    /// Prefaces with IQ_CALIBRATION_RX_TX (0x55): RX_IQBD if <paramref name="rxIq"/>, else TX_IQBD (original).
    /// </summary>
    Task ResetAllIqBandsAsync(bool rxIq = false, CancellationToken cancellationToken = default);

    /// <summary>IQ operation complete (0x56): 1=success, 0=fail.</summary>
    event Action<int> IqOperationCompleteReported;

    /// <summary>Server IQ slider value (0x8B CMD_GET_IQ_VALUE).</summary>
    event Action<int> IqValueReported;
}
