#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
#include "usbavrcmd.h"
#include "usbavrcmd.h"
//#include "SRDLL.h"
#include "extern.h"
#include "startup.h"
#include "version.h"

/* startup.ini: the startup frequency and mode (read at start, written on change). */

uint8_t G_vfo = VFO_A;
uint8_t G_current_vfo = VFO_A;

int Get_Startup(uint32_t *freq, char *mode) {
    int status = 0;
    FILE *Startup_ini = NULL;
    char file_name[PATH_MAX] = {0};
    char startup_init_record[132];

    struct {
        char *freq_ptr;
        char *mode_ptr;
        uint32_t freq;
        char mode;
    } startup_record;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Get_Startup - Called. \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(file_name, homedir);
        strcat(file_name, "/startup.ini");

        Startup_ini = fopen(file_name, "r");
        if (Startup_ini != NULL) {
            while (fgets(startup_init_record, sizeof (startup_init_record), Startup_ini) != NULL) {
                startup_record.freq_ptr = strstr(startup_init_record, "FREQUENCY=");
                if (startup_record.freq_ptr != NULL) {
                    startup_record.freq = atoi(((startup_record.freq_ptr + (sizeof ("FREQUENCY=")))) - 1);
                }
                startup_record.mode_ptr = strstr(startup_init_record, "MODE=");
                if (startup_record.mode_ptr != NULL) {
                    startup_record.mode_ptr = (startup_record.mode_ptr + (sizeof ("MODE=")) - 1);
                    startup_record.mode = *startup_record.mode_ptr;
                }
            }
            fclose(Startup_ini);
            *freq = startup_record.freq;
            *mode = startup_record.mode;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Get_Startup.  Freq: %d, mode %c \n", line_number++, *freq, *mode);
        }
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Get_Startup . Finished\n", line_number++);
    return status;
}

int Update_Startup(uint32_t freq, char mode) {
    int status = 0;
    FILE *Startup_ini = NULL;
    char file_name[PATH_MAX] = {0};

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_Startup - Called. freq: %d, mode: %c\n", line_number++, freq, mode);
    /* 3.204 (Ron 2026-11-10, as Pi 3.201): TUNE is never saved. TUN sends mode T; saved, a stop
     * or crash while TUN was on brought the next start up in TUNE. The file keeps the mode from before. */
    if (mode == MODE_TUNE_LETTER) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_Startup - TUNE: not saved (startup.ini keeps the previous mode)\n",
            line_number++);
        return status;
    }
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(file_name, homedir);
        strcat(file_name,"/startup.ini");
        Startup_ini = fopen(file_name, "w");
        if (Startup_ini != NULL) {
            fprintf(Startup_ini, "FREQUENCY=%d;\n", freq);
            fprintf(Startup_ini, "MODE=%c;\n", mode);
            fflush(Startup_ini);
            fclose(Startup_ini);
        }
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_Startup - Finished \n", line_number++);
    return status;
}
