/**
  Proficio MKII option keyer — PIC16F18326
  Iambic A/B / straight + I2C config + CQ memory (CMD_SET_KEYER_MEMORY 0x9C)

  Device: PIC16F18326 (MCC generated drivers)

  Change history (Grok / MSCC work):
  2026-08-07  CQ memory store/play (0x9C), Morse table, Mode B squeeze memory,
              I2C queue no-delay-on-overrun, logical && for paddles.
  2026-08-07  Straight mode: either contact = solid key/sidetone while held
              (was bug-style auto-dots / click-only on the other paddle).
  2026-08-07  Play abort: wait paddles clear; abort only open→close edge so
              next play is not immediately killed after tap-to-stop.
  2026-08-08  Four CQ memories: subcmd 3 then slot 0..3 (sticky); 48 chars/slot.
  2026-08-08  SET_MEM_TEXT_WPM 0x76: Farnsworth text WPM for memory play only
              (char elements = SET_WPM; stretch inter-letter/word gaps).
  2026-08-08  EEPROM: text_wpm after message arrays (do not shift CQ memory);
              select pending: non-slot params fall through (not discarded).
  2026-08-08  CMD queue full: drop newest (do not wipe queue — lost BEGIN/play).
  2026-08-08  ARRL/PARIS Farnsworth gaps (31 element + 19 spacing units/word).
 */
#include "mcc_generated_files/mcc.h"
#include "defines.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0
#define COMMAND_QUEUE_SIZE 16
#define QUEUE_EMPTY 199

#define KEYER_STRAIGHT 0
#define KEYER_MODE_A 1
#define KEYER_MODE_B 2

#define SET_KEYER_MODE 0x71
#define SET_CW_PADDLE 0x73
#define SET_SPACING 0x75
/* Memory-play Farnsworth text WPM (was SET_MEMORY_TYPE).
 * 0=off; 5–60=overall/text WPM. Char elements stay on SET_WPM. */
#define SET_MEM_TEXT_WPM 0x76
#define SET_MEMORY_TYPE SET_MEM_TEXT_WPM
#define SET_WEIGHT 0x77
#define SET_WPM 0x7B
#define SET_IAMBIC_TUNING 0x7C
#define SET_SIDE_TONE 0x7F
/* ms-sdr / Proficio: CMD_SET_KEYER_MEMORY
 * param: 0=play, 1=store begin, 2=store end,
 *        3=select slot (next param is slot 0..3), sticky thereafter,
 *        0x20-0x7E = append printable ASCII to current slot builder */
#define CMD_SET_KEYER_MEMORY 0x9C

#define KEYER_MEM_PLAY        0
#define KEYER_MEM_STORE_BEGIN 1
#define KEYER_MEM_STORE_END   2
#define KEYER_MEM_SELECT      3

#define CW_DELAY_CONSTANT 2750
#define KEY_ON 1
#define KEY_OFF 0

#define SIDE_TONE_400 0
#define SIDE_TONE_600 1
#define SIDE_TONE_800 2
#define SIDE_TONE_1000 3

/* 4 slots × (1 len + KEYER_MSG_MAX) must fit ~240 B free EEPROM after settings */
#define KEYER_NUM_SLOTS 4
#define KEYER_MSG_MAX   48

enum {
    CHECK = 0,
    PREDOT,
    PREDASH,
    SENDDOT,
    SENDDASH,
    DOTDELAY,
    DASHDELAY,
    DOTHELD,
    DASHHELD,
    LETTERSPACE,
    EXITLOOP
};

static int dot_memory = 0;
static int dash_memory = 0;
static int key_state = 0;
static int kdelay = 0;
static int dot_delay = 0;
static int dash_delay = 0;
static int kcwl = 0;
static int kcwr = 0;
static int *kdot;
static int *kdash;

static int cw_keyer_speed = 18;
static int cw_keyer_weight = 50;
static int cw_keyer_mode = KEYER_MODE_B;
static int cw_keyer_spacing = 0;
static int cw_keys_reversed = 1;
static int cw_mem_text_wpm = 0; /* 0=off; else Farnsworth text WPM for memory play */
static int32_t cw_loop_delay = 0;
/* Memory-play letter/word gaps in element_delay units (Farnsworth-adjusted) */
static int mem_letter_gap = 0;
static int mem_word_gap = 0;

