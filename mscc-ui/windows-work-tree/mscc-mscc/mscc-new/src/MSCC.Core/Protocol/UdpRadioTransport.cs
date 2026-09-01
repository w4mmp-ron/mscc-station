using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;
using MSCC.Core.Logging;

namespace MSCC.Core.Protocol;

/// <summary>
/// Low-level UDP transport for communicating with the MSCC backend servers.
/// Uses modern async patterns (ReadOnlyMemory) and proper CancellationToken support.
/// </summary>
public sealed class UdpRadioTransport : IDisposable
{
    private UdpClient _udpClient;
    private readonly IPEndPoint _remoteEndPoint;
    private CancellationTokenSource? _cts;
    private readonly int _localPort;
    private bool _disposed;

    public event EventHandler<RadioPacketReceivedEventArgs>? PacketReceived;

    public bool IsConnected { get; private set; }

    public UdpRadioTransport(string remoteIp, int remotePort, int localPort = 0)
    {
        var addr = ResolveHostOrIp(remoteIp);
        _remoteEndPoint = new IPEndPoint(addr, remotePort);
        _localPort = localPort;
        if (localPort > 0)
        {
            _udpClient = new UdpClient(localPort);  // binds receive to specific port (like original GUI_PORT)
        }
        else
        {
            _udpClient = new UdpClient();           // ephemeral port
        }
    }

    private static IPAddress ResolveHostOrIp(string hostOrIp)
    {
        if (IPAddress.TryParse(hostOrIp, out var ip))
            return ip;

        try
        {
            var addrs = Dns.GetHostAddresses(hostOrIp);
            // Prefer IPv4
            var ipv4 = addrs.FirstOrDefault(a => a.AddressFamily == AddressFamily.InterNetwork);
            if (ipv4 != null) return ipv4;
            return addrs.FirstOrDefault() ?? throw new ArgumentException($"Could not resolve host '{hostOrIp}' to any IP address.");
        }
        catch (Exception ex)
        {
            throw new ArgumentException($"Invalid IP address or hostname '{hostOrIp}': {ex.Message}", ex);
        }
    }

    public async Task StartAsync(CancellationToken cancellationToken = default)
    {
        if (IsConnected)
            return;

        _cts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);

        // Only bind if we didn't already bind to a specific port in the ctor
        // (original binds rx socket to fixed GUI_PORT; we support that via localPort)
        if (_localPort == 0)
        {
            _udpClient.Client.Bind(new IPEndPoint(IPAddress.Any, 0));
        }

        IsConnected = true;

        DebugMonitor.MonitorTextBoxText($" UdpTransport started: remote={_remoteEndPoint} localBindPort={_localPort}");

        _ = ReceiveLoopAsync(_cts.Token);
    }

    private async Task ReceiveLoopAsync(CancellationToken token)
    {
        while (!token.IsCancellationRequested)
        {
            try
            {
                var result = await _udpClient.ReceiveAsync(token);
                var data = result.Buffer;

                if (data.Length >= 1)
                {
                    byte opcode = data[0];
                    byte[] payload = data.Length > 1 ? data[1..] : Array.Empty<byte>();

                    PacketReceived?.Invoke(this, new RadioPacketReceivedEventArgs(opcode, payload));
                }
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch (Exception ex)
            {
                DebugMonitor.MonitorTextBoxText($" UdpTransport receive error: {ex.Message}");
            }
        }
    }

    /// <summary>
    /// Sends a raw payload to the remote endpoint.
    /// </summary>
    public async Task SendAsync(byte opcode, ReadOnlyMemory<byte> payload, CancellationToken cancellationToken = default)
    {
        if (!IsConnected)
            throw new InvalidOperationException("Transport has not been started.");

        // Build message: [opcode][payload...]
        byte[] message = new byte[payload.Length + 1];
        message[0] = opcode;

        if (!payload.IsEmpty)
            payload.Span.CopyTo(message.AsSpan(1));

        string opcodeName = Opcodes.GetName(opcode);

        // Suppress logging for keep-alive (0xF4 "I'm Alive") messages per user request.
        // These are sent ~1x per second and were flooding the log.
        // All other sends (freq, mode, filters, power, etc.) continue to be logged verbosely.
        if (opcode != Opcodes.CMD_SET_KEEP_ALIVE)
        {
            DebugMonitor.MonitorTextBoxText($" UdpTransport send: opcode 0x{opcode:X2} ({opcodeName}) len={message.Length} payload={BitConverter.ToString(message, 1, Math.Min(8, message.Length-1))}{(message.Length > 9 ? "..." : "")}");
        }

        await _udpClient.SendAsync(message.AsMemory(), _remoteEndPoint, cancellationToken);
    }

    // Convenience overloads (kept for easier migration from old code)
    public Task SendAsync(byte opcode, byte[] payload, CancellationToken cancellationToken = default)
        => SendAsync(opcode, payload.AsMemory(), cancellationToken);

    public Task SendAsync(byte opcode, short data, CancellationToken cancellationToken = default)
        => SendAsync(opcode, BitConverter.GetBytes(data), cancellationToken);

    public Task SendAsync(byte opcode, int data, CancellationToken cancellationToken = default)
        => SendAsync(opcode, BitConverter.GetBytes(data), cancellationToken);

    public Task SendAsync(byte opcode, string data, int maxLength, CancellationToken cancellationToken = default)
    {
        byte[] ascii = System.Text.Encoding.ASCII.GetBytes(data);
        int length = Math.Min(ascii.Length, maxLength);
        return SendAsync(opcode, ascii.AsMemory(0, length), cancellationToken);
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;

        IsConnected = false;

        try { _cts?.Cancel(); } catch { }
        try { _udpClient?.Dispose(); } catch { }
        try { _cts?.Dispose(); } catch { }

        _cts = null;
        _udpClient = null;
    }
}

/// <summary>
/// Event arguments for packets received from the radio backend.
/// </summary>
public sealed class RadioPacketReceivedEventArgs : EventArgs
{
    public byte Opcode { get; }
    public byte[] Payload { get; }

    public RadioPacketReceivedEventArgs(byte opcode, byte[] payload)
    {
        Opcode = opcode;
        Payload = payload ?? Array.Empty<byte>();
    }
}
