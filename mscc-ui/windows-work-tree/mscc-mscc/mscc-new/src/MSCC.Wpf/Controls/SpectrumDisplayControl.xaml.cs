using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using MSCC.Core.Display;

namespace MSCC.Wpf.Controls;

/// <summary>
/// High-performance spectrum / panadapter display using WriteableBitmap.
/// Supports click-to-tune.
/// </summary>
public partial class SpectrumDisplayControl : UserControl
{
    private WriteableBitmap? _bitmap;
    private int _lastWidth;
    private int _lastHeight;
    private SpectrumUpdate? _lastUpdate;

    private readonly List<float[]> _waterfallHistory = new List<float[]>();
    /// <summary>Parallel to history: true if this line is a time-marker (scrolls with waterfall).</summary>
    private readonly List<bool> _waterfallTimeMarkers = new List<bool>();
    /// <summary>UTC time label for marker lines (mm:ss); empty when not a marker.</summary>
    private readonly List<string?> _waterfallTimeLabels = new List<string?>();
    private int _waterfallMarkerLastSecond = -1;
    private const int MaxWaterfallHistory = 300;

    private readonly BasicSpectrumRenderer _renderer = new();

    private double? _cursorFraction;  // 0.0 (left) to 1.0 (right) — resize-safe

    /// <summary>Peak marker RF (Hz) when PEAK MARKER is on and user has clicked. Null = not placed.</summary>
    private long? _peakMarkerFreqHz;

    // Spectrum fill colors managed via SpectrumColorSettings.SetFill (called from S/W window)

    public static readonly DependencyProperty SpectrumUpdateProperty =
        DependencyProperty.Register(
            nameof(SpectrumUpdate),
            typeof(SpectrumUpdate),
            typeof(SpectrumDisplayControl),
            new PropertyMetadata(null, OnSpectrumUpdateChanged));

    public SpectrumUpdate? SpectrumUpdate
    {
        get => (SpectrumUpdate?)GetValue(SpectrumUpdateProperty);
        set => SetValue(SpectrumUpdateProperty, value);
    }

    /// <summary>
    /// When false, the control acts as pure visual reference only (no click-to-tune, no cursor interaction).
    /// Used for Freq Cal tab spectrum display.
    /// </summary>
    public static readonly DependencyProperty IsInteractiveProperty =
        DependencyProperty.Register(
            nameof(IsInteractive),
            typeof(bool),
            typeof(SpectrumDisplayControl),
            new PropertyMetadata(true, OnIsInteractiveChanged));

    public bool IsInteractive
    {
        get => (bool)GetValue(IsInteractiveProperty);
        set => SetValue(IsInteractiveProperty, value);
    }

