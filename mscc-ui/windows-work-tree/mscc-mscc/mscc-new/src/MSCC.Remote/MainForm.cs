using System.Text;

namespace MSCC.Remote;

public sealed class MainForm : Form
{
    private readonly ServerManager _mgr = new();
    private readonly TextBox _log = new();
    private readonly Label _statusLine = new();
    private bool _busy;

    public MainForm()
    {
        Text = "MSCC Remote — backend servers";
        StartPosition = FormStartPosition.CenterScreen;
        MinimumSize = new Size(420, 480);
        Size = new Size(480, 560);
        Font = new Font("Segoe UI", 9f);
        BackColor = Color.FromArgb(0x1C, 0x1C, 0x1C);
        ForeColor = Color.FromArgb(0xE0, 0xE0, 0xE0);
        try
        {
            string ico = Path.Combine(AppContext.BaseDirectory, "mscc-remote.ico");
            if (File.Exists(ico))
                Icon = new Icon(ico);
        }
        catch { /* optional */ }

        var title = new Label
        {
            Text = "MSCC backend servers (no radio UI)",
            Dock = DockStyle.Top,
            Height = 28,
            TextAlign = ContentAlignment.MiddleLeft,
            Padding = new Padding(10, 6, 10, 0),
            Font = new Font("Segoe UI", 11f, FontStyle.Bold),
            ForeColor = Color.FromArgb(0x00, 0xFF, 0xAA),
        };

        var pathLabel = new Label
        {
            Text = _mgr.ServerRoot,
            Dock = DockStyle.Top,
            Height = 22,
            Padding = new Padding(10, 0, 10, 4),
            ForeColor = Color.FromArgb(0xAA, 0xAA, 0xAA),
            AutoEllipsis = true,
        };

        var buttons = new TableLayoutPanel
        {
            Dock = DockStyle.Top,
            Height = 168,
            ColumnCount = 2,
            RowCount = 4,
            Padding = new Padding(10, 4, 10, 4),
        };
        buttons.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50));
        buttons.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50));
        for (int i = 0; i < 4; i++)
            buttons.RowStyles.Add(new RowStyle(SizeType.Percent, 25));

        AddBtn(buttons, 0, 0, "Start servers", async () => await RunAsync(sb => _mgr.StartAllAsync(sb)));
        AddBtn(buttons, 1, 0, "Stop servers", () => Run(sb => _mgr.StopAll(sb)));
        AddBtn(buttons, 0, 1, "Restart servers", async () => await RunAsync(sb => _mgr.RestartAllAsync(sb)));
        AddBtn(buttons, 1, 1, "Status", () => Run(sb => _mgr.AppendStatus(sb)));
        AddBtn(buttons, 0, 2, "Legacy CW / external keyer", async () => await RunAsync(async sb =>
        {
            _mgr.WriteProficioMkii(mkii: false, sb);
            await _mgr.RestartAllAsync(sb);
        }));
        AddBtn(buttons, 1, 2, "MKII internal keyer", async () => await RunAsync(async sb =>
        {
            _mgr.WriteProficioMkii(mkii: true, sb);
            await _mgr.RestartAllAsync(sb);
        }));
        AddBtn(buttons, 0, 3, "Create desktop shortcut", CreateDesktopShortcut);
        AddBtn(buttons, 1, 3, "Close", () => Close());

        _statusLine.Dock = DockStyle.Top;
        _statusLine.Height = 24;
        _statusLine.Padding = new Padding(10, 2, 10, 2);
        _statusLine.ForeColor = Color.FromArgb(0xFF, 0xCC, 0x00);

        var logLabel = new Label
        {
            Text = "Log",
            Dock = DockStyle.Top,
            Height = 20,
            Padding = new Padding(10, 4, 10, 0),
            ForeColor = Color.FromArgb(0x00, 0xFF, 0xAA),
        };

        _log.Dock = DockStyle.Fill;
        _log.Multiline = true;
        _log.ReadOnly = true;
        _log.ScrollBars = ScrollBars.Vertical;
        _log.Font = new Font("Consolas", 9f);
        _log.BackColor = Color.FromArgb(0x12, 0x12, 0x12);
        _log.ForeColor = Color.FromArgb(0x00, 0xFF, 0xAA);
        _log.BorderStyle = BorderStyle.FixedSingle;
        _log.Margin = new Padding(10);

        var logHost = new Panel { Dock = DockStyle.Fill, Padding = new Padding(10, 0, 10, 10) };
        logHost.Controls.Add(_log);

        Controls.Add(logHost);
        Controls.Add(logLabel);
        Controls.Add(_statusLine);
        Controls.Add(buttons);
        Controls.Add(pathLabel);
        Controls.Add(title);

        Shown += (_, _) => RefreshKeyerLine();
    }

    private void AddBtn(TableLayoutPanel grid, int col, int row, string text, Action action)
    {
        var b = MakeButton(text);
        b.Click += (_, _) =>
        {
            if (_busy) return;
            action();
        };
        grid.Controls.Add(b, col, row);
    }

    private void AddBtn(TableLayoutPanel grid, int col, int row, string text, Func<Task> action)
    {
        var b = MakeButton(text);
        b.Click += async (_, _) =>
        {
            if (_busy) return;
            await action();
        };
        grid.Controls.Add(b, col, row);
    }

    private static Button MakeButton(string text)
    {
        return new Button
        {
            Text = text,
            Dock = DockStyle.Fill,
            Margin = new Padding(4),
            FlatStyle = FlatStyle.Flat,
            BackColor = Color.FromArgb(0x33, 0x33, 0x22),
            ForeColor = Color.FromArgb(0xFF, 0xCC, 0x00),
            FlatAppearance = { BorderColor = Color.FromArgb(0x88, 0x88, 0x44) },
            Cursor = Cursors.Hand,
        };
    }

    private void Run(Action<StringBuilder> work)
    {
        _ = RunAsync(sb =>
        {
            work(sb);
            return Task.CompletedTask;
        });
    }

    private async Task RunAsync(Func<StringBuilder, Task> work)
    {
        if (_busy) return;
        _busy = true;
        UseWaitCursor = true;
        var sb = new StringBuilder();
        try
        {
            await work(sb);
            AppendLog(sb.ToString());
            RefreshKeyerLine();
        }
        catch (Exception ex)
        {
            AppendLog("ERROR: " + ex.Message);
        }
        finally
        {
            _busy = false;
            UseWaitCursor = false;
        }
    }

    private void AppendLog(string text)
    {
        if (string.IsNullOrEmpty(text)) return;
        if (_log.Text.Length > 0)
            _log.AppendText(Environment.NewLine);
        _log.AppendText(text.TrimEnd());
        _log.SelectionStart = _log.TextLength;
        _log.ScrollToCaret();
    }

    private void RefreshKeyerLine()
    {
        var sb = new StringBuilder();
        _mgr.AppendKeyerStatus(sb);
        string s = sb.ToString().Replace(Environment.NewLine, "  ").Trim();
        _statusLine.Text = s.Length > 120 ? s[..120] + "…" : s;
    }

    private void CreateDesktopShortcut()
    {
        try
        {
            string desktop = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
            string lnkPath = Path.Combine(desktop, "MSCC Remote.lnk");
            string target = Application.ExecutablePath;
            string workDir = _mgr.ServerRoot;
            string ico = Path.Combine(AppContext.BaseDirectory, "mscc-remote.ico");
            if (!File.Exists(ico))
                ico = target;

            // WScript.Shell COM — available on all Windows desktops
            Type? t = Type.GetTypeFromProgID("WScript.Shell");
            if (t == null)
            {
                MessageBox.Show("Could not create shortcut (WScript.Shell unavailable).", "MSCC Remote",
                    MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            dynamic shell = Activator.CreateInstance(t)!;
            dynamic shortcut = shell.CreateShortcut(lnkPath);
            shortcut.TargetPath = target;
            shortcut.WorkingDirectory = workDir;
            shortcut.WindowStyle = 1;
            shortcut.Description = "MSCC backend servers — start/stop / legacy keyer";
            shortcut.IconLocation = ico + ",0";
            shortcut.Save();

            AppendLog($"Desktop shortcut created:\n  {lnkPath}\n  → {target}");
            MessageBox.Show($"Shortcut created on Desktop:\nMSCC Remote", "MSCC Remote",
                MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
        catch (Exception ex)
        {
            MessageBox.Show("Shortcut failed: " + ex.Message, "MSCC Remote",
                MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }
}
