using MSCC.Avalonia.Models;

namespace MSCC.Avalonia.Services;

/// <summary>
/// Client-only favorites (never sent to ms-sdr as a blob).
/// Linux: ~/.config/MSCC/mscc-favorites.ini
/// Windows: %LocalAppData%\MSCC\mscc-favorites.ini
/// </summary>
public static class FavoritesStore
{
    private const char Sep = '|';

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

            return Path.Combine(root, "mscc-favorites.ini");
        }
    }

    public static List<FavoriteEntry> Load()
    {
        var list = new List<FavoriteEntry>();
        string path = StorePath;
        if (!File.Exists(path))
            return list;

        try
        {
            foreach (string raw in File.ReadAllLines(path))
            {
                string line = raw.Trim();
                if (line.Length == 0 || line.StartsWith('#') || line.StartsWith(';'))
                    continue;

                string[] parts = line.Split(Sep);
                if (parts.Length < 8)
                    continue;
                if (!long.TryParse(parts[2].Trim(), out long freq))
                    continue;
                if (!int.TryParse(parts[4].Trim(), out int low)) low = 0;
                if (!int.TryParse(parts[5].Trim(), out int high)) high = 0;
                if (!int.TryParse(parts[6].Trim(), out int cw)) cw = 0;

                list.Add(new FavoriteEntry
                {
                    Name = Unescape(parts[0]),
                    Band = parts[1].Trim(),
                    FrequencyHz = freq,
                    Mode = parts[3].Trim(),
                    LowCutIndex = low,
                    HighCutIndex = high,
                    CwFilterIndex = cw,
                    Vfo = parts[7].Trim()
                });
            }
        }
        catch
        {
            // Keep empty list
        }

        return list;
    }

    public static void Save(IEnumerable<FavoriteEntry> entries)
    {
        try
        {
            string path = StorePath;
            string? dir = Path.GetDirectoryName(path);
            if (!string.IsNullOrEmpty(dir))
                Directory.CreateDirectory(dir);

            var lines = new List<string>
            {
                "# MSCC Avalonia client-side favorites (not sent to ms-sdr)",
                "# NAME|BAND|FREQ_HZ|MODE|LOWCUT_IDX|HIGHCUT_IDX|CWFILTER_IDX|VFO"
            };

            foreach (var e in entries)
            {
                if (string.IsNullOrWhiteSpace(e.Name))
                    continue;
                lines.Add(string.Join(Sep,
                    Escape(e.Name.Trim()),
                    (e.Band ?? "").Trim(),
                    e.FrequencyHz.ToString(),
                    (e.Mode ?? "USB").Trim(),
                    e.LowCutIndex.ToString(),
                    e.HighCutIndex.ToString(),
                    e.CwFilterIndex.ToString(),
                    string.Equals(e.Vfo, "B", StringComparison.OrdinalIgnoreCase) ? "B" : "A"));
            }

            File.WriteAllLines(path, lines);
        }
        catch
        {
            // Best-effort
        }
    }

    private static string Escape(string s)
        => s.Replace("|", "/").Replace("\r", " ").Replace("\n", " ");

    private static string Unescape(string s)
        => s.Trim();
}