    private static void OnIsInteractiveChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is SpectrumDisplayControl ctrl)
        {
            ctrl.Cursor = (bool)e.NewValue ? Cursors.None : Cursors.Arrow;
            if (!(bool)e.NewValue && ctrl.CursorFreqBox != null)
                ctrl.CursorFreqBox.Visibility = Visibility.Collapsed;
        }
    }

    public static readonly DependencyProperty ShowWaterfallProperty =
        DependencyProperty.Register(
            nameof(ShowWaterfall),
            typeof(bool),
            typeof(SpectrumDisplayControl),
            new PropertyMetadata(true));

    public bool ShowWaterfall
    {
        get => (bool)GetValue(ShowWaterfallProperty);
        set => SetValue(ShowWaterfallProperty, value);
    }

    /// <summary>
    /// Client display zoom (1–4×). Crops/stretches the fixed 72 kHz pan around center; no new RF detail.
    /// </summary>
    public static readonly DependencyProperty ZoomFactorProperty =
        DependencyProperty.Register(
            nameof(ZoomFactor),
            typeof(double),
            typeof(SpectrumDisplayControl),
            new PropertyMetadata(1.0, OnZoomFactorChanged));

    public double ZoomFactor
    {
        get => (double)GetValue(ZoomFactorProperty);
        set => SetValue(ZoomFactorProperty, value);
    }

    private static void OnZoomFactorChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is SpectrumDisplayControl c)
            c.RedrawDisplayOnly();
    }

    /// <summary>Drop waterfall history (e.g. pan resolution change — bin counts no longer match).</summary>
    public void ClearWaterfallHistory()
    {
        _waterfallHistory.Clear();
        _waterfallTimeMarkers.Clear();
        _waterfallTimeLabels.Clear();
        RedrawDisplayOnly();
    }

    /// <summary>Visible span after display zoom (full span / zoom).</summary>
    private int VisibleSpanHz(SpectrumUpdate u)
    {
        int full = u.SpanHz > 0 ? u.SpanHz : SpectrumUpdate.DefaultPanadapterSpanHz;
        double z = Math.Clamp(ZoomFactor, 1.0, 4.0);
        return Math.Max(1, (int)Math.Round(full / z));
    }

    /// <summary>Screen X fraction 0..1 → data array fraction 0..1 (centered viewport).</summary>
    private static double ScreenToDataFraction(double screenFrac, double zoom)
    {
        double z = Math.Max(1.0, zoom);
        double viewStart = 0.5 * (1.0 - 1.0 / z);
        double viewWidth = 1.0 / z;
        return viewStart + Math.Clamp(screenFrac, 0.0, 1.0) * viewWidth;
    }

    private static void OnSpectrumUpdateChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is SpectrumDisplayControl control && e.NewValue is SpectrumUpdate update)
        {
            // Real spectrum packet: append one waterfall line
            control.UpdateSpectrumInternal(update, appendToWaterfall: true);
        }
    }

    public event EventHandler<long>? FrequencyClicked;

    public SpectrumDisplayControl()
    {
        InitializeComponent();
        SizeChanged += OnSizeChanged;
        MouseLeftButtonDown += OnMouseLeftButtonDown;
        MouseRightButtonDown += OnMouseRightButtonDown;
        MouseMove += SpectrumControl_MouseMove;
        MouseLeave += SpectrumControl_MouseLeave;
        Cursor = Cursors.None; // custom vertical cursor line drawn in the bitmap using CURSOR color from S/W
    }

    public void UpdateSpectrum(SpectrumUpdate update)
    {
        // Public method kept for backward compatibility / direct calls if needed
        UpdateSpectrumInternal(update, appendToWaterfall: true);
    }

    /// <summary>
    /// Re-render display (spectrum + waterfall + cursor) without advancing waterfall history.
    /// Used for mouse cursor moves, leave, and resize — those must not speed up the waterfall.
    /// </summary>
    private void RedrawDisplayOnly()
    {
        if (_lastUpdate != null)
            UpdateSpectrumInternal(_lastUpdate, appendToWaterfall: false);
    }

    private void UpdateSpectrumInternal(SpectrumUpdate update, bool appendToWaterfall)
    {
        _lastUpdate = update;

        if (!Dispatcher.CheckAccess())
        {
            Dispatcher.BeginInvoke(
                () => UpdateSpectrumInternal(update, appendToWaterfall),
                DispatcherPriority.Render);
            return;
        }

        // Maintain waterfall history (newest at end) -- only for new spectrum data, never for cursor redraws
        if (appendToWaterfall && update?.Data != null && update.Data.Length > 0)
        {
            bool mark = false;
            string? timeLabel = null;
            int grid = SpectrumColorSettings.WaterfallTimeMarker;
            if (grid > 0)
            {
                // Original: when wall-clock second changes and (second % Time_grid)==0, stamp a line
                int sec = DateTime.Now.Second;
                if (sec != _waterfallMarkerLastSecond)
                {
                    _waterfallMarkerLastSecond = sec;
                    if ((sec % grid) == 0)
                    {
                        mark = true;
                        timeLabel = DateTime.UtcNow.ToString("mm:ss");
                    }
                }
            }

            _waterfallHistory.Add((float[])update.Data.Clone());
            _waterfallTimeMarkers.Add(mark);
            _waterfallTimeLabels.Add(timeLabel);
            while (_waterfallHistory.Count > MaxWaterfallHistory)
            {
                _waterfallHistory.RemoveAt(0);
                if (_waterfallTimeMarkers.Count > 0) _waterfallTimeMarkers.RemoveAt(0);
                if (_waterfallTimeLabels.Count > 0) _waterfallTimeLabels.RemoveAt(0);
            }
        }

        EnsureBitmap((int)ActualWidth, (int)ActualHeight);

        if (_bitmap is null || update.Data.Length == 0)
            return;

        int width = _bitmap.PixelWidth;
        int height = _bitmap.PixelHeight;

        byte[] pixelBuffer = new byte[width * height * 4];

        var wfHistory = ShowWaterfall ? _waterfallHistory : null;
        var wfMarkers = ShowWaterfall ? _waterfallTimeMarkers : null;
        var wfLabels = ShowWaterfall ? _waterfallTimeLabels : null;
        double zoom = Math.Clamp(ZoomFactor, 1.0, 4.0);
        _renderer.Render(update, width, height, pixelBuffer, wfHistory, wfMarkers, wfLabels, zoom);

        // === Peak marker (S/W PEAK MARKER) — after spectrum so it sits on top ===
        if (SpectrumColorSettings.PeakMarker && _peakMarkerFreqHz is long markerFreq)
        {
            DrawPeakMarker(pixelBuffer, width, height, update, markerFreq, out float markerDb, out long displayFreq);
            UpdatePeakMarkerReadout(displayFreq, markerDb);
        }
        else
        {
            if (!SpectrumColorSettings.PeakMarker)
                _peakMarkerFreqHz = null;
            HidePeakMarkerReadout();
        }

        // === Mouse hover cursor: vertical line from top to bottom of the display ===
        // Colored using the CURSOR setting from the S/W controls (SpectrumColorSettings).
        // Drawn as overlay after the main spectrum/waterfall/center/filter rendering so the
        // existing display is not affected. Only added when mouse is over the control.
        if (_cursorFraction.HasValue && width > 0)
        {
            double cxD = _cursorFraction.Value * width;
            int cx = (int)Math.Round(cxD);
            cx = Math.Clamp(cx, 0, width - 1);

            byte cb = SpectrumColorSettings.CursorB;
            byte cg = SpectrumColorSettings.CursorG;
            byte cr = SpectrumColorSettings.CursorR;

            for (int y = 0; y < height; y++)
            {
                int offset = y * (width * 4) + cx * 4;
                if (offset + 3 < pixelBuffer.Length)
                {
                    pixelBuffer[offset + 0] = cb;
                    pixelBuffer[offset + 1] = cg;
                    pixelBuffer[offset + 2] = cr;
                    pixelBuffer[offset + 3] = 0xFF;
                }
            }
        }

        _bitmap.Lock();
        _bitmap.WritePixels(new Int32Rect(0, 0, width, height), pixelBuffer, width * 4, 0);
        _bitmap.AddDirtyRect(new Int32Rect(0, 0, width, height));
        _bitmap.Unlock();

        if (SpectrumImage.Source != _bitmap)
            SpectrumImage.Source = _bitmap;

        // Update the frequency readout overlay (if cursor is active) after (re)render.
        // This ensures the displayed freq/delta stays correct when new spectrum data arrives
        // (e.g. VFO change) while the mouse is hovering.
        if (_cursorFraction.HasValue)
            UpdateCursorReadout();
    }

    private void EnsureBitmap(int width, int height)
    {
        if (width <= 0 || height <= 0) return;

        if (_bitmap == null || width != _lastWidth || height != _lastHeight)
        {
            _bitmap = new WriteableBitmap(width, height, 96, 96, PixelFormats.Bgra32, null);
            _lastWidth = width;
            _lastHeight = height;
        }
    }

    private void OnSizeChanged(object sender, SizeChangedEventArgs e)
    {
        EnsureBitmap((int)e.NewSize.Width, (int)e.NewSize.Height);
        // Redraw only — do not append waterfall lines on resize
        RedrawDisplayOnly();
        // Reposition the freq box for the (possibly changed) width while cursor active
        if (_cursorFraction.HasValue)
            UpdateCursorReadout();
    }

    private void OnMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (!IsInteractive || _lastUpdate == null || ActualWidth <= 0) return;

        // Left-click: click-to-tune only (no peak marker — that is right-click).
        double x = e.GetPosition(this).X;
        double fraction = Math.Clamp(x / ActualWidth, 0.0, 1.0);
        int spanHz = VisibleSpanHz(_lastUpdate);
        int cwOffsetHz = _lastUpdate.CwPitchHz;
        long clickedFreq = _lastUpdate.CenterFrequencyHz
            + (long)((fraction - 0.5) * spanHz)
            - cwOffsetHz;
        if (clickedFreq < 0) clickedFreq = 0;

        FrequencyClicked?.Invoke(this, clickedFreq);
    }

    /// <summary>
    /// Right-click: place peak marker (when PEAK MARKER is on). Does not change tune.
    /// </summary>
    private void OnMouseRightButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (!IsInteractive || _lastUpdate == null || ActualWidth <= 0) return;
        if (!SpectrumColorSettings.PeakMarker) return;
        if (_lastUpdate.Data is not { Length: > 1 } data) return;

        double x = e.GetPosition(this).X;
        double fraction = Math.Clamp(x / ActualWidth, 0.0, 1.0);
        int fullSpan = _lastUpdate.SpanHz > 0
            ? _lastUpdate.SpanHz
            : SpectrumUpdate.DefaultPanadapterSpanHz;
        int cwOffsetHz = _lastUpdate.CwPitchHz;

        // Peak search in the zoomed data window under the click
        double dataFrac = ScreenToDataFraction(fraction, ZoomFactor);
        int peakBin = FindLocalPeakBin(data, dataFrac);
        double peakDataFrac = (double)peakBin / (data.Length - 1);
        long peakFreq = _lastUpdate.CenterFrequencyHz
            + (long)((peakDataFrac - 0.5) * fullSpan)
            - cwOffsetHz;
        if (peakFreq < 0) peakFreq = 0;
        _peakMarkerFreqHz = peakFreq;

        e.Handled = true; // avoid context menu / other right-click handling
        RedrawDisplayOnly(); // show marker immediately
    }

    /// <summary>Find bin with maximum level near the click X (local peak search).</summary>
    private static int FindLocalPeakBin(float[] data, double fraction)
    {
        int n = data.Length;
        int center = (int)Math.Round(fraction * (n - 1));
        center = Math.Clamp(center, 0, n - 1);
        // ± ~2.5% of span (≈ ±1.8 kHz on 72 kHz) or at least ±8 bins
        int halfWin = Math.Max(8, n / 40);
        int lo = Math.Max(0, center - halfWin);
        int hi = Math.Min(n - 1, center + halfWin);
        int best = center;
        float bestDb = data[center];
        for (int i = lo; i <= hi; i++)
        {
            if (data[i] > bestDb)
            {
                bestDb = data[i];
                best = i;
            }
        }
        return best;
    }

    /// <summary>
    /// Draw marker "1" + diamond on peak near stored RF; refine Y from current spectrum data.
    /// </summary>
    private void DrawPeakMarker(
        byte[] buffer, int width, int height, SpectrumUpdate update, long markerFreqHz,
        out float markerDb, out long displayFreqHz)
    {
        markerDb = update.MinDb;
        displayFreqHz = markerFreqHz;

        float[] data = update.Data;
        if (data.Length < 2 || width < 2) return;

        int fullSpan = update.SpanHz > 0 ? update.SpanHz : SpectrumUpdate.DefaultPanadapterSpanHz;
        int visSpan = VisibleSpanHz(update);
        int cwOffsetHz = update.CwPitchHz;
        // Data fraction from absolute RF (full span), then screen X from visible span
        double dataFrac = 0.5 + (double)(markerFreqHz + cwOffsetHz - update.CenterFrequencyHz) / fullSpan;
        dataFrac = Math.Clamp(dataFrac, 0.0, 1.0);

        // Re-peak near that bin each frame so the marker sits on the tip as the trace updates
        int peakBin = FindLocalPeakBin(data, dataFrac);
        double peakDataFrac = (double)peakBin / (data.Length - 1);
        displayFreqHz = update.CenterFrequencyHz
            + (long)((peakDataFrac - 0.5) * fullSpan)
            - cwOffsetHz;
        if (displayFreqHz < 0) displayFreqHz = 0;
        // Track refined RF so marker follows if LO drifts slightly
        _peakMarkerFreqHz = displayFreqHz;

        markerDb = SpectrumColorSettings.ToDisplayDb(data[peakBin]);

        // Layout matches BasicSpectrumRenderer spectrum region
        bool hasWaterfall = ShowWaterfall && _waterfallHistory.Count > 0;
        int spectrumPlotBottom = hasWaterfall
            ? Math.Max(60, (int)(height * 0.38))
            : Math.Max(60, height - 26);

        // Prefer live S/W grid window (GRID MAX = pane top; BASELINE fine-tunes floor only)
        SpectrumColorSettings.GetDisplayGridWindow(out float minDb, out float maxDb);
        if (maxDb - minDb < 20f)
        {
            minDb = update.MinDb;
            maxDb = update.MaxDb;
        }
        float range = maxDb - minDb;
        if (range < 0.001f) range = 0.001f;
        float norm = Math.Clamp((markerDb - minDb) / range, 0f, 1f);
        int y = (int)((1 - norm) * spectrumPlotBottom);
        y = Math.Clamp(y, 2, spectrumPlotBottom - 2);

        // Screen X: offset from center in visible-span coordinates
        long offsetHz = displayFreqHz + cwOffsetHz - update.CenterFrequencyHz;
        double screenFrac = 0.5 + (double)offsetHz / visSpan;
        if (screenFrac < 0.0 || screenFrac > 1.0)
            return; // peak outside zoomed view
        int x = (int)Math.Round(screenFrac * (width - 1));
        x = Math.Clamp(x, 2, width - 3);
        int stride = width * 4;

        // Yellow diamond at peak + digit "1" (tinySA-style)
        const byte mb = 0x00, mg = 0xEE, mr = 0xFF;
        DrawMarkerDiamond(buffer, stride, width, spectrumPlotBottom, x, y, mb, mg, mr);
        int digitY = Math.Max(0, y - 14);
        DrawMarkerDigit1(buffer, stride, width, spectrumPlotBottom, x - 3, digitY, mb, mg, mr);
    }

    private static void DrawMarkerDiamond(
        byte[] buffer, int stride, int width, int height, int cx, int cy, byte b, byte g, byte r)
    {
        for (int dy = -4; dy <= 4; dy++)
        {
            int half = 4 - Math.Abs(dy);
            for (int dx = -half; dx <= half; dx++)
                PeakSetPixel(buffer, stride, width, height, cx + dx, cy + dy, b, g, r);
        }
        for (int i = -4; i <= 4; i++)
        {
            PeakSetPixel(buffer, stride, width, height, cx + i, cy - (4 - Math.Abs(i)), 0, 0, 0);
            PeakSetPixel(buffer, stride, width, height, cx + i, cy + (4 - Math.Abs(i)), 0, 0, 0);
        }
    }

    /// <summary>3×5 digit "1" at scale 2 (matches renderer bitmap font style).</summary>
    private static void DrawMarkerDigit1(
        byte[] buffer, int stride, int width, int height, int x, int y, byte b, byte g, byte r)
    {
        // Shape for digit 1 from GetDigitShape: { {0,1,0}, {1,1,0}, {0,1,0}, {0,1,0}, {1,1,1} }
        bool[,] shape =
        {
            { false, true, false },
            { true, true, false },
            { false, true, false },
            { false, true, false },
            { true, true, true }
        };
        const int scale = 2;
        for (int dy = 0; dy < 5; dy++)
        {
            for (int dx = 0; dx < 3; dx++)
            {
                if (!shape[dy, dx]) continue;
                for (int sy = 0; sy < scale; sy++)
                for (int sx = 0; sx < scale; sx++)
                    PeakSetPixel(buffer, stride, width, height, x + dx * scale + sx, y + dy * scale + sy, b, g, r);
            }
        }
    }

    private static void PeakSetPixel(byte[] buffer, int stride, int width, int height, int x, int y, byte b, byte g, byte r)
    {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        int offset = y * stride + x * 4;
        if (offset + 3 >= buffer.Length) return;
        buffer[offset + 0] = b;
        buffer[offset + 1] = g;
        buffer[offset + 2] = r;
        buffer[offset + 3] = 0xFF;
    }

    private void UpdatePeakMarkerReadout(long freqHz, float db)
    {
        if (PeakMarkerBox == null || PeakMarkerText == null) return;
        // Display-scale dB (not calibrated dBm)
        PeakMarkerText.Text = $"1  {FormatFrequency(freqHz)}  {db:0.0} dB";
        PeakMarkerBox.Visibility = Visibility.Visible;
    }

    private void HidePeakMarkerReadout()
    {
        if (PeakMarkerBox != null)
            PeakMarkerBox.Visibility = Visibility.Collapsed;
    }

    private void SpectrumControl_MouseMove(object sender, MouseEventArgs e)
    {
        if (!IsInteractive || _lastUpdate == null || ActualWidth <= 0) return;

        _cursorFraction = Math.Clamp(e.GetPosition(this).X / ActualWidth, 0.0, 1.0);
        // Redraw cursor only — must NOT append to waterfall (was causing speed-up under the mouse)
        RedrawDisplayOnly();
        UpdateCursorReadout();
    }

    private void SpectrumControl_MouseLeave(object sender, MouseEventArgs e)
    {
        _cursorFraction = null;
        if (CursorFreqBox != null)
            CursorFreqBox.Visibility = Visibility.Collapsed;
        // Redraw without cursor; do not advance waterfall
        RedrawDisplayOnly();
    }

    private void UpdateCursorReadout()
    {
        if (CursorFreqBox == null || CursorFreqText == null)
            return;

        if (!_cursorFraction.HasValue || _lastUpdate == null || ActualWidth <= 0)
        {
            CursorFreqBox.Visibility = Visibility.Collapsed;
            return;
        }

        double fraction = Math.Clamp(_cursorFraction.Value, 0.0, 1.0);
        long centerHz = _lastUpdate.CenterFrequencyHz;
        long spanHz = VisibleSpanHz(_lastUpdate);

        // Same X→freq map as click-to-tune (visible span + CW pitch subtract).
        long cursorHz = centerHz + (long)((fraction - 0.5) * spanHz) - _lastUpdate.CwPitchHz;
        long deltaHz = cursorHz - centerHz;

        string freqText = FormatFrequency(cursorHz);
        string deltaText = "";
        if (deltaHz != 0)
        {
            double deltaKHz = deltaHz / 1000.0;
            deltaText = $"  ({(deltaKHz > 0 ? "+" : "")}{deltaKHz:0.0} kHz)";
        }

        CursorFreqText.Text = freqText + deltaText;

        // Position the box near the top, slightly to the right of the cursor line.
        double pixelX = fraction * ActualWidth;
        double left = pixelX + 8;

        if (CursorFreqBox.ActualWidth > 0)
        {
            double maxLeft = ActualWidth - CursorFreqBox.ActualWidth - 4;
            if (left > maxLeft)
            {
                // Place to the left of the line instead
                left = pixelX - CursorFreqBox.ActualWidth - 8;
            }
        }

        Canvas.SetLeft(CursorFreqBox, Math.Max(4, left));
        Canvas.SetTop(CursorFreqBox, 3);
        CursorFreqBox.Visibility = Visibility.Visible;
    }

    private static string FormatFrequency(long hz)
    {
        if (hz <= 0) return "0.000.000";
        long mhz = hz / 1_000_000;
        long rem = hz % 1_000_000;
        long khz = rem / 1000;
        long hzPart = rem % 1000;
        return $"{mhz}.{khz:D3}.{hzPart:D3}";
    }
}

