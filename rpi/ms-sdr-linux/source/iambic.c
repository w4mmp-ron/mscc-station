#include "extern.h"
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <sys/time.h>
#include <wiringPi.h>


#define KEY_UP 0
#define KEY_DOWN 1
#define STATUS_KEY_1 1
#define CFG_LAG_MAX 25
#define CFG_WEIGHT_DIST 0.50

// Keying memory
#define CFG_MEMORY_NONE 0
#define CFG_MEMORY_DAH 1
#define CFG_MEMORY_DIT 2
#define CFG_MEMORY_BOTH 3

// Morse code timings.
#define DIT 1
#define DAH 3

// Keying modes
#define CFG_MODE_IAMBIC 0
#define CFG_MODE_ULTIMATIC 1
#define CFG_MODE_BUG 2
#define CFG_MODE_STRAIGHT 3

// Keying memory
#define CFG_MEMORY_TYPE_A 0
#define CFG_MEMORY_TYPE_DAH 1
#define CFG_MEMORY_TYPE_DIT 2
#define CFG_MEMORY_TYPE_B 3

// Keying spacing
#define CFG_SPACING_NONE 0
#define CFG_SPACING_EL 1
#define CFG_SPACING_CHAR 2
#define CFG_SPACING_WORD 3

// Paddle reversal
#define CFG_PADDLE_NORMAL 0
#define CFG_PADDLE_REVERSE 1

// Debounce straight key both up and down.
#define KEY_DEBOUNCE_SRAIGHT 8000

// Start looking for key input 2ms early in case the key is bouncing
// up at the moment we're suppose to start a new dit or dah.
#define KEY_DEBOUNCE_IAMBIC 2000


//#define IAMBIC_CALIBRATION_SPEED  1200000
//#define RATIO_FACTOR 10000

#define CW_KEY_IN_0 6
#define CW_KEY_IN_1 7
#define CW_KEY_OUT_0 8
#define CW_KEY_OUT_1 12
#define TX_PIN 8
#define KEY_STATE_DOWN 0
#define KEY_STATE_UP  1
#define CW_SLEEP_TIME 10

typedef void(*cfg_function)(uint8_t);

float cfg_speed_wpm;
long cfg_speed_micros;
long tx_timer = 0;
BOOL tx_flag = TRUE;
int count = 0;
byte key_state = KEY_STATE_UP;

struct cfg cfg;

long micro_sec_key() {
    struct timeval tv;
    long micro_seconds = 0;

    gettimeofday(&tv, NULL);
    micro_seconds = 1000000 * tv.tv_sec + tv.tv_usec;
    return micro_seconds;
}

float cfg_get_speed() {
    return cfg.speed_wpm;
}

long cfg_get_speed_micros() {
    return cfg_speed_micros;
}

void cfg_set_speed(float wpm) {
    cfg.speed_wpm = wpm;
    cfg_speed_micros = (1200000 / (long) wpm);
}

void tx_loop(long mark) {
    long result = 0;
    result = tx_timer - mark;
    if (result < 0) {
        if (key_state == KEY_STATE_DOWN) {
            digitalWrite(CW_KEY_OUT_1, HIGH);
            key_state = KEY_STATE_UP;
        }
    }
}

void tx_send(long mark) {

    tx_timer = mark;
    if (key_state == KEY_STATE_UP) {
        key_state = KEY_STATE_DOWN;
        digitalWrite(CW_KEY_OUT_1, LOW);
           }
}

BOOL tx_enabled() {
    return tx_flag;
}

void tx_enable() {
    tx_flag = TRUE;
}

void tx_disable() {
    tx_flag = FALSE;
}

uint8_t key_read() {
    uint8_t k0;
    uint8_t k1;

    k0 = digitalRead(CW_KEY_IN_0) ^ 1;
    k1 = digitalRead(CW_KEY_IN_1) ^ 1;

    if (cfg.mode == CFG_MODE_STRAIGHT) {
        k0 <<= 1;
        k1 = 0;
    } else if (cfg.paddle == CFG_PADDLE_NORMAL) {
        k1 <<= 1;
    } else {
        k0 <<= 1;
    }
    return (k0 | k1);
}

