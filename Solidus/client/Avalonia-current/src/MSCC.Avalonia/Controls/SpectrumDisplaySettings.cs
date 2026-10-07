namespace MSCC.Avalonia.Controls;

/// <summary>
/// One radio-type S/W bank (Proficio/HF or Geminus/LF). WPF dual-bank pattern.
/// </summary>
public sealed class SpectrumSwBank
{
    public float WaterfallHighDb { get; set; } = -50f;
    public float WaterfallLowDb { get; set; } = -120f;
    public bool WaterfallDirectionNormal { get; set; } = true;
    public float GridMaxDb { get; set; } = -20f;
    public float GridMinDb { get; set; } = -125f;
    public bool ViewGrid { get; set; } = true;
    public bool ShowWaterfall { get; set; } = true;

    /// <summary>Red/Yellow, Enhanced, Spectran, or BlackWhite (per radio bank).</summary>
    public string WaterfallPalette { get; set; } = "Enhanced";

    public static SpectrumSwBank CreateProficioDefaults() => new()
    {
        // Field-tuned HF defaults (WPF WaterfallHf*)
        WaterfallHighDb = -44f,
        WaterfallLowDb = -106f,
        WaterfallDirectionNormal = true,
        GridMaxDb = -20f,
        GridMinDb = -125f,
        ViewGrid = true,
        ShowWaterfall = true,
        WaterfallPalette = "Enhanced",
    };

    public static SpectrumSwBank CreateGeminusDefaults() => new()
    {
        WaterfallHighDb = -50f,
        WaterfallLowDb = -120f,
        WaterfallDirectionNormal = true,
        GridMaxDb = -20f,
        GridMinDb = -125f,
        ViewGrid = true,
        ShowWaterfall = true,
        WaterfallPalette = "Enhanced",
    };

    public SpectrumSwBank Clone() => new()
    {
        WaterfallHighDb = WaterfallHighDb,
        WaterfallLowDb = WaterfallLowDb,
        WaterfallDirectionNormal = WaterfallDirectionNormal,
        GridMaxDb = GridMaxDb,
        GridMinDb = GridMinDb,
        ViewGrid = ViewGrid,
        ShowWaterfall = ShowWaterfall,
        WaterfallPalette = WaterfallPalettes.NormalizeName(WaterfallPalette),
    };

    public void CopyFrom(SpectrumSwBank other)
    {
        WaterfallHighDb = other.WaterfallHighDb;
        WaterfallLowDb = other.WaterfallLowDb;
        WaterfallDirectionNormal = other.WaterfallDirectionNormal;
        GridMaxDb = other.GridMaxDb;
        GridMinDb = other.GridMinDb;
        ViewGrid = other.ViewGrid;
        ShowWaterfall = other.ShowWaterfall;
        WaterfallPalette = WaterfallPalettes.NormalizeName(other.WaterfallPalette);
    }
}

/// <summary>
/// Shared spectrum / waterfall display settings (S/W popup + left zoom).
/// Live fields = active radio bank; HF/LF banks swap with Proficio/Geminus.
/// Client-side only; does not change server pan format.
/// </summary>
public sealed class SpectrumDisplaySettings
{
    public static SpectrumDisplaySettings Instance { get; } = new();

    /// <summary>WPF SpectrumDbCalCenter — absolute offset when relative trim is 0.</summary>
    public const float DbCalCenterAbsolute = -91.3f;

    public event Action? Changed;

    /// <summary>True = Geminus (LF) bank active; false = Proficio (HF).</summary>
    public bool RadioModelIsGeminus { get; private set; }

    public SpectrumSwBank HfBank { get; } = SpectrumSwBank.CreateProficioDefaults();
    public SpectrumSwBank LfBank { get; } = SpectrumSwBank.CreateGeminusDefaults();

    /// <summary>dB CAL relative trim (−20…+20). Global (not banked).</summary>
    public float DbCalRelative { get; private set; }

    public float SpectrumDbOffset => DbCalCenterAbsolute + DbCalRelative;

    // ----- Live (= active bank) -----
    public float GridMaxDb { get; private set; } = -20f;
    public float GridMinDb { get; private set; } = -125f;
    public float WaterfallHighDb { get; private set; } = -50f;
    public float WaterfallLowDb { get; private set; } = -120f;
    public double ZoomFactor { get; private set; } = 1.0;
    public bool ViewGrid { get; private set; } = true;
    public bool ShowWaterfall { get; private set; } = true;
    public bool WaterfallDirectionNormal { get; private set; } = true;
    public string WaterfallPalette { get; private set; } = "Enhanced";

    private SpectrumSwBank ActiveBank => RadioModelIsGeminus ? LfBank : HfBank;

    public SpectrumDisplaySettings()
    {
        // Start with Proficio/HF bank live
        ApplyBankToLive(HfBank, notify: false);
    }

    /// <summary>
    /// Switch radio-type bank: capture live → leaving bank, load entering bank → live.
    /// </summary>
    public void SwitchRadioModel(bool geminus)
    {
        if (geminus == RadioModelIsGeminus)
        {
            // Re-sync live from bank in case of drift
            ApplyBankToLive(ActiveBank, notify: true);
            return;
        }

        CaptureLiveToBank(ActiveBank);
        RadioModelIsGeminus = geminus;
        ApplyBankToLive(ActiveBank, notify: true);
    }

