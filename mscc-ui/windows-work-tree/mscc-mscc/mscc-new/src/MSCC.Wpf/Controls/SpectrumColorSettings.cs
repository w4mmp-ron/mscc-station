using System;

namespace MSCC.Wpf.Controls;

/// <summary>
/// Waterfall color schemes matching original MSCC <c>ColorSheme</c> / PALETTE COLOR list.
/// </summary>
public enum WaterfallColorScheme
{
    /// <summary>UI: Red/Yellow — pre-palette WPF heat map (blue→green→yellow→red).</summary>
    RedYellow = 0,
    /// <summary>UI: Enhanced — multi-band rainbow.</summary>
    Enhanced = 1,
    /// <summary>UI: Spectran — blue → white.</summary>
    Spectran = 2,
    /// <summary>UI: BlackWhite — grayscale.</summary>
    BlackWhite = 3,
}

/// <summary>
/// Shared settings for spectrum colors and waterfall palette, controlled by the S/W window.
/// </summary>
public static class SpectrumColorSettings
{
    public static readonly string[] WaterfallPaletteNames =
    {
        "Red/Yellow",
        "Enhanced",
        "Spectran",
        "BlackWhite",
    };

    public static byte FillR { get; private set; } = 0x00;
    public static byte FillG { get; private set; } = 0x9A;
    public static byte FillB { get; private set; } = 0x30;

    public static byte LineR { get; private set; } = 0x20;
    public static byte LineG { get; private set; } = 0xFF;
    public static byte LineB { get; private set; } = 0x5A;

    public static byte BackgroundR { get; private set; } = 0x08;
    public static byte BackgroundG { get; private set; } = 0x08;
    public static byte BackgroundB { get; private set; } = 0x0E;

    public static byte CursorR { get; private set; } = 0x30;
    public static byte CursorG { get; private set; } = 0xA0;
    public static byte CursorB { get; private set; } = 0xFF;

    public static int SpectrumBaseline { get; private set; } = 50;

    /// <summary>
    /// Display dB offset applied after raw panadapter→dB map (phase-1 spectrum cal).
    /// displayDb = rawMappedDb + SpectrumDbOffset. Global for HF/LF (same path is close).
    /// Stored absolute in MSCC_Client.ini as SPECTRUM_DB_OFFSET.
    /// UI is relative to <see cref="SpectrumDbCalCenter"/> (−91.3 = “0” on the slider),
    /// with a fine trim of ±<see cref="SpectrumDbCalUiSpan"/> dB.
    /// </summary>
    public static float SpectrumDbOffset { get; private set; } = SpectrumDbCalCenter;

    /// <summary>Field-proven center cal (slider “0”) after pan bias 40 — ~−91.3 absolute.</summary>
    public const float SpectrumDbCalCenter = -91.3f;

    /// <summary>UI trim range around center (±dB). Enough for HF/LF tweak; not a full scale swing.</summary>
    public const float SpectrumDbCalUiSpan = 20f;

    public static float SpectrumDbOffsetMin => SpectrumDbCalCenter - SpectrumDbCalUiSpan; // −111.3
    public static float SpectrumDbOffsetMax => SpectrumDbCalCenter + SpectrumDbCalUiSpan; // −71.3

    public static void SetSpectrumDbOffset(float offsetDb) =>
        SpectrumDbOffset = Math.Clamp(offsetDb, SpectrumDbOffsetMin, SpectrumDbOffsetMax);

    /// <summary>UI relative trim: 0 = center cal, negative lowers display, positive raises.</summary>
    public static float GetSpectrumDbCalRelative() =>
        Math.Clamp(SpectrumDbOffset - SpectrumDbCalCenter, -SpectrumDbCalUiSpan, SpectrumDbCalUiSpan);

    public static void SetSpectrumDbCalRelative(float relativeDb) =>
        SetSpectrumDbOffset(SpectrumDbCalCenter + relativeDb);

    /// <summary>Apply saved/global cal offset for spectrum + waterfall display.</summary>
    public static float ToDisplayDb(float rawMappedDb) => rawMappedDb + SpectrumDbOffset;