/// <summary>
/// Significantly improved spectrum renderer with frequency scale, dB grid,
/// better filter visualization, and more professional appearance.
/// </summary>
internal sealed class BasicSpectrumRenderer : ISpectrumRenderer
{
    public void Render(
        SpectrumUpdate update,
        int width,
        int height,
        byte[] pixelBuffer,
        System.Collections.Generic.IReadOnlyList<float[]>? waterfallHistory = null,
        System.Collections.Generic.IReadOnlyList<bool>? waterfallTimeMarkers = null,
        System.Collections.Generic.IReadOnlyList<string?>? waterfallTimeLabels = null,
        double zoomFactor = 1.0)
    {
        // Dark background - now configurable via S/W BACKGROUND listbox
        byte bgB = SpectrumColorSettings.BackgroundB;
        byte bgG = SpectrumColorSettings.BackgroundG;
        byte bgR = SpectrumColorSettings.BackgroundR;
        for (int i = 0; i < pixelBuffer.Length; i += 4)
        {
            pixelBuffer[i + 0] = bgB;
            pixelBuffer[i + 1] = bgG;
            pixelBuffer[i + 2] = bgR;
            pixelBuffer[i + 3] = 0xFF;
        }

        float[] data = update.Data;
        if (data.Length == 0 || width == 0 || height == 0) return;

        // S/W GRID MAX/MIN are the dBm window (PowerSDR-style). Defaults −20…−125.
        // BASELINE fine-tunes floor only — never pixel-shifts (that left blank above −20).
        SpectrumColorSettings.GetDisplayGridWindow(out float minDb, out float maxDb);
        if (maxDb - minDb < 20f)
        {
            minDb = update.MinDb;
            maxDb = update.MaxDb;
        }
        float range = maxDb - minDb;
        if (range < 0.001f) range = 0.001f;
        int stride = width * 4;

        bool hasWaterfall = waterfallHistory != null && waterfallHistory.Count > 0;

        double zoom = Math.Clamp(zoomFactor, 1.0, 4.0);
        int fullSpan = update.SpanHz > 0 ? update.SpanHz : SpectrumUpdate.DefaultPanadapterSpanHz;
        int visibleSpan = Math.Max(1, (int)Math.Round(fullSpan / zoom));
        // Centered viewport into full pan data (fraction of array length)
        double viewStart = 0.5 * (1.0 - 1.0 / zoom);
        double viewWidth = 1.0 / zoom;

        int cwOffsetBins = 0;
        if (update.CwPitchHz != 0 && fullSpan > 0)
        {
            cwOffsetBins = (int)(update.CwPitchHz * (double)data.Length / fullSpan);
        }

        // === Layout: spectrum on top (~38% if waterfall), or full if no waterfall; freq scale at very bottom ===
        // freqMargin: room for scale-2 MHz labels + short ticks below (labels sit above ticks).
        const int freqMargin = 26;
        int spectrumHeight = hasWaterfall
            ? Math.Max(60, (int)(height * 0.38))
            : Math.Max(60, height - freqMargin);
        int wfStartY = hasWaterfall ? spectrumHeight + 3 : height;
        int wfHeight = hasWaterfall ? Math.Max(30, height - wfStartY - freqMargin) : 0;
        int spectrumPlotBottom = spectrumHeight;

        // Filter passband X range — use visible (zoomed) span so passband scales with zoom
        int filterLeftX = int.MinValue;
        int filterRightX = int.MinValue;
        if (update.FilterLowHz != 0 || update.FilterHighHz != 0)
        {
            double hzPerPx = visibleSpan / (double)Math.Max(1, width);
            if (hzPerPx < 0.001) hzPerPx = 0.001;
            int fx0 = (int)Math.Round((update.FilterLowHz / hzPerPx) + (width / 2.0));
            int fx1 = (int)Math.Round((update.FilterHighHz / hzPerPx) + (width / 2.0));
            filterLeftX = Math.Min(fx0, fx1);
            filterRightX = Math.Max(fx0, fx1);
            if (filterRightX < filterLeftX + 1)
                filterRightX = filterLeftX + 1;
        }

        // === Horizontal dB grid (S/W VIEW GRID) — spectrum area, outside filter passband ===
        if (SpectrumColorSettings.ViewGrid)
        {
            // Dim gray so lines read on dark backgrounds without drowning the trace
            const byte gridB = 0x48;
            const byte gridG = 0x48;
            const byte gridR = 0x55;
            // Ticks follow PowerSDR-style grid min/max (not a fixed −140…0 list)
            int[] dbTicks = SpectrumColorSettings.BuildDbTicks(minDb, maxDb);
            foreach (int db in dbTicks)
            {
                float norm = (db - minDb) / range;
                int y = (int)((1 - norm) * spectrumPlotBottom);
                if (y < 0 || y >= spectrumPlotBottom) continue;

                for (int x = 0; x < width; x++)
                {
                    // Leave passband clear — matches prior look (grid not in bandwidth shade)
                    if (filterLeftX != int.MinValue && x >= filterLeftX && x <= filterRightX)
                        continue;

                    int offset = y * stride + x * 4;
                    if (offset + 3 >= pixelBuffer.Length) continue;

                    pixelBuffer[offset + 0] = gridB;
                    pixelBuffer[offset + 1] = gridG;
                    pixelBuffer[offset + 2] = gridR;
                    pixelBuffer[offset + 3] = 0xFF;
                }
            }
        }

        // === Draw spectrum (limited to upper area; X maps through zoomed viewport) ===
        for (int x = 0; x < width; x++)
        {
            double t = width > 1 ? (double)x / (width - 1) : 0.5;
            double dataFrac = viewStart + t * viewWidth;
            int idx = (int)(dataFrac * (data.Length - 1) + cwOffsetBins);
            idx = Math.Clamp(idx, 0, data.Length - 1);

            float dbValue = SpectrumColorSettings.ToDisplayDb(data[idx]);
            float norm = (dbValue - minDb) / range;
            norm = Math.Clamp(norm, 0f, 1f);

            // GRID MAX at y=0, GRID MIN (floor-tuned) at bottom — no pixel BASELINE shift
            int y = (int)((1 - norm) * spectrumPlotBottom);
            y = Math.Clamp(y, 0, spectrumPlotBottom);

            // Filled area under curve (spectrum) -- skip entirely for SCOPE mode (oscilloscope trace / just the line)
            if (SpectrumColorSettings.CurrentFillMode != "SCOPE")
            {
                for (int yy = y; yy < spectrumPlotBottom; yy++)
                {
                    int offset = yy * stride + x * 4;
                    if (offset + 3 >= pixelBuffer.Length) continue;

                    pixelBuffer[offset + 0] = SpectrumColorSettings.FillB;
                    pixelBuffer[offset + 1] = SpectrumColorSettings.FillG;
                    pixelBuffer[offset + 2] = SpectrumColorSettings.FillR;
                    pixelBuffer[offset + 3] = 0xFF;
                }
            }

            // Bright top line
            if (y >= 0 && y < spectrumPlotBottom)
            {
                int offset = y * stride + x * 4;
                pixelBuffer[offset + 0] = SpectrumColorSettings.LineB;
                pixelBuffer[offset + 1] = SpectrumColorSettings.LineG;
                pixelBuffer[offset + 2] = SpectrumColorSettings.LineR;
                pixelBuffer[offset + 3] = 0xFF;
            }
        }

        // === Center tuning line (spectrum area only) - color controlled by CURSOR in S/W window ===
        int centerX = width / 2;
        for (int yy = 0; yy < spectrumPlotBottom; yy++)
        {
            int offset = yy * stride + centerX * 4;
            if (offset + 3 < pixelBuffer.Length)
            {
                pixelBuffer[offset + 0] = SpectrumColorSettings.CursorB;
                pixelBuffer[offset + 1] = SpectrumColorSettings.CursorG;
                pixelBuffer[offset + 2] = SpectrumColorSettings.CursorR;
                pixelBuffer[offset + 3] = 0xEE;
            }
        }

        // === Filter passband shading + edges (spectrum area only) ===
        // Offsets are relative to center (Hz). AM is typically ±high-cut; USB/LSB one-sided.
        // Always normalize left/right so inverted or equal edges still draw correctly.
        if (update.FilterLowHz != 0 || update.FilterHighHz != 0)
        {
            double hzPerPixel = visibleSpan / (double)Math.Max(1, width);
            if (hzPerPixel < 0.001) hzPerPixel = 0.001;

            int x0 = (int)Math.Round((update.FilterLowHz / hzPerPixel) + (width / 2.0));
            int x1 = (int)Math.Round((update.FilterHighHz / hzPerPixel) + (width / 2.0));
            int filterLeft = Math.Min(x0, x1);
            int filterRight = Math.Max(x0, x1);
            // Ensure at least 1 px when edges coincide
            if (filterRight < filterLeft + 1)
                filterRight = filterLeft + 1;

            // Light blue-ish passband -- skip for SCOPE to avoid fill under the trace
            if (SpectrumColorSettings.CurrentFillMode != "SCOPE")
            {
                int xStart = Math.Max(0, filterLeft);
                int xEnd = Math.Min(width - 1, filterRight);
                for (int x = xStart; x <= xEnd; x++)
                {
                    for (int yy = 0; yy < spectrumPlotBottom; yy++)
                    {
                        int offset = yy * stride + x * 4;
                        if (offset + 3 >= pixelBuffer.Length) continue;

                        pixelBuffer[offset + 0] = (byte)Math.Min(255, pixelBuffer[offset + 0] + 25);
                        pixelBuffer[offset + 1] = (byte)Math.Min(255, pixelBuffer[offset + 1] + 45);
                        pixelBuffer[offset + 2] = (byte)Math.Min(255, pixelBuffer[offset + 2] + 80);
                    }
                }
            }

            // Orange filter edges (thin lines, keep even in SCOPE) — only if on-screen
            if (filterLeft >= 0 && filterLeft < width)
                DrawVerticalLine(pixelBuffer, stride, width, spectrumPlotBottom, filterLeft, 0xFF, 0xB0, 0x20, 255);
            if (filterRight >= 0 && filterRight < width)
                DrawVerticalLine(pixelBuffer, stride, width, spectrumPlotBottom, filterRight, 0xFF, 0xB0, 0x20, 255);
        }

        // === dB labels on left (S/W dB LABELS) — absolute dBm (GRID MAX at top) ===
        if (SpectrumColorSettings.ViewDbLabels)
            DrawDbScaleLabels(pixelBuffer, stride, width, spectrumPlotBottom, minDb, maxDb, range);

        // thin separator between spectrum and waterfall (skip if no waterfall for Freq Cal)
        if (hasWaterfall)
        {
            for (int x = 0; x < width; x++)
            {
                int ysep = wfStartY - 1;
                int off = ysep * stride + x * 4;
                if (off + 3 < pixelBuffer.Length)
                {
                    pixelBuffer[off + 0] = 0x55;
                    pixelBuffer[off + 1] = 0x55;
                    pixelBuffer[off + 2] = 0x66;
                    pixelBuffer[off + 3] = 0xFF;
                }
            }
        }

        // === Waterfall ===
        // Normal: oldest at top, newest at bottom. Reversed: newest at top, oldest at bottom.
        // Skip entirely for Freq Cal tab (pure visual reference, no waterfall needed)
        if (hasWaterfall && waterfallHistory != null && waterfallHistory.Count > 0 && wfHeight > 20)
        {
            int numLines = waterfallHistory.Count;
            int lineLen = waterfallHistory[0].Length;
            bool directionNormal = SpectrumColorSettings.WaterfallDirectionNormal;
            for (int y = wfStartY; y < height - freqMargin; y++)
            {
                double frac = (double)(y - wfStartY) / Math.Max(1, wfHeight - 1);
                frac = Math.Clamp(frac, 0.0, 1.0);
                // History index 0 = oldest, Count-1 = newest
                double lineIdxD = directionNormal
                    ? frac * (numLines - 1)
                    : (1.0 - frac) * (numLines - 1);
                int li0 = (int)lineIdxD;
                int li1 = Math.Min(li0 + 1, numLines - 1);
                double t = lineIdxD - li0;

                for (int x = 0; x < width; x++)
                {
                    double xt = width > 1 ? (double)x / (width - 1) : 0.5;
                    double dataFrac = viewStart + xt * viewWidth;
                    int idx = (int)(dataFrac * (lineLen - 1) + cwOffsetBins);
                    idx = Math.Clamp(idx, 0, lineLen - 1);

                    float db0 = SpectrumColorSettings.ToDisplayDb(waterfallHistory[li0][idx]);
                    float db1 = SpectrumColorSettings.ToDisplayDb(waterfallHistory[li1][idx]);
                    float dbv = (float)(db0 * (1.0 - t) + db1 * t);

                    // Color window = Waterfall Low/High (not spectrum GRID). Then GAIN/ZERO fine trim.
                    SpectrumColorSettings.GetWaterfallColorWindow(out float wfLow, out float wfHigh);
                    SpectrumColorSettings.ApplyWaterfallGainZero(
                        dbv, wfLow, wfHigh, out float adjDb, out float adjMin, out float adjMax);
                    GetWaterfallColor(adjDb, adjMin, adjMax, out byte wr, out byte wg, out byte wb);
                    int off = y * stride + x * 4;
                    if (off + 3 < pixelBuffer.Length)
                    {
                        // WriteableBitmap is Bgra32 (B,G,R,A).
                        // Red/Yellow uses the pre-palette write order (r→[0], g→[1], b→[2]) so it
                        // matches the classic look users preferred before palette work.
                        // Other schemes use correct B,G,R channel order.
                        if (SpectrumColorSettings.WaterfallScheme == WaterfallColorScheme.RedYellow)
                        {
                            pixelBuffer[off + 0] = wr;
                            pixelBuffer[off + 1] = wg;
                            pixelBuffer[off + 2] = wb;
                        }
                        else
                        {
                            pixelBuffer[off + 0] = wb;
                            pixelBuffer[off + 1] = wg;
                            pixelBuffer[off + 2] = wr;
                        }
                        pixelBuffer[off + 3] = 0xFF;
                    }
                }
            }

            // Time markers: white horizontal lines + UTC mm:ss (scroll with history)
            if (waterfallTimeMarkers != null && waterfallTimeMarkers.Count > 0)
            {
                int markCount = Math.Min(numLines, waterfallTimeMarkers.Count);
                for (int li = 0; li < markCount; li++)
                {
                    if (!waterfallTimeMarkers[li]) continue;

                    // Same direction mapping as color rows (li 0 = oldest)
                    int y;
                    if (numLines <= 1)
                    {
                        y = wfStartY;
                    }
                    else
                    {
                        double fracLi = (double)li / (numLines - 1);
                        if (!directionNormal)
                            fracLi = 1.0 - fracLi;
                        y = wfStartY + (int)Math.Round(fracLi * (wfHeight - 1));
                    }
                    y = Math.Clamp(y, wfStartY, height - freqMargin - 1);

                    // White line starts after the time label (~mm:ss at scale 2 ≈ 25% under prior size-3)
                    const int timeLabelScale = 2;
                    int x0 = 4 + 5 * (3 * timeLabelScale + 2) + 8; // 5 glyphs × cell width + pad
                    for (int x = x0; x < width; x++)
                    {
                        int off = y * stride + x * 4;
                        if (off + 3 >= pixelBuffer.Length) break;
                        pixelBuffer[off + 0] = 0xFF;
                        pixelBuffer[off + 1] = 0xFF;
                        pixelBuffer[off + 2] = 0xFF;
                        pixelBuffer[off + 3] = 0xFF;
                    }

                    // UTC mm:ss — scaled bitmap font
                    string label = ":";
                    if (waterfallTimeLabels != null && li < waterfallTimeLabels.Count &&
                        !string.IsNullOrEmpty(waterfallTimeLabels[li]))
                        label = waterfallTimeLabels[li]!;
                    int labelH = 5 * timeLabelScale;
                    int labelY = Math.Clamp(y - labelH / 2, wfStartY, height - freqMargin - labelH);
                    DrawTimeLabel(pixelBuffer, stride, width, height, 4, labelY, label, timeLabelScale);
                }
            }
        }

        // === Frequency scale at bottom (visible span after zoom) ===
        DrawFrequencyScale(pixelBuffer, stride, width, height, update, freqMargin, visibleSpan);
    }

