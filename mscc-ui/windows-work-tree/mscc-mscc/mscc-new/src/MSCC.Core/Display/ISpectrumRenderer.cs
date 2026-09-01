namespace MSCC.Core.Display;

/// <summary>
/// Abstraction for rendering spectrum data. Allows swapping rendering backends later.
/// </summary>
public interface ISpectrumRenderer
{
    void Render(
        SpectrumUpdate update,
        int width,
        int height,
        byte[] pixelBuffer,
        System.Collections.Generic.IReadOnlyList<float[]>? waterfallHistory = null,
        System.Collections.Generic.IReadOnlyList<bool>? waterfallTimeMarkers = null,
        System.Collections.Generic.IReadOnlyList<string?>? waterfallTimeLabels = null,
        double zoomFactor = 1.0);
}
