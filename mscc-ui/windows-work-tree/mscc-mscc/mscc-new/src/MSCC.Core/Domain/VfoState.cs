using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace MSCC.Core.Domain;

/// <summary>
/// Represents the state of one VFO (A or B).
/// This is a pure domain model with no UI dependencies.
/// </summary>
public class VfoState : INotifyPropertyChanged
{
    private long _frequencyHz = 7_100_000;
    private RadioMode _mode = RadioMode.USB;
    private FilterSettings _filter = new();
    private bool _ritOn;
    private long _ritOffsetHz;

    public long FrequencyHz
    {
        get => _frequencyHz;
        set
        {
            if (SetField(ref _frequencyHz, value))
            {
                OnPropertyChanged(nameof(FrequencyDisplay));
            }
        }
    }

    public RadioMode Mode
    {
        get => _mode;
        set
        {
            if (SetField(ref _mode, value))
                OnPropertyChanged(nameof(ModeDisplay));
        }
    }

    /// <summary>UI label (DIG-U rather than DigU).</summary>
    public string ModeDisplay => Mode switch
    {
        RadioMode.DigU => "DIG-U",
        _ => Mode.ToString()
    };

    public FilterSettings Filter
    {
        get => _filter;
        set => SetField(ref _filter, value);
    }

    public bool RitOn
    {
        get => _ritOn;
        set
        {
            if (SetField(ref _ritOn, value))
            {
                OnPropertyChanged(nameof(EffectiveFrequencyHz));
                OnPropertyChanged(nameof(RitDisplay));
            }
        }
    }

    public long RitOffsetHz
    {
        get => _ritOffsetHz;
        set
        {
            value = Math.Clamp(value, -500, 500);
            if (SetField(ref _ritOffsetHz, value))
            {
                OnPropertyChanged(nameof(EffectiveFrequencyHz));
                OnPropertyChanged(nameof(RitDisplay));
            }
        }
    }

    public long EffectiveFrequencyHz => FrequencyHz + (RitOn ? RitOffsetHz : 0);

    /// <summary>
    /// Main frequency line only (fixed F6 width). RIT offset is shown separately below.
    /// </summary>
    public string FrequencyDisplay => $"{FrequencyHz / 1_000_000.0:F6}";

    /// <summary>
    /// Compact RIT offset for the line under the frequency (empty when RIT off).
    /// </summary>
    public string RitDisplay => RitOn ? $"RIT {RitOffsetHz:+#;-#;0}" : "";

    public event PropertyChangedEventHandler? PropertyChanged;

    protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }

    protected bool SetField<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value)) return false;
        field = value;
        OnPropertyChanged(propertyName);
        return true;
    }
}

public enum RadioMode
{
    USB,
    LSB,
    AM,
    CW,
    TUNE,
    /// <summary>
    /// Digital upper sideband — client profile (filters/audio). Radio LO still USB.
    /// </summary>
    DigU
}
