using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows;
using MSCC.Wpf.Controls;

namespace MSCC.Wpf;

/// <summary>
/// Persisted client settings for MSCC (spectrum/waterfall prefs, window layout, UI toggles, etc.).
/// Stored in MSCC_Client.ini (in %LocalAppData%\MSCC-NET9 or C:\mscc-net9\init-files).
/// Loaded at startup (in MainWindow).
/// Saved when user changes relevant values.
/// Matches the original MSCC's use of ini files for persistence.
/// </summary>
public static class SpectrumWaterfallSettings
{
    // Spectrum
    public static string SpectrumFill { get; set; } = "SCOPE";
    public static string SpectrumBackground { get; set; } = "BLUE";
    /// <summary>Hex RGB when <see cref="SpectrumBackground"/> is CUSTOM (e.g. #101018 — soft dark).</summary>
    public static string SpectrumBackgroundRgb { get; set; } = "#101018";
    public static string SpectrumCursor { get; set; } = "WHITE";
    public static string SpectrumLine { get; set; } = "WHITE";
    public static int SpectrumBaseline { get; set; } = 50;
    /// <summary>
    /// Global spectrum/waterfall dB cal (absolute). displayDb = mapped + offset.
    /// Default = SpectrumDbCalCenter (−91.3). UI shows relative trim ±20 around that center.
    /// Stored as SPECTRUM_DB_OFFSET in %LocalAppData%\MSCC-NET9\MSCC_Client.ini.
    /// </summary>
    public static float SpectrumDbOffset { get; set; } = SpectrumColorSettings.SpectrumDbCalCenter;

    /// <summary>Spectrum view top (dBm after cal). PowerSDR SpectrumGridMax. Default −20.</summary>
    public static float SpectrumGridMax { get; set; } = -20f;
    /// <summary>Spectrum view bottom (dBm). PowerSDR SpectrumGridMin. Default −125 (grass ~−116).</summary>
    public static float SpectrumGridMin { get; set; } = -125f;
    public static bool SpectrumViewGrid { get; set; } = false;
    /// <summary>dB scale labels on spectrum left (S/W); independent of VIEW GRID.</summary>
    public static bool SpectrumViewDbLabels { get; set; } = false;
    /// <summary>Peak marker on spectrum (click places marker 1 + freq/level readout).</summary>
    public static bool SpectrumPeakMarker { get; set; } = false;

    // Waterfall (live = active radio model bank: Proficio/HF or Geminus/LF)
    /// <summary>0–100; 50 = neutral sensitivity. Higher = hotter waterfall.</summary>
    public static int WaterfallGain { get; set; } = 50;
    /// <summary>0–100; 0 = no offset. Higher = lift levels into the palette.</summary>
    public static int WaterfallZero { get; set; } = 0;
    /// <summary>Palette top dBm (after dB CAL). Default −50 so FT8 ~−80 is vibrant.</summary>
    public static float WaterfallHighDb { get; set; } = SpectrumColorSettings.WaterfallHighDefault;
    /// <summary>Palette floor dBm. Default −125 (grass cold). Independent of spectrum GRID.</summary>
    public static float WaterfallLowDb { get; set; } = SpectrumColorSettings.WaterfallLowDefault;
    /// <summary>Original MSCC palette: Red/Yellow, Enhanced, Spectran, BlackWhite.</summary>
    public static string WaterfallPalette { get; set; } = "Red/Yellow";
    /// <summary>0 = off; 1–5 = mark every N seconds (original Time_grid).</summary>
    public static int WaterfallTimeMarker { get; set; } = 0;
    public static bool WaterfallDirectionNormal { get; set; } = true;

    /// <summary>True = Geminus (LF) waterfall bank; false = Proficio (HF). Sticky RADIO_MODEL.</summary>
    public static bool RadioModelIsGeminus { get; set; }

    // CQ / keyer memory text (client sticky only — not read back from radio). Max 48 printable each.
    public static string KeyerMem0 { get; set; } = "";
    public static string KeyerMem1 { get; set; } = "";
    public static string KeyerMem2 { get; set; } = "";
    public static string KeyerMem3 { get; set; } = "";

    /// <summary>
    /// True = external electronic keyer / legacy radio (mscc.ini PROFICIO-MKII=0).
    /// False (default) = Proficio MKII internal keyer (PROFICIO-MKII=1).
    /// Applied by ms-sdr at process start only — flip mid-session needs Stop/Start.
    /// </summary>
    public static bool ExternalElectronicKeyer { get; set; }

    /// <summary>
    /// When Phones audio path is selected: use remote operator mic (CMD_SET_AUDIO_DEVICE=2).
    /// Ignored while Digital is selected (always sends 0). Sticky client pref.
    /// </summary>
    public static bool RemoteAudio { get; set; }

    // HF (Proficio) bank — field-tuned defaults (FT8 contrast on 20m)
    public static float WaterfallHfHighDb { get; set; } = -44f;
    public static float WaterfallHfLowDb { get; set; } = -106f;
    public static int WaterfallHfGain { get; set; } = 65;
    public static int WaterfallHfZero { get; set; }
    public static string WaterfallHfPalette { get; set; } = "Enhanced";
    public static bool WaterfallHfDirectionNormal { get; set; } = true;

    // LF (Geminus) bank — conservative defaults until tuned on 630/2200
    public static float WaterfallLfHighDb { get; set; } = SpectrumColorSettings.WaterfallHighDefault;
    public static float WaterfallLfLowDb { get; set; } = SpectrumColorSettings.WaterfallLowDefault;
    public static int WaterfallLfGain { get; set; } = 50;
    public static int WaterfallLfZero { get; set; }
    public static string WaterfallLfPalette { get; set; } = "Enhanced";
    public static bool WaterfallLfDirectionNormal { get; set; } = true;

    /// <summary>Copy live waterfall controls into the HF or LF bank.</summary>
    public static void CaptureLiveWaterfallToBank(bool geminus)
    {
        if (geminus)
        {
            WaterfallLfHighDb = WaterfallHighDb;
            WaterfallLfLowDb = WaterfallLowDb;
            WaterfallLfGain = WaterfallGain;
            WaterfallLfZero = WaterfallZero;
            WaterfallLfPalette = WaterfallPalette ?? "Enhanced";
            WaterfallLfDirectionNormal = WaterfallDirectionNormal;
        }
        else
        {
            WaterfallHfHighDb = WaterfallHighDb;
            WaterfallHfLowDb = WaterfallLowDb;
            WaterfallHfGain = WaterfallGain;
            WaterfallHfZero = WaterfallZero;
            WaterfallHfPalette = WaterfallPalette ?? "Enhanced";
            WaterfallHfDirectionNormal = WaterfallDirectionNormal;
        }
    }

    /// <summary>Load HF or LF bank into live waterfall fields (not ColorSettings — call Apply after).</summary>
    public static void LoadWaterfallBankToLive(bool geminus)
    {
        if (geminus)
        {
            WaterfallHighDb = WaterfallLfHighDb;
            WaterfallLowDb = WaterfallLfLowDb;
            WaterfallGain = WaterfallLfGain;
            WaterfallZero = WaterfallLfZero;
            WaterfallPalette = WaterfallLfPalette ?? "Enhanced";
            WaterfallDirectionNormal = WaterfallLfDirectionNormal;
        }
        else
        {
            WaterfallHighDb = WaterfallHfHighDb;
            WaterfallLowDb = WaterfallHfLowDb;
            WaterfallGain = WaterfallHfGain;
            WaterfallZero = WaterfallHfZero;
            WaterfallPalette = WaterfallHfPalette ?? "Enhanced";
            WaterfallDirectionNormal = WaterfallHfDirectionNormal;
        }
    }

