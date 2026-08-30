/**
 * Application shell — host radio logic moves here over phases.
 * Reference: Release-Proficio-MKII-PTT main loop + usbvend/cw/si5351.
 */
#include "proficio_config.h"
#include "app.h"

void app_init(void)
{
#if PROFICIO_FEAT_USB_VENDOR
    /* usb_vendor_init(); */
#endif
}

void app_poll(void)
{
#if PROFICIO_FEAT_USB_VENDOR
    /* usb_vendor_poll(); */
#endif
#if PROFICIO_FEAT_KEYER_I2C
    /* configure_cw_poll(); */
#endif
}
