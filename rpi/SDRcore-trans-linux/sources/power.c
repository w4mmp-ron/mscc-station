#include "extern.h"


//These routines manage the Transceiver User and Calibration values (power.ini and power_cal.ini)
//sdrcore-trans owns power_cal.ini (QRP calibration). ms-sdr only reads it to report the value to the client.
power_stack G_Proficio_Calibration_Levels[12];
power_levels G_power_levels;
extern struct input_devices G_input_devices[MAX_INPUT_DEVICES];
//const char *homedir;

volatile int G_power_cal_save_countdown = 0; /* >0: power_cal.ini save pending (see Drive_Manager) */
volatile int G_amp_cal_save_countdown = 0;   /* >0: amplifier_cal.ini save pending */
volatile int G_drive_recalc = 0;             /* 1: Drive_Manager recomputes the drive */

/* Save any pending QRP / amp cal slider value now (before a reload reads the files). */
void Flush_pending_cal_saves(void) {
    if (G_power_cal_save_countdown > 0) {
        G_power_cal_save_countdown = 0;
        Update_power_cal_file();
    }
    if (G_amp_cal_save_countdown > 0) {
        G_amp_cal_save_countdown = 0;
        Update_amplifier_calibration();
    }
}

#define POWER_CAL_VERSION 3
#define POWER_CAL_RECORDS 12

/* Factory QRP calibration, record order POWER_B160 .. POWER_B2200.
 * Same values ms-sdr used for PCB 2/4/5/6 (and any other version). */
static const int power_cal_defaults[POWER_CAL_RECORDS] = {47, 30, 24, 29, 53, 72, 28, 30, 45, 40, 40, 40};

static int Power_cal_path(char *path, size_t size) {
    const char* homedir;

    if ((homedir = My_getenv("HOME")) == NULL) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Power_cal_path. getenv failed\n", line_number++);
        return 0;
    }
    snprintf(path, size, "%s/power_cal.ini", homedir);
    return 1;
}

/* Create power_cal.ini with the factory values if it does not exist. Returns 1 if created. */
int Create_power_cal_file(void) {
    FILE *fp;
    char l_path[PATH_MAX] = {0};
    int record = 0;

    if (!Power_cal_path(l_path, sizeof (l_path))) {
        return 0;
    }
    fp = fopen(l_path, "r");
    if (fp != NULL) {
        fclose(fp);
        return 0;
    }
    fp = fopen(l_path, "w");
    if (fp == NULL) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Create_power_cal_file. Create failed: %s\n", line_number++, l_path);
        return 0;
    }
    fprintf(fp, "VERSION=%d\n", POWER_CAL_VERSION);
    for (record = 0; record < POWER_CAL_RECORDS; record++) {
        fprintf(fp, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", record, record, power_cal_defaults[record]);
    }
    fclose(fp);
    print_time();
    fprintf(G_fp_logfile, "[%d] Create_power_cal_file. Created %s with factory values\n", line_number++, l_path);
    return 1;
}

/* Write G_Proficio_Calibration_Levels to power_cal.ini (temp file + rename, so a reader never sees half a file). */
int Update_power_cal_file(void) {
    FILE *fp;
    char l_path[PATH_MAX] = {0};
    char tmp_path[PATH_MAX + 8] = {0};
    int record = 0;

    if (!Power_cal_path(l_path, sizeof (l_path))) {
        return 0;
    }
    snprintf(tmp_path, sizeof (tmp_path), "%s.tmp", l_path);
    fp = fopen(tmp_path, "w");
    if (fp == NULL) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Update_power_cal_file. Open failed: %s\n", line_number++, tmp_path);
        return 0;
    }
    fprintf(fp, "VERSION=%d\n", POWER_CAL_VERSION);
    for (record = 0; record < POWER_CAL_RECORDS; record++) {
        fprintf(fp, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", record, record,
                G_Proficio_Calibration_Levels[record].power_level);
    }
    if (fclose(fp) != 0 || rename(tmp_path, l_path) != 0) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Update_power_cal_file. Write/rename failed: %s\n", line_number++, strerror(errno));
        remove(tmp_path);
        return 0;
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Update_power_cal_file. Written\n", line_number++);
    return 1;
}

/* Load power_cal.ini. Starts from the factory values; each "RECORD=n,...,POWER_LEVEL=v" line
 * (n 0..11) overrides record n. Bad or out-of-range lines are skipped. Returns 1 if the file was read. */
