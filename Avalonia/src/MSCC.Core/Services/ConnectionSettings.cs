using System;
using System.IO;

namespace MSCC.Core.Services;

/// <summary>
/// Configuration for connecting to the real MSCC backend servers.
/// Ports and IPs are loaded from MSCC_Client.ini (the client's own initialization file).
/// </summary>
public record ConnectionSettings(
    string RemoteIp,      // PROFICIO_DLL_IP (target for sending commands to the backend/ms-sdr)
    int RemotePort,       // PROFICIO_DLL_PORT (default 8888)
    int LocalPort = 8889  // MSCC_PORT (port to bind local receive socket to, default 8889)
)
{
    /// <summary>
    /// Loads defaults by reading the client's initialization INI (MSCC_Client.ini exclusively).
    /// Creates the file with defaults if it does not exist.
    /// </summary>
    public static ConnectionSettings Default => LoadFromIni();

    private static ConnectionSettings LoadFromIni(string? iniPath = null)
    {
        string appData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        string primaryIni = Path.Combine(appData, "MSCC-NET9", "MSCC_Client.ini");
        string initFilesIni = @"C:\mscc-net9\init-files\MSCC_Client.ini";

        if (string.IsNullOrEmpty(iniPath))
        {
            if (File.Exists(primaryIni))
            {
                iniPath = primaryIni;
            }
            else if (File.Exists(initFilesIni))
            {
                iniPath = initFilesIni;
            }
            else
            {
                // No existing file: use primary (where SpectrumWaterfallSettings creates on first run)
                iniPath = primaryIni;
            }
        }

        if (!File.Exists(iniPath))
        {
            // Create MSCC_Client.ini (exclusively for the MSCC client) with default connection values if missing.
            // Spectrum/waterfall/UI defaults will be merged in by SpectrumWaterfallSettings.Load/Save.
            Directory.CreateDirectory(Path.GetDirectoryName(iniPath)!);
            var lines = new List<string>
            {
                "PROFICIO_DLL_IP=127.0.0.1;",
                "PROFICIO_DLL_PORT=8888;",
                "MSCC_PORT=8889;"
            };
            File.WriteAllLines(iniPath, lines);
        }

        int guiPort = 8889;
        int dllPort = 8888;
        string dllIp = "127.0.0.1";

        if (File.Exists(iniPath))
        {
            try
            {
                foreach (string line in File.ReadAllLines(iniPath))
                {
                    if (line.Contains("PROFICIO_DLL_PORT")) dllPort = ParseIniInt(line, dllPort);
                    if (line.Contains("MSCC_PORT")) guiPort = ParseIniInt(line, guiPort);
                    if (line.Contains("PROFICIO_DLL_IP")) dllIp = ParseIniString(line, dllIp);
                }
            }
            catch { /* use defaults on parse error */ }
        }

        return new ConnectionSettings(dllIp, dllPort, guiPort);
    }

    private static int ParseIniInt(string line, int defaultValue)
    {
        if (!line.Contains("=")) return defaultValue;
        int eq = line.IndexOf('=');
        string val = line.Substring(eq + 1).Trim().TrimEnd(';', ' ');
        return int.TryParse(val, out int result) ? result : defaultValue;
    }

    private static string ParseIniString(string line, string defaultValue)
    {
        if (!line.Contains("=")) return defaultValue;
        int eq = line.IndexOf('=');
        string val = line.Substring(eq + 1).Trim().TrimEnd(';', ' ');
        return string.IsNullOrWhiteSpace(val) ? defaultValue : val;
    }

    public override string ToString() => $"{RemoteIp}:{RemotePort} (local rx port {LocalPort})";
}
