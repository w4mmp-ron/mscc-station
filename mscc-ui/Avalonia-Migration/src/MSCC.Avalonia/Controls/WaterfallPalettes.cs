namespace MSCC.Avalonia.Controls;

/// <summary>
/// Waterfall color schemes matching original MSCC / WPF S/W palette list.
/// </summary>
public enum WaterfallColorScheme
{
    /// <summary>UI: Red/Yellow — classic WPF heat map (legacy channel order at write site).</summary>
    RedYellow = 0,
    /// <summary>UI: Enhanced — multi-band rainbow.</summary>
    Enhanced = 1,
    /// <summary>UI: Spectran — blue → white.</summary>
    Spectran = 2,
    /// <summary>UI: BlackWhite — grayscale.</summary>
    BlackWhite = 3,
}

/// <summary>
/// Palette names, parsing, and dB→RGB maps (ported from MSCC.Wpf SpectrumDisplayControl).
/// </summary>
public static class WaterfallPalettes
{
    public static readonly string[] Names =
    {
        "Red/Yellow",
        "Enhanced",
        "Spectran",
        "BlackWhite",
    };

    public static string CanonicalName(WaterfallColorScheme scheme) =>
        Names[(int)Math.Clamp((int)scheme, 0, Names.Length - 1)];

    public static WaterfallColorScheme Parse(string? name)
    {
        if (string.IsNullOrWhiteSpace(name))
            return WaterfallColorScheme.Enhanced;

        string n = name.Trim();
        if (int.TryParse(n, out int idx) && idx >= 0 && idx <= 3)
            return (WaterfallColorScheme)idx;

        n = n.Replace(" ", "", StringComparison.Ordinal).Replace("-", "", StringComparison.Ordinal);
        return n.ToUpperInvariant() switch
        {
            "RED/YELLOW" or "REDYELLOW" or "ORIGINAL" or "0" => WaterfallColorScheme.RedYellow,
            "ENHANCED" or "1" => WaterfallColorScheme.Enhanced,
            "SPECTRAN" or "2" => WaterfallColorScheme.Spectran,
            "BLACKWHITE" or "BLACK/WHITE" or "GRAYSCALE" or "GREYSCALE" or "3" => WaterfallColorScheme.BlackWhite,
            _ => WaterfallColorScheme.Enhanced,
        };
    }

    public static string NormalizeName(string? name) => CanonicalName(Parse(name));

    /// <summary>
    /// Map normalized intensity 0…1 (or raw dB for clamp edges) to RGB.
    /// For Enhanced/Spectran/BlackWhite, callers write B,G,R correctly.
    /// For Red/Yellow, WPF used a legacy channel order at the write site — see renderer.
    /// </summary>
    public static void MapColor(
        WaterfallColorScheme scheme,
        float db,
        float minDb,
        float maxDb,
        out byte r,
        out byte g,
        out byte b)
    {
        float range = maxDb - minDb;
        if (range < 0.001f)
            range = 0.001f;

        float norm = Math.Clamp((db - minDb) / range, 0f, 1f);

        switch (scheme)
        {
            case WaterfallColorScheme.Enhanced:
                if (db <= minDb) { r = g = b = 0; return; }
                if (db >= maxDb) { r = 192; g = 124; b = 255; return; }
                GetEnhancedColor(norm, out r, out g, out b);
                break;
            case WaterfallColorScheme.Spectran:
                if (db <= minDb) { r = g = b = 0; return; }
                if (db >= maxDb) { r = g = b = 240; return; }
                GetSpectranColor(norm, out r, out g, out b);
                break;
            case WaterfallColorScheme.BlackWhite:
                if (db <= minDb) { r = g = b = 0; return; }
                if (db >= maxDb) { r = g = b = 255; return; }
                {
                    int v = (int)(norm * 255f);
                    r = g = b = (byte)Math.Clamp(v, 0, 255);
                }
                break;
            case WaterfallColorScheme.RedYellow:
            default:
                GetClassicWpfHeatMapColor(norm, out r, out g, out b);
                break;
        }
    }

