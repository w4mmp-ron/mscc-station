/*
 * cw-carrier (recv 3.190, 2026-11-20): with CW_CARRIER on, the radio's receive stream is
 * stopped while transmitting in voice / digital / tune with Monitor off, so the radio's
 * receive and transmit streams don't run together (a Pi Zero 2 W's USB loses receive when
 * both run). CW keeps receive running (the radio makes the CW carrier itself, no transmit
 * stream). Monitor on keeps receive running (own signal heard and shown).
 *
 * ms-sdr turns this on with CMD_SET_CW_CARRIER (0x15) value 1 (0 = off, as before).
 * Receive starts again RX_RESTART_MS after transmit ends, after the radio's PA is off, so a
 * glitch from the restart can't reach the transmitted signal (3.149 lesson: stopping the
 * receive stream during transmit audio made transmit gaps on Windows).
 * All stream starts / stops, here and in manage_stream(), hold g_stream_lock.
 */
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include "extern.h"
#include "commands.h"
#include "cw_carrier_rx.h"

extern PaStream *stream;
extern uint8_t G_Monitor;

pthread_mutex_t g_stream_lock = PTHREAD_MUTEX_INITIALIZER;

#define RX_RESTART_MS 300u

static volatile int g_cwc_rx = 0;

static uint32_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

void cw_carrier_rx_set(uint8_t value) {
    print_time();
    fprintf(G_fp_logfile, "[%d] cw-carrier. CMD_SET_CW_CARRIER %d\n", line_number++, value);
    if (value == 0u || value == 1u)
        g_cwc_rx = value;
}

/* Own thread, every 10 ms. */
void *CW_Carrier_RX_thread(void *arg) {
    uint32_t hold_until = 0;
    int we_stopped = 0;
    (void)arg;

    while (G_all_threads_run) {
        uint32_t now = now_ms();
        int quiet = g_cwc_rx && G_tx_mode && G_mode != 'C' && G_Monitor == 0;
        int want;

        if (quiet)
            hold_until = now + RX_RESTART_MS;
        want = !quiet && (int32_t)(now - hold_until) >= 0;

        pthread_mutex_lock(&g_stream_lock);
        if (stream != NULL) {
            int active = (Pa_IsStreamActive(stream) == 1);
            if (!want && active) {
                (void)Pa_StopStream(stream);
                we_stopped = 1;
                print_time();
                fprintf(G_fp_logfile, "[%d] cw-carrier. receive stream stopped (transmit, mode %c)\n",
                        line_number++, G_mode);
            } else if (want && !active && we_stopped) {
                PaError e = Pa_StartStream(stream);
                we_stopped = 0;
                print_time();
                fprintf(G_fp_logfile, "[%d] cw-carrier. receive stream started%s%s\n", line_number++,
                        e == paNoError ? "" : " FAILED: ", e == paNoError ? "" : Pa_GetErrorText(e));
            }
        } else {
            we_stopped = 0;     /* manage_stream closed it; it opens its own next one */
        }
        pthread_mutex_unlock(&g_stream_lock);
        Sleep(10);
    }
    return NULL;
}
