namespace MsccRemotePhones.Audio;

/// <summary>
/// 3-band playback EQ: low shelf (~120 Hz), presence peak (~2 kHz), high shelf (~5 kHz).
/// Thread-safe: UI updates coeffs via <see cref="ApplySettings"/>; audio thread only processes.
/// </summary>
public sealed class PlaybackEq
{
    public const float LowFreqHz = 120f;
    public const float MidFreqHz = 2000f;
    public const float HighFreqHz = 5000f;
    public const float MidQ = 0.9f;
    public const float MaxGainDb = 12f;

    private readonly Biquad _low = new();
    private readonly Biquad _mid = new();
    private readonly Biquad _high = new();
    private readonly object _gate = new();

    private int _sampleRate = 48000;
    private bool _enabled;
    private volatile bool _processEnabled;
    private float _lowDb;
    private float _midDb;
    private float _highDb;

    public bool Enabled
    {
        get { lock (_gate) return _enabled; }
    }

    public float LowDb
    {
        get { lock (_gate) return _lowDb; }
    }

    public float MidDb
    {
        get { lock (_gate) return _midDb; }
    }

    public float HighDb
    {
        get { lock (_gate) return _highDb; }
    }

    public void SetSampleRate(int sampleRate)
    {
        if (sampleRate <= 0) sampleRate = 48000;
        lock (_gate)
        {
            if (_sampleRate == sampleRate)
                return;
            _sampleRate = sampleRate;
            RebuildUnlocked();
        }
    }

    public void ApplySettings(bool enabled, float lowDb, float midDb, float highDb)
    {
        lock (_gate)
        {
            _enabled = enabled;
            _lowDb = ClampDb(lowDb);
            _midDb = ClampDb(midDb);
            _highDb = ClampDb(highDb);
            RebuildUnlocked();
        }
    }

    public void ResetFlat() => ApplySettings(false, 0f, 0f, 0f);

    /// <summary>Process one mono sample (float -1..1). Safe on audio thread.</summary>
    public float Process(float x)
    {
        if (!_processEnabled)
            return x;
        x = _low.Process(x);
        x = _mid.Process(x);
        x = _high.Process(x);
        return x;
    }

    private void RebuildUnlocked()
    {
        int sr = _sampleRate;
        if (!_enabled || (NearZero(_lowDb) && NearZero(_midDb) && NearZero(_highDb)))
        {
            _low.SetBypass();
            _mid.SetBypass();
            _high.SetBypass();
            _processEnabled = false;
            return;
        }
        _processEnabled = true;

        if (NearZero(_lowDb))
            _low.SetBypass();
        else
            _low.SetLowShelf(sr, LowFreqHz, _lowDb);

        if (NearZero(_midDb))
            _mid.SetBypass();
        else
            _mid.SetPeaking(sr, MidFreqHz, _midDb, MidQ);

        if (NearZero(_highDb))
            _high.SetBypass();
        else
            _high.SetHighShelf(sr, HighFreqHz, _highDb);
    }

    private static float ClampDb(float db) => Math.Clamp(db, -MaxGainDb, MaxGainDb);
    private static bool NearZero(float db) => MathF.Abs(db) < 0.05f;
}
