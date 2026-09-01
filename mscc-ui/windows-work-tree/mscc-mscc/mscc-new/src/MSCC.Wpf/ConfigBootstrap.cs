using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace MSCC.Wpf;

/// <summary>
/// First-run / install setup: seed %LocalAppData%\MSCC-NET9 from install init-files,
/// and report whether local COM + operator audio are ready for Launch Servers.
/// Replaces the need to run Initialize.bat / mscc-init.exe for normal installs.
/// </summary>
public static class ConfigBootstrap
{
    public const string ConfigFolderName = "MSCC-NET9";
    public const string SeedCompleteFlag = "MSCC_SEED_COMPLETE.flag";

    /// <summary>
    /// Local single-PC default. ms-sdr does gethostbyname(MSCC_IP) for GUI UDP replies;
    /// foreign hostnames from seed templates (e.g. Ron-PC) cause "HOST NAME NOT FOUND".
    /// </summary>
    public const string LocalLoopbackHost = "127.0.0.1";

    public static string ConfigDirectory =>
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), ConfigFolderName);

    /// <summary>Install-side templates next to MSCC.Wpf.exe (C:\mscc-net9\init-files).</summary>
    public static string InstallInitFilesDirectory =>
        Path.Combine(AppContext.BaseDirectory, "init-files");

    /// <summary>Backend host ini files that carry MSCC_IP / PROFICIO_DLL_IP.</summary>
    private static readonly string[] BackendHostIniFiles =
    {
        "mscc.ini",       // required by ms-sdr (G_Client_Host_Name)
        "Multus_mscc.ini",
        "mscc-rpi.ini",
    };

    public sealed class SetupStatus
    {
        /// <summary>Hard blockers for local Launch Servers Start.</summary>
        public List<string> Missing { get; } = new();

        /// <summary>Soft issues (warn but allow Start).</summary>
        public List<string> Warnings { get; } = new();

        public bool IsComplete => Missing.Count == 0;

        public string SummaryLine
        {
            get
            {
                if (IsComplete && Warnings.Count == 0)
                    return "";
                if (!IsComplete)
                    return "Setup needed: " + string.Join(", ", Missing);
                return "Setup note: " + string.Join(", ", Warnings);
            }
        }
    }

    /// <summary>
    /// Copy missing files from install init-files into AppData (never overwrite existing).
    /// Creates the config directory. Returns number of files newly copied.
    /// </summary>
    public static int SeedMissingConfigFiles()
    {
        int copied = 0;
        try
        {
            Directory.CreateDirectory(ConfigDirectory);
            string srcRoot = InstallInitFilesDirectory;
            if (Directory.Exists(srcRoot))
            {
                foreach (string srcFile in Directory.GetFiles(srcRoot, "*", SearchOption.AllDirectories))
                {
                    string rel = Path.GetRelativePath(srcRoot, srcFile);
                    // Never seed a flag that would re-block Initialize.bat semantics incorrectly
                    if (string.Equals(Path.GetFileName(srcFile), "MSCC_INIT_COMPLETE.flag", StringComparison.OrdinalIgnoreCase))
                        continue;

                    string dstFile = Path.Combine(ConfigDirectory, rel);
                    if (File.Exists(dstFile))
                        continue;

                    string? dstDir = Path.GetDirectoryName(dstFile);
                    if (!string.IsNullOrEmpty(dstDir))
                        Directory.CreateDirectory(dstDir);

                    File.Copy(srcFile, dstFile, overwrite: false);
                    copied++;
                }

                // Mark that silent seed ran at least once (informational)
                string flag = Path.Combine(ConfigDirectory, SeedCompleteFlag);
                if (!File.Exists(flag))
                {
                    try
                    {
                        File.WriteAllText(flag,
                            $"Seeded {DateTime.Now:O} from {srcRoot}; filesCopied={copied}\n");
                    }
                    catch { /* ignore */ }
                }
            }

            // Templates often ship a developer hostname; pin local backends to loopback.
            // Also rewrites existing AppData carried over from another PC (e.g. MSCC_IP=Ron-PC).
            EnsureLocalBackendHostnames();
        }
        catch
        {
            // Best-effort; app can still run and Settings can write files
        }

        return copied;
    }

    /// <summary>
    /// Rewrite MSCC_IP / PROFICIO_DLL_IP in backend ini files to <paramref name="hostOrIp"/>
    /// (default 127.0.0.1) so ms-sdr can resolve the GUI client on this machine.
    /// Safe for local Launch Servers; remote multi-PC setups should set these manually
    /// on the radio PC and leave Launch Servers off on the remote client.
    /// Returns number of files changed.
    /// </summary>
    public static int EnsureLocalBackendHostnames(string hostOrIp = LocalLoopbackHost)
    {
        int changed = 0;
        string host = string.IsNullOrWhiteSpace(hostOrIp) ? LocalLoopbackHost : hostOrIp.Trim();
        try
        {
            Directory.CreateDirectory(ConfigDirectory);
            foreach (string fileName in BackendHostIniFiles)
            {
                string path = Path.Combine(ConfigDirectory, fileName);
                if (!File.Exists(path))
                    continue;
                if (RewriteHostKeysInIni(path, host))
                    changed++;
            }
        }
        catch
        {
            // Best-effort
        }
        return changed;
    }

    /// <summary>
    /// Update MSCC_IP=… and PROFICIO_DLL_IP=… in a Multus-style ini (KEY=value;).
    /// Also strips corrupted lines produced by an older RewriteIniKey bug ($1127… without a key).
    /// </summary>
    private static bool RewriteHostKeysInIni(string path, string hostOrIp)
    {
        string text = File.ReadAllText(path);
        string original = text;
        text = StripCorruptDollarIpLines(text);
        text = RewriteIniKey(text, "MSCC_IP", hostOrIp);
        text = RewriteIniKey(text, "PROFICIO_DLL_IP", hostOrIp);
        if (string.Equals(text, original, StringComparison.Ordinal))
            return false;
        File.WriteAllText(path, text);
        return true;
    }

    /// <summary>
    /// Write PROFICIO-MKII=0|1 into %LocalAppData%\MSCC-NET9\mscc.ini (read by ms-sdr at start).
    /// Checked UI "external electronic keyer" → 0 (legacy); unchecked → 1 (MKII, default).
    /// </summary>
    public static bool WriteProficioMkii(bool mkii)
    {
        try
        {
            Directory.CreateDirectory(ConfigDirectory);
            string path = Path.Combine(ConfigDirectory, "mscc.ini");
            string text = File.Exists(path) ? File.ReadAllText(path) : "";
            text = StripCorruptDollarIpLines(text);
            string original = text;
            string val = mkii ? "1" : "0";
            // Prefer hyphen form; remove underscored typo form so only one key remains.
            text = RemoveIniKey(text, "PROFICIO_MKII");
            text = RewriteIniKey(text, "PROFICIO-MKII", val);
            if (!File.Exists(path) || !string.Equals(text, original, StringComparison.Ordinal))
            {
                File.WriteAllText(path, text);
                return true;
            }
            return false;
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// Read PROFICIO-MKII from mscc.ini. Default true (MKII) when missing / unreadable.
    /// </summary>
    public static bool ReadProficioMkii(bool defaultMkii = true)
    {
        try
        {
            string path = Path.Combine(ConfigDirectory, "mscc.ini");
            if (!File.Exists(path))
                return defaultMkii;
            foreach (string raw in File.ReadAllLines(path))
            {
                string line = raw.Trim();
                if (line.StartsWith("PROFICIO-MKII=", StringComparison.OrdinalIgnoreCase)
                    || line.StartsWith("PROFICIO_MKII=", StringComparison.OrdinalIgnoreCase))
                {
                    int eq = line.IndexOf('=');
                    if (eq < 0) continue;
                    string rest = line[(eq + 1)..].Trim().TrimEnd(';').Trim();
                    if (rest.Length == 0) return defaultMkii;
                    if (rest.StartsWith('0')) return false;
                    return true;
                }
            }
        }
        catch { /* default */ }
        return defaultMkii;
    }

    private static string RewriteIniKey(string text, string key, string value)
    {
        // Lines like MSCC_IP=Ron-PC;  (semicolon optional)
        var pattern = new System.Text.RegularExpressions.Regex(
            $@"(?im)^(\s*{System.Text.RegularExpressions.Regex.Escape(key)}\s*=)[^;\r\n]*",
            System.Text.RegularExpressions.RegexOptions.CultureInvariant);
        if (pattern.IsMatch(text))
        {
            // Use ${1} so value "127.0.0.1" is not parsed as group $11 + "27…".
            return pattern.Replace(text, "${1}" + value);
        }
        // Key missing: append (mscc.ini style)
        string sep = text.EndsWith("\n") || text.Length == 0 ? "" : Environment.NewLine;
        return text + sep + $"{key}={value};" + Environment.NewLine;
    }

    private static string RemoveIniKey(string text, string key)
    {
        var pattern = new System.Text.RegularExpressions.Regex(
            $@"(?im)^\s*{System.Text.RegularExpressions.Regex.Escape(key)}\s*=[^\r\n]*\r?\n?",
            System.Text.RegularExpressions.RegexOptions.CultureInvariant);
        return pattern.Replace(text, "");
    }

    /// <summary>
    /// Remove orphan lines like "$1127.0.0.1;" from a prior $1+127 regex bug.
    /// </summary>
    private static string StripCorruptDollarIpLines(string text)
    {
        var pattern = new System.Text.RegularExpressions.Regex(
            @"(?im)^\s*\$\d+\.\d+\.\d+\.\d+;?\s*\r?\n?",
            System.Text.RegularExpressions.RegexOptions.CultureInvariant);
        return pattern.Replace(text, "");
    }

    /// <summary>
    /// True when this looks like a brand-new config tree (seed just filled gaps / no prior client ini).
    /// Used to open Settings once for the user.
    /// </summary>
    public static bool IsLikelyFirstRun()
    {
        try
        {
            string clientIni = Path.Combine(ConfigDirectory, "MSCC_Client.ini");
            string flag = Path.Combine(ConfigDirectory, SeedCompleteFlag);
            // First run if client ini missing before seed, or seed flag just created this session
            // Caller should use return of Seed + Evaluate together.
            return !File.Exists(clientIni);
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// Evaluate local device setup for starting with Launch Servers.
    /// When <paramref name="launchServers"/> is false (remote client), no hard requirements.
    /// </summary>
    public static SetupStatus EvaluateLocalSetup(bool launchServers)
    {
        var status = new SetupStatus();
        if (!launchServers)
            return status;

        // --- COM port ---
        var comm = CommPortConfig.Load();
        string port = (comm.PortName ?? "").Trim();
        if (string.IsNullOrEmpty(port))
        {
            status.Missing.Add("COM port");
        }
        else
        {
            var ports = CommPortConfig.GetAvailablePorts();
            bool present = ports.Any(p =>
                string.Equals(p, CommPortConfig.NormalizePortName(port), StringComparison.OrdinalIgnoreCase));
            if (!present)
            {
                // Soft: radio may be unplugged / off — allow Start with a warning
                status.Warnings.Add($"{port} not found (is the radio connected?)");
            }
        }

        // --- Operator speaker (required for local recv path) ---
        // Must be non-empty AND match a currently listed MME device (seed placeholders alone are not enough).
        string speaker = AudioDeviceConfig.ReadIni(AudioDeviceConfig.OperatorSpeakerFile).Trim();
        var outs = AudioDeviceConfig.GetOutputDevices();
        if (string.IsNullOrEmpty(speaker))
        {
            status.Missing.Add("Operator speaker (Settings → Audio)");
        }
        else if (outs.Count == 0)
        {
            status.Missing.Add("No audio output devices detected");
        }
        else if (!AudioDeviceConfig.SavedKeyMatchesDevice(speaker, outs))
        {
            status.Missing.Add("Operator speaker (select a device that exists on this PC)");
        }

        // Operator mic: required by mscc-trans at startup (even digi-only often needs a valid mic name)
        string mic = AudioDeviceConfig.ReadIni(AudioDeviceConfig.OperatorMicFile).Trim();
        var ins = AudioDeviceConfig.GetInputDevices();
        if (string.IsNullOrEmpty(mic))
        {
            status.Missing.Add("Operator microphone (Settings → Audio)");
        }
        else if (ins.Count == 0)
        {
            status.Missing.Add("No audio input devices detected");
        }
        else if (!AudioDeviceConfig.SavedKeyMatchesDevice(mic, ins))
        {
            status.Missing.Add("Operator microphone (select a device that exists on this PC)");
        }

        // Digital path: soft only (VAC optional)
        string digSpk = AudioDeviceConfig.ReadIni(AudioDeviceConfig.DigitalSpeakerFile).Trim();
        if (string.IsNullOrEmpty(digSpk) || !AudioDeviceConfig.SavedKeyMatchesDevice(digSpk, outs))
            status.Warnings.Add("Digital speaker not set (optional unless using digi/VAC)");
        string digMic = AudioDeviceConfig.ReadIni(AudioDeviceConfig.DigitalMicFile).Trim();
        if (string.IsNullOrEmpty(digMic) || !AudioDeviceConfig.SavedKeyMatchesDevice(digMic, ins))
            status.Warnings.Add("Digital microphone not set (optional unless using digi/VAC)");

        return status;
    }

    /// <summary>
    /// Human-readable block for a MessageBox when Start is refused.
    /// </summary>
    public static string FormatStartBlockedMessage(SetupStatus status)
    {
        var lines = new List<string>
        {
            "Local radio needs a serial port plus operator speaker and microphone before starting servers.",
            "Open Settings → Audio, choose each device from the list (blank means not set), then Apply.",
            "",
            "Missing:"
        };
        foreach (string m in status.Missing)
            lines.Add("  • " + m);

        if (status.Warnings.Count > 0)
        {
            lines.Add("");
            lines.Add("Notes:");
            foreach (string w in status.Warnings)
                lines.Add("  • " + w);
        }

        lines.Add("");
        lines.Add("Open Settings to choose COM port and audio devices, then press Start again.");
        return string.Join(Environment.NewLine, lines);
    }
}
