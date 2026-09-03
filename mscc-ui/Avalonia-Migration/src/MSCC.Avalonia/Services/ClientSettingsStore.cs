using System.Globalization;
using System.Text;
using MSCC.Avalonia.Controls;

namespace MSCC.Avalonia.Services;

/// <summary>
/// Client-side sticky settings (INI-style key=value).
/// Linux: ~/.config/MSCC/mscc-avalonia.ini
/// Windows: %LocalAppData%\MSCC\mscc-avalonia.ini
///
/// Sections conceptually:
///   Connection, Radio model, VFO, Operate (audio/filters/CW/power/NB…),
///   Global S/W, HF/LF banks, spectrum colors, UI chrome, GEN.
/// PTT/TUN are never sticky (safety).
/// </summary>
public sealed class ClientSettings
{
    // Connection
    public string Host { get; set; } = "127.0.0.1";
    public string RemotePortText { get; set; } = "8888";
    public string LocalPortText { get; set; } = "8889";

    // Radio model
    public bool IsGeminusRadioModel { get; set; }

    // VFO
    public long LastFrequencyHz { get; set; } = 7_000_000;
    public string LastMode { get; set; } = "USB";
    public long LastVfoBFrequencyHz { get; set; } = 14_200_000;
    public string LastVfoBMode { get; set; } = "USB";
    public bool UseVfoA { get; set; } = true;

    // Operate — filters / step (indices)
    public int StepIndex { get; set; } = 2; // 1 kHz
    public int LowCutIndex { get; set; }
    public int HighCutIndex { get; set; } = 2; // 2.7k
    public int CwFilterIndex { get; set; }

    // Audio paths
    public int PVolume { get; set; } = 50;
    public int PMicGain { get; set; } = 40;
    public int DVolume { get; set; } = 50;
    public int DMicGain { get; set; } = 40;
    public bool IsDigitalAudio { get; set; }

    /// <summary>
    /// With Phones path: CMD_SET_AUDIO_DEVICE=2 (remote mic). Ignored while Digital.
    /// </summary>
    public bool RemoteAudio { get; set; }

    // RIT (offset only; On restored off for safety unless user re-enables)
    public int RitOffset { get; set; }
    public bool RitOn { get; set; }

    // CW
    public int CwKeyerMode { get; set; } = 1;
    public int CwSpacing { get; set; }
    public int CwPaddle { get; set; }
    public int CwWeightIndex { get; set; } = 1;
    public int CwPitchIndex { get; set; } = 1;
    public int CwHold { get; set; } = 100;
    public bool CwQsk { get; set; }
    public bool CwPhones { get; set; }
    public int CwSpeed { get; set; } = 20;
    /// <summary>Keyer CQ memory slot 0..3 text (client-side only; no radio read-back).</summary>
    public string KeyerMem0 { get; set; } = "";
    public string KeyerMem1 { get; set; } = "";
    public string KeyerMem2 { get; set; } = "";
    public string KeyerMem3 { get; set; } = "";

    /// <summary>
    /// True = external electronic keyer / legacy (mscc.ini PROFICIO-MKII=0).
    /// False (default) = Proficio MKII internal keyer (PROFICIO-MKII=1).
    /// </summary>
    public bool ExternalElectronicKeyer { get; set; }

    // Power banks (server-backed)
    public int TunePowerPercent { get; set; } = 25;
    public int CwPowerPercent { get; set; } = 40;
    public int SsbPowerPercent { get; set; } = 50;
    public int AmCarrierPercent { get; set; } = 30;

    // DSP / process
    public int Compression { get; set; }
    public bool CompressionOn { get; set; }
    public int AgcLevel { get; set; }
    public int AgcFastRelease { get; set; } = 50;
    public bool NbOn { get; set; }
    public int NbPulse { get; set; } = 10;
    public int NbThreshold { get; set; } = 20;
    public bool NrOn { get; set; }
    public int NrLevel { get; set; } = 40;
    public bool AnOn { get; set; }
    public bool MonitorOn { get; set; }
    public bool AmpOn { get; set; }
    public bool QrpMode { get; set; } = true;
    public bool FullPower { get; set; }
    public bool AlcOn { get; set; }