    /// <summary>
    /// PowerSDR-style spectrum view window (absolute dBm after dB CAL).
    /// Defaults favor HF/LF grass near −116 (min ≈ −125) without wasting space at 0 dBm.
    /// </summary>
    public static float SpectrumGridMax { get; private set; } = -20f;
    public static float SpectrumGridMin { get; private set; } = -125f;

    /// <summary>Set spectrum dBm window. Ensures max &gt; min by at least 40 dB for usable grass.</summary>
    public static void SetSpectrumGrid(float maxDb, float minDb)
    {
        maxDb = Math.Clamp(maxDb, -80f, 0f);
        // Allow deep floors (grass ~−116 needs min ≤ −125). Upper clamp −80 was too high as a "min".
        minDb = Math.Clamp(minDb, -180f, -90f);
        if (maxDb - minDb < 40f)
            minDb = maxDb - 105f; // e.g. −20 → −125
        if (minDb >= maxDb)
            minDb = maxDb - 40f;
        SpectrumGridMax = maxDb;
        SpectrumGridMin = minDb;
    }

    /// <summary>dB tick labels from grid max down to min (step 10 or 20).</summary>
    public static int[] BuildDbTicks(float minDb, float maxDb)
    {
        float span = maxDb - minDb;
        int step = span > 80f ? 20 : 10;
        int top = (int)Math.Floor(maxDb / step) * step;
        int bot = (int)Math.Ceiling(minDb / step) * step;
        if (top < bot) (top, bot) = (bot, top);
        var list = new System.Collections.Generic.List<int>();
        for (int db = top; db >= bot; db -= step)
            list.Add(db);
        if (list.Count == 0)
            list.Add((int)Math.Round(maxDb));
        return list.ToArray();
    }

    /// <summary>
    /// When true (Geminus radio model), BASELINE has deeper pull-down range for LF noise floor.
    /// Proficio keeps the milder HF mapping. Set from MainWindow radio-model control.
    /// </summary>
    public static bool GeminusBaselineRange { get; private set; }

    public static void SetGeminusBaselineRange(bool geminus) =>
        GeminusBaselineRange = geminus;

    /// <summary>
    /// Effective spectrum dBm window for drawing (absolute dBm after dB CAL).
    /// GRID MAX is always the top of the pane (no blank strip above the top tick).
    /// BASELINE only fine-tunes the floor (min): higher slider raises min so grass sits lower
    /// without a pixel shift that used to clip −100/−120 ticks and leave empty space above −20.
    /// </summary>
    public static void GetDisplayGridWindow(out float minDb, out float maxDb)
    {
        maxDb = SpectrumGridMax;
        minDb = SpectrumGridMin;
        int delta = SpectrumBaseline - 50;
        // Higher baseline → higher floor (grass lower). Geminus: ±30 dB; Proficio: ±15 dB.
        float floorTuneDb = GeminusBaselineRange ? delta * 0.6f : delta * 0.3f;
        minDb += floorTuneDb;
        if (maxDb - minDb < 40f)
            minDb = maxDb - 40f;
        if (minDb >= maxDb)
            minDb = maxDb - 40f;
        minDb = Math.Clamp(minDb, -180f, maxDb - 40f);
    }

    /// <summary>
    /// Legacy pixel offset — always 0. Absolute GRID MAX/MIN + floor-tune BASELINE replaced
    /// the old Y shift (which left blank space above GRID MAX and clipped deep ticks).
    /// </summary>
    public static int GetBaselineYOffset() => 0;

    /// <summary>When true, draw horizontal dB grid lines on the spectrum (S/W VIEW GRID).</summary>
    public static bool ViewGrid { get; private set; }

    public static void SetViewGrid(bool on) => ViewGrid = on;

    /// <summary>When true, draw dB numeric labels on the spectrum left (S/W dB LABELS).</summary>
    public static bool ViewDbLabels { get; private set; }

    public static void SetViewDbLabels(bool on) => ViewDbLabels = on;

    /// <summary>When true, peak marker (click-to-mark) is active (S/W PEAK MARKER).</summary>
    public static bool PeakMarker { get; private set; }

    public static void SetPeakMarker(bool on) => PeakMarker = on;

