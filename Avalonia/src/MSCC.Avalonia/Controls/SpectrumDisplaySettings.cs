namespace MSCC.Avalonia.Controls;

/// <summary>
/// One radio-type S/W bank (Proficio/HF or Geminus/LF). WPF dual-bank pattern.
/// </summary>
public sealed class SpectrumSwBank
{
    public float WaterfallHighDb { get; set; } = -40f;
    public float WaterfallLowDb { get; set; } = -150f;
    public bool WaterfallDirectionNormal { get; set; } = false;   // WPF cmd-075 port: Falling default
    public float GridMaxDb { get; set; } = -20f;
    public float GridMinDb { get; set; } = -155f;
    public bool ViewGrid { get; set; } = true;
    public bool ShowWaterfall { get; set; } = true;
    /// <summary>WPF port: BASELINE 0–100 (50 = no change) fine-tunes the spectrum floor.</summary>
    public int Baseline { get; set; } = 50;
    /// <summary>WPF port: waterfall GAIN 0–100 (50 = mid). Higher = hotter.</summary>
    public int WaterfallGain { get; set; } = 50;
    /// <summary>WPF port: waterfall ZERO 0–100 (0 = no offset), adds 0…+30 dB to the sample.</summary>
    public int WaterfallZero { get; set; } = 0;

    /// <summary>Red/Yellow, Enhanced, Spectran, or BlackWhite (per radio bank).</summary>
    public string WaterfallPalette { get; set; } = "Enhanced";

    public static SpectrumSwBank CreateProficioDefaults() => new()
    {
        WaterfallHighDb = -40f, // cmd-069/070 true-dB defaults (WPF ShipHf*)
        WaterfallLowDb = -115f,
        WaterfallDirectionNormal = false,   // WPF cmd-075 port
        GridMaxDb = -20f,
        GridMinDb = -155f,
        ViewGrid = true,
        ShowWaterfall = true,
        WaterfallPalette = "Enhanced",
    };

    public static SpectrumSwBank CreateGeminusDefaults() => new()
    {
        WaterfallHighDb = -35f, // WPF ShipLf*
        WaterfallLowDb = -105f,
        WaterfallDirectionNormal = false,   // WPF cmd-075 port
        GridMaxDb = -20f,
        GridMinDb = -155f,
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
        Baseline = Baseline,
        WaterfallGain = WaterfallGain,
        WaterfallZero = WaterfallZero,
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
        Baseline = other.Baseline;
        WaterfallGain = other.WaterfallGain;
        WaterfallZero = other.WaterfallZero;
    }
}

/// <summary>WPF cmd-075 port: BASELINE + waterfall HIGH/LOW/GAIN/ZERO saved for one band.</summary>
public sealed class SwBandLevels
{
    public int Baseline { get; set; } = 50;
    public float WaterfallHighDb { get; set; }
    public float WaterfallLowDb { get; set; }
    public int WaterfallGain { get; set; } = 50;
    public int WaterfallZero { get; set; }
}

/// <summary>
/// Shared spectrum / waterfall display settings (S/W popup + left zoom).
/// Live fields = active radio bank; HF/LF banks swap with Proficio/Geminus.
/// Client-side only; does not change server pan format.
/// </summary>
public sealed class SpectrumDisplaySettings
{
    public static SpectrumDisplaySettings Instance { get; } = new();

    /// <summary>Absolute dB CAL at trim 0 (true-dB scale, TG step test 2026-10-10; WPF SpectrumDbCalCenter).</summary>
    public const float DbCalCenterAbsolute = -83.6f;

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
    public float GridMinDb { get; private set; } = -155f;
    public float WaterfallHighDb { get; private set; } = -40f;
    public float WaterfallLowDb { get; private set; } = -150f;
    public double ZoomFactor { get; private set; } = 1.0;
    public bool ViewGrid { get; private set; } = true;
    public bool ShowWaterfall { get; private set; } = true;
    public bool WaterfallDirectionNormal { get; private set; } = false;   // WPF cmd-075 port: Falling default
    public string WaterfallPalette { get; private set; } = "Enhanced";
    public int Baseline { get; private set; } = 50;
    public int WaterfallGain { get; private set; } = 50;
    public int WaterfallZero { get; private set; }

