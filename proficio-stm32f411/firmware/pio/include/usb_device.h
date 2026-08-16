/**
 * USB device: vendor (0xFF) + UAC1 audio (Proficio-compatible layout).
 * VID 0x16C0 PID 0x05DC — matches PSoC USBFS descriptors.
 */
#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <stdint.h>

#define PROFICIO_USB_VID  0x16C0u
#define PROFICIO_USB_PID  0x05DCu

void usb_device_init(void);
void usb_device_poll(void);
uint8_t usb_device_configured(void);

/** True when host enabled streaming alts (TX interface alt 1). */
uint8_t usb_audio_tx_enabled(void);
uint8_t usb_audio_rx_enabled(void);

#endif /* USB_DEVICE_H */
