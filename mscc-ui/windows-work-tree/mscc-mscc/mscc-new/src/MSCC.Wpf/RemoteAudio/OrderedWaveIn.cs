// OrderedWaveIn is NAudio 2.2.1 WaveInEvent, adapted for MSCC.
// Copyright 2020 Mark Heath
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// One behaviour change from NAudio.Wave.WaveInEvent: on each wake, buffers are
// handed out and re-queued oldest first. NAudio scans the array 0..N-1, so a
// late wake can emit a newer buffer before an older one.

using NAudio;
using NAudio.CoreAudioApi;
using NAudio.Wave;

namespace MSCC.Wpf.RemoteAudio;

/// <summary>
/// WaveIn capture with event callbacks. Same surface as NAudio <see cref="WaveInEvent"/>,
/// except done buffers are returned in fill order.
/// </summary>
public sealed class OrderedWaveIn : IWaveIn
{
    private readonly AutoResetEvent _callbackEvent;
    private readonly SynchronizationContext? _syncContext;
    private IntPtr _waveInHandle;
    private volatile CaptureState _captureState;
    private WaveInBuffer[]? _buffers;
    private int _next;
    private int _lateWakes;
    private int _maxAtOnce;

    public event EventHandler<WaveInEventArgs>? DataAvailable;
    public event EventHandler<StoppedEventArgs>? RecordingStopped;

    public OrderedWaveIn()
    {
        _callbackEvent = new AutoResetEvent(false);
        _syncContext = SynchronizationContext.Current;
        DeviceNumber = 0;
        WaveFormat = new WaveFormat(8000, 16, 1);
        BufferMilliseconds = 100;
        NumberOfBuffers = 3;
        _captureState = CaptureState.Stopped;
    }

    public int BufferMilliseconds { get; set; }
    public int NumberOfBuffers { get; set; }
    public int DeviceNumber { get; set; }
    public WaveFormat WaveFormat { get; set; }

    /// <summary>
    /// Late wakes (more than one buffer ready) and the largest batch, then both reset to 0.
    /// </summary>
    public (int lateWakes, int maxAtOnce) TakeTimingStats()
    {
        int late = Interlocked.Exchange(ref _lateWakes, 0);
        int max = Interlocked.Exchange(ref _maxAtOnce, 0);
        return (late, max);
    }

    public void StartRecording()
    {
        if (_captureState != CaptureState.Stopped)
            throw new InvalidOperationException("Already recording");
        OpenWaveInDevice();
        MmException.Try(WaveInterop.waveInStart(_waveInHandle), "waveInStart");
        _captureState = CaptureState.Starting;
        ThreadPool.QueueUserWorkItem(_ => RecordThread());
    }

    public void StopRecording()
    {
        if (_captureState != CaptureState.Stopped)
        {
            _captureState = CaptureState.Stopping;
            MmException.Try(WaveInterop.waveInStop(_waveInHandle), "waveInStop");
            MmException.Try(WaveInterop.waveInReset(_waveInHandle), "waveInReset");
            _callbackEvent.Set();
        }
    }

    public void Dispose()
    {
        Dispose(true);
        GC.SuppressFinalize(this);
    }

    private void Dispose(bool disposing)
    {
        if (disposing)
        {
            if (_captureState != CaptureState.Stopped)
                StopRecording();
            CloseWaveInDevice();
        }
    }

    private void OpenWaveInDevice()
    {
        CloseWaveInDevice();
        MmResult result = WaveInterop.waveInOpenWindow(
            out _waveInHandle,
            (IntPtr)DeviceNumber,
            WaveFormat,
            _callbackEvent.SafeWaitHandle.DangerousGetHandle(),
            IntPtr.Zero,
            WaveInterop.WaveInOutOpenFlags.CallbackEvent);
        MmException.Try(result, "waveInOpen");
        CreateBuffers();
    }

    private void CreateBuffers()
    {
        int bufferSize = BufferMilliseconds * WaveFormat.AverageBytesPerSecond / 1000;
        if (bufferSize % WaveFormat.BlockAlign != 0)
            bufferSize -= bufferSize % WaveFormat.BlockAlign;

        _buffers = new WaveInBuffer[NumberOfBuffers];
        for (int n = 0; n < _buffers.Length; n++)
            _buffers[n] = new WaveInBuffer(_waveInHandle, bufferSize);
        _next = 0;
    }

    private void RecordThread()
    {
        Exception? exception = null;
        try
        {
            DoRecording();
        }
        catch (Exception e)
        {
            exception = e;
        }
        finally
        {
            _captureState = CaptureState.Stopped;
            RaiseRecordingStoppedEvent(exception);
        }
    }

    private void DoRecording()
    {
        var buffers = _buffers ?? throw new InvalidOperationException("WaveIn buffers were not created.");
        _captureState = CaptureState.Capturing;
        foreach (var buffer in buffers)
        {
            if (!buffer.InQueue)
                buffer.Reuse();
        }
        _next = 0;

        while (_captureState == CaptureState.Capturing)
        {
            if (!_callbackEvent.WaitOne())
                continue;

            int handed = 0;
            int n = buffers.Length;
            // One ring only. Reuse clears Done, so a buffer that stays Done cannot spin.
            for (int i = 0; i < n && buffers[_next].Done; i++)
            {
                var buffer = buffers[_next];
                if (buffer.BytesRecorded > 0)
                    DataAvailable?.Invoke(this, new WaveInEventArgs(buffer.Data, buffer.BytesRecorded));

                if (_captureState == CaptureState.Capturing)
                    buffer.Reuse();

                _next = (_next + 1) % n;
                handed++;
            }

            if (handed > 1)
            {
                Interlocked.Increment(ref _lateWakes);
                int seen;
                do
                {
                    seen = Volatile.Read(ref _maxAtOnce);
                    if (handed <= seen)
                        break;
                }
                while (Interlocked.CompareExchange(ref _maxAtOnce, handed, seen) != seen);
            }
        }
    }

    private void RaiseRecordingStoppedEvent(Exception? e)
    {
        var handler = RecordingStopped;
        if (handler == null)
            return;
        if (_syncContext == null)
            handler(this, new StoppedEventArgs(e));
        else
            _syncContext.Post(_ => handler(this, new StoppedEventArgs(e)), null);
    }

    private void CloseWaveInDevice()
    {
        WaveInterop.waveInReset(_waveInHandle);
        if (_buffers != null)
        {
            for (int n = 0; n < _buffers.Length; n++)
                _buffers[n].Dispose();
            _buffers = null;
        }
        WaveInterop.waveInClose(_waveInHandle);
        _waveInHandle = IntPtr.Zero;
    }
}
