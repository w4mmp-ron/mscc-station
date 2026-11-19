using System.Net;
using System.Net.Sockets;
using System.Linq;
using System.Runtime.InteropServices;
using MSCC.Core.Protocol;

namespace MSCC.Avalonia.RemoteAudio;

public sealed class LinuxMicSender : IDisposable
{
    public const int FramesPerPacket = 480;
    public const int TxSilenceDumpMilliseconds = 400;
    public const int TxLevelRampSamples = 240; // 5 ms @ 48 kHz, linear

    private IntPtr _stream;
    private Thread? _thread;
    private volatile bool _run;
    private UdpClient? _udp;
    private IPEndPoint? _ep;
    private float _volume = 1.0f;
    private int _inCh = 1;
    private ushort _seq;
    private volatile bool _digitalPath;
    private int _keyed;

    public event Action<string>? Log;

    /// <summary>Remote Digital only. Phones sends the mic as captured.</summary>
    public bool DigitalPath
    {
        get => _digitalPath;
        set => _digitalPath = value;
    }

    /// <summary>Keyed = PTT or TUNE. A repeat true does not restart the 400 ms dump.</summary>
    public void SetTxGuard(bool keyed) => Interlocked.Exchange(ref _keyed, keyed ? 1 : 0);
    public bool IsRunning => _run;
    public float Volume
    {
        get => _volume;
        set => _volume = Math.Clamp(value, 0f, 1f);
    }

    public static IReadOnlyList<(int Index, string Name, string HostApi, int InCh, int OutCh)> ListCaptureDevices()
    {
        var list = new List<(int, string, string, int, int)> { (-1, "Default capture", "", 0, 0) };
        try
        {
            PortAudioNative.AddRef();
            int n = PortAudioNative.Pa_GetDeviceCount();
            for (int i = 0; i < n; i++)
            {
                var info = PortAudioNative.Info(i);
                if (info is { maxInputChannels: > 0 } inf)
                    list.Add((i, PortAudioNative.DeviceName(i), PortAudioNative.HostApiName(inf.hostApi),
                        inf.maxInputChannels, inf.maxOutputChannels));
            }
        }
        catch (Exception ex)
        {
            list.Add((-2, "PortAudio: " + ex.Message, "", 0, 0));
        }
        return list;
    }

    public void Start(string host, int port, int deviceIndex, int sampleRate = MsccAudioProtocol.DefaultSampleRate)
    {
        Stop();
        if (string.IsNullOrWhiteSpace(host))
            throw new ArgumentException("Host is required.", nameof(host));
        var addr = RemoteMicHost.Resolve(host);
        _ep = new IPEndPoint(addr, port);
        _udp = new UdpClient();
        _seq = 0;
        PortAudioNative.AddRef();
        int dev = deviceIndex < 0 ? PortAudioNative.Pa_GetDefaultInputDevice() : deviceIndex;
        if (dev < 0)
            throw new InvalidOperationException("No PortAudio capture device.");
        var info = PortAudioNative.Info(dev) ?? throw new InvalidOperationException("Bad capture device.");
        _inCh = info.maxInputChannels >= 1 ? 1 : 0;
        if (_inCh < 1)
            throw new InvalidOperationException("Capture device has no input channels.");
        // Prefer mono; stereo if needed
        int ch = info.maxInputChannels >= 1 ? Math.Min(2, info.maxInputChannels) : 1;
        _inCh = ch;
        var sp = new PortAudioNative.StreamParameters
        {
            device = dev,
            channelCount = ch,
            sampleFormat = PortAudioNative.Float32,
            suggestedLatency = info.defaultLowInputLatency,
            hostApiSpecificStreamInfo = IntPtr.Zero,
        };
        IntPtr pIn = Marshal.AllocHGlobal(Marshal.SizeOf<PortAudioNative.StreamParameters>());
        try
        {
            Marshal.StructureToPtr(sp, pIn, false);
            int err = PortAudioNative.Pa_OpenStream(out _stream, pIn, IntPtr.Zero, sampleRate, FramesPerPacket,
                0, IntPtr.Zero, IntPtr.Zero);
            if (err != PortAudioNative.NoError)
                throw new InvalidOperationException("Pa_OpenStream capture: " + PortAudioNative.Err(err));
        }
        finally
        {
            Marshal.FreeHGlobal(pIn);
        }
        int e = PortAudioNative.Pa_StartStream(_stream);
        if (e != PortAudioNative.NoError)
        {
            PortAudioNative.Pa_CloseStream(_stream);
            _stream = IntPtr.Zero;
            PortAudioNative.Release();
            throw new InvalidOperationException("Pa_StartStream capture: " + PortAudioNative.Err(e));
        }
        _run = true;
        int rate = sampleRate;
        _thread = new Thread(() => CaptureLoop(rate, ch))
        {
            IsBackground = true,
            Name = "MsccPaMic",
            Priority = ThreadPriority.AboveNormal,
        };
        _thread.Start();
        Log?.Invoke($"Mic TX → {addr}:{port} MSA1 {sampleRate} Hz from {PortAudioNative.DeviceName(dev)}");
    }

