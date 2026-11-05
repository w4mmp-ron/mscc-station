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
  2026-11-05  SET_PADDLE_FARNSWORTH 0x9D (0/1): paddle letter gap stretched to the
              Farnsworth letter gap (text WPM from 0x76). On also enables the
              letter space (LETTERSPACE) without SET_SPACING.
  2026-11-05  keyer-buffer: BUFFERED FARNSWORTH (Ron's design). Timing rebuilt on a
              1 ms TMR2 tick (ISR only counts ticks); no busy-wait loops, every part
              is a state machine stepped once per tick:
                - paddles: iambic A/B / straight, live sidetone;
                - player: keys KEY_0A from a letter buffer or a CQ memory, with
                  Farnsworth gaps (text WPM from 0x76);
                - announcer: Morse on the sidetone only (STP / RES).
              0x9D = 1: paddles drive the sidetone only; each finished letter goes
              into the buffer and the player sends it on the air with Farnsworth
              spacing. 20 letters waiting -> STP on the sidetone, paddles ignored,
              buffer drains, RES on the sidetone, paddles back once released.
              0x9D = 0: paddles key KEY_0A + sidetone directly as before (with
              SET_SPACING autospace). Element times are now real ms (1200/WPM).
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
/* Buffered Farnsworth on the paddles: 0=off, 1=on (gaps at the 0x76 text WPM) */
#define SET_PADDLE_FARNSWORTH 0x9D
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

#define KEY_ON 1
#define KEY_OFF 0

#define SIDE_TONE_400 0
#define SIDE_TONE_600 1
#define SIDE_TONE_800 2
#define SIDE_TONE_1000 3

/* 4 slots × (1 len + KEYER_MSG_MAX) must fit ~240 B free EEPROM after settings */
#define KEYER_NUM_SLOTS 4
#define KEYER_MSG_MAX   48

/* Buffered Farnsworth (0x9D) */
#define BUF_SIZE            64  /* letter / word-space entries, power of 2 (3 bytes each) */
#define BUF_LIMIT           20  /* letters waiting -> STP, paddles suspended (max ~60 entries with word spaces) */
#define LETTER_END_DITS      2  /* gap after the last mark that closes a letter */
#define WORD_GAP_DITS        5  /* gap after the last mark that adds a word space */
#define LETTER_MAX_ELEMENTS 16  /* longer -> letter closed, next element starts a new one */
#define MEM_START_WAIT_MS  500  /* CQ play: wait this long for the paddles to open */

static int cw_keyer_speed = 18;
static int cw_keyer_weight = 50;
static int cw_keyer_mode = KEYER_MODE_B;
static int cw_keyer_spacing = 0;
static int cw_keys_reversed = 1;
static int cw_mem_text_wpm = 0; /* 0=off; else Farnsworth text WPM for memory play */
static int cw_paddle_farns = 0; /* SET_PADDLE_FARNSWORTH 0x9D: 0=off, 1=buffered */

/* All times in ms (1 ms tick) */
static uint16_t dot_delay = 0;
static uint16_t dash_delay = 0;
/* ARRL / PARIS Farnsworth gaps (see keyer_update) */
static uint16_t mem_letter_gap = 0;
static uint16_t mem_word_gap = 0;
/* Player gaps: whole letter gap after the last mark, extra time for a word space */
static uint16_t tx_letter_gap = 0;
static uint16_t tx_word_extra = 0;

static int kcwl = 0;
static int kcwr = 0;
static int *kdot;
static int *kdash;

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
static uint8_t mem_play_pending = FALSE;   /* PLAY received, waiting for paddles open */
static uint16_t mem_wait_ms = 0;
static uint8_t mem_play_active = FALSE;
static uint8_t mem_abort_armed = FALSE;    /* abort needs paddles open, then closed */

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

/* 1 ms tick from TMR2 (interrupt_manager.c). The ISR only counts. */
volatile uint8_t keyer_ticks = 0;

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

/* Called from the TMR2 interrupt every 1 ms: count only */
void keyer_tick_isr(void) {
    if (keyer_ticks < 255u)
        keyer_ticks++;
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
    dot_delay = (uint16_t)(1200 / cw_keyer_speed);            /* ms */
    dash_delay = (uint16_t)(((uint32_t)dot_delay * 3u * (uint32_t)cw_keyer_weight) / 50u);

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
            mem_letter_gap = (uint16_t)((3 * gap * (int32_t)dot_delay) /
                ((int32_t)19 * (int32_t)text_wpm));
            mem_word_gap = (uint16_t)((4 * gap * (int32_t)dot_delay) /
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
    /* Player: the whole letter gap after the last mark is never under 3 dits,
     * the extra for a word space never under 4 (3 + 4 = 7 dits). */
    tx_letter_gap = mem_letter_gap;
    if (tx_letter_gap < 3 * dot_delay)
        tx_letter_gap = 3 * dot_delay;
    tx_word_extra = mem_word_gap;
    if (tx_word_extra < 4 * dot_delay)
        tx_word_extra = 4 * dot_delay;

    if (cw_keys_reversed) {
        kdot = &kcwr;
        kdash = &kcwl;
    } else {
        kdot = &kcwl;
        kdash = &kcwr;
    }
}

static uint8_t paddles_closed(void) {
    return (uint8_t)((!(KEY_0_GetValue())) || (!(KEY_1_GetValue())));
}

/* ------------------------------------------------------------------------- */
/* Letter buffer (buffered Farnsworth): len 0 = word space                    */
/* ------------------------------------------------------------------------- */

static uint8_t buf_len[BUF_SIZE];
static uint16_t buf_bits[BUF_SIZE];   /* bit i = 1: element i is a dash */
static uint8_t buf_head = 0;
static uint8_t buf_tail = 0;
static uint8_t buf_count = 0;
static uint8_t buf_letters = 0;       /* letters waiting (word spaces not counted) */

static void buf_clear(void) {
    buf_head = buf_tail = buf_count = buf_letters = 0;
}

static void buf_push(uint8_t len, uint16_t bits) {
    if (buf_count >= BUF_SIZE)
        return;
    buf_len[buf_head] = len;
    buf_bits[buf_head] = bits;
    buf_head = (uint8_t)((buf_head + 1u) & (BUF_SIZE - 1u));
    buf_count++;
    if (len != 0)
        buf_letters++;
}

static uint8_t buf_pop(uint8_t *len, uint16_t *bits) {
    if (buf_count == 0)
        return FALSE;
    *len = buf_len[buf_tail];
    *bits = buf_bits[buf_tail];
    buf_tail = (uint8_t)((buf_tail + 1u) & (BUF_SIZE - 1u));
    buf_count--;
    if (*len != 0)
        buf_letters--;
    return TRUE;
}

/* ------------------------------------------------------------------------- */
/* Sequencer: plays letters (from the buffer or a text) as marks and gaps.    */
/* Used twice: the player (keys KEY_0A) and the announcer (sidetone only).    */
/* ------------------------------------------------------------------------- */

enum { SQ_IDLE = 0, SQ_MARK, SQ_SPACE, SQ_GAP };

typedef struct {
    uint8_t state;
    uint8_t mark;          /* 1 while a mark is being sent (output) */
    uint8_t tone;          /* marks also sound on the sidetone */
    uint8_t farns;         /* 1: Farnsworth gaps (tx_*), 0: standard 3 / 4 dits */
    uint8_t use_buf;       /* 1: symbols from the letter buffer, 0: from text */
    const char *text;
    uint8_t text_pos;
    uint8_t text_len;
    uint8_t len;           /* elements in the current letter */
    uint8_t idx;
    uint16_t bits;
    uint16_t timer;
    uint16_t target;
    uint16_t idle_ms;      /* time spent idle since the last gap ended */
} seq_t;

static seq_t player;     /* on the air: KEY_0A */
static seq_t announcer;  /* sidetone only: STP / RES */

/* 0 = nothing, 1 = letter in *len / *bits, 2 = word space */
static uint8_t seq_next(seq_t *s, uint8_t *len, uint16_t *bits) {
    const char *p;
    char c;
    uint8_t n;

    if (s->use_buf) {
        if (!buf_pop(len, bits))
            return 0;
        return (*len == 0) ? 2u : 1u;
    }
    while (s->text != NULL && s->text_pos < s->text_len) {
        c = s->text[s->text_pos++];
        if (c == ' ' || c == '\t')
            return 2;
        p = morse_lookup(c);
        if (p == NULL)
            continue;   /* unknown character: skip */
        *bits = 0;
        for (n = 0; p[n] != '\0' && n < LETTER_MAX_ELEMENTS; n++) {
            if (p[n] == '-')
                *bits |= (uint16_t)(1u << n);
        }
        *len = n;
        if (n > 0)
            return 1;
    }
    return 0;
}

static void seq_start_text(seq_t *s, const char *text, uint8_t len) {
    s->use_buf = FALSE;
    s->text = text;
    s->text_pos = 0;
    s->text_len = len;
}

static void seq_stop(seq_t *s) {
    s->state = SQ_IDLE;
    s->mark = 0;
    s->idle_ms = 0;
}

static uint16_t seq_elem_ms(seq_t *s) {
    return ((s->bits >> s->idx) & 1u) ? dash_delay : dot_delay;
}

static void seq_tick(seq_t *s) {
    uint8_t got;
    uint16_t extra;

    switch (s->state) {
    case SQ_IDLE:
        got = seq_next(s, &s->len, &s->bits);
        if (got == 1) {
            s->idx = 0;
            s->timer = 0;
            s->target = seq_elem_ms(s);
            s->mark = 1;
            s->state = SQ_MARK;
        } else if (got == 2) {
            /* Word space: the letter gap is already done; the idle time since
             * then counts towards the extra, so a long pause adds nothing. */
            extra = s->farns ? tx_word_extra : (uint16_t)(4u * dot_delay);
            s->target = (s->idle_ms >= extra) ? 0u : (uint16_t)(extra - s->idle_ms);
            s->timer = 0;
            if (s->target > 0)
                s->state = SQ_GAP;
        } else if (s->idle_ms < 60000u) {
            s->idle_ms++;
        }
        break;
    case SQ_MARK:
        if (++s->timer >= s->target) {
            s->mark = 0;
            s->timer = 0;
            s->idx++;
            if (s->idx < s->len) {
                s->target = dot_delay;          /* space inside the letter */
                s->state = SQ_SPACE;
            } else {
                s->target = s->farns ? tx_letter_gap : (uint16_t)(3u * dot_delay);
                s->state = SQ_GAP;
            }
        }
        break;
    case SQ_SPACE:
        if (++s->timer >= s->target) {
            s->timer = 0;
            s->target = seq_elem_ms(s);
            s->mark = 1;
            s->state = SQ_MARK;
        }
        break;
    case SQ_GAP:
        if (++s->timer >= s->target) {
            s->idle_ms = 0;
            s->state = SQ_IDLE;
        }
        break;
    default:
        seq_stop(s);
        break;
    }
}

/* ------------------------------------------------------------------------- */
/* Paddles                                                                    */
/* ------------------------------------------------------------------------- */

enum {
    PD_IDLE = 0,
    PD_MARK,        /* element on */
    PD_SPACE,       /* 1 dit after an element */
    PD_LETTERSPACE, /* direct mode + SET_SPACING: 2 more dits, presses remembered */
    PD_SUSPEND,     /* buffered: over the limit, STP / drain / RES */
    PD_WAIT_RELEASE /* buffered: after RES, wait until both paddles are open */
};

static uint8_t pd_state = PD_IDLE;
static uint8_t pd_mark = 0;        /* element on (sidetone; KEY_0A in direct mode) */
static uint8_t pd_dash = 0;        /* current element is a dash */
static uint16_t pd_timer = 0;
static uint8_t dot_memory = 0;
static uint8_t dash_memory = 0;
static uint8_t pd_suspend_phase = 0;
/* buffered letter being keyed */
static uint8_t letter_open = FALSE;
static uint8_t letter_len = 0;
static uint16_t letter_bits = 0;
static uint16_t pd_gap = 0xFFFFu;  /* ms since the last mark ended */
static uint8_t word_pending = FALSE; /* a letter went in since the last word space */

static uint8_t buffered_mode(void) {
    return (uint8_t)(cw_paddle_farns && cw_keyer_mode != KEYER_STRAIGHT);
}

static void pd_close_letter(void) {
    if (!letter_open)
        return;
    letter_open = FALSE;
    buf_push(letter_len, letter_bits);
    word_pending = TRUE;
    if (buf_letters >= BUF_LIMIT && pd_state != PD_SUSPEND) {
        /* Too far ahead: stop taking paddles, drain; STP after a word-space pause
         * so it does not run into the operator's last letter */
        pd_state = PD_SUSPEND;
        pd_suspend_phase = 0;
        pd_timer = 0;
    }
}

static void pd_start_element(uint8_t dash) {
    if (buffered_mode()) {
        if (letter_open && letter_len >= LETTER_MAX_ELEMENTS) {
            pd_close_letter();
            if (pd_state == PD_SUSPEND)
                return;     /* that letter hit the limit: STP, no new element */
        }
    }
    dot_memory = 0;
    dash_memory = 0;
    pd_dash = dash;
    pd_timer = 0;
    pd_mark = 1;
    pd_state = PD_MARK;
    if (buffered_mode()) {
        if (!letter_open) {
            letter_open = TRUE;
            letter_len = 0;
            letter_bits = 0;
        }
        if (dash)
            letter_bits |= (uint16_t)(1u << letter_len);
        letter_len++;
    }
}

static void pd_end_of_letter(void) {
    if (buffered_mode()) {
        pd_gap = dot_delay;          /* the 1-dit space just done */
        pd_state = PD_IDLE;
    } else if (cw_keyer_spacing) {
        dot_memory = 0;
        dash_memory = 0;
        pd_timer = 0;
        pd_state = PD_LETTERSPACE;
    } else {
        pd_state = PD_IDLE;
    }
}

static void paddle_tick(void) {
    kcwl = !(KEY_0_GetValue());
    kcwr = !(KEY_1_GetValue());

    /* CQ memory: paddles only watch for the abort (open, then closed) */
    if (mem_play_pending || mem_play_active) {
        pd_mark = 0;
        if (!mem_play_active)
            return;
        if (!kcwl && !kcwr) {
            mem_abort_armed = TRUE;
        } else if (mem_abort_armed) {
            mem_play_active = FALSE;
            mem_abort_armed = FALSE;
            /* stop the CQ, but finish the element on the air (no clipped mark);
             * mem_tick() hands the player back to the buffer when it is idle */
            player.text_pos = player.text_len;
            if (player.state == SQ_MARK) {
                player.len = (uint8_t)(player.idx + 1u);
            } else if (player.state == SQ_SPACE) {
                player.state = SQ_GAP;
                player.timer = 0;
                player.target = dot_delay;
            }
            /* the abort press only aborts: paddles back once released */
            pd_state = PD_WAIT_RELEASE;
        }
        return;
    }

    if (cw_keyer_mode == KEYER_STRAIGHT) {
        /* Either contact = key + sidetone while held (not buffered) */
        pd_mark = (uint8_t)(kcwl || kcwr);
        pd_state = PD_IDLE;
        return;
    }

    switch (pd_state) {
    case PD_IDLE:
        if (*kdot) {
            pd_start_element(0);
        } else if (*kdash) {
            pd_start_element(1);
        } else if (buffered_mode()) {
            if (pd_gap < 0xFFFFu)
                pd_gap++;
            if (letter_open && pd_gap >= LETTER_END_DITS * dot_delay)
                pd_close_letter();
            if (pd_state == PD_IDLE && word_pending &&
                pd_gap >= WORD_GAP_DITS * dot_delay) {
                word_pending = FALSE;
                buf_push(0, 0);
            }
        }
        break;

    case PD_MARK:
        if (pd_dash) {
            if (cw_keyer_mode == KEYER_MODE_A) {
                if (!*kdot && !*kdash)
                    dot_memory = 0;
                else if (*kdot)
                    dot_memory = 1;
            } else if (*kdot) {
                dot_memory = 1;
            }
        } else {
            if (cw_keyer_mode == KEYER_MODE_A) {
                if (!*kdot && !*kdash)
                    dash_memory = 0;
                else if (*kdash)
                    dash_memory = 1;
            } else if (*kdash) {
                dash_memory = 1;
            }
        }
        if (++pd_timer >= (pd_dash ? dash_delay : dot_delay)) {
            pd_mark = 0;
            pd_timer = 0;
            pd_state = PD_SPACE;
        }
        break;

    case PD_SPACE:
        if (pd_dash) {
            if (*kdot)
                dot_memory = 1;
        } else {
            if (*kdash)
                dash_memory = 1;
        }
        if (++pd_timer >= dot_delay) {
            pd_timer = 0;
            if (pd_dash) {
                if (dot_memory)
                    pd_start_element(0);
                else if (*kdash)
                    pd_start_element(1);
                else if (*kdot)
                    pd_start_element(0);
                else
                    pd_end_of_letter();
            } else {
                if (dash_memory)
                    pd_start_element(1);
                else if (*kdot)
                    pd_start_element(0);
                else if (*kdash)
                    pd_start_element(1);
                else
                    pd_end_of_letter();
            }
        }
        break;

    case PD_LETTERSPACE:
        if (*kdot)
            dot_memory = 1;
        if (*kdash)
            dash_memory = 1;
        if (++pd_timer >= 2u * dot_delay) {
            pd_timer = 0;
            if (dot_memory)
                pd_start_element(0);
            else if (dash_memory)
                pd_start_element(1);
            else
                pd_state = PD_IDLE;
        }
        break;

    case PD_SUSPEND:
        pd_mark = 0;
        if (pd_suspend_phase == 0) {
            /* pause one word space (7 dits) after the operator's last letter */
            if (++pd_timer >= 7u * dot_delay) {
                seq_start_text(&announcer, "STP", 3);
                pd_suspend_phase = 1;
            }
        } else if (pd_suspend_phase == 1) {
            /* STP sent and the buffer fully on the air -> RES */
            if (announcer.state == SQ_IDLE && announcer.text_pos >= announcer.text_len &&
                buf_count == 0 && player.state == SQ_IDLE) {
                seq_start_text(&announcer, "RES", 3);
                pd_suspend_phase = 2;
            }
        } else if (announcer.state == SQ_IDLE && announcer.text_pos >= announcer.text_len) {
            pd_state = PD_WAIT_RELEASE;
        }
        break;

    case PD_WAIT_RELEASE:
        pd_mark = 0;
        if (!kcwl && !kcwr) {
            letter_open = FALSE;
            word_pending = FALSE;
            pd_gap = 0xFFFFu;
            pd_state = PD_IDLE;
        }
        break;

    default:
        pd_state = PD_IDLE;
        break;
    }
}

/* ------------------------------------------------------------------------- */
/* CQ memory play (through the player)                                        */
/* ------------------------------------------------------------------------- */

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

/* PLAY received: drop anything buffered, start once the paddles are open (or 0.5 s) */
static void mem_play_request(void) {
    msg_load_from_eeprom(mem_current_slot);
    if (cw_msg_len == 0)
        return;
    buf_clear();
    letter_open = FALSE;
    word_pending = FALSE;
    seq_stop(&player);
    mem_play_active = FALSE;
    mem_play_pending = TRUE;
    mem_wait_ms = 0;
}

static void mem_tick(void) {
    if (mem_play_pending) {
        if (!paddles_closed() || ++mem_wait_ms >= MEM_START_WAIT_MS) {
            mem_play_pending = FALSE;
            mem_play_active = TRUE;
            mem_abort_armed = FALSE; /* need open then close to abort */
            player.tone = TRUE;      /* hear the CQ */
            seq_start_text(&player, (const char *)cw_msg_ram, cw_msg_len);
        }
    } else if (!player.use_buf && player.state == SQ_IDLE &&
               player.text_pos >= player.text_len) {
        /* message done (or aborted and the last element finished): back to the buffer */
        mem_play_active = FALSE;
        player.tone = FALSE;
        player.use_buf = TRUE;
    }
}

/* ------------------------------------------------------------------------- */
/* Outputs: KEY_0A (to the radio), NCO1 (sidetone), RX_CW / TX_CW             */
/* ------------------------------------------------------------------------- */

static uint8_t out_key = 0;
static uint8_t out_tone = 0;

static void outputs_update(void) {
    uint8_t key;
    uint8_t tone;
    uint8_t active;

    key = (uint8_t)(player.mark || (pd_mark && !buffered_mode()));
    tone = (uint8_t)(pd_mark || (player.mark && player.tone) || announcer.mark);
    active = (uint8_t)(key || tone || pd_state != PD_IDLE ||
                       player.state != SQ_IDLE || announcer.state != SQ_IDLE ||
                       buf_count != 0 || mem_play_active);

    if (active && tx_active == FALSE) {
        tx_active = TRUE;
        RX_CW_SetHigh();
        TX_CW_SetLow();
    }
    if (key != out_key) {
        out_key = key;
        if (key)
            KEY_0A_SetLow();
        else
            KEY_0A_SetHigh();
    }
    if (tone != out_tone) {
        out_tone = tone;
        NCO1CONbits.N1EN = tone ? 1 : 0;
    }
    if (!active && tx_active == TRUE) {
        tx_active = FALSE;
        RX_CW_SetLow();
        TX_CW_SetHigh();
    }
}

/* One 1 ms step of everything */
static void keyer_step(void) {
    paddle_tick();
    mem_tick();
    seq_tick(&player);
    seq_tick(&announcer);
    outputs_update();
}

static uint8_t take_ticks(void) {
    uint8_t n;

    INTERRUPT_GlobalInterruptDisable();
    n = keyer_ticks;
    keyer_ticks = 0;
    INTERRUPT_GlobalInterruptEnable();
    return n;
}

/* TMR2: Fosc/4 = 8 MHz, 1:64 -> 125 kHz, PR2 = 124 -> 1 kHz */
static void tick_timer_init(void) {
    T2CON = 0;
    TMR2 = 0;
    PR2 = 124;
    T2CONbits.T2CKPS = 3;    /* 1:64 */
    T2CONbits.T2OUTPS = 0;   /* 1:1 */
    PIR1bits.TMR2IF = 0;
    PIE1bits.TMR2IE = 1;
    T2CONbits.TMR2ON = 1;
}

void CW_Update_Config() {
    uint8_t scan_msg = QUEUE_EMPTY;
    uint8_t p1 = 0;
    uint8_t p2 = 0;

    scan_msg = CMD_dequeue(&p1, &p2);
    if (scan_msg == QUEUE_EMPTY)
        return;

    /* The sidetone (NCO1) belongs to outputs_update(); do not switch it here */
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
        case SET_PADDLE_FARNSWORTH:
            /* 0=off, anything else=on. RAM only, see CW_Initialize */
            cw_paddle_farns = (p1 != 0) ? 1 : 0;
            if (!cw_paddle_farns) {
                /* back to direct keying: drop what is waiting */
                buf_clear();
                letter_open = FALSE;
                word_pending = FALSE;
                if (pd_state == PD_SUSPEND || pd_state == PD_WAIT_RELEASE)
                    pd_state = PD_IDLE;
            }
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
                mem_play_request();
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
    /* SET_PADDLE_FARNSWORTH is not kept in EEPROM (a new __eeprom variable moves the
     * settings layout): off at power-up until the radio sends it (pill at boot,
     * ms-sdr at every connect). */
    cw_paddle_farns = 0;
    NCO1INCH = NCO1INCH__eeprom;
    NCO1INCL = NCO1INCL__eeprom;
    NCO1CONbits.N1EN = 0;
    mem_current_slot = 0;
    mem_select_pending = FALSE;
    msg_load_from_eeprom(0);

    memset(&player, 0, sizeof(player));
    player.farns = TRUE;       /* on the air: Farnsworth gaps from 0x76 */
    player.use_buf = TRUE;
    memset(&announcer, 0, sizeof(announcer));
    announcer.tone = TRUE;     /* sidetone only, standard spacing */
    buf_clear();
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
    uint8_t n;

    __delay_ms(1000);
    SYSTEM_Initialize();
    I2C_Initialize();
    TMR3_Initialize();
    TMR3_SetInterruptHandler(Audio_interrupt_time_out);
    TMR3_StopTimer();
    TMR3_WriteTimer(0xE000);
    TMR3_Reload();
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
    tick_timer_init();
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    while (1) {
        /* Every 1 ms tick that came in: one step of paddles / player / outputs */
        n = take_ticks();
        while (n--) {
            keyer_step();
        }
        if (CMD_add_queue_busy == 0) {
            CW_Update_Config();
        }
    }
}
