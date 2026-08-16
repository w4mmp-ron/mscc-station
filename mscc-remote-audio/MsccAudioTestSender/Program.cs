using System.Net;
using System.Net.Sockets;
using System.Buffers.Binary;
using System.Diagnostics;

namespace MsccAudioTestSender;

/// <summary>
/// Sends a 440 Hz tone using MSCC remote-phones UDP packets (MSA1).
/// Paced with Stopwatch so packet timing matches sample rate (reduces underrun distortion).
/// </summary>
static class Program
{
    const uint Magic = 0x3141534D; // MSA1
    const int HeaderSize = 16;
    const int DefaultPort = 9100;
    const int SampleRate = 48000;
    const int Channels = 1;
    const int FramesPerPacket = 480; // 10 ms

    static void Main(string[] args)
    {
        string host = args.Length > 0 ? args[0] : "127.0.0.1";
        int port = args.Length > 1 && int.TryParse(args[1], out var p) ? p : DefaultPort;
        double seconds = args.Length > 2 && double.TryParse(args[2], out var s) ? s : 30;

        Console.WriteLine($"MSCC Audio Test Sender → {host}:{port}");
        Console.WriteLine($"440 Hz sine, {SampleRate} Hz mono s16le, {FramesPerPacket} frames/pkt, {seconds}s");
        Console.WriteLine("Ctrl+C to stop early.");

        using var udp = new UdpClient();
        var ep = new IPEndPoint(IPAddress.Parse(host), port);
        ushort seq = 0;
        long totalFrames = 0;
        var phase = 0.0;
        var phaseInc = 2 * Math.PI * 440.0 / SampleRate;
        var packet = new byte[HeaderSize + FramesPerPacket * Channels * 2];

        double packetSec = (double)FramesPerPacket / SampleRate;
        var sw = Stopwatch.StartNew();
        double nextT = 0;
        var endTicks = sw.Elapsed.TotalSeconds + seconds;

        while (sw.Elapsed.TotalSeconds < endTicks)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(0), Magic);
            BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4), seq);
            BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(6), (ushort)FramesPerPacket);
            packet[8] = Channels;
            packet[9] = 0; // s16le
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(10), SampleRate);
            BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(14), 0);

            int o = HeaderSize;
            for (int i = 0; i < FramesPerPacket; i++)
            {
                // modest level to avoid any DAC harshness
                short sample = (short)(Math.Sin(phase) * 6000.0);
                phase += phaseInc;
                if (phase > 2 * Math.PI) phase -= 2 * Math.PI;
                packet[o++] = (byte)(sample & 0xFF);
                packet[o++] = (byte)((sample >> 8) & 0xFF);
            }

            udp.Send(packet, packet.Length, ep);
            seq++;
            totalFrames += FramesPerPacket;

            nextT += packetSec;
            while (sw.Elapsed.TotalSeconds < nextT)
                Thread.SpinWait(50);
        }

        Console.WriteLine($"Done. Sent {seq} packets, {totalFrames} frames.");
    }
}
