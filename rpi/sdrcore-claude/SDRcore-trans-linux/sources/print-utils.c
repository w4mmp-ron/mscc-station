#include "extern.h"
#include "version.h"

uint8_t G_Allow_Log_Write = TRUE;

void print_time_impl(void) {
    time_t tim;
    struct tm *now;

    if (G_Allow_Log_Write == TRUE && G_fp_logfile != NULL) {
        tim = time(NULL);
        now = gmtime(&tim);
        if (now != NULL) {
            fprintf(G_fp_logfile, "[%02d:%02d:%02d:%02d]",
                now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
        }
    }
}

int Open_log_file(void) {
    char file_name[PATH_MAX] = {0};
    FILE *log_file_dir;
    char log_file__dir_name[PATH_MAX] = {0};
    char log_file_record[PATH_MAX] = {0};
    const char* homedir;

    struct {
        char *start;
        char *end;
        int size;
    } log_file_dir_field;

    printf("Open_log_file \n");
    if ((homedir = My_getenv("HOME")) == NULL || homedir[0] == '\0') {
        printf("[%d] Open_log_file. HOME not set\n", line_number++);
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
                    strcat(file_name, "/sdrcore-trans.log");
                    printf("[%d] Open_log_file. User File Name: %s\n",
                        line_number++, file_name);
                }
            }
        }
        fclose(log_file_dir);
    }

    if (file_name[0] == '\0') {
        snprintf(file_name, sizeof(file_name), "%s/sdrcore-trans.log", homedir);
    }

    G_fp_logfile = fopen(file_name, "w");
    if (G_fp_logfile == NULL) {
        printf("[%d] Open_log_file. File Open Failed: %s\n", line_number++, file_name);
        return 0;
    }
#if defined(__linux__) || defined(__APPLE__)
    setvbuf(G_fp_logfile, NULL, _IOLBF, 0);
#endif
    printf("[%d] Open_log_file. Finished: %s\n", line_number++, file_name);
    fprintf(G_fp_logfile, "[%d] Logfile Opened.  Logfile: %s\n", line_number++, file_name);
    fflush(G_fp_logfile);
    return 1;
}

void Reset_Logfile(void) {
    time_t tim;
    struct tm *local_time;
    static uint8_t log_reset = FALSE;

    tim = time(NULL);
    local_time = localtime(&tim);
    if (local_time == NULL)
        return;
    if (local_time->tm_hour == 0 && log_reset == FALSE) {
        G_Allow_Log_Write = FALSE;
        log_reset = TRUE;
        line_number = 0;
        fflush(G_fp_logfile);
        fclose(G_fp_logfile);
        Open_log_file();
        G_Allow_Log_Write = TRUE;
    }
    if (local_time->tm_hour != 0 && log_reset == TRUE) {
        log_reset = FALSE;
    }
}
