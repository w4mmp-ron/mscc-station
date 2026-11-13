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
    /// <summary>comm-port.ini PIN=: 0 off, 1 CTS (WSJT-X RTS), 2 DCD (WSJT-X DTR).</summary>
    private int _pin;

    public KenwoodTs2000 Engine { get; } = new();
    public event Action<string>? Log;
    public bool IsOpen => _port?.IsOpen == true;
    public string? PortName { get; private set; }
    /// <summary>PTT line in use: 0 off (CAT only), 1 CTS (WSJT-X RTS), 2 DCD (WSJT-X DTR).</summary>
    public int PttPin => _pin;

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
        _pin = cfg.Pin is 1 or 2 ? cfg.Pin : 0;
        var port = new NativeComPort();
        try
        {
            // Watching a PTT line: short read wait so the line is checked every ~10 ms
            // (the handle is not overlapped, so the check has to run on the read thread).
            port.Open(name, baud, _pin != 0 ? 10u : 200u);
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
        Log?.Invoke(_pin switch
        {
            1 => "CAT PTT line: CTS (WSJT-X PTT = RTS), PIN=1",
            2 => "CAT PTT line: DCD (WSJT-X PTT = DTR), PIN=2",
            _ => "CAT PTT line: off (PIN=0) — use CAT PTT in WSJT-X",
        });
    }

    public void Stop()
    {
        _run = false;
        try { Engine.CancelTune(); } catch { /* ignore */ }
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
        string line = _pin == 1 ? "CTS" : "DCD";
        uint mask = _pin == 1 ? NativeComPort.MsCtsOn : NativeComPort.MsRlsdOn;
        bool keyed = false;
        bool statusFailed = false;
        try
        {
            ReadLoopBody(port, line, mask, ref keyed, ref statusFailed);
        }
        finally
        {
            if (keyed)
                Engine.SetLinePtt(false, line);   // port closing: never leave the radio keyed
        }
    }

    private void ReadLoopBody(NativeComPort port, string line, uint mask, ref bool keyed, ref bool statusFailed)
    {
        while (_run && port.IsOpen)
        {
            try
            {
                // PTT from a control line (like ms-sdr Pin_Check_Thread / Avalonia 1.6.81):
                // act only on a change.
                if (_pin != 0)
                {
                    bool on = false;
                    if (port.TryGetModemStatus(out uint st))
                    {
                        on = (st & mask) != 0;
                        statusFailed = false;
                    }
                    else if (!statusFailed && _run)
                    {
                        statusFailed = true;
                        Log?.Invoke($"CAT {line} read failed (Win32 {System.Runtime.InteropServices.Marshal.GetLastWin32Error()})");
                    }
                    if (on != keyed)
                    {
                        keyed = on;
                        Engine.SetLinePtt(on, line);
                    }
                }

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