    // Global S/W (not banked)
    public double SpectrumZoom { get; set; } = 1;
    public float DbCalRelative { get; set; }

    // Live snapshot (mirrored from active bank for backward-compatible keys)
    public float GridMaxDb { get; set; } = -20f;
    public float GridMinDb { get; set; } = -125f;
    public float WaterfallHighDb { get; set; } = -50f;
    public float WaterfallLowDb { get; set; } = -120f;
    public bool ViewGrid { get; set; } = true;
    public bool ShowWaterfall { get; set; } = true;
    public bool WaterfallDirectionNormal { get; set; } = true;
    public string WaterfallPalette { get; set; } = "Enhanced";

    // Per-radio S/W banks
    public SpectrumSwBank HfBank { get; set; } = SpectrumSwBank.CreateProficioDefaults();
    public SpectrumSwBank LfBank { get; set; } = SpectrumSwBank.CreateGeminusDefaults();

    // Spectrum pane colors (global)
    public string SpectrumBackground { get; set; } = "BLACK";
    public string SpectrumBackgroundRgb { get; set; } = "#101018";
    public string SpectrumFill { get; set; } = "SCOPE";
    public string SpectrumLine { get; set; } = "GREEN";

    // UI chrome (global)
    public string UiBackground { get; set; } = "BLACK";
    public string UiBackgroundRgb { get; set; } = "#1C1C1C";
    public string UiButton { get; set; } = "YELLOW";
    public string UiButtonRgb { get; set; } = "#FFCC00";
    public string UiPanel { get; set; } = "AUTO";

    // GEN
    public int GenIndexProficio { get; set; } = 7; // USER
    public int GenIndexGeminus { get; set; }
}

