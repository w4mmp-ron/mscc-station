using NAudio.CoreAudioApi;
using NAudio.Wave;
using MsccRemotePhones.Protocol;

namespace MsccRemotePhones.Audio;

public sealed class RemotePhonePlayer : IDisposable
{
    private readonly JitterBuffer _buffer;
    private IWavePlayer? _player;
    private NetworkWaveProvider? _provider;
    private int _sampleRate = MsccAudioProtocol.DefaultSampleRate;
    private float _volume = 1f;

    public event Action<string>? Log;

    public bool IsPlaying => _player is not null;
    public string? DeviceName { get; private set; }

    /// <summary>Local phones volume 0..1 (sample scale; works for WaveOut and WASAPI).</summary>
    public float Volume
    {
        get => _volume;
        set
        {
            _volume = Math.Clamp(value, 0f, 1f);
            if (_provider is not null)
                _provider.Volume = _volume;
        }
    }
    public int BufferedMs
    {
        get
        {
            int samples = _buffer.QueuedSamples;
            int ch = Math.Max(1, _buffer.SourceChannels);
            int frames = samples / ch;
            return _sampleRate > 0 ? frames * 1000 / _sampleRate : 0;
        }
    }

    public RemotePhonePlayer(JitterBuffer buffer)
    {
        _buffer = buffer;
    }

    public static IReadOnlyList<(int Index, string Name)> ListPlayDevices()
    {
        var list = new List<(int, string)> { (-1, "Default Windows playback device") };
        try
        {
            var enumr = new MMDeviceEnumerator();
            var devices = enumr.EnumerateAudioEndPoints(DataFlow.Render, DeviceState.Active);
            int i = 0;
            foreach (var d in devices)
            {
                list.Add((i, d.FriendlyName));
                i++;
            }
        }
        catch
        {
            for (int i = 0; i < WaveOut.DeviceCount; i++)
                list.Add((i, WaveOut.GetCapabilities(i).ProductName));
        }
        return list;
    }

    public void Start(int sampleRate, int channels, int deviceIndex, int jitterMs)
    {
        Stop();
        _sampleRate = sampleRate > 0 ? sampleRate : MsccAudioProtocol.DefaultSampleRate;
        int ch = channels is 1 or 2 ? channels : 1;

        _buffer.PrebufferSamples = Math.Max(
            _sampleRate / 20,
            _sampleRate * Math.Max(50, jitterMs) / 1000 * ch);

        _provider = new NetworkWaveProvider(_buffer, _sampleRate) { Volume = _volume };

        Exception? last = null;

        // Prefer WaveOutEvent — simpler shared path, fewer WASAPI format surprises
        try
        {
            int waveDev = deviceIndex < 0 ? -1 : deviceIndex;
            var wo = new WaveOutEvent
            {
                DeviceNumber = waveDev,
                DesiredLatency = Math.Clamp(jitterMs, 50, 200),
                NumberOfBuffers = 3,
            };
            wo.Init(_provider);
            _player = wo;
            DeviceName = waveDev < 0 ? "Default (WaveOut)" : $"WaveOut #{waveDev}";
        }
        catch (Exception ex)
        {
            last = ex;
            _player = null;
        }

        if (_player is null)
        {
            try
            {
                WasapiOut wasapi;
                if (deviceIndex < 0)
                {
                    wasapi = new WasapiOut(AudioClientShareMode.Shared, 100);
                    DeviceName = "Default (WASAPI)";
                }
                else
                {
                    var enumr = new MMDeviceEnumerator();
                    var devices = enumr.EnumerateAudioEndPoints(DataFlow.Render, DeviceState.Active);
                    var dev = devices[deviceIndex];
                    wasapi = new WasapiOut(dev, AudioClientShareMode.Shared, true, 100);
                    DeviceName = dev.FriendlyName;
                }
                wasapi.Init(_provider);
                _player = wasapi;
            }
            catch (Exception ex)
            {
                last = ex;
            }
        }

        if (_player is null)
            throw last ?? new InvalidOperationException("No playback device could be opened.");

        _player.Play();
        Log?.Invoke($"Playing {_sampleRate} Hz (stereo out, src ch={ch}) → {DeviceName}");
    }

    public void Stop()
    {
        try { _player?.Stop(); } catch { /* ignore */ }
        _player?.Dispose();
        _player = null;
        _provider = null;
        DeviceName = null;
    }

    public void Dispose() => Stop();
}
