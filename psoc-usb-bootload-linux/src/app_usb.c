/*
 * Vendor control transfers to the running Proficio app (libusb-1.0).
 * Used for CMD_ENTER_BOOTLOADER / CMD_REBOOT_APP before HID programming.
 */
#include "app_usb.h"

#include <stdio.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

/* Same wValue ms-sdr uses for many Radio_send_parameters calls */
#define PROFICIO_WVALUE 0x071B

int app_usb_vendor_out(unsigned short vid, unsigned short pid,
    unsigned char bRequest, const void *data, int len)
{
    libusb_context *ctx = NULL;
    libusb_device_handle *h = NULL;
    unsigned char pad[4];
    int r, transferred;
    int i;

    memset(pad, 0, sizeof(pad));
    if (data && len > 0) {
        if (len > (int)sizeof(pad))
            len = (int)sizeof(pad);
        memcpy(pad, data, (size_t)len);
    } else {
        len = (int)sizeof(pad);
    }

    r = libusb_init(&ctx);
    if (r != 0) {
        fprintf(stderr, "libusb_init failed: %s\n", libusb_strerror(r));
        return 1;
    }

    h = libusb_open_device_with_vid_pid(ctx, vid, pid);
    if (!h) {
        fprintf(stderr,
            "No Proficio application USB device %04x:%04x\n"
            "  (Is the radio plugged in and running app firmware?\n"
            "   Stop ms-sdr if it has the device open.)\n",
            vid, pid);
        libusb_exit(ctx);
        return 1;
    }

    /*
     * Do NOT claim interface 0 — on Proficio that is often USB Audio.
     * Device-recipient vendor control uses EP0 only.
     * Detach kernel drivers on all interfaces so snd-usb-audio does not
     * interfere with opening the device.
     */
    for (i = 0; i < 8; i++) {
        if (libusb_kernel_driver_active(h, i) == 1)
            (void)libusb_detach_kernel_driver(h, i);
    }

    transferred = libusb_control_transfer(
        h,
        LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
        bRequest,
        PROFICIO_WVALUE,
        0,
        pad,
        (uint16_t)len,
        3000);

    /*
     * PIPE/NO_DEVICE after reboot commands often means the MCU already reset
     * mid-STATUS stage — treat as success for 0x0E / 0x0F.
     */
    if (transferred < 0) {
        if ((bRequest == CMD_ENTER_BOOTLOADER || bRequest == CMD_REBOOT_APP) &&
            (transferred == LIBUSB_ERROR_PIPE ||
             transferred == LIBUSB_ERROR_NO_DEVICE ||
             transferred == LIBUSB_ERROR_IO ||
             transferred == LIBUSB_ERROR_TIMEOUT)) {
            printf("Vendor OUT 0x%02X: transfer ended with %s\n"
                   "  (Often normal — device reset during USB STATUS.)\n"
                   "  Check: did the radio reboot / re-enumerate?\n",
                bRequest, libusb_strerror(transferred));
            libusb_close(h);
            libusb_exit(ctx);
            return 0;
        }
        fprintf(stderr, "Vendor OUT 0x%02X failed: %s\n",
            bRequest, libusb_strerror(transferred));
        if (transferred == LIBUSB_ERROR_PIPE) {
            fprintf(stderr,
                "  STALL usually means: old app firmware (no 0x0E/0x0F), or\n"
                "  ms-sdr holding USB — stop servers and retry.\n");
        }
        libusb_close(h);
        libusb_exit(ctx);
        return 1;
    }

    printf("Sent vendor OUT 0x%02X (%d bytes) to %04x:%04x\n",
        bRequest, transferred, vid, pid);

    libusb_close(h);
    libusb_exit(ctx);
    return 0;
}
