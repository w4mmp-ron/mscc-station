namespace MSCC.Avalonia.RemoteAudio;

internal static class LinuxDigitalIni
{
    public static string ConfigDir =>
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".local", "mscc");

    public static string DigitalSpeaker => Read("digital-speaker.ini");
    public static string DigitalMic => Read("digital-microphone.ini");
    public static string OperatorSpeaker => Read("operator-speaker.ini");
    public static string OperatorMic => Read("operator-microphone.ini");

    /// <summary>ms-sdr CAT device from comm-port.ini (usually /dev/tnt0).</summary>
    public static string CommPortName()
    {
        string raw = Read("comm-port.ini");
        if (string.IsNullOrWhiteSpace(raw))
            return "";
        const string key = "COMM_PORT_NAME=";
        int i = raw.IndexOf(key, StringComparison.OrdinalIgnoreCase);
        if (i < 0)
            return raw.Trim();
        string rest = raw[(i + key.Length)..];
        int comma = rest.IndexOf(',');
        int semi = rest.IndexOf(';');
        int cut = rest.Length;
        if (comma >= 0) cut = Math.Min(cut, comma);
        if (semi >= 0) cut = Math.Min(cut, semi);
        return rest[..cut].Trim();
    }

    /// <summary>Other tty0tty end for WSJT-X / flrig (tnt0 ↔ tnt1).</summary>
    public static string CommPortAppEnd(string? radioEnd)
    {
        string p = (radioEnd ?? "").Trim();
        if (p.EndsWith("tnt0", StringComparison.OrdinalIgnoreCase))
            return p[..^1] + "1";
        if (p.EndsWith("tnt1", StringComparison.OrdinalIgnoreCase))
            return p[..^1] + "0";
        return "(other end of the CAT pair)";
    }

    public static string ToMatchKey(string fullName)
    {
        if (string.IsNullOrWhiteSpace(fullName))
            return "";
        int paren = fullName.IndexOf('(');
        string key = paren >= 0 ? fullName[..paren] : fullName;
        return key.TrimEnd();
    }

    private static string Read(string file)
    {
        try
        {
            string path = Path.Combine(ConfigDir, file);
            if (!File.Exists(path))
                return "";
            return File.ReadAllText(path).Trim();
        }
        catch
        {
            return "";
        }
    }
}
