using System.Globalization;
using System.Text;
using System.Threading;

namespace MSCC.Core.Protocol;

/// <summary>
/// Kenwood TS-2000 CAT (same dialect as ms-sdr comm-port.c). Semicolon-terminated.
/// Local WSJT-X keeps Rig=TS-2000; the operate client maps FA/MD/TX onto UDP 8888.
/// </summary>
public sealed class KenwoodTs2000
{
    public Func<long> GetFrequencyHz { get; set; } = () => 0;
    public Func<bool> GetTransmitting { get; set; } = () => false;
    /// <summary>Kenwood MD digit: 1 LSB, 2 USB, 3 CW, 4 FM, 5 AM.</summary>
    public Func<char> GetModeDigit { get; set; } = () => '2';
    public Action<long>? SetFrequencyHz { get; set; }
    public Action<char>? SetModeDigit { get; set; }
    public Action<bool>? SetPtt { get; set; }
    /// <summary>Press / release the client's TUN (CAT tune, AC111;).</summary>
    public Action<bool>? SetTune { get; set; }
    /// <summary>Is the client's TUN on? Lets a tune ended elsewhere (TUN button, fault) be seen.</summary>
    public Func<bool> GetTuning { get; set; } = () => false;
    public Action<string>? Trace { get; set; }

    /*
     * CAT tune, same rules as ms-sdr comm-port.c (2026-11-11). WSJT-X right-click Tune -> hamlib
     * TS-2000 RIG_OP_TUNE sends "AC111;" and nothing to end it (a real TS-2000 tuner stops by
     * itself). So AC111; presses TUN, and the tune ends:
     *   - by itself after TuneSeconds,
     *   - at the next AC111; (second right-click); one within TuneDebounceMs of the start is ignored,
     *   - before a CAT TX; (PTT wins),
     *   - if TUN went off some other way (button, fault): only the timer stops.
     */
    public const int TuneSeconds = 10;
    public const int TuneDebounceMs = 1000;
    private readonly object _tuneLock = new();
    private Timer? _tuneTimer;
    private int _tuneSecondsLeft;          // > 0 = a CAT tune is running
    private long _tuneStartMs;

    /// <summary>True while a CAT tune (AC111;) is running.</summary>
    public bool CatTuneRunning
    {
        get { lock (_tuneLock) return _tuneSecondsLeft > 0; }
    }

    private void TuneStartLocked()
    {
        _tuneSecondsLeft = TuneSeconds;
        _tuneStartMs = Environment.TickCount64;
        _tuneTimer ??= new Timer(_ => TuneTick(), null, Timeout.Infinite, Timeout.Infinite);
        _tuneTimer.Change(1000, 1000);
        Trace?.Invoke($"CAT tune start (AC111;), {TuneSeconds} s");
        SetTune?.Invoke(true);
    }

    private void TuneStopLocked(string reason, bool releaseTun = true)
    {
        _tuneSecondsLeft = 0;
        _tuneTimer?.Change(Timeout.Infinite, Timeout.Infinite);
        Trace?.Invoke($"CAT tune stop ({reason})");
        if (releaseTun)
            SetTune?.Invoke(false);
    }

    private void TuneTick()
    {
        lock (_tuneLock)
        {
            if (_tuneSecondsLeft <= 0)
                return;
            // Give the UI the first second to switch TUN on before trusting GetTuning.
            if (Environment.TickCount64 - _tuneStartMs > TuneDebounceMs && !GetTuning())
                TuneStopLocked("ended elsewhere", releaseTun: false);
            else if (--_tuneSecondsLeft == 0)
                TuneStopLocked("time up");
        }
    }

    /// <summary>CAT port closing: end a running CAT tune and free the timer.</summary>
    public void CancelTune()
    {
        lock (_tuneLock)
        {
            if (_tuneSecondsLeft > 0)
                TuneStopLocked("CAT closed");
            _tuneTimer?.Dispose();
            _tuneTimer = null;
        }
    }

    /// <summary>
    /// Set on TX; / cleared on RX; before UI/8888 run. WSJT-X polls IF; immediately;
    /// without this latch IF reports RX and WSJT unkeys.
    /// </summary>
    private int _pttLatched;

    /// <summary>
    /// PTT from a serial control line (digi app RTS → our CTS, like ms-sdr PIN=1).
    /// Same effect as TX; / RX;, so IF; reports TX while the line is on.
    /// </summary>
    public void SetLinePtt(bool tx, string line)
    {
        Volatile.Write(ref _pttLatched, tx ? 1 : 0);
        Trace?.Invoke(tx ? $"CAT {line} on → key" : $"CAT {line} off → unkey");
        SetPtt?.Invoke(tx);
    }

