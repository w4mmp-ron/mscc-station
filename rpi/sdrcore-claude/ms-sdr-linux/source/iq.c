#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
#include "usbavrcmd.h"
//#include "ExtIODll.h"
//#include "ExtIOFunctions.h"
#include "usbavrcmd.h"

//#include "SRDLL.h"
#include "extern.h"
#include "version.h"

#define IQ_RX 0
#define IQ_TX 1
#define IQ_NO_MODE_SET 3

uint8_t G_IQBD_Monitor_ON = 0;
uint8_t G_IQBD_New_Value_Set = 0;
//uint8_t G_iq_band = 0;


uint32_t iq_cal_freq;
uint16_t iq_band;
int iq_offset;
uint8_t rx_tx = 3;
int iq_calibration_freqs[] = {28350000, 24930000, 21225000, 18110000, 14175000, 10125000, 7150000, 5330500, 3750000,
    1900000, 475000, 136000};

int IQ_tune_button(int on_off) {
    int key = 0;
    int ret;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] IQ_tune_button called . on_off: %d\n", line_number++, on_off);
    switch (on_off) {
        case 1:
            print_time(0);
            fprintf(G_fp_logfile, "[%d] IQ_tune_button . Calling SetModeRxTX, on_off: %d\n", line_number++, on_off);
            SDRcore_trans_send_param(CMD_SET_TX_ON, on_off);
            SDRcore_recv_send_param(CMD_SET_TX_ON, on_off);
            ret = Tune_button(on_off,FALSE);

            //ret = srSetPTTGetCWInp(on_off, &key);
            break;
        case 0:
            print_time(0);
            fprintf(G_fp_logfile, "[%d] IQ_tune_button . Calling SetModeRxTX . on_off %d\n", line_number++, on_off);
            SDRcore_trans_send_param(CMD_SET_TX_ON, on_off);
            SDRcore_recv_send_param(CMD_SET_TX_ON, on_off);
            //ret =  srSetPTTGetCWInp(on_off, &key);
            ret = Tune_button(on_off,FALSE);
            break;
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] IQ_tune_button . Finished. Status: %d\n", line_number++, on_off);
    return on_off;
}

int IQ_Band_to_Record(int band) {
    int record = 0;
    switch (band) {
        case 10:
            record = 0;
            break;
        case 12:
            record = 1;
            break;
        case 15:
            record = 2;
            break;
        case 17:
            record = 3;
            break;
        case 20:
            record = 4;
            break;
        case 30:
            record = 5;
            break;
        case 40:
            record = 6;
            break;
        case 60:
            record = 7;
            break;
        case 80:
            record = 8;
            break;
        case 160:
            record = 9;
            break;
        case 630:
            record = 10;
            break;
        case 2200:
            record = 11;
            break;
    }
    return record;
}

