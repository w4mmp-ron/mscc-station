using System.Buffers.Binary;

namespace MsccRemotePhones.Protocol;

/// <summary>
/// MSCC remote operator audio stream (v1) — MSA1 UDP.
/// RX phones: Pi (or test sender) → Windows player (default port 9100).
/// TX mic: Windows capture → Pi sdrcore-trans (default port 9101; Pi ingest TBD).
/// Digi is never on this path.
/// </summary>
public static class MsccAudioProtocol
{
    public const uint Magic = 0x3141534D; // 'MSA1' little-endian
    public const byte FormatS16Le = 0;
    public const int HeaderSize = 16;
    public const int DefaultPort = 9100;       // RX phones listen
    public const int DefaultTxPort = 9101;     // TX mic send (Windows → Pi)
    public const int DefaultSampleRate = 48000;
    public const int DefaultChannels = 1;

    public static bool TryParseHeader(ReadOnlySpan<byte> packet, out AudioPacketHeader header)
    {
        header = default;
        if (packet.Length < HeaderSize)
            return false;

        var magic = BinaryPrimitives.ReadUInt32LittleEndian(packet);
        if (magic != Magic)
            return false;

        header = new AudioPacketHeader
        {
            Sequence = BinaryPrimitives.ReadUInt16LittleEndian(packet.Slice(4)),
            FrameCount = BinaryPrimitives.ReadUInt16LittleEndian(packet.Slice(6)),
            Channels = packet[8],
            Format = packet[9],
            SampleRate = BinaryPrimitives.ReadUInt32LittleEndian(packet.Slice(10)),
            Reserved = BinaryPrimitives.ReadUInt16LittleEndian(packet.Slice(14)),
        };

        if (header.Channels is < 1 or > 2)
            return false;
        if (header.Format != FormatS16Le)
            return false;
        if (header.FrameCount == 0 || header.SampleRate < 8000)
            return false;

        var need = HeaderSize + header.FrameCount * header.Channels * 2;
        return packet.Length >= need;
    }

    public static int PayloadBytes(in AudioPacketHeader h)
        => h.FrameCount * h.Channels * sizeof(short);

    public static void WriteHeader(Span<byte> dest, in AudioPacketHeader h)
    {
        BinaryPrimitives.WriteUInt32LittleEndian(dest, Magic);
        BinaryPrimitives.WriteUInt16LittleEndian(dest.Slice(4), h.Sequence);
        BinaryPrimitives.WriteUInt16LittleEndian(dest.Slice(6), h.FrameCount);
        dest[8] = h.Channels;
        dest[9] = h.Format;
        BinaryPrimitives.WriteUInt32LittleEndian(dest.Slice(10), h.SampleRate);
        BinaryPrimitives.WriteUInt16LittleEndian(dest.Slice(14), h.Reserved);
    }
}

public struct AudioPacketHeader
{
    public ushort Sequence;
    public ushort FrameCount;   // samples per channel in this packet
    public byte Channels;       // 1 or 2
    public byte Format;         // 0 = s16le
    public uint SampleRate;
    public ushort Reserved;
}
