#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
//#include <unistd.h>
#include <fcntl.h>
//#include <unistd.h>
//#include <sys/unistd.h>

//#include <linux/limits.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
//#include <sys/time.h>
#include "extern.h"


struct timeb my_start, end;


void print_time() {
    time_t tim;
    struct tm *now;
    int mills = 0;

    tim = time(NULL);
    now = gmtime(&tim);
    //my_time = asctime(now);
    //size = strlen(my_time);
    //my_time[size - 1] = '\0';
    //strcpy(s_my_time, my_time);
    //ftime(&my_start);
    //mills = my_start.millitm;
    fprintf(G_fp_logfile, "[%02d:%d:%02d:%02d]", now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
}

int Open_log_file() {
    char file_name[MAX_PATH] = {0};
    int lenght = 0;
    FILE *log_file_dir;
    char log_file__dir_name[MAX_PATH] = {0};
    char log_file_record[MAX_PATH] = {0};
    const char *homedir;

    struct {
        char *start;
        char *end;
        int size;
    } log_file_dir_field;


    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(log_file__dir_name, homedir);
        strcat(log_file__dir_name, "/log_file_dir.ini");
        log_file_dir = fopen(log_file__dir_name, "r");
        if (log_file_dir != NULL) {
            fgets(log_file_record, sizeof (log_file_record), log_file_dir);
            log_file_dir_field.start = strstr(log_file_record, "LOGFILE_DIRECTORY");
            log_file_dir_field.start = log_file_dir_field.start + 18;
            //Now find the ending point
            log_file_dir_field.end = strstr(log_file_dir_field.start, ";");
            log_file_dir_field.size = (log_file_dir_field.end - (log_file_dir_field.start));
            strcpy(file_name, homedir);
            strncat(file_name, log_file_dir_field.start, log_file_dir_field.size);
            strcat(file_name, "/sdrcore-recv.log");
            printf("[%d] Open_log_file -> User File Name: %s\n", line_number++, file_name);
            fclose(log_file_dir);
        }

        G_fp_logfile = fopen(file_name, "w");
        if (G_fp_logfile == NULL) {
            printf("[%d] Open_log_file -> File Open Failed: %s\n", line_number++, file_name);
            return 0;
        }
        printf("[%d] Open_log_file -> Finished\n", line_number++);
        fprintf(G_fp_logfile, "[%d] Logfile Opened.  Logfile: %s\n", line_number++, file_name);
    }
    return 1;
}