    /// <summary>
    /// Switch radio model bank: save current live waterfall to the leaving bank,
    /// load the other bank into live + renderer.
    /// </summary>
    public static void SwitchRadioModelWaterfall(bool nowGeminus)
    {
        if (nowGeminus == RadioModelIsGeminus)
        {
            // Still re-apply live in case banks and live drifted
            LoadWaterfallBankToLive(nowGeminus);
            ApplyLiveWaterfallToRenderer();
            return;
        }
        CaptureLiveWaterfallToBank(RadioModelIsGeminus);
        RadioModelIsGeminus = nowGeminus;
        LoadWaterfallBankToLive(nowGeminus);
        ApplyLiveWaterfallToRenderer();
        Save();
    }

    /// <summary>Push live waterfall fields into SpectrumColorSettings (renderer).</summary>
    public static void ApplyLiveWaterfallToRenderer()
    {
        SpectrumColorSettings.SetWaterfallPalette(WaterfallPalette);
        WaterfallPalette = SpectrumColorSettings.WaterfallPaletteName;
        SpectrumColorSettings.SetWaterfallGain(WaterfallGain);
        SpectrumColorSettings.SetWaterfallZero(WaterfallZero);
        SpectrumColorSettings.SetWaterfallRange(WaterfallHighDb, WaterfallLowDb);
        WaterfallHighDb = SpectrumColorSettings.WaterfallHighDb;
        WaterfallLowDb = SpectrumColorSettings.WaterfallLowDb;
        WaterfallGain = SpectrumColorSettings.WaterfallGain;
        WaterfallZero = SpectrumColorSettings.WaterfallZero;
        SpectrumColorSettings.SetWaterfallTimeMarker(WaterfallTimeMarker);
        SpectrumColorSettings.SetWaterfallDirectionNormal(WaterfallDirectionNormal);
        SpectrumColorSettings.SetGeminusBaselineRange(RadioModelIsGeminus);
    }

    /// <summary>
    /// After any live waterfall UI change: update active bank + INI.
    /// Call instead of plain Save() when waterfall high/low/gain/zero/palette/direction change.
    /// </summary>
    public static void SaveLiveWaterfallAndActiveBank()
    {
        CaptureLiveWaterfallToBank(RadioModelIsGeminus);
        Save();
    }

    // Spectrum/Waterfall combined
    public static int SpectrumRefresh { get; set; } = 4;
    public static string SpectrumAverage { get; set; } = "Maximum";
    public static string SpectrumFilterMarker { get; set; } = "WHITE";
    /// <summary>AUTO SNAP checkbox (S/W popup). When true, spectrum click-to-tune snaps (non-CW).</summary>
    public static bool SpectrumAutoSnap { get; set; } = true;
    /// <summary>Snap step label: "1KHz", "500Hz", or "100Hz" (original listBox2).</summary>
    public static string SpectrumAutoSnapFreq { get; set; } = "100Hz";

    /// <summary>Snap step in Hz from <see cref="SpectrumAutoSnapFreq"/> (1000 / 500 / 100).</summary>
    public static int GetAutoSnapStepHz()
    {
        string s = (SpectrumAutoSnapFreq ?? "").Trim();
        if (s.Equals("1KHz", StringComparison.OrdinalIgnoreCase) ||
            s.Equals("1kHz", StringComparison.OrdinalIgnoreCase) ||
            s.Equals("1000", StringComparison.OrdinalIgnoreCase) ||
            s.Equals("1000Hz", StringComparison.OrdinalIgnoreCase))
            return 1000;
        if (s.Equals("500Hz", StringComparison.OrdinalIgnoreCase) ||
            s.Equals("500", StringComparison.OrdinalIgnoreCase))
            return 500;
        if (s.Equals("100Hz", StringComparison.OrdinalIgnoreCase) ||
            s.Equals("100", StringComparison.OrdinalIgnoreCase))
            return 100;
        return 100;
    }

    /// <summary>
    /// Apply original Set_Spectrum_Frequency auto-snap rounding to a click frequency.
    /// 1 kHz: nearest (half rounds up). 500 / 100: floor to step (matches original).
    /// </summary>
    public static long ApplyAutoSnap(long freqHz)
    {
        if (freqHz < 0) freqHz = 0;
        int snap = GetAutoSnapStepHz();
        if (snap <= 0) return freqHz;

        int delta = (int)(freqHz % snap);
        if (delta == 0) return freqHz;

        // Mirror original Main_Form.Set_Spectrum_Frequency cases
        switch (snap)
        {
            case 1000:
                if (delta > snap / 2)
                    return freqHz + (snap - delta);
                return freqHz - delta;

            case 500:
            {
                long floored = freqHz - delta;
                // Original also checks %1000 then re-floors; result is floor-to-500 either way
                return floored;
            }

            case 100:
            {
                long floored = freqHz - delta;
                return floored;
            }

            default:
                // Nearest for any other step
                if (delta > snap / 2)
                    return freqHz + (snap - delta);
                return freqHz - delta;
        }
    }

    // Window placement (client UI layout)
    public static double WindowLeft { get; set; } = double.NaN;
    public static double WindowTop { get; set; } = double.NaN;
    public static double WindowWidth { get; set; } = double.NaN;
    public static double WindowHeight { get; set; } = double.NaN;
    public static WindowState WindowState { get; set; } = WindowState.Normal;

    // UI prefs
    public static bool TimeDisplayOn { get; set; } = false;  // persisted via TIME_DISPLAY=1/0 in ini
    /// <summary>If true, call Start automatically when the main window loads.</summary>
    public static bool AutoStartServers { get; set; } = false;  // AUTO_START_SERVERS=1/0

    /// <summary>
    /// Main window background name: RED…BLACK or CUSTOM.
    /// Default BLACK = Multus deep #1C1C1C.
    /// </summary>
    public static string UiBackground { get; set; } = "BLACK";

    /// <summary>Hex RGB when <see cref="UiBackground"/> is CUSTOM (e.g. #1C1C1C).</summary>
    public static string UiBackgroundRgb { get; set; } = "#1C1C1C";

    /// <summary>
    /// Button face name: RED…BLACK or CUSTOM. Default YELLOW = Multus gold.
    /// </summary>
    public static string UiButton { get; set; } = "YELLOW";

    /// <summary>Hex RGB when <see cref="UiButton"/> is CUSTOM (e.g. #FFCC00).</summary>
    public static string UiButtonRgb { get; set; } = "#FFCC00";

    /// <summary>
    /// Panel fill: AUTO (lighter than background) or RED…BLACK.
    /// No CUSTOM — when background is CUSTOM, panel is always auto-lift of that color.
    /// </summary>
    public static string UiPanel { get; set; } = "AUTO";

    /// <summary>
    /// If true (default), Start also spawns ms-sdr / recv / trans from the app folder.
    /// If false, Start only connects over UDP (backends must already be running).
    /// </summary>
    public static bool LaunchServersOnStart { get; set; } = true;  // LAUNCH_SERVERS=1/0

    // Analog meter ballistics (client-side; sticky across restarts)
    /// <summary>S-meter HOLD (slow fall). Default on.</summary>
    public static bool SmeterHold { get; set; } = true;
    /// <summary>S-meter Peak needle. Default off.</summary>
    public static bool SmeterPeak { get; set; } = false;
    /// <summary>ALC HOLD (slow fall). Default on.</summary>
    public static bool AlcHold { get; set; } = true;
    /// <summary>ALC Peak needle. Default off.</summary>
    public static bool AlcPeak { get; set; } = false;

    // Rx/Tx power levels (persisted in client INI, restored on startup, updated on change; bidirectional with other server)
    /// <summary>Last active Tune power (legacy key TUNE_POWER; mirrors current AMP-state value).</summary>
    public static int TunePower { get; set; } = 10;
    /// <summary>Tune power when AMP is off (QRP path).</summary>
    public static int TunePowerAmpOff { get; set; } = 10;
    /// <summary>Tune power when AMP is on (QRO path).</summary>
    public static int TunePowerAmpOn { get; set; } = 10;
    public static int CwPower { get; set; } = 50;
    public static int SsbPower { get; set; } = 50;
    public static int AmCarrier { get; set; } = 30;

