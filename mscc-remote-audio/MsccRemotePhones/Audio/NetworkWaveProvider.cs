using NAudio.Wave;

namespace MsccRemotePhones.Audio;

/// <summary>
/// WASAPI/WaveOut pull provider. Always outputs stereo float-friendly 16-bit stereo
/// (mono input is duplicated L=R) at a fixed sample rate.
/// </summary>
public sealed class NetworkWaveProvider : IWaveProvider
{
    private readonly JitterBuffer _buffer;
    private readonly WaveFormat _format;
    private readonly short[] _srcScratch = new short[16384];
    private volatile float _volume = 1f;

    public NetworkWaveProvider(JitterBuffer buffer, int sampleRate)
    {
        _buffer = buffer;
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

        float vol = _volume;
        int bi = offset;
        if (srcCh == 1)
        {
            for (int i = 0; i < frames; i++)
            {
                short s = ScaleSample(_srcScratch[i], vol);
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
                short l = ScaleSample(_srcScratch[i * 2], vol);
                short r = ScaleSample(_srcScratch[i * 2 + 1], vol);
                buffer[bi++] = (byte)(l & 0xFF);
                buffer[bi++] = (byte)((l >> 8) & 0xFF);
                buffer[bi++] = (byte)(r & 0xFF);
                buffer[bi++] = (byte)((r >> 8) & 0xFF);
            }
        }

        return count;
    }

    private static short ScaleSample(short s, float vol)
    {
        if (vol >= 0.999f)
            return s;
        if (vol <= 0.001f)
            return 0;
        int v = (int)(s * vol);
        if (v > short.MaxValue) return short.MaxValue;
        if (v < short.MinValue) return short.MinValue;
        return (short)v;
    }
}
