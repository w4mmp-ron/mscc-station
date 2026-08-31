/**
 * TinyUSB UAC2 ↔ Proficio I2S audio bridge.
 * Does not touch USB bring-up (usb_device.c / DWC2 init).
 */
#include "tusb.h"
#include "usb_descriptors.h"
#include "audio.h"
#include <string.h>

static uint8_t s_spk_alt;
static uint8_t s_mic_alt;
static uint32_t s_sample_rate = 96000;

static int8_t s_mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];
static int16_t s_volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];

CFG_TUSB_MEM_ALIGN static uint8_t s_spk_buf[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ];
CFG_TUSB_MEM_ALIGN static uint8_t s_mic_buf[CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ];

uint8_t usb_audio_tx_enabled(void)
{
    return (uint8_t)(s_spk_alt != 0);
}

uint8_t usb_audio_rx_enabled(void)
{
    return (uint8_t)(s_mic_alt != 0);
}

/* ---- Interface alt settings ---- */

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *req)
{
    (void)rhport;
    uint8_t itf = tu_u16_low(tu_le16toh(req->wIndex));
    uint8_t alt = tu_u16_low(tu_le16toh(req->wValue));

    if (itf == ITF_NUM_AUDIO_STREAMING_SPK) {
        s_spk_alt = alt;
    } else if (itf == ITF_NUM_AUDIO_STREAMING_MIC) {
        s_mic_alt = alt;
    }
    return true;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const *req)
{
    (void)rhport;
    uint8_t itf = tu_u16_low(tu_le16toh(req->wIndex));
    if (itf == ITF_NUM_AUDIO_STREAMING_SPK) {
        s_spk_alt = 0;
    } else if (itf == ITF_NUM_AUDIO_STREAMING_MIC) {
        s_mic_alt = 0;
    }
    return true;
}

/* ---- Host → device (speaker / TX IQ) ---- */

bool tud_audio_rx_done_pre_read_cb(uint8_t rhport, uint16_t n_bytes_received,
                                   uint8_t func_id, uint8_t ep_out,
                                   uint8_t cur_alt_setting)
{
    (void)rhport;
    (void)func_id;
    (void)ep_out;
    (void)cur_alt_setting;

    uint16_t n = (uint16_t)tud_audio_read(s_spk_buf, n_bytes_received);
    if (n) {
        audio_usb_out_packet(s_spk_buf, n);
    }
    return true;
}

/* ---- Device → host (mic / RX IQ) ---- */

bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t func_id, uint8_t ep_in,
                                   uint8_t cur_alt_setting)
{
    (void)rhport;
    (void)func_id;
    (void)ep_in;
    (void)cur_alt_setting;

    uint16_t n = audio_usb_in_packet(s_mic_buf, CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX);
    if (n) {
        (void)tud_audio_write(s_mic_buf, n);
    }
    return true;
}

/* ---- Clock / feature unit controls (minimal) ---- */

static bool clock_get(uint8_t rhport, audio_control_request_t const *request)
{
    if (request->bEntityID != UAC2_ENTITY_CLOCK) {
        return false;
    }
    if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ) {
        if (request->bRequest == AUDIO_CS_REQ_CUR) {
            audio_control_cur_4_t cur = {.bCur = (int32_t)tu_htole32(s_sample_rate)};
            return tud_audio_buffer_and_schedule_control_xfer(
                rhport, (tusb_control_request_t const *)request, &cur, sizeof(cur));
        }
        if (request->bRequest == AUDIO_CS_REQ_RANGE) {
            audio_control_range_4_n_t(1) range = {
                .wNumSubRanges = tu_htole16(1),
                .subrange = {{.bMin = (int32_t)96000, .bMax = (int32_t)96000, .bRes = 0}}
            };
            return tud_audio_buffer_and_schedule_control_xfer(
                rhport, (tusb_control_request_t const *)request, &range, sizeof(range));
        }
    }
    if (request->bControlSelector == AUDIO_CS_CTRL_CLK_VALID &&
        request->bRequest == AUDIO_CS_REQ_CUR) {
        audio_control_cur_1_t v = {.bCur = 1};
        return tud_audio_buffer_and_schedule_control_xfer(
            rhport, (tusb_control_request_t const *)request, &v, sizeof(v));
    }
    return false;
}

static bool clock_set(uint8_t rhport, audio_control_request_t const *request,
                      uint8_t const *buf)
{
    (void)rhport;
    if (request->bEntityID != UAC2_ENTITY_CLOCK) {
        return false;
    }
    if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ &&
        request->bRequest == AUDIO_CS_REQ_CUR) {
        s_sample_rate = (uint32_t)((audio_control_cur_4_t const *)buf)->bCur;
        return true;
    }
    return false;
}

static bool feature_get(uint8_t rhport, audio_control_request_t const *request)
{
    if (request->bEntityID != UAC2_ENTITY_SPK_FEATURE_UNIT) {
        return false;
    }
    if (request->bControlSelector == AUDIO_FU_CTRL_MUTE &&
        request->bRequest == AUDIO_CS_REQ_CUR) {
        audio_control_cur_1_t m = {.bCur = s_mute[request->bChannelNumber]};
        return tud_audio_buffer_and_schedule_control_xfer(
            rhport, (tusb_control_request_t const *)request, &m, sizeof(m));
    }
    if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME) {
        if (request->bRequest == AUDIO_CS_REQ_RANGE) {
            audio_control_range_2_n_t(1) r = {
                .wNumSubRanges = tu_htole16(1),
                .subrange = {{.bMin = tu_htole16(-5120), .bMax = tu_htole16(0), .bRes = tu_htole16(256)}}
            };
            return tud_audio_buffer_and_schedule_control_xfer(
                rhport, (tusb_control_request_t const *)request, &r, sizeof(r));
        }
        if (request->bRequest == AUDIO_CS_REQ_CUR) {
            audio_control_cur_2_t c = {.bCur = tu_htole16(s_volume[request->bChannelNumber])};
            return tud_audio_buffer_and_schedule_control_xfer(
                rhport, (tusb_control_request_t const *)request, &c, sizeof(c));
        }
    }
    return false;
}

static bool feature_set(uint8_t rhport, audio_control_request_t const *request,
                        uint8_t const *buf)
{
    (void)rhport;
    if (request->bEntityID != UAC2_ENTITY_SPK_FEATURE_UNIT) {
        return false;
    }
    if (request->bControlSelector == AUDIO_FU_CTRL_MUTE &&
        request->bRequest == AUDIO_CS_REQ_CUR) {
        s_mute[request->bChannelNumber] = ((audio_control_cur_1_t const *)buf)->bCur;
        return true;
    }
    if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME &&
        request->bRequest == AUDIO_CS_REQ_CUR) {
        s_volume[request->bChannelNumber] = ((audio_control_cur_2_t const *)buf)->bCur;
        return true;
    }
    return false;
}

bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    audio_control_request_t const *req = (audio_control_request_t const *)p_request;
    if (req->bEntityID == UAC2_ENTITY_CLOCK) {
        return clock_get(rhport, req);
    }
    if (req->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT) {
        return feature_get(rhport, req);
    }
    return false;
}

bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request,
                                 uint8_t *buf)
{
    audio_control_request_t const *req = (audio_control_request_t const *)p_request;
    if (req->bEntityID == UAC2_ENTITY_CLOCK) {
        return clock_set(rhport, req, buf);
    }
    if (req->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT) {
        return feature_set(rhport, req, buf);
    }
    return false;
}
