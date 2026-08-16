#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>

#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
#include "usbavrcmd.h"
//#include "SRDLL.h"
#include "extern.h"
#include "version.h"

int Delete_amplifier_ini_file();

#define VERSION 1

int amplifier_defaults_PCB_5[] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
int amplifier_defaults_PCB_4[] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
int amplifier_defaults_PCB_C[] = {80, 35, 24, 28, 26, 35, 28, 27, 45, 33, 100, 100};
int amplifier_defaults_PCB_0[] = {80, 35, 24, 28, 26, 35, 28, 27, 45, 33, 100, 100};

typedef struct {
    int record;
    int band;
    int power_level;
} amplifier_stack;

amplifier_stack G_amp_calibration_stack[12];

struct {
    int record;
    int band;
    int power_level;
} G_amplifier_stack [12];


int G_amplifier_version = 0;
//uint8_t Amplifier_Map[] = {160, 80, 60, 40, 30, 20, 17, 15, 12, 10};
int16_t G_Amplifier_band = 0;
uint8_t G_check_bias = FALSE;

int Init_amplifier_calibration() {
    int status = 0;
    char file_name[PATH_MAX] = {0};
    FILE *Power_initialize;
    char power_init_record[132];
    int mynumber;
    int record = 0;

    struct {
        char *record_number;
        char *band;
        char *power_value;
    } power_record;

    //status = Create_amplifier_calibration_file()
    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration . Called \n", line_number++);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/amplifier_cal.ini");

        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration . amplifier_cal.ini-> Path: %s\n", line_number++, homedir);

        Power_initialize = fopen(file_name, "r");
        if (Power_initialize != NULL) {
            //fgets(power_init_record, sizeof(power_init_record), Power_initialize);//Skip the VERSION line
            while (fgets(power_init_record, sizeof (power_init_record), Power_initialize) != NULL) {
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
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration . Record: %d, Band: %d,"
                        "Power Level: %d\n", line_number++, G_amp_calibration_stack[record].record, G_amp_calibration_stack[record].band,
                        G_amp_calibration_stack[record].power_level);
            }
            fclose(Power_initialize);
            status = 1;
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration . Open file failed\n", line_number++);
            status = 0;
        }
    } else {
        status = 0;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration .  SHGetKnownFolderPath failed\n", line_number++);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration . Finished\n", line_number++);
    return status;
}

int Parse_amplifier_record(char *record, char *field) {
    int status = -1;
    char source_field[80] = {0};
    char *result_field;
    int size = 0;

    strcpy(source_field, field);
    size = strlen(source_field);

    if ((result_field = strstr(record, source_field)) != NULL) {
        status = atoi((result_field + (size + 1)));
        return status;
    }
    return status;
}

int Check_Amplifier_Version() {
    int status = -1;
    FILE *fp_User_ini;
    char file_name[PATH_MAX] = {0};
    char init_record[30];
    char field[20] = {0};
    char *record_status = NULL;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Check_Amplifier_Version . Called.\n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_Amplifier_Version . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/amplifier.ini");

        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_Amplifier_Version . amplifier.ini Path: %s\n", line_number++, homedir);
        fp_User_ini = fopen(file_name, "r");
        if (fp_User_ini != NULL) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Check_Amplifier_Version . File Open \n", line_number++);
            record_status = fgets(init_record, 30, fp_User_ini);
            if (record_status != NULL) {
                memset(field, 0, sizeof (field));
                strcpy(field, "VERSION");
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_Amplifier_Version . Calling Parse_User_Controls. field %s", line_number++, field);
                status = Parse_amplifier_record(init_record, field);
                fclose(fp_User_ini);
                if (status != VERSION) {
                    fprintf(G_fp_logfile, "[%d] Check_Amplifier_Version . VERSION not Equal version:%d . Deleting amplifier.ini\n",
                            line_number++, status);
                    Delete_amplifier_ini_file();
                }
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_Amplifier_Version . Read FAILED\n", line_number++);
            }
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Check_Amplifier_Version .  Open FAILED: %s \n", line_number++, homedir);
        }

    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_Amplifier_Version .  SHGetKnownFolderPath FAILED: %s \n", line_number++, homedir);
    }
    print_time(1);
    fprintf(G_fp_logfile, "[%d] Check_Amplifier_Version . Finished.\n", line_number++);
    return status;
}

uint8_t Band_to_amplifier_record(int16_t band) {
    int record = 0;
    switch (band) {
        case 2200:
            record = 11;
            break;
        case 630:
            record = 10;
            break;
        case 10:
            record = 9;
            break;
        case 12:
            record = 8;
            break;
        case 15:
            record = 7;
            break;
        case 17:
            record = 6;
            break;
        case 20:
            record = 5;
            break;
        case 30:
            record = 4;
            break;
        case 40:
            record = 3;
            break;
        case 60:
            record = 2;
            break;
        case 80:
            record = 1;
            break;
        case 160:
            record = 0;
            break;
    }
    return record;
}

