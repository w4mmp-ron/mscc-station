using System.IO;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace MSCC.Wpf.RemoteAudio;

/// <summary>
/// Win32 CreateFile("\\.\COMx") — same path as ms-sdr. .NET SerialPort rejects
/// \\.\COM5 and throws FileNotFound on Eltima/com0com.
/// </summary>
internal sealed class NativeComPort : IDisposable
{
    private const uint GenericRead = 0x80000000;
    private const uint GenericWrite = 0x40000000;
    private const uint OpenExisting = 3;
    private const byte OneStopBit = 0;
    private const byte NoParity = 0;

    private SafeFileHandle? _handle;

    public bool IsOpen => _handle is { IsInvalid: false, IsClosed: false };

    public void Open(string comName, int baud)
    {
        Close();
        string path = comName.StartsWith(@"\\.\", StringComparison.Ordinal)
            ? comName
            : @"\\.\" + comName.Trim();
        var h = CreateFile(path, GenericRead | GenericWrite, 0, IntPtr.Zero, OpenExisting, 0, IntPtr.Zero);
        if (h.IsInvalid)
        {
            int err = Marshal.GetLastWin32Error();
            h.Dispose();
            if (err == 2 && TryDefineDosDevice(comName.Trim()))
            {
                h = CreateFile(path, GenericRead | GenericWrite, 0, IntPtr.Zero, OpenExisting, 0, IntPtr.Zero);
                err = h.IsInvalid ? Marshal.GetLastWin32Error() : 0;
            }
            if (h.IsInvalid)
            {
                h.Dispose();
                throw new IOException(
                    $"CreateFile {path} failed Win32 {err} (COM5 DOS name missing after reboot is a known Eltima issue; retried DefineDosDevice)");
            }
        }

        var dcb = new Dcb { DCBlength = (uint)Marshal.SizeOf<Dcb>() };
        if (!GetCommState(h, ref dcb))
        {
            int err = Marshal.GetLastWin32Error();
            h.Dispose();
            throw new IOException($"GetCommState failed Win32 {err}");
        }
        dcb.BaudRate = (uint)baud;
        dcb.ByteSize = 8;
        dcb.Parity = NoParity;
        dcb.StopBits = OneStopBit;
        dcb.Flags = 1; // fBinary
        if (!SetCommState(h, ref dcb))
        {
            int err = Marshal.GetLastWin32Error();
            h.Dispose();
            throw new IOException($"SetCommState failed Win32 {err}");
        }

        var to = new CommTimeouts
        {
            ReadIntervalTimeout = 0,
            ReadTotalTimeoutMultiplier = 0,
            ReadTotalTimeoutConstant = 200,
            WriteTotalTimeoutMultiplier = 0,
            WriteTotalTimeoutConstant = 200,
        };
        SetCommTimeouts(h, ref to);
        _handle = h;
    }

    /// <returns>0–255, or -1 on timeout / no data.</returns>
    public int ReadByte()
    {
        if (_handle == null || _handle.IsInvalid)
            return -1;
        byte[] buf = new byte[1];
        if (!ReadFile(_handle, buf, 1, out uint n, IntPtr.Zero) || n == 0)
            return -1;
        return buf[0];
    }

    public void Write(byte[] data)
    {
        if (_handle == null || data.Length == 0)
            return;
        WriteFile(_handle, data, (uint)data.Length, out _, IntPtr.Zero);
    }

    public void Close()
    {
        try { _handle?.Dispose(); } catch { /* ignore */ }
        _handle = null;
    }

    public void Dispose() => Close();

    /// <summary>
    /// Eltima/com0com sometimes lists COMx in SERIALCOMM but never creates \DosDevices\COMx
    /// (QueryDosDevice → 2). Recreate the name from the registry mapping.
    /// </summary>
    private static bool TryDefineDosDevice(string comName)
    {
        try
        {
            using var key = Microsoft.Win32.Registry.LocalMachine.OpenSubKey(@"HARDWARE\DEVICEMAP\SERIALCOMM");
            if (key == null)
                return false;
            string? ntDevice = null;
            foreach (string valueName in key.GetValueNames())
            {
                if (string.Equals(key.GetValue(valueName) as string, comName, StringComparison.OrdinalIgnoreCase))
                {
                    ntDevice = valueName;
                    break;
                }
            }
            if (string.IsNullOrEmpty(ntDevice))
                return false;
            // DDD_RAW_TARGET_PATH = 1
            return DefineDosDevice(1, comName, ntDevice);
        }
        catch
        {
            return false;
        }
    }

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern bool DefineDosDevice(uint dwFlags, string lpDeviceName, string lpTargetPath);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern SafeFileHandle CreateFile(
        string lpFileName, uint dwDesiredAccess, uint dwShareMode,
        IntPtr lpSecurityAttributes, uint dwCreationDisposition,
        uint dwFlagsAndAttributes, IntPtr hTemplateFile);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool GetCommState(SafeFileHandle hFile, ref Dcb lpDCB);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool SetCommState(SafeFileHandle hFile, ref Dcb lpDCB);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool SetCommTimeouts(SafeFileHandle hFile, ref CommTimeouts lpCommTimeouts);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool ReadFile(SafeFileHandle hFile, byte[] lpBuffer, uint nNumberOfBytesToRead,
        out uint lpNumberOfBytesRead, IntPtr lpOverlapped);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool WriteFile(SafeFileHandle hFile, byte[] lpBuffer, uint nNumberOfBytesToWrite,
        out uint lpNumberOfBytesWritten, IntPtr lpOverlapped);

    [StructLayout(LayoutKind.Sequential)]
    private struct Dcb
    {
        public uint DCBlength;
        public uint BaudRate;
        public uint Flags;
        public ushort wReserved;
        public ushort XonLim;
        public ushort XoffLim;
        public byte ByteSize;
        public byte Parity;
        public byte StopBits;
        public byte XonChar;
        public byte XoffChar;
        public byte ErrorChar;
        public byte EofChar;
        public byte EvtChar;
        public ushort wReserved1;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct CommTimeouts
    {
        public uint ReadIntervalTimeout;
        public uint ReadTotalTimeoutMultiplier;
        public uint ReadTotalTimeoutConstant;
        public uint WriteTotalTimeoutMultiplier;
        public uint WriteTotalTimeoutConstant;
    }
}
