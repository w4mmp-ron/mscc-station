/*
 * WiFi SWR meter ingest for ms-sdr
 *
 * Meter publishes UDP JSON (port 6999 by default). This thread listens near
 * the radio/Pi, parses fwd/ref/swr/fault, and drives the same GUI path as the
 * I2C Mensuro meter (extended opcodes 0x0B / 0x0C / 0x0D).
 *
 * Display / telemetry only — do not use WiFi SWR for hard TX foldback.
 * See swr-meter/SWR-METER-MSCC-SUMMARY.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>

#include "extern.h"
#include "usbavrcmd.h"

#define SWR_WIFI_DEFAULT_PORT       6999
#define SWR_WIFI_DEFAULT_TIMEOUT_MS 3000
/* Keep GUI meter updates slow — each triple was flooding WPF (KA timeout). */
#define SWR_WIFI_SEND_MIN_MS        500
#define SWR_WIFI_RECV_BUF           1024
#define SWR_WIFI_HTTP_TIMEOUT_SEC   2

/* Config (mscc.ini) — defaults: enabled, port 6999, auto-learn meter IP */
uint8_t G_swr_wifi_enable = 1;
uint16_t G_swr_wifi_port = SWR_WIFI_DEFAULT_PORT;
char G_swr_wifi_meter_ip[64] = {0};   /* empty = accept any source */
int G_swr_wifi_timeout_ms = SWR_WIFI_DEFAULT_TIMEOUT_MS;
uint8_t G_swr_wifi_http_reset = 1;    /* HTTP GET /reset when fault clears */
/* Push F/R/SWR opcodes to GUI (0 = log-only ingest; safer until client ready) */
uint8_t G_swr_wifi_to_gui = 0;

/* Live state (for logging / future GUI use) */
double G_swr_wifi_fwd = 0.0;
double G_swr_wifi_ref = 0.0;
double G_swr_wifi_swr = 1.0;
double G_swr_wifi_v = 0.0;
int G_swr_wifi_fault = 0;
uint8_t G_swr_wifi_online = 0;
char G_swr_wifi_learned_ip[64] = {0};

static pthread_t G_swr_wifi_thread;
static int G_swr_wifi_thread_rc = -1;
static int G_swr_wifi_sock = -1;

/* ---- minimal JSON number extractors (no libjson dependency) ---- */

static const char *json_find_key(const char *json, const char *key)
{
    char pat[48];
    const char *p;
    size_t klen;

    if (!json || !key)
        return NULL;
    klen = strlen(key);
    if (klen + 3 >= sizeof(pat))
        return NULL;
    pat[0] = '"';
    memcpy(pat + 1, key, klen);
    pat[klen + 1] = '"';
    pat[klen + 2] = '\0';

    p = strstr(json, pat);
    if (!p)
        return NULL;
    p += klen + 2;
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
        p++;
    if (*p != ':')
        return NULL;
    p++;
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
        p++;
    return p;
}

static int json_get_double(const char *json, const char *key, double *out)
{
    const char *p = json_find_key(json, key);
    char *end = NULL;
    double v;

    if (!p || !out)
        return 0;
    v = strtod(p, &end);
    if (end == p)
        return 0;
    *out = v;
    return 1;
}

static int json_get_int(const char *json, const char *key, int *out)
{
    const char *p = json_find_key(json, key);
    char *end = NULL;
    long v;

    if (!p || !out)
        return 0;
    v = strtol(p, &end, 10);
    if (end == p)
        return 0;
    *out = (int)v;
    return 1;
}

/* Same packing as meter.c Send_data() */
static uint32_t pack_power_milli(double watts)
{
    uint32_t frac_part;
    uint32_t int_part;
    float decimal_part_float;
    unsigned int decimal_part_int;

    if (watts < 0.0)
        watts = 0.0;
    if (watts > 999999.0)
        watts = 999999.0;

    frac_part = (uint32_t)watts;
    decimal_part_float = (float)(watts - (double)frac_part);
    decimal_part_float *= 1000.0f;
    int_part = frac_part * 1000u;
    decimal_part_int = (unsigned int)decimal_part_float;
    return int_part + (uint32_t)decimal_part_int;
}