    /// <summary>Copy live S/W fields into the active bank (call before save).</summary>
    public void CaptureLiveToActiveBank() => CaptureLiveToBank(ActiveBank);

    /// <summary>Replace both banks from settings load, then push active model to live.</summary>
    public void LoadBanks(SpectrumSwBank hf, SpectrumSwBank lf, bool geminus, float dbCalRelative, double zoom)
    {
        HfBank.CopyFrom(hf);
        LfBank.CopyFrom(lf);
        RadioModelIsGeminus = geminus;
        DbCalRelative = Math.Clamp(dbCalRelative, -20f, 20f);
        ZoomFactor = Math.Clamp(Math.Round(zoom), 1, 4);
        ApplyBankToLive(ActiveBank, notify: true);
    }

    public void SetDbCalRelative(float relative)
    {
        relative = Math.Clamp(relative, -20f, 20f);
        if (Math.Abs(DbCalRelative - relative) < 0.01f) return;
        DbCalRelative = relative;
        Notify();
    }

    public void SetGrid(float maxDb, float minDb)
    {
        maxDb = Math.Clamp(maxDb, -80f, 0f);
        minDb = Math.Clamp(minDb, -180f, -40f);
        if (maxDb - minDb < 40f)
            minDb = maxDb - 105f;
        if (Math.Abs(GridMaxDb - maxDb) < 0.01f && Math.Abs(GridMinDb - minDb) < 0.01f)
            return;
        GridMaxDb = maxDb;
        GridMinDb = minDb;
        ActiveBank.GridMaxDb = maxDb;
        ActiveBank.GridMinDb = minDb;
        Notify();
    }

    public void SetWaterfallWindow(float highDb, float lowDb)
    {
        highDb = Math.Clamp(highDb, -100f, -10f);
        lowDb = Math.Clamp(lowDb, -160f, -40f);
        if (highDb <= lowDb)
            lowDb = highDb - 40f;
        if (Math.Abs(WaterfallHighDb - highDb) < 0.01f && Math.Abs(WaterfallLowDb - lowDb) < 0.01f)
            return;
        WaterfallHighDb = highDb;
        WaterfallLowDb = lowDb;
        ActiveBank.WaterfallHighDb = highDb;
        ActiveBank.WaterfallLowDb = lowDb;
        Notify();
    }

    public void SetZoomFactor(double zoom)
    {
        zoom = Math.Clamp(Math.Round(zoom), 1, 4);
        if (Math.Abs(ZoomFactor - zoom) < 0.01) return;
        ZoomFactor = zoom;
        Notify();
    }

    public void SetViewGrid(bool on)
    {
        if (ViewGrid == on) return;
        ViewGrid = on;
        ActiveBank.ViewGrid = on;
        Notify();
    }

    public void SetShowWaterfall(bool on)
    {
        if (ShowWaterfall == on) return;
        ShowWaterfall = on;
        ActiveBank.ShowWaterfall = on;
        Notify();
    }

    public void SetWaterfallDirectionNormal(bool normal)
    {
        if (WaterfallDirectionNormal == normal) return;
        WaterfallDirectionNormal = normal;
        ActiveBank.WaterfallDirectionNormal = normal;
        Notify();
    }

    public void SetWaterfallPalette(string name)
    {
        name = WaterfallPalettes.NormalizeName(name);
        if (string.Equals(WaterfallPalette, name, StringComparison.OrdinalIgnoreCase))
            return;
        WaterfallPalette = name;
        ActiveBank.WaterfallPalette = name;
        Notify();
    }

    public void ResetDbCal() => SetDbCalRelative(0f);

    private void CaptureLiveToBank(SpectrumSwBank bank)
    {
        bank.WaterfallHighDb = WaterfallHighDb;
        bank.WaterfallLowDb = WaterfallLowDb;
        bank.WaterfallDirectionNormal = WaterfallDirectionNormal;
        bank.GridMaxDb = GridMaxDb;
        bank.GridMinDb = GridMinDb;
        bank.ViewGrid = ViewGrid;
        bank.ShowWaterfall = ShowWaterfall;
        bank.WaterfallPalette = WaterfallPalettes.NormalizeName(WaterfallPalette);
    }

    private void ApplyBankToLive(SpectrumSwBank bank, bool notify)
    {
        WaterfallHighDb = bank.WaterfallHighDb;
        WaterfallLowDb = bank.WaterfallLowDb;
        WaterfallDirectionNormal = bank.WaterfallDirectionNormal;
        GridMaxDb = bank.GridMaxDb;
        GridMinDb = bank.GridMinDb;
        ViewGrid = bank.ViewGrid;
        ShowWaterfall = bank.ShowWaterfall;
        WaterfallPalette = WaterfallPalettes.NormalizeName(bank.WaterfallPalette);
        if (notify)
            Notify();
    }

    private void Notify() => Changed?.Invoke();
}
