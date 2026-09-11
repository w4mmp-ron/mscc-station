using System.Globalization;
using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Media;

namespace MSCC.Avalonia.Controls;

public enum UiChromeRole
{
    WindowBackground,
    ButtonFace,
    PanelBackground,
}

/// <summary>
/// UI chrome + spectrum-background color resolution (WPF UiChromeTheme port).
/// Named RED…BLACK, CUSTOM hex, panel AUTO = lift of window background.
/// </summary>
public static class UiChromeTheme
{
    public static readonly string[] ColorNames =
    {
        "RED", "BLUE", "GREEN", "YELLOW", "WHITE", "BLACK", "CUSTOM"
    };

    public static readonly string[] PanelColorNames =
    {
        "AUTO", "RED", "BLUE", "GREEN", "YELLOW", "WHITE", "BLACK"
    };

    public static readonly string[] SpectrumBackgroundNames =
    {
        "RED", "BLUE", "GREEN", "YELLOW", "WHITE", "BLACK", "CUSTOM"
    };

    /// <summary>WPF FILL list (SCOPE = line only, no under-curve fill).</summary>
    public static readonly string[] SpectrumFillNames =
    {
        "RED", "BLUE", "GREEN", "YELLOW", "WHITE", "BLACK", "SCOPE"
    };

    /// <summary>WPF LINE list — named only (no CUSTOM).</summary>
    public static readonly string[] SpectrumLineNames =
    {
        "RED", "BLUE", "GREEN", "YELLOW", "WHITE", "BLACK"
    };

    public const byte PanelAutoLift = 0x0E;

    public static bool IsCustom(string? name) =>
        string.Equals((name ?? "").Trim(), "CUSTOM", StringComparison.OrdinalIgnoreCase);

    public static string ToHex(byte r, byte g, byte b) => $"#{r:X2}{g:X2}{b:X2}";

    public static string ToHex(Color c) => ToHex(c.R, c.G, c.B);