static int keyer_out = 0;

__eeprom int cw_keyer_speed__eeprom = 18;
__eeprom int cw_keyer_weight__eeprom = 50;
__eeprom int cw_keyer_mode__eeprom = KEYER_MODE_B;
__eeprom int cw_keyer_spacing__eeprom = 0;
__eeprom int cw_keys_reversed__eeprom = 1;
__eeprom int32_t cw_loop_delay_calibration__eeprom = 0;
__eeprom uint8_t NCO1INCH__eeprom = 0;
__eeprom uint8_t NCO1INCL__eeprom = 0x27;
/* Four messages: len[slot] + flat chars[slot * KEYER_MSG_MAX + i]
 * Keep these immediately after NCO so layout matches pre-Farnsworth firmware. */
__eeprom uint8_t cw_msg_len__eeprom[KEYER_NUM_SLOTS];
__eeprom uint8_t cw_msg__eeprom[KEYER_NUM_SLOTS * KEYER_MSG_MAX];
/* After messages so adding this does not shift CQ memory EEPROM */
__eeprom int cw_mem_text_wpm__eeprom = 0;

static uint8_t cw_msg_ram[KEYER_MSG_MAX];
static uint8_t cw_msg_len = 0;
static uint8_t cw_msg_store_len = 0; /* building store in RAM for current slot */
static uint8_t mem_current_slot = 0;   /* sticky 0..3 */
static uint8_t mem_select_pending = FALSE; /* next param is slot after subcmd 3 */
static uint8_t mem_play_active = FALSE;
static uint8_t mem_play_abort = FALSE;

volatile uint8_t SlaveAddress, SlaveR_W, SlaveInit, msg;
volatile uint8_t first_pass = TRUE;
volatile uint8_t tmpSlaveAddr;
volatile uint8_t param1 = 0;
volatile uint8_t param2 = 0;

volatile uint8_t CMD_Queue[COMMAND_QUEUE_SIZE][3] = {0};
volatile int16_t CMD_queue_front = -1;
volatile int16_t CMD_queue_rear = -1;
volatile uint8_t CMD_add_queue_busy = 0;
volatile uint8_t CMD_queue_count = 0;
volatile uint8_t Queue_Overrun = 0;
volatile uint8_t Return_Queue_count = 0;

volatile uint8_t audio_time_out = 0;
static uint8_t tx_active = FALSE;

/*
 * ITU Morse: bits as 1=dash 0=dot, length in low nibble of meta — table as
 * string of '.' and '-' terminated; lookup by ASCII.
 */
static const char *morse_lookup(char c)
{
    static const char *const table[43] = {
        /* 0-9 */
        "-----", ".----", "..---", "...--", "....-", ".....",
        "-....", "--...", "---..", "----.",
        /* A-Z at index 10+ */
        ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",
        ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.",
        "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."
    };
    if (c >= '0' && c <= '9')
        return table[c - '0'];
    if (c >= 'a' && c <= 'z')
        c = (char)(c - 'a' + 'A');
    if (c >= 'A' && c <= 'Z')
        return table[10 + (c - 'A')];
    switch (c) {
        case '/': return "-..-.";
        case '?': return "..--..";
        case '.': return ".-.-.-";
        case ',': return "--..--";
        case '=': return "-...-"; /* BT */
        case '+': return ".-.-."; /* AR */
        case ' ': return NULL;     /* word space */
        default:  return NULL;
    }
}

void Audio_interrupt_time_out(void) {
    if (audio_time_out == TRUE) {
        RX_CW_SetLow();
        audio_time_out = FALSE;
    } else {
        RX_CW_SetHigh();
        audio_time_out = TRUE;
    }
}

uint8_t CMD_dequeue(uint8_t *p1, uint8_t *p2) {
    uint8_t ret = QUEUE_EMPTY;

    if (CMD_add_queue_busy == 0) {
        if (CMD_queue_front == -1) {
            ret = QUEUE_EMPTY;
        } else {
            ret = CMD_Queue[CMD_queue_front][0];
            *p1 = CMD_Queue[CMD_queue_front][1];
            *p2 = CMD_Queue[CMD_queue_front][2];
            CMD_Queue[CMD_queue_front][0] = QUEUE_EMPTY;
            if (CMD_queue_front == CMD_queue_rear) {
                CMD_queue_front = CMD_queue_rear = -1;
            } else {
                CMD_queue_front = (CMD_queue_front + 1) % COMMAND_QUEUE_SIZE;
            }
            if (ret != QUEUE_EMPTY) {
                CMD_queue_count--;
            }
        }
    }
    return ret;
}

