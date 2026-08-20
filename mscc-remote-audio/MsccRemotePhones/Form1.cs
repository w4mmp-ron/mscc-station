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
    private Label _lblStatus = null!;
    private TextBox _txtLog = null!;
    private Label _lblStats = null!;

    private int _lastRate = MsccAudioProtocol.DefaultSampleRate;
    private int _lastCh = MsccAudioProtocol.DefaultChannels;
    private int _playDeviceIndex = int.MinValue; // device used for current player
    private bool _playerArmed;
    /// <summary>True only after INI has been applied — blocks saves during construction.</summary>
    private bool _settingsReady;
    private bool _suppressSave;

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
        AppendLog($"Settings loaded from {AppSettingsStore.ConfigPath}");
    }

    private void InitializeUi()
    {
        Text = "MSCC Remote Phones / Mic";
        Width = 620;
        Height = 780;
        StartPosition = FormStartPosition.CenterScreen;
        FormClosing += (_, _) =>
        {
            _settingsReady = true;
            _suppressSave = false;
            SaveSettingsFromUi();
            _statsTimer.Stop();
            _mic.Stop();
            _player.Stop();
            _receiver.Stop();
        };

        var root = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 18,
            Padding = new Padding(10),
        };
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 130));
        root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));

        int row = 0;

        // --- RX ---
        var rxHdr = new Label
        {
            Text = "RX (phones from Pi)",
            Font = new Font(Font, FontStyle.Bold),
            AutoSize = true,
        };
        root.SetColumnSpan(rxHdr, 2);
        root.Controls.Add(rxHdr, 0, row++);

        root.Controls.Add(new Label { Text = "UDP listen port", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        _numPort = new NumericUpDown
        {
            Minimum = 1024,
            Maximum = 65535,
            Value = MsccAudioProtocol.DefaultPort,
            Width = 100,
        };
        _numPort.ValueChanged += (_, _) => SaveSettingsFromUi();
        root.Controls.Add(_numPort, 1, row++);

        root.Controls.Add(new Label { Text = "Jitter target (ms)", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        _numJitter = new NumericUpDown { Minimum = 20, Maximum = 300, Value = 80, Width = 100 };
        _numJitter.ValueChanged += (_, _) => SaveSettingsFromUi();
        root.Controls.Add(_numJitter, 1, row++);

        root.Controls.Add(new Label { Text = "Play device", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        _cmbDevice = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Dock = DockStyle.Fill };
        _cmbDevice.SelectedIndexChanged += (_, _) =>
        {
            OnPlayDeviceChanged();
            SaveSettingsFromUi();
        };
        root.Controls.Add(_cmbDevice, 1, row++);

        root.Controls.Add(new Label { Text = "Phones volume", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        root.Controls.Add(MakeVolumeRow(out _trkVolume, out _lblVolume, 80, v =>
        {
            _player.Volume = v / 100f;
            _lblVolume.Text = $"{v}%";
            SaveSettingsFromUi();
        }), 1, row++);
        _player.Volume = 0.8f;

        _chkMute = new CheckBox { Text = "Mute phones", AutoSize = true };
        _chkMute.CheckedChanged += (_, _) =>
        {
            _player.Muted = _chkMute.Checked;
            AppendLog(_chkMute.Checked ? "Phones muted" : "Phones unmuted");
            SaveSettingsFromUi();
        };
        root.Controls.Add(new Label { Text = "", AutoSize = true }, 0, row);
        root.Controls.Add(_chkMute, 1, row++);

        // --- Playback EQ ---
        var eqHdr = new Label
        {
            Text = "Playback EQ (local phones only)",
            Font = new Font(Font, FontStyle.Bold),
            AutoSize = true,
        };
        root.SetColumnSpan(eqHdr, 2);
        root.Controls.Add(eqHdr, 0, row++);

        _chkEqEnable = new CheckBox { Text = "Enable EQ", AutoSize = true };
        _chkEqEnable.CheckedChanged += (_, _) =>
        {
            ApplyEqFromUi();
            SaveSettingsFromUi();
        };
        root.Controls.Add(new Label { Text = "", AutoSize = true }, 0, row);
        root.Controls.Add(_chkEqEnable, 1, row++);

        root.Controls.Add(new Label { Text = "Low (~120 Hz)", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        root.Controls.Add(MakeEqRow(out _trkEqLow, out _lblEqLow), 1, row++);
        root.Controls.Add(new Label { Text = "Mid (~2 kHz)", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        root.Controls.Add(MakeEqRow(out _trkEqMid, out _lblEqMid), 1, row++);
        root.Controls.Add(new Label { Text = "High (~5 kHz)", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        root.Controls.Add(MakeEqRow(out _trkEqHigh, out _lblEqHigh), 1, row++);

        _btnEqReset = new Button { Text = "Reset EQ", Width = 90 };
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
        root.Controls.Add(new Label { Text = "", AutoSize = true }, 0, row);
        root.Controls.Add(_btnEqReset, 1, row++);

        var rxButtons = new FlowLayoutPanel { Dock = DockStyle.Fill, AutoSize = true };
        _btnStart = new Button { Text = "Start RX", Width = 90 };
        _btnStop = new Button { Text = "Stop RX", Width = 90, Enabled = false };
        _btnStart.Click += (_, _) => StartRx();
        _btnStop.Click += (_, _) => StopRx();
        rxButtons.Controls.Add(_btnStart);
        rxButtons.Controls.Add(_btnStop);
        root.Controls.Add(new Label { Text = "", AutoSize = true }, 0, row);
        root.Controls.Add(rxButtons, 1, row++);

        // --- TX ---
        var txHdr = new Label
        {
            Text = "TX (mic → Pi / loopback)  — Windows only; sdrcore-trans not wired yet",
            Font = new Font(Font, FontStyle.Bold),
            AutoSize = true,
        };
        root.SetColumnSpan(txHdr, 2);
        root.Controls.Add(txHdr, 0, row++);

        root.Controls.Add(new Label { Text = "TX host (Pi)", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        _txtTxHost = new TextBox { Text = "127.0.0.1", Dock = DockStyle.Fill };
        _txtTxHost.Leave += (_, _) => SaveSettingsFromUi();
        root.Controls.Add(_txtTxHost, 1, row++);

        root.Controls.Add(new Label { Text = "TX UDP port", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        _numTxPort = new NumericUpDown
        {
            Minimum = 1024,
            Maximum = 65535,
            Value = MsccAudioProtocol.DefaultTxPort,
            Width = 100,
        };
        _numTxPort.ValueChanged += (_, _) => SaveSettingsFromUi();
        root.Controls.Add(_numTxPort, 1, row++);

        root.Controls.Add(new Label { Text = "Microphone", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        _cmbMic = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Dock = DockStyle.Fill };
        _cmbMic.SelectedIndexChanged += (_, _) => SaveSettingsFromUi();
        root.Controls.Add(_cmbMic, 1, row++);

        root.Controls.Add(new Label { Text = "Mic volume", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        root.Controls.Add(MakeVolumeRow(out _trkMicVolume, out _lblMicVolume, 80, v =>
        {
            _mic.Volume = v / 100f;
            _lblMicVolume.Text = $"{v}%";
            SaveSettingsFromUi();
        }), 1, row++);
        _mic.Volume = 0.8f;

        var txButtons = new FlowLayoutPanel { Dock = DockStyle.Fill, AutoSize = true };
        _btnMicStart = new Button { Text = "Start Mic TX", Width = 100 };
        _btnMicStop = new Button { Text = "Stop Mic TX", Width = 100, Enabled = false };
        _btnMicStart.Click += (_, _) => StartMic();
        _btnMicStop.Click += (_, _) => StopMic();
        txButtons.Controls.Add(_btnMicStart);
        txButtons.Controls.Add(_btnMicStop);
        root.Controls.Add(new Label { Text = "", AutoSize = true }, 0, row);
        root.Controls.Add(txButtons, 1, row++);

        _lblStatus = new Label
        {
            Text = "RX: Stopped.  TX: Stopped.  Loopback test: Start RX on port 9101, Mic TX to 127.0.0.1:9101.",
            AutoSize = true,
            Dock = DockStyle.Fill,
        };
        root.SetColumnSpan(_lblStatus, 2);
        root.Controls.Add(_lblStatus, 0, row++);

        _lblStats = new Label { Text = "—", AutoSize = true, Dock = DockStyle.Fill };
        root.SetColumnSpan(_lblStats, 2);
        root.Controls.Add(_lblStats, 0, row++);

        _txtLog = new TextBox
        {
            Multiline = true,
            ReadOnly = true,
            ScrollBars = ScrollBars.Vertical,
            Dock = DockStyle.Fill,
            Font = new Font(FontFamily.GenericMonospace, 9f),
        };
        root.SetColumnSpan(_txtLog, 2);
        for (int i = 0; i < row; i++)
            root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        root.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        root.Controls.Add(_txtLog, 0, row);

        Controls.Add(root);
    }

    private static Control MakeVolumeRow(out TrackBar track, out Label pct, int initial, Action<int> onChange)
    {
        var volRow = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 1,
            AutoSize = true,
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
        };
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 56));
        track = new TrackBar
        {
            Minimum = 0,
            Maximum = 240,
            Value = 120,
            TickFrequency = 20,
            Dock = DockStyle.Fill,
            Height = 36,
        };
        dbLabel = new Label
        {
            Text = "0.0 dB",
            AutoSize = true,
            Anchor = AnchorStyles.Left,
            TextAlign = ContentAlignment.MiddleLeft,
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
            SelectComboByName(_cmbDevice, s.PlayDevice);
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
            SelectComboByName(_cmbMic, s.MicDevice);
            _trkMicVolume.Value = Math.Clamp(s.MicVolumePct, 0, 100);
            _lblMicVolume.Text = $"{_trkMicVolume.Value}%";
            _mic.Volume = _trkMicVolume.Value / 100f;
            AppendLog(
                $"Apply settings: RX={s.RxPort} jitter={s.JitterMs} play='{s.PlayDevice}' " +
                $"vol={s.VolumePct} mute={s.Mute} tx={s.TxHost}:{s.TxPort}");
        }
        finally
        {
            _suppressSave = false;
        }
        ApplyEqFromUi();
    }

    private static void SelectComboByName(ComboBox cmb, string name)
    {
        if (string.IsNullOrWhiteSpace(name) || cmb.Items.Count == 0)
        {
            if (cmb.Items.Count > 0)
                cmb.SelectedIndex = 0;
            return;
        }
        for (int i = 0; i < cmb.Items.Count; i++)
        {
            if (cmb.Items[i] is DeviceItem di &&
                string.Equals(di.Name, name, StringComparison.OrdinalIgnoreCase))
            {
                cmb.SelectedIndex = i;
                return;
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
            MicVolumePct = _trkMicVolume.Value,
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
        if (_cmbMic.Items.Count > 0)
            _cmbMic.SelectedIndex = 0;
    }

    private int SelectedDeviceIndex()
        => _cmbDevice.SelectedItem is DeviceItem di ? di.Index : -1;

    private int SelectedMicIndex()
        => _cmbMic.SelectedItem is DeviceItem di ? di.Index : -1;

    private void StartRx()
    {
        try
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
            _receiver.Start(port);

            _btnStart.Enabled = false;
            _btnStop.Enabled = true;
            _numPort.Enabled = false;
            RefreshStatus();
            AppendLog($"RX start port={port} jitter={jitter} ms deviceIndex={deviceIndex}");
        }
        catch (Exception ex)
        {
            AppendLog("RX start failed: " + ex.Message);
            MessageBox.Show(this, ex.Message, "Start RX failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            StopRx();
        }
    }

    private void StopRx()
    {
        _player.Stop();
        _receiver.Stop();
        _playerArmed = false;
        _playDeviceIndex = int.MinValue;
        _btnStart.Enabled = true;
        _btnStop.Enabled = false;
        _numPort.Enabled = true;
        RefreshStatus();
    }

    private void OnPlayDeviceChanged()
    {
        if (!_receiver.IsRunning)
            return;
        // Force player reopen on next packet / immediately if already armed
        _playerArmed = false;
        _playDeviceIndex = int.MinValue;
        _player.Stop();
        AppendLog($"Play device → index {SelectedDeviceIndex()} (will apply on next audio)");
        TryStartPlayer();
        RefreshStatus();
    }

    private void StartMic()
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
        }
        catch (Exception ex)
        {
            AppendLog("Mic TX start failed: " + ex.Message);
            MessageBox.Show(this, ex.Message, "Start Mic TX failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            StopMic();
        }
    }

    private void StopMic()
    {
        _mic.Stop();
        _btnMicStart.Enabled = true;
        _btnMicStop.Enabled = false;
        _txtTxHost.Enabled = true;
        _numTxPort.Enabled = true;
        _cmbMic.Enabled = true;
        RefreshStatus();
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
