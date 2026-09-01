using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Timers;
using MSCC.Core.Display;
using MSCC.Core.Logging;
using MSCC.Core.Protocol;

namespace MSCC.Core.Services;

/// <summary>
/// Real implementation of IRadioService that communicates with the backend servers over UDP.
/// </summary>
public class UdpRadioService : IRadioService, IDisposable
{
    private UdpRadioTransport _transport;
    private readonly string _remoteIp;
    private readonly int _remotePort;
    private readonly int _localPort;
    private bool _started;

    /// <summary>
    /// True if this client instance launched backends (Launch Servers on at Start).
    /// When false (connect-only), Stop() must NOT send CMD_SET_STOP — remote/shared ms-sdr
    /// may stay up for other clients or a host logon task. (ms-sdr may later use a session
    /// counter; client gate is ownership of the launch.)
    /// </summary>
    private bool _launchedSubsystemsThisSession;

    // Multi-segment panadapter (0xD5): each segment is up to 400 bins; Normal=2, High=4, Max=8.
    private const int PanSegmentBins = 400;
    private const int PanMaxSegments = 8;
    private readonly ushort[][] _panSegBuffers = new ushort[PanMaxSegments][];
    private readonly int[] _panSegCounts = new int[PanMaxSegments];
    private readonly bool[] _panSegReady = new bool[PanMaxSegments];
    private int _panSegmentsExpected = 2;
    private int _panBinsExpected = 800;
    private int _panFrameId;

    // Set to true (temporarily) when diving deep into spectrum/waterfall details to re-enable the high-volume
    // per-frame D4 (smeter) + D5 (panadapter) packet logs that we suppressed for normal use.
    private const bool VerboseSpectrumLogging = false;

    // Subsystem processes launched by the UI (matching original Start_subsystem behavior).
    // The launch logic determines the folder from where *this* app is running (dynamic, like original).
    private Process? _msSdr;
    private Process? _sdrcoreRecv;
    private Process? _sdrcoreTrans;

    // Periodic "I'm Alive" / keep-alive to tell ms-sdr the UI is still running (original Master_State_Machine + Manage_Keep_Alive).
    // Sent as CMD_SET_KEEP_ALIVE (0xF4) with short value 1, roughly every second.
    private System.Timers.Timer? _keepAliveTimer;

    /// <summary>UTC of last received 0xF4 from server (or Start time for initial grace).</summary>
    private DateTime _lastKeepAliveReceivedUtc;

    /// <summary>
    /// Watchdog must not fire before this UTC time (cold-start grace after spawning backends).
    /// Prevents false "keep-alive lost" while ms-sdr / USB / audio are still initializing.
    /// </summary>
    private DateTime _keepAliveWatchdogArmedUtc;

    /// <summary>True after ServerKeepAliveLost fired until ResetKeepAliveWatch or Stop.</summary>
    private bool _keepAliveLostLatched;

    /// <summary>
    /// No server I'm-Alive for this long → ServerKeepAliveLost.
    /// Client sends every 1s; 10s ≈ several missed replies without being too twitchy.
    /// </summary>
    private const int KeepAliveReceiveTimeoutMs = 10000;

    /// <summary>After Process.Start of backends, wait before opening UDP / sending 0xFE (was 600ms — racey).</summary>
    private const int SubsystemLaunchSettleMs = 2000;

    /// <summary>Extra keep-alive watchdog grace after a launch-and-connect start (ms-sdr can be slow on first USB open).</summary>
    private const int KeepAliveColdStartGraceMs = 20000;

    /// <summary>How long to wait for orphaned backend processes to exit before re-launch / adopt.</summary>
    private const int OrphanBackendWaitMs = 6000;

    /// <summary>How long to wait for ms-sdr to exit after STOP (was 2000 — often too short → double-start next time).</summary>
    private const int MsSdrStopExitWaitMs = 6000;

    public event Action<SpectrumUpdate>? SpectrumUpdated;
    public event Action<RadioPacketReceivedEventArgs>? PacketReceived;

    public event Action<long>? FrequencyReported;
    public event Action<string>? ModeReported;
    public event Action<int>? SmeterReported;
    public event Action<int>? PowerReported;

    // Bidirectional power levels for Rx/Tx tab (received at startup)
    public event Action<int>? TunePowerReported;
    public event Action<int>? CwPowerReported;
    public event Action<int>? SsbPowerReported;
    public event Action<int>? AmCarrierReported;

    public event Action<int>? DefaultLowCutIndexReported;
    public event Action<int>? DefaultTxIndexReported;
    public event Action<int>? DefaultCwFilterIndexReported;
    public event Action<int>? DefaultHighCutIndexReported;

    public event Action<int>? AgcLevelReported;
    public event Action<int>? AgcFastReleaseReported;

    // Aud/Sys reports
    public event Action<int>? MicPreGainReported;
    public event Action<int>? DigitalMicPreGainReported;
    public event Action<int>? VolumeAttnReported;
    public event Action<int>? DigitalVolumeAttnReported;
    public event Action<int>? SpeakerVolumeReported;
    public event Action<int>? MicVolumeReported;

    public event Action<int>? PhonesVolumeLevelReported;
    public event Action<int>? PhonesMicGainLevelReported;
    public event Action<int>? DigitalVolumeLevelReported;
    public event Action<int>? DigitalMicGainLevelReported;
    public event Action<byte>? AudioDeviceReported;

    public event Action<string>? BandReported;
    public event Action<bool>? CompressionStateReported;
    public event Action<int>? CompressionLevelReported;
    public event Action<bool>? MonitorReported;
    public event Action<bool>? TransverterReported;
    public event Action<bool>? AudioDigitalModeReported;

    // CW tab startup reports (bidirectional)
    public event Action<int>? CwKeyerModeReported;
    public event Action<int>? CwSpacingReported;
    public event Action<int>? CwPaddleReported;
    public event Action<int>? CwWeightReported;
    public event Action<int>? CwWpmReported;
    public event Action<int>? CwTxHoldReported;

    public event Action<double>? ProficioTempReported;
    public event Action<double>? AmpTempReported;
    public event Action<int>? AmpCurrentReported;
    public event Action<string>? FirmwareVersionReported;
    public event Action<string>? CoreVersionReported;
    public event Action? ServerKeepAliveLost;
    public event Action<int>? AlcReported;

    // Frequency Calibration reports (progress 0-100, status 1=success/0=fail, delta Hz from check)
    public event Action<int>? CalProgressReported;
    public event Action<int>? CalStatusReported;
    public event Action<int>? CalDeltaReported;

    public event Action<bool>? TxSetByServerReported;
    public event Action<bool>? PaBypassReported;
    public event Action<int>? BandPowerReported;

    // TX I/Q calibration reports
    public event Action<int>? IqOperationCompleteReported;
    public event Action<int>? IqValueReported;

    public bool IsConnected => _started || (_transport?.IsConnected ?? false);

    public UdpRadioService(string remoteIp, int remotePort, int localPort = 0)
    {
        _remoteIp = remoteIp;
        _remotePort = remotePort;
        _localPort = localPort;
        _transport = CreateTransport();
        for (int i = 0; i < PanMaxSegments; i++)
            _panSegBuffers[i] = new ushort[PanSegmentBins];
        SetPanResolutionLocal(800);
    }

    /// <summary>Update client assembly expectations (and clear in-flight segments).</summary>
    private void SetPanResolutionLocal(int bins)
    {
        bins = bins switch
        {
            1600 => 1600,
            3200 => 3200,
            _ => 800
        };
        _panBinsExpected = bins;
        _panSegmentsTarget = bins / PanSegmentBins;
        if (_panSegmentsTarget < 1) _panSegmentsTarget = 1;
        if (_panSegmentsTarget > PanMaxSegments) _panSegmentsTarget = PanMaxSegments;
        // Start expecting the full target; adapt down if server only sends 2 segs (old mscc-recv)
        _panSegmentsExpected = _panSegmentsTarget;
        _panIncompleteStreak = 0;
        for (int i = 0; i < PanMaxSegments; i++)
        {
            _panSegReady[i] = false;
            _panSegCounts[i] = 0;
        }
    }

    public async Task SetPanResolutionAsync(int bins, CancellationToken cancellationToken = default)
    {
        SetPanResolutionLocal(bins);
        if (!_started) return;
        // Index 0/1/2 → SDRcore G_Panadapter_Pixels 800/1600/3200
        short index = bins switch
        {
            1600 => 1,
            3200 => 2,
            _ => 0
        };
        await _transport.SendAsync(Opcodes.CMD_GET_SET_PANADAPTER_REFRESH, index, cancellationToken);
        DebugMonitor.MonitorTextBoxText($" Send pan resolution: {bins} bins (index {index})");
    }

    private UdpRadioTransport CreateTransport()
    {
        var t = new UdpRadioTransport(_remoteIp, _remotePort, _localPort);
        t.PacketReceived += OnPacketReceived;
        return t;
    }

    /// <summary>
    /// After Stop(), transport is disposed. Always allocate a fresh UDP transport
    /// for each Start so Start → Stop → Start works (COM / server restart without app exit).
    /// </summary>
    private void EnsureTransportForStart()
    {
        try
        {
            if (_transport != null)
            {
                _transport.PacketReceived -= OnPacketReceived;
                _transport.Dispose();
            }
        }
        catch { /* ignore */ }
        _transport = CreateTransport();
    }

    public async Task StartAsync(bool launchSubsystems = true, CancellationToken cancellationToken = default)
    {
        if (_started) return;

        EnsureTransportForStart();

        // Optionally spawn co-located backends (same as original Start_subsystem).
        // When false, assume ms-sdr / recv / trans are already running (e.g. Start-MsccServers.bat).
        _launchedSubsystemsThisSession = false;
        if (launchSubsystems)
        {
            // Previous session often leaves ms-sdr alive past our short STOP wait → port conflict / flaky start.
            await WaitForOrphanBackendsToExitAsync(cancellationToken);
            LaunchSubsystems();
            _launchedSubsystemsThisSession = true;
            // Original WinForms slept ~3s around subsystem start; 600ms was too short on some boots.
            DebugMonitor.MonitorTextBoxText(
                $" UdpRadioService: waiting {SubsystemLaunchSettleMs}ms for backends to open UDP/USB...");
            await Task.Delay(SubsystemLaunchSettleMs, cancellationToken);
        }
        else
        {
            DebugMonitor.MonitorTextBoxText(
                " UdpRadioService Start: launchSubsystems=false (connect only; backends not started by client; will not send STOP on close)");
        }

        await _transport.StartAsync(cancellationToken);
        _started = true;

        // Grace: don't declare keep-alive lost until timeout after Start (server may take a moment)
        _lastKeepAliveReceivedUtc = DateTime.UtcNow;
        _keepAliveLostLatched = false;
        // Cold start (we spawned backends): arm watchdog later so slow ms-sdr/USB init is not a false alarm.
        // Connect-only: normal 10s window from now.
        _keepAliveWatchdogArmedUtc = launchSubsystems
            ? DateTime.UtcNow.AddMilliseconds(KeepAliveColdStartGraceMs)
            : DateTime.UtcNow;
        if (launchSubsystems)
        {
            DebugMonitor.MonitorTextBoxText(
                $" Keep-alive watchdog armed after {KeepAliveColdStartGraceMs}ms cold-start grace (still sending 0xF4)");
        }

        DebugMonitor.MonitorTextBoxText(" UdpRadioService transport started, IsConnected=" + _transport.IsConnected);

        DebugMonitor.MonitorTextBoxText(" UdpRadioService started, GUI_RUNNING sent");

        // Send CMD_CHECK_GUI_STATUS (0xFE) with data=1 to indicate GUI is initialized/ready.
        // ms-sdr responds with versions, status, startup band (CMD_GET_SET_STARTUP_BAND), etc.
        await _transport.SendAsync(Opcodes.CMD_CHECK_GUI_STATUS, (short)1, cancellationToken);
        DebugMonitor.MonitorTextBoxText(" Sent CMD_CHECK_GUI_STATUS (0xFE) with 1 (GUI ready)");

        StartKeepAliveTimer();
    }

