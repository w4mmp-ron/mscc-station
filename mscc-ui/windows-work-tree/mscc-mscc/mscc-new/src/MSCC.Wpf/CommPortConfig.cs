using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Text.RegularExpressions;

namespace MSCC.Wpf;

/// <summary>
/// Read/write ms-sdr <c>comm-port.ini</c> (same format as mscc-init / Start_Serial_Port).
/// Path: %LocalAppData%\MSCC-NET9\comm-port.ini
/// </summary>
public static class CommPortConfig
{
    public const string FileName = "comm-port.ini";

    /// <summary>Baud rate index → bps (matches ms-sdr baud_rates[]).</summary>
    public static readonly int[] BaudRates = { 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200 };

    public static string ConfigDirectory =>
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "MSCC-NET9");

    public static string ConfigPath => Path.Combine(ConfigDirectory, FileName);

    public sealed class Settings
    {
        public string PortName { get; set; } = "COM1";
        public int CommPortIndex { get; set; }
        public int BaudRateIndex { get; set; } = 3; // 9600
        public int ParityIndex { get; set; } // 0=none
        public int DataBitsIndex { get; set; } = 1; // 8
        public int StopBitsIndex { get; set; } // 0=1 stop
        public int Pin { get; set; } = 1; // RTS/CTS PTT style pin enable as used by Multus
    }

    /// <summary>Enumerate system serial ports (sorted). Never throws.</summary>
    public static IReadOnlyList<string> GetAvailablePorts()
    {
        try
        {
            return SerialPort.GetPortNames()
                .Select(NormalizePortName)
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .OrderBy(NaturalComSort)
                .ToList();
        }
        catch
        {
            return Array.Empty<string>();
        }
    }

    public static Settings Load()
    {
        var s = new Settings();
        try
        {
            string path = ConfigPath;
            if (!File.Exists(path))
                return s;

            string line = File.ReadAllText(path).Trim();
            if (string.IsNullOrEmpty(line))
                return s;

            // COMM_PORT_NAME=COMx,COMM_PORT_INDEX=0,BAUD_RATE_INDEX=3,...
            var name = MatchField(line, @"COMM_PORT_NAME=([^,;]+)");
            if (!string.IsNullOrWhiteSpace(name))
                s.PortName = NormalizePortName(name);

            s.CommPortIndex = ParseIntField(line, "COMM_PORT_INDEX", s.CommPortIndex);
            s.BaudRateIndex = ParseIntField(line, "BAUD_RATE_INDEX", s.BaudRateIndex);
            s.ParityIndex = ParseIntField(line, "PARITY_INDEX", s.ParityIndex);
            s.DataBitsIndex = ParseIntField(line, "DATA_BITS_INDEX", s.DataBitsIndex);
            s.StopBitsIndex = ParseIntField(line, "STOP_BITS_INDEX", s.StopBitsIndex);
            s.Pin = ParseIntField(line, "PIN", s.Pin);
        }
        catch
        {
            // keep defaults
        }

        s.BaudRateIndex = Math.Clamp(s.BaudRateIndex, 0, BaudRates.Length - 1);
        s.ParityIndex = Math.Clamp(s.ParityIndex, 0, 2);
        s.DataBitsIndex = Math.Clamp(s.DataBitsIndex, 0, 2);
        s.StopBitsIndex = Math.Clamp(s.StopBitsIndex, 0, 1);
        return s;
    }

    /// <summary>
    /// Write comm-port.ini in the exact single-line format ms-sdr expects.
    /// Returns true on success.
    /// </summary>
    public static bool Save(Settings s)
    {
        try
        {
            Directory.CreateDirectory(ConfigDirectory);
            string port = NormalizePortName(s.PortName);
            int baudIdx = Math.Clamp(s.BaudRateIndex, 0, BaudRates.Length - 1);
            int parityIdx = Math.Clamp(s.ParityIndex, 0, 2);
            int dataIdx = Math.Clamp(s.DataBitsIndex, 0, 2);
            int stopIdx = Math.Clamp(s.StopBitsIndex, 0, 1);
            int pin = s.Pin != 0 ? 1 : 0;

            // Match historical default line (trailing semicolon, no extra spaces)
            string line =
                $"COMM_PORT_NAME={port},COMM_PORT_INDEX={s.CommPortIndex},BAUD_RATE_INDEX={baudIdx}," +
                $"PARITY_INDEX={parityIdx},DATA_BITS_INDEX={dataIdx},STOP_BITS_INDEX={stopIdx},PIN={pin};";

            File.WriteAllText(ConfigPath, line + Environment.NewLine);
            return true;
        }
        catch
        {
            return false;
        }
    }

    private static string? MatchField(string line, string pattern)
    {
        var m = Regex.Match(line, pattern, RegexOptions.IgnoreCase);
        return m.Success ? m.Groups[1].Value.Trim() : null;
    }

    private static int ParseIntField(string line, string key, int fallback)
    {
        var m = Regex.Match(line, key + @"=(\d+)", RegexOptions.IgnoreCase);
        return m.Success && int.TryParse(m.Groups[1].Value, out int v) ? v : fallback;
    }

    public static string NormalizePortName(string name)
    {
        name = (name ?? "").Trim().ToUpperInvariant();
        if (name.StartsWith("\\\\.\\"))
            name = name[4..];
        if (!name.StartsWith("COM", StringComparison.OrdinalIgnoreCase) && int.TryParse(name, out int n))
            name = "COM" + n;
        return name;
    }

    private static string NaturalComSort(string port)
    {
        // COM3 before COM10
        var m = Regex.Match(port, @"COM(\d+)", RegexOptions.IgnoreCase);
        if (m.Success && int.TryParse(m.Groups[1].Value, out int n))
            return n.ToString("D4");
        return port;
    }
}
