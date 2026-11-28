using System.Globalization;
using System.Reflection;
using System.Text;
using MSCC.Core.Logging;

namespace MSCC.Core.Domain;

/// <summary>
/// PowerSDR US band text (Assets/bandplan-us.csv, embedded; ported from Avalonia BandPlanService).
/// Spot rows win; otherwise the narrowest covering range; otherwise "Out of Band". hz &lt;= 0 is blank.
/// </summary>
public static class BandPlan
{
    private const string ResourceName = "MSCC.Core.bandplan-us.csv";

    private static readonly object Gate = new();
    private static BandRow[]? _rows;
    private static bool _loadFailed;

    public static string Describe(long hz)
    {
        if (hz <= 0)
            return "";

        BandRow[]? rows = Rows();
        if (rows == null || rows.Length == 0)
            return "";

        string? spot = null;
        string? bestName = null;
        long bestWidth = long.MaxValue;
        foreach (BandRow row in rows)
        {
            if (row.Low == hz && row.High == hz)
            {
                spot ??= row.Name;
                continue;
            }

            if (row.Low <= hz && hz <= row.High)
            {
                long width = row.High - row.Low;
                if (width < bestWidth)
                {
                    bestWidth = width;
                    bestName = row.Name;
                }
            }
        }

        return spot ?? bestName ?? "Out of Band";
    }

    private static BandRow[]? Rows()
    {
        if (_rows != null || _loadFailed)
            return _rows;

        lock (Gate)
        {
            if (_rows != null || _loadFailed)
                return _rows;

            try
            {
                _rows = Load();
            }
            catch (Exception ex)
            {
                _loadFailed = true;
                _rows = null;
                try { DebugMonitor.MonitorTextBoxText("Band plan failed to load: " + ex.Message); }
                catch { /* logging must not escape Describe */ }
            }

            return _rows;
        }
    }

    private static BandRow[] Load()
    {
        using Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(ResourceName)
            ?? throw new InvalidOperationException(ResourceName + " not embedded");
        using var reader = new StreamReader(stream);
        var rows = new List<BandRow>(480);
        string? line;
        bool header = true;
        while ((line = reader.ReadLine()) != null)
        {
            if (line.Length == 0)
                continue;
            if (header)
            {
                header = false;
                continue;
            }

            List<string> fields = ParseCsvLine(line);
            if (fields.Count < 4)
                continue;
            if (!long.TryParse(fields[0], NumberStyles.Integer, CultureInfo.InvariantCulture, out long low))
                continue;
            if (!long.TryParse(fields[1], NumberStyles.Integer, CultureInfo.InvariantCulture, out long high))
                continue;
            if (!int.TryParse(fields[3], NumberStyles.Integer, CultureInfo.InvariantCulture, out _))
                continue;
            string name = fields[2].Trim();
            if (name.Length == 0)
                continue;
            rows.Add(new BandRow(low, high, name));
        }

        return rows.ToArray();
    }

    /// <summary>RFC 4180. Quoted fields may contain commas.</summary>
    private static List<string> ParseCsvLine(string line)
    {
        var fields = new List<string>(4);
        var sb = new StringBuilder();
        bool inQuotes = false;
        for (int i = 0; i < line.Length; i++)
        {
            char c = line[i];
            if (inQuotes)
            {
                if (c == '"')
                {
                    if (i + 1 < line.Length && line[i + 1] == '"')
                    {
                        sb.Append('"');
                        i++;
                    }
                    else
                    {
                        inQuotes = false;
                    }
                }
                else
                {
                    sb.Append(c);
                }
            }
            else if (c == '"')
            {
                inQuotes = true;
            }
            else if (c == ',')
            {
                fields.Add(sb.ToString());
                sb.Clear();
            }
            else if (c != '\r')
            {
                sb.Append(c);
            }
        }

        fields.Add(sb.ToString());
        return fields;
    }

    private readonly record struct BandRow(long Low, long High, string Name);
}
