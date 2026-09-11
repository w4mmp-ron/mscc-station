using MSCC.Core.Display;

namespace MSCC.Avalonia.Controls;

/// <summary>
/// Spectrum + waterfall pixel renderer (BGRA).
/// Uses <see cref="SpectrumDisplaySettings"/> for cal, grid, zoom, waterfall window.
/// </summary>
internal static class SpectrumRenderer
{
    public static float ToDisplayDb(float rawMappedDb, float dbOffset) => rawMappedDb + dbOffset;

    public static void Render(
        SpectrumUpdate update,
        int width,
        int height,
        byte[] bgra,
        IReadOnlyList<float[]>? waterfallHistory,
        SpectrumDisplaySettings? settings = null)
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

        float minDb = settings.GridMinDb;
        float maxDb = settings.GridMaxDb;
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

            float dbv = ToDisplayDb(data[idx], dbOffset);
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

                    float dbv = ToDisplayDb(line[idx], dbOffset);
                    WaterfallPalettes.MapColor(scheme, dbv, wfLow, wfHigh, out byte wr, out byte wg, out byte wb);

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
