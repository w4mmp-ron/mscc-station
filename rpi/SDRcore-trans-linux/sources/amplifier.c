#include "extern.h"
#define VERSION 1

int amplifier_calibration_PCB_0[] = {-99, -99, -99, -99, -99, -99, -99, -99, -99, -99, -99, -99};

amplifier_stack G_amp_calibration_stack[12];
//const char *homedir;

int Create_amplifier_calibration_file(int force) {
    FILE *Amp_Calibration_create = NULL;
    char l_path[PATH_MAX] = {0};
    int lenght = 0;
    int band = 0;
    int status = 0;
    const char *homedir;

    print_time();
    fprintf(G_fp_logfile, "[%d] Create_amplifier_calibration_file. Called.  force:%d \n", line_number++, force);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        //strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/amplifier_cal.ini");
        if (force == FALSE) {
            Amp_Calibration_create = fopen(l_path, "r");
            if (Amp_Calibration_create == NULL) {
                print_time();
                fprintf(G_fp_logfile, "[%d] Create_amplifier_calibration_file. amplifier_cal.ini not found. Creating amplifier_cal.ini \n",
                        line_number++);
                Amp_Calibration_create = fopen(l_path, "w");
                if (Amp_Calibration_create != NULL) {
                    for (band = 0; band < 12; band++) {
                        fprintf(Amp_Calibration_create, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", band, band,
                                amplifier_calibration_PCB_0[band]);
                    }
                    fclose(Amp_Calibration_create);
                    status = 1;
                } else {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] Create_amplifier_calibration_file. Creation of amplifier_cal.ini file Failed\n",
                            line_number++);
                    status = 0;
                }
            } else {
                print_time();
                fprintf(G_fp_logfile, "[%d] Create_amplifier_calibration_file. File exists. Not creating amplifier_cal.ini file\n",
                        line_number++);
                status = 1;
            }
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] Create_amplifier_calibration_file. Force:%d. Creating amplifier_cal.ini\n",
                    line_number++, force);
            Amp_Calibration_create = fopen(l_path, "w");
            if (Amp_Calibration_create != NULL) {
                for (band = 0; band < 12; band++) {
                    fprintf(Amp_Calibration_create, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", band, band,
                            amplifier_calibration_PCB_0[band]);
                    print_time();
                    fprintf(G_fp_logfile, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", band, band,
                            amplifier_calibration_PCB_0[band]);
                }
                fclose(Amp_Calibration_create);
                status = 1;
            } else {
                print_time();
                fprintf(G_fp_logfile, "[%d] Create_amplifier_calibration_file. Creation of amplifier_cal.ini file Failed\n",
                        line_number++);
                status = 0;
            }
        }
        if(status == 1){
            Init_Power_All();
        }
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Create_amplifier_calibration_file. Finished \n", line_number++);
    return status;
}

int Init_amplifier_calibration() {
    int status = 0;

    FILE *Amplifier_initialize;
    char l_path[PATH_MAX] = {0};
    char power_init_record[132];
    int mynumber;
    int record = 0;
    const char *homedir;

    struct {
        char *record_number;
        char *band;
        char *power_value;
    } power_record;

    //status = Create_amplifier_calibration_file();
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/amplifier_cal.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] Init_amplifier_calibration. File: %s\n", line_number++, l_path);
        Amplifier_initialize = fopen(l_path, "r");
        if (Amplifier_initialize != NULL) {
            //fgets(power_init_record, sizeof(power_init_record), Power_initialize);//Skip the VERSION line
            while (fgets(power_init_record, sizeof (power_init_record), Amplifier_initialize) != NULL) {
                power_record.record_number = strstr(power_init_record, "RECORD=");
                power_record.band = strstr(power_init_record, "BAND=");
                power_record.power_value = strstr(power_init_record, "POWER_LEVEL=");
                mynumber = atoi((power_record.record_number + 7));
                record = mynumber;
                G_amp_calibration_stack[record].record = mynumber;
                mynumber = atoi((power_record.band + 5));
                G_amp_calibration_stack[record].band = mynumber;
                mynumber = atoi((power_record.power_value + 12));
                G_amp_calibration_stack[record].power_level = mynumber;
                print_time();
                fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration. record: %d, band: %d, power_level: %d\n",
                       line_number++,
                       G_amp_calibration_stack[record].record,
                       G_amp_calibration_stack[record].band,
                      G_amp_calibration_stack[record].power_level);
            }
            fclose(Amplifier_initialize);
            status = 1;
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration. Open file failed\n", line_number++);
            status = 0;
        }
    } else {
        status = 0;
        print_time();
        fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration.  SHGetKnownFolderPath failed\n", line_number++);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration. Finished\n", line_number++);
    return status;
}

/* Write G_amp_calibration_stack to amplifier_cal.ini. RECORD/BAND = table index (a missing file
 * left them 0, so every line said RECORD=0). Temp file + rename, so ms-sdr never reads half a file. */
int Update_amplifier_calibration() {
    FILE *Amplifier_initialize = NULL;
    char l_path[PATH_MAX] = {0};
    char tmp_path[PATH_MAX + 8] = {0};
    int band = 0;
    const char *homedir;

    if ((homedir = My_getenv("HOME")) == NULL) {
        return 0;
    }
    snprintf(l_path, sizeof (l_path), "%s/amplifier_cal.ini", homedir);
    snprintf(tmp_path, sizeof (tmp_path), "%s.tmp", l_path);
    Amplifier_initialize = fopen(tmp_path, "w");
    if (Amplifier_initialize == NULL) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Update_amplifier_calibration. Open file failed\n", line_number++);
        return 0;
    }
    for (band = 0; band < 12; band++) {
        fprintf(Amplifier_initialize, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n",
                band, band, G_amp_calibration_stack[band].power_level);
    }
    if (fclose(Amplifier_initialize) != 0 || rename(tmp_path, l_path) != 0) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Update_amplifier_calibration. Write/rename failed: %s\n", line_number++, strerror(errno));
        remove(tmp_path);
        return 0;
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Update_amplifier_calibration. Written\n", line_number++);
    return 1;
}
