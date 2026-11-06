using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using Avalonia.Threading;
using MSCC.Core.Display;

namespace MSCC.Avalonia.Controls;

/// <summary>
/// Spectrum + waterfall display. Consumes <see cref="SpectrumUpdate"/> from MSCC.Core.
/// Left-click = tune (raises <see cref="FrequencyClicked"/>).
/// </summary>
public partial class SpectrumDisplayControl : UserControl
{
    private const int MaxWaterfallHistory = 160;

    /// <summary>Ring buffer of spectrum lines (oldest → newest via head index).</summary>
    private readonly float[]?[] _waterfallRing = new float[MaxWaterfallHistory][];
    /// <summary>WPF port: time-mark label per ring slot (null = no mark), same index as the ring.</summary>
    private readonly string?[] _waterfallTimeRing = new string?[MaxWaterfallHistory];
    private int _waterfallMarkerLastSecond = -1;
    /// <summary>WPF port: PEAK MARKER RF (Hz) placed by right-click; follows the peak each frame.</summary>
    private long? _peakMarkerFreqHz;
    private int _wfCount;
    private int _wfNext; // next write slot

    private WriteableBitmap? _bitmap;
    private int _bmpW;
    private int _bmpH;
    private SpectrumUpdate? _lastUpdate;
    private byte[]? _pixelBuffer;
    private long _lastPaintTicks;
    private const long MinPaintIntervalTicks = TimeSpan.TicksPerMillisecond * 33; // ~30 fps cap

    public static readonly StyledProperty<SpectrumUpdate?> SpectrumUpdateProperty =
        AvaloniaProperty.Register<SpectrumDisplayControl, SpectrumUpdate?>(nameof(SpectrumUpdate));

    public static readonly StyledProperty<double> ZoomFactorProperty =
        AvaloniaProperty.Register<SpectrumDisplayControl, double>(nameof(ZoomFactor), 1.0);

    public static readonly StyledProperty<bool> IsInteractiveProperty =
        AvaloniaProperty.Register<SpectrumDisplayControl, bool>(nameof(IsInteractive), true);

    /// <summary>When false, waterfall is not drawn even if global settings show it (e.g. RX IQ tab).</summary>
    public static readonly StyledProperty<bool> ShowWaterfallProperty =
        AvaloniaProperty.Register<SpectrumDisplayControl, bool>(nameof(ShowWaterfall), true);

    public SpectrumUpdate? SpectrumUpdate
    {
        get => GetValue(SpectrumUpdateProperty);
        set => SetValue(SpectrumUpdateProperty, value);
    }

    public double ZoomFactor
    {
        get => GetValue(ZoomFactorProperty);
        set => SetValue(ZoomFactorProperty, value);
    }

    public bool IsInteractive
    {
        get => GetValue(IsInteractiveProperty);
        set => SetValue(IsInteractiveProperty, value);
    }

    public bool ShowWaterfall
    {
        get => GetValue(ShowWaterfallProperty);
        set => SetValue(ShowWaterfallProperty, value);
    }

    public event EventHandler<long>? FrequencyClicked;

    public SpectrumDisplayControl()
    {
        InitializeComponent();
        PropertyChanged += OnPropertyChangedHandler;
        PointerPressed += OnPointerPressed;
        SizeChanged += (_, _) =>
        {
            if (_lastUpdate != null && IsEffectivelyVisible)
                RenderFrame(_lastUpdate, appendWaterfall: false, force: true);
        };

        SpectrumDisplaySettings.Instance.Changed += OnSettingsChanged;
        AppearanceSettings.Instance.Changed += OnSettingsChanged;
        DetachedFromVisualTree += (_, _) =>
        {
            SpectrumDisplaySettings.Instance.Changed -= OnSettingsChanged;
            AppearanceSettings.Instance.Changed -= OnSettingsChanged;
        };
    }

    private void OnSettingsChanged()
    {
        double z = SpectrumDisplaySettings.Instance.ZoomFactor;
        if (Math.Abs(ZoomFactor - z) > 0.01)
            ZoomFactor = z;

        if (_lastUpdate != null && IsEffectivelyVisible)
        {
            if (Dispatcher.UIThread.CheckAccess())
                RenderFrame(_lastUpdate, appendWaterfall: false, force: true);
            else
                Dispatcher.UIThread.Post(() =>
                {
                    if (_lastUpdate != null && IsEffectivelyVisible)
                        RenderFrame(_lastUpdate, appendWaterfall: false, force: true);
                });
        }
    }

