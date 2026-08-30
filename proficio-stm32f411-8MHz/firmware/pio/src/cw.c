/**
 * CW / keyer bridge — port of MKII Configure_CW + Manage_Paddles_Port.
 * PROFICIO_CW_MKII=1: PIN-diode path (key CONTROL_RX per element).
 * PROFICIO_CW_MKII=0: legacy DIN keying with session-latched AMP/RX.
 */
#include "cw.h"
#include "control.h"
#include "radio_state.h"
#include "usbvend.h"
#include "si5351a.h"
#include "i2c_bus.h"
#include "proficio_config.h"
#include "board.h"

uint8_t keyer_write(uint8_t buffer)
{
    uint8_t attempt;
    for (attempt = 0; attempt < 8u; attempt++) {
        if (i2c_write_byte(KEYER_I2C_ADDR, buffer)) {
            return 1;
        }
        board_delay_ms(5);
    }
    return 0;
}

void cw_init(void)
{
    E_cw_hold = FALSE;
}

void cw_on_hold_expired(void)
{
    E_cw_hold = FALSE;
}

/* ---- Configure_CW: push host params + 0x9C queue to PIC keyer ---- */

uint8_t Configure_CW(void)
{
    static uint8_t state = 0;
    static uint8_t keyer_mode = 0;
    static uint8_t wpm = 0;
    static uint8_t spacing = 0;
    static uint8_t mem_text_wpm = 0;
    static uint8_t weight = 0;
    static uint8_t side_tone = 0;
    static uint8_t paddle = 0;
    static uint8_t buffer[2];
    static uint8_t send_state = 0;
    static uint8_t last_mem_seq = 0;
    uint8_t write_status;
    uint8_t p0, p1, s0, s1;

    p0 = E_keyer_mem_pkt[0];
    s0 = E_keyer_mem_pkt[1];
    p1 = E_keyer_mem_pkt[0];
    s1 = E_keyer_mem_pkt[1];
    if (s0 != 0 && s0 == s1 && p0 == p1 && s0 != last_mem_seq) {
        last_mem_seq = s0;
        if (E_keyer_mem_q_count < KEYER_MEM_Q_SIZE) {
            E_keyer_mem_q[E_keyer_mem_q_head] = p0;
            E_keyer_mem_q_head = (uint8_t)((E_keyer_mem_q_head + 1) % KEYER_MEM_Q_SIZE);
            E_keyer_mem_q_count++;
        }
    }

    switch (state) {
    case 0:
        if (keyer_mode != E_keyer_mode) {
            buffer[0] = SET_KEYER_MODE;
            buffer[1] = E_keyer_mode;
            keyer_mode = E_keyer_mode;
            state = 10;
        } else {
            state++;
        }
        break;
    case 1:
        if (paddle != E_paddle) {
            buffer[0] = SET_CW_PADDLE;
            buffer[1] = E_paddle;
            paddle = E_paddle;
            state = 10;
        } else {
            state++;
        }
        break;
    case 2:
        if (spacing != E_spacing) {
            buffer[0] = SET_SPACING;
            buffer[1] = E_spacing;
            spacing = E_spacing;
            state = 10;
        } else {
            state++;
        }
        break;
    case 3:
        if (weight != E_weight) {
            buffer[0] = SET_WEIGHT;
            buffer[1] = E_weight;
            weight = E_weight;
            state = 10;
        } else {
            state++;
        }
        break;
    case 4:
        if (side_tone != E_side_tone) {
            buffer[0] = SET_SIDE_TONE;
            buffer[1] = E_side_tone;
            side_tone = E_side_tone;
            state = 10;
        } else {
            state++;
        }
        break;
    case 5:
        if (wpm != E_wpm) {
            buffer[0] = SET_WPM;
            buffer[1] = E_wpm;
            wpm = E_wpm;
            state = 10;
        } else {
            state++;
        }
        break;
    case 6:
        if (mem_text_wpm != E_mem_text_wpm) {
            buffer[0] = SET_MEM_TEXT_WPM;
            buffer[1] = E_mem_text_wpm;
            mem_text_wpm = E_mem_text_wpm;
            state = 10;
        } else {
            state++;
        }
        break;
    case 7:
        if (E_keyer_mem_q_count > 0) {
            buffer[0] = CMD_SET_KEYER_MEMORY;
            buffer[1] = E_keyer_mem_q[E_keyer_mem_q_tail];
            E_keyer_mem_q_tail =
                (uint8_t)((E_keyer_mem_q_tail + 1) % KEYER_MEM_Q_SIZE);
            E_keyer_mem_q_count--;
            state = 10;
        } else {
            state = 0;
        }
        break;
    case 10:
        switch (send_state) {
        case 0:
            write_status = keyer_write(buffer[0]);
            if (write_status == 1) {
                send_state = 1;
                state = 10;
            } else {
                state = 0;
                send_state = 0;
                E_keyer_installed = FALSE;
            }
            break;
        case 1:
            write_status = keyer_write(buffer[1]);
            if (write_status == 1) {
                send_state = 0;
                state = 0;
            } else {
                state = 0;
                send_state = 0;
                E_keyer_installed = FALSE;
            }
            break;
        default:
            send_state = 0;
            state = 0;
            break;
        }
        break;
    default:
        state = 0;
        break;
    }
    return state;
}

