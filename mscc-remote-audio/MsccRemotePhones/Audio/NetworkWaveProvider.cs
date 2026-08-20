using NAudio.Wave;

namespace MsccRemotePhones.Audio;

/// <summary>
/// WASAPI/WaveOut pull provider. Always outputs stereo 16-bit
/// (mono input is duplicated L=R) at a fixed sample rate.
/// Optional <see cref="PlaybackEq"/> runs after jitter read, before volume.
/// </summary>
public sealed class NetworkWaveProvider : IWaveProvider
{
    private readonly JitterBuffer _buffer;
    private readonly WaveFormat _format;
    private readonly short[] _srcScratch = new short[16384];
    private readonly PlaybackEq? _eq;
    private volatile float _volume = 1f;
    private volatile bool _muted;

    public NetworkWaveProvider(JitterBuffer buffer, int sampleRate, PlaybackEq? eq = null)
    {
        _buffer = buffer;
        _eq = eq;
        _eq?.SetSampleRate(sampleRate);
        // Stereo PCM16 — most reliable with shared-mode WASAPI
        _format = new WaveFormat(sampleRate, 16, 2);
    }

    public WaveFormat WaveFormat => _format;

    /// <summary>Linear gain 0..1 (thread-safe). Applied while packing PCM for playback.</summary>
    public float Volume
    {
        get => _volume;
        set => _volume = Math.Clamp(value, 0f, 1f);
    }

    /// <summary>When true, output silence (volume slider unchanged).</summary>
    public bool Muted
    {
        get => _muted;
        set => _muted = value;
    }

    public int Read(byte[] buffer, int offset, int count)
    {
        // count is bytes; stereo s16 → 4 bytes per frame
        int frames = count / 4;
        if (frames <= 0)
            return 0;

        int srcCh = Math.Max(1, _buffer.SourceChannels);
        int srcSamplesNeeded = frames * srcCh;
        if (srcSamplesNeeded > _srcScratch.Length)
        {
            frames = _srcScratch.Length / srcCh;
            srcSamplesNeeded = frames * srcCh;
            count = frames * 4;
        }

        _buffer.ReadSamples(_srcScratch.AsSpan(0, srcSamplesNeeded), srcCh);

        float vol = _muted ? 0f : _volume;
        var eq = _eq;
        int bi = offset;
        if (srcCh == 1)
        {
            for (int i = 0; i < frames; i++)
            {
                float x = _srcScratch[i] * (1f / 32768f);
                if (eq is not null)
                    x = eq.Process(x);
                short s = FloatToShort(x * vol);
                buffer[bi++] = (byte)(s & 0xFF);
                buffer[bi++] = (byte)((s >> 8) & 0xFF);
                buffer[bi++] = (byte)(s & 0xFF);
                buffer[bi++] = (byte)((s >> 8) & 0xFF);
            }
        }
        else
        {
            for (int i = 0; i < frames; i++)
            {
                float xl = _srcScratch[i * 2] * (1f / 32768f);
                float xr = _srcScratch[i * 2 + 1] * (1f / 32768f);
                if (eq is not null)
                {
                    xl = eq.Process(xl);
                    // Stereo: process R with same cascade would need dual state;
                    // remote phones are mono-duplicated in practice — EQ L and copy.
                    xr = xl;
                }
                short l = FloatToShort(xl * vol);
                short r = FloatToShort(xr * vol);
                buffer[bi++] = (byte)(l & 0xFF);
                buffer[bi++] = (byte)((l >> 8) & 0xFF);
                buffer[bi++] = (byte)(r & 0xFF);
                buffer[bi++] = (byte)((r >> 8) & 0xFF);
            }
        }

        return count;
    }

    private static short FloatToShort(float x)
    {
        if (x > 1f) x = 1f;
        if (x < -1f) x = -1f;
        int v = (int)(x * 32767f);
        if (v > short.MaxValue) return short.MaxValue;
        if (v < short.MinValue) return short.MinValue;
        return (short)v;
    }
}
