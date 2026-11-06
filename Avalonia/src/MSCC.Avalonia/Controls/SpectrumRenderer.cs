using MSCC.Core.Display;

namespace MSCC.Avalonia.Controls;

/// <summary>
/// Spectrum + waterfall pixel renderer (BGRA).
/// Uses <see cref="SpectrumDisplaySettings"/> for cal, grid, zoom, waterfall window.
/// </summary>
internal static class SpectrumRenderer
{
    public static float ToDisplayDb(float rawMappedDb, float dbOffset) => rawMappedDb + dbOffset;

    /// <summary>
    /// cmd-068: largest value in data[i0 .. i1-1] (at least data[i0]). Indices are clamped.
    /// One pixel covers several points when points &gt; pixels; drawing only one of them
    /// made a carrier dip 10-15 dB while tuning.
    /// </summary>
    private static float MaxInRange(float[] data, int i0, int i1)
    {
        int n = data.Length;
        i0 = Math.Clamp(i0, 0, n - 1);
        i1 = Math.Clamp(i1, i0 + 1, n);
        float m = data[i0];
        for (int i = i0 + 1; i < i1; i++)
            if (data[i] > m) m = data[i];
        return m;
    }

    public static void Render(
        SpectrumUpdate update,
        int width,
        int height,
        byte[] bgra,
        IReadOnlyList<float[]>? waterfallHistory,
        SpectrumDisplaySettings? settings = null,
        IReadOnlyList<string?>? waterfallTimeLabels = null)
    {
        settings ??= SpectrumDisplaySettings.Instance;

        AppearanceSettings.Instance.GetSpectrumBackgroundRgb(out byte bgR, out byte bgG, out byte bgB);
        for (int i = 0; i < bgra.Length; i += 4)
        {
            bgra[i + 0] = bgB;
            bgra[i + 1] = bgG;
            bgra[i + 2] = bgR;
            bgra[i + 3] = 0xFF;
        }

        float[] data = update.Data;
        if (data.Length == 0 || width <= 0 || height <= 0) return;

        int fullSpan = update.SpanHz > 0 ? update.SpanHz : SpectrumUpdate.DefaultPanadapterSpanHz;

        // CW pitch: shift FFT bins so pitch-offset LO lines up under the VFO cursor (WPF)
        int cwOffsetBins = 0;
        if (update.CwPitchHz != 0 && fullSpan > 0 && data.Length > 1)
            cwOffsetBins = (int)(update.CwPitchHz * (double)data.Length / fullSpan);

        // LO null is handled in sdrcore-recv (server) — not client-side.

        // WPF port: GRID MAX is the top; BASELINE fine-tunes the floor
        settings.GetDisplayGridWindow(out float minDb, out float maxDb);
        float range = Math.Max(0.001f, maxDb - minDb);
        float dbOffset = settings.SpectrumDbOffset;
        int stride = width * 4;

        bool showWf = settings.ShowWaterfall && waterfallHistory is { Count: > 0 };
        const int freqMargin = 22;
        int spectrumHeight = showWf
            ? Math.Max(50, (int)(height * 0.40))
            : Math.Max(50, height - freqMargin);
        int wfStartY = showWf ? spectrumHeight + 2 : height;
        int wfHeight = showWf ? Math.Max(24, height - wfStartY - freqMargin) : 0;
        int plotBottom = spectrumHeight;

        double zoom = Math.Clamp(settings.ZoomFactor, 1.0, 4.0);
        double viewStart = 0.5 * (1.0 - 1.0 / zoom);
        double viewWidth = 1.0 / zoom;

        // dB grid
        if (settings.ViewGrid)
        {
            for (int db = (int)Math.Floor(maxDb / 10f) * 10; db >= (int)minDb; db -= 20)
            {
                float norm = (db - minDb) / range;
                int y = (int)((1 - norm) * plotBottom);
                if (y < 0 || y >= plotBottom) continue;
                for (int x = 0; x < width; x++)
                {
                    int o = y * stride + x * 4;
                    if (o + 3 >= bgra.Length) break;
                    bgra[o + 0] = 0x40;
                    bgra[o + 1] = 0x40;
                    bgra[o + 2] = 0x50;
                    bgra[o + 3] = 0xFF;
                }
            }
        }

        // Spectrum fill + line (independent colors; SCOPE = line only — WPF)
        var appearance = AppearanceSettings.Instance;
        appearance.GetSpectrumFillRgb(
            out byte fillR, out byte fillG, out byte fillB, out bool scopeLineOnly);
        appearance.GetSpectrumLineRgb(out byte lineR, out byte lineG, out byte lineB);

        for (int x = 0; x < width; x++)
        {
            double t = width > 1 ? (double)x / (width - 1) : 0.5;
            double dataFrac = viewStart + t * viewWidth;
            int idx = (int)Math.Round(dataFrac * (data.Length - 1) + cwOffsetBins);
            idx = Math.Clamp(idx, 0, data.Length - 1);
            // cmd-068: max of all points up to (not incl.) the next pixel's first point.
            int idxNext = idx;
            if (x + 1 < width)
            {
                double dataFracNext = viewStart + ((double)(x + 1) / (width - 1)) * viewWidth;
                idxNext = (int)Math.Round(dataFracNext * (data.Length - 1) + cwOffsetBins);
            }

            float dbv = ToDisplayDb(MaxInRange(data, idx, idxNext), dbOffset);
            float norm = Math.Clamp((dbv - minDb) / range, 0f, 1f);
            int y = (int)((1 - norm) * plotBottom);
            y = Math.Clamp(y, 0, plotBottom);

            if (!scopeLineOnly)
            {
                for (int yy = y; yy < plotBottom; yy++)
                {
                    int o = yy * stride + x * 4;
                    if (o + 3 >= bgra.Length) break;
                    bgra[o + 0] = fillB;
                    bgra[o + 1] = fillG;
                    bgra[o + 2] = fillR;
                    bgra[o + 3] = 0xFF;
                }
            }

            if (y >= 0 && y < plotBottom)
            {
                int o = y * stride + x * 4;
                bgra[o + 0] = lineB;
                bgra[o + 1] = lineG;
                bgra[o + 2] = lineR;
                bgra[o + 3] = 0xFF;
            }
        }

        // Center tune line
        int cx = width / 2;
        for (int yy = 0; yy < plotBottom; yy++)
        {
            int o = yy * stride + cx * 4;
            if (o + 3 >= bgra.Length) break;
            bgra[o + 0] = 0x40;
            bgra[o + 1] = 0xA0;
            bgra[o + 2] = 0xFF;
            bgra[o + 3] = 0xEE;
        }

        // Filter passband — relative Hz, scaled to visible span
        int visibleSpan = Math.Max(1, (int)Math.Round(fullSpan / zoom));
        if (update.FilterLowHz != 0 || update.FilterHighHz != 0)
        {
            double hzPerPx = visibleSpan / (double)Math.Max(1, width);
            int x0 = (int)Math.Round(update.FilterLowHz / hzPerPx + width / 2.0);
            int x1 = (int)Math.Round(update.FilterHighHz / hzPerPx + width / 2.0);
            int left = Math.Clamp(Math.Min(x0, x1), 0, width - 1);
            int right = Math.Clamp(Math.Max(x0, x1), 0, width - 1);
            if (right < left + 1) right = left + 1;

            for (int x = left; x <= right; x++)
            {
                for (int yy = 0; yy < plotBottom; yy++)
                {
                    int o = yy * stride + x * 4;
                    if (o + 3 >= bgra.Length) continue;
                    bgra[o + 0] = (byte)Math.Min(255, bgra[o + 0] + 30);
                    bgra[o + 1] = (byte)Math.Min(255, bgra[o + 1] + 40);
                    bgra[o + 2] = (byte)Math.Min(255, bgra[o + 2] + 70);
                }
            }

            DrawVLine(bgra, stride, width, plotBottom, left, 0x20, 0xB0, 0xFF);
            DrawVLine(bgra, stride, width, plotBottom, right, 0x20, 0xB0, 0xFF);
        }

        // WPF port: dB numbers on the left (S/W dB LABELS), independent of VIEW GRID
        if (settings.ViewDbLabels)
            DrawDbScaleLabels(bgra, stride, width, plotBottom, minDb, maxDb, range);

        if (showWf)
        {
            int ysep = wfStartY - 1;
            for (int x = 0; x < width; x++)
            {
                int o = ysep * stride + x * 4;
                if (o + 3 >= bgra.Length) break;
                bgra[o + 0] = 0x55;
                bgra[o + 1] = 0x55;
                bgra[o + 2] = 0x66;
                bgra[o + 3] = 0xFF;
            }
        }

        if (showWf && waterfallHistory != null && wfHeight > 16)
        {
            int numLines = waterfallHistory.Count;
            int lineLen = waterfallHistory[0].Length;
            float wfLow = settings.WaterfallLowDb;
            float wfHigh = settings.WaterfallHighDb;
            var scheme = WaterfallPalettes.Parse(settings.WaterfallPalette);
            bool redYellowLegacy = scheme == WaterfallColorScheme.RedYellow;

            // Normal: oldest (index 0) at top → newest at bottom.
            // Reverse: newest at top → oldest at bottom.
            bool directionNormal = settings.WaterfallDirectionNormal;

            for (int y = wfStartY; y < height - freqMargin; y++)
            {
                double frac = (double)(y - wfStartY) / Math.Max(1, wfHeight - 1);
                frac = Math.Clamp(frac, 0.0, 1.0);
                double lineIdxD = directionNormal
                    ? frac * (numLines - 1)
                    : (1.0 - frac) * (numLines - 1);
                int li = (int)Math.Round(lineIdxD);
                li = Math.Clamp(li, 0, numLines - 1);
                float[] line = waterfallHistory[li];

                for (int x = 0; x < width; x++)
                {
                    double t = width > 1 ? (double)x / (width - 1) : 0.5;
                    double dataFrac = viewStart + t * viewWidth;
                    int idx = (int)Math.Round(dataFrac * (lineLen - 1) + cwOffsetBins);
                    idx = Math.Clamp(idx, 0, lineLen - 1);
                    // cmd-068: max of the row's points up to (not incl.) the next pixel's first point.
                    int idxNext = idx;
                    if (x + 1 < width)
                    {
                        double dataFracNext = viewStart + ((double)(x + 1) / (width - 1)) * viewWidth;
                        idxNext = (int)Math.Round(dataFracNext * (lineLen - 1) + cwOffsetBins);
                    }

                    float dbv = ToDisplayDb(MaxInRange(line, idx, idxNext), dbOffset);
                    settings.ApplyWaterfallGainZero(dbv, wfLow, wfHigh, out float adjDb, out float adjLow, out float adjHigh);
                    WaterfallPalettes.MapColor(scheme, adjDb, adjLow, adjHigh, out byte wr, out byte wg, out byte wb);

                    int o = y * stride + x * 4;
                    if (o + 3 >= bgra.Length) continue;
                    // Bgra32: correct B,G,R — except Red/Yellow keeps WPF legacy channel order
                    if (redYellowLegacy)
                    {
                        bgra[o + 0] = wr;
                        bgra[o + 1] = wg;
                        bgra[o + 2] = wb;
                    }
                    else
                    {
                        bgra[o + 0] = wb;
                        bgra[o + 1] = wg;
                        bgra[o + 2] = wr;
                    }
                    bgra[o + 3] = 0xFF;
                }
            }

            // WPF port: time marks — white line + UTC mm:ss, scrolling with the history
            if (waterfallTimeLabels != null && waterfallTimeLabels.Count > 0)
            {
                int markCount = Math.Min(numLines, waterfallTimeLabels.Count);
                const int timeLabelScale = 2;
                int x0 = 4 + 5 * (3 * timeLabelScale + 2) + 8;   // after the mm:ss label
                for (int li = 0; li < markCount; li++)
                {
                    string? label = waterfallTimeLabels[li];
                    if (label == null) continue;
                    int y;
                    if (numLines <= 1)
                        y = wfStartY;
                    else
                    {
                        double fracLi = (double)li / (numLines - 1);
                        if (!directionNormal)
                            fracLi = 1.0 - fracLi;
                        y = wfStartY + (int)Math.Round(fracLi * (wfHeight - 1));
                    }
                    y = Math.Clamp(y, wfStartY, height - freqMargin - 1);
                    for (int x = x0; x < width; x++)
                    {
                        int o = y * stride + x * 4;
                        if (o + 3 >= bgra.Length) break;
                        bgra[o + 0] = 0xFF;
                        bgra[o + 1] = 0xFF;
                        bgra[o + 2] = 0xFF;
                        bgra[o + 3] = 0xFF;
                    }
                    int labelH = 5 * timeLabelScale;
                    int labelY = Math.Clamp(y - labelH / 2, wfStartY, height - freqMargin - labelH);
                    DrawTimeLabel(bgra, stride, width, height, 4, labelY, label.Length > 0 ? label : ":", timeLabelScale);
                }
            }
        }

        DrawFreqScale(bgra, stride, width, height, freqMargin, zoom);
    }