void CMD_queue_add(uint8_t command, uint8_t p1, uint8_t p2) {
    /* No delay in ISR. On full queue drop the *new* command — do not wipe
     * the queue (wiping used to discard STORE_BEGIN / play mid-stream). */
    CMD_add_queue_busy = 1;
    if (CMD_queue_front == (CMD_queue_rear + 1) % COMMAND_QUEUE_SIZE) {
        Queue_Overrun = 1;
        CMD_add_queue_busy = 0;
        return;
    }
    if (CMD_queue_front == -1) {
        CMD_queue_front = CMD_queue_rear = 0;
    } else {
        CMD_queue_rear = (CMD_queue_rear + 1) % COMMAND_QUEUE_SIZE;
    }
    CMD_Queue[CMD_queue_rear][0] = command;
    CMD_Queue[CMD_queue_rear][1] = p1;
    CMD_Queue[CMD_queue_rear][2] = p2;
    CMD_queue_count++;
    if (CMD_queue_count > COMMAND_QUEUE_SIZE) {
        CMD_queue_count = COMMAND_QUEUE_SIZE;
    }
    CMD_add_queue_busy = 0;
}

void I2C_SlaveAddressCallbackHandler() {
    tmpSlaveAddr = I2C1_Read();
    SlaveAddress = tmpSlaveAddr >> 1u;
    SlaveR_W = tmpSlaveAddr & 0x01u;

    if (SlaveR_W == 0) {
        /* Proficio Configure_CW sends cmd then param as two 1-byte I2C writes.
         * first_pass alternates across those transfers — do not reset it here. */
        SlaveInit = 1;
    } else {
        Return_Queue_count = CMD_queue_count;
        if (Return_Queue_count > 32) {
            Return_Queue_count = 32;
        }
    }
}

void I2C_SlaveReceiveCallbackHandler() {
    uint8_t b;

    /* One data byte processed per address cycle (matches Proficio keyer_write) */
    if (!SlaveInit) {
        (void)I2C1_Read();
        return;
    }
    SlaveInit--;
    b = I2C1_Read();
    if (first_pass == TRUE) {
        msg = b;
        first_pass = FALSE;
    } else {
        param1 = b;
        first_pass = TRUE;
        CMD_queue_add(msg, param1, 0);
    }
}

void I2C_SlaveTransmitCallbackHandler() {
    I2C1_Write(Return_Queue_count);
}

void I2C_SlaveCollisionCallbackHandler() {
}