int Init_Proficio_calibration(uint8_t send_to_transceiver) {
    int status = 0;
    FILE *Power_initialize;
    char l_path[PATH_MAX] = {0};
    char iq_init_record[132];
    int record = 0;
    int loaded = 0;
    char *record_number;
    char *power_value;

    (void) send_to_transceiver;
    for (record = 0; record < POWER_CAL_RECORDS; record++) {
        G_Proficio_Calibration_Levels[record].record = record;
        G_Proficio_Calibration_Levels[record].band = record;
        G_Proficio_Calibration_Levels[record].power_level = power_cal_defaults[record];
    }
    if (!Power_cal_path(l_path, sizeof (l_path))) {
        return 0;
    }
    Power_initialize = fopen(l_path, "r");
    if (Power_initialize == NULL) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Initialize_power_calibration. Open file failed. Using factory values\n", line_number++);
        return 0;
    }
    while (fgets(iq_init_record, sizeof (iq_init_record), Power_initialize) != NULL) {
        record_number = strstr(iq_init_record, "RECORD=");
        power_value = strstr(iq_init_record, "POWER_LEVEL=");
        if (record_number == NULL || power_value == NULL) {
            continue; /* VERSION line or junk */
        }
        record = atoi(record_number + 7);
        if (record < 0 || record >= POWER_CAL_RECORDS) {
            print_time();
            fprintf(G_fp_logfile, "[%d] Initialize_power_calibration. Bad record %d skipped\n", line_number++, record);
            continue;
        }
        G_Proficio_Calibration_Levels[record].power_level = atoi(power_value + 12);
        loaded++;
    }
    fclose(Power_initialize);
    status = 1;
    print_time();
    fprintf(G_fp_logfile, "[%d] Initialize_power_calibration. Finished. %d records loaded\n", line_number++, loaded);
    return status;
}

int check_for_power_ini_file() {
    FILE *fp_Power_ini;
    char l_path[PATH_MAX] = {0};
    int status = 0;
    const char* homedir;

    print_time();
    fprintf(G_fp_logfile, "[%d] check_for_power_ini_file. Called.\n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        //strcat(l_path, "/.local/share/mscc");
        //print_time();
        //fprintf(G_fp_logfile, "[%d] check_for_power_ini_file. Default Path: %s\n", line_number++, l_path);
        strcat(l_path, "/power.ini");
        //print_time();
        //fprintf(G_fp_logfile, "[%d] check_for_power_ini_file. power.ini Path: %s\n", line_number++, l_path);
        fp_Power_ini = fopen(l_path, "r");
        if (fp_Power_ini != NULL) {
            fclose(fp_Power_ini);
            print_time();
            fprintf(G_fp_logfile, "[%d] check_for_power_ini_file. File Exists \n", line_number++);
            status = 1;
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] check_for_power_ini_file. File does not Exist.  File will be created \n", line_number++);
            status = 0;
        }
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] check_for_power_ini_file. Finished.\n", line_number++);
    return status;
}