    private static void DrawVLine(byte[] buf, int stride, int width, int height, int x, byte b, byte g, byte r)
    {
        if (x < 0 || x >= width) return;
        for (int y = 0; y < height; y++)
        {
            int o = y * stride + x * 4;
            if (o + 3 >= buf.Length) break;
            buf[o + 0] = b;
            buf[o + 1] = g;
            buf[o + 2] = r;
            buf[o + 3] = 0xFF;
        }
    }

    private static void DrawFreqScale(byte[] buf, int stride, int width, int height, int margin, double zoom)
    {
        int y0 = height - margin;
        for (int x = 0; x < width; x++)
        {
            int o = y0 * stride + x * 4;
            if (o + 3 >= buf.Length) break;
            buf[o + 0] = 0x33;
            buf[o + 1] = 0x33;
            buf[o + 2] = 0x44;
            buf[o + 3] = 0xFF;
        }

        DrawTick(buf, stride, width, height, width / 2, y0, margin);
        DrawTick(buf, stride, width, height, width / 4, y0, margin);
        DrawTick(buf, stride, width, height, (3 * width) / 4, y0, margin);
        _ = zoom;
    }

    /// <summary>WPF port: dB labels left of the spectrum, on each dB tick, over a dark scrim.</summary>
    private static void DrawDbScaleLabels(byte[] buf, int stride, int width, int plotBottom,
        float minDb, float maxDb, float range)
    {
        if (range < 0.001f) range = 0.001f;
        const int scale = 2;
        int glyphH = 5 * scale;
        int glyphW = 3 * scale;
        int gap = Math.Max(1, scale / 2);
        foreach (int db in SpectrumDisplaySettings.BuildDbTicks(minDb, maxDb))
        {
            float norm = (db - minDb) / range;
            int yLine = (int)((1 - norm) * plotBottom);
            if (yLine < 0 || yLine >= plotBottom) continue;

            string text = db.ToString();
            int textW = 4;
            foreach (char c in text)
                textW += (c == '-' ? 2 * scale : glyphW) + gap;

            int labelY = Math.Clamp(yLine - glyphH / 2, 0, Math.Max(0, plotBottom - glyphH));
            const int labelX = 2;
            int scrimY = Math.Max(0, labelY - 1);
            int scrimW = Math.Min(textW + 2, width);
            for (int py = scrimY; py < scrimY + glyphH + 2 && py < plotBottom; py++)
            {
                for (int px = labelX; px < labelX + scrimW && px < width; px++)
                {
                    int o = py * stride + px * 4;
                    if (o + 3 >= buf.Length) continue;
                    buf[o + 0] = (byte)(buf[o + 0] * 2 / 5);
                    buf[o + 1] = (byte)(buf[o + 1] * 2 / 5);
                    buf[o + 2] = (byte)(buf[o + 2] * 2 / 5);
                    buf[o + 3] = 0xFF;
                }
            }

            const byte lb = 0xCC, lg = 0xCC, lr = 0xDD;
            int cx = labelX + 2;
            foreach (char c in text)
            {
                if (c == '-')
                {
                    FillRect(buf, stride, width, plotBottom, cx, labelY + 2 * scale, 2 * scale, Math.Max(1, scale), lb, lg, lr);
                    cx += 2 * scale + gap;
                }
                else if (char.IsDigit(c))
                {
                    DrawDigit(buf, stride, width, plotBottom, cx, labelY, c - '0', scale, lb, lg, lr);
                    cx += glyphW + gap;
                }
            }
        }
    }

