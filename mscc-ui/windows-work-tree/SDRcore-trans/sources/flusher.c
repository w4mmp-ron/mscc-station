#include "extern.h"
#include "commands.h"
#include "version.h"

// {Model}.{[M]M}{Release Number}  (the bytes are reversed)  
// Model 0-OSB, 1-Proficio, 2-Geminus,3-MKII-PTT,4-MKII-ATU,5-Proficio-PTT,6-Proficio-ATU
// Month is the number of months from the starting point of 09/24. 01/25 is 13, 02/25 is 14, etc.
// Release number is in the range of 0 to 9


extern state mystate;

/*struct {
    int Major;
    int Minor;
    char year[20];
    int month;
    int day_number;
    char day[20];
    char date[20];
} Version*/

/*void Get_Version_Month() {
    if (strstr(Version.date, "Jan") != NULL) Version.month = 1 * 100;
    if (strstr(Version.date, "Feb") != NULL) Version.month = 2 * 100;
    if (strstr(Version.date, "Mar") != NULL) Version.month = 3 * 100;
    if (strstr(Version.date, "Apr") != NULL) Version.month = 4 * 100;
    if (strstr(Version.date, "May") != NULL) Version.month = 5 * 100;
    if (strstr(Version.date, "Jun") != NULL) Version.month = 6 * 100;
    if (strstr(Version.date, "Jul") != NULL) Version.month = 7 * 100;
    if (strstr(Version.date, "Aug") != NULL) Version.month = 8 * 100;
    if (strstr(Version.date, "Sep") != NULL) Version.month = 9 * 100;
    if (strstr(Version.date, "Oct") != NULL) Version.month = 110;
    if (strstr(Version.date, "Nov") != NULL) Version.month = 120;
    if (strstr(Version.date, "Dec") != NULL) Version.month = 130;
}*/

void *Flusher_thread(void *t) {
    long long previous_keep_alive = 0;
    static uint8_t count = 0;
    char buffer[10];
    int l_slen = 0;
    unsigned int version = VERSION_MS_SDRCORE_TRANS;
    //long long reset_logfile = 0;
    //int file_number;

    Sleep(5000); //Wait for the subsystems to initialize
    /*strcpy(Version.date, __DATE__);
    strcpy(Version.year, &Version.date[7]);
    Version.Major = atoi(Version.year);
    Version.Major = (Version.Major - 2000) + 100;
    strncpy(Version.day, &Version.date[4], 2);
    Version.Minor = atoi(Version.day);
    Version.day_number = atoi(Version.day);
    Version.month = 0;
    Get_Version_Month();
    Version.Minor = Version.day_number + Version.month;
    */

    while (G_all_threads_run) {
        //Reset_Logfile();
        if (G_Allow_Log_Write == TRUE) {
            Sleep(3000);
            fflush(G_fp_logfile);
            if (count++ > MAX_KEEP_ALIVE_COUNT) {
                if (G_keep_alive <= previous_keep_alive) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] Flusher_thread. Keep Alive failed \n", line_number++);
                    print_time();
                    fprintf(G_fp_logfile, "[%d] Flusher_thread. sdrcore_trans will now stop \n", line_number++);
                    MessageBoxA(NULL, "Keep Alive failed.  sdrcore_trans will terminate", "SDRCore-Trans", MB_OK | MB_ICONSTOP);
                    G_all_threads_run = 0;
                } else {
                    previous_keep_alive = G_keep_alive;
                }
                count = 0;
            }
            buffer[0] = CMD_GET_SET_SDRCORE_TRANS_VERSION;
            l_slen = sizeof (si_ms_sdr);
            if (G_network_initialized) {
                memcpy(&buffer[1], &version, 4);
                sendto(ms_sdr_s, buffer, 5, 0, (struct sockaddr*)&si_ms_sdr, l_slen);
                //print_time();
                //fprintf(G_fp_logfile, "[%d] Flusher_thread. version info sent. Major: %d, Minor: %d\n", line_number++,
                //        Version.Major, Version.Minor);
            }
        }
    }
    pthread_exit(0);
    return (NULL);
}