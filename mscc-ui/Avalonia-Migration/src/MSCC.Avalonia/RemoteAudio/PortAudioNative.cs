using System.Runtime.InteropServices;

namespace MSCC.Avalonia.RemoteAudio;

/// <summary>Minimal PortAudio C API for Linux (libportaudio.so.2 / Pulse).</summary>
internal static class PortAudioNative
{
    public const uint Float32 = 0x00000001;
    public const uint FramesUnspecified = 0;
    public const int NoDevice = -1;
    public const int NoError = 0;

    private const string Lib = "portaudio";

    [StructLayout(LayoutKind.Sequential)]
    public struct StreamParameters
    {
        public int device;
        public int channelCount;
        public uint sampleFormat;
        public double suggestedLatency;
        public IntPtr hostApiSpecificStreamInfo;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DeviceInfo
    {
        public int structVersion;
        public IntPtr name;
        public int hostApi;
        public int maxInputChannels;
        public int maxOutputChannels;
        public double defaultLowInputLatency;
        public double defaultLowOutputLatency;
        public double defaultHighInputLatency;
        public double defaultHighOutputLatency;
        public double defaultSampleRate;
    }

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    public static extern int Pa_Initialize();

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    public static extern int Pa_Terminate();

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    public static extern int Pa_GetDeviceCount();

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    public static extern int Pa_GetDefaultOutputDevice();

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    public static extern int Pa_GetDefaultInputDevice();

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr Pa_GetDeviceInfo(int device);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr Pa_GetErrorText(int errorCode);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    public static extern int Pa_OpenStream(
        out IntPtr stream,
        IntPtr inputParameters,
        IntPtr outputParameters,
        double sampleRate,
        uint framesPerBuffer,
        uint streamFlags,
        IntPtr streamCallback,
        IntPtr userData);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    public static extern int Pa_StartStream(IntPtr stream);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    public static extern int Pa_StopStream(IntPtr stream);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    public static extern int Pa_CloseStream(IntPtr stream);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    public static extern int Pa_WriteStream(IntPtr stream, float[] buffer, uint frames);

    [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
    public static extern int Pa_ReadStream(IntPtr stream, float[] buffer, uint frames);

    public static string Err(int code)
    {
        try
        {
            var p = Pa_GetErrorText(code);
            return p == IntPtr.Zero ? $"PaError {code}" : Marshal.PtrToStringAnsi(p) ?? $"PaError {code}";
        }
        catch
        {
            return $"PaError {code}";
        }
    }

    public static string DeviceName(int device)
    {
        var p = Pa_GetDeviceInfo(device);
        if (p == IntPtr.Zero) return $"#{device}";
        var info = Marshal.PtrToStructure<DeviceInfo>(p);
        return Marshal.PtrToStringAnsi(info.name) ?? $"#{device}";
    }

    public static DeviceInfo? Info(int device)
    {
        var p = Pa_GetDeviceInfo(device);
        if (p == IntPtr.Zero) return null;
        return Marshal.PtrToStructure<DeviceInfo>(p);
    }

    private static int _ref;

    public static void AddRef()
    {
        if (Interlocked.Increment(ref _ref) == 1)
        {
            int e = Pa_Initialize();
            if (e != NoError)
            {
                Interlocked.Decrement(ref _ref);
                throw new InvalidOperationException("PortAudio init: " + Err(e));
            }
        }
    }

    public static void Release()
    {
        if (Interlocked.Decrement(ref _ref) == 0)
        {
            try { Pa_Terminate(); } catch { /* ignore */ }
        }
    }
}