    /// <summary>
    /// Panadapter resolution index: 0=Normal 800, 1=High 1600, 2=Max 3200 bins across 72 kHz.
    /// </summary>
    public static int PanResolutionIndex { get; set; } = 0;

    /// <summary>Bins for current PanResolutionIndex.</summary>
    public static int PanResolutionBins => PanResolutionIndex switch
    {
        1 => 1600,
        2 => 3200,
        _ => 800
    };

    /// <summary>UI label for S/W list.</summary>
    public static string PanResolutionLabel => PanResolutionIndex switch
    {
        1 => "High (1600)",
        2 => "Max (3200)",
        _ => "Normal (800)"
    };

    // Per-mode filter profiles (global; independent of band last-used).
    // Indices match MAIN Lo/Hi cut buttons and CW filter. DIG-U defaults narrower for digi.
    public static int ModeUsbLowCut { get; set; } = 4;   // 75 Hz
    public static int ModeUsbHighCut { get; set; } = 1;  // 4.0 kHz
    public static int ModeLsbLowCut { get; set; } = 4;
    public static int ModeLsbHighCut { get; set; } = 1;
    public static int ModeAmLowCut { get; set; } = 4;
    public static int ModeAmHighCut { get; set; } = 0;   // 5.5 kHz
    public static int ModeCwLowCut { get; set; } = 4;
    public static int ModeCwHighCut { get; set; } = 1;
    public static int ModeCwFilter { get; set; } = 2;    // 200 Hz
    public static int ModeDigULowCut { get; set; } = 1;  // 300 Hz
    public static int ModeDigUHighCut { get; set; } = 4; // 2.4 kHz

    /// <summary>Save Lo/Hi (and CW filter) profile for a mode key: USB, LSB, AM, CW, DIG-U.</summary>
    public static void SaveModeFilterProfile(string modeKey, int lowCut, int highCut, int cwFilter)
    {
        lowCut = Math.Clamp(lowCut, 0, 4);
        highCut = Math.Clamp(highCut, 0, 4);
        cwFilter = Math.Clamp(cwFilter, 0, 2);
        switch (NormalizeModeProfileKey(modeKey))
        {
            case "USB":
                ModeUsbLowCut = lowCut; ModeUsbHighCut = highCut; break;
            case "LSB":
                ModeLsbLowCut = lowCut; ModeLsbHighCut = highCut; break;
            case "AM":
                ModeAmLowCut = lowCut; ModeAmHighCut = highCut; break;
            case "CW":
                ModeCwLowCut = lowCut; ModeCwHighCut = highCut; ModeCwFilter = cwFilter; break;
            case "DIG-U":
                ModeDigULowCut = lowCut; ModeDigUHighCut = highCut; break;
        }
    }

    /// <summary>Load profile; returns (-1,-1,-1) if key unknown.</summary>
    public static (int lowCut, int highCut, int cwFilter) LoadModeFilterProfile(string modeKey)
    {
        return NormalizeModeProfileKey(modeKey) switch
        {
            "USB" => (ModeUsbLowCut, ModeUsbHighCut, -1),
            "LSB" => (ModeLsbLowCut, ModeLsbHighCut, -1),
            "AM" => (ModeAmLowCut, ModeAmHighCut, -1),
            "CW" => (ModeCwLowCut, ModeCwHighCut, ModeCwFilter),
            "DIG-U" => (ModeDigULowCut, ModeDigUHighCut, -1),
            _ => (-1, -1, -1)
        };
    }

    private static string NormalizeModeProfileKey(string modeKey)
    {
        string m = (modeKey ?? "").Trim().ToUpperInvariant().Replace('_', '-');
        if (m is "DIGU" or "DIG") return "DIG-U";
        return m;
    }

    /// <summary>Tune power stored for the given AMP state.</summary>
    public static int GetTunePowerForAmp(bool ampOn) =>
        Math.Clamp(ampOn ? TunePowerAmpOn : TunePowerAmpOff, 0, 100);

    /// <summary>Store Tune power for the given AMP state and keep legacy TUNE_POWER in sync.</summary>
    public static void SetTunePowerForAmp(bool ampOn, int percent)
    {
        percent = Math.Clamp(percent, 0, 100);
        if (ampOn)
            TunePowerAmpOn = percent;
        else
            TunePowerAmpOff = percent;
        TunePower = percent;
    }

    // Step index for VFO A wheel tuning (under CONTROLS on Main tab). Persisted in client INI.
    // 0=100kHz ... 5=1Hz. Default 5 (1Hz) matches original init.
    public static int StepIndex { get; set; } = 5;

    private static string _iniPath = "";
    /// <summary>VFO A last-used bands: MSCC_LastUsed.ini</summary>
    private static string _lastUsedIniPath = "";
    /// <summary>VFO B last-used bands: MSCC_LastUsed_VFOB.ini (independent of A)</summary>
    private static string _lastUsedVfoBIniPath = "";

    static SpectrumWaterfallSettings()
    {
        Load();
    }