    /// <summary>WPF port: mm:ss in the 3×5 bitmap font.</summary>
    private static void DrawTimeLabel(byte[] buf, int stride, int width, int height, int x, int y, string text, int scale)
    {
        int digitW = 3 * scale;
        int colonW = 2 * scale;
        int gap = Math.Max(1, scale / 2);
        foreach (char c in text)
        {
            if (x < 0 || x >= width - digitW) break;
            if (c == ':')
            {
                int dot = Math.Max(1, scale);
                FillRect(buf, stride, width, height, x + scale / 2, y + scale, dot, dot, 0xFF, 0xFF, 0xFF);
                FillRect(buf, stride, width, height, x + scale / 2, y + 3 * scale, dot, dot, 0xFF, 0xFF, 0xFF);
                x += colonW + gap;
            }
            else if (char.IsDigit(c))
            {
                DrawDigit(buf, stride, width, height, x, y, c - '0', scale, 0xFF, 0xFF, 0xFF);
                x += digitW + gap;
            }
            else
                x += digitW;
        }
    }

    private static void DrawDigit(byte[] buf, int stride, int width, int height,
        int x, int y, int digit, int scale, byte b, byte g, byte r)
    {
        bool[,] shape = GetDigitShape(digit);
        for (int dy = 0; dy < 5; dy++)
            for (int dx = 0; dx < 3; dx++)
                if (shape[dy, dx])
                    FillRect(buf, stride, width, height, x + dx * scale, y + dy * scale, scale, scale, b, g, r);
    }

