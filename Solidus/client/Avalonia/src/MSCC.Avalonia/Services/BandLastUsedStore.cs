using System.Globalization;
using System.Text;

namespace MSCC.Avalonia.Services;

/// <summary>
/// Per-band last-used frequency / mode / filters (client-only).
/// VFO A: ~/.config/MSCC/mscc-lastused.ini (Windows: %LocalAppData%\MSCC\…)
/// VFO B: mscc-lastused-vfob.ini
/// Keys: 20M_FREQ, 20M_MODE, 20M_LOWCUT, … (exact KEY= match; no substring bugs).
/// </summary>
public static class BandLastUsedStore
{
    private static readonly HashSet<string> ValidBands = new(StringComparer.OrdinalIgnoreCase)
    {
        "2200M", "630M", "160M", "80M", "60M", "40M", "30M",
        "20M", "17M", "15M", "12M", "10M", "GEN"
    };

    public static string StorePathA => PathFor("mscc-lastused.ini");
    public static string StorePathB => PathFor("mscc-lastused-vfob.ini");

    private static string PathFor(string fileName)
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

        return Path.Combine(root, fileName);
    }

    public static string NormalizeBandKey(string? band)
    {
        string b = (band ?? "").Trim().ToUpperInvariant();
        if (b is "GEN" or "GENERAL") return "GEN";
        // "20m" / "20M" / "20" → "20M"
        if (b.EndsWith('M') && b.Length > 1)
            return b;
        if (b is "2200" or "630" or "160" or "80" or "60" or "40" or "30"
            or "20" or "17" or "15" or "12" or "10")
            return b + "M";
        return b;
    }

    public static void Save(
        string band,
        long frequencyHz,
        string mode,
        int lowCutIndex,
        int highCutIndex,
        int cwFilterIndex,
        bool forVfoB = false)
    {
        string b = NormalizeBandKey(band);
        if (string.IsNullOrEmpty(b) || b == "?" || !ValidBands.Contains(b))
            return;
        if (frequencyHz is < 10_000 or > 60_000_000)
            return;

        string path = forVfoB ? StorePathB : StorePathA;
        try
        {
            string? dir = Path.GetDirectoryName(path);
            if (!string.IsNullOrEmpty(dir))
                Directory.CreateDirectory(dir);

            var map = LoadMap(path);
            map[$"{b}_FREQ"] = frequencyHz.ToString(CultureInfo.InvariantCulture);
            map[$"{b}_MODE"] = string.IsNullOrWhiteSpace(mode) ? "USB" : mode.Trim();
            map[$"{b}_LOWCUT"] = lowCutIndex.ToString(CultureInfo.InvariantCulture);
            map[$"{b}_HIGHCUT"] = highCutIndex.ToString(CultureInfo.InvariantCulture);
            map[$"{b}_CWFILTER"] = cwFilterIndex.ToString(CultureInfo.InvariantCulture);
            WriteMap(path, map);
        }
        catch
        {
            // Best-effort
        }
    }

    public static (long freq, string mode, int lowCut, int highCut, int cwFilter) Load(
        string band,
        bool forVfoB = false)
    {
        long freq = 0;
        string mode = "";
        int lowCut = -1, highCut = -1, cwFilter = -1;

        string b = NormalizeBandKey(band);
        if (string.IsNullOrEmpty(b) || b == "?" || !ValidBands.Contains(b))
            return (freq, mode, lowCut, highCut, cwFilter);

        string path = forVfoB ? StorePathB : StorePathA;
        if (!File.Exists(path))
            return (freq, mode, lowCut, highCut, cwFilter);

        try
        {
            var map = LoadMap(path);
            if (map.TryGetValue($"{b}_FREQ", out string? fs) &&
                long.TryParse(fs, NumberStyles.Integer, CultureInfo.InvariantCulture, out long f))
                freq = f;
            if (map.TryGetValue($"{b}_MODE", out string? m) && !string.IsNullOrWhiteSpace(m))
                mode = m.Trim();
            if (map.TryGetValue($"{b}_LOWCUT", out string? ls) &&
                int.TryParse(ls, NumberStyles.Integer, CultureInfo.InvariantCulture, out int l))
                lowCut = l;
            if (map.TryGetValue($"{b}_HIGHCUT", out string? hs) &&
                int.TryParse(hs, NumberStyles.Integer, CultureInfo.InvariantCulture, out int h))
                highCut = h;
            if (map.TryGetValue($"{b}_CWFILTER", out string? cs) &&
                int.TryParse(cs, NumberStyles.Integer, CultureInfo.InvariantCulture, out int c))
                cwFilter = c;
        }
        catch
        {
            // defaults
        }

        return (freq, mode, lowCut, highCut, cwFilter);
    }

    private static Dictionary<string, string> LoadMap(string path)
    {
        var map = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        if (!File.Exists(path))
            return map;
        foreach (string raw in File.ReadAllLines(path))
        {
            string line = raw.Trim();
            if (line.Length == 0 || line.StartsWith('#') || line.StartsWith(';'))
                continue;
            int eq = line.IndexOf('=');
            if (eq <= 0) continue;
            string key = line[..eq].Trim();
            string val = line[(eq + 1)..].Trim();
            // Only keep valid band-prefixed keys
            int us = key.IndexOf('_');
            if (us <= 0) continue;
            string prefix = key[..us].ToUpperInvariant();
            if (!ValidBands.Contains(prefix))
                continue;
            map[key.ToUpperInvariant()] = val;
        }
        return map;
    }

    private static void WriteMap(string path, Dictionary<string, string> map)
    {
        var sb = new StringBuilder();
        sb.AppendLine("# MSCC Avalonia per-band last-used (client only)");
        sb.AppendLine("# KEY=value  e.g. 20M_FREQ=14074000");
        foreach (var kv in map.OrderBy(k => k.Key, StringComparer.OrdinalIgnoreCase))
            sb.AppendLine($"{kv.Key}={kv.Value}");
        File.WriteAllText(path, sb.ToString());
    }
}
