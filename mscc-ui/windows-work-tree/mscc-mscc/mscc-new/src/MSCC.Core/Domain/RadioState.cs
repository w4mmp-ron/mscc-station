using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace MSCC.Core.Domain;

/// <summary>
/// Root state of the radio. This is the single source of truth for the entire application.
/// All ViewModels should ultimately bind to (or derive from) this.
/// </summary>
public class RadioState : INotifyPropertyChanged
{
    private VfoState _vfoA = new() { FrequencyHz = 7_100_000 };
    private VfoState _vfoB = new() { FrequencyHz = 7_200_000 };
    private VfoState _activeVfo;
    private bool _isTransmitting;
    private int _rfPowerPercent = 80;
    private int _volume = 60;
    private int _micGain = 45;
    private int _compression = 30;
    private string _currentBand = "40m";

    // Separate for P (operator/phones) and D (digital) audio paths
    private int _pVolume = 60;
    private int _pMicGain = 45;
    private int _dVolume = 60;
    private int _dMicGain = 45;

    public RadioState()
    {
        _activeVfo = _vfoA;
    }

    public VfoState VfoA
    {
        get => _vfoA;
        set => SetField(ref _vfoA, value);
    }

    public VfoState VfoB
    {
        get => _vfoB;
        set => SetField(ref _vfoB, value);
    }

    public VfoState ActiveVfo
    {
        get => _activeVfo;
        set => SetField(ref _activeVfo, value);
    }

    public bool IsTransmitting
    {
        get => _isTransmitting;
        set => SetField(ref _isTransmitting, value);
    }

    public int RfPowerPercent
    {
        get => _rfPowerPercent;
        set => SetField(ref _rfPowerPercent, value);
    }

    public int Volume
    {
        get => _volume;
        set => SetField(ref _volume, value);
    }

    public int MicGain
    {
        get => _micGain;
        set => SetField(ref _micGain, value);
    }

    public int Compression
    {
        get => _compression;
        set => SetField(ref _compression, value);
    }

    // P (operator/phones) and D (digital) groups for Volume and Mic Gain
    public int PVolume
    {
        get => _pVolume;
        set => SetField(ref _pVolume, value);
    }

    public int PMicGain
    {
        get => _pMicGain;
        set => SetField(ref _pMicGain, value);
    }

    public int DVolume
    {
        get => _dVolume;
        set => SetField(ref _dVolume, value);
    }

    public int DMicGain
    {
        get => _dMicGain;
        set => SetField(ref _dMicGain, value);
    }

    public string CurrentBand
    {
        get => _currentBand;
        set => SetField(ref _currentBand, value);
    }

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

    /// <summary>
    /// Switches the active VFO between A and B.
    /// </summary>
    public void ToggleActiveVfo()
    {
        ActiveVfo = ActiveVfo == VfoA ? VfoB : VfoA;
        OnPropertyChanged(nameof(ActiveVfo));
    }
}
