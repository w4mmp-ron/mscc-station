namespace MSCC.Core.Display;

/// <summary>
/// Represents a single frame of spectrum data ready for rendering.
/// This is the clean data contract between the radio core and the UI.
/// </summary>
public sealed record SpectrumUpdate
{
    /// <summary>
    /// Full panadapter RF span (Hz). Original MSCC: XCVR_Freq ± 36 kHz, labeled "72KHz"
    /// (CursorValueEvent / Mouse_Set_freq: start = XCVR − 36000, scale 72000 / width).
    /// </summary>
    public const int DefaultPanadapterSpanHz = 72_000;

    /// <summary>
    /// Processed spectrum data in dB scale.
    /// </summary>
    public required float[] Data { get; init; }

    public long CenterFrequencyHz { get; init; }
    public int SpanHz { get; init; }
    public int FilterLowHz { get; init; }
    public int FilterHighHz { get; init; }

    /// <summary>Bottom of spectrum dB window (absolute after client dB cal offset).</summary>
    public float MinDb { get; init; } = -140f;
    /// <summary>Top of spectrum dB window (0 dB = full-scale reference after cal).</summary>
    public float MaxDb { get; init; } = 0f;

    public int SMeter { get; init; } = 5;

    /// <summary>
    /// CW pitch in Hz for display shift compensation.
    /// When non-zero, the spectrum data is shifted by this amount for correct visual alignment in CW mode.
    /// </summary>
    public int CwPitchHz { get; init; } = 0;
}