/* ---- paddle / semi-break-in ---- */

static uint8_t SI5351_status = 0;

static uint8_t cw_keys_down(void)
{
    uint8_t key = Status_Read();
    if (key & STATUS_KEY_0) {
        E_key_0 = TRUE;
    } else {
        E_key_0 = FALSE;
    }
    if (key & STATUS_KEY_1) {
        E_key_1 = TRUE;
    } else {
        E_key_1 = FALSE;
    }
    return (uint8_t)(!E_key_0 || !E_key_1);
}

static void cw_start_hold(void)
{
    /* PSoC: period = E_TX_Hold * 100; CW_Hold timer unit ~10µs-class.
     * Use hold × 10 ms as practical semi-break-in (50 → 500 ms). */
    uint32_t ms = (uint32_t)E_TX_Hold * 10u;
    if (ms < 50u) {
        ms = 50u;
    }
    cw_hold_start_ms(ms);
}

#if PROFICIO_CW_MKII

void Manage_Paddles_Port(void)
{
    static uint8_t state = 0;
    static uint32_t previous_CW_LO_Freq = 0;
    uint8_t control_status;

    switch (state) {
    case 0:
        if (previous_CW_LO_Freq != CW_LO_Freq) {
            SI5351_status = si5351aSetFrequency(CW_LO_Freq);
            if (SI5351_status == 0) {
                previous_CW_LO_Freq = CW_LO_Freq;
                if (!TX_Inhibit && (E_host_mode == 'C')) {
                    state++;
                }
            }
        } else {
            if (!TX_Inhibit && (E_host_mode == 'C')) {
                state++;
            }
        }
        break;
    case 1:
        if (cw_keys_down()) {
            E_key_down = TRUE;
            state++;
        } else {
            state = 0;
        }
        break;
    case 2:
        Control_Write(Control_Read() & (uint8_t)~CONTROL_DOUT);
        SI5351_status =
            si5351aSetFrequency(CW_LO_Freq + (uint32_t)E_cw_pitch_freq);
        if (SI5351_status == 0) {
            state++;
        }
        break;
    case 3:
        control_status = Control_Read();
        if (E_QSK == TRUE) {
            control_status = (uint8_t)(control_status & ~CONTROL_AMP);
            control_status = (uint8_t)(control_status & ~CONTROL_RX);
            E_cw_hold = TRUE;
            state = 5;
        } else {
            control_status = (uint8_t)(control_status & ~CONTROL_AMP);
            state = 4;
        }
        Control_Write(control_status);
        break;
    case 4:
        Control_Write(Control_Read() & (uint8_t)~CONTROL_RX);
        E_cw_hold = TRUE;
        state++;
        break;
    case 5:
        if (!cw_keys_down()) {
            state++;
        }
        break;
    case 6:
        /* KEY UP — start hang (not on key-down) */
        Control_Write(Control_Read() | CONTROL_RX);
        cw_start_hold();
        state++;
        break;
    case 7:
        if (E_cw_hold == FALSE) {
            SI5351_status = si5351aSetFrequency(CW_LO_Freq);
            if (SI5351_status == 0) {
                control_status = Control_Read();
                control_status = (uint8_t)(control_status | CONTROL_AMP);
                control_status = (uint8_t)(control_status | CONTROL_DOUT);
                Control_Write(control_status);
                E_key_down = FALSE;
                state = 0;
            }
        } else {
            state = 10;
        }
        break;
    case 10:
        if (cw_keys_down()) {
            state = 4;
        } else {
            state = 7;
        }
        break;
    default:
        state = 0;
        break;
    }
}