    public static string CurrentFillMode { get; private set; } = "SCOPE";

    /// <summary>Active waterfall palette (original MSCC schemes).</summary>
    public static WaterfallColorScheme WaterfallScheme { get; private set; } = WaterfallColorScheme.RedYellow;

    /// <summary>Display name for current waterfall palette (matches S/W list and INI).</summary>
    public static string WaterfallPaletteName { get; private set; } = "Red/Yellow";

    /// <summary>
    /// Waterfall GAIN 0–100 (default 50 = mid of curve). Higher = hotter/more sensitive.
    /// Same slider for both radios; Geminus maps the whole curve colder (see ApplyWaterfallGainZero).
    /// </summary>
    public static int WaterfallGain { get; private set; } = 50;

    // Sensitivity = floor + (gain/100)*span  (same span for both → same “feel”)
    // Proficio: 0.40 … 1.60   Geminus: 0.20 … 1.40  (entire band shifted colder for LF)
    private const float WaterfallSensSpan = 1.20f;
    private const float WaterfallSensFloorProficio = 0.40f;
    private const float WaterfallSensFloorGeminus = 0.20f;

    /// <summary>
    /// Waterfall ZERO 0–100 (default 0 = no offset). Raises the displayed level
    /// (sample offset into the palette), like original zero add.
    /// </summary>
    public static int WaterfallZero { get; private set; } = 0;

    /// <summary>
    /// PowerSDR-style waterfall color window (absolute dBm after dB CAL).
    /// Independent of spectrum GRID MAX/MIN so FT8 ~−80 can be vibrant without
    /// a 100 dB palette span that washes when GAIN is raised.
    /// Defaults: Low −125 (grass cold), High −50 (−70/−80 mid-hot).
    /// </summary>
    public static float WaterfallHighDb { get; private set; } = -50f;
    public static float WaterfallLowDb { get; private set; } = -125f;

    public const float WaterfallHighDefault = -50f;
    public const float WaterfallLowDefault = -125f;

    /// <summary>
    /// Set waterfall palette dBm window.
    /// <paramref name="highIsPrimary"/> = true when the user moved HIGH (keep HIGH; only nudge LOW if needed).
    /// false = user moved LOW (keep LOW; only nudge HIGH if needed).
    /// Never jump LOW by a fixed −75 dB (that slammed cold to −160 when high ≈ −85).
    /// </summary>
    public static void SetWaterfallRange(float highDb, float lowDb, bool highIsPrimary = true)
    {
        // High: top of palette (hot). Typical −80…−20; default −50 for digital contrast.
        highDb = Math.Clamp(highDb, -100f, -20f);
        // Low: black/cold floor. Typical −160…−90; default −125.
        lowDb = Math.Clamp(lowDb, -160f, -90f);

        // Minimum color span so HIGH and LOW cannot cross or collapse (gentle only).
        const float minSpan = 20f;
        if (highDb - lowDb < minSpan)
        {
            if (highIsPrimary)
            {
                // Preserve HIGH; pull LOW down just enough
                lowDb = highDb - minSpan;
                lowDb = Math.Clamp(lowDb, -160f, -90f);
                if (highDb - lowDb < minSpan)
                    highDb = Math.Clamp(lowDb + minSpan, -100f, -20f);
            }
            else
            {
                // Preserve LOW; raise HIGH just enough
                highDb = lowDb + minSpan;
                highDb = Math.Clamp(highDb, -100f, -20f);
                if (highDb - lowDb < minSpan)
                    lowDb = Math.Clamp(highDb - minSpan, -160f, -90f);
            }
        }

        WaterfallHighDb = highDb;
        WaterfallLowDb = lowDb;
    }

    /// <summary>Effective low/high for color mapping (before GAIN/ZERO fine trim).</summary>
    public static void GetWaterfallColorWindow(out float lowDb, out float highDb)
    {
        lowDb = WaterfallLowDb;
        highDb = WaterfallHighDb;
        if (highDb - lowDb < 20f)
        {
            highDb = WaterfallHighDefault;
            lowDb = WaterfallLowDefault;
        }
    }