    /// <summary>
    /// dB labels on the left of the spectrum, centered on each dB tick Y.
    /// Drawn into the bitmap (approach B) so spectrum/waterfall width is unchanged.
    /// Y is absolute dBm: maxDb at top of pane, minDb at bottom (no pixel baseline shift).
    /// Toggled by S/W dB LABELS (independent of VIEW GRID).
    /// </summary>
    private static void DrawDbScaleLabels(
        byte[] buffer, int stride, int width, int spectrumPlotBottom,
        float minDb, float maxDb, float range)
    {
        if (range < 0.001f) range = 0.001f;
        int[] dbTicks = SpectrumColorSettings.BuildDbTicks(minDb, maxDb);
        const int scale = 2; // 3×5 bitmap font — readable on the spectrum face
        int glyphH = 5 * scale;
        int glyphW = 3 * scale;
        int gap = Math.Max(1, scale / 2);

        foreach (int db in dbTicks)
        {
            float norm = (db - minDb) / range;
            int yLine = (int)((1 - norm) * spectrumPlotBottom);
            if (yLine < 0 || yLine >= spectrumPlotBottom) continue;

            string text = db.ToString(); // e.g. "-80", "0"
            int textW = 0;
            foreach (char c in text)
                textW += (c == '-' ? 2 * scale : glyphW) + gap;
            textW += 4; // pad

            int labelY = Math.Clamp(yLine - glyphH / 2, 0, Math.Max(0, spectrumPlotBottom - glyphH));
            int labelX = 2;

            // Dark scrim behind text so it stays readable on fill / passband
            int scrimH = glyphH + 2;
            int scrimY = Math.Max(0, labelY - 1);
            int scrimW = Math.Min(textW + 2, width);
            for (int py = scrimY; py < scrimY + scrimH && py < spectrumPlotBottom; py++)
            {
                for (int px = labelX; px < labelX + scrimW; px++)
                {
                    int off = py * stride + px * 4;
                    if (off + 3 >= buffer.Length) continue;
                    // Blend toward dark (keeps a bit of underlying color)
                    buffer[off + 0] = (byte)(buffer[off + 0] * 2 / 5);
                    buffer[off + 1] = (byte)(buffer[off + 1] * 2 / 5);
                    buffer[off + 2] = (byte)(buffer[off + 2] * 2 / 5);
                    buffer[off + 3] = 0xFF;
                }
            }

            // Light grey glyphs
            const byte lb = 0xCC, lg = 0xCC, lr = 0xDD;
            int cx = labelX + 2;
            foreach (char c in text)
            {
                if (c == '-')
                {
                    int barW = 2 * scale;
                    int barH = Math.Max(1, scale);
                    FillRect(buffer, stride, width, spectrumPlotBottom,
                        cx, labelY + 2 * scale, barW, barH, lb, lg, lr);
                    cx += barW + gap;
                }
                else if (char.IsDigit(c))
                {
                    DrawDigitScaledColored(buffer, stride, width, spectrumPlotBottom,
                        cx, labelY, c - '0', scale, lb, lg, lr);
                    cx += glyphW + gap;
                }
            }
        }
    }

