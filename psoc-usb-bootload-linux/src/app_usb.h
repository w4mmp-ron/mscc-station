#ifndef APP_USB_H
#define APP_USB_H

/* Running Proficio application (ms-sdr / Multus) */
#define PROFICIO_APP_VID 0x16C0
#define PROFICIO_APP_PID 0x05DC

/* Must match PSoC usbvend.h */
#define CMD_ENTER_BOOTLOADER 0x0E
#define CMD_REBOOT_APP       0x0F

/*
 * Vendor OUT to the running application (not HID bootloader).
 * Returns 0 on success, non-zero on failure.
 */
int app_usb_vendor_out(unsigned short vid, unsigned short pid,
    unsigned char bRequest, const void *data, int len);

#endif
