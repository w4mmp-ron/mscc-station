#include "extern.h"

struct {
    int Major;
    int Minor;
    char year[20];
    int month;
    int day_number;
    char day[20];
    char date[20];
} Version;

void Get_Version_Month() {
    if (strstr(Version.date, "Jan") != NULL) Version.month = 1 * 100;
    if (strstr(Version.date, "Feb") != NULL) Version.month = 2 * 100;
    if (strstr(Version.date, "Mar") != NULL) Version.month = 3 * 100;
    if (strstr(Version.date, "Apr") != NULL) Version.month = 4 * 100;
    if (strstr(Version.date, "May") != NULL) Version.month = 5 * 100;
    if (strstr(Version.date, "Jun") != NULL) Version.month = 6 * 100;
    if (strstr(Version.date, "Jul") != NULL) Version.month = 7 * 100;
    if (strstr(Version.date, "Aug") != NULL) Version.month = 8 * 100;
    if (strstr(Version.date, "Sep") != NULL) Version.month = 9 * 100;
    if (strstr(Version.date, "Oct") != NULL) Version.month = 100;
    if (strstr(Version.date, "Nov") != NULL) Version.month = 110;
    if (strstr(Version.date, "Dec") != NULL) Version.month = 120;

}

void *Flusher_thread(void *t) {
    long long previous_keep_alive = 0;
    uint8_t count = 0;
    int8_t send_version = 0;
    char send_buf[20];
    int status = 0;
    uint8_t error_reported = 0;
    int l_slen = 0;
    //Set up the Version info
    strcpy(Version.date, __DATE__);
    strcpy(Version.year, &Version.date[7]);
    Version.Major = atoi(Version.year);
    Version.Major = (Version.Major - 2000) + 100;
    strncpy(Version.day, &Version.date[4], 2);
    Version.Minor = atoi(Version.day);
    Version.day_number = atoi(Version.day);
    Version.month = 0;
    Get_Version_Month();
    Version.Minor = Version.day_number + Version.month;

    while (G_all_threads_run) {
        Sleep(3000);
        fflush(G_fp_logfile);
        if (count++ > 8) {
            if (G_keep_alive <= previous_keep_alive) {
                print_time();
                fprintf(G_fp_logfile, "[%d] Keep Alive FAILED. sdrcore-recv will now stop \n", line_number++);
                MessageBoxA(NULL, "MS-SDR has stopped sending KeepAlive Messages. sdrcore-recv will now terminate",
                        "sdrcore-recv", MB_OK | MB_ICONEXCLAMATION);
                G_all_threads_run = 0;
            } else {
                previous_keep_alive = G_keep_alive;

            }
            count = 0;
        }
        send_buf[0] = CMD_GET_SET_SDRCORE_RECV_VERSION;
        l_slen = sizeof (si_ms_sdr);
        memcpy(&send_buf[1], &Version.Major, 4);
        sendto(ms_sdr_s, send_buf, 5, 0, (struct sockaddr *) &si_ms_sdr, l_slen);
        Sleep(100);
        memcpy(&send_buf[1], &Version.Minor, 4);
        sendto(ms_sdr_s, send_buf, 5, 0, (struct sockaddr *) &si_ms_sdr, l_slen);
        Sleep(100);
        //print_time();
        //fprintf(G_fp_logfile, "[%d] Flusher_thread -> version info sent. Major: %d, Minor: %d\n", line_number++,
        //        Version.Major, Version.Minor);
    }
    pthread_exit(0);
    return (NULL);
}