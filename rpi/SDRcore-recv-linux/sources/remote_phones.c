/*
 * MSCC remote operator phones — Pi sender (MSA1 UDP).
 * Protocol matches Windows MsccRemotePhones player.
 */
#include "remote_phones.h"
#include "extern.h"

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

#define MSA1_MAGIC          0x3141534Du
#define MSA1_HEADER_SIZE    16
#define MSA1_FORMAT_S16LE   0
#define REMOTE_RATE         48000
#define REMOTE_CHANNELS     1
#define FRAMES_PER_PKT      480   /* 10 ms @ 48 kHz */
#define RING_FRAMES         16384 /* @ 48 kHz mono */

/* config */
static int g_enabled;
static char g_host[128];
static int g_port = 9100;

static int g_sock = -1;
static struct sockaddr_in g_dest;
static pthread_t g_thread;
static volatile int g_run;

/* SPSC ring: writer = audio callback, reader = sender thread */
static int16_t g_ring[RING_FRAMES];
static volatile unsigned g_w;
static volatile unsigned g_r;

/* 96k → 48k: hold previous sample for average */
static float g_prev_l;
static int g_have_prev;

static unsigned g_seq;

static void ring_write_one(int16_t s)
{
    unsigned w = g_w;
    unsigned next = (w + 1u) % RING_FRAMES;
    if (next == g_r)
        return; /* drop on overrun */
    g_ring[w] = s;
    g_w = next;
}

static int ring_read(int16_t *dst, unsigned n)
{
    unsigned got = 0;
    while (got < n) {
        unsigned r = g_r;
        if (r == g_w)
            break;
        dst[got++] = g_ring[r];
        g_r = (r + 1u) % RING_FRAMES;
    }
    return (int)got;
}

static void load_config(void)
{
    char path[512];
    FILE *fp;
    char line[256];

    g_enabled = 0;
    g_host[0] = '\0';
    g_port = 9100;

    {
        const char *home = getenv("HOME");
        if (!home || !home[0])
            home = "/tmp";
        snprintf(path, sizeof(path), "%s/.local/mscc/remote-phones.ini", home);
    }

    fp = fopen(path, "r");
    if (!fp) {
        if (G_fp_logfile) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] remote_phones: no %s (disabled)\n", line_number++, path);
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
        else if (strcmp(k, "HOST") == 0 || strcmp(k, "host") == 0) {
            strncpy(g_host, v, sizeof(g_host) - 1);
            g_host[sizeof(g_host) - 1] = '\0';
        } else if (strcmp(k, "PORT") == 0 || strcmp(k, "port") == 0)
            g_port = atoi(v);
    }
    fclose(fp);

    if (g_enabled && g_host[0] == '\0') {
        g_enabled = 0;
        if (G_fp_logfile) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] remote_phones: ENABLED but HOST empty — disabled\n",
                line_number++);
        }
    }
}

