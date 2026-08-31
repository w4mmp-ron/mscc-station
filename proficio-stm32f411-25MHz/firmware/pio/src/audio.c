/**
 * Audio path main — PSoC audio.c logic without Cypress USB DMA.
 * USB stack calls audio_usb_* ; this mirrors EnableOutEP / LoadInEP flow.
 */
#include "audio.h"
#include "pcm3060.h"
#include "usb_device.h"
#include <string.h>

uint8_t Audio_IQ_Channels = 0;
uint8_t E_Audio_running = 0;

static uint8_t s_tx_enabled = 0;
static uint8_t s_pending_in = 0;

void Audio_Start(void)
{
    s_tx_enabled = 0;
    s_pending_in = 1;
    E_Audio_running = 1;
    memset(TxI2S, 0, sizeof(TxI2S));
    memset(RxI2S, 0, sizeof(RxI2S));
}

void Audio_Stop(void)
{
    E_Audio_running = 0;
    s_tx_enabled = 0;
    (void)PCM3060_Stop();
}

void Audio_Main(void)
{
    if (!E_Audio_running) {
        return;
    }

    /* Host selected TX streaming interface alt */
    if (usb_audio_tx_enabled()) {
        if (!s_tx_enabled) {
            s_tx_enabled = 1;
        }
    } else {
        s_tx_enabled = 0;
    }

    if (usb_audio_rx_enabled()) {
        s_pending_in = 1;
    }

    (void)s_tx_enabled;
}

void audio_usb_out_packet(const uint8_t *data, uint16_t len)
{
    uint16_t n = len;
    if (!data) {
        return;
    }
    if (n > I2S_BUF_SIZE) {
        n = I2S_BUF_SIZE;
    }

    /* Optional IQ swap (PSoC TD_SWAP) */
    if (Audio_IQ_Channels & 0x02u) {
        uint16_t i;
        const uint16_t *s = (const uint16_t *)data;
        uint16_t *d = (uint16_t *)PCM3060_TxBuf();
        uint16_t frames = (uint16_t)(n / 4u);
        for (i = 0; i < frames; i++) {
            d[i * 2u] = s[i * 2u + 1u];
            d[i * 2u + 1u] = s[i * 2u];
        }
    } else {
        memcpy(PCM3060_TxBuf(), data, n);
        if (n < I2S_BUF_SIZE) {
            memset(PCM3060_TxBuf() + n, 0, I2S_BUF_SIZE - n);
        }
    }
}

uint16_t audio_usb_in_packet(uint8_t *data, uint16_t max_len)
{
    uint16_t n = I2S_BUF_SIZE;
    if (!data || max_len == 0) {
        return 0;
    }
    if (n > max_len) {
        n = max_len;
    }

    if (Audio_IQ_Channels & 0x01u) {
        uint16_t i;
        const uint16_t *s = (const uint16_t *)PCM3060_RxBuf();
        uint16_t *d = (uint16_t *)data;
        uint16_t frames = (uint16_t)(n / 4u);
        for (i = 0; i < frames; i++) {
            d[i * 2u] = s[i * 2u + 1u];
            d[i * 2u + 1u] = s[i * 2u];
        }
    } else {
        memcpy(data, PCM3060_RxBuf(), n);
    }
    return n;
}
