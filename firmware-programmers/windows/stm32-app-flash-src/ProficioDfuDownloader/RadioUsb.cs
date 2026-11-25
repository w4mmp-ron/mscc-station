using System.Runtime.InteropServices;

namespace ProficioDfuDownloader;

// Talks to the running Black Pill app (16C0:05DC) through libusb-1.0.dll, the copy that
// sits beside dfu-util. Pill 3.17 and later bind WinUSB by themselves, so no Zadig.
// Same requests as the Pi stm32-app-flash script (2026-11-24):
//   CMD_GET_VERSION 0x00 (vendor IN, 2 bytes: major, minor)
//   0xFE = go to flash mode (the pill resets into its DFU bootloader)
internal static class RadioUsb
{
    private const string Lib = "libusb-1.0.dll";
    private const ushort Vid = 0x16C0;
    private const ushort Pid = 0x05DC;

    [DllImport(Lib)] private static extern int libusb_init(out IntPtr ctx);
    [DllImport(Lib)] private static extern void libusb_exit(IntPtr ctx);
    [DllImport(Lib)] private static extern IntPtr libusb_open_device_with_vid_pid(IntPtr ctx, ushort vid, ushort pid);
    [DllImport(Lib)] private static extern void libusb_close(IntPtr handle);
    [DllImport(Lib)]
    private static extern int libusb_control_transfer(IntPtr handle, byte requestType, byte request,
        ushort value, ushort index, byte[]? data, ushort length, uint timeoutMs);

    // Runs fn with an open radio handle; returns default if the radio is not there.
    private static T? WithRadio<T>(Func<IntPtr, T?> fn)
    {
        IntPtr ctx = IntPtr.Zero;
        try
        {
            if (libusb_init(out ctx) != 0)
                return default;
            var h = libusb_open_device_with_vid_pid(ctx, Vid, Pid);
            if (h == IntPtr.Zero)
                return default;
            try { return fn(h); }
            finally { libusb_close(h); }
        }
        catch (Exception) // DllNotFound etc.: treat as "no radio"
        {
            return default;
        }
        finally
        {
            if (ctx != IntPtr.Zero)
                libusb_exit(ctx);
        }
    }

    // "3.28", or null if no radio app answers.
    public static string? Version() => WithRadio<string>(h =>
    {
        var buf = new byte[2];
        int n = libusb_control_transfer(h, 0xC0, 0x00, 0, 0, buf, 2, 500);
        return n == 2 ? $"{buf[0]}.{buf[1]}" : null;
    });

    // Sends 0xFE. True if the radio was found (the pill may reset before it answers,
    // so the transfer result itself is not checked).
    public static bool SendFlashMode() => WithRadio<object>(h =>
    {
        libusb_control_transfer(h, 0x40, 0xFE, 0, 0, null, 0, 500);
        return new object();
    }) is not null;
}