static void swr_wifi_send_to_gui(double fwd, double ref, double swr)
{
    uint8_t swr_send;
    uint32_t forward_send;
    uint32_t reverse_send;
    byte extended_data[16];

    if (!G_swr_wifi_to_gui)
        return;
    if (!(G_Remote_GUI_Attached == TRUE && G_MSCC_Initialized == TRUE))
        return;

    if (swr < 1.0)
        swr = 1.0;
    if (swr > 9.9)
        swr = 9.9;

    swr_send = (uint8_t)(swr * 10.0 + 0.5);
    memcpy(&extended_data[0], &swr_send, sizeof(swr_send));
    Gui_send_param_extended(CMD_SET_SWR, extended_data, (int)sizeof(swr_send));

    forward_send = pack_power_milli(fwd);
    memcpy(&extended_data[0], &forward_send, sizeof(forward_send));
    Gui_send_param_extended(CMD_SET_FORWARD_POWER, extended_data, (int)sizeof(forward_send));
    /* Optional core power path — skip when GUI off; keep light when on */
    SDRcore_trans_send_param(CMD_SET_FOWARD_POWER_VALUE, (int)forward_send);

    reverse_send = pack_power_milli(ref);
    memcpy(&extended_data[0], &reverse_send, sizeof(reverse_send));
    Gui_send_param_extended(CMD_SET_REVERSE_POWER, extended_data, (int)sizeof(reverse_send));
}

/* 3.188: meter fault / online state to the client (extended sub CMD_SET_SWR_FAULT).
 * Not gated by SWR_METER_TO_GUI: a fault is a safety state, and an old client simply
 * ignores the unknown sub. The meter IP lets the client send its HTTP fault reset. */
static void swr_wifi_send_fault(int fault, int online)
{
    byte data[6];
    struct in_addr a;
    const char *ip = G_swr_wifi_learned_ip[0] ? G_swr_wifi_learned_ip : G_swr_wifi_meter_ip;

    if (!(G_Remote_GUI_Attached == TRUE && G_MSCC_Initialized == TRUE))
        return;

    data[0] = (byte)(fault ? 1 : 0);
    data[1] = (byte)(online ? 1 : 0);
    memset(&data[2], 0, 4);
    if (ip[0] && inet_pton(AF_INET, ip, &a) == 1)
        memcpy(&data[2], &a.s_addr, 4);   /* network order: a.b.c.d as sent */
    Gui_send_param_extended(CMD_SET_SWR_FAULT, data, (int)sizeof(data));
}

static void swr_wifi_zero_gui(void)
{
    G_swr_wifi_fwd = 0.0;
    G_swr_wifi_ref = 0.0;
    G_swr_wifi_swr = 1.0;
    swr_wifi_send_to_gui(0.0, 0.0, 1.0);
    swr_wifi_send_fault(0, 0);
}

/* 3.190: one HTTP GET to the meter's web server (port 80). Returns the number of answer bytes
 * read into resp (NUL-terminated), or -1 when there is no connection. timeout_ms caps the
 * connect, the send and each read (Linux applies SO_SNDTIMEO to connect too). */
static int swr_wifi_http_get(const char *meter_ip, const char *path, int timeout_ms,
                             char *resp, int resp_size)
{
    SOCKET s;
    struct sockaddr_in addr;
    char req[256];
    int n, got;

    if (resp_size < 1)
        return -1;
    resp[0] = '\0';
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET)
        return -1;

#if defined(__linux__) || defined(__APPLE__)
    {
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv));
    }
#endif

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    if (inet_pton(AF_INET, meter_ip, &addr.sin_addr) != 1 ||
        connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        closesocket(s);
        return -1;
    }

    snprintf(req, sizeof(req),
        "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
        path, meter_ip);
    if ((int)send(s, req, (int)strlen(req), 0) <= 0) {
        closesocket(s);
        return -1;
    }

    /* Read the whole answer (status line + JSON); the meter closes the connection */
    got = 0;
    while (got < resp_size - 1 &&
           (n = (int)recv(s, resp + got, resp_size - 1 - got, 0)) > 0)
        got += n;
    resp[got] = '\0';
    closesocket(s);
    return got;
}

