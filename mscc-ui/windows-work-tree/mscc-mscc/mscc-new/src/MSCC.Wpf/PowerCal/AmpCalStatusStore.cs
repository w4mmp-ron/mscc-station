using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace MSCC.Wpf.PowerCal;

/// <summary>
/// Client-only amplifier power-calibration band status.
/// File: %LocalAppData%\MSCC-NET9\amp-cal-status.ini
/// Contains only AMP_B* keys. MSCC owns this file; not sent to ms-sdr.
/// </summary>
public static class AmpCalStatusStore
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

    private static void EnsurePath()
    {
        if (!string.IsNullOrEmpty(_path)) return;
        string appData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        _path = Path.Combine(appData, "MSCC-NET9", "amp-cal-status.ini");
    }

    private static string KeyForBand(int bandNumber) => $"AMP_B{bandNumber}";

    public static void EnsureFileExists()
    {
        EnsurePath();
        string? dir = Path.GetDirectoryName(_path);
        if (!string.IsNullOrEmpty(dir))
            Directory.CreateDirectory(dir);

        if (!File.Exists(_path))
        {
            WriteCleanFile(BandNumbers.ToDictionary(b => b, _ => false));
            return;
        }

        var flags = Load();
        bool missingKey = BandNumbers.Any(b =>
            !File.ReadAllLines(_path).Any(l =>
                l.TrimStart().StartsWith(KeyForBand(b) + "=", StringComparison.OrdinalIgnoreCase)));
        if (missingKey)
            WriteCleanFile(flags);
    }

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
                    result[band] = val is "1" or "true" or "yes"
                        || (int.TryParse(val, out int n) && n != 0);
                    break;
                }
            }
        }

        return result;
    }

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
            "# MSCC amplifier calibration status (client-only; managed by MSCC, not ms-sdr)",
            "# AMP_B* = 0 not calibrated, 1 calibrated"
        };

        foreach (int band in BandNumbers)
        {
            bool cal = bandCalibrated.TryGetValue(band, out bool v) && v;
            lines.Add($"{KeyForBand(band)}={(cal ? "1" : "0")};");
        }

        File.WriteAllLines(_path, lines);
    }
}
