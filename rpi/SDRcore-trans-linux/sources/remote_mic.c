/*
 * MSCC remote operator mic — Pi receiver (MSA1 UDP).
 * Protocol matches Windows MsccRemotePhones TX (default port 9101).
 *
 * Client CMD_SET_AUDIO_DEVICE=2 or 3 selects this mic path.
 * INI supplies PORT only (ENABLED is ignored; kept for old files).
 * Digital (D) ignores this module.
 */
#include "remote_mic.h"
#include "extern.h"
#include "commands.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#define MSA1_MAGIC          0x3141534Du
#define MSA1_HEADER_SIZE    16
#define MSA1_FORMAT_S16LE   0
#define REMOTE_RATE         48000
#define RING_FRAMES         16384 /* @ 48 kHz mono float */
#define MAX_PKT_FRAMES      2048
/* Diagnostic only. ~100 ms between repeat EVENT lines; ~9600 fill frames @ 96 kHz. */
#define EVENT_LIMIT_MS          100ull
#define EVENT_COOLDOWN_FRAMES   9600u
/* 480-frame packs are ~10 ms. Log gaps at 5x that, not one late packet. */
#define UDP_GAP_EVENT_MS        50ull
static int g_port = 9101;
static int g_thread_started;

static int g_sock = -1;
static pthread_t g_thread;
static volatile int g_run;

static float g_ring[RING_FRAMES];
static volatile unsigned g_w;
static volatile unsigned g_r;

static volatile uint64_t g_last_pkt_ms;
static unsigned g_pkt_ok;
static unsigned g_pkt_bad;
static volatile unsigned g_under;
static volatile unsigned g_overflow;       /* drops since last summary */
static unsigned g_overflow_pending;        /* drops not yet in an EVENT */
static uint64_t g_overflow_log_ms;
static uint64_t g_udp_log_ms;
static volatile float g_fill_step = 0.5f;  /* last step chosen by fill */
static float g_step_seen = 0.5f;
static unsigned g_step_cooldown;
static unsigned g_hold_cooldown;
static int g_low_latched;
static unsigned g_low_cooldown;

static float g_hist0, g_hist1, g_frac;

static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

/* t= is UTC. print_time() is whole seconds; milliseconds live in the body.
 * Audio path is unchanged — these lines are diagnostic only. */
static void log_remote_event(const char *fmt, ...)
{
    char body[240];
    char tbuf[64];
    struct timespec ts;
    struct tm tmv;
    va_list ap;

    if (!G_fp_logfile || !fmt)
        return;
    va_start(ap, fmt);
    vsnprintf(body, sizeof(body), fmt, ap);
    va_end(ap);
    clock_gettime(CLOCK_REALTIME, &ts);
    gmtime_r(&ts.tv_sec, &tmv);
    snprintf(tbuf, sizeof(tbuf), "%02u:%02u:%02u.%03uZ",
        (unsigned)tmv.tm_hour, (unsigned)tmv.tm_min, (unsigned)tmv.tm_sec,
        (unsigned)(ts.tv_nsec / 1000000L));
    print_time();
    fprintf(G_fp_logfile,
        "[%d] remote_mic EVENT %s mono_ms=%llu t=%s\n",
        line_number++, body,
        (unsigned long long)now_ms(), tbuf);
    fflush(G_fp_logfile);
}

static void reset_event_state(void)
{
    g_overflow = 0;
    g_overflow_pending = 0;
    g_overflow_log_ms = 0;
    g_udp_log_ms = 0;
    g_fill_step = 0.5f;
    g_step_seen = 0.5f;
    g_step_cooldown = 0;
    g_hold_cooldown = 0;
    g_low_latched = 0;
    g_low_cooldown = 0;
}

static unsigned ring_level(void);

