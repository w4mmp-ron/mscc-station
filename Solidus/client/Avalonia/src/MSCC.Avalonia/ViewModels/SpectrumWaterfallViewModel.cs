using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using MSCC.Avalonia.Controls;

namespace MSCC.Avalonia.ViewModels;

/// <summary>View-model for S/W (spectrum / waterfall) controls popup — WPF-style sections.</summary>
public partial class SpectrumWaterfallViewModel : ViewModelBase
{
    private readonly SpectrumDisplaySettings _s = SpectrumDisplaySettings.Instance;
    private readonly AppearanceSettings _a = AppearanceSettings.Instance;
    private bool _loading;

    public SpectrumWaterfallViewModel()
    {
        _loading = true;
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

    [ObservableProperty] private double _zoomFactor = 1;
    [ObservableProperty] private float _dbCalRelative;
    [ObservableProperty] private float _gridMaxDb = -20;
    [ObservableProperty] private float _gridMinDb = -125;
    [ObservableProperty] private float _waterfallHighDb = -50;
    [ObservableProperty] private float _waterfallLowDb = -120;
    [ObservableProperty] private bool _viewGrid = true;
    [ObservableProperty] private bool _showWaterfall = true;
    /// <summary>True = normal (oldest top); false = reverse (newest top).</summary>
    [ObservableProperty] private bool _waterfallDirectionNormal = true;
    [ObservableProperty] private string _selectedWaterfallPalette = "Enhanced";
    [ObservableProperty] private string _selectedSpectrumBackground = "BLACK";
    [ObservableProperty] private string _selectedSpectrumFill = "SCOPE";
    [ObservableProperty] private string _selectedSpectrumLine = "GREEN";
    [ObservableProperty] private string _spectrumBackgroundRgbText = "#101018";
    [ObservableProperty] private int _specBgR = 0x10;
    [ObservableProperty] private int _specBgG = 0x10;
    [ObservableProperty] private int _specBgB = 0x18;
    [ObservableProperty] private bool _showSpectrumBackgroundRgb;

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
