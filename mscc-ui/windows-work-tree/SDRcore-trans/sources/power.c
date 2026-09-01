#include "extern.h"


//These routines manage the Transceiver User and Calibration values (power.ini and power_cal.ini)
power_stack G_Proficio_Calibration_Levels[12];
power_levels G_power_levels;
extern struct input_devices G_input_devices[MAX_INPUT_DEVICES];
//const char *homedir;

int Init_Proficio_calibration(uint8_t send_to_transceiver) {
    int status = 0;
    FILE *Power_initialize;
    char l_path[PATH_MAX] = {0};
    char iq_init_record[132];
    int mynumber;
    int record = 0;
    int power = 0;
    const char* homedir;

    struct {
        char *record_number;
        char *band;
        char *power_value;
    } power_record;

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        //strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/power_cal.ini");
        Power_initialize = fopen(l_path, "r");
        if (Power_initialize != NULL) {
            fgets(iq_init_record, sizeof (iq_init_record), Power_initialize); //Skip the VERSION line
            while (fgets(iq_init_record, sizeof (iq_init_record), Power_initialize) != NULL) {
                power_record.record_number = strstr(iq_init_record, "RECORD=");
                power_record.band = strstr(iq_init_record, "BAND=");
                power_record.power_value = strstr(iq_init_record, "POWER_LEVEL=");
                mynumber = atoi((power_record.record_number + 7));
                G_Proficio_Calibration_Levels[record].record = mynumber;
                mynumber = atoi((power_record.band + 5));
                G_Proficio_Calibration_Levels[record].band = mynumber;
                mynumber = atoi((power_record.power_value + 12));
                G_Proficio_Calibration_Levels[record].power_level = mynumber;
                power = mynumber;
                //print_time();
                //fprintf(G_fp_logfile, "[%d] Init_Proficio_calibration. Record: %d, Band: %d,"
                //        "Power Level: %d\n",
                 //       line_number++, G_Proficio_Calibration_Levels[record].record, G_Proficio_Calibration_Levels[record].band,
                 //       G_Proficio_Calibration_Levels[record].power_level);
                record++;
            }
            fclose(Power_initialize);
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] Initialize_power_calibration. Open file failed\n", line_number++);
            status = 0;
        }
    } else {
        status = 0;
        print_time();
        fprintf(G_fp_logfile, "[%d] Initialize_power_calibration.  getenv failed\n", line_number++);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Initialize_power_calibration. Finished\n", line_number++);
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
            fprintf(fp_Power_ini, "USB_POWER=%d,LSB_POWER=%d,AM_POWER=%d,CW_POWER=%d,TUNE_POWER=%d;\n", G_power_levels.usb_power,
                    G_power_levels.lsb_power, G_power_levels.am_power, G_power_levels.cw_power, G_power_levels.tune_power);
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
    print_time();
    fprintf(G_fp_logfile, "[%d] build_power_levels. Finished.\n", line_number++);
}

