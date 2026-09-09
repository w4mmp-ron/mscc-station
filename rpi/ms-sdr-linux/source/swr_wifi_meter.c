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
#define SWR_WIFI_RX_LOG_MS          5000  /* periodic "got data" log while online */
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

static void swr_wifi_zero_gui(void)
{
    G_swr_wifi_fwd = 0.0;
    G_swr_wifi_ref = 0.0;
    G_swr_wifi_swr = 1.0;
    swr_wifi_send_to_gui(0.0, 0.0, 1.0);
}

/* Optional fault clear: HTTP GET http://{ip}/reset (Python UI path).
 * Also tries /api?action=reset (firmware dashboard path). Non-fatal on fail. */
static void swr_wifi_http_reset(const char *meter_ip)
{
    SOCKET s;
    struct sockaddr_in addr;
    char req[256];
    char resp[256];
    int n;
    const char *paths[] = { "/reset", "/api?action=reset" };
    int i;

    if (!meter_ip || !meter_ip[0] || !G_swr_wifi_http_reset)
        return;

    for (i = 0; i < 2; i++) {
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == INVALID_SOCKET)
            continue;

#if defined(__linux__) || defined(__APPLE__)
        {
            struct timeval tv;
            tv.tv_sec = SWR_WIFI_HTTP_TIMEOUT_SEC;
            tv.tv_usec = 0;
            setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
            setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv));
        }
#endif

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(80);
        if (inet_pton(AF_INET, meter_ip, &addr.sin_addr) != 1) {
            closesocket(s);
            continue;
        }

        if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
            closesocket(s);
            continue;
        }

        snprintf(req, sizeof(req),
            "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
            paths[i], meter_ip);
        n = (int)send(s, req, (int)strlen(req), 0);
        if (n > 0) {
            /* drain a little so server can finish */
            (void)recv(s, resp, sizeof(resp) - 1, 0);
            print_time(0);
            fprintf(G_fp_logfile,
                "[%d] swr_wifi_http_reset. OK %s on %s\n",
                line_number++, paths[i], meter_ip);
            closesocket(s);
            return;
        }
        closesocket(s);
    }

    print_time(0);
    fprintf(G_fp_logfile,
        "[%d] swr_wifi_http_reset. FAILED for %s\n",
        line_number++, meter_ip);
}

/* Public: clear latched fault on the meter (if IP known). */
void Swr_wifi_request_reset(void)
{
    const char *ip = G_swr_wifi_learned_ip[0] ? G_swr_wifi_learned_ip : G_swr_wifi_meter_ip;
    if (ip[0])
        swr_wifi_http_reset(ip);
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
    unsigned long long last_rx_log_ms = 0;
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

            /* Prove the pipe without a client: first good packet + every 5 s */
            if (first_good
                || last_rx_log_ms == 0
                || (now_ms - last_rx_log_ms) >= (unsigned long long)SWR_WIFI_RX_LOG_MS) {
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
                last_rx_log_ms = now_ms;
            }

            /* Fault edge: message only (protection stays on the meter) */
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
            prev_fault = fault;

            /* Throttle GUI traffic; always allow first after silence */
            if (last_send_ms == 0
                || (now_ms - last_send_ms) >= (unsigned long long)SWR_WIFI_SEND_MIN_MS) {
                swr_wifi_send_to_gui(fwd, ref, swr);
                last_send_ms = now_ms;
            }
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
