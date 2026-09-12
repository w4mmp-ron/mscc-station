namespace MSCC.Avalonia.RemoteAudio;

internal static class LinuxDigitalIni
{
    public static string ConfigDir =>
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".local", "mscc");

    public static string DigitalSpeaker => Read("digital-speaker.ini");
    public static string DigitalMic => Read("digital-microphone.ini");

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
