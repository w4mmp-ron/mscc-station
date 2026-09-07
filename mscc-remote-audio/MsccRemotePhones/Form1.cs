using MsccRemotePhones.Audio;
using MsccRemotePhones.Protocol;

namespace MsccRemotePhones;

public partial class Form1 : Form
{
    private readonly JitterBuffer _jitter = new();
    private readonly UdpAudioReceiver _receiver;
    private readonly RemotePhonePlayer _player;
    private readonly RemoteMicSender _mic;
    private readonly System.Windows.Forms.Timer _statsTimer = new() { Interval = 500 };

    private NumericUpDown _numPort = null!;
    private NumericUpDown _numJitter = null!;
    private ComboBox _cmbDevice = null!;
    private TrackBar _trkVolume = null!;
    private Label _lblVolume = null!;
    private CheckBox _chkMute = null!;
    private CheckBox _chkEqEnable = null!;
    private TrackBar _trkEqLow = null!;
    private TrackBar _trkEqMid = null!;
    private TrackBar _trkEqHigh = null!;
    private Label _lblEqLow = null!;
    private Label _lblEqMid = null!;
    private Label _lblEqHigh = null!;
    private Button _btnEqReset = null!;
    private TextBox _txtTxHost = null!;
    private NumericUpDown _numTxPort = null!;
    private ComboBox _cmbMic = null!;
    private TrackBar _trkMicVolume = null!;
    private Label _lblMicVolume = null!;
    private Button _btnStart = null!;
    private Button _btnStop = null!;
    private Button _btnMicStart = null!;
    private Button _btnMicStop = null!;
    private CheckBox _chkCompression = null!;
    private TrackBar _trkCompression = null!;
    private Label _lblCompression = null!;
    private Label _lblStatus = null!;
    private TextBox _txtLog = null!;
    private Label _lblStats = null!;
    private int _msSdrPort = MsccControlClient.DefaultMsSdrPort;

    private int _lastRate = MsccAudioProtocol.DefaultSampleRate;
    private int _lastCh = MsccAudioProtocol.DefaultChannels;
    private int _playDeviceIndex = int.MinValue; // device used for current player
    private bool _playerArmed;
    /// <summary>True only after INI has been applied — blocks saves during construction.</summary>
    private bool _settingsReady;
    private int _cardsContentHeight;
    private bool _suppressSave;
    private readonly System.Windows.Forms.Timer _cmpLevelDebounce = new() { Interval = 250 };
    private int _pendingCmpLevel = -1;
    private int _cmpSendGeneration;
    private int _lastCmpLevelSent = int.MinValue;

    public Form1()
    {
        _receiver = new UdpAudioReceiver(_jitter);
        _player = new RemotePhonePlayer(_jitter);
        _mic = new RemoteMicSender();
        _receiver.Log += AppendLog;
        _player.Log += AppendLog;
        _mic.Log += AppendLog;
        _receiver.PacketAccepted += OnPacket;

        // Load INI first so init-time ValueChanged handlers cannot overwrite it
        // with control defaults before ApplySettingsToUi runs.
        var saved = AppSettingsStore.Load();

        _suppressSave = true;
        InitializeUi();
        LoadDevices();
        LoadMicDevices();
        ApplySettingsToUi(saved);
        _suppressSave = false;
        _settingsReady = true;

        _statsTimer.Tick += (_, _) => UpdateStats();
        _statsTimer.Start();
        _cmpLevelDebounce.Tick += (_, _) =>
        {
            _cmpLevelDebounce.Stop();
            CommitCompressionLevel();
        };
        AppendLog($"Settings loaded from {AppSettingsStore.ConfigPath}");

        /* Delay: MSCC StopAll→StartOrShow races the prior UDP port release. */
        if (saved.AutoStartRx || saved.AutoStartMic)
        {
            var t = new System.Windows.Forms.Timer { Interval = 400 };
            t.Tick += (_, _) =>
            {
                t.Stop();
                t.Dispose();
                if (saved.AutoStartRx)
                    StartRx(retryBind: true);
                if (saved.AutoStartMic)
                    StartMic(saveSticky: true);
            };
            t.Start();
        }
    }

    private static readonly Color UiBg = Color.FromArgb(236, 240, 245);
    private static readonly Color UiCard = Color.White;
    private static readonly Color UiLabel = Color.FromArgb(71, 85, 105);
    private static readonly Color UiSpeaker = Color.FromArgb(37, 99, 235);
    private static readonly Color UiEq = Color.FromArgb(124, 58, 237);
    private static readonly Color UiMic = Color.FromArgb(5, 150, 105);
    private static readonly Color UiStatusBg = Color.FromArgb(226, 232, 240);