public static class ClientSettingsStore
{
    public static string StorePath
    {
        get
        {
            string root;
            if (OperatingSystem.IsWindows())
            {
                root = Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    "MSCC");
            }
            else
            {
                string home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
                string? xdg = Environment.GetEnvironmentVariable("XDG_CONFIG_HOME");
                root = !string.IsNullOrWhiteSpace(xdg)
                    ? Path.Combine(xdg, "MSCC")
                    : Path.Combine(home, ".config", "MSCC");
            }

            return Path.Combine(root, "mscc-avalonia.ini");
        }
    }

    public static ClientSettings Load()
    {
        var s = new ClientSettings();
        string path = StorePath;
        if (!File.Exists(path))
            return s;

        try
        {
            bool hadBankKeys = false;
            foreach (string raw in File.ReadAllLines(path))
            {
                string line = raw.Trim();
                if (line.Length == 0 || line.StartsWith('#') || line.StartsWith(';'))
                    continue;
                int eq = line.IndexOf('=');
                if (eq <= 0) continue;
                string key = line[..eq].Trim();
                string val = line[(eq + 1)..].Trim();
                if (key.StartsWith("HF_", StringComparison.OrdinalIgnoreCase)
                    || key.StartsWith("LF_", StringComparison.OrdinalIgnoreCase))
                    hadBankKeys = true;
                Apply(s, key, val);
            }

            // Older inis only had live WF_* / GRID_* keys — seed both banks from those
            MigrateLegacyLiveIntoBanks(s, hadBankKeys);
        }
        catch
        {
            // Keep defaults on parse/IO errors
        }

        return s;
    }

    public static void Save(ClientSettings s)
    {
        try
        {
            string path = StorePath;
            string? dir = Path.GetDirectoryName(path);
            if (!string.IsNullOrEmpty(dir))
                Directory.CreateDirectory(dir);

            var sb = new StringBuilder();
            sb.AppendLine("# MSCC Avalonia client settings");
            sb.AppendLine("# Connection");
            sb.AppendLine($"HOST={s.Host}");
            sb.AppendLine($"REMOTE_PORT={s.RemotePortText}");
            sb.AppendLine($"LOCAL_PORT={s.LocalPortText}");
            sb.AppendLine();
            sb.AppendLine("# Radio model");
            sb.AppendLine($"RADIO_MODEL={(s.IsGeminusRadioModel ? "Geminus" : "Proficio")}");
            sb.AppendLine();
            sb.AppendLine("# VFO");
            sb.AppendLine($"LAST_FREQ_HZ={s.LastFrequencyHz.ToString(CultureInfo.InvariantCulture)}");
            sb.AppendLine($"LAST_MODE={s.LastMode}");
            sb.AppendLine($"LAST_VFO_B_FREQ_HZ={s.LastVfoBFrequencyHz.ToString(CultureInfo.InvariantCulture)}");
            sb.AppendLine($"LAST_VFO_B_MODE={s.LastVfoBMode}");
            sb.AppendLine($"USE_VFO_A={(s.UseVfoA ? "1" : "0")}");
            sb.AppendLine();
            sb.AppendLine("# Operate — filters / step");
            sb.AppendLine($"STEP_INDEX={s.StepIndex}");
            sb.AppendLine($"LOW_CUT_INDEX={s.LowCutIndex}");
            sb.AppendLine($"HIGH_CUT_INDEX={s.HighCutIndex}");
            sb.AppendLine($"CW_FILTER_INDEX={s.CwFilterIndex}");
            sb.AppendLine();
            sb.AppendLine("# Audio");
            sb.AppendLine($"P_VOLUME={s.PVolume}");
            sb.AppendLine($"P_MIC={s.PMicGain}");
            sb.AppendLine($"D_VOLUME={s.DVolume}");
            sb.AppendLine($"D_MIC={s.DMicGain}");
            sb.AppendLine($"DIGITAL_AUDIO={(s.IsDigitalAudio ? "1" : "0")}");
            sb.AppendLine($"REMOTE_AUDIO={(s.RemoteAudio ? "1" : "0")}");
            sb.AppendLine();
            sb.AppendLine("# RIT");
            sb.AppendLine($"RIT_ON={(s.RitOn ? "1" : "0")}");
            sb.AppendLine($"RIT_OFFSET={s.RitOffset}");
            sb.AppendLine();
            sb.AppendLine("# CW");
            sb.AppendLine($"CW_KEYER={s.CwKeyerMode}");
            sb.AppendLine($"CW_SPACING={s.CwSpacing}");
            sb.AppendLine($"CW_PADDLE={s.CwPaddle}");
            sb.AppendLine($"CW_WEIGHT_INDEX={s.CwWeightIndex}");
            sb.AppendLine($"CW_PITCH_INDEX={s.CwPitchIndex}");
            sb.AppendLine($"CW_HOLD={s.CwHold}");
            sb.AppendLine($"CW_QSK={(s.CwQsk ? "1" : "0")}");
            sb.AppendLine($"CW_PHONES={(s.CwPhones ? "1" : "0")}");
            sb.AppendLine($"CW_SPEED={s.CwSpeed}");
            sb.AppendLine($"KEYER_MEM0={EscapeIni(s.KeyerMem0)}");
            sb.AppendLine($"KEYER_MEM1={EscapeIni(s.KeyerMem1)}");
            sb.AppendLine($"KEYER_MEM2={EscapeIni(s.KeyerMem2)}");
            sb.AppendLine($"KEYER_MEM3={EscapeIni(s.KeyerMem3)}");
            sb.AppendLine($"EXTERNAL_ELECTRONIC_KEYER={(s.ExternalElectronicKeyer ? "1" : "0")}");
            sb.AppendLine();
            sb.AppendLine("# Power banks");
            sb.AppendLine($"TUNE_POWER={s.TunePowerPercent}");
            sb.AppendLine($"CW_POWER={s.CwPowerPercent}");
            sb.AppendLine($"SSB_POWER={s.SsbPowerPercent}");
            sb.AppendLine($"AM_CARRIER={s.AmCarrierPercent}");
            sb.AppendLine();
            sb.AppendLine("# DSP / process (server-backed on connect)");
            sb.AppendLine($"COMPRESSION={s.Compression}");
            sb.AppendLine($"COMPRESSION_ON={(s.CompressionOn ? "1" : "0")}");
            sb.AppendLine($"AGC_LEVEL={s.AgcLevel}");
            sb.AppendLine($"AGC_FAST_RELEASE={s.AgcFastRelease}");
            sb.AppendLine($"NB_ON={(s.NbOn ? "1" : "0")}");
            sb.AppendLine($"NB_PULSE={s.NbPulse}");
            sb.AppendLine($"NB_THRESHOLD={s.NbThreshold}");
            sb.AppendLine($"NR_ON={(s.NrOn ? "1" : "0")}");
            sb.AppendLine($"NR_LEVEL={s.NrLevel}");
            sb.AppendLine($"AN_ON={(s.AnOn ? "1" : "0")}");
            sb.AppendLine($"MONITOR_ON={(s.MonitorOn ? "1" : "0")}");
            sb.AppendLine($"AMP_ON={(s.AmpOn ? "1" : "0")}");
            sb.AppendLine($"QRP_MODE={(s.QrpMode ? "1" : "0")}");
            sb.AppendLine($"FULL_POWER={(s.FullPower ? "1" : "0")}");
            sb.AppendLine($"ALC_ON={(s.AlcOn ? "1" : "0")}");
            sb.AppendLine();
            sb.AppendLine("# Global S/W");
            sb.AppendLine($"SPECTRUM_ZOOM={s.SpectrumZoom.ToString(CultureInfo.InvariantCulture)}");
            sb.AppendLine($"DB_CAL_REL={s.DbCalRelative.ToString(CultureInfo.InvariantCulture)}");
            sb.AppendLine();
            sb.AppendLine("# Live S/W (active bank snapshot — for tools/compat)");
            WriteLiveSw(sb, s);
            sb.AppendLine();
            sb.AppendLine("# Proficio / HF S/W bank");
            WriteBank(sb, "HF", s.HfBank);
            sb.AppendLine();
            sb.AppendLine("# Geminus / LF S/W bank");
            WriteBank(sb, "LF", s.LfBank);
            sb.AppendLine();
            sb.AppendLine("# Spectrum pane colors");
            sb.AppendLine($"SPECTRUM_BACKGROUND={s.SpectrumBackground}");
            sb.AppendLine($"SPECTRUM_BACKGROUND_RGB={s.SpectrumBackgroundRgb}");
            sb.AppendLine($"SPECTRUM_FILL={s.SpectrumFill}");
            sb.AppendLine($"SPECTRUM_LINE={s.SpectrumLine}");
            sb.AppendLine();
            sb.AppendLine("# UI chrome");
            sb.AppendLine($"UI_BACKGROUND={s.UiBackground}");
            sb.AppendLine($"UI_BACKGROUND_RGB={s.UiBackgroundRgb}");
            sb.AppendLine($"UI_BUTTON={s.UiButton}");
            sb.AppendLine($"UI_BUTTON_RGB={s.UiButtonRgb}");
            sb.AppendLine($"UI_PANEL={s.UiPanel}");
            sb.AppendLine();
            sb.AppendLine("# GEN");
            sb.AppendLine($"GEN_INDEX_PROFICIO={s.GenIndexProficio}");
            sb.AppendLine($"GEN_INDEX_GEMINUS={s.GenIndexGeminus}");
            File.WriteAllText(path, sb.ToString());
        }
        catch
        {
            // Best-effort; UI continues without sticky save
        }
    }

    private static void WriteLiveSw(StringBuilder sb, ClientSettings s)
    {
        sb.AppendLine($"GRID_MAX_DB={F(s.GridMaxDb)}");
        sb.AppendLine($"GRID_MIN_DB={F(s.GridMinDb)}");
        sb.AppendLine($"WF_HIGH_DB={F(s.WaterfallHighDb)}");
        sb.AppendLine($"WF_LOW_DB={F(s.WaterfallLowDb)}");
        sb.AppendLine($"VIEW_GRID={(s.ViewGrid ? "1" : "0")}");
        sb.AppendLine($"SHOW_WATERFALL={(s.ShowWaterfall ? "1" : "0")}");
        sb.AppendLine($"WF_DIR_NORMAL={(s.WaterfallDirectionNormal ? "1" : "0")}");
        sb.AppendLine($"WF_PALETTE={s.WaterfallPalette}");
    }

    private static void WriteBank(StringBuilder sb, string prefix, SpectrumSwBank b)
    {
        sb.AppendLine($"{prefix}_WF_HIGH_DB={F(b.WaterfallHighDb)}");
        sb.AppendLine($"{prefix}_WF_LOW_DB={F(b.WaterfallLowDb)}");
        sb.AppendLine($"{prefix}_WF_DIR_NORMAL={(b.WaterfallDirectionNormal ? "1" : "0")}");
        sb.AppendLine($"{prefix}_GRID_MAX_DB={F(b.GridMaxDb)}");
        sb.AppendLine($"{prefix}_GRID_MIN_DB={F(b.GridMinDb)}");
        sb.AppendLine($"{prefix}_VIEW_GRID={(b.ViewGrid ? "1" : "0")}");
        sb.AppendLine($"{prefix}_SHOW_WATERFALL={(b.ShowWaterfall ? "1" : "0")}");
        sb.AppendLine($"{prefix}_WF_PALETTE={b.WaterfallPalette}");
    }

    private static string F(float v) => v.ToString(CultureInfo.InvariantCulture);

    private static void Apply(ClientSettings s, string key, string val)
    {
        switch (key.ToUpperInvariant())
        {
            case "HOST":
                s.Host = val;
                break;
            case "REMOTE_PORT":
                s.RemotePortText = val;
                break;
            case "LOCAL_PORT":
                s.LocalPortText = val;
                break;
            case "RADIO_MODEL":
                s.IsGeminusRadioModel = val.Equals("Geminus", StringComparison.OrdinalIgnoreCase);
                break;
            case "LAST_FREQ_HZ":
                if (long.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out long hz))
                    s.LastFrequencyHz = hz;
                break;
            case "LAST_MODE":
                if (!string.IsNullOrWhiteSpace(val))
                    s.LastMode = val.Trim();
                break;
            case "LAST_VFO_B_FREQ_HZ":
                if (long.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out long bhz))
                    s.LastVfoBFrequencyHz = bhz;
                break;
            case "LAST_VFO_B_MODE":
                if (!string.IsNullOrWhiteSpace(val))
                    s.LastVfoBMode = val.Trim();
                break;
            case "USE_VFO_A":
                s.UseVfoA = IsTruthy(val);
                break;

            case "STEP_INDEX":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int si))
                    s.StepIndex = si;
                break;
            case "LOW_CUT_INDEX":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int lci))
                    s.LowCutIndex = lci;
                break;
            case "HIGH_CUT_INDEX":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int hci))
                    s.HighCutIndex = hci;
                break;
            case "CW_FILTER_INDEX":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int cfi))
                    s.CwFilterIndex = cfi;
                break;

            case "P_VOLUME":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int pv))
                    s.PVolume = Math.Clamp(pv, 0, 100);
                break;
            case "P_MIC":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int pm))
                    s.PMicGain = Math.Clamp(pm, 0, 100);
                break;
            case "D_VOLUME":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int dv))
                    s.DVolume = Math.Clamp(dv, 0, 100);
                break;
            case "D_MIC":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int dm))
                    s.DMicGain = Math.Clamp(dm, 0, 100);
                break;
            case "DIGITAL_AUDIO":
                s.IsDigitalAudio = IsTruthy(val);
                break;
            case "REMOTE_AUDIO":
                s.RemoteAudio = IsTruthy(val);
                break;

            case "RIT_ON":
                s.RitOn = IsTruthy(val);
                break;
            case "RIT_OFFSET":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int ro))
                    s.RitOffset = ro;
                break;

            case "CW_KEYER":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int ck))
                    s.CwKeyerMode = ck;
                break;
            case "CW_SPACING":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int cs))
                    s.CwSpacing = cs;
                break;
            case "CW_PADDLE":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int cp))
                    s.CwPaddle = cp;
                break;
            case "CW_WEIGHT_INDEX":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int cw))
                    s.CwWeightIndex = cw;
                break;
            case "CW_PITCH_INDEX":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int cpi))
                    s.CwPitchIndex = cpi;
                break;
            case "CW_HOLD":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int ch))
                    s.CwHold = ch;
                break;
            case "CW_QSK":
                s.CwQsk = IsTruthy(val);
                break;
            case "CW_PHONES":
                s.CwPhones = IsTruthy(val);
                break;
            case "CW_SPEED":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int csp))
                    s.CwSpeed = Math.Clamp(csp, 5, 60);
                break;
            case "KEYER_MEM0":
                s.KeyerMem0 = ClampKeyerMem(UnescapeIni(val));
                break;
            case "KEYER_MEM1":
                s.KeyerMem1 = ClampKeyerMem(UnescapeIni(val));
                break;
            case "KEYER_MEM2":
                s.KeyerMem2 = ClampKeyerMem(UnescapeIni(val));
                break;
            case "KEYER_MEM3":
                s.KeyerMem3 = ClampKeyerMem(UnescapeIni(val));
                break;
            case "EXTERNAL_ELECTRONIC_KEYER":
                s.ExternalElectronicKeyer = IsTruthy(val);
                break;

            case "TUNE_POWER":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int tp))
                    s.TunePowerPercent = Math.Clamp(tp, 0, 100);
                break;
            case "CW_POWER":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int cwp))
                    s.CwPowerPercent = Math.Clamp(cwp, 0, 100);
                break;
            case "SSB_POWER":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int sp))
                    s.SsbPowerPercent = Math.Clamp(sp, 0, 100);
                break;
            case "AM_CARRIER":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int am))
                    s.AmCarrierPercent = Math.Clamp(am, 0, 100);
                break;

            case "COMPRESSION":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int cmp))
                    s.Compression = Math.Clamp(cmp, 0, 24);
                break;
            case "COMPRESSION_ON":
                s.CompressionOn = IsTruthy(val);
                break;
            case "AGC_LEVEL":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int agc))
                    s.AgcLevel = Math.Clamp(agc, 0, 2);
                break;
            case "AGC_FAST_RELEASE":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int afr))
                    s.AgcFastRelease = Math.Clamp(afr, 0, 1000);
                break;
            case "NB_ON":
                s.NbOn = IsTruthy(val);
                break;
            case "NB_PULSE":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int nbp))
                    s.NbPulse = Math.Clamp(nbp, 10, 510);
                break;
            case "NB_THRESHOLD":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int nbt))
                    s.NbThreshold = Math.Clamp(nbt, 1, 1009);
                break;
            case "NR_ON":
                s.NrOn = IsTruthy(val);
                break;
            case "NR_LEVEL":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int nrl))
                    s.NrLevel = Math.Clamp(nrl, 0, 100);
                break;
            case "AN_ON":
                s.AnOn = IsTruthy(val);
                break;
            case "MONITOR_ON":
                s.MonitorOn = IsTruthy(val);
                break;
            case "AMP_ON":
                s.AmpOn = IsTruthy(val);
                break;
            case "QRP_MODE":
                s.QrpMode = IsTruthy(val);
                break;
            case "FULL_POWER":
                s.FullPower = IsTruthy(val);
                break;
            case "ALC_ON":
                s.AlcOn = IsTruthy(val);
                break;

            case "SPECTRUM_ZOOM":
                if (double.TryParse(val, NumberStyles.Float, CultureInfo.InvariantCulture, out double z))
                    s.SpectrumZoom = Math.Clamp(z, 1, 4);
                break;
            case "DB_CAL_REL":
                if (float.TryParse(val, NumberStyles.Float, CultureInfo.InvariantCulture, out float db))
                    s.DbCalRelative = Math.Clamp(db, -20f, 20f);
                break;
            // Legacy NULL_LO_* keys ignored — LO null is server-side (sdrcore-recv)

            // Live / legacy keys
            case "GRID_MAX_DB":
                if (TryFloat(val, out float gmax)) s.GridMaxDb = gmax;
                break;
            case "GRID_MIN_DB":
                if (TryFloat(val, out float gmin)) s.GridMinDb = gmin;
                break;
            case "WF_HIGH_DB":
                if (TryFloat(val, out float wh)) s.WaterfallHighDb = wh;
                break;
            case "WF_LOW_DB":
                if (TryFloat(val, out float wl)) s.WaterfallLowDb = wl;
                break;
            case "VIEW_GRID":
                s.ViewGrid = IsTruthy(val);
                break;
            case "SHOW_WATERFALL":
                s.ShowWaterfall = IsTruthy(val);
                break;
            case "WF_DIR_NORMAL":
                s.WaterfallDirectionNormal = IsTruthy(val);
                break;
            case "WF_PALETTE":
                if (!string.IsNullOrWhiteSpace(val))
                    s.WaterfallPalette = WaterfallPalettes.NormalizeName(val);
                break;

            // HF bank
            case "HF_WF_HIGH_DB":
                if (TryFloat(val, out float hfh)) s.HfBank.WaterfallHighDb = hfh;
                break;
            case "HF_WF_LOW_DB":
                if (TryFloat(val, out float hfl)) s.HfBank.WaterfallLowDb = hfl;
                break;
            case "HF_WF_DIR_NORMAL":
                s.HfBank.WaterfallDirectionNormal = IsTruthy(val);
                break;
            case "HF_GRID_MAX_DB":
                if (TryFloat(val, out float hfgx)) s.HfBank.GridMaxDb = hfgx;
                break;
            case "HF_GRID_MIN_DB":
                if (TryFloat(val, out float hfgn)) s.HfBank.GridMinDb = hfgn;
                break;
            case "HF_VIEW_GRID":
                s.HfBank.ViewGrid = IsTruthy(val);
                break;
            case "HF_SHOW_WATERFALL":
                s.HfBank.ShowWaterfall = IsTruthy(val);
                break;
            case "HF_WF_PALETTE":
                if (!string.IsNullOrWhiteSpace(val))
                    s.HfBank.WaterfallPalette = WaterfallPalettes.NormalizeName(val);
                break;

            // LF bank
            case "LF_WF_HIGH_DB":
                if (TryFloat(val, out float lfh)) s.LfBank.WaterfallHighDb = lfh;
                break;
            case "LF_WF_LOW_DB":
                if (TryFloat(val, out float lfl)) s.LfBank.WaterfallLowDb = lfl;
                break;
            case "LF_WF_DIR_NORMAL":
                s.LfBank.WaterfallDirectionNormal = IsTruthy(val);
                break;
            case "LF_GRID_MAX_DB":
                if (TryFloat(val, out float lfgx)) s.LfBank.GridMaxDb = lfgx;
                break;
            case "LF_GRID_MIN_DB":
                if (TryFloat(val, out float lfgn)) s.LfBank.GridMinDb = lfgn;
                break;
            case "LF_VIEW_GRID":
                s.LfBank.ViewGrid = IsTruthy(val);
                break;
            case "LF_SHOW_WATERFALL":
                s.LfBank.ShowWaterfall = IsTruthy(val);
                break;
            case "LF_WF_PALETTE":
                if (!string.IsNullOrWhiteSpace(val))
                    s.LfBank.WaterfallPalette = WaterfallPalettes.NormalizeName(val);
                break;

            case "SPECTRUM_BACKGROUND":
                if (!string.IsNullOrWhiteSpace(val))
                    s.SpectrumBackground = val.Trim().ToUpperInvariant();
                break;
            case "SPECTRUM_BACKGROUND_RGB":
                if (!string.IsNullOrWhiteSpace(val))
                    s.SpectrumBackgroundRgb = val.Trim();
                break;
            case "SPECTRUM_FILL":
                if (!string.IsNullOrWhiteSpace(val))
                    s.SpectrumFill = val.Trim().ToUpperInvariant();
                break;
            case "SPECTRUM_LINE":
                if (!string.IsNullOrWhiteSpace(val))
                    s.SpectrumLine = val.Trim().ToUpperInvariant();
                break;
            case "UI_BACKGROUND":
                if (!string.IsNullOrWhiteSpace(val))
                    s.UiBackground = val.Trim().ToUpperInvariant();
                break;
            case "UI_BACKGROUND_RGB":
                if (!string.IsNullOrWhiteSpace(val))
                    s.UiBackgroundRgb = val.Trim();
                break;
            case "UI_BUTTON":
                if (!string.IsNullOrWhiteSpace(val))
                    s.UiButton = val.Trim().ToUpperInvariant();
                break;
            case "UI_BUTTON_RGB":
                if (!string.IsNullOrWhiteSpace(val))
                    s.UiButtonRgb = val.Trim();
                break;
            case "UI_PANEL":
                if (!string.IsNullOrWhiteSpace(val))
                    s.UiPanel = val.Trim().ToUpperInvariant();
                break;

            case "GEN_INDEX_PROFICIO":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int gp))
                    s.GenIndexProficio = gp;
                break;
            case "GEN_INDEX_GEMINUS":
                if (int.TryParse(val, NumberStyles.Integer, CultureInfo.InvariantCulture, out int gg))
                    s.GenIndexGeminus = gg;
                break;
        }
    }

    private static bool TryFloat(string val, out float f) =>
        float.TryParse(val, NumberStyles.Float, CultureInfo.InvariantCulture, out f);

    private static bool IsTruthy(string val) =>
        val is "1" or "true" or "True" or "yes" or "YES";

    /// <summary>Keyer mem lines: strip CR/LF so one INI line stays one message.</summary>
    private static string EscapeIni(string? s)
    {
        if (string.IsNullOrEmpty(s)) return "";
        return s.Replace("\r", "", StringComparison.Ordinal).Replace("\n", " ", StringComparison.Ordinal);
    }

    private static string UnescapeIni(string? val) => val ?? "";

    private static string ClampKeyerMem(string? text)
    {
        if (string.IsNullOrEmpty(text)) return "";
        if (text.Length > 48) text = text[..48];
        var sb = new StringBuilder(text.Length);
        foreach (char c in text)
        {
            if (c is >= (char)0x20 and <= (char)0x7E)
                sb.Append(c);
        }
        return sb.ToString();
    }

    /// <summary>
    /// Older inis only had live WF keys. Copy them into both banks so first
    /// radio-model switch does not wipe the user's existing window.
    /// </summary>
    internal static void MigrateLegacyLiveIntoBanks(ClientSettings s, bool hadBankKeys)
    {
        if (hadBankKeys) return;

        var live = new SpectrumSwBank
        {
            WaterfallHighDb = s.WaterfallHighDb,
            WaterfallLowDb = s.WaterfallLowDb,
            WaterfallDirectionNormal = s.WaterfallDirectionNormal,
            GridMaxDb = s.GridMaxDb,
            GridMinDb = s.GridMinDb,
            ViewGrid = s.ViewGrid,
            ShowWaterfall = s.ShowWaterfall,
            WaterfallPalette = s.WaterfallPalette ?? "Enhanced",
        };
        s.HfBank.CopyFrom(live);
        s.LfBank.CopyFrom(live);
    }
}
