/**
 * USB FS device (OTG_FS) — Proficio composite:
 *  IF0 vendor (class 0xFF) — EP0 control vendor requests
 *  IF1–3 UAC1 audio — ISO IN 0x82 / OUT 0x03
 *
 * Descriptors match PSoC USBFS (VID 0x16C0 PID 0x05DC, 96 kHz stereo 16-bit).
 * Stack is HAL PCD + minimal class logic (no TinyUSB dependency).
 */
#include "usb_device.h"
#include "usb_vendor.h"
#include "audio.h"
#include "pcm3060.h"
#include "sync_sof.h"
#include "board_pins.h"
#include "stm32f4xx_hal.h"
#include <string.h>

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* EP numbers */
#define EP_CTRL     0x00u
#define EP_ISO_OUT  0x03u
#define EP_ISO_IN   0x82u

static uint8_t s_configured = 0;
static uint8_t s_tx_alt = 0;
static uint8_t s_rx_alt = 0;
static uint8_t s_addr = 0;
static uint8_t s_ep0_state; /* 0 idle 1 data in 2 data out 3 status */
static uint8_t s_ep0_buf[64];
static uint16_t s_ep0_len;
static uint8_t s_setup[8];
static uint8_t s_iso_out_buf[I2S_BUF_SIZE];
static uint8_t s_iso_in_buf[I2S_BUF_SIZE];

/* ---- Descriptors (from PSoC USBFS_descr.c) ---- */
static const uint8_t dev_desc[18] = {
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0xC0, 0x16, 0xDC, 0x05, 0x05, 0x00, 0x01, 0x02, 0x03, 0x01
};

/* Config: vendor IF0 + AC IF1 + AS RX IF2 + AS TX IF3 */
#define CFG_LEN 183
static const uint8_t cfg_desc[CFG_LEN] = {
    0x09, 0x02, 0xB7, 0x00, 0x04, 0x01, 0x00, 0xC0, 0x32,
    /* IF0 vendor */
    0x09, 0x04, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x04,
    /* IF1 AC */
    0x09, 0x04, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x05,
    0x0A, 0x24, 0x01, 0x00, 0x01, 0x34, 0x00, 0x02, 0x02, 0x03,
    0x0C, 0x24, 0x02, 0x01, 0x03, 0x06, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,
    0x09, 0x24, 0x03, 0x02, 0x01, 0x01, 0x00, 0x01, 0x00,
    0x0C, 0x24, 0x02, 0x03, 0x01, 0x01, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,
    0x09, 0x24, 0x03, 0x04, 0x03, 0x06, 0x00, 0x03, 0x00,
    /* IF2 AS RX alt0 */
    0x09, 0x04, 0x02, 0x00, 0x00, 0x01, 0x02, 0x00, 0x06,
    /* IF2 AS RX alt1 */
    0x09, 0x04, 0x02, 0x01, 0x01, 0x01, 0x02, 0x00, 0x06,
    0x07, 0x24, 0x01, 0x02, 0x02, 0x01, 0x00,
    0x0B, 0x24, 0x02, 0x01, 0x02, 0x02, 0x10, 0x01, 0x00, 0x77, 0x01,
    0x09, 0x05, 0x82, 0x2D, 0x80, 0x01, 0x01, 0x00, 0x00,
    0x07, 0x25, 0x01, 0x80, 0x01, 0x00, 0x00,
    /* IF3 AS TX alt0 */
    0x09, 0x04, 0x03, 0x00, 0x00, 0x01, 0x02, 0x00, 0x07,
    /* IF3 AS TX alt1 */
    0x09, 0x04, 0x03, 0x01, 0x01, 0x01, 0x02, 0x00, 0x07,
    0x07, 0x24, 0x01, 0x03, 0x01, 0x01, 0x00,
    0x0B, 0x24, 0x02, 0x01, 0x02, 0x02, 0x10, 0x01, 0x00, 0x77, 0x01,
    0x09, 0x05, 0x03, 0x2D, 0x80, 0x01, 0x01, 0x00, 0x00,
    0x07, 0x25, 0x01, 0x80, 0x01, 0x00, 0x00
};

static const uint8_t str0[] = {0x04, 0x03, 0x09, 0x04};
static const char *const strs[] = {
    "", "Multus SDR", "Proficio STM32F411", "0001",
    "Vendor", "Audio", "Audio RX", "Audio TX"
};

static uint8_t s_str_buf[64];

