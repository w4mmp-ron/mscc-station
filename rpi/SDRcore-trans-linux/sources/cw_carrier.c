/*
 * cw-carrier (trans 3.190, 2026-11-20): the radio (Black Pill 3.90+) plays a stored 1 ms CW
 * carrier block, so trans need not stream to the radio while receiving or in CW.
 *
 * ms-sdr turns this on with CMD_SET_CW_CARRIER (0x15) value 1, only when mscc.ini has
 * CW_CARRIER=1 and the radio answers 0x15. Off (the default) = trans works as before.
 *
 * - Capture: in steady CW, process_mic_to_iq() puts each output frame in a 96-frame ring
 *   (cw_carrier_capture). 12 kHz at 96 kHz repeats every 8 frames, so in steady state
 *   ring[k] is the same at any time: no lock needed. It is the real output (after the
 *   filter and the TX IQ correction), so the radio plays exactly what trans would send.
 * - Send: after 200 ms of steady CW since the last change, the ring goes to ms-sdr as
 *   [0x15][384 bytes int16 I/Q]; ms-sdr passes it to the radio and answers 0x15 value 2
 *   once the radio has it. Any other command from ms-sdr while in CW marks the block stale
 *   (power, band, IQ cal, filter...): trans runs its stream again and sends a new block.
 * - Stream: trans's I/Q output stream runs only when needed: voice / digital / tune transmit
 *   plus the null-modulated flush after it (and at least a 150 ms tail), CW until the radio
 *   has a fresh block. Otherwise it is stopped
 *   (Pa_StopStream / Pa_StartStream, same opened stream). All stream starts / stops, here
 *   and in manage_stream(), hold g_stream_lock.
 * - Remote audio: the I/Q callback is what empties the network mic ring. While the stream is
 *   stopped this thread empties it at the real rate (so it never overflows), and resets it
 *   just before a start, so a transmission never begins with old mic audio (3.191).
 */
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include "extern.h"
#include "commands.h"
#include "cw_carrier.h"
#include "remote_mic.h"

extern PaStream *stream;
extern int ms_sdr_s;
extern struct sockaddr_in si_ms_sdr;
extern socklen_t slen;

pthread_mutex_t g_stream_lock = PTHREAD_MUTEX_INITIALIZER;

#define CWC_FRAMES      96u          /* 1 ms at 96 kHz = 12 cycles of 12 kHz */
#define CWC_SETTLE      19200u       /* 200 ms of steady CW output before a capture */
#define CWC_TAIL_MS     150u         /* stream keeps running after transmit ends */
#define CWC_RESEND_MS   2000u        /* no answer from ms-sdr: send the block again */

static volatile int g_cwc_enabled = 0;     /* ms-sdr said: radio has the feature */
static volatile int g_cwc_confirmed = 0;   /* radio has the current block */
static volatile int g_cwc_stale = 1;       /* block needs (re)capture */
static volatile uint32_t g_cwc_frames = 0; /* steady CW frames captured (callback) */
static volatile uint32_t g_cwc_frames_at_stale = 0;
static float g_cwc_ring[CWC_FRAMES * 2u];
static int16_t g_cwc_last_sent[CWC_FRAMES * 2u];
static uint32_t g_cwc_sent_ms = 0;
static uint32_t g_tx_off_ms = 0;
static int g_prev_tx = 0;

static int remote_audio(void) {
    return (G_audio_mode == REMOTE_AUDIO || G_audio_mode == REMOTE_DIGITAL_AUDIO) && remote_mic_ready();
}

/* Stream stopped: throw away the mic audio that arrived since the last call (96 frames/ms). */
static void drain_remote_mic(uint32_t now) {
    static float scratch[4096 * 2];
    static uint32_t last = 0;
    uint32_t frames = (last == 0u) ? 0u : (now - last) * 96u;

    last = now;
    while (frames > 0u) {
        unsigned n = frames > 4096u ? 4096u : (unsigned)frames;
        remote_mic_fill_stereo_96k(scratch, n);
        frames -= n;
    }
}

static uint32_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

int cw_carrier_enabled(void) {
    return g_cwc_enabled;
}

/* Audio callback: out = interleaved I/Q floats as sent to the radio. */
void cw_carrier_capture(const float *out, unsigned long frames) {
    static uint32_t pos = 0;
    unsigned long i;

    if (!g_cwc_enabled || mystate.opmode != MODE_CW) {
        return;
    }
    for (i = 0; i < frames; i++) {
        g_cwc_ring[pos * 2u] = out[i * 2u];
        g_cwc_ring[pos * 2u + 1u] = out[i * 2u + 1u];
        if (++pos >= CWC_FRAMES)
            pos = 0;
    }
    g_cwc_frames += (uint32_t)frames;
}

/* UDP thread: every command from ms-sdr except keep-alive / TX on-off / 0x15 / the WiFi SWR
 * meter's forward power (3.192: ms-sdr passes each meter reading on, about once a second;
 * trans only stores it, it never changes the carrier). */
void cw_carrier_command_seen(uint8_t opcode) {
    if (!g_cwc_enabled)
        return;
    if (opcode == CMD_SET_KEEP_ALIVE || opcode == CMD_SET_TX_ON || opcode == CMD_SET_CW_CARRIER ||
        opcode == CMD_SET_FOWARD_POWER_VALUE)
        return;
    g_cwc_frames_at_stale = g_cwc_frames;
    g_cwc_stale = 1;
    g_cwc_confirmed = 0;
}

