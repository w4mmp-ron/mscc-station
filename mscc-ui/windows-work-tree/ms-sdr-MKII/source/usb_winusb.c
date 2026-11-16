/*
 * usb_winusb.c - the radio over WinUSB (libusb-1.0 1.0.30), 2026-11-16.
 * See usb_winusb.h. Based on the Pi ms-sdr usbcmd.c libusb-1.0 code.
 */
#include <stddef.h>
#include "libusb-1.0/libusb.h"
#include "usb_winusb.h"

static libusb_context *s_ctx = NULL;
static libusb_device_handle *s_h = NULL;
static struct libusb_device_descriptor s_desc;
static int s_address = -1;

int wusb_open(int vid, int pid, int nth)
{
    libusb_device **list = NULL;
    ssize_t n, i;
    int r, seen = 0;

    wusb_close();
    if (s_ctx == NULL) {
        r = libusb_init_context(&s_ctx, NULL, 0);
        if (r != 0) {
            s_ctx = NULL;
            return r;
        }
    }

    n = libusb_get_device_list(s_ctx, &list);
    if (n < 0) {
        return (int)n;
    }
    r = LIBUSB_ERROR_NOT_FOUND;
    for (i = 0; i < n; i++) {
        struct libusb_device_descriptor d;
        if (libusb_get_device_descriptor(list[i], &d) != 0) {
            continue;
        }
        if (d.idVendor != (uint16_t)vid || d.idProduct != (uint16_t)pid) {
            continue;
        }
        if (seen++ != nth) {
            continue;
        }
        r = libusb_open(list[i], &s_h);
        if (r != 0) {
            s_h = NULL;
            break;
        }
        r = libusb_claim_interface(s_h, 0);
        if (r != 0) {
            libusb_close(s_h);
            s_h = NULL;
            break;
        }
        s_desc = d;
        s_address = libusb_get_device_address(list[i]);
        break;
    }
    libusb_free_device_list(list, 1);
    return r;
}

void wusb_device_info(int *iManufacturer, int *iProduct, int *iSerialNumber, int *address)
{
    *iManufacturer = s_desc.iManufacturer;
    *iProduct = s_desc.iProduct;
    *iSerialNumber = s_desc.iSerialNumber;
    *address = s_address;
}

int wusb_vendor(int in, int cmd, int value, int index, char *buffer, int buflen, int timeout_ms)
{
    if (s_h == NULL) {
        return LIBUSB_ERROR_NO_DEVICE;
    }
    return libusb_control_transfer(s_h,
            (uint8_t)((in ? LIBUSB_ENDPOINT_IN : LIBUSB_ENDPOINT_OUT) |
                      LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE),
            (uint8_t)cmd, (uint16_t)value, (uint16_t)index,
            (unsigned char *)buffer, (uint16_t)buflen, (unsigned int)timeout_ms);
}

int wusb_string_raw(int index, int langid, char *buffer, int buflen, int timeout_ms)
{
    if (s_h == NULL) {
        return LIBUSB_ERROR_NO_DEVICE;
    }
    return libusb_control_transfer(s_h, LIBUSB_ENDPOINT_IN, LIBUSB_REQUEST_GET_DESCRIPTOR,
            (uint16_t)((LIBUSB_DT_STRING << 8) | (index & 0xFF)), (uint16_t)langid,
            (unsigned char *)buffer, (uint16_t)buflen, (unsigned int)timeout_ms);
}

void wusb_close(void)
{
    if (s_h != NULL) {
        libusb_release_interface(s_h, 0);
        libusb_close(s_h);
        s_h = NULL;
    }
    s_address = -1;
}

const char *wusb_error_name(int r)
{
    return libusb_error_name(r);
}
