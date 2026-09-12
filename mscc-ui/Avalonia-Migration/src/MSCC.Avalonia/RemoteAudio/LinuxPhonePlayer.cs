using System.Runtime.InteropServices;
using MSCC.Core.Protocol;

namespace MSCC.Avalonia.RemoteAudio;

/// <summary>Play jitter-buffer PCM through PortAudio (Pulse/PipeWire).</summary>
public sealed class LinuxPhonePlayer : IDisposable
{
    private readonly JitterBuffer _jitter;
    private IntPtr _stream;
    private Thread? _thread;
    private volatile bool _run;
    private int _channels = 1;
    private int _rate = MsccAudioProtocol.DefaultSampleRate;
    private float _volume = 0.8f;
    public PlaybackEq Eq { get; } = new();
    public bool Muted { get; set; }
    public float Volume
    {
        get => _volume;
        set => _volume = Math.Clamp(value, 0f, 1f);
    }

    public event Action<string>? Log;
    public bool IsRunning => _run;
    public int BufferedMs => _rate <= 0 ? 0 : _jitter.QueuedSamples * 1000 / Math.Max(1, _rate);

    public LinuxPhonePlayer(JitterBuffer jitter) => _jitter = jitter;

    public static IReadOnlyList<(int Index, string Name)> ListPlayDevices()
    {
        var list = new List<(int, string)> { (-1, "Default playback") };
        try
        {
            PortAudioNative.AddRef();
            int n = PortAudioNative.Pa_GetDeviceCount();
            for (int i = 0; i < n; i++)
            {
                var info = PortAudioNative.Info(i);
                if (info is { maxOutputChannels: > 0 })
                    list.Add((i, PortAudioNative.DeviceName(i)));
            }
        }
        catch (Exception ex)
        {
            list.Add((-2, "PortAudio: " + ex.Message));
        }
        return list;
    }

    public void Start(int sampleRate, int channels, int deviceIndex, int jitterMs)
    {
        Stop();
        _rate = sampleRate > 0 ? sampleRate : MsccAudioProtocol.DefaultSampleRate;
        _channels = channels is 1 or 2 ? channels : 1;
        _jitter.PrebufferSamples = _rate * Math.Clamp(jitterMs, 20, 400) / 1000;
        Eq.SetSampleRate(_rate);
        PortAudioNative.AddRef();
        int dev = deviceIndex < 0 ? PortAudioNative.Pa_GetDefaultOutputDevice() : deviceIndex;
        if (dev < 0)
            throw new InvalidOperationException("No PortAudio output device.");
        var info = PortAudioNative.Info(dev) ?? throw new InvalidOperationException("Bad output device.");
        int ch = Math.Min(_channels, Math.Max(1, info.maxOutputChannels));
        var sp = new PortAudioNative.StreamParameters
        {
            device = dev,
            channelCount = ch,
            sampleFormat = PortAudioNative.Float32,
            suggestedLatency = info.defaultLowOutputLatency,
            hostApiSpecificStreamInfo = IntPtr.Zero,
        };
        IntPtr pOut = Marshal.AllocHGlobal(Marshal.SizeOf<PortAudioNative.StreamParameters>());
        try
        {
            Marshal.StructureToPtr(sp, pOut, false);
            int err = PortAudioNative.Pa_OpenStream(out _stream, IntPtr.Zero, pOut, _rate, 480,
                0, IntPtr.Zero, IntPtr.Zero);
            if (err != PortAudioNative.NoError)
                throw new InvalidOperationException("Pa_OpenStream play: " + PortAudioNative.Err(err));
        }
        finally
        {
            Marshal.FreeHGlobal(pOut);
        }
        errStart();
        _run = true;
        int outCh = ch;
        _thread = new Thread(() => PlayLoop(outCh))
        {
            IsBackground = true,
            Name = "MsccPaPlay",
            Priority = ThreadPriority.AboveNormal,
        };
        _thread.Start();
        Log?.Invoke($"Play {PortAudioNative.DeviceName(dev)} {_rate} Hz ch={outCh}");
    }

    private void errStart()
    {
        int e = PortAudioNative.Pa_StartStream(_stream);
        if (e != PortAudioNative.NoError)
        {
            PortAudioNative.Pa_CloseStream(_stream);
            _stream = IntPtr.Zero;
            PortAudioNative.Release();
            throw new InvalidOperationException("Pa_StartStream play: " + PortAudioNative.Err(e));
        }
    }

    private void PlayLoop(int outCh)
    {
        var pcm = new short[480 * _channels];
        var f = new float[480 * outCh];
        while (_run && _stream != IntPtr.Zero)
        {
            _jitter.ReadSamples(pcm, _channels);
            float vol = Muted ? 0f : _volume;
            for (int i = 0; i < 480; i++)
            {
                float s = pcm[i * _channels] / 32768f;
                s = Eq.Process(s) * vol;
                if (outCh == 1)
                    f[i] = s;
                else
                {
                    f[i * 2] = s;
                    f[i * 2 + 1] = _channels == 2 ? Eq.Process(pcm[i * 2 + 1] / 32768f) * vol : s;
                }
            }
            int e = PortAudioNative.Pa_WriteStream(_stream, f, 480);
            if (e != PortAudioNative.NoError && _run)
                Log?.Invoke("Pa_WriteStream: " + PortAudioNative.Err(e));
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
    }

    public void Dispose() => Stop();
}