/* Fault clear: HTTP GET http://{ip}/api?action=reset (the meter firmware's only reset route).
 * 3.190: was "/reset" first, then "/api?action=reset". The firmware (HF_SWR_R4_1) has no /reset
 * page, so it answered 404 and ms-sdr took any answer as success: the second path was never
 * tried and the meter was never reset. Now one path, and success is read from the meter's
 * answer: it returns its status JSON, and "fault":0 means the fault is clear.
 * Note: the meter only resets with no RF (FWD < 0.1 W); during TX it answers "fault":1.
 * Non-fatal on fail (logged). */
static void swr_wifi_http_reset(const char *meter_ip)
{
    char resp[1024];
    const char *path = "/api?action=reset";
    int got;

    if (!meter_ip || !meter_ip[0] || !G_swr_wifi_http_reset)
        return;

    got = swr_wifi_http_get(meter_ip, path, SWR_WIFI_HTTP_TIMEOUT_SEC * 1000, resp, (int)sizeof(resp));
    print_time(0);
    if (got < 0) {
        fprintf(G_fp_logfile, "[%d] swr_wifi_http_reset. FAILED for %s (no connection)\n",
            line_number++, meter_ip);
    } else if (strstr(resp, " 200") != NULL && strstr(resp, "\"fault\":0") != NULL) {
        fprintf(G_fp_logfile, "[%d] swr_wifi_http_reset. OK %s on %s: fault clear\n",
            line_number++, path, meter_ip);
    } else if (strstr(resp, "\"fault\":1") != NULL) {
        fprintf(G_fp_logfile, "[%d] swr_wifi_http_reset. %s on %s: meter still faulted "
            "(it resets only with no RF)\n", line_number++, path, meter_ip);
    } else {
        fprintf(G_fp_logfile, "[%d] swr_wifi_http_reset. %s on %s: unexpected answer (%d bytes): %.60s\n",
            line_number++, path, meter_ip, got, resp);
    }
}

/* 3.190: TUNE flag to the meter (Ron 2026-11-01). The meter's "Tune Allowance" (our firmware
 * swr-meter/ron/HF_SWR_R4_1) ignores high SWR while the tuner searches, but only while this flag
 * is set; every other transmission keeps the fast trip. The meter trips after ~58 ms of high SWR
 * (Trip Count 24 x ~2.4 ms), so tune=1 must arrive BEFORE RF:
 *   on = 1: sent and the answer awaited here, BEFORE Tune_button keys the radio, at most
 *           SWR_WIFI_TUNE_TIMEOUT_MS (normally a few ms on the LAN).
 *   on = 0: sent after unkeying, in a short detached thread (nothing waits for it).
 * Skipped when the meter is off, offline or its IP is unknown. A meter with Stew's firmware
 * ignores the "tune" argument (it just answers its status): harmless. If the flag never
 * arrives, the tune simply gets no allowance (may trip as before); protection is never lost.
 * The meter also clears the flag itself after the allowance time. */
#define SWR_WIFI_TUNE_TIMEOUT_MS 300

static const char *swr_wifi_ip(void)
{
    return G_swr_wifi_learned_ip[0] ? G_swr_wifi_learned_ip : G_swr_wifi_meter_ip;
}

static void swr_wifi_send_tune(int on, int timeout_ms)
{
    char resp[512];
    char ip[64];
    int got;
    const char *path = on ? "/api?tune=1" : "/api?tune=0";

    snprintf(ip, sizeof(ip), "%s", swr_wifi_ip());
    if (!ip[0])
        return;
    got = swr_wifi_http_get(ip, path, timeout_ms, resp, (int)sizeof(resp));
    print_time(0);
    if (got < 0)
        fprintf(G_fp_logfile, "[%d] Swr_wifi_tune. %s to %s FAILED (no connection, no allowance)\n",
            line_number++, path, ip);
    else if (strstr(resp, on ? "\"tune\":1" : "\"tune\":0") != NULL)
        fprintf(G_fp_logfile, "[%d] Swr_wifi_tune. %s to %s OK\n", line_number++, path, ip);
    else
        fprintf(G_fp_logfile, "[%d] Swr_wifi_tune. %s to %s: meter has no tune flag (old firmware?)\n",
            line_number++, path, ip);
}

