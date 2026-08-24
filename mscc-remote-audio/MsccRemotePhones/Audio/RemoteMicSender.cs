using System.Net;
using System.Net.Sockets;
using NAudio.CoreAudioApi;
using NAudio.Wave;
using MsccRemotePhones.Protocol;

namespace MsccRemotePhones.Audio;

/// <summary>
/// Captures local microphone and sends MSA1 UDP packets to the Pi (sdrcore-trans remote-mic).
/// TX host may be an IPv4 address or a DNS hostname (IPv4 A record).
/// </summary>
public sealed class RemoteMicSender : IDisposable
{
    private WaveInEvent? _waveIn;
    private UdpClient? _udp;
    private IPEndPoint? _ep;
    private readonly object _gate = new();
    private byte[] _packet = Array.Empty<byte>();
    private int _packetSamples; // mono samples collected toward next packet
    private ushort _seq;
    private float _volume = 0.8f;
    private int _sampleRate = MsccAudioProtocol.DefaultSampleRate;
    private long _packetsSent;
    private long _samplesSent;

    public const int DefaultTxPort = 9101;
    public const int FramesPerPacket = 480; // 10 ms @ 48 kHz

    public event Action<string>? Log;

    public bool IsRunning { get; private set; }
    public long PacketsSent => Interlocked.Read(ref _packetsSent);
    public long SamplesSent => Interlocked.Read(ref _samplesSent);
    public string? DeviceName { get; private set; }

    public float Volume
    {
        get => _volume;
        set => _volume = Math.Clamp(value, 0f, 1f);
    }

    public static IReadOnlyList<(int Index, string Name)> ListCaptureDevices()
    {
        var list = new List<(int, string)> { (-1, "Default Windows recording device") };
        try
        {
            for (int i = 0; i < WaveIn.DeviceCount; i++)
            {
                var caps = WaveIn.GetCapabilities(i);
                list.Add((i, caps.ProductName));
            }
        }
        catch
        {
            try
            {
                var enumr = new MMDeviceEnumerator();
                var devices = enumr.EnumerateAudioEndPoints(DataFlow.Capture, DeviceState.Active);
                int i = 0;
                foreach (var d in devices)
                {
                    list.Add((i, d.FriendlyName));
                    i++;
                }
            }
            catch { /* empty list beyond default */ }
        }
        return list;
    }

    public void Start(string host, int port, int deviceIndex, int sampleRate = MsccAudioProtocol.DefaultSampleRate)
    {
        Stop();
        if (string.IsNullOrWhiteSpace(host))
            throw new ArgumentException("Host is required.", nameof(host));
        if (port is < 1 or > 65535)
            throw new ArgumentOutOfRangeException(nameof(port));

        _sampleRate = sampleRate > 0 ? sampleRate : MsccAudioProtocol.DefaultSampleRate;
        _seq = 0;
        Interlocked.Exchange(ref _packetsSent, 0);
        Interlocked.Exchange(ref _samplesSent, 0);
        _packetSamples = 0;
        int payloadBytes = FramesPerPacket * sizeof(short); // mono
        _packet = new byte[MsccAudioProtocol.HeaderSize + payloadBytes];

        _udp = new UdpClient();
        var addr = ResolveHost(host.Trim());
        _ep = new IPEndPoint(addr, port);

        int waveDev = deviceIndex < 0 ? 0 : deviceIndex;
        // WaveInEvent: DeviceNumber -1 is not always valid; 0 = first device, use 0 for default map
        if (deviceIndex < 0)
            waveDev = 0;

        Exception? last = null;
        // Prefer mono 48 kHz; fall back to stereo then downmix
        foreach (var ch in new[] { 1, 2 })
        {
            try
            {
                var wi = new WaveInEvent
                {
                    DeviceNumber = waveDev,
                    WaveFormat = new WaveFormat(_sampleRate, 16, ch),
                    BufferMilliseconds = 10,
                    NumberOfBuffers = 3,
                };
                wi.DataAvailable += OnDataAvailable;
                wi.RecordingStopped += (_, e) =>
                {
                    if (e.Exception is not null)
                        Log?.Invoke("Mic stopped: " + e.Exception.Message);
                };
                wi.StartRecording();
                _waveIn = wi;
                DeviceName = WaveIn.GetCapabilities(waveDev).ProductName + (ch == 1 ? " (mono)" : " (stereo→mono)");
                last = null;
                break;
            }
            catch (Exception ex)
            {
                last = ex;
                _waveIn?.Dispose();
                _waveIn = null;
            }
        }

        if (_waveIn is null)
        {
            _udp.Dispose();
            _udp = null;
            throw last ?? new InvalidOperationException("Could not open microphone.");
        }

        IsRunning = true;
        var resolved = _ep.Address.ToString();
        var hostNote = string.Equals(host.Trim(), resolved, StringComparison.OrdinalIgnoreCase)
            ? resolved
            : $"{host.Trim()} ({resolved})";
        Log?.Invoke($"Mic TX → {hostNote}:{port} MSA1 {_sampleRate} Hz mono, {FramesPerPacket} frames/pkt, device={DeviceName}");
    }