    private void InitializeUi()
    {
        Text = "MSCC Remote Phones";
        Width = 620;
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = UiBg;
        Font = new Font("Segoe UI", 9f);
        FormClosing += (_, _) =>
        {
            _settingsReady = true;
            _suppressSave = false;
            SaveSettingsFromUi();
            _statsTimer.Stop();
            _cmpLevelDebounce.Stop();
            _mic.Stop();
            _player.Stop();
            _receiver.Stop();
        };

        var root = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 1,
            RowCount = 4,
            Padding = new Padding(10),
            BackColor = UiBg,
        };
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        root.RowStyles.Add(new RowStyle(SizeType.AutoSize)); // speaker
        root.RowStyles.Add(new RowStyle(SizeType.AutoSize)); // eq
        root.RowStyles.Add(new RowStyle(SizeType.AutoSize)); // mic
        root.RowStyles.Add(new RowStyle(SizeType.Absolute, 0)); // log (shown only when user grows the window)

        // --- SPEAKER card ---
        var speakerBody = MakeCardGrid(6);
        int r = 0;
        AddField(speakerBody, r++, "UDP listen port", _numPort = new NumericUpDown
        {
            Minimum = 1024,
            Maximum = 65535,
            Value = MsccAudioProtocol.DefaultPort,
            Width = 100,
        });
        _numPort.ValueChanged += (_, _) => SaveSettingsFromUi();

        AddField(speakerBody, r++, "Jitter target (ms)", _numJitter = new NumericUpDown
        {
            Minimum = 20,
            Maximum = 300,
            Value = 80,
            Width = 100,
        });
        _numJitter.ValueChanged += (_, _) => SaveSettingsFromUi();

        AddField(speakerBody, r++, "Play device", _cmbDevice = new ComboBox
        {
            DropDownStyle = ComboBoxStyle.DropDownList,
            Dock = DockStyle.Fill,
        });
        _cmbDevice.SelectedIndexChanged += (_, _) => SaveSettingsFromUi();

        AddField(speakerBody, r++, "Phones volume", MakeVolumeRow(out _trkVolume, out _lblVolume, 80, v =>
        {
            _player.Volume = v / 100f;
            _lblVolume.Text = $"{v}%";
            SaveSettingsFromUi();
        }));
        _player.Volume = 0.8f;

        _chkMute = new CheckBox { Text = "Mute phones", AutoSize = true, ForeColor = UiLabel };
        _chkMute.CheckedChanged += (_, _) =>
        {
            _player.Muted = _chkMute.Checked;
            AppendLog(_chkMute.Checked ? "Phones muted" : "Phones unmuted");
            SaveSettingsFromUi();
        };
        speakerBody.Controls.Add(MakeFieldLabel(""), 0, r);
        speakerBody.Controls.Add(_chkMute, 1, r++);
        root.Controls.Add(MakeSectionCard("SPEAKER", UiSpeaker, speakerBody), 0, 0);

        // --- EQ card ---
        var eqBody = MakeCardGrid(5);
        r = 0;
        _chkEqEnable = new CheckBox { Text = "Enable EQ", AutoSize = true, ForeColor = UiLabel };
        _chkEqEnable.CheckedChanged += (_, _) =>
        {
            ApplyEqFromUi();
            SaveSettingsFromUi();
        };
        eqBody.Controls.Add(MakeFieldLabel(""), 0, r);
        eqBody.Controls.Add(_chkEqEnable, 1, r++);