static void *swr_wifi_tune_off_thread(void *arg)
{
    (void)arg;
    swr_wifi_send_tune(0, SWR_WIFI_HTTP_TIMEOUT_SEC * 1000);
    return NULL;
}

void Swr_wifi_tune(int on)
{
    pthread_t t;

    if (!G_swr_wifi_enable || !G_swr_wifi_online || !swr_wifi_ip()[0])
        return;
    if (on) {
        swr_wifi_send_tune(1, SWR_WIFI_TUNE_TIMEOUT_MS);
    } else if (pthread_create(&t, NULL, swr_wifi_tune_off_thread, NULL) == 0) {
        pthread_detach(t);
    }
}

/* Public: clear latched fault on the meter (if IP known). */
void Swr_wifi_request_reset(void)
{
    const char *ip = G_swr_wifi_learned_ip[0] ? G_swr_wifi_learned_ip : G_swr_wifi_meter_ip;
    if (ip[0])
        swr_wifi_http_reset(ip);
}

/* ms-sdr 3.189: client RESET button through ms-sdr (CMD_SWR_METER_RESET 0x2E, data ignored).
 * The client's own HTTP /reset only works on the meter's network; a remote client (internet,
 * VPN, other subnet) can't reach a meter on the Pi's network, so it asks ms-sdr. The HTTP call
 * can block up to ~8 s (2 paths x 2 s connect + 2 s read) when the meter is gone, so it runs in
 * its own detached thread, not in the client UDP thread. An explicit RESET is sent even with
 * HTTP_RESET=0 (that setting is for the automatic reset). One at a time: a press while one is
 * running is logged and dropped. Result is in the log only; the meter's next update (0x0F)
 * tells the client whether the fault cleared. */
static volatile int swr_wifi_reset_busy = 0;

static void *swr_wifi_reset_thread(void *arg)
{
    char ip[64];
    uint8_t saved = G_swr_wifi_http_reset;

    (void)arg;
    snprintf(ip, sizeof(ip), "%s",
        G_swr_wifi_learned_ip[0] ? G_swr_wifi_learned_ip : G_swr_wifi_meter_ip);
    if (ip[0]) {
        G_swr_wifi_http_reset = 1;      /* explicit request: not gated by the auto setting */
        swr_wifi_http_reset(ip);
        G_swr_wifi_http_reset = saved;
    } else {
        print_time(0);
        fprintf(G_fp_logfile,
            "[%d] Swr_wifi_client_reset. no meter IP known (no update received, METER_IP unset)\n",
            line_number++);
    }
    swr_wifi_reset_busy = 0;
    return NULL;
}

void Swr_wifi_client_reset(void)
{
    pthread_t t;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Swr_wifi_client_reset. CMD_SWR_METER_RESET from client (meter %s)\n",
        line_number++,
        G_swr_wifi_learned_ip[0] ? G_swr_wifi_learned_ip :
        (G_swr_wifi_meter_ip[0] ? G_swr_wifi_meter_ip : "unknown"));
    if (swr_wifi_reset_busy) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Swr_wifi_client_reset. reset already running, ignored\n",
            line_number++);
        return;
    }
    swr_wifi_reset_busy = 1;
    if (pthread_create(&t, NULL, swr_wifi_reset_thread, NULL) != 0) {
        swr_wifi_reset_busy = 0;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Swr_wifi_client_reset. pthread_create FAILED\n", line_number++);
        return;
    }
    pthread_detach(t);
}

static int swr_wifi_bind(uint16_t port)
{
    struct sockaddr_in addr;
    int yes = 1;

    G_swr_wifi_sock = (int)socket(AF_INET, SOCK_DGRAM, 0);
    if (G_swr_wifi_sock < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Swr_wifi. socket FAILED: %s\n",
            line_number++, strerror(errno));
        return -1;
    }

    if (setsockopt(G_swr_wifi_sock, SOL_SOCKET, SO_REUSEADDR,
            (const char *)&yes, sizeof(yes)) < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Swr_wifi. SO_REUSEADDR warn: %s\n",
            line_number++, strerror(errno));
    }