void keyer_update() {
    int text_wpm;
    int32_t gap;

    if (cw_keyer_speed < 5)
        cw_keyer_speed = 5;
    if (cw_keyer_speed > 60)
        cw_keyer_speed = 60;
    dot_delay = 1200 / cw_keyer_speed;
    cw_loop_delay = CW_DELAY_CONSTANT / cw_keyer_speed;
    dash_delay = (dot_delay * 3 * cw_keyer_weight) / 50;

    /*
     * Memory-play gaps — ARRL / PARIS Farnsworth when text_wpm < char WPM.
     *
     * PARIS word = 50 units: 31 element units (at char speed) + 19 spacing units.
     * Total word time at text T: 60/T s. Element time at char C: 31*(1.2/C) s.
     * Spacing budget (char-dit equivalents per word):
     *   S = 50*C/T - 31 = (50*C - 31*T)/T
     * Split 19 spacing units as 3 (letter) + 4 (word remainder in our
     * send_morse_char layout) + ... : letter gets 3/19, word gap gets 4/19
     * so letter_gap + word_gap = 7/19 of S (standard inter-word total).
     *
     * letter_gap_units = 3 * S * dot_delay / 19
     * word_gap_units   = 4 * S * dot_delay / 19
     *
     * When Farnsworth off: keep legacy 2*dot / 4*dot (unchanged fist feel).
     */
    text_wpm = cw_mem_text_wpm;
    if (text_wpm != 0 && text_wpm < 5)
        text_wpm = 0;
    if (text_wpm > 60)
        text_wpm = 60;
    if (text_wpm > 0 && text_wpm < cw_keyer_speed) {
        /* S_num = 50*C - 31*T  (spacing budget * T); must be > 0 */
        gap = (int32_t)50 * (int32_t)cw_keyer_speed - (int32_t)31 * (int32_t)text_wpm;
        if (gap <= 0) {
            mem_letter_gap = 2 * dot_delay;
            mem_word_gap = 4 * dot_delay;
        } else {
            /* letter = 3 * (50*C-31*T) * dot_delay / (19 * T) */
            mem_letter_gap = (int)((3 * gap * (int32_t)dot_delay) /
                ((int32_t)19 * (int32_t)text_wpm));
            mem_word_gap = (int)((4 * gap * (int32_t)dot_delay) /
                ((int32_t)19 * (int32_t)text_wpm));
            if (mem_letter_gap < 2 * dot_delay)
                mem_letter_gap = 2 * dot_delay;
            if (mem_word_gap < 4 * dot_delay)
                mem_word_gap = 4 * dot_delay;
        }
    } else {
        mem_letter_gap = 2 * dot_delay;
        mem_word_gap = 4 * dot_delay;
    }

    if (cw_keys_reversed) {
        kdot = &kcwr;
        kdash = &kcwl;
    } else {
        kdot = &kcwl;
        kdash = &kcwr;
    }
}

void clear_memory() {
    dot_memory = 0;
    dash_memory = 0;
}

void set_keyer_out(int state) {
    if (keyer_out != state) {
        keyer_out = state;
        if (tx_active == FALSE) {
            tx_active = TRUE;
            RX_CW_SetHigh();
            TX_CW_SetLow();
        }
        if (state) {
            KEY_0A_SetLow();
            NCO1CONbits.N1EN = 1;
        } else {
            KEY_0A_SetHigh();
            NCO1CONbits.N1EN = 0;
        }
    }
}

static void keyer_idle_release(void) {
    if (tx_active == TRUE) {
        tx_active = FALSE;
        RX_CW_SetLow();
        TX_CW_SetHigh();
    }
}

/* Abort only after paddles have been open once, then close (edge). */
static uint8_t mem_abort_armed = FALSE;

static uint8_t paddles_closed(void) {
    return (uint8_t)((!(KEY_0_GetValue())) || (!(KEY_1_GetValue())));
}

static void element_delay(int units) {
    int32_t loop_count;
    int u;
    for (u = 0; u < units; u++) {
        if (mem_play_active) {
            if (!paddles_closed()) {
                mem_abort_armed = TRUE;
            } else if (mem_abort_armed) {
                mem_play_abort = TRUE;
                set_keyer_out(KEY_OFF);
                return;
            }
        }
        loop_count = 0;
        while (loop_count++ < cw_loop_delay) {
            __delay_us(1);
        }
    }
}

static void send_mark(int units) {
    set_keyer_out(KEY_ON);
    element_delay(units);
    set_keyer_out(KEY_OFF);
}

static void send_morse_char(char c) {
    const char *p;
    int i;

    /* Gaps use mem_* (Farnsworth when text WPM active); marks stay char WPM */
    if (c == ' ' || c == '\t') {
        element_delay(mem_word_gap); /* inter-word (plus prior element space) */
        return;
    }
    p = morse_lookup(c);
    if (p == NULL)
        return;
    for (i = 0; p[i] != '\0'; i++) {
        if (mem_play_abort)
            return;
        if (p[i] == '.')
            send_mark(dot_delay);
        else if (p[i] == '-')
            send_mark(dash_delay);
        if (p[i + 1] != '\0')
            element_delay(dot_delay); /* inter-element at char speed */
    }
    element_delay(mem_letter_gap); /* inter-character (Farnsworth-stretched when set) */
}