static uint16_t build_string(uint8_t idx)
{
    if (idx == 0) {
        memcpy(s_str_buf, str0, 4);
        return 4;
    }
    if (idx > 7) {
        return 0;
    }
    const char *s = strs[idx];
    uint8_t n = (uint8_t)strlen(s);
    uint8_t i;
    s_str_buf[0] = (uint8_t)(2 + n * 2);
    s_str_buf[1] = 0x03;
    for (i = 0; i < n; i++) {
        s_str_buf[2 + i * 2] = (uint8_t)s[i];
        s_str_buf[3 + i * 2] = 0;
    }
    return s_str_buf[0];
}

static void ep0_tx(const uint8_t *data, uint16_t len, uint16_t req_len)
{
    if (len > req_len) {
        len = req_len;
    }
    if (len > sizeof(s_ep0_buf)) {
        len = sizeof(s_ep0_buf);
    }
    if (data && len) {
        memcpy(s_ep0_buf, data, len);
    }
    s_ep0_len = len;
    s_ep0_state = 1;
    HAL_PCD_EP_Transmit(&hpcd_USB_OTG_FS, 0x00, s_ep0_buf, len);
}

static void ep0_stall(void)
{
    HAL_PCD_EP_SetStall(&hpcd_USB_OTG_FS, 0x00);
    HAL_PCD_EP_SetStall(&hpcd_USB_OTG_FS, 0x80);
}

