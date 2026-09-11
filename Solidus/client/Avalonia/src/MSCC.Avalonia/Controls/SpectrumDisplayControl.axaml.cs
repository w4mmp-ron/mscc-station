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
        SpectrumRenderer.Render(update, width, height, _pixelBuffer, history, settings);

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
        if (!e.GetCurrentPoint(this).Properties.IsLeftButtonPressed) return;

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

    public void ClearWaterfall()
    {
        Array.Clear(_waterfallRing);
        _wfCount = 0;
        _wfNext = 0;
        if (_lastUpdate != null && IsEffectivelyVisible)
            RenderFrame(_lastUpdate, appendWaterfall: false, force: true);
    }
}
