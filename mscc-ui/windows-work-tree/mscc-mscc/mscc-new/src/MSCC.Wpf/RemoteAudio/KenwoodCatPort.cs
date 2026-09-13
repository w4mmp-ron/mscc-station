using System.Text;
using MSCC.Core.Protocol;

namespace MSCC.Wpf.RemoteAudio;

/// <summary>
/// Opens the ms-sdr side of the local TS-2000 COM pair and runs <see cref="KenwoodTs2000"/>.
/// WSJT-X keeps its existing COM / TS-2000 settings.
/// </summary>
public sealed class KenwoodCatPort : IDisposable
{
    private NativeComPort? _port;
    private Thread? _thread;
    private volatile bool _run;
    private readonly StringBuilder _buf = new();

    public KenwoodTs2000 Engine { get; } = new();
    public event Action<string>? Log;
    public bool IsOpen => _port?.IsOpen == true;
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
        var port = new NativeComPort();
        try
        {
            port.Open(name, baud);
        }
        catch (Exception ex)
        {
            port.Dispose();
            string listed = string.Join(", ", CommPortConfig.GetAvailablePorts());
            Log?.Invoke($"CAT open {name} failed: {ex.Message}");
            Log?.Invoke($" CAT ports on this PC: {(string.IsNullOrEmpty(listed) ? "(none)" : listed)}. WSJT-X = other end of Eltima pair (COM5↔COM15).");
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
        Log?.Invoke($"CAT TS-2000 on {name} {baud} 8N1 (CreateFile \\\\.\\{name}; WSJT-X uses the other end of the pair)");
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
                port.Write(Encoding.ASCII.GetBytes(reply));
        }
        catch (Exception ex)
        {
            Log?.Invoke("CAT write: " + ex.Message);
        }
    }
}