        var eqBands = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 3,
            AutoSize = true,
            Margin = new Padding(0),
            Padding = new Padding(0),
            BackColor = UiCard,
        };
        eqBands.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 48));
        eqBands.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        for (int i = 0; i < 3; i++)
            eqBands.RowStyles.Add(new RowStyle(SizeType.Absolute, 32));
        eqBands.Controls.Add(MakeFieldLabel("Low"), 0, 0);
        eqBands.Controls.Add(MakeEqRow(out _trkEqLow, out _lblEqLow), 1, 0);
        eqBands.Controls.Add(MakeFieldLabel("Mid"), 0, 1);
        eqBands.Controls.Add(MakeEqRow(out _trkEqMid, out _lblEqMid), 1, 1);
        eqBands.Controls.Add(MakeFieldLabel("High"), 0, 2);
        eqBands.Controls.Add(MakeEqRow(out _trkEqHigh, out _lblEqHigh), 1, 2);
        eqBody.SetColumnSpan(eqBands, 2);
        eqBody.Controls.Add(eqBands, 0, r++);

        var eqRxButtons = new FlowLayoutPanel
        {
            Dock = DockStyle.Fill,
            AutoSize = true,
            WrapContents = false,
            Margin = new Padding(0),
            BackColor = UiCard,
        };
        _btnEqReset = StyleButton("Reset EQ", secondary: true);
        _btnEqReset.Click += (_, _) =>
        {
            _suppressSave = true;
            _chkEqEnable.Checked = false;
            SetEqSlider(_trkEqLow, _lblEqLow, 0);
            SetEqSlider(_trkEqMid, _lblEqMid, 0);
            SetEqSlider(_trkEqHigh, _lblEqHigh, 0);
            _suppressSave = false;
            ApplyEqFromUi();
            SaveSettingsFromUi();
        };
        _btnStart = StyleButton("Start RX", secondary: false);
        _btnStop = StyleButton("Stop RX", secondary: true);
        _btnStop.Enabled = false;
        _btnStart.Click += (_, _) => StartRx(retryBind: true);
        _btnStop.Click += (_, _) => StopRx(clearAutoStart: true);
        eqRxButtons.Controls.Add(_btnEqReset);
        eqRxButtons.Controls.Add(_btnStart);
        eqRxButtons.Controls.Add(_btnStop);
        eqBody.Controls.Add(MakeFieldLabel(""), 0, r);
        eqBody.Controls.Add(eqRxButtons, 1, r++);
        root.Controls.Add(MakeSectionCard("EQ", UiEq, eqBody), 0, 1);

        // --- MICROPHONE card ---
        var micBody = MakeCardGrid(7);
        r = 0;
        AddField(micBody, r++, "TX host", _txtTxHost = new TextBox
        {
            Text = "127.0.0.1",
            Dock = DockStyle.Fill,
        });
        _txtTxHost.Leave += (_, _) => SaveSettingsFromUi();

        AddField(micBody, r++, "TX port", _numTxPort = new NumericUpDown
        {
            Minimum = 1024,
            Maximum = 65535,
            Value = MsccAudioProtocol.DefaultTxPort,
            Width = 100,
        });
        _numTxPort.ValueChanged += (_, _) => SaveSettingsFromUi();

        AddField(micBody, r++, "Device", _cmbMic = new ComboBox
        {
            DropDownStyle = ComboBoxStyle.DropDownList,
            Dock = DockStyle.Fill,
        });
        _cmbMic.SelectedIndexChanged += (_, _) => SaveSettingsFromUi();

        AddField(micBody, r++, "Mic volume", MakeVolumeRow(out _trkMicVolume, out _lblMicVolume, 80, v =>
        {
            _mic.Volume = v / 100f;
            _lblMicVolume.Text = $"{v}%";
            SaveSettingsFromUi();
        }));
        _mic.Volume = 0.8f;

        _chkCompression = new CheckBox { Text = "Compress (CMP)", AutoSize = true, ForeColor = UiLabel };
        _chkCompression.CheckedChanged += (_, _) =>
        {
            if (!_suppressSave)
            {
                SendCompressionState(_chkCompression.Checked);
                SaveSettingsFromUi();
            }
        };
        micBody.Controls.Add(MakeFieldLabel("CMP"), 0, r);
        micBody.Controls.Add(_chkCompression, 1, r++);

        AddField(micBody, r++, "CMP level", MakeCompressionRow());

        var txButtons = new FlowLayoutPanel
        {
            Dock = DockStyle.Fill,
            AutoSize = true,
            WrapContents = false,
            BackColor = UiCard,
        };
        _btnMicStart = StyleButton("Start Mic TX", secondary: false, accent: UiMic);
        _btnMicStop = StyleButton("Stop Mic TX", secondary: true);
        _btnMicStop.Enabled = false;
        _btnMicStart.Click += (_, _) => StartMic(saveSticky: true);
        _btnMicStop.Click += (_, _) => StopMic(clearAutoStart: true);
        txButtons.Controls.Add(_btnMicStart);
        txButtons.Controls.Add(_btnMicStop);
        micBody.Controls.Add(MakeFieldLabel(""), 0, r);
        micBody.Controls.Add(txButtons, 1, r++);
        root.Controls.Add(MakeSectionCard("MICROPHONE", UiMic, micBody), 0, 2);

        /* Status kept for RefreshStatus/UpdateStats but not shown under MIC buttons. */
        _lblStatus = new Label { Text = "RX: Stopped.  TX: Stopped.", Visible = false };
        _lblStats = new Label { Text = "—", Visible = false };

        _txtLog = new TextBox
        {
            Multiline = true,
            ReadOnly = true,
            ScrollBars = ScrollBars.Vertical,
            Dock = DockStyle.Fill,
            Font = new Font("Consolas", 9f),
            BackColor = Color.FromArgb(248, 250, 252),
            ForeColor = Color.FromArgb(30, 41, 59),
            BorderStyle = BorderStyle.FixedSingle,
        };
        root.Controls.Add(_txtLog, 0, 3);

        Controls.Add(root);

        /* Size window to the three cards only — no peek of text under MIC buttons. */
        root.PerformLayout();
        int chrome = Height - ClientSize.Height;
        _cardsContentHeight = root.Padding.Vertical + root.GetRowHeights().Take(3).Sum();
        Height = _cardsContentHeight + chrome;
        MinimumSize = new Size(520, Height);
        Width = 620;

        Resize += (_, _) =>
        {
            if (root.RowStyles.Count < 4)
                return;
            int extra = ClientSize.Height - _cardsContentHeight;
            root.RowStyles[3] = extra > 24
                ? new RowStyle(SizeType.Percent, 100f)
                : new RowStyle(SizeType.Absolute, 0f);
        };
    }

    private static TableLayoutPanel MakeCardGrid(int rows)
    {
        var grid = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = rows,
            AutoSize = true,
            BackColor = UiCard,
            Margin = new Padding(0),
            Padding = new Padding(0),
        };
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 120));
        grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        for (int i = 0; i < rows; i++)
            grid.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        return grid;
    }

    private static void AddField(TableLayoutPanel grid, int row, string label, Control control)
    {
        grid.Controls.Add(MakeFieldLabel(label), 0, row);
        grid.Controls.Add(control, 1, row);
    }

    private static Label MakeFieldLabel(string text) => new()
    {
        Text = text,
        AutoSize = true,
        Anchor = AnchorStyles.Left,
        ForeColor = UiLabel,
        Margin = new Padding(0, 6, 8, 0),
    };

    private static Panel MakeSectionCard(string title, Color accent, Control body)
    {
        var card = new Panel
        {
            Dock = DockStyle.Fill,
            AutoSize = true,
            BackColor = UiCard,
            Margin = new Padding(0, 0, 0, 10),
            Padding = new Padding(0),
        };
        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 1,
            RowCount = 2,
            AutoSize = true,
            BackColor = UiCard,
            Margin = new Padding(0),
            Padding = new Padding(0),
        };
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 32));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));

        var header = new Label
        {
            Text = "  " + title,
            Dock = DockStyle.Fill,
            BackColor = accent,
            ForeColor = Color.White,
            Font = new Font("Segoe UI Semibold", 10f, FontStyle.Bold),
            TextAlign = ContentAlignment.MiddleLeft,
        };
        var bodyHost = new Panel
        {
            Dock = DockStyle.Fill,
            AutoSize = true,
            BackColor = UiCard,
            Padding = new Padding(12, 10, 12, 10),
        };
        body.Dock = DockStyle.Fill;
        bodyHost.Controls.Add(body);
        layout.Controls.Add(header, 0, 0);
        layout.Controls.Add(bodyHost, 0, 1);
        card.Controls.Add(layout);

        /* Light border without custom paint. */
        card.Paint += (_, e) =>
        {
            var rc = card.ClientRectangle;
            rc.Width -= 1;
            rc.Height -= 1;
            using var pen = new Pen(Color.FromArgb(203, 213, 225));
            e.Graphics.DrawRectangle(pen, rc);
        };
        return card;
    }

    private static Button StyleButton(string text, bool secondary, Color? accent = null)
    {
        var bg = secondary ? Color.FromArgb(241, 245, 249) : (accent ?? UiSpeaker);
        var fg = secondary ? Color.FromArgb(51, 65, 85) : Color.White;
        return new Button
        {
            Text = text,
            AutoSize = true,
            MinimumSize = new Size(96, 30),
            Margin = new Padding(0, 0, 8, 0),
            FlatStyle = FlatStyle.Flat,
            BackColor = bg,
            ForeColor = fg,
            Font = new Font("Segoe UI Semibold", 9f),
            Cursor = Cursors.Hand,
            FlatAppearance =
            {
                BorderSize = secondary ? 1 : 0,
                BorderColor = Color.FromArgb(203, 213, 225),
            },
        };
    }

    /// <summary>CMP level 0..24 — same range as MSCC.</summary>
    private Control MakeCompressionRow()
    {
        var row = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 1,
            AutoSize = true,
            BackColor = UiCard,
            Margin = new Padding(0),
        };
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 40));
        _trkCompression = new TrackBar
        {
            Minimum = MsccControlClient.CompressionLevelMin,
            Maximum = MsccControlClient.CompressionLevelMax,
            Value = 12,
            SmallChange = 1,
            LargeChange = 1,
            TickFrequency = 1,
            TickStyle = TickStyle.None,
            Dock = DockStyle.Fill,
            Height = 28,
            Margin = new Padding(0),
            AutoSize = false,
        };
        _lblCompression = new Label
        {
            Text = "12",
            AutoSize = false,
            Dock = DockStyle.Fill,
            TextAlign = ContentAlignment.MiddleLeft,
            ForeColor = UiLabel,
            Margin = new Padding(4, 0, 0, 0),
        };
        /* Wheel: force ±1 (default TrackBar wheel jumps by LargeChange * system delta). */
        _trkCompression.MouseWheel += (_, e) =>
        {
            if (e is HandledMouseEventArgs he)
                he.Handled = true;
            int step = e.Delta > 0 ? 1 : -1;
            int next = Math.Clamp(
                _trkCompression.Value + step,
                _trkCompression.Minimum,
                _trkCompression.Maximum);
            if (next != _trkCompression.Value)
                _trkCompression.Value = next;
        };
        /* Label updates live; one UDP 0xEF after settle (debounce only — not MouseUp+timer). */
        _trkCompression.ValueChanged += (_, _) =>
        {
            _lblCompression.Text = _trkCompression.Value.ToString();
            if (_suppressSave || !_settingsReady)
                return;
            _pendingCmpLevel = _trkCompression.Value;
            _cmpSendGeneration++;
            _cmpLevelDebounce.Stop();
            _cmpLevelDebounce.Start();
        };
        row.Controls.Add(_trkCompression, 0, 0);
        row.Controls.Add(_lblCompression, 1, 0);
        return row;
    }

    private void CommitCompressionLevel()
    {
        if (_suppressSave || !_settingsReady)
            return;
        _cmpLevelDebounce.Stop();
        int level = _pendingCmpLevel >= 0 ? _pendingCmpLevel : _trkCompression.Value;
        _pendingCmpLevel = -1;
        if (level == _lastCmpLevelSent)
            return;
        _lastCmpLevelSent = level;
        _cmpSendGeneration++;
        SendCompressionLevel(level);
        SaveSettingsFromUi();
    }

    private void SendCompressionState(bool on)
    {
        string host = _txtTxHost.Text.Trim();
        int port = _msSdrPort;
        MsccControlClient.SetCompressionState(
            host,
            port,
            on,
            onSent: () => AppendLog($"CMP state → {(on ? "ON" : "OFF")}  ({host}:{port} 0xEE)"),
            onError: ex => AppendLog("CMP state failed: " + ex.Message));
    }

    private void SendCompressionLevel(int level)
    {
        string host = _txtTxHost.Text.Trim();
        int port = _msSdrPort;
        MsccControlClient.SetCompressionLevel(
            host,
            port,
            level,
            onSent: () => AppendLog($"CMP level → {level}  ({host}:{port} 0xEF)"),
            onError: ex => AppendLog("CMP level failed: " + ex.Message));
    }

    private static Control MakeVolumeRow(out TrackBar track, out Label pct, int initial, Action<int> onChange)
    {
        var volRow = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 1,
            AutoSize = true,
            BackColor = UiCard,
            Margin = new Padding(0),
        };
        volRow.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        volRow.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 48));
        track = new TrackBar
        {
            Minimum = 0,
            Maximum = 100,
            Value = initial,
            TickFrequency = 10,
            Dock = DockStyle.Fill,
            Height = 36,
        };
        pct = new Label
        {
            Text = $"{initial}%",
            AutoSize = true,
            Anchor = AnchorStyles.Left,
            TextAlign = ContentAlignment.MiddleLeft,
        };
        var tb = track;
        tb.ValueChanged += (_, _) => onChange(tb.Value);
        volRow.Controls.Add(tb, 0, 0);
        volRow.Controls.Add(pct, 1, 0);
        return volRow;
    }

    /// <summary>EQ slider: -12..+12 dB mapped as track 0..240 (center 120 = 0 dB).</summary>
    private Control MakeEqRow(out TrackBar track, out Label dbLabel)
    {
        var row = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 1,
            AutoSize = true,
            Margin = new Padding(0),
            Padding = new Padding(0),
            BackColor = UiCard,
        };
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 58));
        row.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        track = new TrackBar
        {
            Minimum = 0,
            Maximum = 240,
            Value = 120,
            TickFrequency = 20,
            TickStyle = TickStyle.None,
            Dock = DockStyle.Fill,
            Height = 28,
            Margin = new Padding(0),
            AutoSize = false,
        };
        dbLabel = new Label
        {
            Text = "0.0 dB",
            AutoSize = false,
            Dock = DockStyle.Fill,
            TextAlign = ContentAlignment.MiddleLeft,
            Margin = new Padding(4, 0, 0, 0),
        };
        var tb = track;
        var lbl = dbLabel;
        tb.ValueChanged += (_, _) =>
        {
            float db = TrackToDb(tb.Value);
            lbl.Text = $"{db:+0.0;-0.0;0.0} dB";
            if (!_suppressSave)
            {
                ApplyEqFromUi();
                SaveSettingsFromUi();
            }
        };
        row.Controls.Add(tb, 0, 0);
        row.Controls.Add(dbLabel, 1, 0);
        return row;
    }

    private static float TrackToDb(int trackValue) => (trackValue - 120) / 10f;
    private static int DbToTrack(float db) => Math.Clamp((int)Math.Round(db * 10f) + 120, 0, 240);

    private static void SetEqSlider(TrackBar track, Label lbl, float db)
    {
        track.Value = DbToTrack(db);
        lbl.Text = $"{db:+0.0;-0.0;0.0} dB";
    }

    private void ApplySettingsToUi(AppSettings s)
    {
        _suppressSave = true;
        try
        {
            _numPort.Value = Math.Clamp(s.RxPort, (int)_numPort.Minimum, (int)_numPort.Maximum);
            _numJitter.Value = Math.Clamp(s.JitterMs, (int)_numJitter.Minimum, (int)_numJitter.Maximum);
            SelectComboDevice(_cmbDevice, s.PlayDevice, index: null);
            _trkVolume.Value = Math.Clamp(s.VolumePct, 0, 100);
            _lblVolume.Text = $"{_trkVolume.Value}%";
            _player.Volume = _trkVolume.Value / 100f;
            _chkMute.Checked = s.Mute;
            _player.Muted = s.Mute;
            _chkEqEnable.Checked = s.EqEnabled;
            SetEqSlider(_trkEqLow, _lblEqLow, s.EqLowDb);
            SetEqSlider(_trkEqMid, _lblEqMid, s.EqMidDb);
            SetEqSlider(_trkEqHigh, _lblEqHigh, s.EqHighDb);
            _txtTxHost.Text = string.IsNullOrWhiteSpace(s.TxHost) ? "127.0.0.1" : s.TxHost;
            _numTxPort.Value = Math.Clamp(s.TxPort, (int)_numTxPort.Minimum, (int)_numTxPort.Maximum);
            SelectComboDevice(_cmbMic, s.MicDevice, s.MicDeviceIndex);
            _trkMicVolume.Value = Math.Clamp(s.MicVolumePct, 0, 100);
            _lblMicVolume.Text = $"{_trkMicVolume.Value}%";
            _mic.Volume = _trkMicVolume.Value / 100f;
            _msSdrPort = Math.Clamp(s.MsSdrPort, 1024, 65535);
            _chkCompression.Checked = s.CompressionOn;
            _trkCompression.Value = Math.Clamp(
                s.CompressionLevel,
                MsccControlClient.CompressionLevelMin,
                MsccControlClient.CompressionLevelMax);
            _lblCompression.Text = _trkCompression.Value.ToString();
            _lastCmpLevelSent = _trkCompression.Value;
            AppendLog(
                $"Apply settings: RX={s.RxPort} jitter={s.JitterMs} play='{s.PlayDevice}' " +
                $"vol={s.VolumePct} mute={s.Mute} tx={s.TxHost}:{s.TxPort} " +
                $"cmp={(s.CompressionOn ? "on" : "off")}/{s.CompressionLevel}");
        }
        finally
        {
            _suppressSave = false;
        }
        ApplyEqFromUi();
    }

    private static void SelectComboDevice(ComboBox cmb, string name, int? index)
    {
        if (cmb.Items.Count == 0)
            return;

        string want = (name ?? "").Trim();

        // 1) Exact name
        if (want.Length > 0)
        {
            for (int i = 0; i < cmb.Items.Count; i++)
            {
                if (cmb.Items[i] is DeviceItem di &&
                    string.Equals(di.Name, want, StringComparison.OrdinalIgnoreCase))
                {
                    cmb.SelectedIndex = i;
                    return;
                }
            }
            // 2) Prefix / contains (WaveIn 32-char truncation vs WASAPI full name)
            for (int i = 0; i < cmb.Items.Count; i++)
            {
                if (cmb.Items[i] is not DeviceItem di)
                    continue;
                if (di.Name.StartsWith(want, StringComparison.OrdinalIgnoreCase) ||
                    want.StartsWith(di.Name, StringComparison.OrdinalIgnoreCase) ||
                    di.Name.Contains(want, StringComparison.OrdinalIgnoreCase) ||
                    want.Contains(di.Name, StringComparison.OrdinalIgnoreCase))
                {
                    cmb.SelectedIndex = i;
                    return;
                }
            }
        }

        // 3) Saved WaveIn index
        if (index is int idx)
        {
            for (int i = 0; i < cmb.Items.Count; i++)
            {
                if (cmb.Items[i] is DeviceItem di && di.Index == idx)
                {
                    cmb.SelectedIndex = i;
                    return;
                }
            }
        }

        cmb.SelectedIndex = 0;
    }

    private void ApplyEqFromUi()
    {
        _player.Eq.ApplySettings(
            _chkEqEnable.Checked,
            TrackToDb(_trkEqLow.Value),
            TrackToDb(_trkEqMid.Value),
            TrackToDb(_trkEqHigh.Value));
    }

    private void SaveSettingsFromUi()
    {
        if (!_settingsReady || _suppressSave)
            return;
        var s = new AppSettings
        {
            RxPort = (int)_numPort.Value,
            JitterMs = (int)_numJitter.Value,
            PlayDevice = _cmbDevice.SelectedItem is DeviceItem pd ? pd.Name : "",
            VolumePct = _trkVolume.Value,
            Mute = _chkMute.Checked,
            EqEnabled = _chkEqEnable.Checked,
            EqLowDb = TrackToDb(_trkEqLow.Value),
            EqMidDb = TrackToDb(_trkEqMid.Value),
            EqHighDb = TrackToDb(_trkEqHigh.Value),
            TxHost = _txtTxHost.Text.Trim(),
            TxPort = (int)_numTxPort.Value,
            MicDevice = _cmbMic.SelectedItem is DeviceItem md ? md.Name : "",
            MicDeviceIndex = _cmbMic.SelectedItem is DeviceItem mdi ? mdi.Index : -1,
            MicVolumePct = _trkMicVolume.Value,
            AutoStartRx = _receiver.IsRunning || _btnStop.Enabled,
            AutoStartMic = _mic.IsRunning || _btnMicStop.Enabled,
            MsSdrPort = _msSdrPort,
            CompressionOn = _chkCompression.Checked,
            CompressionLevel = _trkCompression.Value,
        };
        AppSettingsStore.Save(s);
    }

    private void LoadDevices()
    {
        _cmbDevice.Items.Clear();
        foreach (var d in RemotePhonePlayer.ListPlayDevices())
            _cmbDevice.Items.Add(new DeviceItem(d.Index, d.Name));
        if (_cmbDevice.Items.Count > 0)
            _cmbDevice.SelectedIndex = 0;
    }

    private void LoadMicDevices()
    {
        _cmbMic.Items.Clear();
        foreach (var d in RemoteMicSender.ListCaptureDevices())
            _cmbMic.Items.Add(new DeviceItem(d.Index, d.Name));
        // Do not force index 0 here — ApplySettingsToUi restores the sticky mic.
    }

    private int SelectedDeviceIndex()
        => _cmbDevice.SelectedItem is DeviceItem di ? di.Index : -1;

    private int SelectedMicIndex()
        => _cmbMic.SelectedItem is DeviceItem di ? di.Index : -1;

    private void StartRx(bool retryBind = false)
    {
        var port = (int)_numPort.Value;
        var jitter = (int)_numJitter.Value;
        var deviceIndex = SelectedDeviceIndex();

        // Always tear down prior player so a new device selection is applied
        // on the next audio packet (OnPacket / EnsurePlayer).
        _player.Stop();
        _jitter.Clear();
        _playerArmed = false;
        _playDeviceIndex = int.MinValue;

        int attempts = retryBind ? 8 : 1;
        Exception? last = null;
        for (int i = 0; i < attempts; i++)
        {
            try
            {
                _receiver.Start(port);
                last = null;
                break;
            }
            catch (Exception ex)
            {
                last = ex;
                AppendLog($"RX bind attempt {i + 1}/{attempts} failed: {ex.Message}");
                if (i + 1 < attempts)
                    Thread.Sleep(250);
            }
        }

        if (last != null)
        {
            AppendLog("RX start failed: " + last.Message);
            /* Do not clear RX_AUTO_START — MSCC may relaunch us after a port race. */
            StopRx(clearAutoStart: false);
            MessageBox.Show(this, last.Message, "Start RX failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return;
        }

        _btnStart.Enabled = false;
        _btnStop.Enabled = true;
        _numPort.Enabled = false;
        _numJitter.Enabled = false;
        _cmbDevice.Enabled = false;
        RefreshStatus();
        AppendLog($"RX start port={port} jitter={jitter} ms deviceIndex={deviceIndex}");
        SaveSettingsFromUi();
    }

    private void StopRx(bool clearAutoStart = true)
    {
        _player.Stop();
        _receiver.Stop();
        _playerArmed = false;
        _playDeviceIndex = int.MinValue;
        _btnStart.Enabled = true;
        _btnStop.Enabled = false;
        _numPort.Enabled = true;
        _numJitter.Enabled = true;
        _cmbDevice.Enabled = true;
        RefreshStatus();
        if (clearAutoStart)
            SaveSettingsFromUi();
    }

    private void StartMic(bool saveSticky = true)
    {
        try
        {
            var host = _txtTxHost.Text.Trim();
            var port = (int)_numTxPort.Value;
            var micIndex = SelectedMicIndex();
            _mic.Volume = _trkMicVolume.Value / 100f;
            _mic.Start(host, port, micIndex);

            _btnMicStart.Enabled = false;
            _btnMicStop.Enabled = true;
            _txtTxHost.Enabled = false;
            _numTxPort.Enabled = false;
            _cmbMic.Enabled = false;
            RefreshStatus();
            AppendLog($"Mic TX start → {host}:{port} deviceIndex={micIndex}");
            if (saveSticky)
                SaveSettingsFromUi();
        }
        catch (Exception ex)
        {
            AppendLog("Mic TX start failed: " + ex.Message);
            MessageBox.Show(this, ex.Message, "Start Mic TX failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            /* Keep MIC_AUTO_START on bind/device race during MSCC relaunch. */
            StopMic(clearAutoStart: false);
        }
    }

    private void StopMic(bool clearAutoStart = true)
    {
        _mic.Stop();
        _btnMicStart.Enabled = true;
        _btnMicStop.Enabled = false;
        _txtTxHost.Enabled = true;
        _numTxPort.Enabled = true;
        _cmbMic.Enabled = true;
        RefreshStatus();
        if (clearAutoStart)
            SaveSettingsFromUi();
    }

    private void RefreshStatus()
    {
        string rx = _receiver.IsRunning
            ? (_playerArmed
                ? $"RX {_lastRate} Hz → {_player.DeviceName}"
                : $"RX listening UDP {_numPort.Value}")
            : "RX stopped";
        string tx = _mic.IsRunning
            ? $"TX → {_txtTxHost.Text.Trim()}:{_numTxPort.Value} ({_mic.DeviceName})"
            : "TX stopped";
        _lblStatus.Text = $"{rx}  |  {tx}";
    }

    private sealed class DeviceItem
    {
        public int Index { get; }
        public string Name { get; }
        public DeviceItem(int index, string name) { Index = index; Name = name; }
        public override string ToString() => Name;
    }

    private void OnPacket(AudioPacketHeader hdr)
    {
        if (InvokeRequired)
        {
            BeginInvoke(() => OnPacket(hdr));
            return;
        }

        _lastRate = (int)hdr.SampleRate;
        _lastCh = hdr.Channels;
        TryStartPlayer();
    }

    /// <summary>
    /// Open/reopen the player when disarmed, format changed, or play-device selection changed.
    /// </summary>
    private void TryStartPlayer()
    {
        if (!_receiver.IsRunning)
            return;

        int deviceIndex = SelectedDeviceIndex();
        bool need =
            !_playerArmed
            || !_player.IsPlaying
            || deviceIndex != _playDeviceIndex;

        if (!need)
            return;

        try
        {
            _player.Start(_lastRate, _lastCh, deviceIndex, (int)_numJitter.Value);
            _playDeviceIndex = deviceIndex;
            _playerArmed = true;
            RefreshStatus();
        }
        catch (Exception ex)
        {
            _playerArmed = false;
            _playDeviceIndex = int.MinValue;
            AppendLog("Player reconfig failed: " + ex.Message);
        }
    }

    private void UpdateStats()
    {
        if (!_receiver.IsRunning && !_player.IsPlaying && !_mic.IsRunning)
            return;
        _lblStats.Text =
            $"RX pkts={_jitter.ReceivedPackets} bad={_receiver.BadPackets} queued={_jitter.QueuedSamples} " +
            $"drop={_jitter.DroppedPackets} play_buf≈{_player.BufferedMs} ms  |  " +
            $"TX pkts={_mic.PacketsSent} samp={_mic.SamplesSent}";
    }

    private void AppendLog(string msg)
    {
        if (IsDisposed) return;
        if (InvokeRequired)
        {
            BeginInvoke(() => AppendLog(msg));
            return;
        }
        var line = $"{DateTime.Now:HH:mm:ss}  {msg}";
        _txtLog.AppendText(line + Environment.NewLine);
    }
}