static void log_overflow_if_due(uint64_t now)
{
    unsigned dropped;

    if (g_overflow_pending == 0u)
        return;
    if (g_overflow_log_ms != 0ull && (now - g_overflow_log_ms) < EVENT_LIMIT_MS)
        return;
    dropped = g_overflow_pending;
    g_overflow_pending = 0u;
    g_overflow_log_ms = now;
    log_remote_event("overflow dropped=%u occ=%u", dropped, ring_level());
}

static void ring_write_one(float s)
{
    unsigned w = g_w;
    unsigned next = (w + 1u) % RING_FRAMES;
    if (next == g_r) {
        g_overflow++;
        g_overflow_pending++;
        return; /* overrun — drop sample, same as before */
    }
    g_ring[w] = s;
    g_w = next;
}

static float ring_read_one(int *ok)
{
    unsigned r = g_r;
    if (r == g_w) {
        *ok = 0;
        return 0.0f;
    }
    {
        float s = g_ring[r];
        g_r = (r + 1u) % RING_FRAMES;
        *ok = 1;
        return s;
    }
}

static unsigned ring_level(void)
{
    unsigned w = g_w, r = g_r;
    return (w + RING_FRAMES - r) % RING_FRAMES;
}

static void load_config(void)
{
    char path[512];
    FILE *fp;
    char line[256];

    g_port = 9101;

    {
        const char *home = getenv("HOME");
        if (!home || !home[0])
            home = "/tmp";
        snprintf(path, sizeof(path), "%s/.local/mscc/remote-mic.ini", home);
    }

    fp = fopen(path, "r");
    if (!fp) {
        if (G_fp_logfile) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] remote_mic: no %s — using default PORT %d\n",
                line_number++, path, g_port);
        }
        return;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *eq, *k, *v;
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        eq = strchr(line, '=');
        if (!eq)
            continue;
        *eq = '\0';
        k = line;
        v = eq + 1;
        while (*v == ' ' || *v == '\t')
            v++;
        {
            size_t n = strlen(v);
            while (n > 0 && (v[n - 1] == '\n' || v[n - 1] == '\r' || v[n - 1] == ' '))
                v[--n] = '\0';
        }
        /* ENABLED ignored — mode comes from CMD_SET_AUDIO_DEVICE=2 */
        if (strcmp(k, "PORT") == 0 || strcmp(k, "port") == 0)
            g_port = atoi(v);
    }
    fclose(fp);

    if (g_port <= 0 || g_port > 65535)
        g_port = 9101;
}

static int parse_msa1(const uint8_t *pkt, int len,
    uint16_t *seq, uint16_t *nframes, uint8_t *ch, uint32_t *rate)
{
    uint32_t magic;
    uint16_t frames;
    uint8_t channels, fmt;
    uint32_t sr;
    int need;

    if (len < MSA1_HEADER_SIZE)
        return -1;
    magic = (uint32_t)pkt[0] | ((uint32_t)pkt[1] << 8) |
            ((uint32_t)pkt[2] << 16) | ((uint32_t)pkt[3] << 24);
    if (magic != MSA1_MAGIC)
        return -1;
    *seq = (uint16_t)(pkt[4] | (pkt[5] << 8));
    frames = (uint16_t)(pkt[6] | (pkt[7] << 8));
    channels = pkt[8];
    fmt = pkt[9];
    sr = (uint32_t)pkt[10] | ((uint32_t)pkt[11] << 8) |
         ((uint32_t)pkt[12] << 16) | ((uint32_t)pkt[13] << 24);
    if (channels < 1 || channels > 2)
        return -1;
    if (fmt != MSA1_FORMAT_S16LE)
        return -1;
    if (frames == 0 || frames > MAX_PKT_FRAMES)
        return -1;
    if (sr < 8000)
        return -1;
    need = MSA1_HEADER_SIZE + (int)frames * (int)channels * 2;
    if (len < need)
        return -1;
    *nframes = frames;
    *ch = channels;
    *rate = sr;
    return need;
}