int Delete_amplifier_ini_file() {
    int status = 0;
    FILE *Power_ini_delete;
    char file_name[PATH_MAX] = {0};
    int ret;

    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Delete_amplifier_ini_file . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/amplifier.ini");

        print_time(0);
        fprintf(G_fp_logfile, "[%d] Delete_amplifier_ini_file . Delete_power_ini_file Path: %s\n", line_number++, homedir);
        Power_ini_delete = fopen(file_name, "r");
        if (Power_ini_delete != NULL) {
            fclose(Power_ini_delete);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Delete_amplifier_ini_file . File Exists . Will now delete \n", line_number++);
            ret = remove(homedir);
            if (ret == 0) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Delete_amplifier_ini_file . The initialization file has been deleted \n", line_number++);
                status = 1;
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Delete_amplifier_ini_file . The initialization file Deletion has FAILED \n", line_number++);
                status = 0;
            }
        }
    }
    return status;
}

int Update_amplifier_ini_file() {
    FILE *Power_ini_update;
    char file_name[PATH_MAX] = {0};
    //int lenght = 0;
    int band = 0;
    int status = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_amplifier_ini_file. Called \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(file_name, homedir);
        strcat(file_name, "/amplifier.ini");
        Power_ini_update = fopen(file_name, "w");
        if (Power_ini_update != NULL) {
            fprintf(Power_ini_update, "VERSION=%d\n", VERSION);
            for (band = 0; band < 12; band++) {
                fprintf(Power_ini_update, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", G_amplifier_stack[band].record,
                        G_amplifier_stack[band].band, G_amplifier_stack[band].power_level);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Update_amplifier_ini_file . RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", line_number++,
                        G_amplifier_stack[band].record, G_amplifier_stack[band].band, G_amplifier_stack[band].power_level);
            }
            fflush(Power_ini_update);
            fclose(Power_ini_update);
            //Sleep(100);
            status = 1;
            SDRcore_trans_send_param(CMD_SET_SDRCORE_TRANS_INITIALIZE, 1);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Update_amplifier_ini_file. Finished \n", line_number++);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Update_amplifier_ini_file. Open file failed\n", line_number++);
            status = 0;
        }
    }
    return status;
}

int Create_amplifier_ini_file() {
    FILE *Power_create_ini;
    char file_name[PATH_MAX] = {0};
    //int lenght = 0;
    int band = 0;
    int status = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Create_amplifier_ini_file . Called \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(file_name, homedir);
        strcat(file_name, "/amplifier.ini");
        Power_create_ini = fopen(file_name, "r");
        if (Power_create_ini == NULL) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Create_amplifier_ini_file . amplifier.ini not found. Creating amplifier.ini \n", line_number++);
            Power_create_ini = fopen(file_name, "w");
            if (Power_create_ini != NULL) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Create_amplifier_ini_file . Creating amplifier.ini for Version: %d \n",
                        line_number++, VERSION);
                fprintf(Power_create_ini, "VERSION=%d\n", VERSION);
                for (band = 0; band < 12; band++) {
                    switch (G_pcb_version) {
                        case 5:
                            fprintf(Power_create_ini, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", band, band, amplifier_defaults_PCB_5[band]);
                            break;
                        case 4:
                            fprintf(Power_create_ini, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", band, band, amplifier_defaults_PCB_4[band]);
                            break;
                        case 2:
                            fprintf(Power_create_ini, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", band, band, amplifier_defaults_PCB_C[band]);
                            break;
                        default:
                            fprintf(Power_create_ini, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", band, band, amplifier_defaults_PCB_0[band]);
                            break;
                    }
                }
                fclose(Power_create_ini);
                status = 2;
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Create_amplifier_ini_file . Creation of amplifier.ini file Failed\n", line_number++);
                status = 0;
            }
        } else {
            if (Power_create_ini != NULL) {
                fclose(Power_create_ini);
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Create_amplifier_ini_file . File exists . Not creating amplifier.ini file\n", line_number++);
            status = 1;
        }
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Create_amplifier_ini_file . Finished \n", line_number++);
    return status;
}

int Initialize_amplifier_power() {
    int status = 0;
    char file_name[PATH_MAX] = {0};
    FILE *Power_initialize;
    char iq_init_record[132];
    int mynumber;
    int record = 0;

    struct {
        char *record_number;
        char *band;
        char *power_value;
    } power_record;

    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_amplifier_power. Called \n", line_number++);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_amplifier_power. Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/amplifier.ini");

        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_amplifier_power. filename: %s\n", line_number++, homedir);
        Power_initialize = fopen(file_name, "r");
        if (Power_initialize != NULL) {
            fgets(iq_init_record, sizeof (iq_init_record), Power_initialize); //Skip the VERSION line
            while (fgets(iq_init_record, sizeof (iq_init_record), Power_initialize) != NULL) {
                power_record.record_number = strstr(iq_init_record, "RECORD=");
                power_record.band = strstr(iq_init_record, "BAND=");
                power_record.power_value = strstr(iq_init_record, "POWER_LEVEL=");
                mynumber = atoi((power_record.record_number + 7));
                record = mynumber;
                G_amplifier_stack[record].record = mynumber;
                mynumber = atoi((power_record.band + 5));
                G_amplifier_stack[record].band = mynumber;
                mynumber = atoi((power_record.power_value + 12));
                G_amplifier_stack[record].power_level = mynumber;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Initialize_amplifier_power. Record: :%d, Band: :%d, Power Level: %d\n",
                        line_number++, G_amplifier_stack[record].record, G_amplifier_stack[record].band,
                        G_amplifier_stack[record].power_level);
            }
            fclose(Power_initialize);
            status = 1;
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Initialize_amplifier_power. Open file FAILED\n", line_number++);
            status = 0;
        }
    } else {
        status = 0;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_amplifier_power. SHGetKnownFolderPath FAILED\n", line_number++);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Initialize_amplifier_power. FINISHED\n", line_number++);
    return status;
}

