/**
 * Main application loop — PSoC-style beater + USB audio path.
 */
#include "app.h"
#include "proficio_config.h"
#include "radio_state.h"
#include "control.h"
#include "board.h"
#include "usb_vendor.h"
#include "usb_device.h"
#include "cw.h"
#include "si5351a.h"
#include "band.h"
#include "pcm3060.h"
#include "audio.h"
#include "sync_sof.h"
#include "die_temp.h"

extern uint8_t g_status_beat;

static uint8_t s_beater = 0;
static uint8_t s_prev_host = 0;
static uint8_t s_prev_tx_hold = 0;
static uint32_t s_last_beat_ms = 0;
static uint8_t s_audio_started = 0;

void app_init(void)
{
    radio_state_init();

#if PROFICIO_FEAT_USB_VENDOR || PROFICIO_FEAT_USB_AUDIO
    usb_device_init();
#endif
#if PROFICIO_FEAT_CW
    cw_init();
#endif
#if PROFICIO_FEAT_PCM3060 || PROFICIO_FEAT_USB_AUDIO
    (void)PCM3060_Init();
#endif
#if PROFICIO_FEAT_SI5351
    si5351a_init();
    (void)si5351aSetFrequency_wait(E_current_LO_freq);
#endif
#if PROFICIO_FEAT_BAND
    Band_Control_Write(CONTROL_BAND_20_30);
#endif
    die_temp_init();

    s_prev_host = E_host_mode;
    s_prev_tx_hold = E_TX_Hold;
    s_last_beat_ms = board_millis();
}

void app_poll(void)
{
    uint32_t now = board_millis();

    cw_hold_poll();

#if PROFICIO_FEAT_USB_VENDOR || PROFICIO_FEAT_USB_AUDIO
    usb_device_poll();
#endif

#if PROFICIO_FEAT_USB_AUDIO
    /* Start codec + I2S when USB configures (host present) */
    if (usb_device_configured() && !s_audio_started) {
        Audio_Start();
        sync_sof_start();
        PCM3060_Start();
        s_audio_started = 1;
    }
    if (!usb_device_configured() && s_audio_started) {
        Audio_Stop();
        sync_sof_stop();
        s_audio_started = 0;
    }
    Audio_Main();
#endif

    if ((now - s_last_beat_ms) < 1u) {
        return;
    }
    s_last_beat_ms = now;
    g_status_beat ^= 1u;

    switch (s_beater++) {
    case 0:
        break;
    case 1:
        if (E_host_mode != 'C') {
            if (s_prev_host != E_host_mode) {
                uint8_t c = Control_Read();
                c = (uint8_t)(c & ~CONTROL_LED);
                c = (uint8_t)(c | CONTROL_RX | CONTROL_AMP | CONTROL_DOUT);
                Control_Write(c);
                TX_Request = 0;
                s_prev_host = E_host_mode;
            }
        } else {
            s_prev_host = E_host_mode;
        }
        break;
    case 2:
        if (s_prev_tx_hold != E_TX_Hold) {
            s_prev_tx_hold = E_TX_Hold;
        }
#if PROFICIO_FEAT_KEYER_I2C
        if (E_keyer_installed == TRUE) {
            (void)Configure_CW();
        }
#endif
        break;
    case 3:
        break;
    case 4:
#if PROFICIO_FEAT_BAND
        if (E_si5351_status == 0) {
            (void)Band_Main();
        }
#endif
        break;
    case 5:
        die_temp_poll();
        break;
    case 6:
#if PROFICIO_FEAT_SI5351
        if (E_host_mode != 'C') {
            E_si5351_status = si5351aSetFrequency(E_current_LO_freq);
        }
#endif
        s_beater = 0;
        break;
    default:
        s_beater = 0;
        break;
    }

#if PROFICIO_FEAT_CW
    if (E_host_mode == 'C') {
        Manage_Paddles_Port();
    }
#endif
}