    /// <summary>Handle one command including trailing ';'. Returns reply or null.</summary>
    public string? Handle(string command)
    {
        if (string.IsNullOrWhiteSpace(command))
            return null;
        string cmd = command.Trim().ToUpperInvariant();
        if (!cmd.EndsWith(';'))
            cmd += ';';

        if (cmd == "AI;" || cmd.StartsWith("AI", StringComparison.Ordinal) && cmd.Length <= 4)
            return "AI0;";
        if (cmd == "ID;")
            return "ID019;"; // TS-2000
        if (cmd == "PS;")
            return "PS1;";
        if (cmd == "FW;")
            return "FW3000;";
        if (cmd == "KS;")
            return "KS018;";
        if (cmd == "MD;")
            return "MD" + GetModeDigit() + ";";
        if (cmd is "MD1;" or "MD2;" or "MD3;" or "MD4;" or "MD5;" or "MD6;" or "MD9;")
        {
            SetModeDigit?.Invoke(cmd[2]);
            return null;
        }
        if (cmd == "FA;")
            return FormatFa(GetFrequencyHz());
        if (cmd.StartsWith("FA", StringComparison.Ordinal) && cmd.Length > 3)
        {
            if (TryParseFreq(cmd.AsSpan(2), out long hz))
                SetFrequencyHz?.Invoke(hz);
            return null;
        }
        if (cmd == "FB;")
            return FormatFb(GetFrequencyHz());
        if (cmd.StartsWith("FB", StringComparison.Ordinal) && cmd.Length > 3)
        {
            if (TryParseFreq(cmd.AsSpan(2), out long hz))
                SetFrequencyHz?.Invoke(hz);
            return null;
        }
        if (cmd == "IF;")
            return FormatIf();
        if (cmd.StartsWith("RM", StringComparison.Ordinal))
            return FormatRm(cmd);
        if (cmd is "RX;" or "RX0;" or "RX1;")
        {
            Volatile.Write(ref _pttLatched, 0);
            Trace?.Invoke("CAT RX → unkey");
            SetPtt?.Invoke(false);
            // Hamlib TS-2000 does not read after RX/TX; a TX0;/RX0; reply
            // left in the COM buffer is later parsed as the next IF/ID and
            // WSJT-X unkeys. Local ms-sdr can get away with it; we cannot.
            return null;
        }
        if (cmd is "TX;" or "TX0;" or "TX1;" or "TX2;")
        {
            lock (_tuneLock)
            {
                if (_tuneSecondsLeft > 0)
                    TuneStopLocked("CAT TX;");
            }
            Volatile.Write(ref _pttLatched, 1);
            Trace?.Invoke("CAT TX → key");
            SetPtt?.Invoke(true);
            return null;
        }
        if (cmd == "AC111;")
        {
            lock (_tuneLock)
            {
                if (_tuneSecondsLeft > 0 && Environment.TickCount64 - _tuneStartMs < TuneDebounceMs)
                    Trace?.Invoke($"CAT tune. AC111; ignored (too soon, {Environment.TickCount64 - _tuneStartMs} ms after start)");
                else if (_tuneSecondsLeft > 0)
                    TuneStopLocked("second AC111;");
                else
                    TuneStartLocked();
            }
            return null; // ms-sdr sends no reply either
        }
        if (cmd.StartsWith("VX", StringComparison.Ordinal))
            return null; // hamlib PTT ON is "VX0;TX;"
        if (cmd is "FR;" or "FR0;" or "FT;" or "FT0;" or "FT1;")
            return cmd.Length == 3 ? cmd[..2] + "0;" : cmd;
        return null;
    }

    public static char ModeDigitFromName(string mode)
    {
        string m = (mode ?? "").Trim().ToUpperInvariant().Replace('_', '-');
        return m switch
        {
            "LSB" or "DIG-L" or "DIGL" => '1',
            "CW" => '3',
            "FM" => '4',
            "AM" => '5',
            _ => '2', // USB / DIG-U
        };
    }

    public static string ModeNameFromDigit(char d, bool preferDigU)
    {
        return d switch
        {
            '1' => "LSB",
            '3' => "CW",
            '4' => "FM",
            '5' => "AM",
            '2' when preferDigU => "DIG-U",
            _ => "USB",
        };
    }

    private static string FormatFa(long hz) => "FA" + Hz11(hz) + ";";
    private static string FormatFb(long hz) => "FB" + Hz11(hz) + ";";

    private string FormatIf()
    {
        // Match ms-sdr: IF + 11 freq + 15 zeros + TX + MD + 7 zeros + ;
        var sb = new StringBuilder(40);
        sb.Append("IF");
        sb.Append(Hz11(GetFrequencyHz()));
        sb.Append('0', 15);
        // A CAT tune is TX too (ms-sdr: the tune carrier sets TX on).
        bool tx = Volatile.Read(ref _pttLatched) != 0 || GetTransmitting() || CatTuneRunning;
        sb.Append(tx ? '1' : '0');
        sb.Append(GetModeDigit());
        sb.Append('0', 7);
        sb.Append(';');
        return sb.ToString();
    }

    private static string FormatRm(string cmd)
    {
        char meter = (cmd.Length >= 3 && cmd[2] is >= '0' and <= '3') ? cmd[2] : '1';
        int dots = meter == '1' ? 2 : 0;
        return "RM" + meter + dots.ToString("D4", CultureInfo.InvariantCulture) + ";";
    }

    private static string Hz11(long hz)
    {
        if (hz < 0) hz = 0;
        if (hz > 99_999_999_999)
            hz = 99_999_999_999;
        return hz.ToString("D11", CultureInfo.InvariantCulture);
    }

    private static bool TryParseFreq(ReadOnlySpan<char> digitsAndSemi, out long hz)
    {
        hz = 0;
        var s = digitsAndSemi.TrimEnd(';').Trim();
        if (s.IsEmpty)
            return false;
        return long.TryParse(s, NumberStyles.Integer, CultureInfo.InvariantCulture, out hz) && hz > 0;
    }
}
