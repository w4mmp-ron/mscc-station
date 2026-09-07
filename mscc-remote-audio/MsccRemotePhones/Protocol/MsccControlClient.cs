using System.Net;
using System.Net.Sockets;
using System.Buffers.Binary;

namespace MsccRemotePhones.Protocol;

/// <summary>
/// Minimal ms-sdr control UDP (same wire format as MSCC client).
/// Injects into a live session — no GUI handshake.
/// Sends are fire-and-forget on a thread-pool thread (UI never blocks on DNS/Send).
/// </summary>
public static class MsccControlClient
{
    public const int DefaultMsSdrPort = 8888;

    public const byte CmdSetAudioDevice = 0x9B;
    public const byte CmdSetCompressionState = 0xEE;
    public const byte CmdSetCompressionLevel = 0xEF;

    public const byte AudioDigital = 0;
    public const byte AudioPhones = 1;
    public const byte AudioRemote = 2;

    public const int CompressionLevelMin = 0;
    public const int CompressionLevelMax = 24;

    /// <summary>Resolve hostname or IPv4; throw if none.</summary>
    public static IPAddress ResolveHost(string host)
    {
        if (string.IsNullOrWhiteSpace(host))
            throw new ArgumentException("Host is required.", nameof(host));
        host = host.Trim();
        if (IPAddress.TryParse(host, out var ip) && ip.AddressFamily == AddressFamily.InterNetwork)
            return ip;
        var addrs = Dns.GetHostAddresses(host);
        var v4 = Array.Find(addrs, a => a.AddressFamily == AddressFamily.InterNetwork);
        if (v4 is null)
            throw new InvalidOperationException($"No IPv4 address for '{host}'.");
        return v4;
    }

    /// <summary>
    /// Queue [opcode u8][int16 LE] to ms-sdr without blocking the caller.
    /// </summary>
    public static void SendOpcode(string host, int port, byte opcode, short data,
        Action? onSent = null, Action<Exception>? onError = null)
    {
        if (port is < 1 or > 65535)
            throw new ArgumentOutOfRangeException(nameof(port));

        string hostCopy = host;
        _ = Task.Run(() =>
        {
            try
            {
                byte[] pkt = new byte[3];
                pkt[0] = opcode;
                BinaryPrimitives.WriteInt16LittleEndian(pkt.AsSpan(1), data);

                using var udp = new UdpClient();
                var ep = new IPEndPoint(ResolveHost(hostCopy), port);
                udp.Send(pkt, ep);
                onSent?.Invoke();
            }
            catch (Exception ex)
            {
                onError?.Invoke(ex);
            }
        });
    }

    /// <summary>Send CMD_SET_AUDIO_DEVICE (0x9B) with data 0/1/2.</summary>
    public static void SetAudioDevice(string host, int port, byte device,
        Action? onSent = null, Action<Exception>? onError = null)
    {
        if (device > 2)
            throw new ArgumentOutOfRangeException(nameof(device));
        SendOpcode(host, port, CmdSetAudioDevice, device, onSent, onError);
    }

    /// <summary>Send CMD_SET_COMPRESSION_STATE (0xEE) — 0 off, 1 on.</summary>
    public static void SetCompressionState(string host, int port, bool on,
        Action? onSent = null, Action<Exception>? onError = null)
        => SendOpcode(host, port, CmdSetCompressionState, (short)(on ? 1 : 0), onSent, onError);

    /// <summary>Send CMD_SET_COMPRESSION_LEVEL (0xEF) — 0..24.</summary>
    public static void SetCompressionLevel(string host, int port, int level,
        Action? onSent = null, Action<Exception>? onError = null)
    {
        level = Math.Clamp(level, CompressionLevelMin, CompressionLevelMax);
        SendOpcode(host, port, CmdSetCompressionLevel, (short)level, onSent, onError);
    }

    public static string AudioDeviceName(byte device) => device switch
    {
        AudioDigital => "Digital (0)",
        AudioPhones => "Phones (1)",
        AudioRemote => "Remote (2)",
        _ => $"Unknown ({device})",
    };
}