    public static bool TryParseHex(string? hex, out byte r, out byte g, out byte b)
    {
        r = g = b = 0;
        if (string.IsNullOrWhiteSpace(hex)) return false;
        string s = hex.Trim();
        if (s.StartsWith('#')) s = s[1..];
        if (s.Length == 8) s = s[2..]; // strip AA
        if (s.Length != 6) return false;
        if (!byte.TryParse(s.AsSpan(0, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out r)) return false;
        if (!byte.TryParse(s.AsSpan(2, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out g)) return false;
        if (!byte.TryParse(s.AsSpan(4, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out b)) return false;
        return true;
    }

    public static Color? TryParseHex(string? hex) =>
        TryParseHex(hex, out byte r, out byte g, out byte b) ? Color.FromRgb(r, g, b) : null;

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

    public static Color Lighten(Color c, byte amount = 0x22) =>
        Color.FromRgb(
            (byte)Math.Min(255, c.R + amount),
            (byte)Math.Min(255, c.G + amount),
            (byte)Math.Min(255, c.B + amount));

    public static Color Darken(Color c, byte amount = 0x22) =>
        Color.FromRgb(
            (byte)Math.Max(0, c.R - amount),
            (byte)Math.Max(0, c.G - amount),
            (byte)Math.Max(0, c.B - amount));

    /// <summary>Named or CUSTOM spectrum pane background → BGR bytes for renderer.</summary>
    public static void ResolveSpectrumBackground(
        string? name,
        string? rgbHex,
        out byte r,
        out byte g,
        out byte b)
    {
        if (IsCustom(name) && TryParseHex(rgbHex, out r, out g, out b))
            return;

        string n = (name ?? "BLACK").Trim().ToUpperInvariant();
        switch (n)
        {
            case "RED":
                r = 0xFF; g = 0x00; b = 0x00; break;
            case "BLUE":
                r = 0x00; g = 0x00; b = 0xFF; break;
            case "GREEN":
                r = 0x00; g = 0xFF; b = 0x00; break;
            case "YELLOW":
                r = 0xFF; g = 0xFF; b = 0x00; break;
            case "WHITE":
                r = 0xFF; g = 0xFF; b = 0xFF; break;
            case "BLACK":
            default:
                // Soft dark (better than pure black on spectrum)
                r = 0x08; g = 0x08; b = 0x0E; break;
        }
    }

    /// <summary>WPF SpectrumColorSettings.SetFill RGB (SCOPE uses green for line color).</summary>
    public static void ResolveSpectrumFill(string? name, out byte r, out byte g, out byte b, out bool scopeLineOnly)
    {
        string n = (name ?? "SCOPE").Trim().ToUpperInvariant();
        scopeLineOnly = n is "SCOPE" or "";
        switch (n)
        {
            case "RED":
                r = 0xFF; g = 0x00; b = 0x00; break;
            case "BLUE":
                r = 0x00; g = 0x00; b = 0xFF; break;
            case "GREEN":
                r = 0x00; g = 0x9A; b = 0x30; break;
            case "YELLOW":
                r = 0xFF; g = 0xFF; b = 0x00; break;
            case "WHITE":
                r = 0xFF; g = 0xFF; b = 0xFF; break;
            case "BLACK":
                r = 0x22; g = 0x22; b = 0x22; break;
            case "SCOPE":
            default:
                r = 0x00; g = 0x9A; b = 0x30; break;
        }
    }

    /// <summary>WPF SpectrumColorSettings.SetLine RGB.</summary>
    public static void ResolveSpectrumLine(string? name, out byte r, out byte g, out byte b)
    {
        string n = (name ?? "GREEN").Trim().ToUpperInvariant();
        switch (n)
        {
            case "RED":
                r = 0xFF; g = 0x40; b = 0x40; break;
            case "BLUE":
                r = 0x40; g = 0x40; b = 0xFF; break;
            case "GREEN":
                r = 0x20; g = 0xFF; b = 0x5A; break;
            case "YELLOW":
                r = 0xFF; g = 0xFF; b = 0x40; break;
            case "WHITE":
                r = 0xEE; g = 0xEE; b = 0xEE; break;
            case "BLACK":
                r = 0x44; g = 0x44; b = 0x44; break;
            default:
                r = 0x20; g = 0xFF; b = 0x5A; break;
        }
    }
}

/// <summary>
/// Sticky UI chrome + spectrum background (global — not HF/LF banked).
/// </summary>
public sealed class AppearanceSettings
{
    public static AppearanceSettings Instance { get; } = new();

    public event Action? Changed;

    // Spectrum pane colors (global)
    public string SpectrumBackground { get; private set; } = "BLACK";
    public string SpectrumBackgroundRgb { get; private set; } = "#101018";
    public string SpectrumFill { get; private set; } = "SCOPE";
    public string SpectrumLine { get; private set; } = "GREEN";

    // Window / panel / button
    public string UiBackground { get; private set; } = "BLACK";
    public string UiBackgroundRgb { get; private set; } = "#1C1C1C";
    public string UiButton { get; private set; } = "YELLOW";
    public string UiButtonRgb { get; private set; } = "#FFCC00";
    public string UiPanel { get; private set; } = "AUTO";

    public Color ResolveWindowBackground()
    {
        if (UiChromeTheme.IsCustom(UiBackground))
            return UiChromeTheme.TryParseHex(UiBackgroundRgb)
                   ?? UiChromeTheme.Resolve("BLACK", UiChromeRole.WindowBackground);
        return UiChromeTheme.Resolve(UiBackground, UiChromeRole.WindowBackground);
    }

    public Color ResolveButtonFace()
    {
        if (UiChromeTheme.IsCustom(UiButton))
            return UiChromeTheme.TryParseHex(UiButtonRgb)
                   ?? UiChromeTheme.Resolve("YELLOW", UiChromeRole.ButtonFace);
        return UiChromeTheme.Resolve(UiButton, UiChromeRole.ButtonFace);
    }

    public Color ResolvePanel()
    {
        Color bg = ResolveWindowBackground();
        if (UiChromeTheme.IsCustom(UiBackground))
            return UiChromeTheme.Lighten(bg, UiChromeTheme.PanelAutoLift);

        string n = (UiPanel ?? "AUTO").Trim().ToUpperInvariant();
        if (n is "" or "AUTO")
            return UiChromeTheme.Lighten(bg, UiChromeTheme.PanelAutoLift);
        return UiChromeTheme.Resolve(n, UiChromeRole.PanelBackground);
    }

    public void GetSpectrumBackgroundRgb(out byte r, out byte g, out byte b) =>
        UiChromeTheme.ResolveSpectrumBackground(SpectrumBackground, SpectrumBackgroundRgb, out r, out g, out b);

    public void GetSpectrumFillRgb(out byte r, out byte g, out byte b, out bool scopeLineOnly) =>
        UiChromeTheme.ResolveSpectrumFill(SpectrumFill, out r, out g, out b, out scopeLineOnly);

    public void GetSpectrumLineRgb(out byte r, out byte g, out byte b) =>
        UiChromeTheme.ResolveSpectrumLine(SpectrumLine, out r, out g, out b);

    public void SetSpectrumFill(string name)
    {
        name = (name ?? "SCOPE").Trim().ToUpperInvariant();
        if (!UiChromeTheme.SpectrumFillNames.Contains(name, StringComparer.OrdinalIgnoreCase))
            name = "SCOPE";
        if (string.Equals(SpectrumFill, name, StringComparison.OrdinalIgnoreCase))
            return;
        SpectrumFill = name;
        Notify();
    }

    public void SetSpectrumLine(string name)
    {
        name = (name ?? "GREEN").Trim().ToUpperInvariant();
        if (!UiChromeTheme.SpectrumLineNames.Contains(name, StringComparer.OrdinalIgnoreCase))
            name = "GREEN";
        if (string.Equals(SpectrumLine, name, StringComparison.OrdinalIgnoreCase))
            return;
        SpectrumLine = name;
        Notify();
    }

    public void SetSpectrumBackground(string name, string? rgbHex = null)
    {
        name = (name ?? "BLACK").Trim().ToUpperInvariant();
        if (!UiChromeTheme.SpectrumBackgroundNames.Contains(name, StringComparer.OrdinalIgnoreCase))
            name = "BLACK";
        SpectrumBackground = name;
        if (UiChromeTheme.IsCustom(name) && !string.IsNullOrWhiteSpace(rgbHex))
            SpectrumBackgroundRgb = NormalizeHex(rgbHex!);
        Notify();
    }

    public void SetSpectrumBackgroundRgb(byte r, byte g, byte b)
    {
        SpectrumBackground = "CUSTOM";
        SpectrumBackgroundRgb = UiChromeTheme.ToHex(r, g, b);
        Notify();
    }

    public void SetUiBackground(string name, string? rgbHex = null)
    {
        name = (name ?? "BLACK").Trim().ToUpperInvariant();
        if (name != "CUSTOM" && !UiChromeTheme.ColorNames.Contains(name, StringComparer.OrdinalIgnoreCase))
            name = "BLACK";
        UiBackground = name;
        if (UiChromeTheme.IsCustom(name))
        {
            if (!string.IsNullOrWhiteSpace(rgbHex))
                UiBackgroundRgb = NormalizeHex(rgbHex!);
            UiPanel = "AUTO"; // CUSTOM bg forces AUTO panel
        }
        Notify();
    }

    public void SetUiBackgroundRgb(byte r, byte g, byte b)
    {
        UiBackground = "CUSTOM";
        UiBackgroundRgb = UiChromeTheme.ToHex(r, g, b);
        UiPanel = "AUTO";
        Notify();
    }

    public void SetUiButton(string name, string? rgbHex = null)
    {
        name = (name ?? "YELLOW").Trim().ToUpperInvariant();
        if (name != "CUSTOM" && !UiChromeTheme.ColorNames.Contains(name, StringComparer.OrdinalIgnoreCase))
            name = "YELLOW";
        UiButton = name;
        if (UiChromeTheme.IsCustom(name) && !string.IsNullOrWhiteSpace(rgbHex))
            UiButtonRgb = NormalizeHex(rgbHex!);
        Notify();
    }

    public void SetUiButtonRgb(byte r, byte g, byte b)
    {
        UiButton = "CUSTOM";
        UiButtonRgb = UiChromeTheme.ToHex(r, g, b);
        Notify();
    }

    public void SetUiPanel(string name)
    {
        if (UiChromeTheme.IsCustom(UiBackground))
        {
            UiPanel = "AUTO";
            Notify();
            return;
        }

        name = (name ?? "AUTO").Trim().ToUpperInvariant();
        if (!UiChromeTheme.PanelColorNames.Contains(name, StringComparer.OrdinalIgnoreCase))
            name = "AUTO";
        UiPanel = name;
        Notify();
    }

    public void ResetUiChromeDefaults()
    {
        UiBackground = "BLACK";
        UiBackgroundRgb = "#1C1C1C";
        UiPanel = "AUTO";
        UiButton = "YELLOW";
        UiButtonRgb = "#FFCC00";
        Notify();
    }

    public void LoadFrom(
        string spectrumBackground,
        string spectrumBackgroundRgb,
        string spectrumFill,
        string spectrumLine,
        string uiBackground,
        string uiBackgroundRgb,
        string uiButton,
        string uiButtonRgb,
        string uiPanel)
    {
        SpectrumBackground = string.IsNullOrWhiteSpace(spectrumBackground) ? "BLACK" : spectrumBackground.Trim().ToUpperInvariant();
        SpectrumBackgroundRgb = NormalizeHex(string.IsNullOrWhiteSpace(spectrumBackgroundRgb) ? "#101018" : spectrumBackgroundRgb);
        SpectrumFill = string.IsNullOrWhiteSpace(spectrumFill) ? "SCOPE" : spectrumFill.Trim().ToUpperInvariant();
        if (!UiChromeTheme.SpectrumFillNames.Contains(SpectrumFill, StringComparer.OrdinalIgnoreCase))
            SpectrumFill = "SCOPE";
        SpectrumLine = string.IsNullOrWhiteSpace(spectrumLine) ? "GREEN" : spectrumLine.Trim().ToUpperInvariant();
        if (!UiChromeTheme.SpectrumLineNames.Contains(SpectrumLine, StringComparer.OrdinalIgnoreCase))
            SpectrumLine = "GREEN";
        UiBackground = string.IsNullOrWhiteSpace(uiBackground) ? "BLACK" : uiBackground.Trim().ToUpperInvariant();
        UiBackgroundRgb = NormalizeHex(string.IsNullOrWhiteSpace(uiBackgroundRgb) ? "#1C1C1C" : uiBackgroundRgb);
        UiButton = string.IsNullOrWhiteSpace(uiButton) ? "YELLOW" : uiButton.Trim().ToUpperInvariant();
        UiButtonRgb = NormalizeHex(string.IsNullOrWhiteSpace(uiButtonRgb) ? "#FFCC00" : uiButtonRgb);
        UiPanel = string.IsNullOrWhiteSpace(uiPanel) ? "AUTO" : uiPanel.Trim().ToUpperInvariant();
        if (UiChromeTheme.IsCustom(UiBackground))
            UiPanel = "AUTO";
        Notify();
    }

    /// <summary>Push resolved brushes into Application resources + optional main window.</summary>
    public void ApplyToApplication()
    {
        var app = Application.Current;
        if (app is null) return;

        Color bg = ResolveWindowBackground();
        Color panel = ResolvePanel();
        Color face = ResolveButtonFace();
        Color border = UiChromeTheme.Darken(face, 0x22);
        Color hover = UiChromeTheme.Lighten(face, 0x22);
        Color selected = UiChromeTheme.Darken(face, 0x40);
        Color selectedBorder = UiChromeTheme.Darken(face, 0x55);
        double lum = 0.299 * face.R + 0.587 * face.G + 0.114 * face.B;
        Color text = lum > 140 ? Colors.Black : Colors.White;
        double panelLum = 0.299 * panel.R + 0.587 * panel.G + 0.114 * panel.B;
        Color primaryText = panelLum > 140 ? Colors.Black : Colors.White;
        Color mutedText = panelLum > 140
            ? Color.FromRgb(0x44, 0x44, 0x44)
            : Color.FromRgb(0xD0, 0xD0, 0xD0);

        SetBrush(app, "UiWindowBackgroundBrush", bg);
        SetBrush(app, "UiPanelBackgroundBrush", panel);
        SetBrush(app, "UiPrimaryTextBrush", primaryText);
        SetBrush(app, "UiMutedTextBrush", mutedText);
        SetBrush(app, "UiButtonFaceBrush", face);
        SetBrush(app, "UiButtonBorderBrush", border);
        SetBrush(app, "UiButtonHoverBrush", hover);
        SetBrush(app, "UiButtonTextBrush", text);
        SetBrush(app, "UiButtonSelectedBrush", selected);
        SetBrush(app, "UiButtonSelectedBorderBrush", selectedBorder);

        // Update open windows (DynamicResource may lag on Window.Background)
        if (app.ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
        {
            var brush = new SolidColorBrush(bg);
            if (desktop.MainWindow is not null)
                desktop.MainWindow.Background = brush;
            foreach (var w in desktop.Windows)
            {
                if (!ReferenceEquals(w, desktop.MainWindow))
                    w.Background = brush;
            }
        }
    }

    private static void SetBrush(Application app, string key, Color color)
    {
        app.Resources[key] = new SolidColorBrush(color);
    }

    private static string NormalizeHex(string hex)
    {
        if (UiChromeTheme.TryParseHex(hex, out byte r, out byte g, out byte b))
            return UiChromeTheme.ToHex(r, g, b);
        return "#1C1C1C";
    }

    private void Notify()
    {
        ApplyToApplication();
        Changed?.Invoke();
    }
}