    public void ResetKeepAliveWatch()
    {
        _lastKeepAliveReceivedUtc = DateTime.UtcNow;
        _keepAliveLostLatched = false;
        // Continue immediately uses normal timeout (user already waited).
        _keepAliveWatchdogArmedUtc = DateTime.UtcNow;
        DebugMonitor.MonitorTextBoxText(" Keep-alive watch reset (user chose Continue)");
    }

    public void Stop()
    {
        DebugMonitor.MonitorTextBoxText(
            $" UdpRadioService.Stop called. _started={_started}, launchedSubsystems={_launchedSubsystemsThisSession}, transport.IsConnected={_transport?.IsConnected ?? false}");

        // Only send CMD_SET_STOP if THIS client launched the backends (Launch Servers was on at Start).
        // Connect-only clients (remote GUI or local with Launch Servers off) must leave ms-sdr running
        // for other sessions / host Start-MsccServers. (Future: ms-sdr session counter for multi-client.)
        bool shouldAttemptStopSend =
            _launchedSubsystemsThisSession &&
            (_started || (_transport?.IsConnected ?? false));
        bool stopSent = false;

        if (shouldAttemptStopSend)
        {
            try
            {
                // Send exactly as original: CMD_SET_STOP + short STOP_NORMAL (0) => 2-byte payload 00-00
                // (original SendCommand used short overload for this). ms-sdr Command_Interface reacts to it.
                // Wait for the send to complete so the packet is handed to the OS.
                var sendTask = _transport.SendAsync(Opcodes.CMD_SET_STOP, (short)0 /* STOP_NORMAL */);
                if (!sendTask.Wait(500))
                {
                    DebugMonitor.MonitorTextBoxText(" Stop send timed out waiting for completion");
                }
                else
                {
                    DebugMonitor.MonitorTextBoxText(" Sent CMD_SET_STOP (0xFF) + STOP_NORMAL (client launched backends)");
                    stopSent = true;
                }
            }
            catch (Exception ex) { DebugMonitor.MonitorTextBoxText($" Stop send error: {ex.Message}"); }
        }
        else if (_launchedSubsystemsThisSession)
        {
            DebugMonitor.MonitorTextBoxText(" Stop: skipping STOP send (not started / transport not connected)");
        }
        else
        {
            DebugMonitor.MonitorTextBoxText(
                " Stop: skipping CMD_SET_STOP (connect-only session — Launch Servers was off; leave backends running)");
        }

        if (stopSent)
        {
            // Give ms-sdr time to actually receive + process the UDP stop command in its Command_Interface.
            // We do NOT force-kill the launched child processes — opcode only.
            DebugMonitor.MonitorTextBoxText(" Grace period after STOP send to allow ms-sdr to receive/process it...");
            System.Threading.Thread.Sleep(500);

            // ms-sdr often needs several seconds; short wait caused next Start to race a still-running server.
            WaitForLaunchedProcessToExit(_msSdr, "ms-sdr", MsSdrStopExitWaitMs);
            WaitForLaunchedProcessToExit(_sdrcoreRecv, "sdrcore-recv", 3000);
            WaitForLaunchedProcessToExit(_sdrcoreTrans, "sdrcore-trans", 3000);
        }

        // Clear tracked process refs (we waited above instead of killing).
        _msSdr = null;
        _sdrcoreRecv = null;
        _sdrcoreTrans = null;
        _launchedSubsystemsThisSession = false;

        _transport.Dispose();
        _started = false;

        StopKeepAliveTimer();

        DebugMonitor.MonitorTextBoxText(
            stopSent
                ? " UdpRadioService stopped (STOP sent; waited for children if any, no force kill)"
                : " UdpRadioService stopped (no STOP; connect-only or not connected)");
    }

    private void WaitForLaunchedProcessToExit(Process? p, string name, int timeoutMs)
    {
        if (p == null) return;
        try
        {
            if (!p.HasExited)
            {
                DebugMonitor.MonitorTextBoxText($" Waiting up to {timeoutMs}ms for {name} to exit after STOP...");
                if (p.WaitForExit(timeoutMs))
                    DebugMonitor.MonitorTextBoxText($" {name} exited cleanly after STOP.");
                else
                    DebugMonitor.MonitorTextBoxText($" {name} did not exit within timeout after STOP (no kill; will let it shut down on its own).");
            }
        }
        catch (Exception ex)
        {
            DebugMonitor.MonitorTextBoxText($" Wait for {name} exit error: {ex.Message}");
        }
    }

    private void StartKeepAliveTimer()
    {
        StopKeepAliveTimer();

        _keepAliveTimer = new System.Timers.Timer(1000); // ~1s interval (original state machine + counter >=8 produced similar cadence)
        _keepAliveTimer.Elapsed += async (s, e) =>
        {
            if (!_started || !(_transport?.IsConnected ?? false))
                return;

            try
            {
                await _transport.SendAsync(Opcodes.CMD_SET_KEEP_ALIVE, (short)1);
                // "I'm Alive" logging removed per user request (too noisy at 1 Hz)
            }
            catch (Exception ex)
            {
                DebugMonitor.MonitorTextBoxText($" KeepAlive send error: {ex.Message}");
            }

            // Watchdog: server must answer with 0xF4 within KeepAliveReceiveTimeoutMs
            CheckKeepAliveReceiveTimeout();
        };
        _keepAliveTimer.AutoReset = true;
        _keepAliveTimer.Start();

        // Keep-alive timer started (no per-message logging to avoid "I'm Alive" spam)
    }

    private void CheckKeepAliveReceiveTimeout()
    {
        if (!_started || _keepAliveLostLatched)
            return;

        // Cold-start: still send 0xF4, but do not pop the dialog until backends have had time to come up.
        if (DateTime.UtcNow < _keepAliveWatchdogArmedUtc)
            return;

        double ms = (DateTime.UtcNow - _lastKeepAliveReceivedUtc).TotalMilliseconds;
        if (ms < KeepAliveReceiveTimeoutMs)
            return;

        _keepAliveLostLatched = true;
        DebugMonitor.MonitorTextBoxText(
            $" Keep-alive LOST: no CMD_SET_KEEP_ALIVE from server for {ms:F0} ms (limit {KeepAliveReceiveTimeoutMs} ms)");
        try
        {
            ServerKeepAliveLost?.Invoke();
        }
        catch (Exception ex)
        {
            DebugMonitor.MonitorTextBoxText($" ServerKeepAliveLost handler error: {ex.Message}");
        }
    }

    private void StopKeepAliveTimer()
    {
        if (_keepAliveTimer != null)
        {
            try
            {
                _keepAliveTimer.Stop();
                _keepAliveTimer.Dispose();
            }
            catch { }
            _keepAliveTimer = null;
            DebugMonitor.MonitorTextBoxText(" Keep-alive timer stopped");
        }
    }

    /// <summary>
    /// CMD_SET_VFO (0xF2): VFO_A=0, VFO_B=1. Call before frequency/mode when the user switches VFO.
    /// </summary>
    public async Task SetActiveVfoAsync(byte vfo, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Original SendCommand uses short-sized payload for small values; vfo is 0 or 1.
        short v = (short)(vfo == Opcodes.VFO_B ? Opcodes.VFO_B : Opcodes.VFO_A);
        DebugMonitor.MonitorTextBoxText($" Send CMD_SET_VFO (0xF2): {(v == Opcodes.VFO_B ? "VFO_B" : "VFO_A")} ({v})");
        await _transport.SendAsync(Opcodes.CMD_SET_VFO, v, cancellationToken);
    }

    public async Task SetBandPowerBandAsync(int bandNumber, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Original: oCode.SendCommand(..., CMD_SET_BAND_POWER_BAND, (short)Band) with Band=160,80,...
        short band = (short)bandNumber;
        DebugMonitor.MonitorTextBoxText($" Send CMD_SET_BAND_POWER_BAND (0xA1): {band}");
        await _transport.SendAsync(Opcodes.CMD_SET_BAND_POWER_BAND, band, cancellationToken);
    }

    public async Task SetBandPowerPowerAsync(int percent, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Original Proficio_Calibrate_Power_hScrollBar_Scroll → CMD_SET_BAND_POWER_POWER (0xA2)
        short p = (short)Math.Clamp(percent, 0, 100);
        DebugMonitor.MonitorTextBoxText($" Send CMD_SET_BAND_POWER_POWER (0xA2): {p}");
        await _transport.SendAsync(Opcodes.CMD_SET_BAND_POWER_POWER, p, cancellationToken);
    }