/* UDP thread: CMD_SET_CW_CARRIER from ms-sdr. 0 = off, 1 = on, 2 = radio has the block. */
void cw_carrier_set(uint8_t value) {
    print_time();
    fprintf(G_fp_logfile, "[%d] cw-carrier. CMD_SET_CW_CARRIER %d\n", line_number++, value);
    if (value == 0u) {
        g_cwc_enabled = 0;
        g_cwc_confirmed = 0;
        g_cwc_stale = 1;
    } else if (value == 1u) {
        g_cwc_frames_at_stale = g_cwc_frames;
        g_cwc_stale = 1;
        g_cwc_confirmed = 0;
        g_cwc_enabled = 1;
    } else if (value == 2u) {
        if (!g_cwc_stale)
            g_cwc_confirmed = 1;
    }
}

static void send_block(void) {
    char buf[1u + CWC_FRAMES * 4u];
    int16_t blk[CWC_FRAMES * 2u];
    unsigned i;

    for (i = 0; i < CWC_FRAMES * 2u; i++) {
        float v = g_cwc_ring[i] * 32767.0f;
        if (v > 32767.0f) v = 32767.0f;
        if (v < -32768.0f) v = -32768.0f;
        blk[i] = (int16_t)lrintf(v);
    }
    buf[0] = (char)CMD_SET_CW_CARRIER;
    memcpy(&buf[1], blk, sizeof(blk));
    if (sendto(ms_sdr_s, buf, (int)sizeof(buf), 0, (struct sockaddr *)&si_ms_sdr, slen) == SOCKET_ERROR) {
        print_time();
        fprintf(G_fp_logfile, "[%d] cw-carrier. send to ms-sdr FAILED: %s\n", line_number++, strerror(errno));
        return;
    }
    if (memcmp(blk, g_cwc_last_sent, sizeof(blk)) != 0) {
        print_time();
        fprintf(G_fp_logfile, "[%d] cw-carrier. block sent to ms-sdr: I[0..3] %d %d %d %d  Q[0..3] %d %d %d %d\n",
                line_number++, blk[0], blk[2], blk[4], blk[6], blk[1], blk[3], blk[5], blk[7]);
        memcpy(g_cwc_last_sent, blk, sizeof(blk));
    }
    g_cwc_sent_ms = now_ms();
}

static int want_stream(uint32_t now) {
    if (mystate.opmode == MODE_CW)
        return !g_cwc_confirmed;
    if (mystate.opmode == MODE_TUNE)
        return 1;
    if (G_tx_mode)
        return 1;
    if (!trans_null_flush_done())
        return 1;
    return (now - g_tx_off_ms) < CWC_TAIL_MS;
}

/* Own thread, every 10 ms. */
void *CW_Carrier_thread(void *arg) {
    int was_active = -1;
    (void)arg;

    while (G_all_threads_run) {
        uint32_t now = now_ms();

        if (G_tx_mode != g_prev_tx) {
            if (!G_tx_mode)
                g_tx_off_ms = now;
            g_prev_tx = G_tx_mode;
        }

        if (g_cwc_enabled) {
            /* CW: capture once steady, send, wait for ms-sdr's answer (resend if none). */
            if (mystate.opmode == MODE_CW && !g_cwc_confirmed &&
                (uint32_t)(g_cwc_frames - g_cwc_frames_at_stale) >= CWC_SETTLE &&
                (g_cwc_stale || (now - g_cwc_sent_ms) >= CWC_RESEND_MS)) {
                g_cwc_stale = 0;
                send_block();
            }

            pthread_mutex_lock(&g_stream_lock);
            if (stream != NULL) {
                int active = (Pa_IsStreamActive(stream) == 1);
                int want = want_stream(now);
                if (want && !active) {
                    PaError e;
                    if (remote_audio())
                        remote_mic_reset_stream();   /* no old mic audio at the start */
                    e = Pa_StartStream(stream);
                    if (mystate.opmode == MODE_CW)
                        g_cwc_frames_at_stale = g_cwc_frames;   /* settle again */
                    active = (e == paNoError);
                    if (e != paNoError) {
                        print_time();
                        fprintf(G_fp_logfile, "[%d] cw-carrier. Pa_StartStream FAILED: %s\n",
                                line_number++, Pa_GetErrorText(e));
                    }
                } else if (!want && active) {
                    (void)Pa_StopStream(stream);
                    active = 0;
                }
                if (!active && remote_audio())
                    drain_remote_mic(now);
                if (active != was_active) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] cw-carrier. I/Q stream %s (mode %d, tx %d)\n",
                            line_number++, active ? "running" : "stopped", mystate.opmode, G_tx_mode);
                    was_active = active;
                }
            }
            pthread_mutex_unlock(&g_stream_lock);
        } else if (was_active == 0) {
            /* turned off: make sure the stream runs, as before */
            pthread_mutex_lock(&g_stream_lock);
            if (stream != NULL && Pa_IsStreamActive(stream) != 1)
                (void)Pa_StartStream(stream);
            pthread_mutex_unlock(&g_stream_lock);
            was_active = -1;
        }
        Sleep(10);
    }
    return NULL;
}