    /// <summary>
    /// Set waterfall palette by UI/INI name or index string.
    /// Accepts: Red/Yellow, Enhanced, Spectran, BlackWhite (and common aliases).
    /// </summary>
    public static void SetWaterfallPalette(string? name)
    {
        var scheme = ParseWaterfallPalette(name);
        WaterfallScheme = scheme;
        WaterfallPaletteName = WaterfallPaletteNames[(int)scheme];
    }

    public static void SetWaterfallGain(int value) =>
        WaterfallGain = Math.Clamp(value, 0, 100);

    public static void SetWaterfallZero(int value) =>
        WaterfallZero = Math.Clamp(value, 0, 100);

    /// <summary>
    /// Waterfall time-marker interval (original Time_grid / WATERFALL_GRID).
    /// 0 = off; 1–5 = mark every N seconds (UTC mm:ss on the line).
    /// Cycles 0→1→2→3→4→5→0 like original S/W button.
    /// </summary>
    public static int WaterfallTimeMarker { get; private set; } = 0;

    public static void SetWaterfallTimeMarker(int value) =>
        WaterfallTimeMarker = Math.Clamp(value, 0, 5);

    /// <summary>Advance time marker: 0→1→…→5→0.</summary>
    public static int CycleWaterfallTimeMarker()
    {
        int next = WaterfallTimeMarker + 1;
        if (next > 5) next = 0;
        WaterfallTimeMarker = next;
        return WaterfallTimeMarker;
    }

    /// <summary>
    /// true = normal (oldest at top, newest at bottom);
    /// false = reversed (newest at top, oldest at bottom).
    /// Matches original MSCC Direction Normal / Reversed.
    /// </summary>
    public static bool WaterfallDirectionNormal { get; private set; } = true;

    public static void SetWaterfallDirectionNormal(bool normal) =>
        WaterfallDirectionNormal = normal;

    /// <summary>
    /// Apply Gain/Zero fine trim on top of the Waterfall Low/High color window.
    /// Pass low/high from <see cref="GetWaterfallColorWindow"/> (not spectrum GRID).
    /// Zero: adds 0…+30 dB to the sample. Gain compresses the high end (hotter).
    /// </summary>
    public static void ApplyWaterfallGainZero(
        float db, float minDb, float maxDb,
        out float adjDb, out float adjMin, out float adjMax)
    {
        float span = maxDb - minDb;
        if (span < 0.001f) span = 0.001f;

        // Zero: lift sample into the map (original added zero to Y samples)
        float zeroDb = WaterfallZero * 0.30f; // 0..+30 dB
        adjDb = db + zeroDb;

        // Gain: same span, Geminus shifted colder (LF noise floor)
        // Proficio: 0.40 … 1.60   Geminus: 0.20 … 1.40
        float g = WaterfallGain;
        float sensFloor = GeminusBaselineRange
            ? WaterfallSensFloorGeminus
            : WaterfallSensFloorProficio;
        float sens = sensFloor + (g / 100f) * WaterfallSensSpan;
        if (sens < 0.10f) sens = 0.10f;

        adjMin = minDb;
        adjMax = minDb + span / sens;
    }

    public static WaterfallColorScheme ParseWaterfallPalette(string? name)
    {
        if (string.IsNullOrWhiteSpace(name))
            return WaterfallColorScheme.RedYellow;

        string n = name.Trim();
        if (int.TryParse(n, out int idx) && idx >= 0 && idx <= 3)
            return (WaterfallColorScheme)idx;

        n = n.Replace(" ", "", StringComparison.Ordinal).Replace("-", "", StringComparison.Ordinal);
        return n.ToUpperInvariant() switch
        {
            "RED/YELLOW" or "REDYELLOW" or "ORIGINAL" or "0" => WaterfallColorScheme.RedYellow,
            "ENHANCED" or "1" => WaterfallColorScheme.Enhanced,
            "SPECTRAN" or "2" => WaterfallColorScheme.Spectran,
            "BLACKWHITE" or "BLACK/WHITE" or "GRAYSCALE" or "GREYSCALE" or "3" => WaterfallColorScheme.BlackWhite,
            _ => WaterfallColorScheme.RedYellow,
        };
    }

