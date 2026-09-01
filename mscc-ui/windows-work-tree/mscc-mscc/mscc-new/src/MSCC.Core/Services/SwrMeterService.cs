using System.Globalization;
using System.Net;
using System.Net.Http;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using MSCC.Core.Logging;

namespace MSCC.Core.Services;

/// <summary>
/// Listens for external Multus WiFi SWR meter UDP (JSON or legacy pipe) and optional HTTP fault reset.
/// Independent of radio UDP (UdpRadioService).
/// </summary>
public sealed class SwrMeterService : IDisposable
{
    private UdpClient? _udp;
    private CancellationTokenSource? _cts;
    private Task? _loop;
    private int _port = 6999;
    private bool _enabled;
    private string? _lastSourceIp;
    private static readonly HttpClient Http = new() { Timeout = TimeSpan.FromSeconds(3) };

    public event Action<SwrMeterReading>? ReadingReceived;
    public event Action<string>? StatusChanged;

    public bool IsListening => _udp != null && _enabled;
    public int ListenPort => _port;
    public string? LastSourceIp => _lastSourceIp;

    /// <summary>Start or restart UDP listen on the given port. No-op if already listening on same port.</summary>
    public void Start(int port)
    {
        port = Math.Clamp(port, 1, 65535);
        if (_enabled && _udp != null && _port == port)
            return;

        Stop();
        _port = port;
        _enabled = true;

        try
        {
            _udp = new UdpClient(new IPEndPoint(IPAddress.Any, _port));
            _udp.Client.ReceiveTimeout = 500;
            _cts = new CancellationTokenSource();
            _loop = Task.Run(() => ReceiveLoopAsync(_cts.Token));
            RaiseStatus($"Listening UDP port {_port}");
            DebugMonitor.MonitorTextBoxText($" SwrMeterService: listening on UDP {_port}");
        }
        catch (Exception ex)
        {
            _enabled = false;
            RaiseStatus($"Listen failed on {_port}: {ex.Message}");
            DebugMonitor.MonitorTextBoxText($" SwrMeterService start error: {ex.Message}");
            try { _udp?.Dispose(); } catch { /* ignore */ }
            _udp = null;
        }
    }

    public void Stop()
    {
        _enabled = false;
        try { _cts?.Cancel(); } catch { /* ignore */ }
        try { _udp?.Dispose(); } catch { /* ignore */ }
        _udp = null;
        _cts = null;
        _loop = null;
        RaiseStatus("Stopped");
    }

    /// <summary>
    /// GET http://{meterIp}/api?action=reset — meter rejects if RF still present.
    /// </summary>
    public async Task<(bool Ok, string Message)> ResetFaultAsync(string? meterIp, CancellationToken ct = default)
    {
        if (string.IsNullOrWhiteSpace(meterIp))
            meterIp = _lastSourceIp;
        if (string.IsNullOrWhiteSpace(meterIp))
            return (false, "No meter IP (wait for UDP packet or enter IP in Settings)");

        try
        {
            string url = $"http://{meterIp.Trim()}/api?action=reset";
            using var resp = await Http.GetAsync(url, ct).ConfigureAwait(false);
            string body = await resp.Content.ReadAsStringAsync(ct).ConfigureAwait(false);
            if (!resp.IsSuccessStatusCode)
                return (false, $"HTTP {(int)resp.StatusCode}: {body}");
            DebugMonitor.MonitorTextBoxText($" SwrMeterService RESET OK → {meterIp}");
            return (true, "Reset requested (meter only clears when RF is low)");
        }
        catch (Exception ex)
        {
            return (false, ex.Message);
        }
    }