    private void OnPropertyChangedHandler(object? sender, AvaloniaPropertyChangedEventArgs e)
    {
        if (e.Property == SpectrumUpdateProperty && e.NewValue is SpectrumUpdate update)
            OnNewSpectrum(update);
        else if (e.Property == ZoomFactorProperty && e.NewValue is double z)
        {
            SpectrumDisplaySettings.Instance.SetZoomFactor(z);
            if (_lastUpdate != null && IsEffectivelyVisible)
                RenderFrame(_lastUpdate, appendWaterfall: false, force: true);
        }
    }

    private void OnNewSpectrum(SpectrumUpdate update)
    {
        if (!Dispatcher.UIThread.CheckAccess())
        {
            Dispatcher.UIThread.Post(() => OnNewSpectrum(update));
            return;
        }

        // Skip paint when control is not on-screen (other tabs still receive bindings)
        if (!IsEffectivelyVisible)
        {
            _lastUpdate = update;
            return;
        }

        RenderFrame(update, appendWaterfall: true, force: false);
    }

    private void RenderFrame(SpectrumUpdate update, bool appendWaterfall, bool force)
    {
        _lastUpdate = update;

        if (!force)
        {
            long now = DateTime.UtcNow.Ticks;
            if (now - _lastPaintTicks < MinPaintIntervalTicks)
            {
                // Still append waterfall history so scroll stays continuous when we next paint
                if (appendWaterfall && ShowWaterfall && SpectrumDisplaySettings.Instance.ShowWaterfall
                    && update.Data is { Length: > 0 })
                    PushWaterfallLine(update.Data);
                return;
            }
            _lastPaintTicks = now;
        }
        else
        {
            _lastPaintTicks = DateTime.UtcNow.Ticks;
        }

        var settings = SpectrumDisplaySettings.Instance;
        bool allowWf = ShowWaterfall && settings.ShowWaterfall;
        if (appendWaterfall && allowWf && update.Data is { Length: > 0 })
            PushWaterfallLine(update.Data);

        int width = Math.Max(32, (int)Bounds.Width);
        int height = Math.Max(32, (int)Bounds.Height);
        if (double.IsNaN(Bounds.Width) || Bounds.Width < 8)
            width = 640;
        if (double.IsNaN(Bounds.Height) || Bounds.Height < 8)
            height = 220;

        EnsureBitmap(width, height);
        if (_bitmap == null || _pixelBuffer == null || update.Data.Length == 0)
            return;

        IReadOnlyList<float[]>? history = allowWf ? BuildWaterfallSnapshot() : null;
        IReadOnlyList<string?>? timeLabels = allowWf ? BuildTimeLabelSnapshot() : null;
        SpectrumRenderer.Render(update, width, height, _pixelBuffer, history, settings, timeLabels);

        // WPF port: PEAK MARKER on top of the spectrum
        if (settings.PeakMarker && _peakMarkerFreqHz is long markerFreq)
        {
            bool hasWf = history is { Count: > 0 };
            DrawPeakMarker(_pixelBuffer, width, height, update, markerFreq, hasWf, out float markerDb, out long displayFreq);
            PeakMarkerText.Text = $"1  {displayFreq / 1_000_000.0:0.000000} MHz  {markerDb:0.0} dB";
            PeakMarkerBox.IsVisible = true;
        }
        else
        {
            if (!settings.PeakMarker)
                _peakMarkerFreqHz = null;
            PeakMarkerBox.IsVisible = false;
        }

        using (var fb = _bitmap.Lock())
        {
            int srcStride = width * 4;
            int dstStride = fb.RowBytes;
            unsafe
            {
                byte* dst = (byte*)fb.Address;
                for (int y = 0; y < height; y++)
                {
                    System.Runtime.InteropServices.Marshal.Copy(
                        _pixelBuffer, y * srcStride, (IntPtr)(dst + y * dstStride), srcStride);
                }
            }
        }

        // Avoid nulling Source every frame (cheaper invalidate)
        if (!ReferenceEquals(SpectrumImage.Source, _bitmap))
            SpectrumImage.Source = _bitmap;
        else
            SpectrumImage.InvalidateVisual();

        if (HintText != null && update.CenterFrequencyHz > 0)
        {
            double mhz = update.CenterFrequencyHz / 1_000_000.0;
            int vis = Math.Max(1, (int)Math.Round(
                (update.SpanHz > 0 ? update.SpanHz : SpectrumUpdate.DefaultPanadapterSpanHz) /
                Math.Max(1.0, settings.ZoomFactor)));
            HintText.Text =
                $"Center {mhz:0.000000} MHz  |  {update.Data.Length} bins  |  zoom {settings.ZoomFactor:0}× (~{vis / 1000.0:0.#} kHz)  |  click to tune";
        }
    }

