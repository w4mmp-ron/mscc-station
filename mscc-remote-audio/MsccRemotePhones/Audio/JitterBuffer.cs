using System.Collections.Concurrent;
using MsccRemotePhones.Protocol;

namespace MsccRemotePhones.Audio;

/// <summary>
/// Thread-safe queue of PCM frames (already de-packetized).
/// Simple FIFO after reordering by sequence.
/// </summary>
public sealed class JitterBuffer
{
    private readonly object _gate = new();
    private readonly SortedDictionary<ushort, short[]> _ordered = new();
    private readonly Queue<short> _sampleQ = new(); // interleaved stream after reordering
    private ushort? _nextSeq;
    private bool _started;
    private int _channels = 1;

    public int PrebufferSamples { get; set; } = 48000 * 80 / 1000; // 80 ms @ 48k mono
    public int DroppedPackets { get; private set; }
    public int ReceivedPackets { get; private set; }
    public int QueuedSamples
    {
        get { lock (_gate) return _sampleQ.Count; }
    }

    public void Clear()
    {
        lock (_gate)
        {
            _ordered.Clear();
            _sampleQ.Clear();
            _nextSeq = null;
            _started = false;
            DroppedPackets = 0;
            ReceivedPackets = 0;
        }
    }

    public void Push(in AudioPacketHeader hdr, ReadOnlySpan<byte> pcmBytes)
    {
        ReceivedPackets++;
        int n = pcmBytes.Length / 2;
        var pcm = new short[n];
        for (int i = 0; i < n; i++)
            pcm[i] = (short)(pcmBytes[i * 2] | (pcmBytes[i * 2 + 1] << 8));

        lock (_gate)
        {
            _channels = hdr.Channels is 1 or 2 ? hdr.Channels : 1;

            if (_ordered.Count > 200)
            {
                _ordered.Clear();
                _sampleQ.Clear();
                _nextSeq = hdr.Sequence;
                _started = false;
            }

            _ordered[hdr.Sequence] = pcm;
            DrainOrderedToQueue();
        }
    }

    private void DrainOrderedToQueue()
    {
        if (_nextSeq is null && _ordered.Count > 0)
            _nextSeq = _ordered.Keys.First();

        while (_nextSeq is not null && _ordered.Remove(_nextSeq.Value, out var block))
        {
            foreach (var s in block)
                _sampleQ.Enqueue(s);
            _nextSeq = (ushort)(_nextSeq.Value + 1);
        }

        // skip small gaps if next packets are waiting
        if (_nextSeq is not null && _ordered.Count > 0 && !_ordered.ContainsKey(_nextSeq.Value))
        {
            var first = _ordered.Keys.First();
            var gap = (ushort)(first - _nextSeq.Value);
            if (gap is > 0 and < 16)
            {
                DroppedPackets += gap;
                _nextSeq = first;
                DrainOrderedToQueue();
            }
        }

        if (!_started && _sampleQ.Count >= PrebufferSamples)
            _started = true;
    }

    /// <summary>
    /// Fill interleaved PCM (source channel count). Returns sample count written (not frames).
    /// Pads with zeros if underrun after started; returns silence if not yet prebuffered.
    /// </summary>
    public int ReadSamples(Span<short> dest, int sourceChannels)
    {
        lock (_gate)
        {
            if (!_started)
            {
                dest.Clear();
                return dest.Length;
            }

            int i = 0;
            while (i < dest.Length)
            {
                if (_sampleQ.Count > 0)
                    dest[i++] = _sampleQ.Dequeue();
                else
                    dest[i++] = 0;
            }
            return dest.Length;
        }
    }

    public int SourceChannels
    {
        get { lock (_gate) return _channels; }
    }
}
