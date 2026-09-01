using System;
using System.IO;

namespace MSCC.Core.Logging;

/// <summary>
/// Static logger that replicates the original MonitorTextBoxText / LogWrite behavior.
/// Writes to a log file in LocalApplicationData\MSCC-NET9\logs\mscc.log (with timestamp header per entry).
/// This matches the folder structure created by initialize.bat in C:\mscc-net9 (which xcopies init-files to %localappdata%\MSCC-NET9 ).
/// Raises LogMessage for UI consumers to display (file write always happens; UI display can be suspended by consumer).
/// </summary>
public static class DebugMonitor
{
    private static readonly object _lock = new();
    private static string? _logFilePath;
    private static long _lineCount;
    private static bool _initialized;

    /// <summary>
    /// Fired for every logged message (prefixed with line number). Consumers should handle thread affinity (e.g. Dispatcher).
    /// </summary>
    public static event Action<string>? LogMessage;

    /// <summary>
    /// Initialize the log file location. Call once early (e.g. in VM ctor). Safe to call multiple times.
    /// </summary>
    public static void Initialize(string? baseFolder = null)
    {
        if (_initialized) return;

        string folder = baseFolder ?? Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "MSCC-NET9", "logs");

        Directory.CreateDirectory(folder);
        _logFilePath = Path.Combine(folder, "mscc.log");

        // Match original Manage_Log_File: delete previous log on (re)start
        if (File.Exists(_logFilePath))
        {
            try
            {
                File.Delete(_logFilePath);
                _lineCount = 0;
            }
            catch
            {
                // ignore; will append anyway
            }
        }

        _initialized = true;
    }

    /// <summary>
    /// The main entry point, equivalent to original MonitorTextBoxText.
    /// Always writes to the log file (with date/time header).
    /// Raises LogMessage with "linecount + text" so UI can decide to display (based on its suspend flag).
    /// </summary>
    public static void MonitorTextBoxText(string text)
    {
        if (!_initialized)
        {
            Initialize();
        }

        if (string.IsNullOrEmpty(text))
        {
            return;
        }

        string messageText = _lineCount++ + text;

        // Always write to file (original LogWrite is unconditional)
        WriteLogEntry(messageText);

        // Raise for UI (original would append only if !monitor_suspend)
        LogMessage?.Invoke(messageText);
    }

    private static void WriteLogEntry(string logMessage)
    {
        if (string.IsNullOrEmpty(_logFilePath)) return;

        lock (_lock)
        {
            try
            {
                using var writer = File.AppendText(_logFilePath);
                writer.WriteLine("{0} {1}", DateTime.Now.ToLongTimeString(), DateTime.Now.ToLongDateString());
                writer.WriteLine("  :{0}", logMessage);
            }
            catch
            {
                // In original this would show error and potentially exit; here we silently continue for robustness.
                // Could log to Debug/Console if desired.
            }
        }
    }

    /// <summary>
    /// Optional: call to reset the log file (similar to original daily reset or RESET LOGS button).
    /// </summary>
    public static void ResetLogFile()
    {
        lock (_lock)
        {
            if (!string.IsNullOrEmpty(_logFilePath) && File.Exists(_logFilePath))
            {
                try
                {
                    File.Delete(_logFilePath);
                }
                catch { }
            }
            _lineCount = 0;
        }
    }
}
