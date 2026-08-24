/*
 * MSCC remote operator mic — Pi receiver (MSA1 UDP).
 * Protocol matches Windows MsccRemotePhones TX (default port 9101).
 *
 * INI ENABLED=1 → Phones (P) uses this path; ENABLED=0 → local operator mic.
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
static int g_enabled;
static int g_port = 9101;

static int g_sock = -1;
static pthread_t g_thread;
static volatile int g_run;

static float g_ring[RING_FRAMES];
static volatile unsigned g_w;
static volatile unsigned g_r;

static volatile uint64_t g_last_pkt_ms;
static unsigned g_pkt_ok;
static unsigned g_pkt_bad;

static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

static void ring_write_one(float s)
{
    unsigned w = g_w;
    unsigned next = (w + 1u) % RING_FRAMES;
    if (next == g_r)
        return; /* overrun — drop */
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

static void load_config(void)
{
    char path[512];
    FILE *fp;
    char line[256];

    g_enabled = 0;
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
                "[%d] remote_mic: no %s (disabled)\n", line_number++, path);
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
        if (strcmp(k, "ENABLED") == 0 || strcmp(k, "enabled") == 0)
            g_enabled = (atoi(v) != 0);
        else if (strcmp(k, "PORT") == 0 || strcmp(k, "port") == 0)
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
            const int16_t *pcm = (const int16_t *)(buf + MSA1_HEADER_SIZE);
            for (i = 0; i < frames; i++) {
                int16_t s = (ch >= 2) ? pcm[i * 2u] : pcm[i];
                float f = (float)s / 32768.0f;
                ring_write_one(f);
            }
        }
        g_last_pkt_ms = now_ms();
        g_pkt_ok++;
        if (G_fp_logfile && (g_pkt_ok == 1u || (g_pkt_ok % 500u) == 0u)) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] remote_mic: pkt ok=%u bad=%u rate=%u ch=%u frames=%u from %s\n",
                line_number++, g_pkt_ok, g_pkt_bad, rate, ch, frames,
                inet_ntoa(from.sin_addr));
            fflush(G_fp_logfile);
        }
    }
    return NULL;
}

void remote_mic_init(void)
{
    struct sockaddr_in addr;
    int yes = 1;

    g_w = g_r = 0;
    g_sock = -1;
    g_run = 0;
    g_last_pkt_ms = 0;
    g_pkt_ok = g_pkt_bad = 0;

    load_config();
    if (!g_enabled)
        return;

    g_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sock < 0) {
        if (G_fp_logfile) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] remote_mic: socket failed: %s\n",
                line_number++, strerror(errno));
        }
        g_enabled = 0;
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
        g_enabled = 0;
        return;
    }

    g_run = 1;
    if (pthread_create(&g_thread, NULL, receiver_thread, NULL) != 0) {
        close(g_sock);
        g_sock = -1;
        g_run = 0;
        g_enabled = 0;
        if (G_fp_logfile) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] remote_mic: thread create failed\n", line_number++);
        }
        return;
    }

    if (G_fp_logfile) {
        print_time();
        fprintf(G_fp_logfile,
            "[%d] remote_mic: ENABLED listen UDP :%d (MSA1 → operator mic when active)\n",
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
    if (g_enabled) {
        pthread_join(g_thread, NULL);
        g_enabled = 0;
    }
}

int remote_mic_enabled(void)
{
    return g_enabled && g_run;
}

void remote_mic_fill_stereo_96k(float *stereo_interleaved, unsigned frames)
{
    unsigned i = 0;
    if (!stereo_interleaved || frames == 0)
        return;

    /* Ring @ 48 kHz mono → 96 kHz stereo (each sample held for two frames). */
    while (i < frames) {
        int ok = 0;
        float s = ring_read_one(&ok);
        if (!ok)
            s = 0.0f;
        stereo_interleaved[i * 2u] = s;
        stereo_interleaved[i * 2u + 1u] = s;
        i++;
        if (i < frames) {
            stereo_interleaved[i * 2u] = s;
            stereo_interleaved[i * 2u + 1u] = s;
            i++;
        }
    }
}