    private void PushWaterfallLine(float[] data)
    {
        int n = data.Length;
        float[]? slot = _waterfallRing[_wfNext];
        if (slot == null || slot.Length != n)
        {
            slot = new float[n];
            _waterfallRing[_wfNext] = slot;
        }
        Array.Copy(data, slot, n);

        // WPF port: when the wall-clock second changes and (second % interval) == 0, stamp a line
        string? timeLabel = null;
        int grid = SpectrumDisplaySettings.Instance.WaterfallTimeMarker;
        if (grid > 0)
        {
            int sec = DateTime.Now.Second;
            if (sec != _waterfallMarkerLastSecond)
            {
                _waterfallMarkerLastSecond = sec;
                if (sec % grid == 0)
                    timeLabel = DateTime.UtcNow.ToString("mm:ss");
            }
        }
        _waterfallTimeRing[_wfNext] = timeLabel;

        _wfNext = (_wfNext + 1) % MaxWaterfallHistory;
        if (_wfCount < MaxWaterfallHistory)
            _wfCount++;
    }

    /// <summary>Oldest → newest list for renderer (no alloc of line arrays).</summary>
    private IReadOnlyList<float[]>? BuildWaterfallSnapshot()
    {
        if (_wfCount == 0) return null;
        var list = new List<float[]>(_wfCount);
        int start = (_wfNext - _wfCount + MaxWaterfallHistory) % MaxWaterfallHistory;
        for (int i = 0; i < _wfCount; i++)
        {
            float[]? line = _waterfallRing[(start + i) % MaxWaterfallHistory];
            if (line != null)
                list.Add(line);
        }
        return list.Count > 0 ? list : null;
    }

    /// <summary>Oldest → newest time labels, aligned with <see cref="BuildWaterfallSnapshot"/>.</summary>
    private IReadOnlyList<string?>? BuildTimeLabelSnapshot()
    {
        if (_wfCount == 0 || SpectrumDisplaySettings.Instance.WaterfallTimeMarker <= 0) return null;
        var list = new List<string?>(_wfCount);
        int start = (_wfNext - _wfCount + MaxWaterfallHistory) % MaxWaterfallHistory;
        for (int i = 0; i < _wfCount; i++)
        {
            int slot = (start + i) % MaxWaterfallHistory;
            if (_waterfallRing[slot] != null)
                list.Add(_waterfallTimeRing[slot]);
        }
        return list;
    }

