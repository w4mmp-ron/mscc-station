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
            "GREEN" => Color.FromRgb(0x00, 0xFF, 0x00),
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
}
