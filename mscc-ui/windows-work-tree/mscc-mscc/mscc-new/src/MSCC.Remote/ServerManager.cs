using System.Diagnostics;
using System.Text;
using System.Text.RegularExpressions;

namespace MSCC.Remote;

/// <summary>
/// Host-side backend control (same role as Start-MsccServers.bat).
/// Finds binaries next to this exe (typically C:\mscc-net9).
/// </summary>
public sealed class ServerManager
{
    public const int StartGapMs = 2000;
    public const string TransExe = "Mscc-trans.exe";
    public const string RecvExe = "mscc-recv.exe";

    private static readonly string[] SdrCandidates =
    {
        "ms-sdr-MKII.exe",
        "ms-sdr-proficio.exe",
        "ms-sdr.exe",
    };

    public string ServerRoot { get; }
    public string SdrExeName { get; private set; } = SdrCandidates[0];
    public string MsccIniPath { get; }

    public ServerManager(string? serverRoot = null)
    {
        ServerRoot = string.IsNullOrWhiteSpace(serverRoot)
            ? AppContext.BaseDirectory.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)
            : serverRoot.Trim().TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);

        MsccIniPath = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "MSCC-NET9",
            "mscc.ini");

        ResolveSdrExe();
    }

    private void ResolveSdrExe()
    {
        foreach (string name in SdrCandidates)
        {
            if (File.Exists(Path.Combine(ServerRoot, name)))
            {
                SdrExeName = name;
                return;
            }
        }
        SdrExeName = SdrCandidates[0];
    }

    public IEnumerable<string> RequiredExes() => new[] { TransExe, RecvExe, SdrExeName };

    public string? EnsurePresent()
    {
        ResolveSdrExe();
        foreach (string name in RequiredExes())
        {
            if (!File.Exists(Path.Combine(ServerRoot, name)))
                return $"Missing: {Path.Combine(ServerRoot, name)}";
        }
        return null;
    }

    public bool IsRunning(string exeFileName)
    {
        string baseName = Path.GetFileNameWithoutExtension(exeFileName);
        Process[] list;
        try
        {
            list = Process.GetProcessesByName(baseName);
        }
        catch
        {
            return false;
        }

        try
        {
            foreach (var p in list)
            {
                try
                {
                    if (!p.HasExited)
                        return true;
                }
                catch { /* access denied / exited */ }
            }
            return false;
        }
        finally
        {
            foreach (var p in list)
            {
                try { p.Dispose(); } catch { /* ignore */ }
            }
        }
    }

    public void AppendStatus(StringBuilder sb)
    {
        sb.AppendLine($"Folder: {ServerRoot}");
        sb.AppendLine("Process status:");
        foreach (string name in RequiredExes())
        {
            bool run = File.Exists(Path.Combine(ServerRoot, name)) && IsRunning(name);
            sb.AppendLine(run ? $"  RUNNING  {name}" : $"  stopped  {name}");
        }
        sb.AppendLine();
        AppendKeyerStatus(sb);
    }

    public void AppendKeyerStatus(StringBuilder sb)
    {
        sb.AppendLine("Host keyer (ms-sdr reads at start):");
        sb.AppendLine($"  File: {MsccIniPath}");
        if (!File.Exists(MsccIniPath))
        {
            sb.AppendLine("  PROFICIO-MKII: not set (ms-sdr default = 1 / MKII)");
            return;
        }

        try
        {
            foreach (string raw in File.ReadAllLines(MsccIniPath))
            {
                string line = raw.Trim();
                if (line.StartsWith("PROFICIO-MKII=", StringComparison.OrdinalIgnoreCase)
                    || line.StartsWith("PROFICIO_MKII=", StringComparison.OrdinalIgnoreCase))
                {
                    int eq = line.IndexOf('=');
                    string rest = eq >= 0 ? line[(eq + 1)..].Trim().TrimEnd(';').Trim() : "";
                    if (rest.StartsWith('0'))
                        sb.AppendLine("  PROFICIO-MKII=0  LEGACY / external electronic keyer");
                    else
                        sb.AppendLine($"  PROFICIO-MKII={rest}  MKII internal keyer");
                    return;
                }
            }
        }
        catch (Exception ex)
        {
            sb.AppendLine($"  (read error: {ex.Message})");
            return;
        }

        sb.AppendLine("  PROFICIO-MKII: not set (ms-sdr default = 1 / MKII)");
    }

    /// <summary>Write PROFICIO-MKII=0|1. mkii true → 1.</summary>
    public void WriteProficioMkii(bool mkii, StringBuilder log)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(MsccIniPath)!);
        string val = mkii ? "1" : "0";
        string text = File.Exists(MsccIniPath) ? File.ReadAllText(MsccIniPath) : "";
        text = Regex.Replace(text, @"(?im)^\s*\$\d+\.\d+\.\d+\.\d+;?\s*\r?\n?", "");
        text = Regex.Replace(text, @"(?im)^\s*PROFICIO_MKII\s*=[^\r\n]*\r?\n?", "");
        if (Regex.IsMatch(text, @"(?im)^\s*PROFICIO-MKII\s*="))
            text = Regex.Replace(text, @"(?im)^(\s*PROFICIO-MKII\s*=)[^;\r\n]*", "${1}" + val);
        else
        {
            if (text.Length > 0 && !text.EndsWith('\n'))
                text += Environment.NewLine;
            text += $"PROFICIO-MKII={val};{Environment.NewLine}";
        }
        File.WriteAllText(MsccIniPath, text);
        log.AppendLine($"Wrote PROFICIO-MKII={val} to {MsccIniPath}");
        log.AppendLine(mkii
            ? "  Host keyer: MKII internal keyer (PROFICIO-MKII=1)"
            : "  Host keyer: LEGACY / external electronic keyer (PROFICIO-MKII=0)");
    }

    public async Task StartAllAsync(StringBuilder log, CancellationToken ct = default)
    {
        string? missing = EnsurePresent();
        if (missing != null)
        {
            log.AppendLine("ERROR: " + missing);
            return;
        }

        log.AppendLine($"Starting backends in {ServerRoot} ...");
        log.AppendLine("  order: trans → recv → ms-sdr");
        await StartOneAsync(TransExe, log, ct);
        await Task.Delay(StartGapMs, ct);
        await StartOneAsync(RecvExe, log, ct);
        await Task.Delay(StartGapMs, ct);
        await StartOneAsync(SdrExeName, log, ct);
        log.AppendLine("Done.");
        AppendKeyerStatus(log);
    }

    public void StopAll(StringBuilder log)
    {
        log.AppendLine("Stopping backends...");
        StopOne(SdrExeName, log);
        StopOne(RecvExe, log);
        StopOne(TransExe, log);
    }

    public async Task RestartAllAsync(StringBuilder log, CancellationToken ct = default)
    {
        string? missing = EnsurePresent();
        if (missing != null)
        {
            log.AppendLine("ERROR: " + missing);
            return;
        }

        log.AppendLine($"Restarting in {ServerRoot} ...");
        StopOne(SdrExeName, log);
        StopOne(RecvExe, log);
        StopOne(TransExe, log);
        await Task.Delay(2000, ct);
        await StartOneAsync(TransExe, log, ct);
        await Task.Delay(StartGapMs, ct);
        await StartOneAsync(RecvExe, log, ct);
        await Task.Delay(StartGapMs, ct);
        await StartOneAsync(SdrExeName, log, ct);
        AppendKeyerStatus(log);
    }

    private async Task StartOneAsync(string exeName, StringBuilder log, CancellationToken ct)
    {
        if (IsRunning(exeName))
        {
            log.AppendLine($"  [skip] {exeName} already running");
            return;
        }

        string path = Path.Combine(ServerRoot, exeName);
        if (!File.Exists(path))
        {
            log.AppendLine($"  [fail] not found: {path}");
            return;
        }

        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = path,
                WorkingDirectory = ServerRoot,
                Arguments = "test",
                UseShellExecute = false,
                CreateNoWindow = true,
            };
            Process.Start(psi);
            // Brief settle so next IsRunning is meaningful
            await Task.Delay(300, ct);
            log.AppendLine(IsRunning(exeName)
                ? $"  [ok]   started {exeName}"
                : $"  [warn] launched {exeName} but process not seen yet");
        }
        catch (Exception ex)
        {
            log.AppendLine($"  [fail] could not launch {exeName}: {ex.Message}");
        }
    }

    private void StopOne(string exeName, StringBuilder log)
    {
        if (!IsRunning(exeName))
        {
            log.AppendLine($"  [skip] {exeName} was not running");
            return;
        }

        string baseName = Path.GetFileNameWithoutExtension(exeName);
        log.AppendLine($"  Stopping {exeName}...");
        try
        {
            foreach (var p in Process.GetProcessesByName(baseName))
            {
                try
                {
                    if (!p.HasExited)
                    {
                        p.Kill(entireProcessTree: true);
                        p.WaitForExit(3000);
                    }
                }
                catch (Exception ex)
                {
                    log.AppendLine($"  [warn] {exeName}: {ex.Message}");
                }
                finally
                {
                    try { p.Dispose(); } catch { /* ignore */ }
                }
            }
        }
        catch (Exception ex)
        {
            log.AppendLine($"  [warn] {exeName}: {ex.Message}");
        }

        Thread.Sleep(500);
        log.AppendLine(IsRunning(exeName)
            ? $"  [warn] {exeName} still present"
            : $"  [ok]   stopped {exeName}");
    }

    /// <summary>CLI headless command. Returns process exit code.</summary>
    public static async Task<int> RunCliAsync(string[] args)
    {
        var mgr = new ServerManager();
        var log = new StringBuilder();
        string cmd = args[0].Trim().ToLowerInvariant();

        try
        {
            switch (cmd)
            {
                case "start":
                case "silent":
                case "auto":
                    await mgr.StartAllAsync(log);
                    break;
                case "stop":
                    mgr.StopAll(log);
                    break;
                case "restart":
                    await mgr.RestartAllAsync(log);
                    break;
                case "status":
                    mgr.AppendStatus(log);
                    break;
                case "legacy":
                    mgr.WriteProficioMkii(mkii: false, log);
                    await mgr.RestartAllAsync(log);
                    break;
                case "mkii":
                    mgr.WriteProficioMkii(mkii: true, log);
                    await mgr.RestartAllAsync(log);
                    break;
                case "keyer":
                    mgr.AppendKeyerStatus(log);
                    break;
                default:
                    Console.Error.WriteLine(
                        "Usage: MSCC-Remote.exe [start|stop|restart|status|legacy|mkii|keyer]");
                    return 1;
            }
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine(ex.Message);
            return 1;
        }

        Console.Write(log.ToString());
        return log.ToString().Contains("ERROR:", StringComparison.Ordinal) ? 1 : 0;
    }
}
