using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace MSCC.Wpf.Favorites;

/// <summary>
/// One client-side favorite: named snapshot of operating conditions.
/// All data stays on the client (never sent to ms-sdr).
/// </summary>
public sealed class FavoriteEntry : INotifyPropertyChanged
{
    private string _name = "";
    private string _band = "";
    private long _frequencyHz;
    private string _mode = "USB";
    private int _lowCutIndex;
    private int _highCutIndex;
    private int _cwFilterIndex;
    private string _vfo = "A";

    public string Name
    {
        get => _name;
        set { if (Set(ref _name, value ?? "")) OnPropertyChanged(nameof(FrequencyDisplay)); }
    }

    public string Band
    {
        get => _band;
        set => Set(ref _band, value ?? "");
    }

    public long FrequencyHz
    {
        get => _frequencyHz;
        set
        {
            if (Set(ref _frequencyHz, value))
                OnPropertyChanged(nameof(FrequencyDisplay));
        }
    }

    /// <summary>Human-readable MHz for list display.</summary>
    public string FrequencyDisplay => $"{FrequencyHz / 1_000_000.0:F6}";

    public string Mode
    {
        get => _mode;
        set => Set(ref _mode, string.IsNullOrWhiteSpace(value) ? "USB" : value);
    }

    public int LowCutIndex
    {
        get => _lowCutIndex;
        set => Set(ref _lowCutIndex, value);
    }

    public int HighCutIndex
    {
        get => _highCutIndex;
        set => Set(ref _highCutIndex, value);
    }

    public int CwFilterIndex
    {
        get => _cwFilterIndex;
        set => Set(ref _cwFilterIndex, value);
    }

    /// <summary>"A" or "B".</summary>
    public string Vfo
    {
        get => _vfo;
        set => Set(ref _vfo, string.Equals(value, "B", StringComparison.OrdinalIgnoreCase) ? "B" : "A");
    }

    // Display helpers for list columns (labels filled by store/VM when useful)
    public string LowCutLabel { get; set; } = "";
    public string HighCutLabel { get; set; } = "";
    public string CwFilterLabel { get; set; } = "";

    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged([CallerMemberName] string? name = null)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));

    private bool Set<T>(ref T field, T value, [CallerMemberName] string? name = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value)) return false;
        field = value;
        OnPropertyChanged(name);
        return true;
    }
}