static void msg_load_from_eeprom(uint8_t slot) {
    uint8_t i;
    uint16_t base;

    if (slot >= KEYER_NUM_SLOTS)
        slot = 0;
    cw_msg_len = cw_msg_len__eeprom[slot];
    if (cw_msg_len > KEYER_MSG_MAX)
        cw_msg_len = KEYER_MSG_MAX;
    base = (uint16_t)slot * KEYER_MSG_MAX;
    for (i = 0; i < cw_msg_len; i++)
        cw_msg_ram[i] = cw_msg__eeprom[base + i];
}

static void msg_save_to_eeprom(uint8_t slot) {
    uint8_t i;
    uint16_t base;

    if (slot >= KEYER_NUM_SLOTS)
        slot = 0;
    if (cw_msg_store_len > KEYER_MSG_MAX)
        cw_msg_store_len = KEYER_MSG_MAX;
    base = (uint16_t)slot * KEYER_MSG_MAX;
    cw_msg_len__eeprom[slot] = cw_msg_store_len;
    for (i = 0; i < cw_msg_store_len; i++)
        cw_msg__eeprom[base + i] = cw_msg_ram[i];
    cw_msg_len = cw_msg_store_len;
}

static void keyer_play_message(void) {
    uint8_t i;
    uint16_t wait;

    msg_load_from_eeprom(mem_current_slot);
    if (cw_msg_len == 0)
        return;

    /*
     * Wait for paddles open so a "tap to abort" still held does not
     * instantly kill this play. Timeout ~0.5 s then start anyway.
     */
    for (wait = 0; wait < 500 && paddles_closed(); wait++) {
        __delay_ms(1);
    }

    mem_play_active = TRUE;
    mem_play_abort = FALSE;
    mem_abort_armed = FALSE; /* need open then close to abort */
    keyer_update();

    for (i = 0; i < cw_msg_len; i++) {
        if (mem_play_abort)
            break;
        send_morse_char((char)cw_msg_ram[i]);
    }

    set_keyer_out(KEY_OFF);
    keyer_idle_release();
    mem_play_active = FALSE;
    mem_play_abort = FALSE;
    mem_abort_armed = FALSE;
}

