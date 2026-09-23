using System.Net;
using System.Net.Sockets;
using NAudio.CoreAudioApi;
using NAudio.Wave;
using MSCC.Core.Protocol;

namespace MSCC.Wpf.RemoteAudio;

/// <summary>
/// Captures local microphone and sends MSA1 UDP packets to the Pi (sdrcore-trans remote-mic).
/// TX host may be an IPv4 address or a DNS hostname (IPv4 A record).
/// WaveIn only enqueues. A send thread blocks until one 10 ms block is ready, then sends it.
/// </summary>
public sealed class RemoteMicSender : IDisposable
{
    private WaveInEvent? _waveIn;
    private UdpClient? _udp;
    private IPEndPoint? _ep;
    private readonly object _gate = new();
    private readonly short[] _ring = new short[FramesPerPacket * 50]; // 500 ms @ 48 kHz
    private int _ringRead;
    private int _ringWrite;
    private int _ringCount;
    private int _ringDrops;
    private Thread? _sendThread;
    private volatile bool _sendRun;
    private ushort _seq;
    private float _volume = 1.0f;
    private int _sampleRate = MsccAudioProtocol.DefaultSampleRate;
    private long _packetsSent;
    private long _samplesSent;
    private int _peakAbs;

    public const int DefaultTxPort = 9101;
    public const int FramesPerPacket = 480; // 10 ms @ 48 kHz
    public const int CaptureBufferMilliseconds = 10;
    public const int CaptureBufferCount = 8; // 80 ms in the driver so a short stall cannot drop

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
        List<string> wasapiNames = new();
        try
        {
            var enumr = new MMDeviceEnumerator();
            foreach (var d in enumr.EnumerateAudioEndPoints(DataFlow.Capture, DeviceState.Active))
                wasapiNames.Add(d.FriendlyName);
        }
        catch { /* WaveIn names only */ }

        try
        {
            for (int i = 0; i < WaveIn.DeviceCount; i++)
            {
                // WaveIn ProductName is fixed 32 chars, often null-padded — trim or sticky restore breaks.
                string waveName = (WaveIn.GetCapabilities(i).ProductName ?? "").TrimEnd('\0').Trim();
                string display = waveName;
                foreach (string friendly in wasapiNames)
                {
                    if (friendly.StartsWith(waveName, StringComparison.OrdinalIgnoreCase) ||
                        waveName.StartsWith(friendly, StringComparison.OrdinalIgnoreCase) ||
                        friendly.Contains(waveName, StringComparison.OrdinalIgnoreCase))
                    {
                        display = friendly; // full sticky name
                        break;
                    }
                }
                if (string.IsNullOrWhiteSpace(display))
                    display = $"WaveIn #{i}";
                list.Add((i, display));
            }
        }
        catch
        {
            for (int i = 0; i < wasapiNames.Count; i++)
                list.Add((i, wasapiNames[i]));
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
        lock (_gate)
        {
            _ringRead = 0;
            _ringWrite = 0;
            _ringCount = 0;
            _ringDrops = 0;
            _peakAbs = 0;
        }

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
            WaveInEvent? wi = null;
            try
            {
                wi = new WaveInEvent
                {
                    DeviceNumber = waveDev,
                    WaveFormat = new WaveFormat(_sampleRate, 16, ch),
                    BufferMilliseconds = CaptureBufferMilliseconds,
                    NumberOfBuffers = CaptureBufferCount,
                };
                wi.DataAvailable += OnDataAvailable;
                wi.RecordingStopped += (_, e) =>
                {
                    if (e.Exception is not null)
                        Log?.Invoke("Mic stopped: " + e.Exception.Message);
                };
                IsRunning = true;
                wi.StartRecording();
                _waveIn = wi;
                string capName = (WaveIn.GetCapabilities(waveDev).ProductName ?? "").TrimEnd('\0').Trim();
                DeviceName = capName + (ch == 1 ? " (mono)" : " (stereo→mono)");
                last = null;
                break;
            }
            catch (Exception ex)
            {
                last = ex;
                IsRunning = false;
                try { wi?.Dispose(); } catch { /* ignore */ }
                _waveIn = null;
            }
        }

        if (_waveIn is null)
        {
            _udp.Dispose();
            _udp = null;
            throw last ?? new InvalidOperationException("Could not open microphone.");
        }

