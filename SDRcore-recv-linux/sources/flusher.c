#include "extern.h"

#define MAX_KEEP_ALIVE_STARTUP_COUNT 15
#define MAX_KEEP_ALIVE_RUN_COUNT 8

/*
 * Keep-alive (clear rules):
 *   - ms-sdr → this core:  CMD_SET_KEEP_ALIVE (0xF4)  — bumps G_keep_alive here
 *   - this core → ms-sdr:  CMD_SET_KEEP_ALIVE (0xF4) + KEEP_ALIVE_FROM_RECV
 *   Version opcodes are NOT used as keep-alive.
 */

void *Flusher_thread(void *t) {
    long long previous_keep_alive = 0;
    uint8_t count = 0;
    uint8_t count_limit = MAX_KEEP_ALIVE_STARTUP_COUNT;
    char send_buf[20];
    int l_slen = 0;
    int ka_src = KEEP_ALIVE_FROM_RECV;
    unsigned int version = VERSION_MS_SDRCORE_RECV;
    static unsigned ka_send_log = 0;

    (void)t;

    while (G_all_threads_run) {
        Sleep(3000);
        fflush(G_fp_logfile);

        /* Watchdog: ms-sdr must send us 0xF4 */
        if (count++ > count_limit) {
            if (G_keep_alive <= previous_keep_alive) {
                print_time();
                fprintf(G_fp_logfile,
                    "[%d] Keep Alive FAILED — no CMD_SET_KEEP_ALIVE (0xF4) from ms-sdr; sdrcore-recv stopping\n",
                    line_number++);
                fflush(G_fp_logfile);
                MessageBoxA(NULL, "MS-SDR has stopped sending KeepAlive Messages. sdrcore-recv will now terminate",
                        "sdrcore-recv", MB_OK | MB_ICONEXCLAMATION);
                G_all_threads_run = 0;
            } else {
                previous_keep_alive = G_keep_alive;
                count_limit = MAX_KEEP_ALIVE_RUN_COUNT;
            }
            count = 0;
        }

        /* Heartbeat to ms-sdr: real keep-alive opcode + source id */
        l_slen = (int)sizeof(si_ms_sdr);
        memset(send_buf, 0, sizeof(send_buf));
        send_buf[0] = CMD_SET_KEEP_ALIVE;
        memcpy(&send_buf[1], &ka_src, 4);
        sendto(ms_sdr_s, send_buf, 5, 0, (struct sockaddr *)&si_ms_sdr, (socklen_t)l_slen);

        /* Optional version packet (not used for keep-alive counters on ms-sdr) */
        memset(send_buf, 0, sizeof(send_buf));
        send_buf[0] = CMD_GET_SET_SDRCORE_RECV_VERSION;
        memcpy(&send_buf[1], &version, 4);
        sendto(ms_sdr_s, send_buf, 5, 0, (struct sockaddr *)&si_ms_sdr, (socklen_t)l_slen);

        if ((ka_send_log++ % 10) == 0) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] Flusher_thread. Sent CMD_SET_KEEP_ALIVE (0xF4) source=RECV(%d) to ms-sdr\n",
                line_number++, ka_src);
            fflush(G_fp_logfile);
        }
    }
    pthread_exit(0);
    return (NULL);
}
