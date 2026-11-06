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
    public Action<string>? Trace { get; set; }

    /// <summary>
    /// Set on TX; / cleared on RX; before UI/8888 run. WSJT-X polls IF; immediately;
    /// without this latch IF reports RX and WSJT unkeys.
    /// </summary>
    private int _pttLatched;

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
            Volatile.Write(ref _pttLatched, 1);
            Trace?.Invoke("CAT TX → key");
            SetPtt?.Invoke(true);
            return null;
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
        bool tx = Volatile.Read(ref _pttLatched) != 0 || GetTransmitting();
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
