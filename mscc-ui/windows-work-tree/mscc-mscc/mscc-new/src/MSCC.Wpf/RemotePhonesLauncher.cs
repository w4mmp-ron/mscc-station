using System;
using System.Diagnostics;
using System.IO;
using MSCC.Core.Logging;

namespace MSCC.Wpf;

/// <summary>
/// Start/stop the standalone operator AF app <c>MsccRemotePhones.exe</c>
/// (UDP phones RX/TX). Do <b>not</b> use <c>MSCC-Remote.exe</c> — that is the
/// backend "Remote Servers" manager, a different product.
/// </summary>
internal static class RemotePhonesLauncher
{
    public const string ExeFileName = "MsccRemotePhones.exe";
    private const string ProcessName = "MsccRemotePhones";

    public static string? ResolveExePath()
    {
        // 1) Beside MSCC.Wpf (preferred after copy into C:\mscc-net9)
        string beside = Path.Combine(AppContext.BaseDirectory, ExeFileName);
        if (File.Exists(beside))
            return beside;

        // 2) Deploy folder
        string deploy = Path.Combine(@"C:\mscc-net9", ExeFileName);
        if (File.Exists(deploy))
            return deploy;

        // 3) Dev build output
        string[] devHints =
        {
            Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
                @".grok\worktrees\mscc-remote-audio\MsccRemotePhones\bin\Release\net8.0-windows",
                ExeFileName),
            Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
                @".grok\worktrees\mscc-remote-audio\MsccRemotePhones\bin\Debug\net8.0-windows",
                ExeFileName),
        };
        foreach (string p in devHints)
        {
            if (File.Exists(p))
                return p;
        }

        return null;
    }

    public static bool IsRunning()
    {
        try
        {
            return Process.GetProcessesByName(ProcessName).Length > 0;
        }
        catch
        {
            return false;
        }
    }

    /// <summary>Start if not already running; activate an existing window if possible.</summary>
    public static void StartOrShow()
    {
        try
        {
            var existing = Process.GetProcessesByName(ProcessName);
            if (existing.Length > 0)
            {
                try
                {
                    var p = existing[0];
                    if (p.MainWindowHandle != IntPtr.Zero)
                        NativeShowWindow(p.MainWindowHandle);
                }
                catch { /* ignore activate failures */ }
                DebugMonitor.MonitorTextBoxText(" MsccRemotePhones already running (shown)");
                return;
            }

            string? exe = ResolveExePath();
            if (exe == null)
            {
                DebugMonitor.MonitorTextBoxText(
                    " MsccRemotePhones.exe not found — place it next to MSCC.Wpf.exe (e.g. C:\\mscc-net9) " +
                    "or build mscc-remote-audio\\MsccRemotePhones");
                return;
            }

            var psi = new ProcessStartInfo
            {
                FileName = exe,
                WorkingDirectory = Path.GetDirectoryName(exe) ?? AppContext.BaseDirectory,
                UseShellExecute = true
            };
            Process.Start(psi);
            DebugMonitor.MonitorTextBoxText($" Started remote phones: {exe}");
        }
        catch (Exception ex)
        {
            DebugMonitor.MonitorTextBoxText($" Failed to start MsccRemotePhones: {ex.Message}");
        }
    }

    /// <summary>Kill all MsccRemotePhones processes so background AF stops.</summary>
    public static void StopAll()
    {
        try
        {
            // Also kill mistaken prior launches of the servers UI if still around from the bug
            KillByName(ProcessName);
            KillByName("MSCC-Remote"); // old wrong target — ensure it is not left running
        }
        catch (Exception ex)
        {
            DebugMonitor.MonitorTextBoxText($" Failed to stop remote phones: {ex.Message}");
        }
    }

    private static void KillByName(string name)
    {
        var procs = Process.GetProcessesByName(name);
        if (procs.Length == 0)
            return;
        foreach (var p in procs)
        {
            try
            {
                if (!p.HasExited)
                {
                    p.CloseMainWindow();
                    if (!p.WaitForExit(1500))
                        p.Kill(entireProcessTree: true);
                }
            }
            catch
            {
                try { p.Kill(entireProcessTree: true); } catch { /* ignore */ }
            }
            finally
            {
                try { p.Dispose(); } catch { /* ignore */ }
            }
        }
        DebugMonitor.MonitorTextBoxText($" Stopped {name}");
    }

    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern bool SetForegroundWindow(IntPtr hWnd);

    [System.Runtime.InteropServices.DllImport("user32.dll")]
    private static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    private static void NativeShowWindow(IntPtr hwnd)
    {
        const int SW_RESTORE = 9;
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
    }
}
