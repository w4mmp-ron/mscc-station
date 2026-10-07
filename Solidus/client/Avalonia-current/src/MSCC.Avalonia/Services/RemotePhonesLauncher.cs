using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;

namespace MSCC.Avalonia.Services;

/// <summary>
/// Start/stop the standalone operator AF app <c>MsccRemotePhones.exe</c>
/// (UDP phones RX/TX / MSA1). Windows-only companion for Remote Audio.
/// Do not use <c>MSCC-Remote.exe</c> — that is the backend servers manager.
/// </summary>
internal static class RemotePhonesLauncher
{
    public const string ExeFileName = "MsccRemotePhones.exe";
    private const string ProcessName = "MsccRemotePhones";

    public static string? ResolveExePath()
    {
        string beside = Path.Combine(AppContext.BaseDirectory, ExeFileName);
        if (File.Exists(beside))
            return beside;

        string deploy = Path.Combine(@"C:\mscc-net9", ExeFileName);
        if (File.Exists(deploy))
            return deploy;

        // Repo / publish layouts
        string[] devHints =
        {
            Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
                @"OneDrive\Documents\GitHub\mscc-station\mscc-remote-audio\MsccRemotePhones\bin\Release\net8.0-windows",
                ExeFileName),
            Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
                @"OneDrive\Documents\GitHub\mscc-station\mscc-remote-audio\MsccRemotePhones\bin\Debug\net8.0-windows",
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

    public static void StartOrShow(Action<string>? log = null)
    {
        if (!OperatingSystem.IsWindows())
        {
            log?.Invoke("RemotePhones launcher skipped (not Windows)");
            return;
        }

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
                    log?.Invoke("MsccRemotePhones already running");
                    return;
                }
            }

            string? exe = ResolveExePath();
            if (exe == null)
            {
                log?.Invoke(
                    "MsccRemotePhones.exe not found — place beside MSCC / in C:\\mscc-net9 " +
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
            log?.Invoke($"Started remote phones: {exe}");
        }
        catch (Exception ex)
        {
            log?.Invoke($"Failed to start MsccRemotePhones: {ex.Message}");
        }
    }

    public static void StopAll(Action<string>? log = null)
    {
        if (!OperatingSystem.IsWindows())
            return;

        try
        {
            KillByName(ProcessName, log);
            KillByName("MSCC-Remote", log); // mistaken prior launches
            WaitUntilGone(ProcessName, 2000);
        }
        catch (Exception ex)
        {
            log?.Invoke($"Failed to stop remote phones: {ex.Message}");
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

    private static void KillByName(string name, Action<string>? log)
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
        log?.Invoke($"Stopped {name}");
    }

    [DllImport("user32.dll")]
    private static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    private static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [SupportedOSPlatform("windows")]
    private static void NativeShowWindow(IntPtr hwnd)
    {
        const int SW_RESTORE = 9;
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
    }
}