void keyer() {
    int32_t loop_count = 0;

    /* Don't fight message playback */
    if (mem_play_active)
        return;

    /*
     * Straight key: either paddle contact = continuous key/sidetone while held.
     * Old logic was "bug" style (one side auto-dots, other briefly keyed then
     * keyer_idle_release() killed the tone → click/thump only).
     * Fixed 2026-08-07 (Grok / MSCC).
     */
    if (cw_keyer_mode == KEYER_STRAIGHT) {
        kcwl = !(KEY_0_GetValue());
        kcwr = !(KEY_1_GetValue());
        /* Both sides solid — ignore paddle reverse for "either key" closure */
        if (kcwl || kcwr) {
            set_keyer_out(KEY_ON);
        } else {
            set_keyer_out(KEY_OFF);
            keyer_idle_release();
        }
        /* Brief yield so main can still service I2C */
        loop_count = 0;
        while (loop_count++ < 50) {
            __delay_us(1);
        }
        return;
    }

    key_state = CHECK;
    while (key_state != EXITLOOP) {
        kcwl = !(KEY_0_GetValue());
        kcwr = !(KEY_1_GetValue());
        switch (key_state) {
            case CHECK:
                if (*kdot)
                    key_state = PREDOT;
                else if (*kdash)
                    key_state = PREDASH;
                else {
                    set_keyer_out(KEY_OFF);
                    key_state = EXITLOOP;
                }
                break;
            case PREDOT:
                clear_memory();
                key_state = SENDDOT;
                break;
            case PREDASH:
                clear_memory();
                key_state = SENDDASH;
                break;
            case SENDDOT:
                set_keyer_out(KEY_ON);
                if (kdelay == dot_delay) {
                    kdelay = 0;
                    set_keyer_out(KEY_OFF);
                    key_state = DOTDELAY;
                } else
                    kdelay++;
                if (cw_keyer_mode == KEYER_MODE_A) {
                    if (!*kdot && !*kdash)
                        dash_memory = 0;
                    else if (*kdash)
                        dash_memory = 1;
                } else if (cw_keyer_mode == KEYER_MODE_B) {
                    if (*kdash)
                        dash_memory = 1;
                }
                break;
            case SENDDASH:
                set_keyer_out(KEY_ON);
                if (kdelay == dash_delay) {
                    kdelay = 0;
                    set_keyer_out(KEY_OFF);
                    key_state = DASHDELAY;
                } else
                    kdelay++;
                if (cw_keyer_mode == KEYER_MODE_A) {
                    if (!*kdot && !*kdash)
                        dot_memory = 0;
                    else if (*kdot)
                        dot_memory = 1;
                } else if (cw_keyer_mode == KEYER_MODE_B) {
                    if (*kdot)
                        dot_memory = 1;
                }
                break;
            case DOTDELAY:
                if (kdelay == dot_delay) {
                    kdelay = 0;
                    if (!*kdot && cw_keyer_mode == KEYER_STRAIGHT)
                        key_state = EXITLOOP;
                    else if (dash_memory)
                        key_state = PREDASH;
                    else
                        key_state = DOTHELD;
                } else
                    kdelay++;
                if (*kdash)
                    dash_memory = 1;
                break;
            case DASHDELAY:
                if (kdelay == dot_delay) {
                    kdelay = 0;
                    if (dot_memory)
                        key_state = PREDOT;
                    else
                        key_state = DASHHELD;
                } else
                    kdelay++;
                if (*kdot)
                    dot_memory = 1;
                break;
            case DOTHELD:
                if (*kdot)
                    key_state = PREDOT;
                else if (*kdash)
                    key_state = PREDASH;
                else if (cw_keyer_spacing) {
                    clear_memory();
                    key_state = LETTERSPACE;
                } else
                    key_state = EXITLOOP;
                break;
            case DASHHELD:
                if (*kdash)
                    key_state = PREDASH;
                else if (*kdot)
                    key_state = PREDOT;
                else if (cw_keyer_spacing) {
                    clear_memory();
                    key_state = LETTERSPACE;
                } else
                    key_state = EXITLOOP;
                break;
            case LETTERSPACE:
                if (kdelay == 2 * dot_delay) {
                    kdelay = 0;
                    if (dot_memory)
                        key_state = PREDOT;
                    else if (dash_memory)
                        key_state = PREDASH;
                    else
                        key_state = EXITLOOP;
                } else
                    kdelay++;
                if (*kdot)
                    dot_memory = 1;
                if (*kdash)
                    dash_memory = 1;
                break;
            default:
                key_state = EXITLOOP;
        }
        loop_count = 0;
        while (loop_count++ < cw_loop_delay) {
            __delay_us(1);
        }
    }
    keyer_idle_release();
}