void Init_Proficio_User_power() {
    FILE *fp_Power_ini;
    char l_path[PATH_MAX] = {0};
    int lenght = 0;
    int record = 0;
    char init_record[132];
    int mynumber;
    const char* homedir;

    struct {
        char *lsb_power;
        char *usb_power;
        char *am_power;
        char *cw_power;
        char *tune_power;
        char *fm_power;
    } device_record;

    print_time();
    fprintf(G_fp_logfile, "[%d] init_power_ini. Called.\n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        //strcat(l_path, "/.local/share/mscc");
        print_time();
        fprintf(G_fp_logfile, "[%d] init_power_ini. Default Path: %s\n", line_number++, l_path);
        strcat(l_path, "/power.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] init_power_ini. power.ini Path: %s\n", line_number++, l_path);
        fp_Power_ini = fopen(l_path, "r");
        if (fp_Power_ini != NULL) {
            while (fgets(init_record, sizeof (init_record), fp_Power_ini) != NULL) {
                device_record.usb_power = strstr(init_record, "USB_POWER");
                device_record.lsb_power = strstr(init_record, "LSB_POWER");
                device_record.am_power = strstr(init_record, "AM_POWER");
                device_record.cw_power = strstr(init_record, "CW_POWER");
                device_record.tune_power = strstr(init_record, "TUNE_POWER");
                device_record.fm_power = strstr(init_record, "FM_POWER");
                mynumber = atoi((device_record.usb_power + sizeof ("USB_POWER")));
                G_power_levels.usb_power = mynumber;
                if (device_record.lsb_power != NULL) {
                    mynumber = atoi((device_record.lsb_power + sizeof ("LSB_POWER")));
                    G_power_levels.lsb_power = mynumber;
                }
                mynumber = atoi((device_record.am_power + sizeof ("AM_POWER")));
                G_power_levels.am_power = mynumber;
                mynumber = atoi((device_record.cw_power + sizeof ("CW_POWER")));
                G_power_levels.cw_power = mynumber;
                mynumber = atoi((device_record.tune_power + sizeof ("TUNE_POWER")));
                G_power_levels.tune_power = mynumber;
                if (device_record.fm_power != NULL) {
                    mynumber = atoi((device_record.fm_power + sizeof ("FM_POWER")));
                    G_power_levels.fm_power = mynumber;
                } else {
                    G_power_levels.fm_power = 50;
                }
                //print_time();
                //fprintf(G_fp_logfile, "[%d] init_power_ini. USB_POWER=%d,LSB_POWER=%d,AM_POWER=%d,CW_POWER=%d,TUNE_POWER=%d\n", line_number++,
                //G_power_levels.usb_power, G_power_levels.lsb_power, G_power_levels.am_power, G_power_levels.cw_power,
                //G_power_levels.tune_power);
            }
            fclose(fp_Power_ini);
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] init_power_ini. Open file failed\n", line_number++);
        }
        print_time();
        fprintf(G_fp_logfile, "[%d] init_power_ini. Finished\n", line_number++);
    }
}

int Update_Proficio_User_Power_ini() {
    FILE *fp_Power_ini;
    char l_path[PATH_MAX] = {0};
    const char* homedir;

    print_time();
    fprintf(G_fp_logfile, "[%d] Update_Proficio_User_Power_ini. Called.\n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        //strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/power.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] Update_Proficio_User_Power_ini.  Path: %s\n", line_number++, l_path);
        fp_Power_ini = fopen(l_path, "w");
        if (fp_Power_ini != NULL) {
            fprintf(fp_Power_ini,
                    "USB_POWER=%d,LSB_POWER=%d,AM_POWER=%d,CW_POWER=%d,TUNE_POWER=%d,FM_POWER=%d;\n",
                    G_power_levels.usb_power, G_power_levels.lsb_power, G_power_levels.am_power,
                    G_power_levels.cw_power, G_power_levels.tune_power, G_power_levels.fm_power);
            fclose(fp_Power_ini);
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] Update_Proficio_User_Power_ini. Open file failed\n", line_number++);
            return 0;
        }
        print_time();
        fprintf(G_fp_logfile, "[%d] Update_Proficio_User_Power_ini. Finished\n", line_number++);
    }
    return 1;
}

void set_selected_power(int power_field, int power_value) {
    print_time();
    fprintf(G_fp_logfile, "[%d] set_selected_power. Called with power_field: %d, power_value: %d\n", line_number++,
            power_field, power_value);
    switch (power_field) {
        case AM_POWER:
            G_power_levels.am_power = power_value;
            break;
        case CW_POWER:
            G_power_levels.cw_power = power_value;
            break;
        case USB_POWER:
            G_power_levels.usb_power = power_value;
            break;
        case LSB_POWER:
            G_power_levels.lsb_power = power_value;
            break;
        case TUNE_POWER:
            G_power_levels.tune_power = power_value;
            break;
        case FM_POWER:
            G_power_levels.fm_power = power_value;
            break;
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] set_selected_power. Finished\n", line_number++);
}

void build_power_levels(void) {
    print_time();
    fprintf(G_fp_logfile, "[%d] build_power_levels. Called.\n", line_number++);
    G_power_levels.am_power = 50;
    G_power_levels.cw_power = 50;
    G_power_levels.tune_power = 50;
    G_power_levels.usb_power = 50;
    G_power_levels.lsb_power = 50;
    G_power_levels.fm_power = 50;
    print_time();
    fprintf(G_fp_logfile, "[%d] build_power_levels. Finished.\n", line_number++);
}