    /// <summary>
    /// Loads from MSCC_Client.ini (searches appdata, C:\mscc-net9/init-files). Creates with defaults if missing.
    /// Client uses this file exclusively for its settings.
    /// </summary>
    public static void Load(string? iniPath = null)
    {
        string appData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        string primaryIni = Path.Combine(appData, "MSCC-NET9", "MSCC_Client.ini");
        string initFilesIni = @"C:\mscc-net9\init-files\MSCC_Client.ini";

        if (string.IsNullOrEmpty(iniPath))
        {
            if (File.Exists(primaryIni))
            {
                iniPath = primaryIni;
            }
            else if (File.Exists(initFilesIni))
            {
                iniPath = initFilesIni;
            }
            else
            {
                // For first run / no existing, create in the primary (installed) location
                iniPath = primaryIni;
            }
        }

        _iniPath = iniPath;

        if (!File.Exists(iniPath))
        {
            // Create using Save (it will ensure primary appdata path + dir + defaults)
            Save();

            // Re-resolve to the file Save actually wrote (primary)
            iniPath = _iniPath;

            // Seed default connection parameters so ConnectionSettings will find them on next run / same process
            var lines = File.Exists(iniPath) ? File.ReadAllLines(iniPath).ToList() : new List<string>();
            bool changed = false;
            if (!lines.Any(l => l.Contains("PROFICIO_DLL_IP")))
            {
                lines.Add("PROFICIO_DLL_IP=127.0.0.1;");
                changed = true;
            }
            if (!lines.Any(l => l.Contains("PROFICIO_DLL_PORT")))
            {
                lines.Add("PROFICIO_DLL_PORT=8888;");
                changed = true;
            }
            if (!lines.Any(l => l.Contains("MSCC_PORT")))
            {
                lines.Add("MSCC_PORT=8889;");
                changed = true;
            }
            if (!lines.Any(l => LineMatchesKey(l, "TUNE_POWER")))
            {
                lines.Add("TUNE_POWER=10;");
                changed = true;
            }
            if (!lines.Any(l => LineMatchesKey(l, "TUNE_POWER_AMP_OFF")))
            {
                lines.Add("TUNE_POWER_AMP_OFF=10;");
                changed = true;
            }
            if (!lines.Any(l => LineMatchesKey(l, "TUNE_POWER_AMP_ON")))
            {
                lines.Add("TUNE_POWER_AMP_ON=10;");
                changed = true;
            }
            if (!lines.Any(l => l.Contains("CW_POWER")))
            {
                lines.Add("CW_POWER=50;");
                changed = true;
            }
            if (!lines.Any(l => l.Contains("SSB_POWER")))
            {
                lines.Add("SSB_POWER=50;");
                changed = true;
            }
            if (!lines.Any(l => l.Contains("AM_CARRIER")))
            {
                lines.Add("AM_CARRIER=30;");
                changed = true;
            }
            if (!lines.Any(l => l.Contains("STEP_INDEX")))
            {
                lines.Add("STEP_INDEX=5;");
                changed = true;
            }
            if (changed)
                File.WriteAllLines(iniPath, lines);
        }

        EnsureLastUsedPaths();
        EnsureLastUsedFileExists(_lastUsedIniPath);
        EnsureLastUsedFileExists(_lastUsedVfoBIniPath);

        if (File.Exists(iniPath))
        {
            try
            {
                var fileLines = File.ReadAllLines(iniPath);
                foreach (string line in fileLines)
                {
                    if (line.Contains("SPECTRUM_FILL")) SpectrumFill = ParseIniString(line, SpectrumFill);
                    if (LineMatchesKey(line, "SPECTRUM_BACKGROUND")) SpectrumBackground = ParseIniString(line, SpectrumBackground);
                    if (LineMatchesKey(line, "SPECTRUM_BACKGROUND_RGB")) SpectrumBackgroundRgb = ParseIniString(line, SpectrumBackgroundRgb);
                    if (line.Contains("SPECTRUM_CURSOR")) SpectrumCursor = ParseIniString(line, SpectrumCursor);
                    if (line.Contains("SPECTRUM_LINE")) SpectrumLine = ParseIniString(line, SpectrumLine);
                    if (line.Contains("SPECTRUM_BASELINE")) SpectrumBaseline = ParseIniInt(line, SpectrumBaseline);
                    if (LineMatchesKey(line, "SPECTRUM_DB_OFFSET"))
                        SpectrumDbOffset = Math.Clamp(ParseIniFloat(line, SpectrumDbOffset),
                            SpectrumColorSettings.SpectrumDbOffsetMin,
                            SpectrumColorSettings.SpectrumDbOffsetMax);
                    if (LineMatchesKey(line, "SPECTRUM_GRID_MAX"))
                        SpectrumGridMax = Math.Clamp(ParseIniFloat(line, SpectrumGridMax), -80f, 0f);
                    if (LineMatchesKey(line, "SPECTRUM_GRID_MIN"))
                        SpectrumGridMin = Math.Clamp(ParseIniFloat(line, SpectrumGridMin), -180f, -90f);
                    // Repair earlier bugs:
                    // - slider max −60/−80 forced MIN up (ticks only −80…−20)
                    // - S/W open ValueChanged clamp wrote GRID_MIN=-100 (slider Maximum) and lost −120 tick
                    // - same open path wrote SPECTRUM_DB_OFFSET=-20 (DbCal Maximum) wiping cal
                    if (SpectrumGridMin > -120f)
                        SpectrumGridMin = -125f;
                    if (SpectrumGridMax > -10f)
                        SpectrumGridMax = -20f;
                    if (SpectrumGridMax - SpectrumGridMin < 40f)
                    {
                        SpectrumGridMax = -20f;
                        SpectrumGridMin = -125f;
                    }
                    // Old S/W open clobber wrote −20; values outside the ±20 center window snap to center.
                    if (SpectrumDbOffset >= -40f || SpectrumDbOffset < SpectrumColorSettings.SpectrumDbOffsetMin - 0.5f
                        || SpectrumDbOffset > SpectrumColorSettings.SpectrumDbOffsetMax + 0.5f)
                        SpectrumDbOffset = SpectrumColorSettings.SpectrumDbCalCenter;
                    if (line.Contains("SPECTRUM_VIEW_GRID")) SpectrumViewGrid = ParseIniBool(line, SpectrumViewGrid);
                    if (LineMatchesKey(line, "SPECTRUM_VIEW_DB_LABELS")) SpectrumViewDbLabels = ParseIniBool(line, SpectrumViewDbLabels);
                    if (LineMatchesKey(line, "SPECTRUM_PEAK_MARKER")) SpectrumPeakMarker = ParseIniBool(line, SpectrumPeakMarker);

                    // Live waterfall (legacy single keys) — also seed banks if bank keys missing
                    if (LineMatchesKey(line, "WATERFALL_GAIN"))
                        WaterfallGain = Math.Clamp(ParseIniInt(line, WaterfallGain), 0, 100);
                    if (LineMatchesKey(line, "WATERFALL_ZERO"))
                        WaterfallZero = Math.Clamp(ParseIniInt(line, WaterfallZero), 0, 100);
                    if (LineMatchesKey(line, "WATERFALL_HIGH"))
                        WaterfallHighDb = ParseIniFloat(line, WaterfallHighDb);
                    if (LineMatchesKey(line, "WATERFALL_LOW"))
                        WaterfallLowDb = ParseIniFloat(line, WaterfallLowDb);
                    if (line.Contains("WATERFALL_PALETTE") || line.Contains("WATERFALL_PALLET"))
                        WaterfallPalette = ParseIniString(line, WaterfallPalette);
                    if (line.Contains("WATERFALL_TIME_MARKER") || line.Contains("WATERFALL_GRID"))
                        WaterfallTimeMarker = Math.Clamp(ParseIniInt(line, WaterfallTimeMarker), 0, 5);
                    if (LineMatchesKey(line, "WATERFALL_DIRECTION_NORMAL"))
                        WaterfallDirectionNormal = ParseIniBool(line, WaterfallDirectionNormal);

                    // Dual banks: Proficio (HF) / Geminus (LF)
                    if (LineMatchesKey(line, "WATERFALL_HF_HIGH"))
                        WaterfallHfHighDb = ParseIniFloat(line, WaterfallHfHighDb);
                    if (LineMatchesKey(line, "WATERFALL_HF_LOW"))
                        WaterfallHfLowDb = ParseIniFloat(line, WaterfallHfLowDb);
                    if (LineMatchesKey(line, "WATERFALL_HF_GAIN"))
                        WaterfallHfGain = Math.Clamp(ParseIniInt(line, WaterfallHfGain), 0, 100);
                    if (LineMatchesKey(line, "WATERFALL_HF_ZERO"))
                        WaterfallHfZero = Math.Clamp(ParseIniInt(line, WaterfallHfZero), 0, 100);
                    if (LineMatchesKey(line, "WATERFALL_HF_PALETTE"))
                        WaterfallHfPalette = ParseIniString(line, WaterfallHfPalette);
                    if (LineMatchesKey(line, "WATERFALL_HF_DIRECTION_NORMAL"))
                        WaterfallHfDirectionNormal = ParseIniBool(line, WaterfallHfDirectionNormal);

                    if (LineMatchesKey(line, "WATERFALL_LF_HIGH"))
                        WaterfallLfHighDb = ParseIniFloat(line, WaterfallLfHighDb);
                    if (LineMatchesKey(line, "WATERFALL_LF_LOW"))
                        WaterfallLfLowDb = ParseIniFloat(line, WaterfallLfLowDb);
                    if (LineMatchesKey(line, "WATERFALL_LF_GAIN"))
                        WaterfallLfGain = Math.Clamp(ParseIniInt(line, WaterfallLfGain), 0, 100);
                    if (LineMatchesKey(line, "WATERFALL_LF_ZERO"))
                        WaterfallLfZero = Math.Clamp(ParseIniInt(line, WaterfallLfZero), 0, 100);
                    if (LineMatchesKey(line, "WATERFALL_LF_PALETTE"))
                        WaterfallLfPalette = ParseIniString(line, WaterfallLfPalette);
                    if (LineMatchesKey(line, "WATERFALL_LF_DIRECTION_NORMAL"))
                        WaterfallLfDirectionNormal = ParseIniBool(line, WaterfallLfDirectionNormal);

                    if (LineMatchesKey(line, "RADIO_MODEL"))
                    {
                        string rm = ParseIniString(line, "Proficio");
                        RadioModelIsGeminus = rm.Equals("Geminus", StringComparison.OrdinalIgnoreCase)
                            || rm.Equals("LF", StringComparison.OrdinalIgnoreCase);
                    }

                    if (line.Contains("SPECTRUM_REFRESH")) SpectrumRefresh = ParseIniInt(line, SpectrumRefresh);
                    if (line.Contains("SPECTRUM_AVERAGE")) SpectrumAverage = ParseIniString(line, SpectrumAverage);
                    if (line.Contains("SPECTRUM_FILTER_MARKER")) SpectrumFilterMarker = ParseIniString(line, SpectrumFilterMarker);
                    if (line.Contains("SPECTRUM_AUTO_SNAP")) SpectrumAutoSnap = ParseIniBool(line, SpectrumAutoSnap);
                    if (line.Contains("SPECTRUM_AUTO_SNAP_FREQ")) SpectrumAutoSnapFreq = ParseIniString(line, SpectrumAutoSnapFreq);

                    if (line.Contains("WINDOW_LEFT")) WindowLeft = ParseIniDouble(line, WindowLeft);
                    if (line.Contains("WINDOW_TOP")) WindowTop = ParseIniDouble(line, WindowTop);
                    if (line.Contains("WINDOW_WIDTH")) WindowWidth = ParseIniDouble(line, WindowWidth);
                    if (line.Contains("WINDOW_HEIGHT")) WindowHeight = ParseIniDouble(line, WindowHeight);
                    if (line.Contains("WINDOW_STATE")) WindowState = ParseIniWindowState(line, WindowState);
                    if (line.Contains("TIME_DISPLAY")) TimeDisplayOn = ParseIniBool(line, TimeDisplayOn);
                    if (LineMatchesKey(line, "AUTO_START_SERVERS")) AutoStartServers = ParseIniBool(line, AutoStartServers);
                    if (LineMatchesKey(line, "LAUNCH_SERVERS")) LaunchServersOnStart = ParseIniBool(line, LaunchServersOnStart);
                    if (LineMatchesKey(line, "UI_BACKGROUND")) UiBackground = ParseIniString(line, UiBackground);
                    if (LineMatchesKey(line, "UI_BACKGROUND_RGB")) UiBackgroundRgb = ParseIniString(line, UiBackgroundRgb);
                    if (LineMatchesKey(line, "UI_BUTTON")) UiButton = ParseIniString(line, UiButton);
                    if (LineMatchesKey(line, "UI_BUTTON_RGB")) UiButtonRgb = ParseIniString(line, UiButtonRgb);
                    if (LineMatchesKey(line, "UI_PANEL")) UiPanel = ParseIniString(line, UiPanel);

                    if (LineMatchesKey(line, "SMETER_HOLD")) SmeterHold = ParseIniBool(line, SmeterHold);
                    if (LineMatchesKey(line, "SMETER_PEAK")) SmeterPeak = ParseIniBool(line, SmeterPeak);
                    if (LineMatchesKey(line, "ALC_HOLD")) AlcHold = ParseIniBool(line, AlcHold);
                    if (LineMatchesKey(line, "ALC_PEAK")) AlcPeak = ParseIniBool(line, AlcPeak);

                    if (LineMatchesKey(line, "TUNE_POWER")) TunePower = ParseIniInt(line, TunePower);
                    if (LineMatchesKey(line, "TUNE_POWER_AMP_OFF")) TunePowerAmpOff = ParseIniInt(line, TunePowerAmpOff);
                    if (LineMatchesKey(line, "TUNE_POWER_AMP_ON")) TunePowerAmpOn = ParseIniInt(line, TunePowerAmpOn);
                    if (LineMatchesKey(line, "CW_POWER")) CwPower = ParseIniInt(line, CwPower);
                    if (LineMatchesKey(line, "SSB_POWER")) SsbPower = ParseIniInt(line, SsbPower);
                    if (LineMatchesKey(line, "AM_CARRIER")) AmCarrier = ParseIniInt(line, AmCarrier);
                    if (LineMatchesKey(line, "STEP_INDEX")) StepIndex = ParseIniInt(line, StepIndex);
                    if (LineMatchesKey(line, "PAN_RESOLUTION"))
                    {
                        int pr = ParseIniInt(line, PanResolutionIndex);
                        PanResolutionIndex = Math.Clamp(pr, 0, 2);
                    }

                    if (LineMatchesKey(line, "MODE_USB_LOWCUT")) ModeUsbLowCut = ParseIniInt(line, ModeUsbLowCut);
                    if (LineMatchesKey(line, "MODE_USB_HIGHCUT")) ModeUsbHighCut = ParseIniInt(line, ModeUsbHighCut);
                    if (LineMatchesKey(line, "MODE_LSB_LOWCUT")) ModeLsbLowCut = ParseIniInt(line, ModeLsbLowCut);
                    if (LineMatchesKey(line, "MODE_LSB_HIGHCUT")) ModeLsbHighCut = ParseIniInt(line, ModeLsbHighCut);
                    if (LineMatchesKey(line, "MODE_AM_LOWCUT")) ModeAmLowCut = ParseIniInt(line, ModeAmLowCut);
                    if (LineMatchesKey(line, "MODE_AM_HIGHCUT")) ModeAmHighCut = ParseIniInt(line, ModeAmHighCut);
                    if (LineMatchesKey(line, "MODE_CW_LOWCUT")) ModeCwLowCut = ParseIniInt(line, ModeCwLowCut);
                    if (LineMatchesKey(line, "MODE_CW_HIGHCUT")) ModeCwHighCut = ParseIniInt(line, ModeCwHighCut);
                    if (LineMatchesKey(line, "MODE_CW_FILTER")) ModeCwFilter = ParseIniInt(line, ModeCwFilter);
                    if (LineMatchesKey(line, "MODE_DIGU_LOWCUT")) ModeDigULowCut = ParseIniInt(line, ModeDigULowCut);
                    if (LineMatchesKey(line, "MODE_DIGU_HIGHCUT")) ModeDigUHighCut = ParseIniInt(line, ModeDigUHighCut);

                    if (LineMatchesKey(line, "KEYER_MEM0")) KeyerMem0 = ClampKeyerMemText(ParseIniString(line, KeyerMem0));
                    if (LineMatchesKey(line, "KEYER_MEM1")) KeyerMem1 = ClampKeyerMemText(ParseIniString(line, KeyerMem1));
                    if (LineMatchesKey(line, "KEYER_MEM2")) KeyerMem2 = ClampKeyerMemText(ParseIniString(line, KeyerMem2));
                    if (LineMatchesKey(line, "KEYER_MEM3")) KeyerMem3 = ClampKeyerMemText(ParseIniString(line, KeyerMem3));
                    if (LineMatchesKey(line, "EXTERNAL_ELECTRONIC_KEYER"))
                        ExternalElectronicKeyer = ParseIniBool(line, ExternalElectronicKeyer);
                    if (LineMatchesKey(line, "REMOTE_AUDIO"))
                        RemoteAudio = ParseIniBool(line, RemoteAudio);
                }

                // Migrate legacy single TUNE_POWER → dual AMP stores when new keys were missing.
                bool hasAmpOff = fileLines.Any(l => LineMatchesKey(l, "TUNE_POWER_AMP_OFF"));
                bool hasAmpOn = fileLines.Any(l => LineMatchesKey(l, "TUNE_POWER_AMP_ON"));
                if (!hasAmpOff) TunePowerAmpOff = TunePower;
                if (!hasAmpOn) TunePowerAmpOn = TunePower;

                // Migrate single WATERFALL_* → dual HF/LF banks if bank keys never written.
                bool hasHfBank = fileLines.Any(l => LineMatchesKey(l, "WATERFALL_HF_HIGH"));
                bool hasLfBank = fileLines.Any(l => LineMatchesKey(l, "WATERFALL_LF_HIGH"));
                if (!hasHfBank)
                {
                    // Live keys (or defaults) become the HF bank — preserves your tuned 20m look.
                    WaterfallHfHighDb = WaterfallHighDb;
                    WaterfallHfLowDb = WaterfallLowDb;
                    WaterfallHfGain = WaterfallGain;
                    WaterfallHfZero = WaterfallZero;
                    WaterfallHfPalette = WaterfallPalette;
                    WaterfallHfDirectionNormal = WaterfallDirectionNormal;
                }
                if (!hasLfBank)
                {
                    // First run of dual banks: LF starts from defaults (not a copy of HF).
                    // Leave WaterfallLf* as property defaults unless live was already Geminus.
                    if (RadioModelIsGeminus)
                    {
                        WaterfallLfHighDb = WaterfallHighDb;
                        WaterfallLfLowDb = WaterfallLowDb;
                        WaterfallLfGain = WaterfallGain;
                        WaterfallLfZero = WaterfallZero;
                        WaterfallLfPalette = WaterfallPalette;
                        WaterfallLfDirectionNormal = WaterfallDirectionNormal;
                    }
                }

                // Active bank → live (so Proficio vs Geminus restores correctly)
                LoadWaterfallBankToLive(RadioModelIsGeminus);
            }
            catch { /* use defaults */ }
        }

        // Ensure the file contains default values for all client settings (in case it was created minimally elsewhere
        // or on first run). Save merges with existing content (e.g. connection keys).
        Save();

        // Apply to the renderer settings
        SpectrumColorSettings.SetFill(SpectrumFill);
        SpectrumColorSettings.SetLine(SpectrumLine);
        ApplySpectrumBackgroundToRenderer();
        SpectrumColorSettings.SetCursor(SpectrumCursor);
        SpectrumColorSettings.SetBaseline(SpectrumBaseline);
        SpectrumColorSettings.SetSpectrumDbOffset(SpectrumDbOffset);
        SpectrumDbOffset = SpectrumColorSettings.SpectrumDbOffset;
        SpectrumColorSettings.SetSpectrumGrid(SpectrumGridMax, SpectrumGridMin);
        SpectrumGridMax = SpectrumColorSettings.SpectrumGridMax;
        SpectrumGridMin = SpectrumColorSettings.SpectrumGridMin;
        ApplyLiveWaterfallToRenderer();
        SpectrumColorSettings.SetViewGrid(SpectrumViewGrid);
        SpectrumColorSettings.SetViewDbLabels(SpectrumViewDbLabels);
        SpectrumColorSettings.SetPeakMarker(SpectrumPeakMarker);
        WaterfallTimeMarker = SpectrumColorSettings.WaterfallTimeMarker;
    }

