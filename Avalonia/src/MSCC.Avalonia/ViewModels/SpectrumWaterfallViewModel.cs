using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using MSCC.Avalonia.Controls;

namespace MSCC.Avalonia.ViewModels;

/// <summary>View-model for S/W (spectrum / waterfall) controls popup — WPF-style sections.</summary>
public partial class SpectrumWaterfallViewModel : ViewModelBase
{
    private readonly SpectrumDisplaySettings _s = SpectrumDisplaySettings.Instance;
    private readonly AppearanceSettings _a = AppearanceSettings.Instance;
    private readonly MainViewModel? _main;
    private bool _loading;

    public SpectrumWaterfallViewModel() : this(null)
    {
    }

    public SpectrumWaterfallViewModel(MainViewModel? main)
    {
        _main = main;
        _loading = true;
        PanResolutionIndex = main?.PanResolutionIndex ?? 0;
        SelectedSmoothing = main?.SpectrumSmoothing ?? 3;
        ZoomFactor = _s.ZoomFactor;
        DbCalRelative = _s.DbCalRelative;
        GridMaxDb = _s.GridMaxDb;
        GridMinDb = _s.GridMinDb;
        WaterfallHighDb = _s.WaterfallHighDb;
        WaterfallLowDb = _s.WaterfallLowDb;
        ViewGrid = _s.ViewGrid;
        ShowWaterfall = _s.ShowWaterfall;
        WaterfallDirectionNormal = _s.WaterfallDirectionNormal;
        SelectedWaterfallPalette = WaterfallPalettes.NormalizeName(_s.WaterfallPalette);
        Baseline = _s.Baseline;
        WaterfallGain = _s.WaterfallGain;
        WaterfallZero = _s.WaterfallZero;
        ViewDbLabels = _s.ViewDbLabels;
        PeakMarker = _s.PeakMarker;
        AutoSnap = _s.AutoSnap;
        SelectedAutoSnapStep = AutoSnapLabel(_s.AutoSnapStepHz);
        TimeMarkerText = _s.WaterfallTimeMarker.ToString();
        SpectrumSplitPercent = _s.SpectrumSplitPercent;
        TimeDisplayOn = main?.TimeDisplayOn ?? false;
        SelectedSpectrumBackground = _a.SpectrumBackground;
        SelectedSpectrumFill = _a.SpectrumFill;
        SelectedSpectrumLine = _a.SpectrumLine;
        SpectrumBackgroundRgbText = _a.SpectrumBackgroundRgb;
        if (UiChromeTheme.TryParseHex(_a.SpectrumBackgroundRgb, out byte r, out byte g, out byte b))
        {
            SpecBgR = r; SpecBgG = g; SpecBgB = b;
        }
        ShowSpectrumBackgroundRgb = UiChromeTheme.IsCustom(_a.SpectrumBackground);
        _loading = false;

        _s.Changed += OnSettingsChanged;
        _a.Changed += OnAppearanceChanged;
    }

    public string[] WaterfallPaletteNames => WaterfallPalettes.Names;
    public string[] SpectrumBackgroundNames => UiChromeTheme.SpectrumBackgroundNames;
    public string[] SpectrumFillNames => UiChromeTheme.SpectrumFillNames;
    public string[] SpectrumLineNames => UiChromeTheme.SpectrumLineNames;

    [ObservableProperty] private int _panResolutionIndex;

    /// <summary>WPF cmd-075 port: SMOOTHING choices (server time constant = value x 25 ms).</summary>
    public int[] SmoothingOptions { get; } = { 0, 1, 2, 3, 4, 6, 8, 10, 15, 20, 30 };
    [ObservableProperty] private int _selectedSmoothing = 3;
    [ObservableProperty] private double _zoomFactor = 1;
    [ObservableProperty] private float _dbCalRelative;
    [ObservableProperty] private float _gridMaxDb = -20;
    [ObservableProperty] private float _gridMinDb = -155;
    [ObservableProperty] private float _waterfallHighDb = -40;
    [ObservableProperty] private float _waterfallLowDb = -150;
    [ObservableProperty] private bool _viewGrid = true;
    [ObservableProperty] private bool _showWaterfall = true;
    [ObservableProperty] private int _spectrumSplitPercent = SpectrumDisplaySettings.SpectrumSplitDefault;
    /// <summary>True = Climbing (newest at bottom); false = Falling (newest at top, default; WPF cmd-075).</summary>
    [ObservableProperty] private bool _waterfallDirectionNormal;
    [ObservableProperty] private string _selectedWaterfallPalette = "Enhanced";
    [ObservableProperty] private string _selectedSpectrumBackground = "BLACK";
    [ObservableProperty] private string _selectedSpectrumFill = "SCOPE";
    [ObservableProperty] private string _selectedSpectrumLine = "GREEN";
    [ObservableProperty] private string _spectrumBackgroundRgbText = "#101018";
    [ObservableProperty] private int _specBgR = 0x10;
    [ObservableProperty] private int _specBgG = 0x10;
    [ObservableProperty] private int _specBgB = 0x18;
    [ObservableProperty] private bool _showSpectrumBackgroundRgb;