        _sendRun = true;
        _sendThread = new Thread(SendLoop)
        {
            IsBackground = true,
            Name = "MsccMicTx",
            Priority = ThreadPriority.AboveNormal,
        };
        _sendThread.Start();
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
        if (!IsRunning || e.BytesRecorded <= 0 || _waveIn is null)
            return;

        int bytesPerFrame = _waveIn.WaveFormat.Channels * 2;
        int frames = e.BytesRecorded / bytesPerFrame;
        if (frames <= 0)
            return;

        float vol = _volume;
        int srcCh = _waveIn.WaveFormat.Channels;

        lock (_gate)
        {
            /* A full ring means the send thread is stuck. Drop this callback
             * as one gap instead of blocking WaveIn and losing the driver buffers. */
            if (_ringCount + frames > _ring.Length)
            {
                _ringDrops += frames;
                return;
            }

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
                    /* VAC/WSJT often drives only the left channel. Averaging with
                     * silence halves amplitude (~6 dB, ~4× less RF). */
                    int al = l < 0 ? -l : l;
                    int ar = r < 0 ? -r : r;
                    mono = al >= ar ? l : r;
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

                int abs = mono < 0 ? -mono : (int)mono;
                if (abs > _peakAbs)
                    _peakAbs = abs;

                _ring[_ringWrite] = mono;
                _ringWrite++;
                if (_ringWrite >= _ring.Length)
                    _ringWrite = 0;
                _ringCount++;
                Interlocked.Increment(ref _samplesSent);
            }

            if (_ringCount >= FramesPerPacket)
                Monitor.Pulse(_gate);
        }
    }

    private void SendLoop()
    {
        var packet = new byte[MsccAudioProtocol.HeaderSize + FramesPerPacket * 2];
        var block = new short[FramesPerPacket];
        while (_sendRun)
        {
            lock (_gate)
            {
                while (_sendRun && _ringCount < FramesPerPacket)
                    Monitor.Wait(_gate);
                if (!_sendRun || _ringCount < FramesPerPacket)
                    return;
                for (int i = 0; i < FramesPerPacket; i++)
                {
                    block[i] = _ring[_ringRead];
                    _ringRead++;
                    if (_ringRead >= _ring.Length)
                        _ringRead = 0;
                }
                _ringCount -= FramesPerPacket;
            }

            int payloadOff = MsccAudioProtocol.HeaderSize;
            for (int i = 0; i < FramesPerPacket; i++)
            {
                short mono = block[i];
                packet[payloadOff + i * 2] = (byte)(mono & 0xFF);
                packet[payloadOff + i * 2 + 1] = (byte)((mono >> 8) & 0xFF);
            }

            var udp = _udp;
            var ep = _ep;
            if (udp is null || ep is null)
                return;

            var hdr = new AudioPacketHeader
            {
                Sequence = _seq++,
                FrameCount = (ushort)FramesPerPacket,
                Channels = 1,
                Format = MsccAudioProtocol.FormatS16Le,
                SampleRate = (uint)_sampleRate,
                Reserved = 0,
            };
            MsccAudioProtocol.WriteHeader(packet.AsSpan(0, MsccAudioProtocol.HeaderSize), hdr);
            try
            {
                udp.Send(packet, packet.Length, ep);
                long n = Interlocked.Increment(ref _packetsSent);
                if (n == 1 || n % 500 == 0)
                {
                    int peak;
                    int drops;
                    lock (_gate)
                    {
                        peak = _peakAbs;
                        _peakAbs = 0;
                        drops = _ringDrops;
                        _ringDrops = 0;
                    }
                    Log?.Invoke($"Mic TX {n} pkts → {ep} peak={peak} drops={drops}");
                }
            }
            catch (Exception ex)
            {
                if (_sendRun)
                    Log?.Invoke("Mic send error: " + ex.Message);
            }
        }
    }

    public void Stop()
    {
        IsRunning = false;
        _sendRun = false;
        lock (_gate)
            Monitor.PulseAll(_gate);
        try { _sendThread?.Join(TimeSpan.FromSeconds(2)); } catch { /* ignore */ }
        _sendThread = null;
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
        lock (_gate)
        {
            _ringRead = 0;
            _ringWrite = 0;
            _ringCount = 0;
        }
        Log?.Invoke("Mic TX stopped");
    }

    public void Dispose() => Stop();
}
