#include "extern.h"
#include "version.h"

//const char *homedir;
uint8_t G_Allow_Log_Write = TRUE;

void print_time(void) {
    time_t tim;
    struct tm *now;

    if (G_Allow_Log_Write == TRUE) {
        tim = time(NULL);
        now = gmtime(&tim);
        fprintf(G_fp_logfile, "[%02d:%d:%02d:%02d]", now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
    }
}

int Open_log_file() {
    char file_name[PATH_MAX] = {0};
    int lenght = 0;
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
            strcat(file_name, "/sdrcore-trans.log");
            printf("[%d] Open_log_file. User File Name: %s\n", line_number++, file_name);
            fclose(log_file_dir);
        }

        G_fp_logfile = fopen(file_name, "w");
        if (G_fp_logfile == NULL) {
            printf("[%d] Open_log_file. File Open Failed: %s\n", line_number++, file_name);
            return 0;
        }
        printf("[%d] Open_log_file. Finished\n", line_number++);
        fprintf(G_fp_logfile, "[%d] Logfile Opened.  Logfile: %s\n", line_number++, file_name);
    }
    return 1;
}

void Reset_Logfile() {
    time_t tim;
    struct tm *local_time;
    static uint8_t log_reset = FALSE;

    tim = time(NULL);
    local_time = localtime(&tim);
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