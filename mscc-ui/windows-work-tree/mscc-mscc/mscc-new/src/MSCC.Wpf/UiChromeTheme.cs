using System;
using System.Globalization;
using System.Windows;
using System.Windows.Media;

namespace MSCC.Wpf;

/// <summary>
/// UI chrome colors: named RED…BLACK, CUSTOM (hex RGB for bg/button), AUTO panel (lift of bg).
/// Plain strings in INI for Avalonia later.
/// </summary>
public static class UiChromeTheme
{
    public static readonly string[] ColorNames =
    {
        "RED", "BLUE", "GREEN", "YELLOW", "WHITE", "BLACK"
    };

    /// <summary>Panel: AUTO or named only (no CUSTOM — custom bg always drives panel lighter).</summary>
    public static readonly string[] PanelColorNames =
    {
        "AUTO", "RED", "BLUE", "GREEN", "YELLOW", "WHITE", "BLACK"
    };

    /// <summary>Accent: AUTO (from background), named, or CUSTOM hex. GREEN = mint #00FFAA.</summary>
    public static readonly string[] AccentColorNames =
    {
        "AUTO", "RED", "BLUE", "GREEN", "YELLOW", "WHITE", "BLACK", "CUSTOM"
    };

    public static readonly Color DefaultMint = Color.FromRgb(0x00, 0xFF, 0xAA);

    public const byte PanelAutoLift = 0x0E;

    public static bool IsCustom(string? name) =>
        string.Equals((name ?? "").Trim(), "CUSTOM", StringComparison.OrdinalIgnoreCase);

    public static string ToHex(Color c) =>
        $"#{c.R:X2}{c.G:X2}{c.B:X2}";

