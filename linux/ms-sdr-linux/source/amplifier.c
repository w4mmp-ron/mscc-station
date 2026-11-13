#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>

#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
#include "usbavrcmd.h"
//#include "SRDLL.h"
#include "extern.h"
#include "version.h"

/* Amplifier (QRO) calibration. sdrcore-trans owns ~/amplifier_cal.ini; ms-sdr only reads it on
 * amp band select to report the value to the client. amplifier.ini (amp "user power") is gone:
 * it was never used in the drive. The client still gets 0xFB = 100 on band select, the value
 * amplifier.ini always held once a band had been selected. */

#define AMPLIFIER_POWER_REPORT 100

typedef struct {
    int record;
    int band;
    int power_level;
} amplifier_stack;

amplifier_stack G_amp_calibration_stack[12];

//uint8_t Amplifier_Map[] = {160, 80, 60, 40, 30, 20, 17, 15, 12, 10};
int16_t G_Amplifier_band = 0;
uint8_t G_check_bias = FALSE;

/* Read-only load of amplifier_cal.ini (written by sdrcore-trans). Bad lines are skipped. */
int Init_amplifier_calibration() {
    int status = 0;
    char file_name[PATH_MAX] = {0};
    FILE *Power_initialize;
    char power_init_record[132];
    int record = 0;
    char *record_number;
    char *power_value;

    if ((homedir = My_getenv("HOME")) == NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration . getenv failed\n", line_number++);
        return 0;
    }
    snprintf(file_name, sizeof (file_name), "%s/amplifier_cal.ini", homedir);
    Power_initialize = fopen(file_name, "r");
    if (Power_initialize == NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration . Open failed: %s\n", line_number++, file_name);
        return 0;
    }
    while (fgets(power_init_record, sizeof (power_init_record), Power_initialize) != NULL) {
        record_number = strstr(power_init_record, "RECORD=");
        power_value = strstr(power_init_record, "POWER_LEVEL=");
        if (record_number == NULL || power_value == NULL) {
            continue;
        }
        record = atoi(record_number + 7);
        if (record < 0 || record >= 12) {
            continue;
        }
        G_amp_calibration_stack[record].record = record;
        G_amp_calibration_stack[record].band = record;
        G_amp_calibration_stack[record].power_level = atoi(power_value + 12);
    }
    fclose(Power_initialize);
    status = 1;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Initialize_amplifier_calibration . Finished\n", line_number++);
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

int Cal_amplifier_tune_button(int on_off) {

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Cal_amplifier_tune_button . NOOP\n", line_number++);
    return on_off;
}

int Amplifier_Set_Power_Level(uint8_t command, char *buf) {
    int status = 0;
    static int record = 0;
    uint8_t opcode_data_8_bit;
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
            Init_amplifier_calibration();
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_AMPLIFIER_INITIALIZE . Band: %d, Record: %d, Cal: %d\n", line_number++,
                    G_Amplifier_band, record, G_amp_calibration_stack[record].power_level);
            Sleep(50);
            Gui_send_param(CMD_GET_AMPLIFIER_POWER, AMPLIFIER_POWER_REPORT);
            Gui_send_param(CMD_SET_POTENTIA_CALIBRATION, G_amp_calibration_stack[record].power_level);
            break;

        case CMD_SET_POTENTIA_CALIBRATION:
            op_code_data_32 = (int*) &buf[1];
            memcpy(&i_opcode_data, op_code_data_32, 4);
            print_time(1);
            fprintf(G_fp_logfile, "[%d] UDP Thread . CMD_SET_POTENTIA_CALIBRATION Calibration Value: %d\n",
                    line_number++, i_opcode_data);
            G_amp_calibration_stack[record].power_level = i_opcode_data;
            SDRcore_trans_send_param(CMD_SET_POTENTIA_CALIBRATION, i_opcode_data);
            break;

        case CMD_SET_AMPLIFIER_POWER:
            /* amplifier.ini removed; the value was never used. Client sends 100 on band select. */
            print_time(1);
            fprintf(G_fp_logfile, "[%d] CMD_SET_AMPLIFIER_POWER . %d . Ignored\n", line_number++, opcode_data_8_bit);
            break;

        case CMD_GET_AMPLIFIER_POWER:
            print_time(1);
            fprintf(G_fp_logfile, "[%d] CMD_GET_AMPLIFIER_POWER . Called . NOOP\n", line_number++);
            break;
    }
    return status;
}