    // WPF S/W extras
    [ObservableProperty] private int _baseline = 50;
    [ObservableProperty] private int _waterfallGain = 50;
    [ObservableProperty] private int _waterfallZero;
    [ObservableProperty] private bool _viewDbLabels;
    [ObservableProperty] private bool _peakMarker;
    [ObservableProperty] private bool _autoSnap;
    public string[] AutoSnapSteps { get; } = { "1KHz", "500Hz", "100Hz" };
    [ObservableProperty] private string _selectedAutoSnapStep = "100Hz";
    [ObservableProperty] private string _timeMarkerText = "0";
    [ObservableProperty] private bool _timeDisplayOn;

    private static string AutoSnapLabel(int hz) => hz switch { 1000 => "1KHz", 500 => "500Hz", _ => "100Hz" };
    private static int AutoSnapHz(string? label) => label switch { "1KHz" => 1000, "500Hz" => 500, _ => 100 };

    partial void OnBaselineChanged(int value) { if (!_loading) _s.SetBaseline(value); }
    partial void OnWaterfallGainChanged(int value) { if (!_loading) _s.SetWaterfallGain(value); }
    partial void OnWaterfallZeroChanged(int value) { if (!_loading) _s.SetWaterfallZero(value); }
    partial void OnViewDbLabelsChanged(bool value) { if (!_loading) _s.SetViewDbLabels(value); }
    partial void OnPeakMarkerChanged(bool value) { if (!_loading) _s.SetPeakMarker(value); }
    partial void OnSpectrumSplitPercentChanged(int value) { if (!_loading) _s.SetSpectrumSplitPercent(value); }
    partial void OnAutoSnapChanged(bool value) { if (!_loading) _s.SetAutoSnap(value); }
    partial void OnSelectedAutoSnapStepChanged(string value) { if (!_loading) _s.SetAutoSnapStepHz(AutoSnapHz(value)); }
    partial void OnTimeDisplayOnChanged(bool value) { if (!_loading && _main != null) _main.TimeDisplayOn = value; }

    /// <summary>TIME MARKER button: 0 → 1 → … → 5 → 0 seconds between waterfall time marks.</summary>
    [RelayCommand]
    private void CycleTimeMarker() => TimeMarkerText = _s.CycleWaterfallTimeMarker().ToString();

    private void OnSettingsChanged()
    {
        _loading = true;
        if (Math.Abs(ZoomFactor - _s.ZoomFactor) > 0.01)
            ZoomFactor = _s.ZoomFactor;
        if (Math.Abs(DbCalRelative - _s.DbCalRelative) > 0.01f)
            DbCalRelative = _s.DbCalRelative;
        if (Math.Abs(GridMaxDb - _s.GridMaxDb) > 0.01f)
            GridMaxDb = _s.GridMaxDb;
        if (Math.Abs(GridMinDb - _s.GridMinDb) > 0.01f)
            GridMinDb = _s.GridMinDb;
        if (Math.Abs(WaterfallHighDb - _s.WaterfallHighDb) > 0.01f)
            WaterfallHighDb = _s.WaterfallHighDb;
        if (Math.Abs(WaterfallLowDb - _s.WaterfallLowDb) > 0.01f)
            WaterfallLowDb = _s.WaterfallLowDb;
        if (ViewGrid != _s.ViewGrid)
            ViewGrid = _s.ViewGrid;
        if (ShowWaterfall != _s.ShowWaterfall)
            ShowWaterfall = _s.ShowWaterfall;
        if (WaterfallDirectionNormal != _s.WaterfallDirectionNormal)
            WaterfallDirectionNormal = _s.WaterfallDirectionNormal;
        string pal = WaterfallPalettes.NormalizeName(_s.WaterfallPalette);
        if (!string.Equals(SelectedWaterfallPalette, pal, StringComparison.Ordinal))
            SelectedWaterfallPalette = pal;
        if (Baseline != _s.Baseline) Baseline = _s.Baseline;
        if (WaterfallGain != _s.WaterfallGain) WaterfallGain = _s.WaterfallGain;
        if (WaterfallZero != _s.WaterfallZero) WaterfallZero = _s.WaterfallZero;
        if (ViewDbLabels != _s.ViewDbLabels) ViewDbLabels = _s.ViewDbLabels;
        if (PeakMarker != _s.PeakMarker) PeakMarker = _s.PeakMarker;
        if (AutoSnap != _s.AutoSnap) AutoSnap = _s.AutoSnap;
        string step = AutoSnapLabel(_s.AutoSnapStepHz);
        if (SelectedAutoSnapStep != step) SelectedAutoSnapStep = step;
        TimeMarkerText = _s.WaterfallTimeMarker.ToString();
        if (SpectrumSplitPercent != _s.SpectrumSplitPercent) SpectrumSplitPercent = _s.SpectrumSplitPercent;
        _loading = false;
    }