#else /* legacy DIN keying */

#define CW_ST_IDLE          0
#define CW_ST_WAIT_KEY      1
#define CW_ST_SESSION_START 2
#define CW_ST_MARK          3
#define CW_ST_MARK_HOLD     5
#define CW_ST_SPACE         6
#define CW_ST_HANG          7
#define CW_ST_HANG_POLL     10

static uint8_t cw_session_start(void)
{
    uint8_t c = Control_Read();
    c = (uint8_t)(c & ~CONTROL_DOUT);
    c = (uint8_t)(c & ~CONTROL_DIN);
    Control_Write(c);
    return si5351aSetFrequency(CW_LO_Freq + (uint32_t)E_cw_pitch_freq);
}

static void cw_mark_on(void)
{
    uint8_t c = Control_Read();
    c = (uint8_t)(c & ~CONTROL_AMP);
    c = (uint8_t)(c & ~CONTROL_RX);
    c = (uint8_t)(c | CONTROL_DIN);
    Control_Write(c);
    E_cw_hold = TRUE;
}

static void cw_mark_off(void)
{
    Control_Write(Control_Read() & (uint8_t)~CONTROL_DIN);
    cw_start_hold();
}

static uint8_t cw_session_end(void)
{
    uint8_t c;
    uint8_t st = si5351aSetFrequency(CW_LO_Freq);
    if (st != 0) {
        return st;
    }
    c = Control_Read();
    c = (uint8_t)(c | CONTROL_AMP | CONTROL_RX | CONTROL_DOUT);
    Control_Write(c);
    E_key_down = FALSE;
    return 0;
}

void Manage_Paddles_Port(void)
{
    static uint8_t state = CW_ST_IDLE;
    static uint32_t previous_CW_LO_Freq = 0;

    switch (state) {
    case CW_ST_IDLE:
        if (previous_CW_LO_Freq != CW_LO_Freq) {
            SI5351_status = si5351aSetFrequency(CW_LO_Freq);
            if (SI5351_status == 0) {
                previous_CW_LO_Freq = CW_LO_Freq;
            } else {
                break;
            }
        }
        if (!TX_Inhibit && (E_host_mode == 'C')) {
            state = CW_ST_WAIT_KEY;
        }
        break;
    case CW_ST_WAIT_KEY:
        if (cw_keys_down()) {
            E_key_down = TRUE;
            state = CW_ST_SESSION_START;
        } else {
            state = CW_ST_IDLE;
        }
        break;
    case CW_ST_SESSION_START:
        SI5351_status = cw_session_start();
        if (SI5351_status == 0) {
            state = CW_ST_MARK;
        }
        break;
    case CW_ST_MARK:
        cw_mark_on();
        state = CW_ST_MARK_HOLD;
        break;
    case CW_ST_MARK_HOLD:
        if (!cw_keys_down()) {
            state = CW_ST_SPACE;
        }
        break;
    case CW_ST_SPACE:
        cw_mark_off();
        state = CW_ST_HANG;
        break;
    case CW_ST_HANG:
        if (E_cw_hold == FALSE) {
            SI5351_status = cw_session_end();
            if (SI5351_status == 0) {
                state = CW_ST_IDLE;
            }
        } else {
            state = CW_ST_HANG_POLL;
        }
        break;
    case CW_ST_HANG_POLL:
        if (cw_keys_down()) {
            state = CW_ST_MARK;
        } else {
            state = CW_ST_HANG;
        }
        break;
    default:
        state = CW_ST_IDLE;
        break;
    }
}

#endif /* PROFICIO_CW_MKII */
