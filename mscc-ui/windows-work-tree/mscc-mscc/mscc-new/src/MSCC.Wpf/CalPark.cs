namespace MSCC.Wpf;

/// <summary>
/// Firmware-major folder names for dialogs. The host parks the files (opcode 0x29).
/// </summary>
public static class CalPark
{
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
}
