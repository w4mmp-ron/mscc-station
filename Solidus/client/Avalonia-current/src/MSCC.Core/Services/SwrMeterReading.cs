namespace MSCC.Core.Services;

/// <summary>One sample from the external WiFi SWR meter (JSON or pipe payload).</summary>
public sealed class SwrMeterReading
{
    public float ForwardWatts { get; init; }
    public float ReflectedWatts { get; init; }
    public float PeakWatts { get; init; }
    public float Swr { get; init; } = 1f;
    public bool Fault { get; init; }
    public bool Tx { get; init; }
    public float SwrThreshold { get; init; } = 2f;
    public string? SourceIp { get; init; }
    public DateTime Utc { get; init; } = DateTime.UtcNow;
}