    private static void DrawDigitScaledColored(
        byte[] buffer, int stride, int width, int height,
        int x, int y, int digit, int scale, byte b, byte g, byte r)
    {
        bool[,] shape = GetDigitShape(digit);
        for (int dy = 0; dy < 5; dy++)
        {
            for (int dx = 0; dx < 3; dx++)
            {
                if (!shape[dy, dx]) continue;
                FillRect(buffer, stride, width, height,
                    x + dx * scale, y + dy * scale, scale, scale, b, g, r);
            }
        }
    }

    /// <summary>Draw mm:ss with a scaled 3×5 bitmap font (scale 2 ≈ readable, not oversized).</summary>
    private static void DrawTimeLabel(byte[] buffer, int stride, int width, int height, int x, int y, string text, int scale = 2)
    {
        if (scale < 1) scale = 1;
        int digitW = 3 * scale;
        int colonW = 2 * scale;
        int gap = Math.Max(1, scale / 2);

        foreach (char c in text)
        {
            if (x < 0 || x >= width - digitW) break;
            if (c == ':')
            {
                // Two square dots, vertically spaced
                int dot = Math.Max(1, scale);
                FillRect(buffer, stride, width, height, x + scale / 2, y + scale, dot, dot, 0xFF, 0xFF, 0xFF);
                FillRect(buffer, stride, width, height, x + scale / 2, y + 3 * scale, dot, dot, 0xFF, 0xFF, 0xFF);
                x += colonW + gap;
            }
            else if (char.IsDigit(c))
            {
                DrawDigitScaled(buffer, stride, width, height, x, y, c - '0', scale);
                x += digitW + gap;
            }
            else
            {
                x += digitW;
            }
        }
    }