    private void OnAppearanceChanged()
    {
        _loading = true;
        if (!string.Equals(SelectedSpectrumBackground, _a.SpectrumBackground, StringComparison.OrdinalIgnoreCase))
            SelectedSpectrumBackground = _a.SpectrumBackground;
        if (!string.Equals(SelectedSpectrumFill, _a.SpectrumFill, StringComparison.OrdinalIgnoreCase))
            SelectedSpectrumFill = _a.SpectrumFill;
        if (!string.Equals(SelectedSpectrumLine, _a.SpectrumLine, StringComparison.OrdinalIgnoreCase))
            SelectedSpectrumLine = _a.SpectrumLine;
        SpectrumBackgroundRgbText = _a.SpectrumBackgroundRgb;
        if (UiChromeTheme.TryParseHex(_a.SpectrumBackgroundRgb, out byte r, out byte g, out byte b))
        {
            SpecBgR = r; SpecBgG = g; SpecBgB = b;
        }
        ShowSpectrumBackgroundRgb = UiChromeTheme.IsCustom(_a.SpectrumBackground);
        _loading = false;
    }

    partial void OnPanResolutionIndexChanged(int value)
    {
        if (_loading) return;
        if (value < 0 || value > 2) return;
        _main?.SetPanResolutionIndexFromUi(value);
    }

    partial void OnSelectedSmoothingChanged(int value)
    {
        if (_loading) return;
        _main?.SetSpectrumSmoothingFromUi(value);
    }

    partial void OnZoomFactorChanged(double value)
    {
        if (_loading) return;
        _s.SetZoomFactor(value);
    }

    partial void OnDbCalRelativeChanged(float value)
    {
        if (_loading) return;
        _s.SetDbCalRelative(value);
    }

    partial void OnGridMaxDbChanged(float value)
    {
        if (_loading) return;
        _s.SetGrid(value, _s.GridMinDb);
    }

    partial void OnGridMinDbChanged(float value)
    {
        if (_loading) return;
        _s.SetGrid(_s.GridMaxDb, value);
    }

    partial void OnWaterfallHighDbChanged(float value)
    {
        if (_loading) return;
        _s.SetWaterfallWindow(value, _s.WaterfallLowDb);
    }

    partial void OnWaterfallLowDbChanged(float value)
    {
        if (_loading) return;
        _s.SetWaterfallWindow(_s.WaterfallHighDb, value);
    }

    partial void OnViewGridChanged(bool value)
    {
        if (_loading) return;
        _s.SetViewGrid(value);
    }

    partial void OnShowWaterfallChanged(bool value)
    {
        if (_loading) return;
        _s.SetShowWaterfall(value);
    }

    partial void OnWaterfallDirectionNormalChanged(bool value)
    {
        if (_loading) return;
        _s.SetWaterfallDirectionNormal(value);
    }

    partial void OnSelectedWaterfallPaletteChanged(string value)
    {
        if (_loading) return;
        if (string.IsNullOrWhiteSpace(value)) return;
        _s.SetWaterfallPalette(value);
    }

    partial void OnSelectedSpectrumFillChanged(string value)
    {
        if (_loading || string.IsNullOrWhiteSpace(value)) return;
        _a.SetSpectrumFill(value);
    }

    partial void OnSelectedSpectrumLineChanged(string value)
    {
        if (_loading || string.IsNullOrWhiteSpace(value)) return;
        _a.SetSpectrumLine(value);
    }

    partial void OnSelectedSpectrumBackgroundChanged(string value)
    {
        if (_loading || string.IsNullOrWhiteSpace(value)) return;
        if (UiChromeTheme.IsCustom(value))
        {
            _a.SetSpectrumBackgroundRgb(
                (byte)Math.Clamp(SpecBgR, 0, 255),
                (byte)Math.Clamp(SpecBgG, 0, 255),
                (byte)Math.Clamp(SpecBgB, 0, 255));
        }
        else
        {
            _a.SetSpectrumBackground(value);
        }
    }

    partial void OnSpecBgRChanged(int value) => ApplySpectrumBgRgb();
    partial void OnSpecBgGChanged(int value) => ApplySpectrumBgRgb();
    partial void OnSpecBgBChanged(int value) => ApplySpectrumBgRgb();

    private void ApplySpectrumBgRgb()
    {
        if (_loading) return;
        if (!UiChromeTheme.IsCustom(SelectedSpectrumBackground)) return;
        _a.SetSpectrumBackgroundRgb(
            (byte)Math.Clamp(SpecBgR, 0, 255),
            (byte)Math.Clamp(SpecBgG, 0, 255),
            (byte)Math.Clamp(SpecBgB, 0, 255));
    }

    [RelayCommand]
    private void ResetDbCal()
    {
        _s.ResetDbCal();
        _loading = true;
        DbCalRelative = 0;
        _loading = false;
    }
}
