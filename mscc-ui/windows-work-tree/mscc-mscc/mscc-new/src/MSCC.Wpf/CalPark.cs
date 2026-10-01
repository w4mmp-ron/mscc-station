using System.IO;

namespace MSCC.Wpf;

/// <summary>
/// Save settings: copy the current radio's live TX IQ, QRP cal, and amplifier cal
/// into %LocalAppData%\MSCC-NET9\cal\&lt;line&gt;\. No protocol change.
/// </summary>
public static class CalPark
{
    public static readonly string[] ParkFiles = { "iq.ini", "power_cal.ini", "amplifier_cal.ini" };

    /// <summary>Folder name under cal\. Unknown majors are refused (no proficio-mkii fallback).</summary>
    public static bool TryLineFromMajor(int major, out string line)
    {
        line = major switch
        {
            1 => "proficio-legacy",
            2 => "geminus-mkii",
            3 or 4 => "proficio-mkii",
            5 => "geminus-legacy",
            6 => "ultimus-legacy",
            7 or 8 => "ultimus-mkii",
            _ => ""
        };
        return line.Length > 0;
    }

    /// <summary>
    /// Copy whichever of the park files exist. Missing files are skipped.
    /// Writes cal\LAST_LINE.txt. Returns the copied names, or null on failure.
    /// </summary>
    public static string[]? SaveLiveToParked(string line, out string error)
    {
        error = "";
        if (string.IsNullOrWhiteSpace(line))
        {
            error = "Radio line is unknown.";
            return null;
        }

        string live = ConfigBootstrap.ConfigDirectory;
        string destDir = Path.Combine(live, "cal", line);
        try
        {
            Directory.CreateDirectory(destDir);
            var copied = new List<string>();
            foreach (string name in ParkFiles)
            {
                string src = Path.Combine(live, name);
                if (!File.Exists(src))
                    continue;
                File.Copy(src, Path.Combine(destDir, name), overwrite: true);
                copied.Add(name);
            }
            File.WriteAllText(Path.Combine(live, "cal", "LAST_LINE.txt"), line + "\n");
            return copied.ToArray();
        }
        catch (Exception ex)
        {
            error = ex.Message;
            return null;
        }
    }
}