static void *receiver_thread(void *arg)
{
    uint8_t buf[MSA1_HEADER_SIZE + MAX_PKT_FRAMES * 2 * 2];
    (void)arg;

    while (g_run) {
        struct sockaddr_in from;
        socklen_t flen = sizeof(from);
        int n = (int)recvfrom(g_sock, buf, sizeof(buf), 0,
            (struct sockaddr *)&from, &flen);
        uint16_t seq, frames;
        uint8_t ch;
        uint32_t rate;
        int need;
        unsigned i;

        if (!g_run)
            break;
        if (n < 0) {
            if (errno == EINTR)
                continue;
            usleep(2000);
            continue;
        }
        need = parse_msa1(buf, n, &seq, &frames, &ch, &rate);
        if (need < 0) {
            g_pkt_bad++;
            continue;
        }
        (void)seq;
        /* Prefer 48 kHz; if other rate, still ingest (host should send 48k). */
        {
            uint64_t now = now_ms();
            uint64_t gap_ms = 0;
            int have_prev = 0;
            const int16_t *pcm = (const int16_t *)(buf + MSA1_HEADER_SIZE);

            if (g_last_pkt_ms != 0ull && now >= g_last_pkt_ms) {
                gap_ms = now - g_last_pkt_ms;
                have_prev = 1;
            }
            for (i = 0; i < frames; i++) {
                int16_t s = (ch >= 2) ? pcm[i * 2u] : pcm[i];
                float f = (float)s / 32768.0f;
                ring_write_one(f);
            }
            if (have_prev && gap_ms >= UDP_GAP_EVENT_MS &&
                (g_udp_log_ms == 0ull || (now - g_udp_log_ms) >= EVENT_LIMIT_MS)) {
                log_remote_event("udp_gap gap_ms=%llu occ=%u pkts=%u",
                    (unsigned long long)gap_ms, ring_level(), g_pkt_ok);
                g_udp_log_ms = now;
            }
            log_overflow_if_due(now);
            g_last_pkt_ms = now;
        }
        g_pkt_ok++;
        if (G_fp_logfile && (g_pkt_ok == 1u || (g_pkt_ok % 500u) == 0u)) {
            float peak = 0.f;
            unsigned j;
            const int16_t *pcm = (const int16_t *)(buf + MSA1_HEADER_SIZE);
            for (j = 0; j < frames; j++) {
                int16_t s = (ch >= 2) ? pcm[j * 2u] : pcm[j];
                float a = (s < 0) ? -(float)s : (float)s;
                if (a > peak) peak = a;
            }
            print_time();
            fprintf(G_fp_logfile,
                "[%d] remote_mic: pkt ok=%u bad=%u rate=%u ch=%u frames=%u peak=%.0f occ=%u under=%u overflow=%u step=%.3f mono_ms=%llu from %s\n",
                line_number++, g_pkt_ok, g_pkt_bad, rate, ch, frames, peak,
                ring_level(), g_under, g_overflow, (double)g_fill_step,
                (unsigned long long)now_ms(), inet_ntoa(from.sin_addr));
            g_under = 0;
            g_overflow = 0;
            fflush(G_fp_logfile);
        }
    }
    return NULL;
}

void remote_mic_reset_stream(void)
{
    g_w = g_r = 0;
    g_hist0 = g_hist1 = 0.0f;
    g_frac = 0.0f;
    g_under = 0;
    reset_event_state();
    if (G_fp_logfile) {
        print_time();
        fprintf(G_fp_logfile,
            "[%d] remote_mic: stream reset (REMOTE path)\n",
            line_number++);
        fflush(G_fp_logfile);
    }
}