    /// <summary>Named or CUSTOM spectrum background → <see cref="SpectrumColorSettings"/>.</summary>
    public static void ApplySpectrumBackgroundToRenderer()
    {
        if (string.Equals((SpectrumBackground ?? "").Trim(), "CUSTOM", StringComparison.OrdinalIgnoreCase))
        {
            var c = UiChromeTheme.TryParseHex(SpectrumBackgroundRgb);
            if (c.HasValue)
                SpectrumColorSettings.SetBackgroundRgb(c.Value.R, c.Value.G, c.Value.B);
            else
                SpectrumColorSettings.SetBackgroundRgb(0x10, 0x10, 0x18);
        }
        else
        {
            SpectrumColorSettings.SetBackground(SpectrumBackground);
        }
    }

    /// <summary>
    /// Saves current values back to MSCC_Client.ini (creates/updates keys, preserves other content).
    /// </summary>
    public static void Save()
    {
        if (string.IsNullOrEmpty(_iniPath))
        {
            // ensure primary location for MSCC_Client.ini
            string appData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            _iniPath = Path.Combine(appData, "MSCC-NET9", "MSCC_Client.ini");
        }
        Directory.CreateDirectory(Path.GetDirectoryName(_iniPath)!);

        var lines = File.Exists(_iniPath) ? File.ReadAllLines(_iniPath).ToList() : new List<string>();

        UpdateOrAdd(lines, "SPECTRUM_FILL", SpectrumFill);
        UpdateOrAdd(lines, "SPECTRUM_BACKGROUND", SpectrumBackground);
        UpdateOrAdd(lines, "SPECTRUM_BACKGROUND_RGB", SpectrumBackgroundRgb);
        UpdateOrAdd(lines, "SPECTRUM_CURSOR", SpectrumCursor);
        UpdateOrAdd(lines, "SPECTRUM_LINE", SpectrumLine);
        UpdateOrAdd(lines, "SPECTRUM_BASELINE", SpectrumBaseline.ToString());
        UpdateOrAdd(lines, "SPECTRUM_DB_OFFSET",
            SpectrumDbOffset.ToString("0.##", System.Globalization.CultureInfo.InvariantCulture));
        UpdateOrAdd(lines, "SPECTRUM_GRID_MAX",
            SpectrumGridMax.ToString("0.##", System.Globalization.CultureInfo.InvariantCulture));
        UpdateOrAdd(lines, "SPECTRUM_GRID_MIN",
            SpectrumGridMin.ToString("0.##", System.Globalization.CultureInfo.InvariantCulture));
        UpdateOrAdd(lines, "SPECTRUM_VIEW_GRID", SpectrumViewGrid ? "1" : "0");
        UpdateOrAdd(lines, "SPECTRUM_VIEW_DB_LABELS", SpectrumViewDbLabels ? "1" : "0");
        UpdateOrAdd(lines, "SPECTRUM_PEAK_MARKER", SpectrumPeakMarker ? "1" : "0");

        // Live (active bank mirror) + dual HF/LF banks
        UpdateOrAdd(lines, "WATERFALL_GAIN", WaterfallGain.ToString());
        UpdateOrAdd(lines, "WATERFALL_ZERO", WaterfallZero.ToString());
        UpdateOrAdd(lines, "WATERFALL_HIGH",
            WaterfallHighDb.ToString("0.##", System.Globalization.CultureInfo.InvariantCulture));
        UpdateOrAdd(lines, "WATERFALL_LOW",
            WaterfallLowDb.ToString("0.##", System.Globalization.CultureInfo.InvariantCulture));
        UpdateOrAdd(lines, "WATERFALL_PALETTE", WaterfallPalette);
        UpdateOrAdd(lines, "WATERFALL_TIME_MARKER", WaterfallTimeMarker.ToString());
        UpdateOrAdd(lines, "WATERFALL_DIRECTION_NORMAL", WaterfallDirectionNormal ? "1" : "0");

        UpdateOrAdd(lines, "RADIO_MODEL", RadioModelIsGeminus ? "Geminus" : "Proficio");
        UpdateOrAdd(lines, "WATERFALL_HF_HIGH",
            WaterfallHfHighDb.ToString("0.##", System.Globalization.CultureInfo.InvariantCulture));
        UpdateOrAdd(lines, "WATERFALL_HF_LOW",
            WaterfallHfLowDb.ToString("0.##", System.Globalization.CultureInfo.InvariantCulture));
        UpdateOrAdd(lines, "WATERFALL_HF_GAIN", WaterfallHfGain.ToString());
        UpdateOrAdd(lines, "WATERFALL_HF_ZERO", WaterfallHfZero.ToString());
        UpdateOrAdd(lines, "WATERFALL_HF_PALETTE", WaterfallHfPalette);
        UpdateOrAdd(lines, "WATERFALL_HF_DIRECTION_NORMAL", WaterfallHfDirectionNormal ? "1" : "0");
        UpdateOrAdd(lines, "WATERFALL_LF_HIGH",
            WaterfallLfHighDb.ToString("0.##", System.Globalization.CultureInfo.InvariantCulture));
        UpdateOrAdd(lines, "WATERFALL_LF_LOW",
            WaterfallLfLowDb.ToString("0.##", System.Globalization.CultureInfo.InvariantCulture));
        UpdateOrAdd(lines, "WATERFALL_LF_GAIN", WaterfallLfGain.ToString());
        UpdateOrAdd(lines, "WATERFALL_LF_ZERO", WaterfallLfZero.ToString());
        UpdateOrAdd(lines, "WATERFALL_LF_PALETTE", WaterfallLfPalette);
        UpdateOrAdd(lines, "WATERFALL_LF_DIRECTION_NORMAL", WaterfallLfDirectionNormal ? "1" : "0");

        UpdateOrAdd(lines, "SPECTRUM_REFRESH", SpectrumRefresh.ToString());
        UpdateOrAdd(lines, "SPECTRUM_AVERAGE", SpectrumAverage);
        UpdateOrAdd(lines, "SPECTRUM_FILTER_MARKER", SpectrumFilterMarker);
        UpdateOrAdd(lines, "SPECTRUM_AUTO_SNAP", SpectrumAutoSnap ? "1" : "0");
        UpdateOrAdd(lines, "SPECTRUM_AUTO_SNAP_FREQ", SpectrumAutoSnapFreq);

        UpdateOrAdd(lines, "WINDOW_LEFT", WindowLeft.ToString(System.Globalization.CultureInfo.InvariantCulture));
        UpdateOrAdd(lines, "WINDOW_TOP", WindowTop.ToString(System.Globalization.CultureInfo.InvariantCulture));
        UpdateOrAdd(lines, "WINDOW_WIDTH", WindowWidth.ToString(System.Globalization.CultureInfo.InvariantCulture));
        UpdateOrAdd(lines, "WINDOW_HEIGHT", WindowHeight.ToString(System.Globalization.CultureInfo.InvariantCulture));
        UpdateOrAdd(lines, "WINDOW_STATE", WindowState.ToString());
        UpdateOrAdd(lines, "TIME_DISPLAY", TimeDisplayOn ? "1" : "0");
        UpdateOrAdd(lines, "AUTO_START_SERVERS", AutoStartServers ? "1" : "0");
        UpdateOrAdd(lines, "LAUNCH_SERVERS", LaunchServersOnStart ? "1" : "0");
        UpdateOrAdd(lines, "UI_BACKGROUND", UiBackground);
        UpdateOrAdd(lines, "UI_BACKGROUND_RGB", UiBackgroundRgb);
        UpdateOrAdd(lines, "UI_BUTTON", UiButton);
        UpdateOrAdd(lines, "UI_BUTTON_RGB", UiButtonRgb);
        UpdateOrAdd(lines, "UI_PANEL", UiPanel);

        UpdateOrAdd(lines, "SMETER_HOLD", SmeterHold ? "1" : "0");
        UpdateOrAdd(lines, "SMETER_PEAK", SmeterPeak ? "1" : "0");
        UpdateOrAdd(lines, "ALC_HOLD", AlcHold ? "1" : "0");
        UpdateOrAdd(lines, "ALC_PEAK", AlcPeak ? "1" : "0");

        UpdateOrAdd(lines, "TUNE_POWER", TunePower.ToString());
        UpdateOrAdd(lines, "TUNE_POWER_AMP_OFF", TunePowerAmpOff.ToString());
        UpdateOrAdd(lines, "TUNE_POWER_AMP_ON", TunePowerAmpOn.ToString());
        UpdateOrAdd(lines, "CW_POWER", CwPower.ToString());
        UpdateOrAdd(lines, "SSB_POWER", SsbPower.ToString());
        UpdateOrAdd(lines, "AM_CARRIER", AmCarrier.ToString());
        UpdateOrAdd(lines, "STEP_INDEX", StepIndex.ToString());
        UpdateOrAdd(lines, "PAN_RESOLUTION", Math.Clamp(PanResolutionIndex, 0, 2).ToString());

        UpdateOrAdd(lines, "MODE_USB_LOWCUT", ModeUsbLowCut.ToString());
        UpdateOrAdd(lines, "MODE_USB_HIGHCUT", ModeUsbHighCut.ToString());
        UpdateOrAdd(lines, "MODE_LSB_LOWCUT", ModeLsbLowCut.ToString());
        UpdateOrAdd(lines, "MODE_LSB_HIGHCUT", ModeLsbHighCut.ToString());
        UpdateOrAdd(lines, "MODE_AM_LOWCUT", ModeAmLowCut.ToString());
        UpdateOrAdd(lines, "MODE_AM_HIGHCUT", ModeAmHighCut.ToString());
        UpdateOrAdd(lines, "MODE_CW_LOWCUT", ModeCwLowCut.ToString());
        UpdateOrAdd(lines, "MODE_CW_HIGHCUT", ModeCwHighCut.ToString());
        UpdateOrAdd(lines, "MODE_CW_FILTER", ModeCwFilter.ToString());
        UpdateOrAdd(lines, "MODE_DIGU_LOWCUT", ModeDigULowCut.ToString());
        UpdateOrAdd(lines, "MODE_DIGU_HIGHCUT", ModeDigUHighCut.ToString());

        UpdateOrAdd(lines, "KEYER_MEM0", EscapeKeyerMemForIni(KeyerMem0));
        UpdateOrAdd(lines, "KEYER_MEM1", EscapeKeyerMemForIni(KeyerMem1));
        UpdateOrAdd(lines, "KEYER_MEM2", EscapeKeyerMemForIni(KeyerMem2));
        UpdateOrAdd(lines, "KEYER_MEM3", EscapeKeyerMemForIni(KeyerMem3));
        UpdateOrAdd(lines, "EXTERNAL_ELECTRONIC_KEYER", ExternalElectronicKeyer ? "1" : "0");
        UpdateOrAdd(lines, "REMOTE_AUDIO", RemoteAudio ? "1" : "0");

        File.WriteAllLines(_iniPath, lines);
    }