    private static void DrawDigitScaled(byte[] buffer, int stride, int width, int height, int x, int y, int digit, int scale)
    {
        bool[,] shape = GetDigitShape(digit);
        for (int dy = 0; dy < 5; dy++)
        {
            for (int dx = 0; dx < 3; dx++)
            {
                if (!shape[dy, dx]) continue;
                FillRect(buffer, stride, width, height,
                    x + dx * scale, y + dy * scale, scale, scale, 0xFF, 0xFF, 0xFF);
            }
        }
    }

    private static void FillRect(byte[] buffer, int stride, int width, int height,
        int x, int y, int w, int h, byte b, byte g, byte r)
    {
        for (int py = y; py < y + h; py++)
        {
            for (int px = x; px < x + w; px++)
                SetPixel(buffer, stride, width, height, px, py, b, g, r);
        }
    }

    private static void DrawVerticalLine(byte[] buffer, int stride, int width, int height, int x, byte b, byte g, byte r, byte a)
    {
        if (x < 0 || x >= width) return;

        for (int y = 0; y < height; y++)
        {
            int offset = y * stride + x * 4;
            if (offset + 3 >= buffer.Length) continue;

            buffer[offset + 0] = b;
            buffer[offset + 1] = g;
            buffer[offset + 2] = r;
            buffer[offset + 3] = a;
        }
    }

