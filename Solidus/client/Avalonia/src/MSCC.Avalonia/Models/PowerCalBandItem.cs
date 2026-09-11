using CommunityToolkit.Mvvm.ComponentModel;

namespace MSCC.Avalonia.Models;

/// <summary>One band row for QRP CAL / AMP CAL status lamps and band selection.</summary>
public partial class PowerCalBandItem : ObservableObject
{
    public int BandNumber { get; set; }
    public string BandLabel { get; set; } = "";

    /// <summary>Session-side calibrated flag (not persisted yet).</summary>
    [ObservableProperty]
    private bool _isCalibrated;

    [ObservableProperty]
    private bool _isSelected;
}
