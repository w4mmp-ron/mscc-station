using System.Diagnostics;

namespace ProficioDfuDownloader;

public sealed class MainForm : Form
{
    private const string AppAddress = "0x08008000";

    private readonly TextBox _fileBox = new() { ReadOnly = true, Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top };
    private readonly Button _browseBtn = new() { Text = "Browse…", AutoSize = true };
    private readonly Button _listBtn = new() { Text = "Check Radio", AutoSize = true };
    private readonly Button _burnBtn = new() { Text = "Burn application", AutoSize = true };
    private readonly TextBox _log = new()
    {
        Multiline = true,
        ReadOnly = true,
        ScrollBars = ScrollBars.Both,
        Font = new Font(FontFamily.GenericMonospace, 9f),
        Dock = DockStyle.Fill,
        WordWrap = false,
    };

    private readonly Label _hint = new()
    {
        AutoSize = false,
        Height = 48,
        Dock = DockStyle.Top,
        Text = "Application update only (@ 0x08008000). Bootloader is not flashed here (use ST-Link / Cube).\r\n" +
               "1) Browse to the app .bin   2) Burn. The pill goes to flash mode by itself and restarts when done.",
    };

    public MainForm()
    {
        Text = "STM32 App Flash (Black Pill, USB DFU)";
        Width = 720;
        Height = 520;
        MinimumSize = new Size(560, 400);
        StartPosition = FormStartPosition.CenterScreen;

        var top = new TableLayoutPanel
        {
            Dock = DockStyle.Top,
            ColumnCount = 2,
            RowCount = 2,
            AutoSize = true,
            Padding = new Padding(8),
        };
        top.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        top.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));

        top.Controls.Add(_fileBox, 0, 0);
        top.Controls.Add(_browseBtn, 1, 0);

        var buttons = new FlowLayoutPanel { AutoSize = true, Dock = DockStyle.Fill };
        buttons.Controls.Add(_listBtn);
        buttons.Controls.Add(_burnBtn);
        top.SetColumnSpan(buttons, 2);
        top.Controls.Add(buttons, 0, 1);

        var host = new Panel { Dock = DockStyle.Fill, Padding = new Padding(8) };
        host.Controls.Add(_log);

        Controls.Add(host);
        Controls.Add(top);
        Controls.Add(_hint);

        _browseBtn.Click += (_, _) => Browse();
        _listBtn.Click += async (_, _) => await CheckRadioAsync();
        _burnBtn.Click += async (_, _) => await BurnAsync();
    }

    // MSCC servers (Windows image names without .exe): ms-sdr-MKII, mscc-recv, Mscc-trans,
    // or older sdrcore-*. No firmware update while any runs (Ron 2026-11-12).
    private static readonly string[] ServerPrefixes = { "ms-sdr", "sdrcore", "mscc-recv", "mscc-trans" };

    public static List<string> ServersRunning()
    {
        var found = new List<string>();
        foreach (var p in Process.GetProcesses())
        {
            string name;
            try { name = p.ProcessName; } catch { continue; }
            finally { p.Dispose(); }
            var low = name.ToLowerInvariant();
            if (ServerPrefixes.Any(low.StartsWith) && !found.Contains(name))
                found.Add(name);
        }
        return found;
    }

    public static string ServersMessage(List<string> names) =>
        $"The MSCC servers are running ({string.Join(", ", names)}). Close them, then run the Black Pill update again.";

    // Servers running: tell the user and close the window.
    private bool ServersOk()
    {
        var running = ServersRunning();
        if (running.Count == 0)
            return true;
        MessageBox.Show(this, ServersMessage(running), Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
        Close();
        return false;
    }

    private static string AppDir => AppContext.BaseDirectory;

    private static string DfuUtilPath => Path.Combine(AppDir, "dfu-util.exe");

    private void Browse()
    {
        using var dlg = new OpenFileDialog
        {
            // only <version>-app.bin; hides -boot-app.bin and bootload-PA8.bin (ST-Link)
            Filter = "Black Pill app (BlackPill-25MHz-<version>-app.bin)|BlackPill-25MHz-?.?-app.bin;BlackPill-25MHz-?.??-app.bin;BlackPill-25MHz-?.???-app.bin|All files (*.*)|*.*",
            Title = "Select application .bin",
        };
        if (dlg.ShowDialog(this) == DialogResult.OK)
            _fileBox.Text = dlg.FileName;
    }

    private const string NoPillText =
        "No radio and no Black Pill in flash mode found. Is the rig powered and the USB cable in?\r\n" +
        "If it still is not found, fit the BOOT jumper and press NRST (LED B), then try again.";

    private void SetBusy(bool busy)
    {
        _listBtn.Enabled = !busy;
        _burnBtn.Enabled = !busy;
        _browseBtn.Enabled = !busy;
    }

    // Pill in flash mode (0483:df11)? Runs dfu-util -l without logging it.
    private async Task<bool> DfuPresentAsync()
    {
        await RunDfuAsync("-l", quiet: true);
        return OutputHas("0483:df11");
    }

    private static Task<string?> RadioVersionAsync() => Task.Run(RadioUsb.Version);

    private async Task CheckRadioAsync()
    {
        if (!ServersOk())
            return;
        SetBusy(true);
        try
        {
            var ver = await RadioVersionAsync();
            if (ver is not null)
                AppendLog($"Radio running firmware {ver}. Ready to Burn.\r\n");
            else if (await DfuPresentAsync())
                AppendLog("Black Pill in flash mode (LED B). Ready to Burn.\r\n");
            else
                AppendLog(NoPillText + "\r\n");
        }
        finally { SetBusy(false); }
    }

    private async Task BurnAsync()
    {
        var file = _fileBox.Text.Trim();
        if (string.IsNullOrEmpty(file) || !File.Exists(file))
        {
            MessageBox.Show(this, "Choose an application .bin first.", Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }
        if (Path.GetFileName(file).EndsWith("-boot-app.bin", StringComparison.OrdinalIgnoreCase)
            || Path.GetFileName(file).StartsWith("bootload", StringComparison.OrdinalIgnoreCase))
        {
            MessageBox.Show(this, $"{Path.GetFileName(file)} holds the bootloader (ST-Link only). Pick the BlackPill-25MHz-<version>-app.bin file.",
                Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
            return;
        }
        if (!ServersOk())
            return;

        SetBusy(true);
        try { await BurnStepsAsync(file); }
        finally { SetBusy(false); }
    }

    // Same steps as the Pi stm32-app-flash script.
    private async Task BurnStepsAsync(string file)
    {
        bool jumperSuspect = false;
        if (await DfuPresentAsync())
        {
            AppendLog("Pill already in flash mode (BOOT jumper on?). Flashing anyway.\r\n");
            jumperSuspect = true;
        }
        else
        {
            var ver = await RadioVersionAsync();
            if (ver is null)
            {
                AppendLog(NoPillText + "\r\n");
                return;
            }
            AppendLog($"Radio running firmware {ver}. Sending it to flash mode...\r\n");
            await Task.Run(RadioUsb.SendFlashMode);
            bool inDfu = false;
            for (int i = 0; i < 20 && !inDfu; i++)
            {
                await Task.Delay(500);
                inDfu = await DfuPresentAsync();
            }
            if (!inDfu)
            {
                AppendLog("ERROR: the pill did not go to flash mode in 10 s.\r\n" +
                          "Fit the BOOT jumper and press NRST (LED B), then Burn again.\r\n");
                return;
            }
        }

        var args = $"-d 0483:df11 -a 0 -s {AppAddress}:leave -D \"{file}\"";
        AppendLog($"> dfu-util {args}\r\n");
        await RunDfuAsync(args);
        // dfu-util often ends with "Error during download get_status" after :leave because
        // the pill resets at once; that is normal, so judge by this line, not the exit code.
        if (!OutputHas("File downloaded successfully"))
        {
            AppendLog("\r\nERROR: flash FAILED (see above).\r\n");
            return;
        }

        AppendLog("\r\nFlashed OK (the get_status error above is normal). Waiting for the radio to start...\r\n");
        string? now = null;
        for (int i = 0; i < 20 && now is null; i++)
        {
            await Task.Delay(500);
            now = await RadioVersionAsync();
        }
        if (now is not null)
            AppendLog($"Done: radio running firmware {now}.\r\n");
        else if (await DfuPresentAsync())
            AppendLog("Flashed OK, but the pill is still in flash mode (LED B):\r\n" +
                      "remove the BOOT jumper and press NRST. No power cycle needed." +
                      (jumperSuspect ? "" : " (Or KEY is held / stuck.)") + "\r\n");
        else
            AppendLog("Radio not seen on USB after 10 s. Check the LED letter.\r\n");
    }

    private bool _quiet;

    private async Task<int> RunDfuAsync(string args, bool quiet = false)
    {
        if (!File.Exists(DfuUtilPath))
        {
            AppendLog($"ERROR: dfu-util.exe not found next to the app:\r\n  {DfuUtilPath}\r\n");
            return -1;
        }

        _quiet = quiet;
        lock (_lastOutput) _lastOutput.Clear();
        try
        {
            var psi = new ProcessStartInfo
            {
                FileName = DfuUtilPath,
                Arguments = args,
                WorkingDirectory = AppDir,
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                CreateNoWindow = true,
            };

            using var proc = new Process { StartInfo = psi, EnableRaisingEvents = true };
            proc.OutputDataReceived += (_, e) => OnDfuLine(e.Data);
            proc.ErrorDataReceived += (_, e) => OnDfuLine(e.Data);

            proc.Start();
            proc.BeginOutputReadLine();
            proc.BeginErrorReadLine();
            await proc.WaitForExitAsync();
            return proc.ExitCode;
        }
        catch (Exception ex)
        {
            AppendLog("ERROR: " + ex.Message + "\r\n");
            return -1;
        }
    }

    // dfu-util output of the current run (for the found / not found check)
    private readonly List<string> _lastOutput = new();

    private bool OutputHas(string text)
    {
        lock (_lastOutput) return _lastOutput.Any(l => l.Contains(text));
    }

    // dfu-util prints a copyright banner on every run: keep it (and blank lines) out of the log
    private static readonly string[] BannerStarts =
        { "dfu-util 0.", "Copyright ", "This program is Free Software", "Please report bugs" };

    private void OnDfuLine(string? line)
    {
        if (line is null) return;
        lock (_lastOutput) _lastOutput.Add(line);
        if (_quiet || line.Trim().Length == 0 || BannerStarts.Any(line.StartsWith)) return;
        BeginInvoke(() => AppendLog(line + "\r\n"));
    }

    private void AppendLog(string text)
    {
        _log.AppendText(text);
    }
}