    /// <summary>
    /// Accepts IPv4 dotted-quad or DNS hostname. Prefers IPv4 (Pi / mscc path).
    /// </summary>
    public static IPAddress ResolveHost(string host)
    {
        if (string.IsNullOrWhiteSpace(host))
            throw new ArgumentException("Host is required.", nameof(host));

        host = host.Trim();
        if (IPAddress.TryParse(host, out var parsed))
        {
            if (parsed.AddressFamily == AddressFamily.InterNetwork)
                return parsed;
            throw new ArgumentException(
                $"Host must be IPv4 or a hostname that resolves to IPv4 (got {parsed.AddressFamily}).",
                nameof(host));
        }

        IPAddress[] addrs;
        try
        {
            addrs = Dns.GetHostAddresses(host);
        }
        catch (Exception ex)
        {
            throw new ArgumentException($"Could not resolve host '{host}': {ex.Message}", nameof(host), ex);
        }

        var ipv4 = addrs.FirstOrDefault(a => a.AddressFamily == AddressFamily.InterNetwork);
        if (ipv4 is null)
            throw new ArgumentException($"Host '{host}' has no IPv4 address.", nameof(host));
        return ipv4;
    }

    private void OnDataAvailable(object? sender, WaveInEventArgs e)
    {
        if (!IsRunning || _udp is null || _ep is null || e.BytesRecorded <= 0)
            return;

        int bytesPerFrame = _waveIn!.WaveFormat.Channels * 2;
        int frames = e.BytesRecorded / bytesPerFrame;
        if (frames <= 0)
            return;

        float vol = _volume;
        int srcCh = _waveIn.WaveFormat.Channels;
        int payloadOff = MsccAudioProtocol.HeaderSize;
        int payloadCap = FramesPerPacket * 2;

        lock (_gate)
        {
            int bi = 0;
            for (int f = 0; f < frames; f++)
            {
                short mono;
                if (srcCh == 1)
                {
                    mono = (short)(e.Buffer[bi] | (e.Buffer[bi + 1] << 8));
                    bi += 2;
                }
                else
                {
                    short l = (short)(e.Buffer[bi] | (e.Buffer[bi + 1] << 8));
                    short r = (short)(e.Buffer[bi + 2] | (e.Buffer[bi + 3] << 8));
                    bi += 4;
                    mono = (short)((l + r) / 2);
                }

                if (vol < 0.999f)
                {
                    if (vol <= 0.001f)
                        mono = 0;
                    else
                    {
                        int v = (int)(mono * vol);
                        if (v > short.MaxValue) v = short.MaxValue;
                        if (v < short.MinValue) v = short.MinValue;
                        mono = (short)v;
                    }
                }

                int o = payloadOff + _packetSamples * 2;
                _packet[o] = (byte)(mono & 0xFF);
                _packet[o + 1] = (byte)((mono >> 8) & 0xFF);
                _packetSamples++;
                Interlocked.Increment(ref _samplesSent);

                if (_packetSamples >= FramesPerPacket)
                {
                    var hdr = new AudioPacketHeader
                    {
                        Sequence = _seq++,
                        FrameCount = (ushort)FramesPerPacket,
                        Channels = 1,
                        Format = MsccAudioProtocol.FormatS16Le,
                        SampleRate = (uint)_sampleRate,
                        Reserved = 0,
                    };
                    MsccAudioProtocol.WriteHeader(_packet.AsSpan(0, MsccAudioProtocol.HeaderSize), hdr);
                    try
                    {
                        _udp.Send(_packet, MsccAudioProtocol.HeaderSize + payloadCap, _ep);
                        Interlocked.Increment(ref _packetsSent);
                    }
                    catch (Exception ex)
                    {
                        Log?.Invoke("Mic send error: " + ex.Message);
                    }
                    _packetSamples = 0;
                }
            }
        }
    }

    public void Stop()
    {
        IsRunning = false;
        try
        {
            if (_waveIn is not null)
            {
                _waveIn.DataAvailable -= OnDataAvailable;
                _waveIn.StopRecording();
            }
        }
        catch { /* ignore */ }
        try { _waveIn?.Dispose(); } catch { /* ignore */ }
        _waveIn = null;
        try { _udp?.Dispose(); } catch { /* ignore */ }
        _udp = null;
        _ep = null;
        DeviceName = null;
        _packetSamples = 0;
        Log?.Invoke("Mic TX stopped");
    }

    public void Dispose() => Stop();
}
