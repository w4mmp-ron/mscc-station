using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;
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

        // 3) Dev build / repo drop (GitHub tree first; old grok worktree last)
        string user = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        string[] devHints =
        {
            Path.Combine(user,
                @"OneDrive\Documents\GitHub\mscc-station\mscc-remote-audio\MsccRemotePhones\bin\Release\net8.0-windows",
                ExeFileName),
            Path.Combine(user,
                @"OneDrive\Documents\GitHub\mscc-station\mscc-remote-audio\MsccRemotePhones\bin\Debug\net8.0-windows",
                ExeFileName),
            Path.Combine(user,
                @"OneDrive\Documents\GitHub\mscc-station\mscc-ui\Release\windows-wpf",
                ExeFileName),
            Path.Combine(user,
                @".grok\worktrees\mscc-remote-audio\MsccRemotePhones\bin\Release\net8.0-windows",
                ExeFileName),
            Path.Combine(user,
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
            return Process.GetProcessesByName(ProcessName).Any(p =>
            {
                try { return !p.HasExited; }
                catch { return false; }
            });
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
            /* After StopAll, a dying process can still be listed — wait, then start fresh. */
            if (!WaitUntilGone(ProcessName, 1500))
            {
                var live = Process.GetProcessesByName(ProcessName)
                    .Where(p => { try { return !p.HasExited; } catch { return false; } })
                    .ToArray();
                if (live.Length > 0)
                {
                    try
                    {
                        var p = live[0];
                        if (p.MainWindowHandle != IntPtr.Zero)
                            NativeShowWindow(p.MainWindowHandle);
                    }
                    catch { /* ignore activate failures */ }
                    finally
                    {
                        foreach (var p in live) { try { p.Dispose(); } catch { /* ignore */ } }
                    }
                    DebugMonitor.MonitorTextBoxText(" MsccRemotePhones already running (shown)");
                    return;
                }
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
            KillByName(ProcessName);
            KillByName("MSCC-Remote"); // old wrong target — ensure it is not left running
            WaitUntilGone(ProcessName, 2000);
        }
        catch (Exception ex)
        {
            DebugMonitor.MonitorTextBoxText($" Failed to stop remote phones: {ex.Message}");
        }
    }

    /// <returns>True if no live processes remain.</returns>
    private static bool WaitUntilGone(string name, int timeoutMs)
    {
        var sw = Stopwatch.StartNew();
        while (sw.ElapsedMilliseconds < timeoutMs)
        {
            Process[] procs;
            try { procs = Process.GetProcessesByName(name); }
            catch { return true; }
            bool anyLive = false;
            foreach (var p in procs)
            {
                try
                {
                    if (!p.HasExited)
                        anyLive = true;
                }
                catch { /* ignore */ }
                finally
                {
                    try { p.Dispose(); } catch { /* ignore */ }
                }
            }
            if (!anyLive)
                return true;
            Thread.Sleep(50);
        }
        return false;
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
                    p.WaitForExit(1000);
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