    // ----- Global (not banked) — WPF S/W extras -----
    /// <summary>dB numbers on the left of the spectrum (S/W dB LABELS).</summary>
    public bool ViewDbLabels { get; private set; }
    /// <summary>Right-click the spectrum to mark the nearest peak (S/W PEAK MARKER).</summary>
    public bool PeakMarker { get; private set; }
    /// <summary>Spectrum click-to-tune snaps to <see cref="AutoSnapStepHz"/> (not in CW).</summary>
    public bool AutoSnap { get; private set; }
    public int AutoSnapStepHz { get; private set; } = 100;
    /// <summary>Waterfall time marks: 0 = off, 1–5 = a line every N seconds (UTC mm:ss).</summary>
    public int WaterfallTimeMarker { get; private set; }

    /// <summary>Per-band levels (key "20M", "160M", …). Band not in the map keeps the bank values.</summary>
    public Dictionary<string, SwBandLevels> BandLevels { get; } = new(StringComparer.OrdinalIgnoreCase);
    /// <summary>Band whose levels are live ("" = unknown, nothing saved per band).</summary>
    public string ActiveBandKey { get; private set; } = "";

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
            minDb = maxDb - 135f; // e.g. −20 → −155
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
        SaveActiveBandLevels();
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

    public void SetBaseline(int value)
    {
        value = Math.Clamp(value, 0, 100);
        if (Baseline == value) return;
        Baseline = value;
        ActiveBank.Baseline = value;
        SaveActiveBandLevels();
        Notify();
    }

    public void SetWaterfallGain(int value)
    {
        value = Math.Clamp(value, 0, 100);
        if (WaterfallGain == value) return;
        WaterfallGain = value;
        ActiveBank.WaterfallGain = value;
        SaveActiveBandLevels();
        Notify();
    }

    public void SetWaterfallZero(int value)
    {
        value = Math.Clamp(value, 0, 100);
        if (WaterfallZero == value) return;
        WaterfallZero = value;
        ActiveBank.WaterfallZero = value;
        SaveActiveBandLevels();
        Notify();
    }

    public void SetViewDbLabels(bool on) { if (ViewDbLabels == on) return; ViewDbLabels = on; Notify(); }
    public void SetPeakMarker(bool on) { if (PeakMarker == on) return; PeakMarker = on; Notify(); }
    public void SetAutoSnap(bool on) { if (AutoSnap == on) return; AutoSnap = on; Notify(); }

    public void SetAutoSnapStepHz(int hz)
    {
        hz = hz switch { 1000 => 1000, 500 => 500, _ => 100 };
        if (AutoSnapStepHz == hz) return;
        AutoSnapStepHz = hz;
        Notify();
    }

    public void SetWaterfallTimeMarker(int value)
    {
        value = Math.Clamp(value, 0, 5);
        if (WaterfallTimeMarker == value) return;
        WaterfallTimeMarker = value;
        Notify();
    }

    /// <summary>Advance the time marker 0→1→…→5→0 (WPF S/W button).</summary>
    public int CycleWaterfallTimeMarker()
    {
        SetWaterfallTimeMarker(WaterfallTimeMarker >= 5 ? 0 : WaterfallTimeMarker + 1);
        return WaterfallTimeMarker;
    }

    /// <summary>Load the global extras and per-band levels from the settings file.</summary>
    public void LoadExtras(bool dbLabels, bool peakMarker, bool autoSnap, int autoSnapStepHz, int timeMarker,
        IReadOnlyDictionary<string, SwBandLevels>? bandLevels)
    {
        ViewDbLabels = dbLabels;
        PeakMarker = peakMarker;
        AutoSnap = autoSnap;
        AutoSnapStepHz = autoSnapStepHz switch { 1000 => 1000, 500 => 500, _ => 100 };
        WaterfallTimeMarker = Math.Clamp(timeMarker, 0, 5);
        BandLevels.Clear();
        if (bandLevels != null)
            foreach (var kv in bandLevels)
                BandLevels[kv.Key] = kv.Value;
        ActiveBandKey = "";   // next ApplyBand loads the saved levels for the current band
        Notify();
    }

    /// <summary>"20m" / "20M" / "160 m" → "20M"; anything without a number → "".</summary>
    public static string NormalizeBandKey(string? band)
    {
        string b = (band ?? "").Trim().ToUpperInvariant().Replace(" ", "");
        if (b.Length == 0 || !char.IsDigit(b[0])) return "";
        if (!b.EndsWith("M")) b += "M";
        return b;
    }

