using System.Text.RegularExpressions;

namespace MSCC.Avalonia.Services;

/// <summary>
/// Read/write mscc.ini PROFICIO-MKII for legacy vs MKII host features.
/// Windows: %LocalAppData%\MSCC-NET9\mscc.ini (same as WPF / Windows ms-sdr).
/// Linux: ~/mscc.ini (same as Linux ms-sdr initialize_mscc).
/// </summary>
public static class MsccIniProficio
{
    public static string MsccIniPath
    {
        get
        {
            if (OperatingSystem.IsWindows())
            {
                return Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    "MSCC-NET9",
                    "mscc.ini");
            }

            string home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
            return Path.Combine(home, "mscc.ini");
        }
    }

    /// <summary>
    /// Write PROFICIO-MKII=0|1. mkii=true → 1 (default MKII); false → 0 (legacy/external keyer).
    /// </summary>
    public static bool WriteProficioMkii(bool mkii)
    {
        try
        {
            string path = MsccIniPath;
            string? dir = Path.GetDirectoryName(path);
            if (!string.IsNullOrEmpty(dir))
                Directory.CreateDirectory(dir);

            string text = File.Exists(path) ? File.ReadAllText(path) : "";
            text = StripCorruptDollarIpLines(text);
            string original = text;
            string val = mkii ? "1" : "0";
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

    /// <summary>Default true (MKII) when missing.</summary>
    public static bool ReadProficioMkii(bool defaultMkii = true)
    {
        try
        {
            string path = MsccIniPath;
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
        var pattern = new Regex(
            $@"(?im)^(\s*{Regex.Escape(key)}\s*=)[^;\r\n]*",
            RegexOptions.CultureInvariant);
        if (pattern.IsMatch(text))
            return pattern.Replace(text, "${1}" + value);
        string sep = text.EndsWith('\n') || text.Length == 0 ? "" : Environment.NewLine;
        return text + sep + $"{key}={value};" + Environment.NewLine;
    }

    private static string RemoveIniKey(string text, string key)
    {
        var pattern = new Regex(
            $@"(?im)^\s*{Regex.Escape(key)}\s*=[^\r\n]*\r?\n?",
            RegexOptions.CultureInvariant);
        return pattern.Replace(text, "");
    }

    private static string StripCorruptDollarIpLines(string text)
    {
        var pattern = new Regex(
            @"(?im)^\s*\$\d+\.\d+\.\d+\.\d+;?\s*\r?\n?",
            RegexOptions.CultureInvariant);
        return pattern.Replace(text, "");
    }
}
