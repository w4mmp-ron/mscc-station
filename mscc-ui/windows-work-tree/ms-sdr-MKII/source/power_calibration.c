#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
#include "usbavrcmd.h"
//#include "SRDLL.h"
#include "extern.h"
#include "version.h"

/* QRP power calibration.
 * sdrcore-trans owns ~/power_cal.ini (creates it with factory values, writes it on 0xA2).
 * ms-sdr only reads it on band select to report the stored value to the client, tunes the
 * radio, and forwards the calibration commands to sdrcore-trans. */

struct {
    int record;
    int band;
    int power_level;
} G_power_stack [12];

uint32_t Tranceiver_band = 0;
//const char *homedir;

uint8_t Band_to_record(uint32_t band) {
    int record = 0;
    switch (band) {
        case 630:
            record = 10;
            break;
        case 2200:
            record = 11;
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

uint32_t Set_freq_for_power_cal(uint32_t band) {

    int status = 0;
    uint32_t freq = 0;

    G_dll_active = 1;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Set_band_for_power_cal . Band: %d\n", line_number++, band);

    switch (band) {
        case 2200:
            freq = 136000;
            break;
        case 630:
            freq = 475000;
            break;
        case 160:
            freq = 1810000;
            break;
        case 80:
            freq = 3510000;
            break;
        case 60:
            freq = 5346500;
            break;
        case 40:
            freq = 7010000;
            break;
        case 30:
            freq = 10110000;
            break;
        case 20:
            freq = 14010000;
            break;
        case 17:
            freq = 18078000;
            break;
        case 15:
            freq = 21010000;
            break;
        case 12:
            freq = 24900000;
            break;
        case 10:
            freq = 28010000;
            break;
    }

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Set_band_for_power_cal . Finished . Returning Frequency: %ld\n", line_number++, freq);
    G_dll_active = 0;
    return freq;
}

/* Read-only load of power_cal.ini (written by sdrcore-trans). Missing file or bad lines -> 0. */
int Initialize_power_calibration() {
    int status = 0;
    FILE *Power_initialize;
    char file_name[PATH_MAX] = {0};
    char iq_init_record[132];
    int record = 0;
    char *record_number;
    char *power_value;

    for (record = 0; record < 12; record++) {
        G_power_stack[record].record = record;
        G_power_stack[record].band = record;
        G_power_stack[record].power_level = 0;
    }
    if ((homedir = My_getenv("HOME")) == NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_power_calibration . getenv failed\n", line_number++);
        return 0;
    }
    snprintf(file_name, sizeof (file_name), "%s/power_cal.ini", homedir);
    Power_initialize = fopen(file_name, "r");
    if (Power_initialize == NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_power_calibration . Open failed: %s\n", line_number++, file_name);
        return 0;
    }
    while (fgets(iq_init_record, sizeof (iq_init_record), Power_initialize) != NULL) {
        record_number = strstr(iq_init_record, "RECORD=");
        power_value = strstr(iq_init_record, "POWER_LEVEL=");
        if (record_number == NULL || power_value == NULL) {
            continue; /* VERSION line or junk */
        }
        record = atoi(record_number + 7);
        if (record < 0 || record >= 12) {
            continue;
        }
        G_power_stack[record].power_level = atoi(power_value + 12);
    }
    fclose(Power_initialize);
    status = 1;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Initialize_power_calibration . Finished\n", line_number++);
    return status;
}

int Cal_tune_button(int on_off) {
    static int tuning_mode = 0;
    int key = 0;
    //int ret;

    G_dll_active = TRUE;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Cal_tune_button called . on_off: %d\n", line_number++, on_off);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Cal_tune_button . Calling SDRcore_trans_send_param . CMD_SET_TX_ON. on_off: %d\n", line_number++, on_off);
    SDRcore_trans_send_param(CMD_SET_TX_ON, on_off);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Cal_tune_button . Calling SDRcore_recv_send_param . CMD_SET_TX_ON. on_off: %d\n", line_number++, on_off);
    SDRcore_recv_send_param(CMD_SET_TX_ON, on_off);
    switch (on_off) {
        case 1:
            if (G_Hw_Started) {
                Gui_send_param(CMD_SET_HDSDR_STATUS, 0);
                tuning_mode = TRUE;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Cal_tune_button . Calling SetModeRxTX, on_off: %d\n", line_number++, on_off);
                Tune_button(on_off, FALSE);
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Cal_tune_button . Tranceiver not started. on_off: %d\n", line_number++, on_off);
                Gui_send_param(CMD_SET_HDSDR_STATUS, HDSDR_STATUS_STOP_MODE);
            }
            break;
        case 0:
            if (G_Hw_Started) {
                if (tuning_mode) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Cal_tune_button . Calling SetModeRxTX . on_off %d\n", line_number++, on_off);
                    Tune_button(on_off, FALSE);
                    tuning_mode = FALSE;
                }
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Cal_tune_button . Transceiver not started.  on_off: %d, mode: %c\n", line_number++, on_off, G_mode);
            }
            break;
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Cal_tune_button . Finished. Status: %d\n", line_number++, on_off);
    G_dll_active = FALSE;
    return on_off;
}

int Power_calibration(uint32_t command, char *buf) {
    int status = 0;
    static uint8_t calibration_value = 0;
    static uint32_t calibration_frequency = 0;
    static uint32_t proficio_configured = 0;
    unsigned long setLO_status = 0;
    static int record = 0;
    static uint8_t opcode_data_8_bit;
    static short int *opcode_data_short;
    static short int s_opcode_data;

    opcode_data_8_bit = (uint8_t) (buf[1]);
    opcode_data_short = (short int*) &buf[1];
    memcpy(&s_opcode_data, opcode_data_short, 2);

    switch (command) {
        case CMD_SET_BAND_POWER_BAND:
            print_time(1);
            fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_BAND . Called\n", line_number++);
            G_calibration_mode = 1;
            Tranceiver_band = s_opcode_data;
            record = Band_to_record(Tranceiver_band);
            calibration_frequency = Set_freq_for_power_cal(Tranceiver_band);
            G_tune_freq = calibration_frequency;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_BAND . Band: %d, Record: %d, Calibration Freq: %ld\n", line_number++,
                    Tranceiver_band, record, calibration_frequency);
            setLO_status = freq_queue_add(calibration_frequency);
            Sleep(50);
            Initialize_power_calibration(); /* file is written by sdrcore-trans */
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_BAND . Band: %d, Power_Value: %d\n", line_number++,
                    Tranceiver_band, G_power_stack[record].power_level);
            Gui_send_param(CMD_GET_BAND_POWER, G_power_stack[record].power_level);
            SDRcore_recv_send_param(CMD_SET_SDR_CORE_BAND, Tranceiver_band);
            SDRcore_recv_send_param(CMD_SET_MAIN_MODE, MODE_TUNE); //Set the SDRcore processes into TUNE mode
            SDRcore_trans_send_param(CMD_SET_SDR_CORE_BAND, Tranceiver_band);
            SDRcore_trans_send_param(CMD_SET_MAIN_MODE, MODE_TUNE);
            SDRcore_trans_send_param(CMD_SET_BAND_POWER_BAND, Tranceiver_band);
            proficio_configured = 1;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_BAND . FINISHED \n", line_number++);
            break;

        case CMD_SET_BAND_POWER_POWER:
            calibration_value = opcode_data_8_bit;
            print_time(1);
            fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_POWER . Received . Cal Value: %d\n", line_number++,
                    calibration_value);
            if (proficio_configured) {
                G_power_stack[record].power_level = calibration_value; /* copy for 0xB4; trans saves the file */
                SDRcore_trans_send_param(CMD_SET_BAND_POWER_BAND, Tranceiver_band);
                SDRcore_trans_send_param(CMD_SET_BAND_POWER_POWER, calibration_value);
            } else {
                Gui_send_param(CMD_GET_BAND_POWER, 0);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_POWER . Transceiver was not configured properly\n", line_number++);
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_POWER . Finished \n", line_number++);
            break;

        case CMD_SET_BAND_VOLUME_DEFAULTS:
            /* Not used by the client. Removed with the move of power_cal.ini to sdrcore-trans. */
            print_time(1);
            fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_VOLUME_DEFAULTS . Not supported. Ignored\n", line_number++);
            break;

        case CMD_GET_BAND_POWER:
            print_time(1);
            if (proficio_configured) {
                fprintf(G_fp_logfile, "[%d] CMD_GET_BAND_POWER . Called . Sending: %d \n", line_number++,
                        G_power_stack[record].power_level);
                Gui_send_param(CMD_GET_BAND_POWER, G_power_stack[record].power_level);
            } else {
                Gui_send_param(CMD_GET_BAND_POWER, 0);
                fprintf(G_fp_logfile, "[%d] CMD_GET_BAND_POWER . Transceiver was not configured properly\n", line_number++);
            }
            break;

        case CMD_CALIBRATION_MASTER_RESET:
            print_time(1);
            fprintf(G_fp_logfile, "[%d] CMD_CALIBRATION_MASTER_RESET . Called . Initializing all variables \n", line_number++);
            Tranceiver_band = 0;
            calibration_value = 0;
            calibration_frequency = 0;
            proficio_configured = 0;
            G_calibration_mode = 0;
            G_Proficio_Allow_Temp_Check = TRUE;
            SDRcore_trans_send_param(CMD_SET_SDR_CORE_BAND, Tranceiver_band);
            break;

        case CMD_CALIBRATION_TUNE:
            print_time(1);
            fprintf(G_fp_logfile, "[%d] CMD_CALIBRATION_TUNE . Tune: %d \n", line_number++, opcode_data_8_bit);
            Cal_tune_button(opcode_data_8_bit);
            break;
    }
    (void) setLO_status;
    return status;
}