#if defined(__linux__) || defined(__APPLE__)
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000; /* 200 ms poll so thread can exit cleanly */
        setsockopt(G_swr_wifi_sock, SOL_SOCKET, SO_RCVTIMEO,
            (const char *)&tv, sizeof(tv));
    }
#endif

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(G_swr_wifi_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Swr_wifi. bind port %u FAILED: %s\n",
            line_number++, (unsigned)port, strerror(errno));
        closesocket(G_swr_wifi_sock);
        G_swr_wifi_sock = -1;
        return -1;
    }

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Swr_wifi. listening UDP 0.0.0.0:%u\n",
        line_number++, (unsigned)port);
    return 0;
}

void *Swr_wifi_meter_main(void *param)
{
    Set_Thread_Name("swr-meter");
    char buf[SWR_WIFI_RECV_BUF];
    struct sockaddr_in src;
    socklen_t src_len;
    int n;
    double fwd = 0.0, ref = 0.0, swr = 1.0, v = 0.0;
    int fault = 0;
    int prev_fault = 0;
    int have_data = 0;
    int zero_sent = 0;
    int first_good = 1;
    int first_bad = 1;
    unsigned long long pkt_count = 0;
    unsigned long long pkt_since_log = 0;
    unsigned long long last_pkt_ms = 0;
    unsigned long long last_send_ms = 0;
    unsigned long long now_ms;
    char src_ip[64];
    char message[256];
    struct timespec ts;

    (void)param;

    print_time(0);
    fprintf(G_fp_logfile,
        "[%d] Swr_wifi_meter_main STARTED. enable=%u port=%u filter_ip=%s timeout_ms=%d to_gui=%u\n",
        line_number++,
        (unsigned)G_swr_wifi_enable,
        (unsigned)G_swr_wifi_port,
        G_swr_wifi_meter_ip[0] ? G_swr_wifi_meter_ip : "(any)",
        G_swr_wifi_timeout_ms,
        (unsigned)G_swr_wifi_to_gui);

    if (swr_wifi_bind(G_swr_wifi_port) != 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Swr_wifi_meter_main. bind failed — thread exit\n",
            line_number++);
        pthread_exit(0);
        return NULL;
    }

    while (G_all_threads_run) {
        src_len = sizeof(src);
        n = (int)recvfrom(G_swr_wifi_sock, buf, sizeof(buf) - 1, 0,
            (struct sockaddr *)&src, &src_len);

        clock_gettime(CLOCK_MONOTONIC, &ts);
        now_ms = (unsigned long long)ts.tv_sec * 1000ull
            + (unsigned long long)ts.tv_nsec / 1000000ull;

        if (n > 0) {
            buf[n] = '\0';
            if (!inet_ntop(AF_INET, &src.sin_addr, src_ip, sizeof(src_ip)))
                src_ip[0] = '\0';

            /* Optional fixed IP filter from mscc.ini */
            if (G_swr_wifi_meter_ip[0] && src_ip[0]
                && strcmp(G_swr_wifi_meter_ip, src_ip) != 0) {
                continue;
            }

            if (src_ip[0]) {
                if (strcmp(G_swr_wifi_learned_ip, src_ip) != 0) {
                    strncpy(G_swr_wifi_learned_ip, src_ip, sizeof(G_swr_wifi_learned_ip) - 1);
                    G_swr_wifi_learned_ip[sizeof(G_swr_wifi_learned_ip) - 1] = '\0';
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Swr_wifi. meter source IP: %s\n",
                        line_number++, G_swr_wifi_learned_ip);
                }
            }

            /* Require at least one power/SWR field */
            have_data = 0;
            if (json_get_double(buf, "fwd", &fwd))
                have_data = 1;
            if (json_get_double(buf, "ref", &ref))
                have_data = 1;
            if (json_get_double(buf, "swr", &swr))
                have_data = 1;
            (void)json_get_double(buf, "v", &v);
            if (!json_get_int(buf, "fault", &fault))
                fault = 0;

            if (!have_data) {
                /* Something arrived but not our JSON — log once + sample */
                if (first_bad) {
                    print_time(0);
                    fprintf(G_fp_logfile,
                        "[%d] Swr_wifi. UDP from %s (%d bytes) but no fwd/ref/swr keys. sample: %.80s\n",
                        line_number++, src_ip[0] ? src_ip : "?", n, buf);
                    first_bad = 0;
                }
                continue;
            }

            if (fwd < 0.0) fwd = 0.0;
            if (ref < 0.0) ref = 0.0;
            if (swr < 1.0) swr = 1.0;
            if (swr > 25.0) swr = 25.0;

            G_swr_wifi_fwd = fwd;
            G_swr_wifi_ref = ref;
            G_swr_wifi_swr = swr;
            G_swr_wifi_v = v;
            G_swr_wifi_fault = fault;
            G_swr_wifi_online = 1;
            last_pkt_ms = now_ms;
            zero_sent = 0;
            pkt_count++;
            pkt_since_log++;

            /* Prove the pipe without a client: first good packet only (SD card wear) */
            if (first_good) {
                print_time(0);
                fprintf(G_fp_logfile,
                    "[%d] Swr_wifi. RX from %s  fwd=%.2f ref=%.2f swr=%.2f fault=%d  "
                    "pkts=%llu (+%llu)  gui=%s\n",
                    line_number++,
                    src_ip[0] ? src_ip : "?",
                    fwd, ref, swr, fault,
                    (unsigned long long)pkt_count,
                    (unsigned long long)pkt_since_log,
                    (G_Remote_GUI_Attached == TRUE && G_MSCC_Initialized == TRUE)
                        ? "ready" : "not-ready");
                first_good = 0;
                pkt_since_log = 0;
            }

            /* Fault edge: the meter protects the radio; ms-sdr tells the client at once
             * (3.188: CMD_SET_SWR_FAULT, so the client can force RX and offer RESET). */
            if (fault && !prev_fault) {
                print_time(0);
                fprintf(G_fp_logfile,
                    "[%d] Swr_wifi. FAULT latched. fwd=%.2f ref=%.2f swr=%.2f\n",
                    line_number++, fwd, ref, swr);
                snprintf(message, sizeof(message),
                    "SWR METER FAULT\nSWR=%.2f  FWD=%.1fW  REF=%.1fW",
                    swr, fwd, ref);
                Gui_Add_Message(message);
            }
            /* Falling edge: optional HTTP reset already done by user/app;
             * we only log recovery. */
            if (!fault && prev_fault) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Swr_wifi. fault cleared\n", line_number++);
            }

            /* Throttle GUI traffic; always allow first after silence. A fault change
             * goes out at once, with the readings that caused it. */
            if (last_send_ms == 0 || fault != prev_fault
                || (now_ms - last_send_ms) >= (unsigned long long)SWR_WIFI_SEND_MIN_MS) {
                swr_wifi_send_to_gui(fwd, ref, swr);
                swr_wifi_send_fault(fault, 1);
                last_send_ms = now_ms;
            }
            prev_fault = fault;
        }

        /* Offline timeout → zero meters once */
        if (G_swr_wifi_online && last_pkt_ms != 0
            && (now_ms - last_pkt_ms) > (unsigned long long)G_swr_wifi_timeout_ms) {
            print_time(0);
            fprintf(G_fp_logfile,
                "[%d] Swr_wifi. meter offline (timeout %d ms)\n",
                line_number++, G_swr_wifi_timeout_ms);
            G_swr_wifi_online = 0;
            G_swr_wifi_fault = 0;
            prev_fault = 0;
            if (!zero_sent) {
                swr_wifi_zero_gui();
                zero_sent = 1;
            }
        }
    }

    if (G_swr_wifi_sock >= 0) {
        closesocket(G_swr_wifi_sock);
        G_swr_wifi_sock = -1;
    }

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Swr_wifi_meter_main NORMAL EXIT\n", line_number++);
    pthread_exit(0);
    return NULL;
}