int IQ_calibration(uint8_t command, char *buffer) {
    int status = 0;
    uint8_t tune;
    uint8_t t_opcode_data;
    short int *opcode_data;
    short int s_opcode_data;
    int *op_code_data_32;
    int i_opcode_data;
    t_opcode_data = (uint8_t) (buffer[1]);
    opcode_data = (short int*) &buffer[1];
    memcpy(&s_opcode_data, opcode_data, 2);
    op_code_data_32 = (int*) &buffer[1];
    memcpy(&i_opcode_data, op_code_data_32, 4);
    char mode = 'T';
    uint8_t iq_record = 0;

    switch (command) {
        case CMD_SET_IQBD_MONITOR:
            print_time(0);
            G_IQBD_Monitor_ON = t_opcode_data;
            fprintf(G_fp_logfile, "[%d] IQ_calibration . CMD_SET_IQBD_MONITOR . G_IQBD_Monitor %d\n",
                    line_number++, G_IQBD_Monitor_ON);
            break;

        case CMD_SET_IQ_DEFAULTS:
            print_time(0);
            fprintf(G_fp_logfile, "[%d] IQ_calibration . CMD_SET_IQ_DEFAULTS . RX/TX Mode %d\n", line_number++, rx_tx);
            if (rx_tx == IQ_TX) {
                SDRcore_trans_send_param(CMD_SET_IQ_DEFAULTS, 1);
            } else {
                SDRcore_recv_send_param(CMD_SET_IQ_DEFAULTS, 1);
            }
            break;

        case IQ_CALIBRATION_RX_TX:
            rx_tx = t_opcode_data;
            G_calibration_mode = TRUE;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] IQ_calibration . IQ_CALIBRATION_RX_TX . RX/TX Mode %d, G_calibration_mode: %d\n",
                    line_number++, rx_tx, G_calibration_mode);
            break;

        case CMD_SET_IQ_BAND:
            iq_band = s_opcode_data;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] IQ_calibration . CMD_SET_IQ_BAND . iq_band: %d\n", line_number++, iq_band);
            if (rx_tx == IQ_TX) {
                iq_record = IQ_Band_to_Record(iq_band);
                iq_cal_freq = iq_calibration_freqs[iq_record];
                G_tune_freq = iq_cal_freq;
                ModeChanged(mode);
                Sleep(50);
                freq_queue_add(iq_cal_freq);
                Sleep(50);
                SDRcore_trans_send_param(CMD_SET_IQ_BAND, iq_band);
                SDRcore_trans_send_param(CMD_SET_MAIN_MODE, MODE_TUNE);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] IQ_calibration . CMD_SET_IQ_BAND . IQ Calibration Freq: %d\n",
                        line_number++, iq_cal_freq);
            } else {
                SDRcore_recv_send_param(CMD_SET_IQ_BAND, iq_band);
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] IQ_calibration . CMD_SET_IQ_BAND . Finished. status: %d\n", line_number++, status);
            break;

        case CMD_SET_IQ_OFFSET:
            op_code_data_32 = (int*) &buffer[1];
            memcpy(&i_opcode_data, op_code_data_32, 4);
            iq_offset = i_opcode_data;
            print_time(1);
            fprintf(G_fp_logfile, "[%d] IQ_calibration . CMD_SET_IQ_OFFSET . Calling set_iq_record . iq_offset: %d\n", line_number++, iq_offset);
            if (rx_tx == IQ_RX) {
                SDRcore_recv_send_param(CMD_SET_IQ_OFFSET, iq_offset);
            } else {
                SDRcore_trans_send_param(CMD_SET_IQ_OFFSET, iq_offset);
            }
            G_IQBD_New_Value_Set = TRUE;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] IQ_calibration . CMD_SET_IQ_OFFSET . Finished. status: %d\n", line_number++, status);
            break;

        case CMD_SET_COMMIT_IQ:
            print_time(1);
            fprintf(G_fp_logfile, "[%d] IQ_calibration . CMD_SET_COMMIT_IQ . Calling SDRcore: RX/TX: %d, t_opcode_data: %d\n",
                    line_number++, rx_tx, t_opcode_data);

            if (rx_tx == IQ_RX) {
                SDRcore_recv_send_param(CMD_SET_COMMIT_IQ, iq_band);
            } else {
                SDRcore_trans_send_param(CMD_SET_COMMIT_IQ, iq_band);
            }
            Sleep(1000);
            Gui_send_param(IQ_OPERATION_COMPLETE, 1);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] IQ_calibration . CMD_SET_COMMIT_IQ . Finished . iq_offset: %d\n", line_number++, iq_offset);
            break;

        case IQ_CALIBRATION_TUNE:
            tune = t_opcode_data;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] IQ_calibration . IQ_CALIBRATION_TUNE . tune: %d\n",
                    line_number++, tune);
            if (rx_tx != IQ_TX) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] IQ_calibration . IQ_CALIBRATION_TUNE . Invalid TUNE request. IQ in RX mode %d\n",
                        line_number++, rx_tx);
            } else {
                IQ_tune_button(tune);
            }
            break;
    }
    return status;
}