static void handle_setup(void)
{
    uint8_t bm = s_setup[0];
    uint8_t req = s_setup[1];
    uint16_t wValue = (uint16_t)(s_setup[2] | (s_setup[3] << 8));
    uint16_t wIndex = (uint16_t)(s_setup[4] | (s_setup[5] << 8));
    uint16_t wLength = (uint16_t)(s_setup[6] | (s_setup[7] << 8));
    uint8_t type = (uint8_t)((bm >> 5) & 3u);
    uint8_t recip = (uint8_t)(bm & 0x1Fu);

    /* Standard device */
    if (type == 0 && recip == 0) {
        switch (req) {
        case 0x06: /* GET_DESCRIPTOR */
            switch (wValue >> 8) {
            case 0x01:
                ep0_tx(dev_desc, sizeof(dev_desc), wLength);
                return;
            case 0x02:
                ep0_tx(cfg_desc, sizeof(cfg_desc), wLength);
                return;
            case 0x03: {
                uint16_t n = build_string((uint8_t)(wValue & 0xFF));
                if (n) {
                    ep0_tx(s_str_buf, n, wLength);
                } else {
                    ep0_stall();
                }
                return;
            }
            default:
                ep0_stall();
                return;
            }
        case 0x05: /* SET_ADDRESS */
            s_addr = (uint8_t)(wValue & 0x7F);
            HAL_PCD_EP_Transmit(&hpcd_USB_OTG_FS, 0x00, NULL, 0);
            s_ep0_state = 3;
            return;
        case 0x09: /* SET_CONFIGURATION */
            s_configured = (uint8_t)(wValue & 0xFF);
            if (s_configured) {
                HAL_PCD_EP_Open(&hpcd_USB_OTG_FS, EP_ISO_OUT, I2S_BUF_SIZE,
                                EP_TYPE_ISOC);
                HAL_PCD_EP_Open(&hpcd_USB_OTG_FS, EP_ISO_IN, I2S_BUF_SIZE,
                                EP_TYPE_ISOC);
                HAL_PCD_EP_Receive(&hpcd_USB_OTG_FS, EP_ISO_OUT, s_iso_out_buf,
                                   I2S_BUF_SIZE);
            }
            HAL_PCD_EP_Transmit(&hpcd_USB_OTG_FS, 0x00, NULL, 0);
            s_ep0_state = 3;
            return;
        case 0x08: /* GET_CONFIGURATION */
            ep0_tx(&s_configured, 1, wLength);
            return;
        default:
            break;
        }
    }

    /* Standard interface — SET_INTERFACE for audio alts */
    if (type == 0 && recip == 1 && req == 0x0B) {
        uint8_t itf = (uint8_t)(wIndex & 0xFF);
        uint8_t alt = (uint8_t)(wValue & 0xFF);
        if (itf == RX_INTERFACE) {
            s_rx_alt = alt;
        }
        if (itf == TX_INTERFACE) {
            s_tx_alt = alt;
        }
        HAL_PCD_EP_Transmit(&hpcd_USB_OTG_FS, 0x00, NULL, 0);
        s_ep0_state = 3;
        return;
    }
    if (type == 0 && recip == 1 && req == 0x0A) {
        uint8_t itf = (uint8_t)(wIndex & 0xFF);
        uint8_t alt = 0;
        if (itf == RX_INTERFACE) {
            alt = s_rx_alt;
        }
        if (itf == TX_INTERFACE) {
            alt = s_tx_alt;
        }
        ep0_tx(&alt, 1, wLength);
        return;
    }

    /* Vendor — type 2 (vendor), or bmRequestType 0xC0/0x40 style */
    if (type == 2 || (bm & 0x60u) == 0x40u) {
        usb_vendor_xfer_t x = usb_vendor_setup(bm, req, wValue, wIndex, wLength,
                                               s_ep0_buf, sizeof(s_ep0_buf));
        if (!x.handled) {
            ep0_stall();
            return;
        }
        if ((bm & 0x80u) != 0) {
            /* device-to-host */
            ep0_tx(s_ep0_buf, x.len, wLength);
        } else if (wLength > 0) {
            s_ep0_len = (wLength > sizeof(s_ep0_buf)) ? sizeof(s_ep0_buf)
                                                      : wLength;
            s_ep0_state = 2;
            HAL_PCD_EP_Receive(&hpcd_USB_OTG_FS, 0x00, s_ep0_buf, s_ep0_len);
        } else {
            HAL_PCD_EP_Transmit(&hpcd_USB_OTG_FS, 0x00, NULL, 0);
            s_ep0_state = 3;
        }
        return;
    }

    ep0_stall();
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
    memcpy(s_setup, (uint8_t *)hpcd->Setup, 8);
    handle_setup();
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    if (epnum == 0) {
        if (s_ep0_state == 3 && s_addr) {
            HAL_PCD_SetAddress(hpcd, s_addr);
            s_addr = 0;
        }
        if (s_ep0_state == 1) {
            /* status OUT */
            HAL_PCD_EP_Receive(hpcd, 0x00, NULL, 0);
            s_ep0_state = 0;
        }
        return;
    }
    if (epnum == (EP_ISO_IN & 0x7Fu) && s_rx_alt) {
        uint16_t n = audio_usb_in_packet(s_iso_in_buf, I2S_BUF_SIZE);
        HAL_PCD_EP_Transmit(hpcd, EP_ISO_IN, s_iso_in_buf, n);
    }
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    if (epnum == 0) {
        if (s_ep0_state == 2) {
            uint8_t req = s_setup[1];
            uint16_t len = (uint16_t)hpcd->OUT_ep[0].xfer_count;
            usb_vendor_complete_out(req, s_ep0_buf, len);
            HAL_PCD_EP_Transmit(hpcd, 0x00, NULL, 0);
            s_ep0_state = 3;
        }
        return;
    }
    if (epnum == EP_ISO_OUT && s_tx_alt) {
        uint16_t len = (uint16_t)hpcd->OUT_ep[EP_ISO_OUT].xfer_count;
        audio_usb_out_packet(s_iso_out_buf, len);
        HAL_PCD_EP_Receive(hpcd, EP_ISO_OUT, s_iso_out_buf, I2S_BUF_SIZE);
    }
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
    s_configured = 0;
    s_tx_alt = 0;
    s_rx_alt = 0;
    s_addr = 0;
    HAL_PCD_EP_Open(hpcd, 0x00, 64, EP_TYPE_CTRL);
    HAL_PCD_EP_Open(hpcd, 0x80, 64, EP_TYPE_CTRL);
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
    (void)hpcd;

    /* PSoC Sync_Main runs on SOF-related timing — lock I2S clock here */
    sync_sof_on_sof();

    /*
     * One ISO IN packet per USB frame while RX streaming is active
     * (matches 96 kHz × 1 ms = 384 bytes).
     */
    if (s_configured && s_rx_alt) {
        uint16_t n = audio_usb_in_packet(s_iso_in_buf, I2S_BUF_SIZE);
        HAL_PCD_EP_Transmit(&hpcd_USB_OTG_FS, EP_ISO_IN, s_iso_in_buf, n);
    }
}

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
    if (hpcd->Instance != USB_OTG_FS) {
        return;
    }
    GPIO_InitTypeDef g = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

    g.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &g);

    HAL_NVIC_SetPriority(OTG_FS_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
}

void OTG_FS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}

void usb_device_init(void)
{
    usb_vendor_init();
    sync_sof_init();

    hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
    hpcd_USB_OTG_FS.Init.dev_endpoints = 4;
    hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
    hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
    hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.vbus_sensing_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;

    if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK) {
        return;
    }

    HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x80);
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, 0x40);
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1, 0x40);
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 2, 0x80);

    HAL_PCD_Start(&hpcd_USB_OTG_FS);
}

void usb_device_poll(void)
{
    /* IRQ driven */
}

uint8_t usb_device_configured(void)
{
    return s_configured;
}

uint8_t usb_audio_tx_enabled(void)
{
    return (uint8_t)(s_configured && s_tx_alt);
}

uint8_t usb_audio_rx_enabled(void)
{
    return (uint8_t)(s_configured && s_rx_alt);
}