int Cal_amplifier_tune_button(int on_off) {

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Cal_amplifier_tune_button . NOOP\n", line_number++);
    return on_off;
}

int Amplifier_Set_Power_Level(uint8_t command, char *buf) {
    int status = 0;
    static uint8_t calibration_value = 0;
    static int record = 0;
    uint8_t opcode_data_8_bit;
    //short int *opcode_data_short;
    //short int s_opcode_data;
    int *op_code_data_32;
    int i_opcode_data;

    opcode_data_8_bit = (uint8_t) (buf[1]);
    switch (command) {
        case CMD_GET_POTENTIA_BIAS:
            G_check_bias = opcode_data_8_bit;
            print_time(1);
            fprintf(G_fp_logfile, "[%d] CMD_GET_POTENTIA_BIAS . G_check_bias: %d\n", line_number++, G_check_bias);
            break;

        case CMD_SET_AMPLIFIER_INITIALIZE:
            op_code_data_32 = (int*) &buf[1];
            memcpy(&i_opcode_data, op_code_data_32, 4);
            G_Amplifier_band = i_opcode_data;
            record = Band_to_amplifier_record(G_Amplifier_band);
            Initialize_amplifier_power();
            Init_amplifier_calibration();
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_AMPLIFIER_INITIALIZE . Band: %d, Record: %d\n", line_number++,
                    G_Amplifier_band, record);
            Sleep(50);
            Gui_send_param(CMD_GET_AMPLIFIER_POWER, G_amplifier_stack[record].power_level);
            Gui_send_param(CMD_SET_POTENTIA_CALIBRATION, G_amp_calibration_stack[record].power_level);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_AMPLIFIER_INITIALIZE . Finished \n", line_number++);
            break;

        case CMD_SET_POTENTIA_CALIBRATION:
            op_code_data_32 = (int*) &buf[1];
            memcpy(&i_opcode_data, op_code_data_32, 4);
            print_time(1);
            fprintf(G_fp_logfile, "[%d] UDP Thread . CMD_SET_POTENTIA_CALIBRATION Calibration Value: %d\n",
                    line_number++, i_opcode_data);
            SDRcore_trans_send_param(CMD_SET_POTENTIA_CALIBRATION, i_opcode_data);
            break;

        case CMD_SET_AMPLIFIER_POWER:
            calibration_value = opcode_data_8_bit;
            print_time(1);
            fprintf(G_fp_logfile, "[%d] CMD_SET_AMPLIFIER_POWER . Received . Cal Value: %d\n", line_number++, calibration_value);
            G_amplifier_stack[record].power_level = calibration_value;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_AMPLIFIER_POWER . Calling Update_power_ini_file \n", line_number++);
            status = Update_amplifier_ini_file();
            Sleep(100);
            if (status == 1) {
                SDRcore_trans_send_param(CMD_SET_AMPLIFIER_POWER, G_amplifier_stack[record].power_level);
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_AMPLIFIER_POWER . Finished \n", line_number++);
            break;

        case CMD_GET_AMPLIFIER_POWER:
            print_time(1);
            fprintf(G_fp_logfile, "[%d] CMD_GET_AMPLIFIER_POWER . Called . NOOP", line_number++);
            break;
    }
    return status;
}