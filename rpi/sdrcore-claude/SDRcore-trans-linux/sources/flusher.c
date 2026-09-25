#include "extern.h"
#include "commands.h"
#include "version.h"

extern state mystate;

/*
 * Keep-alive (clear rules):
 *   - ms-sdr → this core:  CMD_SET_KEEP_ALIVE (0xF4)  — bumps G_keep_alive here
 *   - this core → ms-sdr:  CMD_SET_KEEP_ALIVE (0xF4) + KEEP_ALIVE_FROM_TRANS
 *   Version opcodes are NOT used as keep-alive.
 */

void *Flusher_thread(void *t) {
    long long previous_keep_alive = 0;
    static uint8_t count = 0;
    char buffer[20];
    int l_slen = 0;
    int ka_src = KEEP_ALIVE_FROM_TRANS;
    unsigned int version = VERSION_MS_SDRCORE_TRANS;
    static unsigned ka_send_log = 0;

    (void)t;
    (void)mystate;

    Sleep(2000); /* short settle; do not wait forever before KA */

    while (G_all_threads_run) {
        if (G_Allow_Log_Write == TRUE) {
            Sleep(3000);
            fflush(G_fp_logfile);

            /* Watchdog: ms-sdr must send us 0xF4 */
            if (count++ > MAX_KEEP_ALIVE_COUNT) {
                if (G_keep_alive <= previous_keep_alive) {
                    print_time();
                    fprintf(G_fp_logfile,
                        "[%d] Flusher_thread. Keep Alive failed — no CMD_SET_KEEP_ALIVE (0xF4) from ms-sdr; stopping\n",
                        line_number++);
                    fflush(G_fp_logfile);
                    MessageBoxA(NULL, "Keep Alive failed.  sdrcore_trans will terminate", "SDRCore-Trans", MB_OK | MB_ICONSTOP);
                    G_all_threads_run = 0;
                } else {
                    previous_keep_alive = G_keep_alive;
                }
                count = 0;
            }

            /* Always send KA to ms-sdr (do not gate on G_network_initialized) */
            l_slen = (int)sizeof(si_ms_sdr);
            memset(buffer, 0, sizeof(buffer));
            buffer[0] = CMD_SET_KEEP_ALIVE;
            memcpy(&buffer[1], &ka_src, 4);
            sendto(ms_sdr_s, buffer, 5, 0, (struct sockaddr *)&si_ms_sdr, (socklen_t)l_slen);

            /* Version optional — not used for keep-alive */
            memset(buffer, 0, sizeof(buffer));
            buffer[0] = CMD_GET_SET_SDRCORE_TRANS_VERSION;
            memcpy(&buffer[1], &version, 4);
            sendto(ms_sdr_s, buffer, 5, 0, (struct sockaddr *)&si_ms_sdr, (socklen_t)l_slen);

            if ((ka_send_log++ % 10) == 0) {
                print_time();
                fprintf(G_fp_logfile,
                    "[%d] Flusher_thread. Sent CMD_SET_KEEP_ALIVE (0xF4) source=TRANS(%d) to ms-sdr\n",
                    line_number++, ka_src);
                fflush(G_fp_logfile);
            }
        } else {
            Sleep(3000);
        }
    }
    pthread_exit(0);
    return (NULL);
}