    private static void DrawFrequencyScale(
        byte[] buffer, int stride, int width, int height, SpectrumUpdate update, int bottomMargin, int visibleSpanHz)
    {
        // Labels above ticks (scale 2), short ticks at the very bottom — no glyph/tick overlap.
        const int scale = 2;
        int glyphH = 5 * scale;
        int gap = Math.Max(1, scale / 2);
        int tickH = 5;
        int padBottom = 2;

        int yTickTop = height - padBottom - tickH;
        int yLabel = Math.Max(0, yTickTop - glyphH - 2); // number sits fully above tick

        if (visibleSpanHz <= 0)
            visibleSpanHz = update.SpanHz > 0 ? update.SpanHz : SpectrumUpdate.DefaultPanadapterSpanHz;
        double startFreq = update.CenterFrequencyHz - (visibleSpanHz / 2.0);
        double step = visibleSpanHz / 10.0;

        for (int i = 0; i <= 10; i++)
        {
            double freq = startFreq + (step * i);
            // Clamp tick X so edge ticks stay on-screen
            int xTick = (int)Math.Round(i / 10.0 * (width - 1));
            xTick = Math.Clamp(xTick, 0, width - 1);

            // Short tick under the label row
            for (int yy = yTickTop; yy < height - padBottom; yy++)
            {
                int offset = yy * stride + xTick * 4;
                if (offset + 3 < buffer.Length)
                {
                    buffer[offset + 0] = 0xAA;
                    buffer[offset + 1] = 0xAA;
                    buffer[offset + 2] = 0xAA;
                    buffer[offset + 3] = 0xFF;
                }
            }

            // MHz label (e.g. 7.100) — larger bitmap font
            string label = $"{freq / 1_000_000.0:F3}";
            int textW = MeasureSimpleTextWidth(label, scale, gap);
            int xText;
            if (i == 0)
                xText = xTick; // left-most: start at tick
            else if (i == 10)
                xText = xTick - textW + 1; // right-most: end at tick
            else
                xText = xTick - textW / 2; // center on tick

            xText = Math.Clamp(xText, 1, Math.Max(1, width - textW - 1));
            DrawSimpleTextScaled(buffer, stride, width, height, xText, yLabel, label, scale, gap);
        }
    }