    /// <summary>
    /// WPF cmd-075 port: switch the live BASELINE + waterfall levels to the band just selected.
    /// A band with nothing saved keeps the current values (they get saved for it on the next change).
    /// </summary>
    public void ApplyBand(string? band)
    {
        string key = NormalizeBandKey(band);
        if (key == ActiveBandKey) return;
        ActiveBandKey = key;
        if (key.Length == 0 || !BandLevels.TryGetValue(key, out var lv)) return;
        Baseline = Math.Clamp(lv.Baseline, 0, 100);
        WaterfallHighDb = Math.Clamp(lv.WaterfallHighDb, -100f, -10f);
        WaterfallLowDb = Math.Clamp(lv.WaterfallLowDb, -160f, -40f);
        if (WaterfallHighDb <= WaterfallLowDb) WaterfallLowDb = WaterfallHighDb - 40f;
        WaterfallGain = Math.Clamp(lv.WaterfallGain, 0, 100);
        WaterfallZero = Math.Clamp(lv.WaterfallZero, 0, 100);
        CaptureLiveToBank(ActiveBank);
        Notify();
    }

    private void SaveActiveBandLevels()
    {
        if (ActiveBandKey.Length == 0) return;
        BandLevels[ActiveBandKey] = new SwBandLevels
        {
            Baseline = Baseline,
            WaterfallHighDb = WaterfallHighDb,
            WaterfallLowDb = WaterfallLowDb,
            WaterfallGain = WaterfallGain,
            WaterfallZero = WaterfallZero,
        };
    }

    /// <summary>
    /// WPF port: spectrum dBm window for drawing. GRID MAX is the top; BASELINE only moves the floor
    /// (higher slider = higher floor, grass lower). Geminus ±30 dB, Proficio ±15 dB. Span ≥ 40 dB.
    /// </summary>
    public void GetDisplayGridWindow(out float minDb, out float maxDb)
    {
        maxDb = GridMaxDb;
        minDb = GridMinDb;
        int delta = Baseline - 50;
        float floorTuneDb = RadioModelIsGeminus ? delta * 0.6f : delta * 0.3f;
        minDb += floorTuneDb;
        if (maxDb - minDb < 40f || minDb >= maxDb)
            minDb = maxDb - 40f;
        minDb = Math.Clamp(minDb, -180f, maxDb - 40f);
    }

    /// <summary>
    /// WPF port: GAIN / ZERO trim on top of the waterfall LOW/HIGH window. ZERO lifts the sample
    /// 0…+30 dB; GAIN narrows the window (hotter). Proficio 0.40…1.60, Geminus 0.20…1.40.
    /// </summary>
    public void ApplyWaterfallGainZero(float db, float minDb, float maxDb,
        out float adjDb, out float adjMin, out float adjMax)
    {
        float span = Math.Max(0.001f, maxDb - minDb);
        adjDb = db + WaterfallZero * 0.30f;
        float sensFloor = RadioModelIsGeminus ? 0.20f : 0.40f;
        float sens = Math.Max(0.10f, sensFloor + (WaterfallGain / 100f) * 1.20f);
        adjMin = minDb;
        adjMax = minDb + span / sens;
    }

    /// <summary>WPF BuildDbTicks: every 20 dB when the span is over 80 dB, else every 10 dB.</summary>
    public static int[] BuildDbTicks(float minDb, float maxDb)
    {
        float span = maxDb - minDb;
        int step = span > 80f ? 20 : 10;
        int top = (int)Math.Floor(maxDb / step) * step;
        int bot = (int)Math.Ceiling(minDb / step) * step;
        if (top < bot) (top, bot) = (bot, top);
        var list = new List<int>();
        for (int db = top; db >= bot; db -= step)
            list.Add(db);
        if (list.Count == 0)
            list.Add((int)Math.Round(maxDb));
        return list.ToArray();
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
        bank.Baseline = Baseline;
        bank.WaterfallGain = WaterfallGain;
        bank.WaterfallZero = WaterfallZero;
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
        Baseline = Math.Clamp(bank.Baseline, 0, 100);
        WaterfallGain = Math.Clamp(bank.WaterfallGain, 0, 100);
        WaterfallZero = Math.Clamp(bank.WaterfallZero, 0, 100);
        if (notify)
            Notify();
    }

    private void Notify() => Changed?.Invoke();
}