    /// <summary>
    /// Exact pre-palette WPF heat map. Written with legacy channel order
    /// (r→B, g→G, b→R in Bgra32) so classic Red/Yellow matches Windows users.
    /// </summary>
    private static void GetClassicWpfHeatMapColor(float norm, out byte r, out byte g, out byte b)
    {
        if (norm < 0.2f)
        {
            float t = norm / 0.2f;
            r = 0; g = 0; b = (byte)(t * 90);
        }
        else if (norm < 0.4f)
        {
            float t = (norm - 0.2f) / 0.2f;
            r = 0; g = (byte)(t * 120); b = (byte)(90 + t * 110);
        }
        else if (norm < 0.6f)
        {
            float t = (norm - 0.4f) / 0.2f;
            r = (byte)(t * 60); g = (byte)(120 + t * 135); b = (byte)(200 * (1 - t));
        }
        else if (norm < 0.8f)
        {
            float t = (norm - 0.6f) / 0.2f;
            r = (byte)(60 + t * 195); g = 255; b = (byte)(200 * (1 - t));
        }
        else
        {
            float t = (norm - 0.8f) / 0.2f;
            r = 255; g = (byte)(255 * (1 - t * 0.6f)); b = (byte)(t * 80);
        }
    }

    /// <summary>Original Enhanced multi-band rainbow (2/9 … 8/9 segments).</summary>
    private static void GetEnhancedColor(float overall, out byte r, out byte g, out byte b)
    {
        if (overall < 2f / 9f)
        {
            float local = overall / (2f / 9f);
            r = 0;
            g = 0;
            b = (byte)(local * 255f);
        }
        else if (overall < 3f / 9f)
        {
            float local = (overall - 2f / 9f) / (1f / 9f);
            r = 0;
            g = (byte)(local * 255f);
            b = 255;
        }
        else if (overall < 4f / 9f)
        {
            float local = (overall - 3f / 9f) / (1f / 9f);
            r = 0;
            g = 255;
            b = (byte)((1f - local) * 255f);
        }
        else if (overall < 5f / 9f)
        {
            float local = (overall - 4f / 9f) / (1f / 9f);
            r = (byte)(local * 255f);
            g = 255;
            b = 0;
        }
        else if (overall < 7f / 9f)
        {
            float local = (overall - 5f / 9f) / (2f / 9f);
            r = 255;
            g = (byte)((1f - local) * 255f);
            b = 0;
        }
        else if (overall < 8f / 9f)
        {
            float local = (overall - 7f / 9f) / (1f / 9f);
            r = 255;
            g = 0;
            b = (byte)(local * 255f);
        }
        else
        {
            float local = (overall - 8f / 9f) / (1f / 9f);
            r = (byte)((0.75f + 0.25f * (1f - local)) * 255f);
            g = (byte)(local * 255f * 0.5f);
            b = 255;
        }
    }

    /// <summary>Original Spectran-style blue → cyan/white ramp.</summary>
    private static void GetSpectranColor(float overall, out byte r, out byte g, out byte b)
    {
        float localPercent = overall * 100f;
        if (localPercent < 51f)
        {
            r = 0;
            g = 0;
            int blue = (int)(localPercent * 5f);
            b = (byte)Math.Clamp(blue, 0, 255);
        }
        else if (localPercent < 66f)
        {
            int rg = (int)((localPercent - 50f) * 2f);
            r = g = (byte)Math.Clamp(rg, 0, 255);
            b = 255;
        }
        else if (localPercent < 77f)
        {
            int rg = (int)((localPercent - 50f) * 3f);
            r = g = (byte)Math.Clamp(rg, 0, 255);
            b = 255;
        }
        else if (localPercent < 88f)
        {
            int rg = (int)((localPercent - 50f) * 4f);
            r = g = (byte)Math.Clamp(rg, 0, 255);
            b = 255;
        }
        else
        {
            int rg = (int)((localPercent - 50f) * 5f);
            r = g = (byte)Math.Clamp(rg, 0, 255);
            b = 255;
        }
    }
}
