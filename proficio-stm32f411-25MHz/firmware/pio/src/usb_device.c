/**
 * USB device — TinyUSB (Synopsys DWC2 OTG_FS) for Black Pill F411.
 *
 * Keep this path simple: aggressive SOF/re-enum recovery was killing a
 * healthy link (fast blink → slow blink). Host-reboot attach can be
 * revisited later without touching the steady-state enum path.
 */
#include "usb_device.h"
#include "usb_vendor.h"
#include "board.h"
#include "proficio_config.h"
#include "tusb.h"
#include "stm32f4xx_hal.h"

static volatile uint8_t s_configured;

void OTG_FS_IRQHandler(void)
{
    tud_int_handler(0);
}

static void usb_hw_init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

    g.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &g);

    g.Pin = GPIO_PIN_9;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);

    g.Pin = GPIO_PIN_10;
    g.Mode = GPIO_MODE_AF_OD;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &g);

#ifdef USB_OTG_GCCFG_NOVBUSSENS
    USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;
#endif
#ifdef USB_OTG_GCCFG_VBUSBSEN
    USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBUSBSEN;
#endif
#ifdef USB_OTG_GCCFG_VBUSASEN
    USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBUSASEN;
#endif

    HAL_NVIC_SetPriority(OTG_FS_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
}

void usb_device_init(void)
{
    usb_vendor_init();
    board_delay_ms(50);
    usb_hw_init();
    (void)tusb_init();
}

void usb_device_poll(void)
{
    tud_task();
}

uint8_t usb_device_configured(void)
{
    return s_configured;
}

uint32_t usb_device_bus_resets(void)
{
    return 0;
}

uint32_t usb_device_setups(void)
{
    return 0;
}

void tud_mount_cb(void)
{
    s_configured = 1;
}

void tud_umount_cb(void)
{
    s_configured = 0;
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
}

void tud_resume_cb(void)
{
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                tusb_control_request_t const *request)
{
    static uint8_t buf[64];
    const uint8_t is_in = (uint8_t)((request->bmRequestType & 0x80u) != 0);

    if (stage == CONTROL_STAGE_SETUP) {
        usb_vendor_xfer_t x = usb_vendor_setup(
            request->bmRequestType, request->bRequest, request->wValue,
            request->wIndex, request->wLength, buf, sizeof(buf));

        if (!x.handled) {
            return false;
        }

        if (is_in) {
            return tud_control_xfer(rhport, request, buf, x.len);
        }

        if (request->wLength > 0) {
            uint16_t n = request->wLength;
            if (n > sizeof(buf)) {
                n = sizeof(buf);
            }
            return tud_control_xfer(rhport, request, buf, n);
        }

        return tud_control_status(rhport, request);
    }

    if (stage == CONTROL_STAGE_DATA) {
        if (!is_in && request->wLength > 0) {
            uint16_t n = request->wLength;
            if (n > sizeof(buf)) {
                n = sizeof(buf);
            }
            usb_vendor_complete_out(request->bRequest, buf, n);
        }
        return true;
    }

    return true;
}