    /// <summary>Printable ASCII only, max 48 — matches PIC / Core sanitize rules.</summary>
    public static string ClampKeyerMemText(string? text)
    {
        if (string.IsNullOrEmpty(text)) return "";
        var sb = new System.Text.StringBuilder(Math.Min(text.Length, 48));
        foreach (char c in text)
        {
            if (c is >= (char)0x20 and <= (char)0x7E)
            {
                sb.Append(c);
                if (sb.Length >= 48) break;
            }
        }
        return sb.ToString();
    }

    private static string EscapeKeyerMemForIni(string? text)
    {
        // One INI line: strip CR/LF; Clamp already removes non-printable.
        string s = ClampKeyerMemText(text);
        return s.Replace("\r", "").Replace("\n", " ");
    }

    /// <summary>
    /// Updates PROFICIO_DLL_IP and PROFICIO_DLL_PORT in MSCC_Client.ini when user changes
    /// the Server address on the Main tab. Does NOT re-initialize the service or anything else.
    /// The user must stop and restart MSCC for the new address to be used.
    /// </summary>
    public static void UpdateServerAddress(string ip, int port)
    {
        if (string.IsNullOrEmpty(_iniPath))
        {
            Load();
        }
        if (string.IsNullOrEmpty(_iniPath))
            return;

        var lines = File.Exists(_iniPath) ? File.ReadAllLines(_iniPath).ToList() : new List<string>();
        UpdateOrAdd(lines, "PROFICIO_DLL_IP", ip);
        UpdateOrAdd(lines, "PROFICIO_DLL_PORT", port.ToString());
        File.WriteAllLines(_iniPath, lines);
    }

