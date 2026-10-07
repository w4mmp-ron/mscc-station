using System.IO.Ports;
using System.Text;
using MSCC.Core.Protocol;

namespace MSCC.Avalonia.RemoteAudio;

/// <summary>TS-2000 CAT on Linux PTY (/dev/tnt0 or $HOME/ms-sdr-cat).</summary>
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

    public void Start()
    {
        Stop();
        string? name = ResolveLinuxCatPath();
        if (string.IsNullOrWhiteSpace(name) || !File.Exists(name))
        {
            Log?.Invoke("CAT skip: no /dev/tnt0 or $HOME/ms-sdr-cat (tty0tty).");
            return;
        }

        var port = new SerialPort
        {
            PortName = name,
            BaudRate = 38400,
            Parity = Parity.None,
            DataBits = 8,
            StopBits = StopBits.One,
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
            Log?.Invoke($"CAT open {name} failed: {ex.Message} (ms-sdr may still hold the PTY)");
            return;
        }

        _port = port;
        PortName = name;
        Engine.Trace = s => Log?.Invoke(s);
        _run = true;
        _thread = new Thread(ReadLoop) { IsBackground = true, Name = "KenwoodCAT" };
        _thread.Start();
        Log?.Invoke($"CAT TS-2000 on {name} (WSJT-X uses the other tty0tty end)");
    }

    public static string? ResolveLinuxCatPath()
    {
        string home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        string[] candidates =
        {
            Path.Combine(home, "ms-sdr-cat"),
            "/dev/tnt0",
            "/dev/tnt1",
        };
        try
        {
            string ini = Path.Combine(home, ".local", "mscc", "comm-port.ini");
            if (File.Exists(ini))
            {
                string text = File.ReadAllText(ini);
                int i = text.IndexOf("COMM_PORT_NAME=", StringComparison.OrdinalIgnoreCase);
                if (i >= 0)
                {
                    string rest = text[(i + 15)..];
                    int comma = rest.IndexOf(',');
                    string n = (comma >= 0 ? rest[..comma] : rest).Trim();
                    if (n.StartsWith("/dev/", StringComparison.Ordinal) || n.Contains("tnt", StringComparison.Ordinal))
                        return n;
                }
            }
        }
        catch { /* fall through */ }

        foreach (string c in candidates)
        {
            try
            {
                if (File.Exists(c) || Directory.Exists(c))
                    return c;
                var fi = new FileInfo(c);
                if (fi.Exists || fi.LinkTarget != null)
                    return c;
            }
            catch { /* next */ }
        }
        return File.Exists("/dev/tnt0") ? "/dev/tnt0" : null;
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
            catch (TimeoutException) { /* ok */ }
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
            if (_port is { IsOpen: true })
                _port.Write(reply);
        }
        catch (Exception ex)
        {
            Log?.Invoke("CAT write: " + ex.Message);
        }
    }
}
