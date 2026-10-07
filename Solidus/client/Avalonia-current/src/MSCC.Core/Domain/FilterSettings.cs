using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace MSCC.Core.Domain;

public class FilterSettings : INotifyPropertyChanged
{
    private int _lowHz = 75;
    private int _highHz = 4000;
    private int _cwPitchHz = 1;  // default to index for 600Hz (CW pitch feature deferred; backend for PITCH expects index 0-3)

    public int LowHz
    {
        get => _lowHz;
        set => SetField(ref _lowHz, value);
    }

    public int HighHz
    {
        get => _highHz;
        set => SetField(ref _highHz, value);
    }

    public int CwPitchHz
    {
        get => _cwPitchHz;
        set => SetField(ref _cwPitchHz, value);
    }

    public int Bandwidth => HighHz - LowHz;

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
