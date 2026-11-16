/*
 * usb_winusb.h - the radio over WinUSB (libusb-1.0), 2026-11-16.
 *
 * Pill firmware 3.17+ asks Windows for WinUSB itself (Microsoft OS 2.0 descriptors),
 * so no Zadig is needed. usbcmd.c tries this first and falls back to libusb-win32
 * (the Zadig driver) when the radio is not on WinUSB. Kept in its own file so the
 * libusb-1.0 and libusb-win32 headers never meet.
 */
#ifndef USB_WINUSB_H
#define USB_WINUSB_H

/* Open the n-th (0-based) device with this VID/PID and claim interface 0.
 * Returns 0 if open, or a negative libusb error (-5 = no such device). */
int wusb_open(int vid, int pid, int nth);

/* Device descriptor string indexes and bus address of the open device. */
void wusb_device_info(int *iManufacturer, int *iProduct, int *iSerialNumber, int *address);

/* Vendor control transfer to the device. in = 1 device-to-host.
 * Returns bytes transferred, or a negative libusb error. */
int wusb_vendor(int in, int cmd, int value, int index, char *buffer, int buflen, int timeout_ms);

/* GET_DESCRIPTOR string, raw descriptor bytes into buffer. Returns length or < 0. */
int wusb_string_raw(int index, int langid, char *buffer, int buflen, int timeout_ms);

/* Release interface 0 and close (the libusb context is kept). */
void wusb_close(void);

/* Short name of a libusb error code, for the log. */
const char *wusb_error_name(int r);

#endif /* USB_WINUSB_H */