    /// <summary>
    /// Resolves paths for VFO A (MSCC_LastUsed.ini) and VFO B (MSCC_LastUsed_VFOB.ini).
    /// Prefers %LocalAppData%\MSCC-NET9; falls back to C:\mscc-net9\init-files if only that exists.
    /// </summary>
    private static void EnsureLastUsedPaths()
    {
        if (string.IsNullOrEmpty(_lastUsedIniPath))
            _lastUsedIniPath = ResolveLastUsedPath("MSCC_LastUsed.ini");
        if (string.IsNullOrEmpty(_lastUsedVfoBIniPath))
            _lastUsedVfoBIniPath = ResolveLastUsedPath("MSCC_LastUsed_VFOB.ini");
    }

    private static string ResolveLastUsedPath(string fileName)
    {
        string appData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        string primary = Path.Combine(appData, "MSCC-NET9", fileName);
        string initFiles = Path.Combine(@"C:\mscc-net9\init-files", fileName);
        if (File.Exists(primary))
            return primary;
        if (File.Exists(initFiles))
            return initFiles;
        return primary;
    }

    private static void EnsureLastUsedFileExists(string path)
    {
        if (string.IsNullOrEmpty(path) || File.Exists(path)) return;
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        File.WriteAllLines(path, new List<string>());
    }

    private static string GetLastUsedPath(bool forVfoB)
    {
        EnsureLastUsedPaths();
        return forVfoB ? _lastUsedVfoBIniPath : _lastUsedIniPath;
    }