void remote_mic_init(void)
{
    struct sockaddr_in addr;
    int yes = 1;

    g_w = g_r = 0;
    g_sock = -1;
    g_run = 0;
    g_thread_started = 0;
    g_last_pkt_ms = 0;
    g_pkt_ok = g_pkt_bad = 0;
    g_under = 0;
    reset_event_state();
    g_hist0 = g_hist1 = 0.0f;
    g_frac = 0.0f;

    load_config();

    g_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sock < 0) {
        if (G_fp_logfile) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] remote_mic: socket failed: %s\n",
                line_number++, strerror(errno));
        }
        return;
    }
    setsockopt(g_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)g_port);
    if (bind(g_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        if (G_fp_logfile) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] remote_mic: bind :%d failed: %s\n",
                line_number++, g_port, strerror(errno));
        }
        close(g_sock);
        g_sock = -1;
        return;
    }

    g_run = 1;
    if (pthread_create(&g_thread, NULL, receiver_thread, NULL) != 0) {
        close(g_sock);
        g_sock = -1;
        g_run = 0;
        if (G_fp_logfile) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] remote_mic: thread create failed\n", line_number++);
        }
        return;
    }
    g_thread_started = 1;

    if (G_fp_logfile) {
        print_time();
        fprintf(G_fp_logfile,
            "[%d] remote_mic: listen UDP :%d (use when CMD_SET_AUDIO_DEVICE=2 or 3)\n",
            line_number++, g_port);
        fflush(G_fp_logfile);
    }
}

void remote_mic_shutdown(void)
{
    if (!g_run && g_sock < 0)
        return;
    g_run = 0;
    if (g_sock >= 0) {
        shutdown(g_sock, SHUT_RDWR);
        close(g_sock);
        g_sock = -1;
    }
    if (g_thread_started) {
        pthread_join(g_thread, NULL);
        g_thread_started = 0;
    }
}

int remote_mic_ready(void)
{
    return g_thread_started && g_run;
}

void remote_mic_fill_stereo_96k(float *stereo_interleaved, unsigned frames)
{
    unsigned i;
    if (!stereo_interleaved || frames == 0)
        return;

    /*
     * 48 kHz mono ring → 96 kHz stereo. Old path held each sample twice and
     * wrote zeros on underrun — WSJT TUNE looked like FM. Linear interp +
     * hold-last; nudge consume rate so two clocks don't click.
     */
    for (i = 0; i < frames; i++) {
        unsigned occ = ring_level();
        float step = 0.5f;
        float s;
        if (occ > (RING_FRAMES * 3u) / 4u)
            step = 0.512f;
        else if (occ > (RING_FRAMES * 5u) / 8u)
            step = 0.504f;
        else if (occ < RING_FRAMES / 8u)
            step = 0.488f;
        else if (occ < RING_FRAMES / 4u)
            step = 0.496f;

        g_fill_step = step;
        if (g_step_cooldown > 0u)
            g_step_cooldown--;
        if (step != g_step_seen && g_step_cooldown == 0u) {
            g_step_seen = step;
            g_step_cooldown = EVENT_COOLDOWN_FRAMES;
            log_remote_event("adaptive step=%.3f occ=%u", (double)step, occ);
        }

        if (occ < (RING_FRAMES / 8u)) {
            if (!g_low_latched || g_low_cooldown == 0u) {
                g_low_latched = 1;
                g_low_cooldown = EVENT_COOLDOWN_FRAMES;
                log_remote_event("low_occ occ=%u under=%u step=%.3f",
                    occ, g_under, (double)step);
            } else {
                g_low_cooldown--;
            }
        } else {
            g_low_latched = 0;
        }

        if (g_hold_cooldown > 0u)
            g_hold_cooldown--;

        while (g_frac >= 1.0f) {
            int ok = 0;
            g_hist0 = g_hist1;
            g_hist1 = ring_read_one(&ok);
            if (!ok) {
                g_hist1 = g_hist0;
                g_under++;
                if (g_hold_cooldown == 0u) {
                    g_hold_cooldown = EVENT_COOLDOWN_FRAMES;
                    log_remote_event("hold_last occ=%u under=%u step=%.3f",
                        occ, g_under, (double)step);
                }
            }
            g_frac -= 1.0f;
        }
        s = g_hist0 + (g_hist1 - g_hist0) * g_frac;
        stereo_interleaved[i * 2u] = s;
        stereo_interleaved[i * 2u + 1u] = s;
        g_frac += step;
    }
}