    private async Task ReceiveLoopAsync(CancellationToken ct)
    {
        while (!ct.IsCancellationRequested && _udp != null)
        {
            try
            {
                var result = await _udp.ReceiveAsync(ct).ConfigureAwait(false);
                string text = Encoding.UTF8.GetString(result.Buffer).Trim();
                if (string.IsNullOrEmpty(text))
                    continue;

                string ip = result.RemoteEndPoint.Address.ToString();
                if (_lastSourceIp != ip)
                {
                    _lastSourceIp = ip;
                    RaiseStatus($"Listening UDP {_port} — last packet from {ip}");
                }

                var reading = TryParse(text, ip);
                if (reading != null)
                    ReadingReceived?.Invoke(reading);
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch (ObjectDisposedException)
            {
                break;
            }
            catch (SocketException)
            {
                // timeout / transient
            }
            catch (Exception ex)
            {
                DebugMonitor.MonitorTextBoxText($" SwrMeterService recv: {ex.Message}");
            }
        }
    }

    public static SwrMeterReading? TryParse(string text, string? sourceIp)
    {
        text = text.Trim();
        if (text.StartsWith('{'))
            return ParseJson(text, sourceIp);
        if (text.Contains('|'))
            return ParsePipe(text, sourceIp);
        return null;
    }

    private static SwrMeterReading? ParseJson(string text, string? sourceIp)
    {
        try
        {
            using var doc = JsonDocument.Parse(text);
            var r = doc.RootElement;
            float fwd = GetFloat(r, "fwd");
            float rev = GetFloat(r, "ref");
            float peak = GetFloat(r, "peak");
            float swr = GetFloat(r, "swr", 1f);
            if (swr < 1f) swr = 1f;
            bool fault = GetInt(r, "fault") != 0;
            bool tx = GetInt(r, "tx") != 0;
            float thr = GetFloat(r, "swrThr", 2f);
            return new SwrMeterReading
            {
                ForwardWatts = fwd,
                ReflectedWatts = rev,
                PeakWatts = peak,
                Swr = swr,
                Fault = fault,
                Tx = tx,
                SwrThreshold = thr,
                SourceIp = sourceIp,
                Utc = DateTime.UtcNow,
            };
        }
        catch
        {
            return null;
        }
    }

    /// <summary>Legacy desktop-app pipe format: v|fwd|ref|?|swr|?|fault|temp|...</summary>
    private static SwrMeterReading? ParsePipe(string text, string? sourceIp)
    {
        try
        {
            string[] parts = text.Split('|');
            if (parts.Length < 7)
                return null;
            float fwd = float.Parse(parts[1], CultureInfo.InvariantCulture);
            float rev = float.Parse(parts[2], CultureInfo.InvariantCulture);
            float swr = float.Parse(parts[4], CultureInfo.InvariantCulture);
            if (swr < 1f) swr = 1f;
            int fault = int.Parse(parts[6], CultureInfo.InvariantCulture);
            return new SwrMeterReading
            {
                ForwardWatts = fwd,
                ReflectedWatts = rev,
                PeakWatts = fwd,
                Swr = swr,
                Fault = fault != 0,
                Tx = fwd > 0.5f,
                SwrThreshold = 2f,
                SourceIp = sourceIp,
                Utc = DateTime.UtcNow,
            };
        }
        catch
        {
            return null;
        }
    }

    private static float GetFloat(JsonElement r, string name, float fallback = 0f)
    {
        if (!r.TryGetProperty(name, out var p))
            return fallback;
        if (p.ValueKind == JsonValueKind.Number && p.TryGetSingle(out float f))
            return f;
        if (p.ValueKind == JsonValueKind.String &&
            float.TryParse(p.GetString(), NumberStyles.Float, CultureInfo.InvariantCulture, out f))
            return f;
        return fallback;
    }

    private static int GetInt(JsonElement r, string name)
    {
        if (!r.TryGetProperty(name, out var p))
            return 0;
        if (p.ValueKind == JsonValueKind.Number && p.TryGetInt32(out int i))
            return i;
        if (p.ValueKind == JsonValueKind.String && int.TryParse(p.GetString(), out i))
            return i;
        return 0;
    }

    private void RaiseStatus(string s) => StatusChanged?.Invoke(s);

    public void Dispose() => Stop();
}