    /// <summary>WPF port: bin with the highest level near a data fraction (± ~2.5 % of span, ≥ 8 bins).</summary>
    private static int FindLocalPeakBin(float[] data, double fraction)
    {
        int n = data.Length;
        int center = Math.Clamp((int)Math.Round(fraction * (n - 1)), 0, n - 1);
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

    /// <summary>WPF port: yellow diamond + "1" on the peak near the stored RF; re-peaks every frame.</summary>
    private void DrawPeakMarker(byte[] buffer, int width, int height, SpectrumUpdate update, long markerFreqHz,
        bool hasWaterfall, out float markerDb, out long displayFreqHz)
    {
        var settings = SpectrumDisplaySettings.Instance;
        markerDb = 0;
        displayFreqHz = markerFreqHz;
        float[] data = update.Data;
        if (data.Length < 2 || width < 2) return;

        int fullSpan = update.SpanHz > 0 ? update.SpanHz : SpectrumUpdate.DefaultPanadapterSpanHz;
        double zoom = Math.Clamp(settings.ZoomFactor, 1.0, 4.0);
        int visSpan = Math.Max(1, (int)Math.Round(fullSpan / zoom));
        int cwOffsetHz = update.CwPitchHz;

        double dataFrac = Math.Clamp(0.5 + (double)(markerFreqHz + cwOffsetHz - update.CenterFrequencyHz) / fullSpan, 0.0, 1.0);
        int peakBin = FindLocalPeakBin(data, dataFrac);
        double peakDataFrac = (double)peakBin / (data.Length - 1);
        displayFreqHz = Math.Max(0, update.CenterFrequencyHz + (long)((peakDataFrac - 0.5) * fullSpan) - cwOffsetHz);
        _peakMarkerFreqHz = displayFreqHz;   // follow the peak if the LO drifts slightly

        markerDb = SpectrumRenderer.ToDisplayDb(data[peakBin], settings.SpectrumDbOffset);

        // Same layout as SpectrumRenderer
        const int freqMargin = 22;
        int plotBottom = hasWaterfall ? Math.Max(50, (int)(height * 0.40)) : Math.Max(50, height - freqMargin);
        settings.GetDisplayGridWindow(out float minDb, out float maxDb);
        float range = Math.Max(0.001f, maxDb - minDb);
        float norm = Math.Clamp((markerDb - minDb) / range, 0f, 1f);
        int y = Math.Clamp((int)((1 - norm) * plotBottom), 2, plotBottom - 2);

        long offsetHz = displayFreqHz + cwOffsetHz - update.CenterFrequencyHz;
        double screenFrac = 0.5 + (double)offsetHz / visSpan;
        if (screenFrac < 0.0 || screenFrac > 1.0)
            return;   // peak outside the zoomed view
        int x = Math.Clamp((int)Math.Round(screenFrac * (width - 1)), 2, width - 3);
        int stride = width * 4;

        const byte mb = 0x00, mg = 0xEE, mr = 0xFF;
        for (int dy = -4; dy <= 4; dy++)
        {
            int half = 4 - Math.Abs(dy);
            SpectrumRenderer.FillRect(buffer, stride, width, plotBottom, x - half, y + dy, 2 * half + 1, 1, mb, mg, mr);
        }
        for (int i = -4; i <= 4; i++)
        {
            SpectrumRenderer.FillRect(buffer, stride, width, plotBottom, x + i, y - (4 - Math.Abs(i)), 1, 1, 0, 0, 0);
            SpectrumRenderer.FillRect(buffer, stride, width, plotBottom, x + i, y + (4 - Math.Abs(i)), 1, 1, 0, 0, 0);
        }
        bool[,] one = SpectrumRenderer.GetDigitShape(1);
        int digitX = x - 3, digitY = Math.Max(0, y - 14);
        for (int dy = 0; dy < 5; dy++)
            for (int dx = 0; dx < 3; dx++)
                if (one[dy, dx])
                    SpectrumRenderer.FillRect(buffer, stride, width, plotBottom, digitX + dx * 2, digitY + dy * 2, 2, 2, mb, mg, mr);
    }

    private void EnsureBitmap(int width, int height)
    {
        if (_bitmap != null && _bmpW == width && _bmpH == height)
            return;

        _bmpW = width;
        _bmpH = height;
        _bitmap = new WriteableBitmap(
            new PixelSize(width, height),
            new Vector(96, 96),
            PixelFormat.Bgra8888,
            AlphaFormat.Opaque);
        _pixelBuffer = new byte[width * height * 4];
    }

    private void OnPointerPressed(object? sender, PointerPressedEventArgs e)
    {
        if (!IsInteractive) return;
        if (_lastUpdate == null || Bounds.Width <= 0) return;
        var props = e.GetCurrentPoint(this).Properties;
        if (props.IsRightButtonPressed)
        {
            PlacePeakMarker(e.GetPosition(this).X);
            e.Handled = true;
            return;
        }
        if (!props.IsLeftButtonPressed) return;

        double x = e.GetPosition(this).X;
        double fraction = Math.Clamp(x / Bounds.Width, 0.0, 1.0);
        int fullSpan = _lastUpdate.SpanHz > 0
            ? _lastUpdate.SpanHz
            : SpectrumUpdate.DefaultPanadapterSpanHz;
        double zoom = Math.Clamp(SpectrumDisplaySettings.Instance.ZoomFactor, 1.0, 4.0);
        int visibleSpan = Math.Max(1, (int)Math.Round(fullSpan / zoom));
        long freq = _lastUpdate.CenterFrequencyHz
            + (long)((fraction - 0.5) * visibleSpan)
            - _lastUpdate.CwPitchHz;
        if (freq < 0) freq = 0;

        FrequencyClicked?.Invoke(this, freq);
    }

    /// <summary>WPF port: right-click places the PEAK MARKER on the nearest peak (no tuning).</summary>
    private void PlacePeakMarker(double x)
    {
        var settings = SpectrumDisplaySettings.Instance;
        if (!settings.PeakMarker || _lastUpdate?.Data is not { Length: > 1 } data) return;
        double fraction = Math.Clamp(x / Bounds.Width, 0.0, 1.0);
        int fullSpan = _lastUpdate.SpanHz > 0 ? _lastUpdate.SpanHz : SpectrumUpdate.DefaultPanadapterSpanHz;
        double zoom = Math.Clamp(settings.ZoomFactor, 1.0, 4.0);
        double dataFrac = 0.5 * (1.0 - 1.0 / zoom) + fraction / zoom;
        int peakBin = FindLocalPeakBin(data, dataFrac);
        double peakDataFrac = (double)peakBin / (data.Length - 1);
        _peakMarkerFreqHz = Math.Max(0, _lastUpdate.CenterFrequencyHz
            + (long)((peakDataFrac - 0.5) * fullSpan) - _lastUpdate.CwPitchHz);
        RenderFrame(_lastUpdate, appendWaterfall: false, force: true);
    }

    public void ClearWaterfall()
    {
        Array.Clear(_waterfallRing);
        Array.Clear(_waterfallTimeRing);
        _wfCount = 0;
        _wfNext = 0;
        if (_lastUpdate != null && IsEffectivelyVisible)
            RenderFrame(_lastUpdate, appendWaterfall: false, force: true);
    }
}
