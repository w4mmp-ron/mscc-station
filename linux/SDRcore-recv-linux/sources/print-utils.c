#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include "extern.h"

void print_time_impl(void) {
    time_t tim;
    struct tm *now;

    tim = time(NULL);
    now = gmtime(&tim);
    if (G_fp_logfile != NULL && now != NULL) {
        fprintf(G_fp_logfile, "[%02d:%02d:%02d:%02d]",
            now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
    }
}

int Open_log_file(void) {
    char file_name[MAX_PATH] = {0};
    FILE *log_file_dir;
    char log_file__dir_name[MAX_PATH] = {0};
    char log_file_record[MAX_PATH] = {0};
    const char *homedir;

    struct {
        char *start;
        char *end;
        int size;
    } log_file_dir_field;

    if ((homedir = My_getenv("HOME")) == NULL || homedir[0] == '\0') {
        printf("[%d] Open_log_file -> HOME not set\n", line_number++);
        return 0;
    }

    strcpy(log_file__dir_name, homedir);
    strcat(log_file__dir_name, "/log_file_dir.ini");
    log_file_dir = fopen(log_file__dir_name, "r");
    if (log_file_dir != NULL) {
        if (fgets(log_file_record, sizeof(log_file_record), log_file_dir) != NULL) {
            log_file_dir_field.start = strstr(log_file_record, "LOGFILE_DIRECTORY");
            if (log_file_dir_field.start != NULL) {
                log_file_dir_field.start += 18;
                log_file_dir_field.end = strstr(log_file_dir_field.start, ";");
                if (log_file_dir_field.end != NULL) {
                    log_file_dir_field.size =
                        (int)(log_file_dir_field.end - log_file_dir_field.start);
                    strcpy(file_name, homedir);
                    strncat(file_name, log_file_dir_field.start,
                        (size_t)log_file_dir_field.size);
                    strcat(file_name, "/sdrcore-recv.log");
                    printf("[%d] Open_log_file -> User File Name: %s\n",
                        line_number++, file_name);
                }
            }
        }
        fclose(log_file_dir);
    }

    if (file_name[0] == '\0') {
        snprintf(file_name, sizeof(file_name), "%s/sdrcore-recv.log", homedir);
    }

    G_fp_logfile = fopen(file_name, "w");
    if (G_fp_logfile == NULL) {
        printf("[%d] Open_log_file -> File Open Failed: %s\n", line_number++, file_name);
        return 0;
    }
#if defined(__linux__) || defined(__APPLE__)
    setvbuf(G_fp_logfile, NULL, _IOLBF, 0);
#endif
    printf("[%d] Open_log_file -> Finished: %s\n", line_number++, file_name);
    fprintf(G_fp_logfile, "[%d] Logfile Opened.  Logfile: %s\n", line_number++, file_name);
    fflush(G_fp_logfile);
    return 1;
}