int Start_swr_wifi_meter_thread(void)
{
    int status = 1;
    long t = 0;

    if (!G_swr_wifi_enable) {
        print_time(0);
        fprintf(G_fp_logfile,
            "[%d] Start_swr_wifi_meter_thread SKIPPED (SWR_METER=0)\n",
            line_number++);
        return 1;
    }

    print_time(0);
    fprintf(G_fp_logfile, "[%d] main. Starting Swr_wifi_meter thread\n", line_number++);
    G_swr_wifi_thread_rc = pthread_create(&G_swr_wifi_thread, NULL,
        Swr_wifi_meter_main, (void *)t);
    if (G_swr_wifi_thread_rc) {
        print_time(0);
        fprintf(G_fp_logfile,
            "[%d] main. Swr_wifi_meter thread failed rc=%d\n",
            line_number++, G_swr_wifi_thread_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile,
            "[%d] main. Swr_wifi_meter thread Started\n", line_number++);
    }
    return status;
}

void Parse_swr_wifi_record(const char *record)
{
    const char *parameter;
    int length;
    int cmd_value;
    char *field_start;
    char *field_end;
    size_t ncopy;

    if (!record)
        return;

    parameter = strstr(record, "SWR_METER=");
    if (parameter != NULL) {
        length = (int)strlen("SWR_METER=");
        cmd_value = atoi(&parameter[length]);
        G_swr_wifi_enable = (cmd_value != 0) ? 1 : 0;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Parse_mscc_record. SWR_METER: %d\n",
            line_number++, (int)G_swr_wifi_enable);
    }

    parameter = strstr(record, "SWR_METER_PORT=");
    if (parameter != NULL) {
        length = (int)strlen("SWR_METER_PORT=");
        cmd_value = atoi(&parameter[length]);
        if (cmd_value > 0 && cmd_value < 65536)
            G_swr_wifi_port = (uint16_t)cmd_value;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Parse_mscc_record. SWR_METER_PORT: %u\n",
            line_number++, (unsigned)G_swr_wifi_port);
    }

    parameter = strstr(record, "SWR_METER_IP=");
    if (parameter != NULL) {
        field_start = strstr(record, "SWR_METER_IP=");
        field_end = strstr(field_start, ";");
        G_swr_wifi_meter_ip[0] = '\0';
        if (field_end && field_end > field_start + 13) {
            ncopy = (size_t)(field_end - (field_start + 13));
            if (ncopy >= sizeof(G_swr_wifi_meter_ip))
                ncopy = sizeof(G_swr_wifi_meter_ip) - 1;
            memcpy(G_swr_wifi_meter_ip, field_start + 13, ncopy);
            G_swr_wifi_meter_ip[ncopy] = '\0';
            /* trim trailing spaces */
            while (ncopy > 0 && isspace((unsigned char)G_swr_wifi_meter_ip[ncopy - 1])) {
                G_swr_wifi_meter_ip[--ncopy] = '\0';
            }
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Parse_mscc_record. SWR_METER_IP: %s\n",
            line_number++,
            G_swr_wifi_meter_ip[0] ? G_swr_wifi_meter_ip : "(any)");
    }

    parameter = strstr(record, "SWR_METER_TIMEOUT=");
    if (parameter != NULL) {
        length = (int)strlen("SWR_METER_TIMEOUT=");
        cmd_value = atoi(&parameter[length]);
        if (cmd_value >= 500 && cmd_value <= 60000)
            G_swr_wifi_timeout_ms = cmd_value;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Parse_mscc_record. SWR_METER_TIMEOUT: %d\n",
            line_number++, G_swr_wifi_timeout_ms);
    }

    parameter = strstr(record, "SWR_METER_HTTP_RESET=");
    if (parameter != NULL) {
        length = (int)strlen("SWR_METER_HTTP_RESET=");
        cmd_value = atoi(&parameter[length]);
        G_swr_wifi_http_reset = (cmd_value != 0) ? 1 : 0;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Parse_mscc_record. SWR_METER_HTTP_RESET: %d\n",
            line_number++, (int)G_swr_wifi_http_reset);
    }

    /* SWR_METER_TO_GUI=1 → push 0x0B/0x0C/0x0D to client (default 0: ingest/log only) */
    parameter = strstr(record, "SWR_METER_TO_GUI=");
    if (parameter != NULL) {
        length = (int)strlen("SWR_METER_TO_GUI=");
        cmd_value = atoi(&parameter[length]);
        G_swr_wifi_to_gui = (cmd_value != 0) ? 1 : 0;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Parse_mscc_record. SWR_METER_TO_GUI: %d\n",
            line_number++, (int)G_swr_wifi_to_gui);
    }
}
