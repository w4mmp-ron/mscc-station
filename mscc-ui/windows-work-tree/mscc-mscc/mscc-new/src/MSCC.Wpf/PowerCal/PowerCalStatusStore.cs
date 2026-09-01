using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace MSCC.Wpf.PowerCal;

/// <summary>
/// Client-only power-calibration (QRP CAL) band status.
/// File: %LocalAppData%\MSCC-NET9\client-settings.ini
/// HF: PROFICIO_B* keys; LF (Geminus): GEMINUS_B2200 / GEMINUS_B630 (matches original MSCC).
/// </summary>
public static class PowerCalStatusStore
{
    /// <summary>Band numbers in display order (LF then HF).</summary>
    public static readonly int[] BandNumbers = { 2200, 630, 160, 80, 60, 40, 30, 20, 17, 15, 12, 10 };

    private static string _path = "";

    public static string StorePath
    {
        get
        {
            EnsurePath();
            return _path;
        }
    }

    /// <summary>
    /// Always %LocalAppData%\MSCC-NET9\client-settings.ini (MSCC-owned).
    /// </summary>
    private static void EnsurePath()
    {
        if (!string.IsNullOrEmpty(_path)) return;
        string appData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        _path = Path.Combine(appData, "MSCC-NET9", "client-settings.ini");
    }

    /// <summary>INI key for a band (original: PROFICIO_B160…10, GEMINUS_B2200/630).</summary>
    private static string KeyForBand(int bandNumber) =>
        bandNumber is 2200 or 630 ? $"GEMINUS_B{bandNumber}" : $"PROFICIO_B{bandNumber}";

    private static bool IsKnownStatusKey(string key) =>
        key.StartsWith("PROFICIO_B", StringComparison.OrdinalIgnoreCase) ||
        key.StartsWith("GEMINUS_B", StringComparison.OrdinalIgnoreCase);

    /// <summary>
    /// Ensure a clean client status file exists with only PROFICIO_B* / GEMINUS_B* keys.
    /// Does not copy from C:\mscc-net9\init-files or any other source.
    /// </summary>
    public static void EnsureFileExists()
    {
        EnsurePath();
        string? dir = Path.GetDirectoryName(_path);
        if (!string.IsNullOrEmpty(dir))
            Directory.CreateDirectory(dir);

        if (!File.Exists(_path))
        {
            // New clean file: band status only
            WriteCleanFile(BandNumbers.ToDictionary(b => b, _ => false));
            return;
        }

        // File exists but may be an old mixed client-settings.ini — rewrite to status-only
        // if it contains non-status content, or ensure all keys present.
        var flags = Load();
        bool hasOnlyStatusKeys = File.ReadAllLines(_path)
            .Select(l => l.Trim())
            .Where(l => l.Length > 0 && !l.StartsWith('#') && !l.StartsWith(';'))
            .All(l =>
            {
                int eq = l.IndexOf('=');
                string key = eq > 0 ? l.Substring(0, eq).Trim() : l;
                return IsKnownStatusKey(key);
            });

        bool missingKey = BandNumbers.Any(b =>
            !File.ReadAllLines(_path).Any(l =>
                l.TrimStart().StartsWith(KeyForBand(b) + "=", StringComparison.OrdinalIgnoreCase)));

        if (!hasOnlyStatusKeys || missingKey)
            WriteCleanFile(flags);
    }

    /// <summary>
    /// Load calibrated flags. Missing keys default to false (0).
    /// </summary>
    public static Dictionary<int, bool> Load()
    {
        EnsurePath();
        var result = BandNumbers.ToDictionary(b => b, _ => false);
        if (!File.Exists(_path))
            return result;

        foreach (string raw in File.ReadAllLines(_path))
        {
            string line = raw.Trim();
            if (string.IsNullOrEmpty(line) || line.StartsWith('#') || line.StartsWith(';'))
                continue;
            if (!line.Contains('='))
                continue;

            int eq = line.IndexOf('=');
            string key = line.Substring(0, eq).Trim().ToUpperInvariant();
            string val = line.Substring(eq + 1).Trim().TrimEnd(';', ' ');

            foreach (int band in BandNumbers)
            {
                if (key == KeyForBand(band).ToUpperInvariant())
                {
                    // Accept 1 / true / yes, or any non-zero integer (matches original byte flags)
                    result[band] = val is "1" or "true" or "yes"
                        || (int.TryParse(val, out int n) && n != 0);
                    break;
                }
            }
        }

        return result;
    }

    /// <summary>
    /// Overwrite the status file with only PROFICIO_B* / GEMINUS_B* keys (clean client-owned format).
    /// </summary>
    public static void Save(IReadOnlyDictionary<int, bool> bandCalibrated)
    {
        EnsurePath();
        WriteCleanFile(bandCalibrated);
    }

    private static void WriteCleanFile(IReadOnlyDictionary<int, bool> bandCalibrated)
    {
        EnsurePath();
        string? dir = Path.GetDirectoryName(_path);
        if (!string.IsNullOrEmpty(dir))
            Directory.CreateDirectory(dir);

        var lines = new List<string>
        {
            "# MSCC QRP power calibration status (client-only; managed by MSCC, not ms-sdr)",
            "# PROFICIO_B* = HF bands; GEMINUS_B2200/630 = LF; 0 = not calibrated, 1 = calibrated"
        };

        foreach (int band in BandNumbers)
        {
            bool cal = bandCalibrated.TryGetValue(band, out bool v) && v;
            lines.Add($"{KeyForBand(band)}={(cal ? "1" : "0")};");
        }

        File.WriteAllLines(_path, lines);
    }
}
