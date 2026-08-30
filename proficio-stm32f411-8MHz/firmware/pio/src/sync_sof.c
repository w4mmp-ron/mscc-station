/**
 * USB SOF clock sync — PSoC SyncSOF / FracN equivalent.
 *
 * PSoC: measure SOF frame position → PD loop → FracN.
 * STM32: count I2S buffer completions per USB 1 ms SOF → trim PLLI2S N.
 *
 * Target: one I2S buffer (96 stereo frames @ 96 kHz) per SOF.
 * No USB feedback endpoint (PSoC bSynchAddress = 0); implicit clock lock.
 */
#include "sync_sof.h"
#include "stm32f4xx_hal.h"

#define SYNC_P_NUM    1
#define SYNC_P_DEN    32
#define SYNC_D_NUM    1
#define SYNC_D_DEN    4

#define PLLI2S_N_NOM  192u
#define PLLI2S_N_MIN  (PLLI2S_N_NOM - 4u)
#define PLLI2S_N_MAX  (PLLI2S_N_NOM + 4u)
#define PLLI2S_R_NOM  2u

static uint8_t  s_running;
static volatile uint16_t s_i2s_frames;
static volatile int16_t  s_last_error;
static int16_t  s_prev_error;
static uint16_t s_plln = PLLI2S_N_NOM;
static uint8_t  s_settle;

static void plli2s_apply(uint16_t n)
{
    RCC_PeriphCLKInitTypeDef p = {0};

    if (n < PLLI2S_N_MIN) {
        n = PLLI2S_N_MIN;
    }
    if (n > PLLI2S_N_MAX) {
        n = PLLI2S_N_MAX;
    }
    if (n == s_plln) {
        return;
    }
    s_plln = n;

    p.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    p.PLLI2S.PLLI2SN = s_plln;
    p.PLLI2S.PLLI2SR = PLLI2S_R_NOM;
    (void)HAL_RCCEx_PeriphCLKConfig(&p);
}

void sync_sof_init(void)
{
    s_running = 0;
    s_i2s_frames = 0;
    s_last_error = 0;
    s_prev_error = 0;
    s_plln = PLLI2S_N_NOM;
    s_settle = 0;
}

void sync_sof_start(void)
{
    s_i2s_frames = 0;
    s_prev_error = 0;
    s_settle = 16;
    s_running = 1;
    plli2s_apply(PLLI2S_N_NOM);
}

void sync_sof_stop(void)
{
    s_running = 0;
    plli2s_apply(PLLI2S_N_NOM);
}

uint8_t sync_sof_running(void)
{
    return s_running;
}

int16_t sync_sof_last_error(void)
{
    return s_last_error;
}

void sync_sof_on_i2s_frame(void)
{
    if (s_running) {
        s_i2s_frames++;
    }
}

void sync_sof_on_sof(void)
{
    int16_t cur_error, diff, p_term, d_term;
    int32_t adj;
    uint16_t frames;

    if (!s_running) {
        return;
    }

    frames = s_i2s_frames;
    s_i2s_frames = 0;

    /* Expect 1 completed I2S buffer per 1 ms SOF */
    cur_error = (int16_t)((int16_t)frames - 1);
    s_last_error = cur_error;

    if (s_settle) {
        s_settle--;
        s_prev_error = cur_error;
        return;
    }

    /* Only adjust when error persists (ignore single-frame jitter) */
    if (cur_error == 0 && s_prev_error == 0) {
        return;
    }

    diff = (int16_t)(cur_error - s_prev_error);
    p_term = (int16_t)((cur_error * SYNC_P_NUM) / SYNC_P_DEN);
    d_term = (int16_t)((diff * SYNC_D_NUM) / SYNC_D_DEN);
    s_prev_error = cur_error;

    /* Fast clock → more frames → positive error → lower PLLI2S N */
    adj = (int32_t)s_plln - (int32_t)p_term - (int32_t)d_term;
    if (adj < (int32_t)PLLI2S_N_MIN) {
        adj = PLLI2S_N_MIN;
    }
    if (adj > (int32_t)PLLI2S_N_MAX) {
        adj = PLLI2S_N_MAX;
    }
    plli2s_apply((uint16_t)adj);
}
