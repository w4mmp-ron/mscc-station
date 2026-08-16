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
    private bool _playerArmed;

    public Form1()
    {
        _receiver = new UdpAudioReceiver(_jitter);
        _player = new RemotePhonePlayer(_jitter);
        _mic = new RemoteMicSender();
        _receiver.Log += AppendLog;
        _player.Log += AppendLog;
        _mic.Log += AppendLog;
        _receiver.PacketAccepted += OnPacket;

        InitializeUi();
        LoadDevices();
        LoadMicDevices();
        _statsTimer.Tick += (_, _) => UpdateStats();
        _statsTimer.Start();
    }

    private void InitializeUi()
    {
        Text = "MSCC Remote Phones / Mic";
        Width = 600;
        Height = 640;
        StartPosition = FormStartPosition.CenterScreen;
        FormClosing += (_, _) =>
        {
            _statsTimer.Stop();
            _mic.Stop();
            _player.Stop();
            _receiver.Stop();
        };

        var root = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 2,
            RowCount = 13,
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
        root.Controls.Add(_numPort, 1, row++);

        root.Controls.Add(new Label { Text = "Jitter target (ms)", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        _numJitter = new NumericUpDown { Minimum = 20, Maximum = 300, Value = 80, Width = 100 };
        root.Controls.Add(_numJitter, 1, row++);

        root.Controls.Add(new Label { Text = "Play device", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        _cmbDevice = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Dock = DockStyle.Fill };
        root.Controls.Add(_cmbDevice, 1, row++);

        root.Controls.Add(new Label { Text = "Phones volume", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        root.Controls.Add(MakeVolumeRow(out _trkVolume, out _lblVolume, 80, v =>
        {
            _player.Volume = v / 100f;
            _lblVolume.Text = $"{v}%";
        }), 1, row++);
        _player.Volume = 0.8f;

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
        root.Controls.Add(_txtTxHost, 1, row++);

        root.Controls.Add(new Label { Text = "TX UDP port", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        _numTxPort = new NumericUpDown
        {
            Minimum = 1024,
            Maximum = 65535,
            Value = MsccAudioProtocol.DefaultTxPort,
            Width = 100,
        };
        root.Controls.Add(_numTxPort, 1, row++);

        root.Controls.Add(new Label { Text = "Microphone", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        _cmbMic = new ComboBox { DropDownStyle = ComboBoxStyle.DropDownList, Dock = DockStyle.Fill };
        root.Controls.Add(_cmbMic, 1, row++);

        root.Controls.Add(new Label { Text = "Mic volume", AutoSize = true, Anchor = AnchorStyles.Left }, 0, row);
        root.Controls.Add(MakeVolumeRow(out _trkMicVolume, out _lblMicVolume, 80, v =>
        {
            _mic.Volume = v / 100f;
            _lblMicVolume.Text = $"{v}%";
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

            _jitter.Clear();
            _playerArmed = false;
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
        _btnStart.Enabled = true;
        _btnStop.Enabled = false;
        _numPort.Enabled = true;
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

        if (!_playerArmed || hdr.SampleRate != _lastRate || hdr.Channels != _lastCh)
        {
            _lastRate = (int)hdr.SampleRate;
            _lastCh = hdr.Channels;
            var deviceIndex = SelectedDeviceIndex();

            try
            {
                _player.Start(_lastRate, _lastCh, deviceIndex, (int)_numJitter.Value);
                _playerArmed = true;
                RefreshStatus();
            }
            catch (Exception ex)
            {
                AppendLog("Player reconfig failed: " + ex.Message);
            }
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