    public async Task SetCalibrationTuneAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Original powertunebutton1: CMD_CALIBRATION_TUNE 1 = TX on, 0 = TX off
        short v = (short)(on ? 1 : 0);
        DebugMonitor.MonitorTextBoxText($" Send CMD_CALIBRATION_TUNE (0xAC): {(on ? "ON" : "OFF")} ({v})");
        await _transport.SendAsync(Opcodes.CMD_CALIBRATION_TUNE, v, cancellationToken);
    }

    public async Task SetAmplifierInitializeAsync(int bandNumber, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Original Calibration_Band_button8: CMD_SET_AMPLIFIER_INITIALIZE (0xF9), (short)Band
        short band = (short)bandNumber;
        DebugMonitor.MonitorTextBoxText($" Send CMD_SET_AMPLIFIER_INITIALIZE (0xF9): {band}");
        await _transport.SendAsync(Opcodes.CMD_SET_AMPLIFIER_INITIALIZE, band, cancellationToken);
    }

    public async Task SetAmplifierPowerAsync(int power, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Original band select: CMD_SET_AMPLIFIER_POWER (0xFA), Power_Value=100
        short p = (short)power;
        DebugMonitor.MonitorTextBoxText($" Send CMD_SET_AMPLIFIER_POWER (0xFA): {p}");
        await _transport.SendAsync(Opcodes.CMD_SET_AMPLIFIER_POWER, p, cancellationToken);
    }

    public async Task SetPotentiaCalibrationAsync(int calibrationValue, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Original PA_hScrollBar_Scroll / PA_Manual_Calibrate: SendCommand32 CMD_SET_POTENTIA_CALIBRATION (0x08)
        int v = calibrationValue;
        DebugMonitor.MonitorTextBoxText($" Send CMD_SET_POTENTIA_CALIBRATION (0x08): {v}");
        await _transport.SendAsync(Opcodes.CMD_SET_POTENTIA_CALIBRATION, v, cancellationToken);
    }

    public async Task SetIqBandAsync(int bandNumber, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        short band = (short)bandNumber;
        DebugMonitor.MonitorTextBoxText($" Send CMD_SET_IQ_BAND (0x58): {band}");
        await _transport.SendAsync(Opcodes.CMD_SET_IQ_BAND, band, cancellationToken);
    }

    public async Task SetIqCalibrationRxTxAsync(bool txIq, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        short v = (short)(txIq ? Opcodes.TX_IQBD : Opcodes.RX_IQBD);
        DebugMonitor.MonitorTextBoxText($" Send IQ_CALIBRATION_RX_TX (0x55): {(txIq ? "TX_IQBD" : "RX_IQBD")} ({v})");
        await _transport.SendAsync(Opcodes.IQ_CALIBRATION_RX_TX, v, cancellationToken);
    }

    public async Task SetIqCalibrationTuneAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        short v = (short)(on ? 1 : 0);
        DebugMonitor.MonitorTextBoxText($" Send IQ_CALIBRATION_TUNE (0x54): {(on ? "ON" : "OFF")}");
        await _transport.SendAsync(Opcodes.IQ_CALIBRATION_TUNE, v, cancellationToken);
    }

    public async Task SetIqOffsetAsync(int offset, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        int v = Math.Clamp(offset, -200, 200);
        DebugMonitor.MonitorTextBoxText($" Send CMD_SET_IQ_OFFSET (0x52): {v}");
        await _transport.SendAsync(Opcodes.CMD_SET_IQ_OFFSET, v, cancellationToken);
    }

    public async Task CommitIqAsync(CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText(" Send CMD_SET_COMMIT_IQ (0x57)");
        await _transport.SendAsync(Opcodes.CMD_SET_COMMIT_IQ, (short)0, cancellationToken);
    }

    public async Task ResetAllIqBandsAsync(bool rxIq = false, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Original TX reset: IQ_CALIBRATION_RX_TX TX then 0x8D.
        // Original RX reset (IQ_Reset_All_button2): IQ_CALIBRATION_RX_TX RX then 0x8D.
        await SetIqCalibrationRxTxAsync(txIq: !rxIq, cancellationToken);
        DebugMonitor.MonitorTextBoxText(
            $" Send CMD_SET_IQ_ALL_BANDS (0x8D): 1 ({(rxIq ? "RX" : "TX")} path)");
        await _transport.SendAsync(Opcodes.CMD_SET_IQ_ALL_BANDS, (short)1, cancellationToken);
    }

    public async Task SetFrequencyAsync(long frequencyHz, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send set freq: {frequencyHz}");
        await _transport.SendAsync(Opcodes.CMD_SET_MAIN_FREQ, (int)frequencyHz, cancellationToken);
    }

    public async Task SetRfPowerAsync(int percent, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // The original protocol often sends power as part of band power commands.
        DebugMonitor.MonitorTextBoxText($" Send RF power: {percent}%");
        await _transport.SendAsync(Opcodes.CMD_SET_BAND_POWER_POWER, (short)percent, cancellationToken);
    }

    public async Task SetModeAsync(string mode, CancellationToken cancellationToken = default)
    {
        if (!_started) return;

        // Numeric mode value per original mainmodebutton2_Click / Power_Controls.Mode and ms-sdr Command_Interface switch(t_opcode_data)
        // 0=AM, 1=LSB, 2=USB, 3=CW, 4=TUNE. DIG-U is a client profile — radio LO is USB (2).
        string m = (mode ?? "").Trim().ToUpperInvariant().Replace('_', '-');
        short modeNum = m switch
        {
            "AM" => 0,
            "LSB" => 1,
            "USB" => 2,
            "DIG-U" or "DIGU" or "DIG" => 2, // same RF as USB
            "CW" => 3,
            "TUNE" => 4,
            _ => 2
        };

        await _transport.SendAsync(Opcodes.CMD_SET_MAIN_MODE, modeNum, cancellationToken);
        DebugMonitor.MonitorTextBoxText($" Send set mode: {mode} (as numeric {modeNum})");
    }

    public async Task SetFilterLowAsync(int lowHz, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        short index = LowCutHzToIndex(lowHz);
        DebugMonitor.MonitorTextBoxText($" Send low cut index: {index} (from {lowHz})");
        await _transport.SendAsync(Opcodes.CMD_SET_BW_LOCUT, index, cancellationToken);
    }

    public async Task SetFilterHighAsync(int highHz, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        short index = HighCutHzToIndex(highHz);
        DebugMonitor.MonitorTextBoxText($" Send high cut index: {index} (from {highHz})");
        await _transport.SendAsync(Opcodes.CMD_SET_BW_HICUT, index, cancellationToken);
    }

    private short LowCutHzToIndex(int hz)
    {
        hz = Math.Abs(hz);
        return hz switch
        {
            500 => 0,
            300 => 1,
            200 => 2,
            100 => 3,
            75 => 4,
            _ => 0
        };
    }

    private short HighCutHzToIndex(int hz)
    {
        return hz switch
        {
            5500 => 0,
            4000 => 1,
            3000 => 2,
            2700 => 3,
            2400 => 4,
            _ => 0
        };
    }

    /// <summary>
    /// Converts a mode byte from wire (either numeric 0-6 per ms-sdr CMD_SET_MAIN_MODE switch,
    /// or char 'A'/'L'/'U'/'C' etc. per original GUI receive for report opcodes) to canonical string.
    /// </summary>
    private static string ByteToModeString(byte b)
    {
        if (b <= 6)
        {
            return b switch
            {
                0 => "AM",
                1 => "LSB",
                2 => "USB",
                3 => "CW",
                4 => "TUNE",
                5 => "E",
                6 => "D",
                _ => "USB"
            };
        }
        char c = (char)b;
        return c switch
        {
            'U' => "USB",
            'L' => "LSB",
            'A' => "AM",
            'C' => "CW",
            'T' => "TUNE",
            'E' => "E",
            'D' => "D",
            _ => c.ToString()
        };
    }

    private static string GetBandName(int band)
    {
        return band switch
        {
            2200 => "2200m",
            630 => "630m",
            160 => "160m",
            80 => "80m",
            60 => "60m",
            40 => "40m",
            30 => "30m",
            20 => "20m",
            17 => "17m",
            15 => "15m",
            12 => "12m",
            10 => "10m",
            _ => band + "m"
        };
    }

    public async Task SetRitAsync(bool on, long offsetHz, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_RIT_STATUS, on ? (byte)1 : (byte)0, cancellationToken);
        if (on)
        {
            await _transport.SendAsync(Opcodes.CMD_SET_RIT_FREQ, (int)offsetHz, cancellationToken);
        }
    }

    public async Task SetCwPitchAsync(int pitch, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // pitch here is the INDEX (0-3) when called from CW tab (to match original ms-sdr expectation)
        DebugMonitor.MonitorTextBoxText($" Send CW pitch (index): {pitch}");
        await _transport.SendAsync(Opcodes.CMD_SET_CW_PITCH, (short)pitch, cancellationToken);
    }

    public async Task SetCwFilterAsync(int index, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send CW filter index: {index}");
        await _transport.SendAsync(Opcodes.CMD_SET_CW_BW, (short)index, cancellationToken);
    }

    // CW tab (keyer settings from original CW tab)
    public async Task SetCwWpmAsync(int wpm, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send CW WPM: {wpm}");
        await _transport.SendAsync(Opcodes.CMD_SET_CW_WPM, (short)wpm, cancellationToken);
    }

    public async Task SetCwKeyerModeAsync(int mode, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send CW keyer mode: {mode}");
        await _transport.SendAsync(Opcodes.CMD_SET_CW_KEYER_MODE, (short)mode, cancellationToken);
    }

    public async Task SetCwSpacingAsync(int spacing, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send CW spacing: {spacing}");
        await _transport.SendAsync(Opcodes.CMD_SET_CW_SPACING, (short)spacing, cancellationToken);
    }

    public async Task SetCwPaddleAsync(int paddle, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send CW paddle: {paddle}");
        await _transport.SendAsync(Opcodes.CMD_SET_CW_PADDLE, (short)paddle, cancellationToken);
    }

    public async Task SetCwWeightAsync(int weight, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send CW weight: {weight}");
        await _transport.SendAsync(Opcodes.CMD_SET_CW_WEIGHT, (short)weight, cancellationToken);
    }

    public async Task SetCwTxHoldAsync(int holdMs, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send CW TX hold: {holdMs}");
        await _transport.SendAsync(Opcodes.CMD_SET_CW_TX_HOLD, (short)holdMs, cancellationToken);
    }

    public async Task SetCwQskAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send CW QSK: {on}");
        await _transport.SendAsync(Opcodes.CMD_SET_CW_QSK, (short)(on ? 1 : 0), cancellationToken);
    }

    public async Task SetCwPhonesAsync(bool phones, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send CW Phones: {phones}");
        // Uses SET_CW_MODE for phones flag (0/1); actual audio routing handled via Manage_CW_Phones style in original
        await _transport.SendAsync(Opcodes.CMD_SET_CW_MODE, (short)(phones ? 1 : 0), cancellationToken);
    }

    // Keyer CQ memory (0x9C) — one param per UDP; host paces USB
    private async Task SendKeyerMemParamAsync(int param, CancellationToken cancellationToken)
    {
        if (!_started) return;
        int p = param & 0xFF;
        string label = p switch
        {
            Opcodes.KEYER_MEM_PLAY => "PLAY",
            Opcodes.KEYER_MEM_STORE_BEGIN => "STORE_BEGIN",
            Opcodes.KEYER_MEM_STORE_END => "STORE_END",
            Opcodes.KEYER_MEM_SELECT => "SELECT",
            _ when p is >= 0x20 and <= 0x7E => $"'{((char)p)}'",
            _ => $"param={p}"
        };
        DebugMonitor.MonitorTextBoxText($" Send KEYER_MEM 0x9C {label}");
        await _transport.SendAsync(Opcodes.CMD_SET_KEYER_MEMORY, (short)p, cancellationToken);
    }

    public async Task KeyerMemorySelectAsync(int slot, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        slot = Math.Clamp(slot, 0, Opcodes.KEYER_MEM_SLOT_COUNT - 1);
        DebugMonitor.MonitorTextBoxText($" Keyer memory select slot {slot}");
        await SendKeyerMemParamAsync(Opcodes.KEYER_MEM_SELECT, cancellationToken);
        await SendKeyerMemParamAsync(slot, cancellationToken);
    }

    public async Task KeyerMemoryPlayAsync(int slot, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        slot = Math.Clamp(slot, 0, Opcodes.KEYER_MEM_SLOT_COUNT - 1);
        DebugMonitor.MonitorTextBoxText($" Keyer memory play slot {slot}");
        await KeyerMemorySelectAsync(slot, cancellationToken);
        await SendKeyerMemParamAsync(Opcodes.KEYER_MEM_PLAY, cancellationToken);
    }

    public async Task KeyerMemoryStoreAsync(int slot, string text, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        slot = Math.Clamp(slot, 0, Opcodes.KEYER_MEM_SLOT_COUNT - 1);
        text ??= "";
        if (text.Length > Opcodes.KEYER_MEM_MAX_CHARS)
            text = text[..Opcodes.KEYER_MEM_MAX_CHARS];

        DebugMonitor.MonitorTextBoxText(
            $" Keyer memory store slot {slot} ({text.Length} chars): \"{text}\"");

        await KeyerMemorySelectAsync(slot, cancellationToken);
        await SendKeyerMemParamAsync(Opcodes.KEYER_MEM_STORE_BEGIN, cancellationToken);
        foreach (char ch in text)
        {
            int o = ch;
            if (o is < 0x20 or > 0x7E) continue;
            await SendKeyerMemParamAsync(o, cancellationToken);
        }
        await SendKeyerMemParamAsync(Opcodes.KEYER_MEM_STORE_END, cancellationToken);
        DebugMonitor.MonitorTextBoxText($" Keyer memory store slot {slot} complete");
    }

    public async Task SetKeyerMemTextWpmAsync(int textWpm, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // 0 = off; 1–4 treated as off by host/PIC; clamp upper to 60
        int w = textWpm <= 0 ? 0 : Math.Clamp(textWpm, 0, 60);
        if (w is >= 1 and <= 4) w = 0;
        DebugMonitor.MonitorTextBoxText($" Send SET_MEM_TEXT_WPM 0x76: {w}");
        await _transport.SendAsync(Opcodes.SET_MEM_TEXT_WPM, (short)w, cancellationToken);
    }

    // Frequency Calibration (manual)
    public async Task SetForceCalibrationAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send Force Calibration: {on}");
        await _transport.SendAsync(Opcodes.CMD_SET_FORCE_CALIBRATION, (short)(on ? 1 : 0), cancellationToken);
    }

    public async Task SetCalSetCoarseAsync(int value, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send Cal Set Coarse: {value}");
        await _transport.SendAsync(Opcodes.CMD_SET_CAL_SET_COARSE, (short)value, cancellationToken);
    }

    public async Task SetCalSetFineAsync(int value, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send Cal Set Fine: {value}");
        await _transport.SendAsync(Opcodes.CMD_SET_CAL_SET_FINE, (short)value, cancellationToken);
    }

    public async Task SetCalLooseAsync(bool loose, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send Cal Loose: {loose}");
        await _transport.SendAsync(Opcodes.CMD_SET_CAL_LOOSE, (short)(loose ? 1 : 0), cancellationToken);
    }

    public async Task SetCalResetAsync(bool reset, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send Cal Reset: {reset}");
        await _transport.SendAsync(Opcodes.CMD_SET_CAL_RESET, (short)(reset ? 1 : 0), cancellationToken);
    }

    public async Task SetCalCheckAsync(bool check, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send Cal Check: {check}");
        await _transport.SendAsync(Opcodes.CMD_SET_FREQ_CAL_CHECK, (short)(check ? 1 : 0), cancellationToken);
    }

    public async Task SetCalModeAsync(int mode, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // 0 = coarse, 1 = fine (ms-sdr Process_Frequency_Calibration CMD_SET_CAL_MODE)
        int m = mode != 0 ? 1 : 0;
        DebugMonitor.MonitorTextBoxText($" Send Cal Mode: {(m == 0 ? "COARSE" : "FINE")} ({m})");
        await _transport.SendAsync(Opcodes.CMD_SET_CAL_MODE, (short)m, cancellationToken);
    }

    public async Task StartCalibrateAsync(int frequencyHz = 0, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send Start Calibrate: {frequencyHz} Hz");
        // Original used SendCommand32; server mainly uses its own G_tune_freq for the sweep.
        await _transport.SendAsync(Opcodes.CMD_START_CALIBRATE, frequencyHz, cancellationToken);
    }

    public async Task SetCalibrationFinishedAsync(bool accept, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send Calibration Finished: {accept}");
        await _transport.SendAsync(Opcodes.CMD_SET_CALIBRATION_FINISHED, (short)(accept ? 1 : 0), cancellationToken);
    }

    public async Task SetStepAsync(int index, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send step index: {index}");
        await _transport.SendAsync(Opcodes.CMD_SET_STEP_VALUE, (short)index, cancellationToken);
    }

    // === Rx/Tx tab specific ===
    public async Task SetMainPowerAsync(int percent, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_MAIN_POWER, (short)percent, cancellationToken);
    }

    public async Task SetTunePowerAsync(int percent, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_TUNE_POWER, (short)percent, cancellationToken);
    }

    public async Task SetCwPowerAsync(int percent, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_CW_POWER, (short)percent, cancellationToken);
    }

    public async Task SetSsbPowerAsync(int percent, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // SSB often uses main power in this protocol
        await _transport.SendAsync(Opcodes.CMD_SET_MAIN_POWER, (short)percent, cancellationToken);
    }

    public async Task SetAmCarrierAsync(int percent, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_AM_POWER, (short)percent, cancellationToken);
    }

    public async Task SetFullPowerAsync(bool full, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Full/QRO vs QRP uses CMD_SET_PA_BYPASS (0xF7), not 0xF2 (that is CMD_SET_VFO only).
        // QRO_MODE=1 when full, QRP_MODE=0 when not (matches original Master_Controls).
        short mode = full ? Opcodes.QRO_MODE : Opcodes.QRP_MODE;
        DebugMonitor.MonitorTextBoxText($" Send FullPower via CMD_SET_PA_BYPASS (0xF7): {(full ? "QRO" : "QRP")} ({mode})");
        await _transport.SendAsync(Opcodes.CMD_SET_PA_BYPASS, mode, cancellationToken);
    }

    public async Task SetAlcOnAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Rx/Tx ALC button: CMD_SET_ALC_MULTIPLIER (0x23), 1=on, 0=off (backend interprets)
        short v = (short)(on ? 1 : 0);
        DebugMonitor.MonitorTextBoxText($" Send CMD_SET_ALC_MULTIPLIER (0x23): {(on ? "ON" : "OFF")} ({v})");
        await _transport.SendAsync(Opcodes.CMD_SET_ALC_MULTIPLIER, v, cancellationToken);
    }

    public async Task SetAutoTuneAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_RIG_TUNE, on ? (byte)1 : (byte)0, cancellationToken);
    }

    public async Task SetQrpModeAsync(bool qrp, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Original QRP button → CMD_SET_PA_BYPASS: QRP_MODE=0, QRO_MODE=1
        short mode = qrp ? Opcodes.QRP_MODE : Opcodes.QRO_MODE;
        DebugMonitor.MonitorTextBoxText($" Send QRP via CMD_SET_PA_BYPASS (0xF7): {(qrp ? "QRP" : "QRO")} ({mode})");
        await _transport.SendAsync(Opcodes.CMD_SET_PA_BYPASS, mode, cancellationToken);
    }

    public async Task SetTransmitAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Always short LE (same framing as other GUI params). Opcode 0xBA → ms-sdr Set_PTT → USB 0x50 TX_Request.
        // Note: Proficio only applies TX_Request via TX_Main when host mode is NOT CW; in CW, PA is keyed by the keyer line.
        short v = (short)(on ? 1 : 0);
        DebugMonitor.MonitorTextBoxText($" Send CMD_SET_TX_ON (0xBA) PTT={(on ? "ON" : "OFF")} ({v})");
        await _transport.SendAsync(Opcodes.CMD_SET_TX_ON, v, cancellationToken);
    }

    public async Task SetPaBypassAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // on=true → QRO/amp path (1); on=false → QRP (0) — same CMD_SET_PA_BYPASS 0xF7 as original.
        short mode = on ? Opcodes.QRO_MODE : Opcodes.QRP_MODE;
        DebugMonitor.MonitorTextBoxText($" Send PA Bypass (AMP) CMD_SET_PA_BYPASS (0xF7): {(on ? "QRO" : "QRP")} ({mode})");
        await _transport.SendAsync(Opcodes.CMD_SET_PA_BYPASS, mode, cancellationToken);
    }

    public async Task SetTxBandwidthAsync(int index, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_TX_HICUT, (short)index, cancellationToken);
    }

    public event Action<bool>? NbEnableReported;
    public event Action<int>? NbPulseWidthReported;
    public event Action<int>? NbThresholdReported;
    public event Action<int>? NrValueReported;
    public event Action<bool>? AutoNotchReported;

    public async Task SetNbOnAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Original NB_Controls.NB_ENABLE (0x80): short 1=on, 0=off
        short v = (short)(on ? 1 : 0);
        DebugMonitor.MonitorTextBoxText($" Send CMD_GET_SET_NB_ENABLE (0x80): {(on ? "ON" : "OFF")} ({v})");
        await _transport.SendAsync(Opcodes.CMD_GET_SET_NB_ENABLE, v, cancellationToken);
    }

    public async Task SetNbPulseWidthAsync(int pulseWidthUs, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Original SendCommand32 NB_PULSE_WIDTH (0x81)
        int v = pulseWidthUs;
        DebugMonitor.MonitorTextBoxText($" Send CMD_GET_SET_NB_PULSE_WIDTH (0x81): {v} uS");
        await _transport.SendAsync(Opcodes.CMD_GET_SET_NB_PULSE_WIDTH, v, cancellationToken);
    }

    public async Task SetNbThresholdAsync(int threshold, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Original SendCommand32 NB_THRESHOLD (0x82)
        int v = threshold;
        DebugMonitor.MonitorTextBoxText($" Send CMD_GET_SET_NB_THRESHOLD (0x82): {v}");
        await _transport.SendAsync(Opcodes.CMD_GET_SET_NB_THRESHOLD, v, cancellationToken);
    }

    public async Task SetNrOnAsync(bool on, int levelWhenOn, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // CMD_SET_NR (0xA3): 0 = OFF; non-zero = ON with that level (SDRcore-recv / ms-sdr).
        int payload = on ? Math.Clamp(levelWhenOn, 0, 100) : 0;
        if (on && payload == 0)
            payload = 1; // ON with zeroed slider would look like OFF; use minimum 1
        DebugMonitor.MonitorTextBoxText($" Send CMD_SET_NR (0xA3): {(on ? $"ON level={payload}" : "OFF (0)")}");
        await _transport.SendAsync(Opcodes.CMD_SET_NR, (short)payload, cancellationToken);
    }

    public async Task SetNrLevelAsync(int level, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        int payload = Math.Clamp(level, 0, 100);
        DebugMonitor.MonitorTextBoxText($" Send CMD_SET_NR (0xA3) level: {payload}");
        await _transport.SendAsync(Opcodes.CMD_SET_NR, (short)payload, cancellationToken);
    }

    public async Task SetAutoNotchOnAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        // Ron/Build: lit AN → payload 1; dim AN → payload 0. Never invert.
        // Legacy guiCode.SendCommand(short): [opcode][lo][hi] little-endian.
        const short OnPayload = 1;
        const short OffPayload = 0;
        short v = on ? OnPayload : OffPayload;
        DebugMonitor.MonitorTextBoxText(
            $" Send 0x8E AUTO_NOTCH: payload={v} (AnOn={(on ? "true/lit" : "false/dim")})");
        await _transport.SendAsync(Opcodes.CMD_GET_SET_AUTO_NOTCH, v, cancellationToken);
    }

    // === Default filters (Rx/Tx tab) ===
    public async Task SetDefaultLowCutAsync(int index, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_BW_LOCUT_DEFAULT, (short)index, cancellationToken);
    }

    public async Task SetDefaultTxAsync(int index, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_TX_HICUT, (short)index, cancellationToken);
    }

    public async Task SetDefaultCwFilterAsync(int index, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_CW_BW_DEFAULT, (short)index, cancellationToken);
    }

    public async Task SetDefaultHighCutAsync(int index, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_BW_HICUT_DEFAULT, (short)index, cancellationToken);
    }

    // AGC
    public async Task SetAgcLevelAsync(int level, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_GET_SET_AGC, (byte)level, cancellationToken);
    }

    public async Task SetAgcFastReleaseAsync(int ms, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_AGC_FAST_LEVEL, ms, cancellationToken);
    }

    // === Aud/Sys Audio ===
    public async Task SetMicPreGainAsync(int index, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_MIC_GAIN, (short)index, cancellationToken);
    }

    public async Task SetDigitalMicPreGainAsync(int index, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_DIGITAL_MIC_GAIN, (short)index, cancellationToken);
    }

    public async Task SetVolumeAttnAsync(int index, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_VOLUME_ATTN, (short)index, cancellationToken);
    }

    public async Task SetDigitalVolumeAttnAsync(int index, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_DIGITAL_VOLUME_ATTN, (short)index, cancellationToken);
    }

    public async Task SetSpeakerVolumeAsync(int value, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send speaker volume: {value}");
        await _transport.SendAsync(Opcodes.CMD_SET_SPEAKER_VOLUME, (short)value, cancellationToken);
    }

    public async Task SetMicVolumeAsync(int value, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        DebugMonitor.MonitorTextBoxText($" Send mic volume: {value}");
        await _transport.SendAsync(Opcodes.CMD_SET_MIC_VOLUME, (short)value, cancellationToken);
    }

    public async Task SetCompressionStateAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_COMPRESSION_STATE, on ? (byte)1 : (byte)0, cancellationToken);
    }

    public async Task SetCompressionLevelAsync(int level, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_COMPRESSION_LEVEL, (short)level, cancellationToken);
    }

    public async Task SetMonitorAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_MONITOR, on ? (byte)1 : (byte)0, cancellationToken);
    }

    public async Task SetAudioDigitalModeAsync(bool isDigital, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        byte mode = isDigital ? (byte)0 : (byte)1; // 0=DIGITAL_AUDIO, 1=OPERATOR_AUDIO
        DebugMonitor.MonitorTextBoxText($" Send audio digital mode: {(isDigital ? "D" : "P")}");
        await _transport.SendAsync(Opcodes.CMD_SET_CONFIGURATION, mode, cancellationToken);
    }

    public async Task SetPhonesVolumeLevelAsync(int level, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_PHONES_VOLUME_LEVEL, new byte[] { (byte)level }, cancellationToken);
        DebugMonitor.MonitorTextBoxText($" Send phones volume level: {level}");
    }

    public async Task SetPhonesMicGainLevelAsync(int level, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_PHONES_MIC_GAIN_LEVEL, new byte[] { (byte)level }, cancellationToken);
        DebugMonitor.MonitorTextBoxText($" Send phones mic gain level: {level}");
    }

    public async Task SetDigitalVolumeLevelAsync(int level, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_DIGITAL_VOLUME_LEVEL, new byte[] { (byte)level }, cancellationToken);
        DebugMonitor.MonitorTextBoxText($" Send digital volume level: {level}");
    }

    public async Task SetDigitalMicGainLevelAsync(int level, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_DIGITAL_MIC_GAIN_LEVEL, new byte[] { (byte)level }, cancellationToken);
        DebugMonitor.MonitorTextBoxText($" Send digital mic gain level: {level}");
    }

    public async Task SetAudioDeviceAsync(byte device, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_AUDIO_DEVICE, new byte[] { device }, cancellationToken);
        DebugMonitor.MonitorTextBoxText($" Send audio device: {device} ({(device == Opcodes.DIGITAL_SOUND_DEVICE ? "D" : "P")})");
    }

    public async Task SetTransverterAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await _transport.SendAsync(Opcodes.CMD_SET_TRANSVERTER, on ? (short)1 : (short)0, cancellationToken);
    }

    public async Task SetTimeDisplayAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await Task.CompletedTask; // mostly local display flag
    }

    public async Task SetBetaTestAsync(bool on, CancellationToken cancellationToken = default)
    {
        if (!_started) return;
        await Task.CompletedTask; // client-side beta flag
    }

    #region Subsystem launching (matches original Start_subsystem)

    /// <summary>
    /// Launches the backend subsystem processes from the same folder where *this application* is running.
    /// This exactly replicates the original WinForms Start_subsystem(1) behavior:
    ///   path = AppDomain.CurrentDomain.BaseDirectory;
    ///   then construct full paths for ms-sdr-*.exe, mscc-recv.exe, Mscc-trans.exe etc.
    /// 
    /// The binaries for this project live in C:\mscc-net9. To use them, ensure the built
    /// MSCC.Wpf.exe is run from a folder that contains those binaries (the app discovers
    /// its own running directory at startup, just like the original).
    /// </summary>
    private void LaunchSubsystems()
    {
        // Determine the folder from where the current application (our WPF exe) is running.
        // This is what the original does. The binaries must be in the same folder.
        string baseDir = AppContext.BaseDirectory;
        DebugMonitor.MonitorTextBoxText($" LaunchSubsystems: baseDir={baseDir} (determined from running application)");

        // ms-sdr main binary name varies slightly by hardware variant (MKII, Proficio, etc.).
        // Try common names; first one that exists wins.
        string[] msSdrCandidates = { "ms-sdr-MKII.exe", "ms-sdr-proficio.exe", "ms-sdr.exe" };
        string msSdrPath = msSdrCandidates
            .Select(name => Path.Combine(baseDir, name))
            .FirstOrDefault(File.Exists)
            ?? Path.Combine(baseDir, msSdrCandidates[0]);
        DebugMonitor.MonitorTextBoxText($" LaunchSubsystems: using msSdrPath={msSdrPath}");

        // Robust discovery for recv/trans (original used direct paths; we try likely names present in mscc-net9)
        string[] recvCandidates = { "mscc-recv.exe" };
        string recvPath = recvCandidates
            .Select(name => Path.Combine(baseDir, name))
            .FirstOrDefault(File.Exists)
            ?? Path.Combine(baseDir, recvCandidates[0]);

        string[] transCandidates = { "Mscc-trans.exe", "mscc-trans.exe" };
        string transPath = transCandidates
            .Select(name => Path.Combine(baseDir, name))
            .FirstOrDefault(File.Exists)
            ?? Path.Combine(baseDir, transCandidates[0]);

        DebugMonitor.MonitorTextBoxText($" LaunchSubsystems: recv={recvPath}, trans={transPath}");

        // Start in roughly the order from the reference (trans, recv, ms_sdr).
        // Prefer adopting an already-running instance (orphan from previous STOP timeout) over a second copy.
        if (_sdrcoreTrans == null || _sdrcoreTrans.HasExited)
        {
            _sdrcoreTrans = TryAdoptOrStartProcess(transPath, "Mscc-trans", "mscc-trans", "sdrcore-trans");
            DebugMonitor.MonitorTextBoxText($" LaunchSubsystems: trans ready? {(_sdrcoreTrans != null && !_sdrcoreTrans.HasExited)}");
        }
        if (_sdrcoreRecv == null || _sdrcoreRecv.HasExited)
        {
            _sdrcoreRecv = TryAdoptOrStartProcess(recvPath, "mscc-recv", null, "sdrcore-recv");
            DebugMonitor.MonitorTextBoxText($" LaunchSubsystems: recv ready? {(_sdrcoreRecv != null && !_sdrcoreRecv.HasExited)}");
        }
        if (_msSdr == null || _msSdr.HasExited)
        {
            // Process name is the file name without extension (e.g. ms-sdr-MKII).
            string msName = Path.GetFileNameWithoutExtension(msSdrPath);
            _msSdr = TryAdoptOrStartProcess(msSdrPath, msName, "ms-sdr", "ms-sdr");
            DebugMonitor.MonitorTextBoxText($" LaunchSubsystems: ms_sdr ready? {(_msSdr != null && !_msSdr.HasExited)}");
        }
    }

    /// <summary>
    /// If a prior session left backends running (STOP wait too short), wait for them to exit
    /// so we do not bind UDP / USB twice.
    /// </summary>
    private async Task WaitForOrphanBackendsToExitAsync(CancellationToken cancellationToken)
    {
        string[] processNames = { "ms-sdr-MKII", "ms-sdr-proficio", "ms-sdr", "mscc-recv", "Mscc-trans", "mscc-trans" };
        var deadline = DateTime.UtcNow.AddMilliseconds(OrphanBackendWaitMs);
        bool logged = false;

        while (DateTime.UtcNow < deadline)
        {
            cancellationToken.ThrowIfCancellationRequested();
            var still = new List<string>();
            foreach (var name in processNames)
            {
                try
                {
                    foreach (var p in Process.GetProcessesByName(name))
                    {
                        try
                        {
                            if (!p.HasExited)
                                still.Add($"{name}(pid {p.Id})");
                        }
                        catch { /* access denied / exited */ }
                        finally { p.Dispose(); }
                    }
                }
                catch { /* ignore */ }
            }

            if (still.Count == 0)
            {
                if (logged)
                    DebugMonitor.MonitorTextBoxText(" Prior backend processes have exited — safe to launch.");
                return;
            }

            if (!logged)
            {
                DebugMonitor.MonitorTextBoxText(
                    $" Waiting for prior backends to exit before re-launch: {string.Join(", ", still)}");
                logged = true;
            }

            await Task.Delay(250, cancellationToken);
        }

        DebugMonitor.MonitorTextBoxText(
            " Prior backends still running after wait — will adopt if possible instead of double-start.");
    }

    /// <summary>
    /// Use an existing process with the same image name if present; otherwise start a new one.
    /// Avoids two ms-sdr instances fighting for the radio / UDP ports.
    /// </summary>
    private Process? TryAdoptOrStartProcess(string fileName, string processNamePrimary, string? processNameAlt, string friendlyName)
    {
        var existing = FindRunningProcess(processNamePrimary) ?? (processNameAlt != null ? FindRunningProcess(processNameAlt) : null);
        if (existing != null)
        {
            DebugMonitor.MonitorTextBoxText(
                $" Adopting existing {friendlyName} process pid={existing.Id} (no second instance)");
            return existing;
        }

        return TryStartProcess(fileName, friendlyName);
    }

    private static Process? FindRunningProcess(string processName)
    {
        try
        {
            var list = Process.GetProcessesByName(processName);
            foreach (var p in list)
            {
                try
                {
                    if (!p.HasExited)
                    {
                        // Dispose extras
                        foreach (var other in list)
                        {
                            if (!ReferenceEquals(other, p))
                            {
                                try { other.Dispose(); } catch { }
                            }
                        }
                        return p;
                    }
                }
                catch { }
                try { p.Dispose(); } catch { }
            }
        }
        catch { }
        return null;
    }

    private Process? TryStartProcess(string fileName, string friendlyName)
    {
        if (!File.Exists(fileName))
        {
            DebugMonitor.MonitorTextBoxText($" Subsystem binary not found for {friendlyName}: {fileName}");
            return null;
        }

        var proc = new Process();
        try
        {
            proc.StartInfo.UseShellExecute = false;
            proc.StartInfo.FileName = fileName;
            // Set working directory to the folder containing this binary (so exes can find
            // init-files/, .ini files, logs, etc. relative to their location, exactly as original).
            proc.StartInfo.WorkingDirectory = Path.GetDirectoryName(fileName) ?? AppContext.BaseDirectory;
            proc.StartInfo.CreateNoWindow = true;
            proc.StartInfo.Arguments = "test";
            proc.Start();
            return proc;
        }
        catch (Exception ex)
        {
            DebugMonitor.MonitorTextBoxText($" Failed to start {friendlyName} ({fileName}): {ex.Message}");
            return null;
        }
    }

    #endregion

    private void OnPacketReceived(object? sender, RadioPacketReceivedEventArgs e)
    {
        // Forward raw packets so higher layers (ViewModel, debug tools) can inspect them
        PacketReceived?.Invoke(e);

        // Log raw packet only for non-high-volume continuous updates. These flood the log unless VerboseSpectrumLogging:
        // spectrum D5, smeter D4, keep-alive 0xF4, transceiver temp 0xBF, ALC meter 0x4F.
        if (VerboseSpectrumLogging ||
            (e.Opcode != Opcodes.CMD_GET_SET_PANADAPTER &&
             e.Opcode != Opcodes.CMD_GET_SET_SMETER &&
             e.Opcode != Opcodes.CMD_SET_KEEP_ALIVE &&
             e.Opcode != Opcodes.CMD_GET_TRANSCEIVER_TEMP &&
             e.Opcode != Opcodes.CMD_SET_ALC))
        {
            string payloadHex = e.Payload.Length > 0 ? BitConverter.ToString(e.Payload, 0, Math.Min(16, e.Payload.Length)) + (e.Payload.Length > 16 ? "..." : "") : "";
            string opcodeName = Opcodes.GetName(e.Opcode);
            DebugMonitor.MonitorTextBoxText($" Udp packet received: opcode 0x{e.Opcode:X2} ({opcodeName}) len={e.Payload.Length} payload={payloadHex}");
        }

        // Decode key opcodes from the real backend and raise typed events for UI sync
        switch (e.Opcode)
        {
            case Opcodes.CMD_GET_SET_PANADAPTER:
                DecodePanadapterFrame(e.Payload);
                if (VerboseSpectrumLogging)
                    DebugMonitor.MonitorTextBoxText(" Processed panadapter/spectrum frame (0xD5)");
                break;

            case Opcodes.CMD_SET_MAIN_FREQ:
            case Opcodes.CMD_GET_SET_LAST_USED_FREQ:
            case Opcodes.CMD_GET_FREQ_INIT:
            case Opcodes.CMD_SET_SPECTRUM_WATERFALL_FREQ: // from panadapter contexts
            case Opcodes.CMD_SET_DISPLAY_FREQ: // ms-sdr now sends this for startup frequency: Gui_send_param(CMD_SET_DISPLAY_FREQ, G_tune_freq)
                if (e.Payload.Length >= 4)
                {
                    long freq = BitConverter.ToInt32(e.Payload, 0);
                    FrequencyReported?.Invoke(freq);
                    DebugMonitor.MonitorTextBoxText($" Processed freq report (startup/last/display): {freq}");
                }
                break;

            case Opcodes.CMD_SET_MAIN_MODE:
            case Opcodes.CMD_MODE_SET_BY_SERVER:
            case Opcodes.CMD_GET_SET_LAST_USED_MODE:
            case Opcodes.CMD_GET_MODE_INIT:
            case Opcodes.CMD_SET_SPECTRUM_WATERFALL_MODE:
                if (e.Payload.Length >= 1)
                {
                    byte b = e.Payload[0];
                    string mode = ByteToModeString(b);
                    ModeReported?.Invoke(mode);
                    DebugMonitor.MonitorTextBoxText($" Processed mode report (startup/last): {mode} (raw=0x{b:X2})");
                }
                break;

            case Opcodes.CMD_GET_SET_STARTUP_BAND:
                // The exact opcode ms-sdr sends after our CMD_CHECK_GUI_STATUS (0xFE with data=1) in its
                // CMD_CHECK_GUI_STATUS case when t_opcode_data==1: fprintf ... "Sending Start Up Band %d" then
                // Gui_send_param(CMD_GET_SET_STARTUP_BAND, G_band_normal). This is the one that should set the
                // active band in the UI (per current wind-back scope: only this band opcode).
                if (e.Payload.Length >= 4)
                {
                    int bandNum = BitConverter.ToInt32(e.Payload, 0);
                    string bandName = GetBandName(bandNum);
                    BandReported?.Invoke(bandName);
                    string payloadHex = BitConverter.ToString(e.Payload);
                    DebugMonitor.MonitorTextBoxText($" Processed CMD_GET_SET_STARTUP_BAND (0xF6) report: {bandName} (raw={bandNum}) payload={payloadHex}");
                }
                break;

            case Opcodes.CMD_SET_KEEP_ALIVE:
                // Server I'm Alive (or echo). Original: Master_Controls.Keep_Alive = true.
                _lastKeepAliveReceivedUtc = DateTime.UtcNow;
                // Do not clear _keepAliveLostLatched here — user must Continue after a warning.
                // Logging suppressed (1 Hz would flood the log).
                break;

            case Opcodes.CMD_GET_SET_SMETER:
                if (e.Payload.Length >= 4)
                {
                    int dbm = BitConverter.ToInt32(e.Payload, 0);  // negative dBm value from ms-sdr
                    SmeterReported?.Invoke(dbm);
                    if (VerboseSpectrumLogging)
                        DebugMonitor.MonitorTextBoxText($" Processed smeter report (dBm): {dbm}");
                }
                break;

            case Opcodes.CMD_SET_ALC:
                if (e.Payload.Length >= 4)
                {
                    int alc = BitConverter.ToInt32(e.Payload, 0);
                    // ALC meter value (separate from power)
                    AlcReported?.Invoke(Math.Clamp(alc, 0, 1000)); // scale as needed; original uses for VU
                    // Also treat as power for backward
                    PowerReported?.Invoke(Math.Clamp(alc / 10, 0, 100)); // rough scale
                    // High-rate meter stream — do not log every CMD_SET_ALC (0x4F) report
                }
                break;

            case Opcodes.CMD_GET_AMP_POWER:
            case Opcodes.CMD_GET_AMPLIFIER_POWER:
                if (e.Payload.Length >= 1)
                {
                    int pwr = e.Payload[0];
                    PowerReported?.Invoke(Math.Clamp(pwr, 0, 100));
                }
                break;

            // Rx/Tx power levels (bidirectional - server reports startup values using set opcodes)
            case Opcodes.CMD_SET_TUNE_POWER:
                if (e.Payload.Length >= 2)
                {
                    int val = BitConverter.ToInt16(e.Payload, 0);
                    TunePowerReported?.Invoke(Math.Clamp(val, 0, 100));
                }
                break;
            case Opcodes.CMD_SET_CW_POWER:
                if (e.Payload.Length >= 2)
                {
                    int val = BitConverter.ToInt16(e.Payload, 0);
                    CwPowerReported?.Invoke(Math.Clamp(val, 0, 100));
                }
                break;

            case Opcodes.CMD_SET_CALIBRATIION_PROGRESS: // note spelling from original
                if (e.Payload.Length >= 2)
                {
                    int val = BitConverter.ToInt16(e.Payload, 0);
                    CalProgressReported?.Invoke(val);
                    DebugMonitor.MonitorTextBoxText($" Processed cal progress: {val}");
                }
                break;

            case Opcodes.CMD_SET_CALIBRATION_FINISHED:
                if (e.Payload.Length >= 2)
                {
                    int val = BitConverter.ToInt16(e.Payload, 0);
                    CalStatusReported?.Invoke(val);
                    DebugMonitor.MonitorTextBoxText($" Processed cal finished status: {val}");
                }
                break;

            case Opcodes.CMD_GET_SET_CAL_FREQ_DELTA:
                if (e.Payload.Length >= 4)
                {
                    int val = BitConverter.ToInt32(e.Payload, 0);
                    CalDeltaReported?.Invoke(val);
                    DebugMonitor.MonitorTextBoxText($" Processed cal delta (Hz): {val}");
                }
                break;

            case Opcodes.CMD_SET_MAIN_POWER:
                // SSB power reports via main power opcode (as send does)
                if (e.Payload.Length >= 2)
                {
                    int val = BitConverter.ToInt16(e.Payload, 0);
                    SsbPowerReported?.Invoke(Math.Clamp(val, 0, 100));
                }
                break;
            case Opcodes.CMD_SET_AM_POWER:
                if (e.Payload.Length >= 2)
                {
                    int val = BitConverter.ToInt16(e.Payload, 0);
                    AmCarrierReported?.Invoke(Math.Clamp(val, 0, 100));
                }
                break;

            case Opcodes.CMD_SET_BW_LOCUT_DEFAULT:
                if (e.Payload.Length >= 1)
                {
                    DefaultLowCutIndexReported?.Invoke(e.Payload[0]);
                }
                break;

            case Opcodes.CMD_SET_TX_HICUT:
                if (e.Payload.Length >= 1)
                {
                    DefaultTxIndexReported?.Invoke(e.Payload[0]);
                }
                break;

            case Opcodes.CMD_SET_TX_SET_BY_SERVER:
                if (e.Payload.Length >= 1)
                {
                    byte val = e.Payload[0];
                    TxSetByServerReported?.Invoke(val != 0);
                    DebugMonitor.MonitorTextBoxText($" Processed TX set by server (0xBC): {val} (server control={(val != 0)})");
                }
                break;

            case Opcodes.CMD_SET_PA_BYPASS:
                // Bidirectional AMP/PA path. ms-sdr pushes at startup and on change.
                // QRP_MODE=0 → AMP off; QRO_MODE=1 (or non-zero) → AMP on.
                if (e.Payload.Length >= 1)
                {
                    int val = e.Payload.Length >= 2
                        ? BitConverter.ToInt16(e.Payload, 0)
                        : e.Payload[0];
                    bool ampOn = val != Opcodes.QRP_MODE;
                    PaBypassReported?.Invoke(ampOn);
                    DebugMonitor.MonitorTextBoxText(
                        $" Processed CMD_SET_PA_BYPASS (0xF7): {val} → AmpOn={ampOn}");
                }
                break;

            case Opcodes.CMD_GET_SET_NB_ENABLE:
                // Original NB_Controls.NB_ENABLE: operand 1=on, 0=off
                if (e.Payload.Length >= 1)
                {
                    int val = e.Payload.Length >= 2
                        ? BitConverter.ToInt16(e.Payload, 0)
                        : e.Payload[0];
                    bool on = val != 0;
                    NbEnableReported?.Invoke(on);
                    DebugMonitor.MonitorTextBoxText($" Processed CMD_GET_SET_NB_ENABLE (0x80): {val} → NbOn={on}");
                }
                break;

            case Opcodes.CMD_GET_SET_NB_PULSE_WIDTH:
                // Original: BitConverter.ToInt32(message, 1) → scrollbar
                if (e.Payload.Length >= 4)
                {
                    int us = BitConverter.ToInt32(e.Payload, 0);
                    NbPulseWidthReported?.Invoke(us);
                    DebugMonitor.MonitorTextBoxText($" Processed CMD_GET_SET_NB_PULSE_WIDTH (0x81): {us} uS");
                }
                else if (e.Payload.Length >= 2)
                {
                    int us = BitConverter.ToInt16(e.Payload, 0);
                    NbPulseWidthReported?.Invoke(us);
                    DebugMonitor.MonitorTextBoxText($" Processed CMD_GET_SET_NB_PULSE_WIDTH (0x81): {us} uS (i16)");
                }
                break;

            case Opcodes.CMD_GET_SET_NB_THRESHOLD:
                if (e.Payload.Length >= 4)
                {
                    int thr = BitConverter.ToInt32(e.Payload, 0);
                    NbThresholdReported?.Invoke(thr);
                    DebugMonitor.MonitorTextBoxText($" Processed CMD_GET_SET_NB_THRESHOLD (0x82): {thr}");
                }
                else if (e.Payload.Length >= 2)
                {
                    int thr = BitConverter.ToInt16(e.Payload, 0);
                    NbThresholdReported?.Invoke(thr);
                    DebugMonitor.MonitorTextBoxText($" Processed CMD_GET_SET_NB_THRESHOLD (0x82): {thr} (i16)");
                }
                break;

            case Opcodes.CMD_SET_NR:
                // Bidirectional NR (0xA3): NR_VALUE 0 = off; non-zero = on with that level.
                // ms-sdr pushes this on client connect (User_Controls_Send_To_Gui).
                if (e.Payload.Length >= 1)
                {
                    int val = e.Payload.Length >= 4
                        ? BitConverter.ToInt32(e.Payload, 0)
                        : e.Payload.Length >= 2
                            ? BitConverter.ToInt16(e.Payload, 0)
                            : e.Payload[0];
                    if (val < 0) val = 0;
                    if (val > 100) val = 100;
                    NrValueReported?.Invoke(val);
                    DebugMonitor.MonitorTextBoxText(
                        $" Processed CMD_SET_NR (0xA3): NR_VALUE={val} → NrOn={(val != 0)} level={val}");
                }
                break;

            case Opcodes.CMD_GET_SET_AUTO_NOTCH:
                // Bidirectional AN (0x8E): 0 = off; non-zero = on.
                // ms-sdr pushes this on client connect (User_Controls_Send_To_Gui).
                if (e.Payload.Length >= 1)
                {
                    int val = e.Payload.Length >= 4
                        ? BitConverter.ToInt32(e.Payload, 0)
                        : e.Payload.Length >= 2
                            ? BitConverter.ToInt16(e.Payload, 0)
                            : e.Payload[0];
                    bool on = val != 0;
                    AutoNotchReported?.Invoke(on);
                    DebugMonitor.MonitorTextBoxText(
                        $" Processed CMD_GET_SET_AUTO_NOTCH (0x8E): payload={val} → AnOn={on}");
                }
                break;

            case Opcodes.CMD_GET_BAND_POWER:
                // After CMD_SET_BAND_POWER_BAND (0xA1), ms-sdr replies with calibration step.
                // Original: Power_Calibration_Controls.Received_Power_Value = message[1]
                // (transport already strips opcode → payload[0] is that byte).
                if (e.Payload.Length >= 1)
                {
                    int step = e.Payload[0];
                    BandPowerReported?.Invoke(step);
                    DebugMonitor.MonitorTextBoxText($" Processed CMD_GET_BAND_POWER (0xB4): step={step}");
                }
                break;

            case Opcodes.CMD_SET_CW_BW_DEFAULT:
                if (e.Payload.Length >= 1)
                {
                    DefaultCwFilterIndexReported?.Invoke(e.Payload[0]);
                }
                break;

            case Opcodes.CMD_SET_BW_HICUT_DEFAULT:
                if (e.Payload.Length >= 1)
                {
                    DefaultHighCutIndexReported?.Invoke(e.Payload[0]);
                }
                break;

            case Opcodes.CMD_GET_SET_AGC:
                if (e.Payload.Length >= 1)
                {
                    AgcLevelReported?.Invoke(e.Payload[0]);
                }
                break;

            case Opcodes.CMD_SET_AGC_FAST_LEVEL:
                if (e.Payload.Length >= 4)
                {
                    int ms = BitConverter.ToInt32(e.Payload, 0);
                    AgcFastReleaseReported?.Invoke(ms);
                }
                break;

            // Aud/Sys audio reports (echo or push from server)
            case Opcodes.CMD_SET_MIC_GAIN:
                if (e.Payload.Length >= 1) MicPreGainReported?.Invoke(e.Payload[0]);
                break;
            case Opcodes.CMD_SET_DIGITAL_MIC_GAIN:
                if (e.Payload.Length >= 1) DigitalMicPreGainReported?.Invoke(e.Payload[0]);
                break;
            case Opcodes.CMD_SET_VOLUME_ATTN:
                if (e.Payload.Length >= 1) VolumeAttnReported?.Invoke(e.Payload[0]);
                break;
            case Opcodes.CMD_SET_DIGITAL_VOLUME_ATTN:
                if (e.Payload.Length >= 1) DigitalVolumeAttnReported?.Invoke(e.Payload[0]);
                break;

            case Opcodes.CMD_SET_SPEAKER_VOLUME:
                if (e.Payload.Length >= 2)
                {
                    int vol = BitConverter.ToInt16(e.Payload, 0);
                    SpeakerVolumeReported?.Invoke(vol);
                }
                break;
            case Opcodes.CMD_SET_MIC_VOLUME:
                if (e.Payload.Length >= 2)
                {
                    int vol = BitConverter.ToInt16(e.Payload, 0);
                    MicVolumeReported?.Invoke(vol);
                }
                break;

            case Opcodes.CMD_SET_VOLUME_BY_SERVER:
                if (e.Payload.Length >= 1)
                {
                    SpeakerVolumeReported?.Invoke(e.Payload[0]);
                }
                break;

            case Opcodes.CMD_SET_COMPRESSION_STATE:
                if (e.Payload.Length >= 1) CompressionStateReported?.Invoke(e.Payload[0] != 0);
                break;
            case Opcodes.CMD_SET_COMPRESSION_LEVEL:
                if (e.Payload.Length >= 1) CompressionLevelReported?.Invoke(e.Payload[0]);
                break;
            case Opcodes.CMD_SET_MONITOR:
                if (e.Payload.Length >= 1) MonitorReported?.Invoke(e.Payload[0] != 0);
                break;

            case Opcodes.CMD_SET_CONFIGURATION:
                if (e.Payload.Length >= 1)
                {
                    bool isDigital = e.Payload[0] == 0; // 0=digital, 1=operator
                    AudioDigitalModeReported?.Invoke(isDigital);
                    DebugMonitor.MonitorTextBoxText($" Audio digital mode reported: {(isDigital ? "D" : "P")}");
                }
                break;

            case Opcodes.CMD_SET_PHONES_VOLUME_LEVEL:
                if (e.Payload.Length >= 1)
                {
                    int level = e.Payload[0];
                    PhonesVolumeLevelReported?.Invoke(level);
                }
                break;

            case Opcodes.CMD_SET_PHONES_MIC_GAIN_LEVEL:
                if (e.Payload.Length >= 1)
                {
                    int level = e.Payload[0];
                    PhonesMicGainLevelReported?.Invoke(level);
                }
                break;

            case Opcodes.CMD_SET_DIGITAL_VOLUME_LEVEL:
                if (e.Payload.Length >= 1)
                {
                    int level = e.Payload[0];
                    DigitalVolumeLevelReported?.Invoke(level);
                }
                break;

            case Opcodes.CMD_SET_DIGITAL_MIC_GAIN_LEVEL:
                if (e.Payload.Length >= 1)
                {
                    int level = e.Payload[0];
                    DigitalMicGainLevelReported?.Invoke(level);
                }
                break;

            case Opcodes.CMD_SET_AUDIO_DEVICE:
                if (e.Payload.Length >= 1)
                {
                    byte device = e.Payload[0];
                    AudioDeviceReported?.Invoke(device);
                }
                break;

            case Opcodes.CMD_SET_TRANSVERTER:
                if (e.Payload.Length >= 1) TransverterReported?.Invoke(e.Payload[0] != 0);
                break;

            case Opcodes.CMD_GET_TRANSCEIVER_TEMP:
                if (e.Payload.Length >= 4)
                {
                    int t = BitConverter.ToInt32(e.Payload, 0);
                    ProficioTempReported?.Invoke(t);  // ms-sdr sends literal °C
                }
                break;

            // CMD_RPI_SET_TEMPERATURE (0x12) ignored per user instruction - not valid for this work.

            // Versions — 4-byte int; major = low byte, minor = next (original guiCode style)
            case Opcodes.CMD_GET_SET_FIRMWARE_VERSION: // 0xB2 → UI FW:
                if (e.Payload.Length >= 4)
                {
                    int v = BitConverter.ToInt32(e.Payload, 0);
                    int major = v & 0xFF;
                    int minor = (v >> 8) & 0xFF;
                    string ver = $"{major}.{minor}";
                    FirmwareVersionReported?.Invoke(ver);
                    DebugMonitor.MonitorTextBoxText($" Firmware version (FW): {ver} (raw 0x{v:X8})");
                }
                break;

            case Opcodes.CMD_GET_SET_MSSDR_VERSION: // 0xB3 → UI Core:
                if (e.Payload.Length >= 4)
                {
                    int v = BitConverter.ToInt32(e.Payload, 0);
                    int major = v & 0xFF;
                    int minor = (v >> 8) & 0xFF;
                    string ver = $"{major}.{minor}";
                    CoreVersionReported?.Invoke(ver);
                    DebugMonitor.MonitorTextBoxText($" MSSDR version (Core): {ver} (raw 0x{v:X8})");
                }
                break;

            // Recv/trans version reports removed on server; ignore if ever seen
            case Opcodes.CMD_GET_SET_SDRCORE_RECV_VERSION:
            case Opcodes.CMD_GET_SET_SDRCORE_TRANS_VERSION:
                break;



            case Opcodes.CMD_GET_OPTIONS_STATUS:
                if (e.Payload.Length >= 1)
                {
                    byte status = e.Payload[0];
                    DebugMonitor.MonitorTextBoxText($" Received options/status 0xBE: 0x{status:X2} payload={BitConverter.ToString(e.Payload)}");
                    // TODO: use to set QRP/full mode, options flags etc. per original
                }
                break;

            case Opcodes.CMD_SET_CW_PITCH:
                if (e.Payload.Length >= 1)
                {
                    // payload often index (0..) not Hz; original had validation issues with this
                    int pitchVal = BitConverter.ToInt32(e.Payload, 0);
                    DebugMonitor.MonitorTextBoxText($" Received CW pitch (0xD2): {pitchVal} (0x{pitchVal:X8})");
                    // TODO: wire to CW tab / state if not already sending the other direction
                }
                break;

            case Opcodes.CMD_SET_CW_KEYER_MODE:
                if (e.Payload.Length >= 1)
                {
                    int mode = BitConverter.ToInt16(e.Payload, 0);  // or [0] if byte
                    CwKeyerModeReported?.Invoke(mode);
                }
                break;

            case Opcodes.CMD_SET_CW_SPACING:
                if (e.Payload.Length >= 1)
                {
                    int spacing = BitConverter.ToInt16(e.Payload, 0);
                    CwSpacingReported?.Invoke(spacing);
                }
                break;

            case Opcodes.CMD_SET_CW_PADDLE:
                if (e.Payload.Length >= 1)
                {
                    int paddle = BitConverter.ToInt16(e.Payload, 0);
                    CwPaddleReported?.Invoke(paddle);
                }
                break;

            case Opcodes.CMD_SET_CW_WEIGHT:
                if (e.Payload.Length >= 1)
                {
                    int weight = BitConverter.ToInt16(e.Payload, 0);
                    CwWeightReported?.Invoke(weight);
                }
                break;

            case Opcodes.CMD_SET_CW_WPM:
                if (e.Payload.Length >= 1)
                {
                    int wpm = BitConverter.ToInt16(e.Payload, 0);
                    CwWpmReported?.Invoke(wpm);
                }
                break;

            case Opcodes.CMD_SET_CW_TX_HOLD:
                if (e.Payload.Length >= 1)
                {
                    int hold = BitConverter.ToInt16(e.Payload, 0);
                    CwTxHoldReported?.Invoke(hold);
                }
                break;

            case Opcodes.IQ_OPERATION_COMPLETE:
                {
                    int op = e.Payload.Length >= 2
                        ? BitConverter.ToInt16(e.Payload, 0)
                        : (e.Payload.Length >= 1 ? e.Payload[0] : -1);
                    IqOperationCompleteReported?.Invoke(op);
                    DebugMonitor.MonitorTextBoxText($" Processed IQ_OPERATION_COMPLETE (0x56): {op}");
                }
                break;

            case Opcodes.CMD_GET_IQ_VALUE:
                if (e.Payload.Length >= 4)
                {
                    int iq = BitConverter.ToInt32(e.Payload, 0);
                    IqValueReported?.Invoke(iq);
                    DebugMonitor.MonitorTextBoxText($" Processed CMD_GET_IQ_VALUE (0x8B): {iq}");
                }
                break;

            case Opcodes.CMD_SET_EXTENDED_COMMAND:  // 0x0B wrapper for many sub-reports (power, swr, waterfall params, status, etc.)
                if (e.Payload.Length >= 1)
                {
                    byte sub = e.Payload[0];
                    string subName = GetExtendedSubName(sub);
                    if (VerboseSpectrumLogging)
                    {
                        DebugMonitor.MonitorTextBoxText($" Received extended (0x0B) sub 0x{sub:X2} ({subName}) payload={BitConverter.ToString(e.Payload)}");
                    }
                }
                break;

            // TODO: more opcodes (temps, bias, status, extended forward/reverse power, drift, antenna, etc.)
            default:
                string opcodeName = Opcodes.GetName(e.Opcode);
                DebugMonitor.MonitorTextBoxText($" No handler for opcode 0x{e.Opcode:X2} ({opcodeName}) (nothing available to process it)");
                break;
        }
    }

    private static string GetExtendedSubName(byte sub)
    {
        // Common extended subs from original Extended_Commands (waterfall, solidus/power, etc.)
        return sub switch
        {
            0x00 => "WATERFALL_DISPLAY",
            0x01 => "WATERFALL_GAIN",
            0x02 => "WATERFALL_GRID",
            0x03 => "WATERFALL_ZERO",
            0x04 => "WATERFALL_SPEED",
            0x05 => "WATERFALL_DIRECTION",
            0x06 => "WATERFALL_PALETTE",
            0x09 => "IQBD_MONITOR",
            0x0A => "IQBD_DATA",
            0x0B => "FORWARD_POWER",
            0x0C => "REVERSE_POWER",
            0x0D => "SWR",
            0x0E => "SOLIDUS_STATUS",
            _ => "UNKNOWN_SUB"
        };
    }

    private int _panIncompleteStreak;
    private int _panSegmentsTarget = 2; // user-selected; may differ from what server actually sends

    private void DecodePanadapterFrame(byte[] payload)
    {
        if (payload.Length < 2) return;

        byte seq = payload[0];
        if (seq >= PanMaxSegments) return;

        // Server may send more segs than we asked for — expand assembly window
        int needSeg = seq + 1;
        if (needSeg > _panSegmentsExpected && needSeg <= PanMaxSegments)
        {
            _panSegmentsExpected = needSeg;
            _panBinsExpected = _panSegmentsExpected * PanSegmentBins;
        }

        int dataBytes = payload.Length - 1;
        int numSamples = dataBytes / 2;
        if (numSamples <= 0) return;

        // New frame starts with seq 0. If prior frame was incomplete (e.g. asked for 4 segs
        // but old mscc-recv only sent 2), flush contiguous 0..k so spectrum does not freeze.
        if (seq == 0)
        {
            TryEmitPanFrame(forceIncomplete: true);
            for (int s = 1; s < PanMaxSegments; s++)
            {
                _panSegReady[s] = false;
                _panSegCounts[s] = 0;
            }
        }

        ushort[] target = _panSegBuffers[seq];
        int count = Math.Min(numSamples, target.Length);
        for (int i = 0; i < count; i++)
            target[i] = BitConverter.ToUInt16(payload, 1 + (i * 2));
        _panSegCounts[seq] = count;
        _panSegReady[seq] = true;

        TryEmitPanFrame(forceIncomplete: false);
    }

    /// <summary>
    /// Emit a spectrum frame when all expected segments are ready, or (force) when a new
    /// seq0 arrives with a contiguous incomplete prefix (old server only sends 2 segs).
    /// </summary>
    private void TryEmitPanFrame(bool forceIncomplete)
    {
        int contiguous = 0;
        while (contiguous < PanMaxSegments && contiguous < _panSegmentsExpected && _panSegReady[contiguous])
            contiguous++;

        if (contiguous <= 0)
            return;

        bool complete = contiguous >= _panSegmentsExpected;
        if (!complete)
        {
            // Incomplete: only flush when a new seq0 arrives (force) and we have ≥2 segs.
            // This recovers when old mscc-recv still sends only seq0+seq1 while UI asked for High/Max.
            if (!forceIncomplete || contiguous < 2)
                return;

            _panIncompleteStreak++;
            if (contiguous < _panSegmentsTarget && _panSegmentsExpected != contiguous)
            {
                DebugMonitor.MonitorTextBoxText(
                    $" Pan assembly: server only completed {contiguous} segment(s) " +
                    $"(wanted {_panSegmentsTarget} ×400 = {_panSegmentsTarget * PanSegmentBins} bins). " +
                    $"Using {contiguous * PanSegmentBins} bins — rebuild/redeploy mscc-recv for true High/Max.");
                _panSegmentsExpected = contiguous;
                _panBinsExpected = contiguous * PanSegmentBins;
            }
        }
        else
        {
            _panIncompleteStreak = 0;
            // Recover toward target if server later sends full High/Max frames
            if (contiguous >= _panSegmentsTarget && _panSegmentsExpected < _panSegmentsTarget)
            {
                _panSegmentsExpected = _panSegmentsTarget;
                _panBinsExpected = _panSegmentsExpected * PanSegmentBins;
                DebugMonitor.MonitorTextBoxText(
                    $" Pan assembly: full {_panSegmentsExpected} segments — {_panBinsExpected} bins active");
            }
        }

        int segs = complete ? _panSegmentsExpected : contiguous;
        int total = 0;
        for (int s = 0; s < segs; s++)
            total += _panSegCounts[s];
        if (total <= 0) return;

        float[] data = new float[total];
        float peak = float.MinValue;
        int o = 0;
        for (int s = 0; s < segs; s++)
        {
            int n = _panSegCounts[s];
            var buf = _panSegBuffers[s];
            for (int i = 0; i < n; i++)
            {
                float db = RawYToDb(buf[i]);
                data[o++] = db;
                if (db > peak) peak = db;
            }
            _panSegReady[s] = false;
            _panSegCounts[s] = 0;
        }

        // SpectrumUpdate.SMeter is unused for the face S-meter (that uses radio dBm / 0xD4).
        // Keep a rough fill for any consumer of SpectrumUpdate.SMeter.
        int sMeter = Math.Clamp((int)((peak + 140f) / 10f), 0, 15);
        // Absolute-ish dB after client SPECTRUM_DB_OFFSET. Window fits HDSDR-like floors (~−116)
        // with headroom; top 0 dB for full-scale after cal.
        var update = new SpectrumUpdate
        {
            Data = data,
            CenterFrequencyHz = 7_100_000,
            SpanHz = SpectrumUpdate.DefaultPanadapterSpanHz,
            FilterLowHz = -2400,
            FilterHighHz = 200,
            MinDb = -140,
            MaxDb = 0,
            SMeter = sMeter,
            CwPitchHz = 0
        };
        SpectrumUpdated?.Invoke(update);
        _panFrameId++;
    }

    /// <summary>
    /// Invert sdrcore panadapter Y → relative dB.
    /// Server (dsputils.c): Y = max(0, (10*log10(|FFT|) + bias) * 150), then MAX_Y (16000).
    /// Inverse: relDb = Y/150 − bias. Client SPECTRUM_DB_OFFSET then sets absolute labels.
    /// Bias must match dsputils.c (40). Raising bias alone without MAX_Y would clip peaks;
    /// raising bias with matching client inverse deepens the Y=0 floor without moving peaks
    /// that already had Y&gt;0 (same 10*log10 + offset). Deploy mscc-recv + client together.
    /// </summary>
    private static float RawYToDb(ushort y)
    {
        const float scale = 150f;
        const float bias = 40f; // must match dsputils.c mag += 40
        float relDb = (y / scale) - bias;
        if (relDb < -200f) relDb = -200f;
        if (relDb > 80f) relDb = 80f;
        return relDb;
    }

    public void Dispose()
    {
        Stop();
    }
}
