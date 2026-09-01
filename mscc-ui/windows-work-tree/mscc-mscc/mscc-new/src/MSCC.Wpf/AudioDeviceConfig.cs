using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using NAudio.Wave;

namespace MSCC.Wpf;

/// <summary>
/// Enumerate Windows MME devices (NAudio) and read/write Multus audio INI files
/// the same way mscc-init does (substring match in PortAudio / sdrcore).
/// Files under %LocalAppData%\MSCC-NET9\:
///   operator-speaker.ini, operator-microphone.ini,
///   digital-speaker.ini, digital-microphone.ini
/// Content is the short name (text before first '('), used with strstr on PA names.
/// </summary>
public static class AudioDeviceConfig
{
    public const string OperatorSpeakerFile = "operator-speaker.ini";
    public const string OperatorMicFile = "operator-microphone.ini";
    public const string DigitalSpeakerFile = "digital-speaker.ini";
    public const string DigitalMicFile = "digital-microphone.ini";

    public static string ConfigDirectory =>
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "MSCC-NET9");

    public sealed class DeviceChoice
    {
        /// <summary>Full MME product name as shown in the list.</summary>
        public string DisplayName { get; init; } = "";

        /// <summary>Substring written to INI (before '('), matching mscc-init strtok.</summary>
        public string MatchKey { get; init; } = "";

        public override string ToString() => DisplayName;
    }

    public sealed class Settings
    {
        public string OperatorSpeaker { get; set; } = "";
        public string OperatorMic { get; set; } = "";
        public string DigitalSpeaker { get; set; } = "";
        public string DigitalMic { get; set; } = "";
    }

    public static IReadOnlyList<DeviceChoice> GetOutputDevices()
    {
        var list = new List<DeviceChoice>();
        try
        {
            int n = WaveOut.DeviceCount;
            for (int i = 0; i < n; i++)
            {
                var caps = WaveOut.GetCapabilities(i);
                string name = caps.ProductName?.Trim() ?? $"Output {i}";
                list.Add(new DeviceChoice
                {
                    DisplayName = name,
                    MatchKey = ToMatchKey(name),
                });
            }
        }
        catch
        {
            // empty list
        }
        return list;
    }

    public static IReadOnlyList<DeviceChoice> GetInputDevices()
    {
        var list = new List<DeviceChoice>();
        try
        {
            int n = WaveIn.DeviceCount;
            for (int i = 0; i < n; i++)
            {
                var caps = WaveIn.GetCapabilities(i);
                string name = caps.ProductName?.Trim() ?? $"Input {i}";
                list.Add(new DeviceChoice
                {
                    DisplayName = name,
                    MatchKey = ToMatchKey(name),
                });
            }
        }
        catch
        {
            // empty list
        }
        return list;
    }

    /// <summary>mscc-init: strtok(name, "(") — keep text before first parenthesis, trim.</summary>
    public static string ToMatchKey(string fullName)
    {
        if (string.IsNullOrWhiteSpace(fullName))
            return "";
        int paren = fullName.IndexOf('(');
        string key = paren >= 0 ? fullName[..paren] : fullName;
        return key.TrimEnd();
    }

    public static Settings Load()
    {
        return new Settings
        {
            OperatorSpeaker = ReadIni(OperatorSpeakerFile),
            OperatorMic = ReadIni(OperatorMicFile),
            DigitalSpeaker = ReadIni(DigitalSpeakerFile),
            DigitalMic = ReadIni(DigitalMicFile),
        };
    }

    public static bool Save(Settings s)
    {
        try
        {
            Directory.CreateDirectory(ConfigDirectory);
            WriteIni(OperatorSpeakerFile, s.OperatorSpeaker);
            WriteIni(OperatorMicFile, s.OperatorMic);
            WriteIni(DigitalSpeakerFile, s.DigitalSpeaker);
            WriteIni(DigitalMicFile, s.DigitalMic);
            return true;
        }
        catch
        {
            return false;
        }
    }

    public static string ReadIni(string fileName)
    {
        try
        {
            string path = Path.Combine(ConfigDirectory, fileName);
            if (!File.Exists(path))
                return "";
            string line = File.ReadAllText(path);
            // Servers use fgets + strstr: strip CR/LF or match never finds the device.
            return line.TrimEnd('\r', '\n');
        }
        catch
        {
            return "";
        }
    }

    public static void WriteIni(string fileName, string matchKey)
    {
        string path = Path.Combine(ConfigDirectory, fileName);
        // mscc-init writes the short name ONLY (no trailing newline).
        // sdrcore fgets() keeps '\n' in the buffer and strstr()'s it against PortAudio
        // names — a trailing newline makes match fail → "No output device found".
        string text = (matchKey ?? "").TrimEnd('\r', '\n', ' ', '\t');
        // Keep a single trailing space like some historic Multus INIs (optional aid to prefix match)
        if (text.Length > 0 && !text.EndsWith(' '))
            text += " ";
        File.WriteAllText(path, text);
    }

    /// <summary>
    /// Pick list index matching a saved INI key. Returns -1 when nothing is saved or no match
    /// (UI should stay blank so first-run users must choose explicitly — do not default to index 0).
    /// </summary>
    public static int FindBestIndex(IReadOnlyList<DeviceChoice> devices, string savedKey)
    {
        if (devices.Count == 0)
            return -1;
        string want = (savedKey ?? "").Trim();
        if (string.IsNullOrEmpty(want))
            return -1;

        // Exact MatchKey (mscc-init short name)
        for (int i = 0; i < devices.Count; i++)
        {
            string key = devices[i].MatchKey.Trim();
            if (string.Equals(key, want, StringComparison.OrdinalIgnoreCase))
                return i;
        }
        // Prefix / contains — same idea as PortAudio strstr on device name
        for (int i = 0; i < devices.Count; i++)
        {
            string key = devices[i].MatchKey.Trim();
            string display = devices[i].DisplayName;
            if (display.StartsWith(want, StringComparison.OrdinalIgnoreCase) ||
                display.Contains(want, StringComparison.OrdinalIgnoreCase) ||
                (!string.IsNullOrEmpty(key) && want.StartsWith(key, StringComparison.OrdinalIgnoreCase)))
                return i;
        }
        return -1;
    }

    /// <summary>True if saved INI key matches a currently enumerated device (for setup gate).</summary>
    public static bool SavedKeyMatchesDevice(string savedKey, IReadOnlyList<DeviceChoice> devices) =>
        FindBestIndex(devices, savedKey) >= 0;
}