    private static int MeasureSimpleTextWidth(string text, int scale, int gap)
    {
        int w = 0;
        int glyphW = 3 * scale;
        int dotW = Math.Max(2, scale);
        foreach (char c in text)
        {
            if (c == '.')
                w += dotW + gap;
            else if (char.IsDigit(c))
                w += glyphW + gap;
        }
        return Math.Max(1, w - gap);
    }

    private static void DrawSimpleTextScaled(
        byte[] buffer, int stride, int width, int height,
        int x, int y, string text, int scale, int gap)
    {
        const byte lb = 0xCC, lg = 0xCC, lr = 0xDD;
        int glyphW = 3 * scale;
        int cx = x;
        foreach (char c in text)
        {
            if (cx >= width - 2) break;

            if (c == '.')
            {
                int dot = Math.Max(1, scale);
                FillRect(buffer, stride, width, height,
                    cx + scale / 2, y + 4 * scale, dot, dot, lb, lg, lr);
                cx += Math.Max(2, scale) + gap;
            }
            else if (char.IsDigit(c))
            {
                DrawDigitScaledColored(buffer, stride, width, height,
                    cx, y, c - '0', scale, lb, lg, lr);
                cx += glyphW + gap;
            }
        }
    }

    private static void SetPixel(byte[] buffer, int stride, int width, int height, int x, int y, byte b, byte g, byte r)
    {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        int offset = y * stride + x * 4;
        if (offset + 3 >= buffer.Length) return;

        buffer[offset + 0] = b;
        buffer[offset + 1] = g;
        buffer[offset + 2] = r;
        buffer[offset + 3] = 0xFF;
    }

    private static bool[,] GetDigitShape(int digit)
    {
        return digit switch
        {
            0 => new bool[5,3] { {true,true,true}, {true,false,true}, {true,false,true}, {true,false,true}, {true,true,true} },
            1 => new bool[5,3] { {false,true,false}, {true,true,false}, {false,true,false}, {false,true,false}, {true,true,true} },
            2 => new bool[5,3] { {true,true,true}, {false,false,true}, {true,true,true}, {true,false,false}, {true,true,true} },
            3 => new bool[5,3] { {true,true,true}, {false,false,true}, {true,true,true}, {false,false,true}, {true,true,true} },
            4 => new bool[5,3] { {true,false,true}, {true,false,true}, {true,true,true}, {false,false,true}, {false,false,true} },
            5 => new bool[5,3] { {true,true,true}, {true,false,false}, {true,true,true}, {false,false,true}, {true,true,true} },
            6 => new bool[5,3] { {true,true,true}, {true,false,false}, {true,true,true}, {true,false,true}, {true,true,true} },
            7 => new bool[5,3] { {true,true,true}, {false,false,true}, {false,false,true}, {false,false,true}, {false,false,true} },
            8 => new bool[5,3] { {true,true,true}, {true,false,true}, {true,true,true}, {true,false,true}, {true,true,true} },
            9 => new bool[5,3] { {true,true,true}, {true,false,true}, {true,true,true}, {false,false,true}, {true,true,true} },
            _ => new bool[5,3]
        };
    }

    /// <summary>
    /// Map dB intensity to RGB using the active S/W waterfall palette.
    /// Red/Yellow = exact pre-palette WPF gradient (written with legacy channel order at call site).
    /// Enhanced / Spectran / BlackWhite = original MSCC schemes (correct BGR write order).
    /// </summary>
    private static void GetWaterfallColor(float db, float minDb, float maxDb, out byte r, out byte g, out byte b)
    {
        float range = maxDb - minDb;
        if (range < 0.001f)
            range = 0.001f;

        float norm = (db - minDb) / range;
        norm = Math.Clamp(norm, 0f, 1f);

        switch (SpectrumColorSettings.WaterfallScheme)
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
                // Exact pre-palette GetWaterfallColor (byte-for-byte same math)
                GetClassicWpfHeatMapColor(norm, out r, out g, out b);
                break;
        }
    }

    /// <summary>
    /// Exact waterfall color function used before palette work began.
    /// Values are written with the legacy channel order (r→B, g→G, b→R in Bgra32)
    /// so the on-screen look matches what users saw before.
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
        // Low color = black for WPF (original used WaterfallLowColor, default black)
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
            // Original multiplies local_percent * 5 in several bands; clamp for safety
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
