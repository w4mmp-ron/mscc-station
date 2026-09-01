using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace MSCC.Wpf.Favorites;

/// <summary>
/// Client-only persistence for favorites (%LocalAppData%\MSCC-NET9\MSCC_Favorites.ini).
/// Never talks to ms-sdr.
/// </summary>
public static class FavoritesStore
{
    // Pipe-delimited: NAME|BAND|FREQ|MODE|LOWCUT|HIGHCUT|CWFILTER|VFO
    private const char Sep = '|';
    private static string _path = "";

    public static string StorePath
    {
        get
        {
            EnsurePath();
            return _path;
        }
    }

    private static void EnsurePath()
    {
        if (!string.IsNullOrEmpty(_path)) return;
        string appData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        string primary = Path.Combine(appData, "MSCC-NET9", "MSCC_Favorites.ini");
        string initFiles = Path.Combine(@"C:\mscc-net9\init-files", "MSCC_Favorites.ini");
        if (File.Exists(primary))
            _path = primary;
        else if (File.Exists(initFiles))
            _path = initFiles;
        else
            _path = primary;
    }

    public static List<FavoriteEntry> Load()
    {
        EnsurePath();
        var list = new List<FavoriteEntry>();
        if (!File.Exists(_path))
            return list;

        foreach (string raw in File.ReadAllLines(_path))
        {
            string line = raw.Trim();
            if (string.IsNullOrEmpty(line) || line.StartsWith('#') || line.StartsWith(';'))
                continue;

            string[] parts = line.Split(Sep);
            if (parts.Length < 8)
                continue;

            if (!long.TryParse(parts[2].Trim(), out long freq))
                continue;
            if (!int.TryParse(parts[4].Trim(), out int low))
                low = 0;
            if (!int.TryParse(parts[5].Trim(), out int high))
                high = 0;
            if (!int.TryParse(parts[6].Trim(), out int cw))
                cw = 0;

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

        return list;
    }

    public static void Save(IEnumerable<FavoriteEntry> entries)
    {
        EnsurePath();
        string? dir = Path.GetDirectoryName(_path);
        if (!string.IsNullOrEmpty(dir))
            Directory.CreateDirectory(dir);

        var lines = new List<string>
        {
            "# MSCC client-side favorites (not sent to ms-sdr)",
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

        File.WriteAllLines(_path, lines);
    }

    private static string Escape(string s)
        => s.Replace("|", "/").Replace("\r", " ").Replace("\n", " ");

    private static string Unescape(string s)
        => s.Trim();
}