    /// <summary>
    /// Saves last-used freq/mode/filters for a band into the active VFO's last-used file.
    /// VFO A → MSCC_LastUsed.ini; VFO B → MSCC_LastUsed_VFOB.ini.
    /// </summary>
    /// <param name="forVfoB">True when the active VFO is B (use VFO B file).</param>
    public static void SaveLastUsedForBand(string band, long frequencyHz, string mode, int lowCutIndex, int highCutIndex, int cwFilterIndex, bool forVfoB = false)
    {
        string path = GetLastUsedPath(forVfoB);
        if (string.IsNullOrEmpty(path)) return;

        string b = NormalizeBandKey(band);
        // Refuse unknown/placeholder bands so we never write "?_FREQ" or corrupt the file.
        if (string.IsNullOrEmpty(b) || b == "?" || !GetValidBandPrefixes().Contains(b))
            return;

        var lines = File.Exists(path) ? File.ReadAllLines(path).ToList() : new List<string>();
        // Cleanup legacy/invalid band keys (e.g. old "200M"). Match on exact KEY= only.
        var validPrefixes = GetValidBandPrefixes();
        lines = lines.Where(l =>
        {
            if (string.IsNullOrWhiteSpace(l)) return false;
            string? lineKey = GetIniKey(l);
            if (lineKey == null) return false;
            int us = lineKey.IndexOf('_');
            if (us <= 0) return false;
            string prefix = lineKey.Substring(0, us).ToUpperInvariant();
            return validPrefixes.Contains(prefix);
        }).ToList();

        UpdateOrAdd(lines, $"{b}_FREQ", frequencyHz.ToString());
        UpdateOrAdd(lines, $"{b}_MODE", mode);
        UpdateOrAdd(lines, $"{b}_LOWCUT", lowCutIndex.ToString());
        UpdateOrAdd(lines, $"{b}_HIGHCUT", highCutIndex.ToString());
        UpdateOrAdd(lines, $"{b}_CWFILTER", cwFilterIndex.ToString());
        EnsureLastUsedFileExists(path);
        File.WriteAllLines(path, lines);
    }

    /// <summary>
    /// Loads last-used for a band from VFO A or VFO B file depending on <paramref name="forVfoB"/>.
    /// </summary>
    public static (long freq, string mode, int lowCut, int highCut, int cwFilter) LoadLastUsedForBand(string band, bool forVfoB = false)
    {
        string path = GetLastUsedPath(forVfoB);
        long freq = 0;
        string mode = "";
        int lowCut = -1;
        int highCut = -1;
        int cwFilter = -1;
        if (string.IsNullOrEmpty(path) || !File.Exists(path))
            return (freq, mode, lowCut, highCut, cwFilter);

        string b = NormalizeBandKey(band);
        if (string.IsNullOrEmpty(b) || b == "?")
            return (freq, mode, lowCut, highCut, cwFilter);

        // Exact KEY= match only. Never use string.Contains — "60M_FREQ" is a substring of "160M_FREQ"
        // and would load/save the wrong band (same for other overlapping names).
        string kFreq = $"{b}_FREQ";
        string kMode = $"{b}_MODE";
        string kLow = $"{b}_LOWCUT";
        string kHigh = $"{b}_HIGHCUT";
        string kCw = $"{b}_CWFILTER";

        foreach (string line in File.ReadAllLines(path))
        {
            if (LineMatchesKey(line, kFreq)) freq = ParseIniLong(line, freq);
            else if (LineMatchesKey(line, kMode)) mode = ParseIniString(line, mode);
            else if (LineMatchesKey(line, kLow)) lowCut = ParseIniInt(line, lowCut);
            else if (LineMatchesKey(line, kHigh)) highCut = ParseIniInt(line, highCut);
            else if (LineMatchesKey(line, kCw)) cwFilter = ParseIniInt(line, cwFilter);
        }
        return (freq, mode, lowCut, highCut, cwFilter);
    }

    private static string NormalizeBandKey(string band)
    {
        // "40m" / "gen" → "40M" / "GEN"
        string b = (band ?? "").Trim().ToUpperInvariant();
        if (b is "GEN" or "GENERAL")
            return "GEN";
        return b;
    }

    /// <summary>
    /// True if line is exactly KEY=value (case-insensitive key). Avoids substring false matches
    /// (e.g. 60M inside 160M).
    /// </summary>
    private static bool LineMatchesKey(string line, string key)
    {
        string? lineKey = GetIniKey(line);
        return lineKey != null &&
               string.Equals(lineKey, key, StringComparison.OrdinalIgnoreCase);
    }

    private static string? GetIniKey(string line)
    {
        if (string.IsNullOrWhiteSpace(line)) return null;
        int eq = line.IndexOf('=');
        if (eq <= 0) return null;
        return line.Substring(0, eq).Trim();
    }

    private static void UpdateOrAdd(List<string> lines, string key, string value)
    {
        bool found = false;
        for (int i = 0; i < lines.Count; i++)
        {
            if (LineMatchesKey(lines[i], key))
            {
                lines[i] = $"{key}={value};";
                found = true;
                break;
            }
        }
        if (!found)
        {
            lines.Add($"{key}={value};");
        }
    }

    private static int ParseIniInt(string line, int defaultValue)
    {
        if (!line.Contains("=")) return defaultValue;
        int eq = line.IndexOf('=');
        string val = line.Substring(eq + 1).Trim().TrimEnd(';', ' ');
        return int.TryParse(val, out int result) ? result : defaultValue;
    }

    private static string ParseIniString(string line, string defaultValue)
    {
        if (!line.Contains("=")) return defaultValue;
        int eq = line.IndexOf('=');
        string val = line.Substring(eq + 1).Trim().TrimEnd(';', ' ');
        return string.IsNullOrWhiteSpace(val) ? defaultValue : val;
    }

    private static bool ParseIniBool(string line, bool defaultValue)
    {
        if (!line.Contains("=")) return defaultValue;
        int eq = line.IndexOf('=');
        string val = line.Substring(eq + 1).Trim().TrimEnd(';', ' ').ToLower();
        if (val == "1" || val == "true" || val == "yes") return true;
        if (val == "0" || val == "false" || val == "no") return false;
        return defaultValue;
    }

    private static long ParseIniLong(string line, long defaultValue)
    {
        if (!line.Contains("=")) return defaultValue;
        int eq = line.IndexOf('=');
        string val = line.Substring(eq + 1).Trim().TrimEnd(';', ' ');
        return long.TryParse(val, out long result) ? result : defaultValue;
    }

    private static List<string> GetValidBandPrefixes()
    {
        return new List<string> { "160M", "80M", "60M", "40M", "30M", "20M", "17M", "15M", "12M", "10M", "GEN" };
    }

    private static double ParseIniDouble(string line, double defaultValue)
    {
        if (!line.Contains("=")) return defaultValue;
        int eq = line.IndexOf('=');
        string val = line.Substring(eq + 1).Trim().TrimEnd(';', ' ');
        return double.TryParse(val, System.Globalization.NumberStyles.Any, System.Globalization.CultureInfo.InvariantCulture, out double result) ? result : defaultValue;
    }

    private static float ParseIniFloat(string line, float defaultValue)
    {
        if (!line.Contains("=")) return defaultValue;
        int eq = line.IndexOf('=');
        string val = line.Substring(eq + 1).Trim().TrimEnd(';', ' ');
        return float.TryParse(val, System.Globalization.NumberStyles.Any, System.Globalization.CultureInfo.InvariantCulture, out float result)
            ? result
            : defaultValue;
    }

    private static WindowState ParseIniWindowState(string line, WindowState defaultValue)
    {
        if (!line.Contains("=")) return defaultValue;
        int eq = line.IndexOf('=');
        string val = line.Substring(eq + 1).Trim().TrimEnd(';', ' ');
        if (Enum.TryParse<WindowState>(val, true, out var state))
            return state;
        return defaultValue;
    }
}
