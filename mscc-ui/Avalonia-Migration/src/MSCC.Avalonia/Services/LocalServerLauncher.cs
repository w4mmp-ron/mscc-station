using System.Diagnostics;

namespace MSCC.Avalonia.Services;

/// <summary>Start/stop local ms-sdr / sdrcore processes from MSCC UI (Launch checkbox).</summary>
internal static class LocalServerLauncher
{
    public static int RunningCount()
    {
        int n = 0;
        foreach (string name in new[] { "ms-sdr", "sdrcore-recv", "sdrcore-trans" })
        {
            try
            {
                if (Process.GetProcessesByName(name).Length > 0)
                    n++;
            }
            catch
            {
                // ignore
            }
        }
        return n;
    }

    public static bool AllRunning() => RunningCount() == 3;

    public static string? ResolveDesktopCtl()
    {
        const string p = "/usr/local/bin/mscc-desktop-ctl";
        return File.Exists(p) ? p : null;
    }

    public static string? ResolveMscc()
    {
        const string a = "/usr/local/bin/mscc";
        if (File.Exists(a)) return a;
        string home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        string b = Path.Combine(home, "mscc", "mscc.sh");
        return File.Exists(b) ? b : null;
    }

    public static string? ResolveVirtualAudio()
    {
        const string p = "/usr/local/bin/mscc-virtual-audio";
        return File.Exists(p) ? p : null;
    }

    public static IReadOnlyList<string> MissingSetupItems()
    {
        var missing = new List<string>();
        string home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        string msSdr = Path.Combine(home, "mscc", "ms-sdr");
        if (!File.Exists(msSdr) || !IsExecutable(msSdr))
            missing.Add("~/mscc/ms-sdr");
        string cfg = Path.Combine(home, ".local", "mscc");
        if (!Directory.Exists(cfg))
            missing.Add("~/.local/mscc/");
        else
        {
            if (!File.Exists(Path.Combine(cfg, "comm-port.ini")))
                missing.Add("~/.local/mscc/comm-port.ini");
            if (!File.Exists(Path.Combine(cfg, "operator-speaker.ini")))
                missing.Add("~/.local/mscc/operator-speaker.ini");
        }
        return missing;
    }

    private static bool IsExecutable(string path)
    {
        try
        {
            if (OperatingSystem.IsWindows())
                return true;
            var m = File.GetUnixFileMode(path);
            return (m & (UnixFileMode.UserExecute | UnixFileMode.GroupExecute | UnixFileMode.OtherExecute)) != 0;
        }
        catch
        {
            return true;
        }
    }

    public static async Task<int> RunAsync(string file, string args, Action<string> log, TimeSpan timeout)
    {
        var psi = new ProcessStartInfo
        {
            FileName = file,
            Arguments = args ?? "",
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            RedirectStandardInput = true,
            CreateNoWindow = true,
        };
        StripLdLibraryPath(psi);
        using var p = new Process { StartInfo = psi };
        p.OutputDataReceived += (_, e) =>
        {
            if (e.Data != null) log(" [mscc] " + e.Data);
        };
        p.ErrorDataReceived += (_, e) =>
        {
            if (e.Data != null) log(" [mscc] " + e.Data);
        };
        p.Start();
        try { p.StandardInput.Close(); } catch { /* ignore */ }
        p.BeginOutputReadLine();
        p.BeginErrorReadLine();
        using var cts = new CancellationTokenSource(timeout);
        try
        {
            await p.WaitForExitAsync(cts.Token).ConfigureAwait(false);
        }
        catch (OperationCanceledException)
        {
            try { p.Kill(entireProcessTree: true); } catch { /* ignore */ }
            log(" [mscc] timed out");
            return -1;
        }
        return p.ExitCode;
    }

    public static void StartDetachedStop(string logFile)
    {
        string? ctl = ResolveDesktopCtl();
        string? mscc = ResolveMscc();
        string inner;
        if (ctl != null)
            inner = $"\"{ctl}\" stop";
        else if (mscc != null)
            inner = $"\"{mscc}\" stop";
        else
            return;
        string? dir = Path.GetDirectoryName(logFile);
        if (!string.IsNullOrEmpty(dir))
            Directory.CreateDirectory(dir);
        var psi = new ProcessStartInfo
        {
            FileName = "/bin/sh",
            Arguments = $"-c '{inner} >> \"{logFile}\" 2>&1'",
            UseShellExecute = false,
            CreateNoWindow = true,
        };
        StripLdLibraryPath(psi);
        Process.Start(psi);
    }

    internal static void StripLdLibraryPath(ProcessStartInfo psi)
    {
        var drop = new HashSet<string>(StringComparer.Ordinal)
        {
            AppContext.BaseDirectory.TrimEnd('/', '\\'),
            "/opt/mscc-ui",
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "mscc-ui"),
        };
        if (!psi.Environment.TryGetValue("LD_LIBRARY_PATH", out string? cur) || string.IsNullOrEmpty(cur))
            return;
        var kept = cur.Split(':', StringSplitOptions.RemoveEmptyEntries)
            .Select(p => p.TrimEnd('/', '\\'))
            .Where(p => !drop.Contains(p))
            .ToList();
        if (kept.Count == 0)
            psi.Environment.Remove("LD_LIBRARY_PATH");
        else
            psi.Environment["LD_LIBRARY_PATH"] = string.Join(":", kept);
    }
}