    internal static void FillRect(byte[] buf, int stride, int width, int height,
        int x, int y, int w, int h, byte b, byte g, byte r)
    {
        for (int py = y; py < y + h; py++)
        {
            if (py < 0 || py >= height) continue;
            for (int px = x; px < x + w; px++)
            {
                if (px < 0 || px >= width) continue;
                int o = py * stride + px * 4;
                if (o + 3 >= buf.Length) continue;
                buf[o + 0] = b;
                buf[o + 1] = g;
                buf[o + 2] = r;
                buf[o + 3] = 0xFF;
            }
        }
    }

    internal static bool[,] GetDigitShape(int digit) => digit switch
    {
        0 => new bool[5, 3] { { true, true, true }, { true, false, true }, { true, false, true }, { true, false, true }, { true, true, true } },
        1 => new bool[5, 3] { { false, true, false }, { true, true, false }, { false, true, false }, { false, true, false }, { true, true, true } },
        2 => new bool[5, 3] { { true, true, true }, { false, false, true }, { true, true, true }, { true, false, false }, { true, true, true } },
        3 => new bool[5, 3] { { true, true, true }, { false, false, true }, { true, true, true }, { false, false, true }, { true, true, true } },
        4 => new bool[5, 3] { { true, false, true }, { true, false, true }, { true, true, true }, { false, false, true }, { false, false, true } },
        5 => new bool[5, 3] { { true, true, true }, { true, false, false }, { true, true, true }, { false, false, true }, { true, true, true } },
        6 => new bool[5, 3] { { true, true, true }, { true, false, false }, { true, true, true }, { true, false, true }, { true, true, true } },
        7 => new bool[5, 3] { { true, true, true }, { false, false, true }, { false, false, true }, { false, false, true }, { false, false, true } },
        8 => new bool[5, 3] { { true, true, true }, { true, false, true }, { true, true, true }, { true, false, true }, { true, true, true } },
        9 => new bool[5, 3] { { true, true, true }, { true, false, true }, { true, true, true }, { false, false, true }, { true, true, true } },
        _ => new bool[5, 3]
    };

    private static void DrawTick(byte[] buf, int stride, int width, int height, int x, int y0, int margin)
    {
        if (x < 0 || x >= width) return;
        int y1 = Math.Min(height - 1, y0 + Math.Min(8, margin - 2));
        for (int y = y0; y <= y1; y++)
        {
            int o = y * stride + x * 4;
            if (o + 3 >= buf.Length) break;
            buf[o + 0] = 0xAA;
            buf[o + 1] = 0xAA;
            buf[o + 2] = 0xBB;
            buf[o + 3] = 0xFF;
        }
    }
}
