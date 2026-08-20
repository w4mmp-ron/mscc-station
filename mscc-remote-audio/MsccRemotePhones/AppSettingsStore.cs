using System.Globalization;
using MsccRemotePhones.Audio;
using MsccRemotePhones.Protocol;

namespace MsccRemotePhones;

/// <summary>
/// Client UI settings — %LocalAppData%\MSCC-NET9\MsccRemotePhones.ini
/// (Not remote-phones.ini — that belongs to sdrcore-recv on the Pi.)
/// </summary>
public sealed class AppSettings
{
    public int RxPort { get; set; } = MsccAudioProtocol.DefaultPort;
    public int JitterMs { get; set; } = 80;
    public string PlayDevice { get; set; } = "";
    public int VolumePct { get; set; } = 80;
    public bool Mute { get; set; }
    public bool EqEnabled { get; set; }
    public float EqLowDb { get; set; }
    public float EqMidDb { get; set; }
    public float EqHighDb { get; set; }
    public string TxHost { get; set; } = "127.0.0.1";
    public int TxPort { get; set; } = MsccAudioProtocol.DefaultTxPort;
    public string MicDevice { get; set; } = "";
    public int MicVolumePct { get; set; } = 80;
}

public static class AppSettingsStore
{
    public static string ConfigDirectory
    {
        get
        {
            string dir = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "MSCC-NET9");
            Directory.CreateDirectory(dir);
            return dir;
        }
    }

    public static string ConfigPath => Path.Combine(ConfigDirectory, "MsccRemotePhones.ini");

    public static AppSettings Load()
    {
        var s = new AppSettings();
        try
        {
            string path = ConfigPath;
            if (!File.Exists(path))
                return s;

            foreach (string raw in File.ReadAllLines(path))
            {
                string line = raw.Trim();
                if (line.Length == 0 || line[0] is '#' or ';')
                    continue;
                int eq = line.IndexOf('=');
                if (eq <= 0)
                    continue;
                string k = line[..eq].Trim();
                string v = line[(eq + 1)..].Trim();
                switch (k.ToUpperInvariant())
                {
                    case "RX_PORT":
                        if (int.TryParse(v, out int rp)) s.RxPort = rp;
                        break;
                    case "JITTER_MS":
                        if (int.TryParse(v, out int j)) s.JitterMs = j;
                        break;
                    case "PLAY_DEVICE":
                        s.PlayDevice = v;
                        break;
                    case "VOLUME":
                        if (int.TryParse(v, out int vol)) s.VolumePct = vol;
                        break;
                    case "MUTE":
                        s.Mute = IsTruthy(v);
                        break;
                    case "EQ_ENABLED":
                        s.EqEnabled = IsTruthy(v);
                        break;
                    case "EQ_LOW_DB":
                        if (TryFloat(v, out float lo)) s.EqLowDb = lo;
                        break;
                    case "EQ_MID_DB":
                        if (TryFloat(v, out float mid)) s.EqMidDb = mid;
                        break;
                    case "EQ_HIGH_DB":
                        if (TryFloat(v, out float hi)) s.EqHighDb = hi;
                        break;
                    case "TX_HOST":
                        s.TxHost = v;
                        break;
                    case "TX_PORT":
                        if (int.TryParse(v, out int tp)) s.TxPort = tp;
                        break;
                    case "MIC_DEVICE":
                        s.MicDevice = v;
                        break;
                    case "MIC_VOLUME":
                        if (int.TryParse(v, out int mv)) s.MicVolumePct = mv;
                        break;
                }
            }

            s.RxPort = Clamp(s.RxPort, 1024, 65535);
            s.TxPort = Clamp(s.TxPort, 1024, 65535);
            s.JitterMs = Clamp(s.JitterMs, 20, 300);
            s.VolumePct = Clamp(s.VolumePct, 0, 100);
            s.MicVolumePct = Clamp(s.MicVolumePct, 0, 100);
            s.EqLowDb = Math.Clamp(s.EqLowDb, -PlaybackEq.MaxGainDb, PlaybackEq.MaxGainDb);
            s.EqMidDb = Math.Clamp(s.EqMidDb, -PlaybackEq.MaxGainDb, PlaybackEq.MaxGainDb);
            s.EqHighDb = Math.Clamp(s.EqHighDb, -PlaybackEq.MaxGainDb, PlaybackEq.MaxGainDb);
            if (string.IsNullOrWhiteSpace(s.TxHost))
                s.TxHost = "127.0.0.1";
        }
        catch
        {
            // defaults
        }
        return s;
    }

    public static void Save(AppSettings s)
    {
        try
        {
            var inv = CultureInfo.InvariantCulture;
            File.WriteAllText(ConfigPath,
                "# MsccRemotePhones client settings (Windows)\n" +
                "# Not remote-phones.ini — that file is for sdrcore-recv on the Pi.\n" +
                $"RX_PORT={s.RxPort}\n" +
                $"JITTER_MS={s.JitterMs}\n" +
                $"PLAY_DEVICE={s.PlayDevice}\n" +
                $"VOLUME={s.VolumePct}\n" +
                $"MUTE={(s.Mute ? 1 : 0)}\n" +
                $"EQ_ENABLED={(s.EqEnabled ? 1 : 0)}\n" +
                $"EQ_LOW_DB={s.EqLowDb.ToString("0.##", inv)}\n" +
                $"EQ_MID_DB={s.EqMidDb.ToString("0.##", inv)}\n" +
                $"EQ_HIGH_DB={s.EqHighDb.ToString("0.##", inv)}\n" +
                $"TX_HOST={s.TxHost}\n" +
                $"TX_PORT={s.TxPort}\n" +
                $"MIC_DEVICE={s.MicDevice}\n" +
                $"MIC_VOLUME={s.MicVolumePct}\n");
        }
        catch
        {
            // ignore
        }
    }

    private static bool IsTruthy(string v) =>
        v is "1" or "true" or "TRUE" or "yes" or "YES";

    private static bool TryFloat(string v, out float f) =>
        float.TryParse(v, NumberStyles.Float, CultureInfo.InvariantCulture, out f);

    private static int Clamp(int v, int lo, int hi) => Math.Clamp(v, lo, hi);
}
