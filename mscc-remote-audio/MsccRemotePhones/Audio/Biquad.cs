namespace MsccRemotePhones.Audio;

/// <summary>Direct-form I biquad (RBJ cookbook coefficients).</summary>
public sealed class Biquad
{
    private float _b0, _b1, _b2, _a1, _a2;
    private float _z1, _z2;

    public void Reset() => _z1 = _z2 = 0f;

    public float Process(float x)
    {
        float y = _b0 * x + _z1;
        _z1 = _b1 * x - _a1 * y + _z2;
        _z2 = _b2 * x - _a2 * y;
        return y;
    }

    public void SetLowShelf(float sampleRate, float freqHz, float gainDb)
    {
        float a = MathF.Pow(10f, gainDb / 40f);
        float w0 = 2f * MathF.PI * freqHz / sampleRate;
        float cos = MathF.Cos(w0);
        float sin = MathF.Sin(w0);
        float alpha = sin / 2f * MathF.Sqrt((a + 1f / a) * (1f / 0.707f - 1f) + 2f);
        float twoSqrtAAlpha = 2f * MathF.Sqrt(a) * alpha;

        float b0 = a * ((a + 1f) - (a - 1f) * cos + twoSqrtAAlpha);
        float b1 = 2f * a * ((a - 1f) - (a + 1f) * cos);
        float b2 = a * ((a + 1f) - (a - 1f) * cos - twoSqrtAAlpha);
        float a0 = (a + 1f) + (a - 1f) * cos + twoSqrtAAlpha;
        float a1 = -2f * ((a - 1f) + (a + 1f) * cos);
        float a2 = (a + 1f) + (a - 1f) * cos - twoSqrtAAlpha;
        SetNormalized(b0, b1, b2, a0, a1, a2);
    }

    public void SetPeaking(float sampleRate, float freqHz, float gainDb, float q)
    {
        float a = MathF.Pow(10f, gainDb / 40f);
        float w0 = 2f * MathF.PI * freqHz / sampleRate;
        float cos = MathF.Cos(w0);
        float sin = MathF.Sin(w0);
        float alpha = sin / (2f * Math.Max(0.1f, q));

        float b0 = 1f + alpha * a;
        float b1 = -2f * cos;
        float b2 = 1f - alpha * a;
        float a0 = 1f + alpha / a;
        float a1 = -2f * cos;
        float a2 = 1f - alpha / a;
        SetNormalized(b0, b1, b2, a0, a1, a2);
    }

    public void SetHighShelf(float sampleRate, float freqHz, float gainDb)
    {
        float a = MathF.Pow(10f, gainDb / 40f);
        float w0 = 2f * MathF.PI * freqHz / sampleRate;
        float cos = MathF.Cos(w0);
        float sin = MathF.Sin(w0);
        float alpha = sin / 2f * MathF.Sqrt((a + 1f / a) * (1f / 0.707f - 1f) + 2f);
        float twoSqrtAAlpha = 2f * MathF.Sqrt(a) * alpha;

        float b0 = a * ((a + 1f) + (a - 1f) * cos + twoSqrtAAlpha);
        float b1 = -2f * a * ((a - 1f) + (a + 1f) * cos);
        float b2 = a * ((a + 1f) + (a - 1f) * cos - twoSqrtAAlpha);
        float a0 = (a + 1f) - (a - 1f) * cos + twoSqrtAAlpha;
        float a1 = 2f * ((a - 1f) - (a + 1f) * cos);
        float a2 = (a + 1f) - (a - 1f) * cos - twoSqrtAAlpha;
        SetNormalized(b0, b1, b2, a0, a1, a2);
    }

    public void SetBypass()
    {
        _b0 = 1f;
        _b1 = _b2 = _a1 = _a2 = 0f;
        Reset();
    }

    private void SetNormalized(float b0, float b1, float b2, float a0, float a1, float a2)
    {
        float inv = 1f / a0;
        _b0 = b0 * inv;
        _b1 = b1 * inv;
        _b2 = b2 * inv;
        _a1 = a1 * inv;
        _a2 = a2 * inv;
        Reset();
    }
}
