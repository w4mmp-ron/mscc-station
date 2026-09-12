using MSCC.Core.Protocol;

namespace MSCC.Avalonia.RemoteAudio;

public sealed class RemoteAfEngine : IDisposable
{
    private readonly JitterBuffer _jitter = new();
    private readonly UdpAudioReceiver _rx;
    private readonly LinuxPhonePlayer _player;
    private readonly LinuxMicSender _mic;
    private bool _playerArmed;

    public RemoteAfEngine()
    {
        _rx = new UdpAudioReceiver(_jitter);
        _player = new LinuxPhonePlayer(_jitter);
        _mic = new LinuxMicSender();
        _rx.Log += s => Log?.Invoke(s);
        _player.Log += s => Log?.Invoke(s);
        _mic.Log += s => Log?.Invoke(s);
        _rx.PacketAccepted += hdr =>
        {
            if (_playerArmed) return;
            _playerArmed = true;
            try
            {
                _player.Start((int)hdr.SampleRate, hdr.Channels, PlayDeviceIndex, JitterMs);
            }
            catch (Exception ex)
            {
                _playerArmed = false;
                Log?.Invoke("Play start failed: " + ex.Message);
            }
        };
    }

    public event Action<string>? Log;
    public int PlayDeviceIndex { get; set; } = -1;
    public int MicDeviceIndex { get; set; } = -1;
    public int JitterMs { get; set; } = 80;
    public int RxPort { get; set; } = MsccAudioProtocol.DefaultPort;
    public int TxPort { get; set; } = MsccAudioProtocol.DefaultTxPort;

    public float PlayVolume
    {
        get => _player.Volume;
        set => _player.Volume = value;
    }

    public bool PlayMuted
    {
        get => _player.Muted;
        set => _player.Muted = value;
    }

    public float MicVolume
    {
        get => _mic.Volume;
        set => _mic.Volume = value;
    }

    public PlaybackEq Eq => _player.Eq;
    public bool RxRunning => _rx.IsRunning;
    public bool MicRunning => _mic.IsRunning;
    public string Status =>
        $"RX: {(_rx.IsRunning ? "on" : "off")}  TX: {(_mic.IsRunning ? "on" : "off")}  buf={_player.BufferedMs} ms";

    public static IReadOnlyList<(int Index, string Name)> PlayDevices => LinuxPhonePlayer.ListPlayDevices();
    public static IReadOnlyList<(int Index, string Name)> MicDevices => LinuxMicSender.ListCaptureDevices();

    public void StartRx()
    {
        _playerArmed = false;
        _rx.Start(RxPort);
    }

    public void StartMic(string txHost) => _mic.Start(txHost, TxPort, MicDeviceIndex);

    public void Stop()
    {
        _playerArmed = false;
        try { _rx.Stop(); } catch { /* ignore */ }
        try { _player.Stop(); } catch { /* ignore */ }
        try { _mic.Stop(); } catch { /* ignore */ }
    }

    public void ApplyEq(bool enabled, float lowDb, float midDb, float highDb)
        => _player.Eq.ApplySettings(enabled, lowDb, midDb, highDb);

    public void Dispose() => Stop();
}
