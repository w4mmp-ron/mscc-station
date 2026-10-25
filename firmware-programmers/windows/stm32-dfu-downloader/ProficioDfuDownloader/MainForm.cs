using System.Diagnostics;

namespace ProficioDfuDownloader;

public sealed class MainForm : Form
{
    private const string AppAddress = "0x08008000";

    private readonly TextBox _fileBox = new() { ReadOnly = true, Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top };
    private readonly Button _browseBtn = new() { Text = "Browse…", AutoSize = true };
    private readonly Button _listBtn = new() { Text = "List DFU devices", AutoSize = true };
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
               "1) Hold KEY (PA0) + NRST, release KEY → DFU   2) Browse to app .bin   3) Burn",
    };

    public MainForm()
    {
        Text = "Proficio DFU Downloader";
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
        _listBtn.Click += async (_, _) => await RunDfuAsync("-l");
        _burnBtn.Click += async (_, _) => await BurnAsync();
    }

    private static string AppDir => AppContext.BaseDirectory;

    private static string DfuUtilPath => Path.Combine(AppDir, "dfu-util.exe");

    private void Browse()
    {
        using var dlg = new OpenFileDialog
        {
            Filter = "Application firmware (*.bin)|*.bin|All files (*.*)|*.*",
            Title = "Select application .bin",
        };
        if (dlg.ShowDialog(this) == DialogResult.OK)
            _fileBox.Text = dlg.FileName;
    }

    private async Task BurnAsync()
    {
        var file = _fileBox.Text.Trim();
        if (string.IsNullOrEmpty(file) || !File.Exists(file))
        {
            MessageBox.Show(this, "Choose an application .bin first.", Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }

        var args = $"-d 0483:df11 -a 0 -s {AppAddress}:leave -D \"{file}\"";
        AppendLog($"> dfu-util {args}\r\n");
        var code = await RunDfuAsync(args);
        if (code == 0)
            AppendLog("\r\nDone. Reset without KEY to run the app.\r\n");
        else
            AppendLog($"\r\ndfu-util exited with code {code}. Is the pill in DFU mode?\r\n");
    }

    private async Task<int> RunDfuAsync(string args)
    {
        if (!File.Exists(DfuUtilPath))
        {
            AppendLog($"ERROR: dfu-util.exe not found next to the app:\r\n  {DfuUtilPath}\r\n");
            return -1;
        }

        _listBtn.Enabled = false;
        _burnBtn.Enabled = false;
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
            proc.OutputDataReceived += (_, e) =>
            {
                if (e.Data is null) return;
                BeginInvoke(() => AppendLog(e.Data + "\r\n"));
            };
            proc.ErrorDataReceived += (_, e) =>
            {
                if (e.Data is null) return;
                BeginInvoke(() => AppendLog(e.Data + "\r\n"));
            };

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
        finally
        {
            _listBtn.Enabled = true;
            _burnBtn.Enabled = true;
        }
    }

    private void AppendLog(string text)
    {
        _log.AppendText(text);
    }
}
