/**
 * USB vendor request dispatch (stack-agnostic).
 * TinyUSB / Cube USB should call usb_vendor_handle_* from setup stage.
 */
#ifndef USB_VENDOR_H
#define USB_VENDOR_H

#include <stdint.h>
#include <stddef.h>

#define USB_DIR_D2H  0x80u
#define USB_DIR_H2D  0x00u

typedef struct {
    uint8_t  *data;
    uint16_t  len;
    uint8_t   handled;   /* 1 if opcode known */
} usb_vendor_xfer_t;

void usb_vendor_init(void);
void usb_vendor_poll(void);

/**
 * Handle vendor SETUP.
 * @param bmRequestType  standard USB bmRequestType
 * @param bRequest       opcode (usbvend.h)
 * @param wValue         wValue
 * @param wIndex         wIndex
 * @param wLength        host-requested length
 * @param buf            data stage buffer (IN: fill; OUT: host payload after)
 * @param buf_cap        buffer capacity
 * @return bytes for IN stage, or 0 for OUT/no-data; xfer.handled set
 *
 * For OUT with wLength>0: call again after data stage with
 * usb_vendor_complete_out() once payload is in buf.
 */
usb_vendor_xfer_t usb_vendor_setup(uint8_t bmRequestType, uint8_t bRequest,
                                   uint16_t wValue, uint16_t wIndex,
                                   uint16_t wLength, uint8_t *buf, uint16_t buf_cap);

/** Apply OUT data stage after host wrote into buf. */
void usb_vendor_complete_out(uint8_t bRequest, const uint8_t *buf, uint16_t len);

uint8_t emulated_register(void);

#endif /* USB_VENDOR_H */