void CW_Update_Config() {
    uint8_t scan_msg = QUEUE_EMPTY;
    uint8_t p1 = 0;
    uint8_t p2 = 0;

    scan_msg = CMD_dequeue(&p1, &p2);
    if (scan_msg == QUEUE_EMPTY)
        return;

    /* Do not force NCO off for play — sidetone is managed inside keyer_play_message */
    if (scan_msg != CMD_SET_KEYER_MEMORY || p1 != KEYER_MEM_PLAY)
        NCO1CONbits.N1EN = 0;
    switch (scan_msg) {
        case SET_CW_PADDLE:
            cw_keys_reversed = p1;
            cw_keys_reversed__eeprom = p1;
            break;
        case SET_WEIGHT:
            cw_keyer_weight = p1;
            cw_keyer_weight__eeprom = p1;
            break;
        case SET_KEYER_MODE:
            cw_keyer_mode = p1;
            cw_keyer_mode__eeprom = p1;
            break;
        case SET_WPM:
            if (p1 == 5)
                p1 = 6;
            cw_keyer_speed = p1;
            cw_keyer_speed__eeprom = p1;
            break;
        case SET_SPACING:
            cw_keyer_spacing = p1;
            cw_keyer_spacing__eeprom = p1;
            break;
        case SET_MEM_TEXT_WPM:
            /* 0=off; 1–4 → off; 5–60 = text/overall WPM for memory play */
            if (p1 != 0 && p1 < 5)
                p1 = 0;
            if (p1 > 60)
                p1 = 60;
            cw_mem_text_wpm = p1;
            cw_mem_text_wpm__eeprom = p1;
            break;
        case SET_IAMBIC_TUNING:
            break;
        case SET_SIDE_TONE:
            switch (p1) {
                case SIDE_TONE_400:
                    NCO1INCH = 0;
                    NCO1INCL = 0x1A;
                    NCO1INCH__eeprom = 0;
                    NCO1INCL__eeprom = 0x1A;
                    break;
                case SIDE_TONE_600:
                    NCO1INCH = 0;
                    NCO1INCL = 0x27;
                    NCO1INCH__eeprom = 0;
                    NCO1INCL__eeprom = 0x27;
                    break;
                case SIDE_TONE_800:
                    NCO1INCH = 0;
                    NCO1INCL = 0x34;
                    NCO1INCH__eeprom = 0;
                    NCO1INCL__eeprom = 0x34;
                    break;
                case SIDE_TONE_1000:
                    NCO1INCH = 0;
                    NCO1INCL = 0x42;
                    NCO1INCH__eeprom = 0;
                    NCO1INCL__eeprom = 0x42;
                    break;
                default:
                    NCO1INCH = 0;
                    NCO1INCL = 0x27;
                    NCO1INCH__eeprom = 0;
                    NCO1INCL__eeprom = 0x27;
            }
            break;
        case CMD_SET_KEYER_MEMORY:
            /* After subcmd 3, next param must be slot 0..3 (sticky). */
            if (mem_select_pending) {
                mem_select_pending = FALSE;
                if (p1 < KEYER_NUM_SLOTS) {
                    mem_current_slot = p1;
                    break;
                }
                /* Not a slot — fall through and treat p1 as a normal mem param */
            }
            if (p1 == KEYER_MEM_SELECT) {
                mem_select_pending = TRUE;
            } else if (p1 == KEYER_MEM_PLAY) {
                keyer_play_message();
            } else if (p1 == KEYER_MEM_STORE_BEGIN) {
                cw_msg_store_len = 0;
            } else if (p1 == KEYER_MEM_STORE_END) {
                msg_save_to_eeprom(mem_current_slot);
            } else if (p1 >= 0x20 && p1 <= 0x7E) {
                if (cw_msg_store_len < KEYER_MSG_MAX) {
                    cw_msg_ram[cw_msg_store_len++] = p1;
                }
            }
            break;
        default:
            break;
    }
    keyer_update();
}

void CW_Initialize() {
    cw_keyer_speed = cw_keyer_speed__eeprom;
    cw_keys_reversed = cw_keys_reversed__eeprom;
    cw_keyer_weight = cw_keyer_weight__eeprom;
    cw_keyer_mode = cw_keyer_mode__eeprom;
    cw_keyer_spacing = cw_keyer_spacing__eeprom;
    cw_mem_text_wpm = cw_mem_text_wpm__eeprom;
    NCO1INCH = NCO1INCH__eeprom;
    NCO1INCL = NCO1INCL__eeprom;
    NCO1CONbits.N1EN = 0;
    mem_current_slot = 0;
    mem_select_pending = FALSE;
    msg_load_from_eeprom(0);
    keyer_update();
}

void I2C_Initialize() {
    I2C1_Initialize();
    I2C1_Open();
    I2C1_SlaveSetReadIntHandler(I2C_SlaveReceiveCallbackHandler);
    I2C1_SlaveSetAddrIntHandler(I2C_SlaveAddressCallbackHandler);
    I2C1_SlaveSetWriteIntHandler(I2C_SlaveTransmitCallbackHandler);
    I2C1_SlaveSetBusColIntHandler(I2C_SlaveCollisionCallbackHandler);
}

void main(void) {
    __delay_ms(1000);
    SYSTEM_Initialize();
    I2C_Initialize();
    TMR3_Initialize();
    TMR3_SetInterruptHandler(Audio_interrupt_time_out);
    TMR3_StopTimer();
    TMR3_WriteTimer(0xE000);
    TMR3_Reload();
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();
    CW_Initialize();
    KEY_0_SetPullup();
    KEY_1_SetPullup();
    KEY_1A_SetPullup();
    KEY_0A_SetPullup();
    KEY_1_SetLow();
    KEY_0_SetLow();
    KEY_1A_SetHigh();
    KEY_0A_SetHigh();
    RX_CW_SetLow();
    TX_CW_SetHigh();

    while (1) {
        keyer();
        if (CMD_add_queue_busy == 0) {
            CW_Update_Config();
        }
    }
}
