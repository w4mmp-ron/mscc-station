using System.Net.Sockets;
using MsccRemotePhones.Protocol;

namespace MsccRemotePhones.Audio;

public sealed class UdpAudioReceiver : IDisposable
{
    private UdpClient? _udp;
    private CancellationTokenSource? _cts;
    private Thread? _thread;
    private readonly JitterBuffer _buffer;

    public event Action<string>? Log;
    public event Action<AudioPacketHeader>? PacketAccepted;

    public JitterBuffer Buffer => _buffer;
    public bool IsRunning { get; private set; }
    public int ListenPort { get; private set; }
    public int BadPackets { get; private set; }

    public UdpAudioReceiver(JitterBuffer buffer) => _buffer = buffer;

    public void Start(int port)
    {
        Stop();
        ListenPort = port;
        BadPackets = 0;
        _udp = new UdpClient(port);
        _udp.Client.ReceiveBufferSize = 1 << 20;
        _cts = new CancellationTokenSource();
        IsRunning = true;
        var token = _cts.Token;
        _thread = new Thread(() => ReceiveLoop(token))
        {
            IsBackground = true,
            Name = "MsccUdpAudio",
            Priority = ThreadPriority.AboveNormal,
        };
        _thread.Start();
        Log?.Invoke($"Listening UDP port {port}");
    }

    public void Stop()
    {
        IsRunning = false;
        try { _cts?.Cancel(); } catch { /* ignore */ }
        try { _udp?.Close(); } catch { /* ignore */ }
        try { _thread?.Join(500); } catch { /* ignore */ }
        _udp?.Dispose();
        _cts?.Dispose();
        _udp = null;
        _cts = null;
        _thread = null;
        _buffer.Clear();
        Log?.Invoke("Receiver stopped");
    }

    private void ReceiveLoop(CancellationToken token)
    {
        var remote = new System.Net.IPEndPoint(System.Net.IPAddress.Any, 0);
        while (!token.IsCancellationRequested && _udp is not null)
        {
            try
            {
                var data = _udp.Receive(ref remote);
                if (!MsccAudioProtocol.TryParseHeader(data, out var hdr))
                {
                    BadPackets++;
                    continue;
                }
                var payload = data.AsSpan(MsccAudioProtocol.HeaderSize, MsccAudioProtocol.PayloadBytes(hdr));
                _buffer.Push(hdr, payload);
                PacketAccepted?.Invoke(hdr);
            }
            catch (SocketException)
            {
                if (token.IsCancellationRequested) break;
            }
            catch (ObjectDisposedException)
            {
                break;
            }
            catch (Exception ex)
            {
                if (!token.IsCancellationRequested)
                    Log?.Invoke($"Receive error: {ex.Message}");
            }
        }
    }

    public void Dispose() => Stop();
}
