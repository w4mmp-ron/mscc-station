/**
 * USB descriptors — vendor + UAC2 stereo (TinyUSB).
 */
#include "tusb.h"
#include "usb_descriptors.h"
#include "proficio_config.h"
#include <string.h>

#define USB_VID   0x16C0u
#define USB_PID   0x05DCu

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

enum {
    CONFIG_TOTAL_LEN = TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN + TUD_AUDIO_PROFICIO_DESC_LEN
};

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_SELF_POWERED, 100),
    /* stridx 7 = "Multus Control" (was wrongly 6 = Capture — Zadig/Device Manager name) */
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 7, EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN, 64),
    TUD_AUDIO_PROFICIO_DESCRIPTOR(4, EPNUM_AUDIO_OUT, EPNUM_AUDIO_IN)
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

static char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04},
    "Multus",
    "Multus Control",        /* iProduct — Device Manager / Zadig composite name */
    /* iSerial = build/release stamp (updated every PIO build) */
    PROFICIO_FW_RELEASE_DATETIME,
    "Multus Sound",          /* Audio Control — Windows Speakers/Mic parent name */
    "Multus Sound Playback", /* TX IQ (host → radio) */
    "Multus Sound Capture",  /* RX IQ (radio → host) */
    "Multus Control"         /* Vendor/control interface (stridx 7) */
};

static uint16_t _desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) {
            return NULL;
        }
        const char *str = string_desc_arr[index];
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31) {
            chr_count = 31;
        }
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}