    public static void SetFill(string fillName)
    {
        CurrentFillMode = fillName;

        switch (fillName.ToUpperInvariant())
        {
            case "RED":
                FillR = 0xFF; FillG = 0x00; FillB = 0x00;
                break;
            case "BLUE":
                FillR = 0x00; FillG = 0x00; FillB = 0xFF;
                break;
            case "GREEN":
                FillR = 0x00; FillG = 0x9A; FillB = 0x30;
                break;
            case "YELLOW":
                FillR = 0xFF; FillG = 0xFF; FillB = 0x00;
                break;
            case "WHITE":
                FillR = 0xFF; FillG = 0xFF; FillB = 0xFF;
                break;
            case "BLACK":
                FillR = 0x22; FillG = 0x22; FillB = 0x22;
                break;
            case "SCOPE":
            default:
                FillR = 0x00; FillG = 0x9A; FillB = 0x30;
                break;
        }
    }

    public static void SetLine(string lineName)
    {
        switch (lineName.ToUpperInvariant())
        {
            case "RED":
                LineR = 0xFF; LineG = 0x40; LineB = 0x40;
                break;
            case "BLUE":
                LineR = 0x40; LineG = 0x40; LineB = 0xFF;
                break;
            case "GREEN":
                LineR = 0x20; LineG = 0xFF; LineB = 0x5A;
                break;
            case "YELLOW":
                LineR = 0xFF; LineG = 0xFF; LineB = 0x40;
                break;
            case "WHITE":
                LineR = 0xEE; LineG = 0xEE; LineB = 0xEE;
                break;
            case "BLACK":
                LineR = 0x44; LineG = 0x44; LineB = 0x44;
                break;
            default:
                // keep current
                break;
        }
    }

    public static void SetBackground(string bgName)
    {
        switch ((bgName ?? "").Trim().ToUpperInvariant())
        {
            case "RED":
                BackgroundR = 0xFF; BackgroundG = 0x00; BackgroundB = 0x00;
                break;
            case "BLUE":
                BackgroundR = 0x00; BackgroundG = 0x00; BackgroundB = 0xFF;
                break;
            case "GREEN":
                BackgroundR = 0x00; BackgroundG = 0xFF; BackgroundB = 0x00;
                break;
            case "YELLOW":
                BackgroundR = 0xFF; BackgroundG = 0xFF; BackgroundB = 0x00;
                break;
            case "WHITE":
                BackgroundR = 0xFF; BackgroundG = 0xFF; BackgroundB = 0xFF;
                break;
            case "BLACK":
                BackgroundR = 0x00; BackgroundG = 0x00; BackgroundB = 0x00;
                break;
            case "CUSTOM":
                // RGB applied via SetBackgroundRgb from settings / S/W picker
                break;
            default:
                BackgroundR = 0x08; BackgroundG = 0x08; BackgroundB = 0x0E;
                break;
        }
    }

    /// <summary>Direct RGB for CUSTOM spectrum background (soft darks match waterfall pane).</summary>
    public static void SetBackgroundRgb(byte r, byte g, byte b)
    {
        BackgroundR = r;
        BackgroundG = g;
        BackgroundB = b;
    }

    public static void SetCursor(string cursorName)
    {
        switch (cursorName.ToUpperInvariant())
        {
            case "RED":
                CursorR = 0xFF; CursorG = 0x40; CursorB = 0x40;
                break;
            case "BLUE":
                CursorR = 0x40; CursorG = 0x40; CursorB = 0xFF;
                break;
            case "GREEN":
                CursorR = 0x20; CursorG = 0xFF; CursorB = 0x5A;
                break;
            case "YELLOW":
                CursorR = 0xFF; CursorG = 0xFF; CursorB = 0x40;
                break;
            case "WHITE":
                CursorR = 0xEE; CursorG = 0xEE; CursorB = 0xEE;
                break;
            case "BLACK":
                CursorR = 0x44; CursorG = 0x44; CursorB = 0x44;
                break;
            default:
                CursorR = 0x30; CursorG = 0xA0; CursorB = 0xFF;
                break;
        }
    }

    public static void SetBaseline(int value)
    {
        SpectrumBaseline = Math.Clamp(value, 0, 100);
    }
}
