using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace MSCC.Wpf;

/// <summary>
/// External WiFi SWR meter preferences (HF / LF profiles). Stored in MSCC_Client.ini.
/// </summary>
public static class SwrMeterSettings
{
    public static bool Enabled { get; set; }
    public static int UdpListenPort { get; set; } = 6999;

    /// <summary>Optional HF meter IP (HTTP reset / identity). Auto-filled from UDP when empty.</summary>
    public static string HfMeterIp { get; set; } = "";

    /// <summary>Optional LF meter IP.</summary>
    public static string LfMeterIp { get; set; } = "";

    public static string ConfigPath
    {
        get
        {
            string appData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            return Path.Combine(appData, "MSCC-NET9", "MSCC_Client.ini");
        }
    }

    public static void Load()
    {
        try
        {
            string path = ConfigPath;
            if (!File.Exists(path))
                return;
            foreach (string line in File.ReadAllLines(path))
            {
                if (LineKey(line, "SWR_ENABLE")) Enabled = ParseBool(line, Enabled);
                if (LineKey(line, "SWR_UDP_PORT")) UdpListenPort = Math.Clamp(ParseInt(line, UdpListenPort), 1, 65535);
                if (LineKey(line, "SWR_HF_IP")) HfMeterIp = ParseString(line, HfMeterIp);
                if (LineKey(line, "SWR_LF_IP")) LfMeterIp = ParseString(line, LfMeterIp);
            }
        }
        catch { /* defaults */ }
    }

    public static void Save()
    {
        try
        {
            string path = ConfigPath;
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            var lines = File.Exists(path) ? File.ReadAllLines(path).ToList() : new List<string>();
            Set(lines, "SWR_ENABLE", Enabled ? "1" : "0");
            Set(lines, "SWR_UDP_PORT", UdpListenPort.ToString());
            Set(lines, "SWR_HF_IP", HfMeterIp ?? "");
            Set(lines, "SWR_LF_IP", LfMeterIp ?? "");
            File.WriteAllLines(path, lines);
        }
        catch { /* ignore */ }
    }

    public static string ActiveMeterIp(bool geminus)
    {
        string ip = geminus ? LfMeterIp : HfMeterIp;
        return (ip ?? "").Trim();
    }

    public static void SetActiveMeterIp(bool geminus, string ip)
    {
        ip = (ip ?? "").Trim();
        if (geminus) LfMeterIp = ip;
        else HfMeterIp = ip;
        Save();
    }

    private static bool LineKey(string line, string key) =>
        line.TrimStart().StartsWith(key + "=", StringComparison.OrdinalIgnoreCase) ||
        line.Contains(key + "=", StringComparison.OrdinalIgnoreCase);

    private static string ParseString(string line, string fallback)
    {
        int eq = line.IndexOf('=');
        if (eq < 0) return fallback;
        string v = line[(eq + 1)..].Trim().TrimEnd(';');
        return string.IsNullOrEmpty(v) ? fallback : v;
    }

    private static int ParseInt(string line, int fallback)
    {
        string v = ParseString(line, "");
        return int.TryParse(v, out int n) ? n : fallback;
    }

    private static bool ParseBool(string line, bool fallback)
    {
        string v = ParseString(line, "").ToLowerInvariant();
        if (v is "1" or "true" or "yes" or "on") return true;
        if (v is "0" or "false" or "no" or "off") return false;
        return fallback;
    }

    private static void Set(List<string> lines, string key, string value)
    {
        string prefix = key + "=";
        for (int i = 0; i < lines.Count; i++)
        {
            if (lines[i].TrimStart().StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
            {
                lines[i] = prefix + value + ";";
                return;
            }
        }
        lines.Add(prefix + value + ";");
    }
}
