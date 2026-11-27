using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;

namespace MSCC.Avalonia.Controls;

/// <summary>
/// One wheel notch = <see cref="Slider.SmallChange"/> (or 1). Same rule as
/// <c>MainWindow.Slider_OnPointerWheelChanged</c>, so a style can turn it on
/// without a code-behind handler on each slider.
/// </summary>
public static class SliderWheel
{
    public static readonly AttachedProperty<bool> OneStepPerNotchProperty =
        AvaloniaProperty.RegisterAttached<Slider, bool>("OneStepPerNotch", typeof(SliderWheel));

    static SliderWheel()
    {
        OneStepPerNotchProperty.Changed.AddClassHandler<Slider>(OnOneStepPerNotchChanged);
    }

    public static bool GetOneStepPerNotch(AvaloniaObject element) =>
        element.GetValue(OneStepPerNotchProperty);

    public static void SetOneStepPerNotch(AvaloniaObject element, bool value) =>
        element.SetValue(OneStepPerNotchProperty, value);

    private static void OnOneStepPerNotchChanged(Slider slider, AvaloniaPropertyChangedEventArgs e)
    {
        slider.PointerWheelChanged -= OnPointerWheelChanged;
        if (e.NewValue is true)
            slider.PointerWheelChanged += OnPointerWheelChanged;
    }

    private static void OnPointerWheelChanged(object? sender, PointerWheelEventArgs e)
    {
        if (sender is not Slider slider)
            return;
        int direction = e.Delta.Y > 0 ? 1 : e.Delta.Y < 0 ? -1 : 0;
        if (direction == 0)
            return;
        double step = slider.SmallChange > 0 ? slider.SmallChange : 1.0;
        slider.Value = Math.Max(slider.Minimum, Math.Min(slider.Maximum, slider.Value + direction * step));
        e.Handled = true;
    }
}