    private void CaptureLoop(int sampleRate, int ch)
    {
        var f = new float[FramesPerPacket * ch];
        var packet = new byte[MsccAudioProtocol.HeaderSize + FramesPerPacket * 2];
        int dumpN = Math.Max(1, sampleRate * TxSilenceDumpMilliseconds / 1000);
        int rampN = Math.Max(1, sampleRate * TxLevelRampSamples / 48000);
        int dumpLeft = 0;
        int fadeLeft = 0;
        int fadeDir = 0; // +1 fade in, -1 fade out
        float fadeStart = 0f;
        float liveGain = 0f;
        bool capDigital = false;
        bool capKeyed = false;

        while (_run && _stream != IntPtr.Zero && _udp != null && _ep != null)
        {
            int err = PortAudioNative.Pa_ReadStream(_stream, f, FramesPerPacket);
            if (err != PortAudioNative.NoError)
            {
                if (_run)
                    Log?.Invoke("Pa_ReadStream: " + PortAudioNative.Err(err));
                continue;
            }
            bool digital = _digitalPath;
            bool keyed = Volatile.Read(ref _keyed) != 0;
            if (!digital)
            {
                if (capDigital)
                {
                    capDigital = false;
                    capKeyed = false;
                    dumpLeft = 0;
                    fadeLeft = 0;
                    fadeDir = 0;
                    liveGain = 1f;
                }
            }
            else
            {
                if (!capDigital)
                {
                    capDigital = true;
                    capKeyed = false;
                }
                if (keyed != capKeyed)
                {
                    if (keyed)
                    {
                        dumpLeft = dumpN;
                        fadeLeft = 0;
                        fadeDir = 0;
                        liveGain = 0f;
                    }
                    else
                    {
                        dumpLeft = 0;
                        if (liveGain <= 0.0001f)
                        {
                            fadeLeft = 0;
                            fadeDir = 0;
                            liveGain = 0f;
                        }
                        else
                        {
                            fadeStart = liveGain;
                            fadeLeft = rampN;
                            fadeDir = -1;
                        }
                    }
                    capKeyed = keyed;
                }
            }

            float vol = _volume;
            int off = MsccAudioProtocol.HeaderSize;
            for (int i = 0; i < FramesPerPacket; i++)
            {
                float raw;
                if (ch == 1)
                    raw = f[i];
                else
                {
                    float l = f[i * 2];
                    float r = f[i * 2 + 1];
                    raw = Math.Abs(l) >= Math.Abs(r) ? l : r;
                }

                float guard = 1f;
                if (digital)
                {
                    if (dumpLeft > 0)
                    {
                        guard = 0f;
                        dumpLeft--;
                        if (dumpLeft == 0 && keyed)
                        {
                            fadeLeft = rampN;
                            fadeDir = 1;
                        }
                    }
                    else if (fadeLeft > 0 && fadeDir > 0)
                    {
                        int step = rampN - fadeLeft;
                        guard = (step + 1) / (float)rampN;
                        fadeLeft--;
                    }
                    else if (fadeLeft > 0 && fadeDir < 0)
                    {
                        int step = rampN - fadeLeft;
                        guard = fadeStart * (1f - (step + 1) / (float)rampN);
                        fadeLeft--;
                    }
                    else if (!keyed)
                        guard = 0f;
                    liveGain = guard;
                }

                float s = raw * vol * guard;
                int v = (int)(s * 32767f);
                if (v > short.MaxValue) v = short.MaxValue;
                if (v < short.MinValue) v = short.MinValue;
                packet[off++] = (byte)(v & 0xFF);
                packet[off++] = (byte)((v >> 8) & 0xFF);
            }
            var hdr = new AudioPacketHeader
            {
                Sequence = _seq++,
                FrameCount = FramesPerPacket,
                Channels = 1,
                Format = MsccAudioProtocol.FormatS16Le,
                SampleRate = (uint)sampleRate,
                Reserved = 0,
            };
            MsccAudioProtocol.WriteHeader(packet.AsSpan(0, MsccAudioProtocol.HeaderSize), hdr);
            try { _udp.Send(packet, packet.Length, _ep); }
            catch (Exception ex) { Log?.Invoke("Mic send: " + ex.Message); }
        }
    }

    public void Stop()
    {
        _run = false;
        try { _thread?.Join(400); } catch { /* ignore */ }
        _thread = null;
        if (_stream != IntPtr.Zero)
        {
            try { PortAudioNative.Pa_StopStream(_stream); } catch { /* ignore */ }
            try { PortAudioNative.Pa_CloseStream(_stream); } catch { /* ignore */ }
            _stream = IntPtr.Zero;
            PortAudioNative.Release();
        }
        try { _udp?.Dispose(); } catch { /* ignore */ }
        _udp = null;
    }

    public void Dispose() => Stop();
}

internal static class RemoteMicHost
{
    public static IPAddress Resolve(string host)
    {
        host = host.Trim();
        if (IPAddress.TryParse(host, out var parsed))
        {
            if (parsed.AddressFamily == AddressFamily.InterNetwork)
                return parsed;
            throw new ArgumentException("Host must be IPv4.");
        }
        var addrs = Dns.GetHostAddresses(host);
        return addrs.FirstOrDefault(a => a.AddressFamily == AddressFamily.InterNetwork)
               ?? throw new ArgumentException("No IPv4 for host " + host);
    }
}