    public static Color? TryParseHex(string? hex)
    {
        if (string.IsNullOrWhiteSpace(hex)) return null;
        string s = hex.Trim();
        if (s.StartsWith('#')) s = s[1..];
        if (s.Length == 8) s = s[2..]; // strip AA if present
        if (s.Length != 6) return null;
        if (!byte.TryParse(s.AsSpan(0, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out byte r)) return null;
        if (!byte.TryParse(s.AsSpan(2, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out byte g)) return null;
        if (!byte.TryParse(s.AsSpan(4, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out byte b)) return null;
        return Color.FromRgb(r, g, b);
    }

    /// <summary>
    /// Resolve a named color for UI chrome (not CUSTOM — use ResolveBackground/ResolveButton).
    /// </summary>
    public static Color Resolve(string? name, UiChromeRole role)
    {
        string n = (name ?? "").Trim().ToUpperInvariant();
        return n switch
        {
            "RED" => Color.FromRgb(0xFF, 0x00, 0x00),
            "BLUE" => Color.FromRgb(0x00, 0x00, 0xFF),
            "GREEN" => role == UiChromeRole.Accent
                ? DefaultMint
                : Color.FromRgb(0x00, 0xFF, 0x00),
            "YELLOW" => role == UiChromeRole.ButtonFace
                ? Color.FromRgb(0xFF, 0xCC, 0x00)
                : Color.FromRgb(0xFF, 0xFF, 0x00),
            "WHITE" => Color.FromRgb(0xFF, 0xFF, 0xFF),
            "BLACK" => role switch
            {
                UiChromeRole.ButtonFace => Color.FromRgb(0x33, 0x33, 0x33),
                UiChromeRole.PanelBackground => Color.FromRgb(0x25, 0x25, 0x25),
                _ => Color.FromRgb(0x1C, 0x1C, 0x1C),
            },
            _ => role == UiChromeRole.ButtonFace
                ? Color.FromRgb(0xFF, 0xCC, 0x00)
                : role == UiChromeRole.PanelBackground
                    ? Color.FromRgb(0x25, 0x25, 0x25)
                    : role == UiChromeRole.Accent
                        ? DefaultMint
                        : Color.FromRgb(0x1C, 0x1C, 0x1C),
        };
    }

    public static Color ResolveBackground()
    {
        if (IsCustom(SpectrumWaterfallSettings.UiBackground))
            return TryParseHex(SpectrumWaterfallSettings.UiBackgroundRgb)
                   ?? Resolve("BLACK", UiChromeRole.WindowBackground);
        return Resolve(SpectrumWaterfallSettings.UiBackground, UiChromeRole.WindowBackground);
    }

    public static Color ResolveButtonFace()
    {
        if (IsCustom(SpectrumWaterfallSettings.UiButton))
            return TryParseHex(SpectrumWaterfallSettings.UiButtonRgb)
                   ?? Resolve("YELLOW", UiChromeRole.ButtonFace);
        return Resolve(SpectrumWaterfallSettings.UiButton, UiChromeRole.ButtonFace);
    }

    /// <summary>
    /// Panel color. If background is CUSTOM, always AUTO-lift of that bg (panel list ignored).
    /// Else AUTO lifts named/custom bg; named panel uses fixed map.
    /// </summary>
    public static Color ResolvePanel(Color windowBackground)
    {
        if (IsCustom(SpectrumWaterfallSettings.UiBackground))
            return Lighten(windowBackground, PanelAutoLift);

        string n = (SpectrumWaterfallSettings.UiPanel ?? "AUTO").Trim().ToUpperInvariant();
        if (n is "" or "AUTO")
            return Lighten(windowBackground, PanelAutoLift);
        return Resolve(n, UiChromeRole.PanelBackground);
    }

    /// <summary>
    /// Accent for operate chrome (headers, readouts, VFO borders).
    /// AUTO: mint on dark/gray backgrounds; darker teal on light; hue-shifted if bg is already green.
    /// </summary>
    public static Color ResolveAccent(Color windowBackground)
    {
        string n = (SpectrumWaterfallSettings.UiAccent ?? "AUTO").Trim().ToUpperInvariant();
        if (n is "" or "AUTO")
            return AutoAccentFromBackground(windowBackground);
        if (IsCustom(n))
            return TryParseHex(SpectrumWaterfallSettings.UiAccentRgb) ?? DefaultMint;
        return Resolve(n, UiChromeRole.Accent);
    }

    public static Color AutoAccentFromBackground(Color bg)
    {
        double lum = (0.299 * bg.R + 0.587 * bg.G + 0.114 * bg.B) / 255.0;
        int max = Math.Max(bg.R, Math.Max(bg.G, bg.B));
        int min = Math.Min(bg.R, Math.Min(bg.G, bg.B));
        bool gray = (max - min) < 22;

        if (gray)
            return lum > 0.55 ? Color.FromRgb(0x00, 0x6B, 0x5A) : DefaultMint;

        RgbToHsl(bg, out double h, out _, out _);
        const double mintHue = 160.0;
        double dist = Math.Abs(h - mintHue);
        if (dist > 180) dist = 360 - dist;
        double nh = dist < 35 ? (h + 180) % 360 : mintHue;
        double nl = lum > 0.55 ? 0.36 : 0.56;
        return HslToRgb(nh, 0.85, nl);
    }

    private static void RgbToHsl(Color c, out double h, out double s, out double l)
    {
        double r = c.R / 255.0, g = c.G / 255.0, b = c.B / 255.0;
        double max = Math.Max(r, Math.Max(g, b)), min = Math.Min(r, Math.Min(g, b));
        l = (max + min) / 2.0;
        if (max == min)
        {
            h = s = 0;
            return;
        }
        double d = max - min;
        s = l > 0.5 ? d / (2.0 - max - min) : d / (max + min);
        if (max == r) h = ((g - b) / d + (g < b ? 6 : 0)) * 60.0;
        else if (max == g) h = ((b - r) / d + 2) * 60.0;
        else h = ((r - g) / d + 4) * 60.0;
    }

    private static Color HslToRgb(double h, double s, double l)
    {
        h = ((h % 360) + 360) % 360;
        double c = (1 - Math.Abs(2 * l - 1)) * s;
        double x = c * (1 - Math.Abs((h / 60.0) % 2 - 1));
        double m = l - c / 2;
        double r, g, b;
        if (h < 60) { r = c; g = x; b = 0; }
        else if (h < 120) { r = x; g = c; b = 0; }
        else if (h < 180) { r = 0; g = c; b = x; }
        else if (h < 240) { r = 0; g = x; b = c; }
        else if (h < 300) { r = x; g = 0; b = c; }
        else { r = c; g = 0; b = x; }
        return Color.FromRgb(
            (byte)Math.Clamp((int)Math.Round((r + m) * 255), 0, 255),
            (byte)Math.Clamp((int)Math.Round((g + m) * 255), 0, 255),
            (byte)Math.Clamp((int)Math.Round((b + m) * 255), 0, 255));
    }

    public static Color Lighten(Color c, byte amount = 0x22)
    {
        return Color.FromRgb(
            (byte)Math.Min(255, c.R + amount),
            (byte)Math.Min(255, c.G + amount),
            (byte)Math.Min(255, c.B + amount));
    }

    public static Color Darken(Color c, byte amount = 0x22)
    {
        return Color.FromRgb(
            (byte)Math.Max(0, c.R - amount),
            (byte)Math.Max(0, c.G - amount),
            (byte)Math.Max(0, c.B - amount));
    }

    public static void ApplyToMainWindow()
    {
        if (Application.Current?.MainWindow is not MainWindow main)
            return;
        main.ApplyUiChromeTheme();
    }
}

public enum UiChromeRole
{
    WindowBackground,
    ButtonFace,
    PanelBackground,
    Accent,
}