static void *sender_thread(void *arg)
{
    int16_t mono[FRAMES_PER_PKT];
    uint8_t packet[MSA1_HEADER_SIZE + FRAMES_PER_PKT * 2];
    (void)arg;

    while (g_run) {
        int got = 0;
        while (got < FRAMES_PER_PKT && g_run) {
            int n = ring_read(mono + got, (unsigned)(FRAMES_PER_PKT - got));
            if (n <= 0) {
                usleep(2000);
                continue;
            }
            got += n;
        }
        if (!g_run || got < FRAMES_PER_PKT)
            continue;

        /* header MSA1 LE */
        packet[0] = (uint8_t)(MSA1_MAGIC);
        packet[1] = (uint8_t)(MSA1_MAGIC >> 8);
        packet[2] = (uint8_t)(MSA1_MAGIC >> 16);
        packet[3] = (uint8_t)(MSA1_MAGIC >> 24);
        packet[4] = (uint8_t)(g_seq);
        packet[5] = (uint8_t)(g_seq >> 8);
        packet[6] = (uint8_t)(FRAMES_PER_PKT);
        packet[7] = (uint8_t)(FRAMES_PER_PKT >> 8);
        packet[8] = REMOTE_CHANNELS;
        packet[9] = MSA1_FORMAT_S16LE;
        packet[10] = (uint8_t)(REMOTE_RATE);
        packet[11] = (uint8_t)(REMOTE_RATE >> 8);
        packet[12] = (uint8_t)(REMOTE_RATE >> 16);
        packet[13] = (uint8_t)(REMOTE_RATE >> 24);
        packet[14] = 0;
        packet[15] = 0;
        memcpy(packet + MSA1_HEADER_SIZE, mono, FRAMES_PER_PKT * 2);

        if (sendto(g_sock, packet, sizeof(packet), 0,
                (struct sockaddr *)&g_dest, sizeof(g_dest)) < 0) {
            /* rare log */
            static unsigned err_ct;
            if (G_fp_logfile && (err_ct++ % 100) == 0) {
                print_time();
                fprintf(G_fp_logfile,
                    "[%d] remote_phones: sendto failed: %s\n",
                    line_number++, strerror(errno));
            }
        }
        g_seq++;
    }
    return NULL;
}

void remote_phones_init(void)
{
    g_w = g_r = 0;
    g_have_prev = 0;
    g_seq = 0;
    g_sock = -1;
    g_run = 0;

    load_config();
    if (!g_enabled)
        return;

    memset(&g_dest, 0, sizeof(g_dest));
    g_dest.sin_family = AF_INET;
    g_dest.sin_port = htons((uint16_t)g_port);
    if (inet_pton(AF_INET, g_host, &g_dest.sin_addr) != 1) {
        if (G_fp_logfile) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] remote_phones: bad HOST '%s'\n", line_number++, g_host);
        }
        g_enabled = 0;
        return;
    }

    g_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sock < 0) {
        if (G_fp_logfile) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] remote_phones: socket failed: %s\n",
                line_number++, strerror(errno));
        }
        g_enabled = 0;
        return;
    }

    g_run = 1;
    if (pthread_create(&g_thread, NULL, sender_thread, NULL) != 0) {
        close(g_sock);
        g_sock = -1;
        g_run = 0;
        g_enabled = 0;
        if (G_fp_logfile) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] remote_phones: thread create failed\n", line_number++);
        }
        return;
    }

    if (G_fp_logfile) {
        print_time();
        fprintf(G_fp_logfile,
            "[%d] remote_phones: ENABLED → %s:%d (MSA1 48k mono)\n",
            line_number++, g_host, g_port);
        fflush(G_fp_logfile);
    }
}

void remote_phones_shutdown(void)
{
    if (!g_run && g_sock < 0)
        return;
    g_run = 0;
    if (g_sock >= 0) {
        /* wake thread blocked in short usleep; join */
    }
    if (g_enabled) {
        pthread_join(g_thread, NULL);
        g_enabled = 0;
    }
    if (g_sock >= 0) {
        close(g_sock);
        g_sock = -1;
    }
}

int remote_phones_enabled(void)
{
    return g_enabled && g_run;
}

void remote_phones_feed(const float *stereo_interleaved, unsigned frames)
{
    unsigned i;
    if (!g_enabled || !stereo_interleaved || frames == 0)
        return;

    /*
     * Input @ 96 kHz stereo float (I/Q path rate). Emit 48 kHz mono s16:
     * average pairs of left-channel samples.
     */
    for (i = 0; i < frames; i++) {
        float l = stereo_interleaved[i * 2u];
        if (!g_have_prev) {
            g_prev_l = l;
            g_have_prev = 1;
        } else {
            float avg = 0.5f * (g_prev_l + l);
            float s = avg;
            int v;
            if (s > 1.0f) s = 1.0f;
            if (s < -1.0f) s = -1.0f;
            v = (int)(s * 30000.0f);
            if (v > 32767) v = 32767;
            if (v < -32768) v = -32768;
            ring_write_one((int16_t)v);
            g_have_prev = 0;
        }
    }
}
