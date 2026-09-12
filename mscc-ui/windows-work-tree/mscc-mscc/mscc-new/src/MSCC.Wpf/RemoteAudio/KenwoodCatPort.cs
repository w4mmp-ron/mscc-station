using System.IO.Ports;
using System.Text;
using MSCC.Core.Protocol;

namespace MSCC.Wpf.RemoteAudio;

/// <summary>
/// Opens the ms-sdr side of the local TS-2000 COM pair and runs <see cref="KenwoodTs2000"/>.
/// WSJT-X keeps its existing COM / TS-2000 settings.
/// </summary>
public sealed class KenwoodCatPort : IDisposable
{
    private SerialPort? _port;
    private Thread? _thread;
    private volatile bool _run;
    private readonly StringBuilder _buf = new();

    public KenwoodTs2000 Engine { get; } = new();
    public event Action<string>? Log;
    public bool IsOpen => _port is { IsOpen: true };
    public string? PortName { get; private set; }

    public void Start(CommPortConfig.Settings cfg)
    {
        Stop();
        string name = CommPortConfig.NormalizePortName(cfg.PortName);
        if (string.IsNullOrWhiteSpace(name) ||
            name.Contains("PTY", StringComparison.OrdinalIgnoreCase) ||
            name.StartsWith("/DEV", StringComparison.OrdinalIgnoreCase))
        {
            Log?.Invoke("CAT skip: comm-port.ini is not a Windows COM (PTY/Linux).");
            return;
        }

        int baud = CommPortConfig.BaudRates[Math.Clamp(cfg.BaudRateIndex, 0, CommPortConfig.BaudRates.Length - 1)];
        var port = new SerialPort
        {
            PortName = name,
            BaudRate = baud,
            Parity = cfg.ParityIndex switch { 1 => Parity.Odd, 2 => Parity.Even, _ => Parity.None },
            DataBits = cfg.DataBitsIndex == 0 ? 7 : 8,
            StopBits = cfg.StopBitsIndex == 1 ? StopBits.Two : StopBits.One,
            Handshake = Handshake.None,
            DtrEnable = true,
            RtsEnable = true,
            ReadTimeout = 200,
            WriteTimeout = 200,
            Encoding = Encoding.ASCII,
        };
        try
        {
            port.Open();
        }
        catch (Exception ex)
        {
            port.Dispose();
            Log?.Invoke($"CAT open {name} failed: {ex.Message}");
            return;
        }

        _port = port;
        PortName = name;
        Engine.Trace = s => Log?.Invoke(s);
        _run = true;
        _thread = new Thread(ReadLoop)
        {
            IsBackground = true,
            Name = "KenwoodCAT",
        };
        _thread.Start();
        Log?.Invoke($"CAT TS-2000 on {name} {baud} 8N1 (WSJT-X uses the other end of the pair)");
    }

    public void Stop()
    {
        _run = false;
        try { _port?.Close(); } catch { /* ignore */ }
        try { _thread?.Join(400); } catch { /* ignore */ }
        try { _port?.Dispose(); } catch { /* ignore */ }
        _port = null;
        _thread = null;
        PortName = null;
        lock (_buf) { _buf.Clear(); }
    }

    public void Dispose() => Stop();

    private void ReadLoop()
    {
        var port = _port;
        if (port == null) return;
        while (_run && port.IsOpen)
        {
            try
            {
                int b = port.ReadByte();
                if (b < 0) continue;
                char c = (char)b;
                if (c == ';')
                {
                    string cmd;
                    lock (_buf)
                    {
                        _buf.Append(';');
                        cmd = _buf.ToString();
                        _buf.Clear();
                    }
                    string? reply = Engine.Handle(cmd);
                    if (!string.IsNullOrEmpty(reply))
                        Write(reply);
                }
                else if (c >= 32 && c < 127)
                {
                    lock (_buf)
                    {
                        if (_buf.Length > 256)
                            _buf.Clear();
                        _buf.Append(c);
                    }
                }
            }
            catch (TimeoutException)
            {
                /* ReadTimeout */
            }
            catch (Exception ex)
            {
                if (_run)
                    Log?.Invoke("CAT read: " + ex.Message);
                break;
            }
        }
    }

    private void Write(string reply)
    {
        try
        {
            var port = _port;
            if (port is { IsOpen: true })
                port.Write(reply);
        }
        catch (Exception ex)
        {
            Log?.Invoke("CAT write: " + ex.Message);
        }
    }
}