uint8_t key_loop(long mark) {
    static uint8_t last, spacing = 2, ultimatic, state = 3, staged = 0, mcode = 0x80;
    static long read_after, start_after = 0;
    uint8_t k0, k1, ret = 0;
    long i;

    k0 = key_read();
    k1 = k0 & 2;
    k0 = k0 & 1;
    switch (state) {
        case 1: // waiting until ready for read
            if (cfg.spacing == CFG_SPACING_NONE)
                if ((k0 && last == DIT) || (k1 && last == DAH))
                    read_after = mark + KEY_DEBOUNCE_IAMBIC;
            if (read_after - mark < 0) state = 2;
            break;
        case 2: // waiting and reading
            if (start_after - mark < 0) state = 3;
            if (spacing < 4) break;
            //nobreak;
        case 3: // idle, spacing
            if (start_after - mark < 0) {
                switch (spacing) {
                    case 0:
                    case 2:
                    case 3:
                        break;
                    case 1:
                        ret = mcode;
                        mcode = 0x80;
                        if (cfg.spacing >= CFG_SPACING_CHAR) state = 2;
                        break;
                    case 4:
                        ret = mcode;
                        //nobreak
                    case 5:
                    case 6:
                        if (cfg.spacing >= CFG_SPACING_WORD) state = 2;
                        break;
                }
                if (spacing < 7) spacing += 1;
                if (cfg.mode == CFG_MODE_BUG) state = 3;
                start_after += DIT * cfg_speed_micros;
            }
            break;
        case 4: // debouncing straight/bug down
            if (start_after - mark < 0) state = 5;
            break;
        case 5: // holding straight/bug
            break;
        case 6: // debouncing straight/bug up
            if (read_after - mark < 0) {
                state = 3;
                staged = 0;
                start_after = mark + DIT * cfg_speed_micros;
                spacing = 0;
                if (mcode & 0x01) {
                    mcode = 0xFF;
                } else {
                    mcode >>= 1;
                    mcode |= 0x80;
                }
            }
            break;
    }

    if (cfg.mode == CFG_MODE_STRAIGHT || cfg.mode == CFG_MODE_BUG) {
        if (k1) {
            i = mark + KEY_DEBOUNCE_SRAIGHT;
            if (state < 4) {
                state = 4;
                start_after = i;
            }
            if (state < 6) {
                read_after = i;
                tx_send(i);
            }
            last = DAH;
            staged = 0;
        } else if (state == 5) {
            if (staged == DIT) {
                state = 3;
            } else {
                state = 6;
                tx_send(mark);
            }
        }
    } else {
        if (state > 3) state = 6;
    }

    if (!staged) {
        if (state > 1) {
            if (k0 && k1) {
                if (ultimatic && cfg.mode == CFG_MODE_ULTIMATIC) staged = last;
                else if (last == DIT) staged = DAH;
                else staged = DIT;
                ultimatic = 1;
            } else {
                if (k0) staged = DIT;
                if (k1) staged = DAH;
                ultimatic = 0;
            }
        } else if (!ultimatic || cfg.mode != CFG_MODE_ULTIMATIC) {
            if (k0 && (last == DAH || spacing > 0)) {
                if (cfg.memory & CFG_MEMORY_DIT) {
                    staged = DIT;
                    ultimatic = 1;
                }
            }
            if (k1 && (last == DIT || spacing > 0)) {
                if (cfg.memory & CFG_MEMORY_DAH) {
                    staged = DAH;
                    ultimatic = 1;
                }
            }
        }
    }

    if (state == 3 && staged) {
        i = mark + (long) staged * cfg_speed_micros;
        i += DIT * cfg_speed_micros * (cfg.weight * 2 - 1);
        read_after = start_after = i + (long) cfg.lag * 1000;
        tx_send(start_after);
        i += DIT * cfg_speed_micros * (2.0 - cfg.weight * 2);
        if (cfg.spacing >= CFG_SPACING_EL) {
            read_after = i - KEY_DEBOUNCE_IAMBIC;
            start_after = i;
        }
        spacing = 0;
        if (mcode & 0x01) {
            if (mcode != 0x01 || staged == DAH) mcode = 0xFF;
        } else {
            mcode >>= 1;
            if (staged == DAH) mcode |= 0x80;
        }
        last = staged;
        staged = 0;
        state = 1;
    }
    if (cfg.mode == CFG_MODE_STRAIGHT) return 0;
    return ret;
}

void print_cfg() {
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Manage_CW_Key . %s . cfg configuration . mode:%d, "
            "spacing:%d, memory:%d, paddle:%d, weight:%f, lag:%d, speed: %f  \n",
            line_number++, __func__,
            cfg.mode, cfg.spacing, cfg.memory, cfg.paddle, cfg.weight, cfg.lag, cfg.speed_wpm);
}

void *Manage_CW_Key(void *t) {
    int count = 0;
    int count_limit = 2;
    long mark;
    uint8_t mcode;
    uint8_t mode_reported = 0;
    int sleep_ret;
    float previous_speed_wpm = 0.0f;

    cfg.mode = CFG_MODE_IAMBIC;
    cfg.spacing = CFG_SPACING_EL;
    cfg.memory = CFG_MEMORY_TYPE_A;
    cfg.paddle = CFG_PADDLE_NORMAL;
    cfg.weight = CFG_WEIGHT_DIST;
    cfg.lag = 0;
    digitalWrite(CW_KEY_OUT_1, HIGH);
    while (G_all_threads_run) {
        if (G_mode == 'C') {
            if (mode_reported == 0) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s . In CW Mode  \n", line_number++, __func__);
                mode_reported = 1;
            }
            count = 0;
            count_limit = 1;
            mark = micro_sec_key();
            mcode = key_loop(mark);
            tx_loop(mark);
            sleep_ret = usleep(CW_SLEEP_TIME);
            if (previous_speed_wpm != cfg.speed_wpm) {
                cfg_set_speed(cfg.speed_wpm);
                previous_speed_wpm = cfg.speed_wpm;
            }
            if (G_Mscc_Ini_updated == TRUE) {
                G_Mscc_Ini_updated = FALSE;
                print_cfg();
            }
        } else {
            Sleep(3);
            digitalWrite(CW_KEY_OUT_1, HIGH);
            if (mode_reported == 1) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s . Out of CW Mode. usleep return: %d \n",
                        line_number++, __func__, sleep_ret);
                mode_reported = 0;
            }
            if (previous_speed_wpm != cfg.speed_wpm) {
                cfg_set_speed(cfg.speed_wpm);
                previous_speed_wpm = cfg.speed_wpm;
            }
            if (G_Mscc_Ini_updated == TRUE) {
                G_Mscc_Ini_updated = FALSE;
                print_cfg();
            }
        }
    }
    digitalWrite(CW_KEY_OUT_1, HIGH);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s . NORMAL EXIT  \n", line_number++, __func__);
    pthread_exit(0);
    return NULL;
}
