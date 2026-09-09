#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
#include "usbavrcmd.h"
//#include "SRDLL.h"
#include "extern.h"
#include "version.h"

int Delete_power_ini_file();

#define VERSION 3

int power_defaults_PCB_6[] = {47, 30, 24, 29, 53, 72, 28, 30, 45, 40, 40, 40};
int power_defaults_PCB_5[] = {47, 30, 24, 29, 53, 72, 28, 30, 45, 40, 40, 40};
int power_defaults_PCB_4[] = {47, 30, 24, 29, 53, 72, 28, 30, 45, 40, 40, 40};
int power_defaults_PCB_C[] = {47, 30, 24, 29, 53, 72, 28, 30, 45, 40, 40, 40};
int power_defaults_PCB_0[] = {80, 35, 24, 28, 26, 35, 28, 27, 45, 33, 40, 40};

struct {
    int record;
    int band;
    int power_level;
} G_power_stack [12];


int G_version = 0;
uint32_t Tranceiver_band = 0;
//const char *homedir;

int Parse_power_cal_record(char *record, char *field) {
    int status = -1;
    char source_field[80] = {0};
    char *result_field;
    int size = 0;

    strcpy(source_field, field);
    size = strlen(source_field);

    if ((result_field = strstr(record, source_field)) != NULL) {
        status = atoi((result_field + (size + 1)));
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] Parse_User_Controls_record. Record: %s, Field: %s, Size: %d, Result: %s\n", line_number++, record, source_field,
        //	size, result_field);
        return status;
    }
    return status;
}

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

int Delete_power_ini_file() {
    int status = 0;
    FILE *Power_ini_delete;
    char file_name[PATH_MAX] = {0};
    int ret;

    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Delete_power_ini_file . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/power_cal.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] delete_ini_file . Delete_power_ini_file Path: %s\n", line_number++, homedir);
        Power_ini_delete = fopen(file_name, "r");
        if (Power_ini_delete != NULL) {
            fclose(Power_ini_delete);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Delete_power_ini_file . File Exists . Will now delete \n", line_number++);
            ret = remove(homedir);
            if (ret == 0) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Delete_power_ini_file . The initialization file has been deleted \n", line_number++);
                status = 1;
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Delete_power_ini_file . The initialization file Deletion has FAILED \n", line_number++);
                status = 0;
            }
        }
    }
    return status;
}

int Update_power_ini_file() {
    FILE *Power_ini_update;
    char file_name[PATH_MAX] = {0};
    int lenght = 0;
    int band = 0;
    int status = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_power_ini_file . Called \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(file_name, homedir);
        strcat(file_name, "/power_cal.ini");
        Power_ini_update = fopen(file_name, "w");
        if (Power_ini_update != NULL) {
            fprintf(Power_ini_update, "VERSION=%d\n", VERSION);
            for (band = 0; band < 12; band++) {
                fprintf(Power_ini_update, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", G_power_stack[band].record, G_power_stack[band].band,
                        G_power_stack[band].power_level);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Update_power_ini_file . RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", line_number++,
                        G_power_stack[band].record, G_power_stack[band].band, G_power_stack[band].power_level);
            }
            fflush(Power_ini_update);
            fclose(Power_ini_update);
            status = 1;
            SDRcore_trans_send_param(CMD_SET_SDRCORE_TRANS_INITIALIZE, 1);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Update_power_ini_file . Finished \n", line_number++);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Update_power_ini_file . Open file failed\n", line_number++);
            status = 0;
        }
    }
    return status;
}

int Create_power_ini_file() {
    FILE *Power_create_ini;
    char file_name[PATH_MAX] = {0};
    //int lenght = 0;
    int band = 0;
    int status = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Create_power_ini_file . Called \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(file_name, homedir);
        strcat(file_name, "/power_cal.ini");
        Power_create_ini = fopen(file_name, "r");
        if (Power_create_ini == NULL) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Create_power_ini_file . power_cal.ini not found. Creating power_cal.ini \n", line_number++);
            Power_create_ini = fopen(file_name, "w");
            if (Power_create_ini != NULL) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Create_power_ini_file . Creating power_cal.ini for Version: %d \n",
                        line_number++, VERSION);
                fprintf(Power_create_ini, "VERSION=%d\n", VERSION);
                for (band = 0; band < 12; band++) {
                    switch (G_pcb_version) {
                        case 6:
                            fprintf(Power_create_ini, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", band, band, power_defaults_PCB_6[band]);
                            break;
                        case 5:
                            fprintf(Power_create_ini, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", band, band, power_defaults_PCB_5[band]);
                            break;
                        case 4:
                            fprintf(Power_create_ini, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", band, band, power_defaults_PCB_4[band]);
                            break;
                        case 2:
                            fprintf(Power_create_ini, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", band, band, power_defaults_PCB_C[band]);
                            break;
                        default:
                            fprintf(Power_create_ini, "RECORD=%d,BAND=%d,POWER_LEVEL=%d\n", band, band, power_defaults_PCB_5[band]);
                            break;
                    }
                }
                fclose(Power_create_ini);
                status = 2;
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Create_power_ini_file . Creation of power_cal.ini file Failed\n", line_number++);
                status = 0;
            }
        } else {
            if (Power_create_ini != NULL) {
                fclose(Power_create_ini);
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Create_power_ini_file . File exists . Not creating power_cal.ini file\n", line_number++);
            status = 1;
        }
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Create_power_ini_file . Finished \n", line_number++);
    return status;
}

int Initialize_power_calibration() {
    int status = 0;
    FILE *Power_initialize;
    char file_name[PATH_MAX] = {0};
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
        fprintf(G_fp_logfile, "[%d] Initialize_power_calibration . Called \n", line_number++);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_power_calibration . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/power_cal.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_power_calibration . power_cal.ini-> Path: %s\n", line_number++, homedir);
        Power_initialize = fopen(file_name, "r");
        if (Power_initialize != NULL) {
            fgets(iq_init_record, sizeof (iq_init_record), Power_initialize); //Skip the VERSION line
            while (fgets(iq_init_record, sizeof (iq_init_record), Power_initialize) != NULL) {
                power_record.record_number = strstr(iq_init_record, "RECORD=");
                power_record.band = strstr(iq_init_record, "BAND=");
                power_record.power_value = strstr(iq_init_record, "POWER_LEVEL=");
                mynumber = atoi((power_record.record_number + 7));
                G_power_stack[record].record = mynumber;
                mynumber = atoi((power_record.band + 5));
                G_power_stack[record].band = mynumber;
                mynumber = atoi((power_record.power_value + 12));
                G_power_stack[record].power_level = mynumber;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Initialize_power_calibration . Record: %d, Band: %d, Power Level: %d\n",
                        line_number++, G_power_stack[record].record, G_power_stack[record].band,
                        G_power_stack[record].power_level);
                record++;
            }
            fclose(Power_initialize);
            status = 1;
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Initialize_power_calibration . Open file failed\n", line_number++);
            status = 0;
        }
    } else {
        status = 0;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_power_calibration .  SHGetKnownFolderPath failed\n", line_number++);
    }
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
    //static uint8_t band = 0;
    static uint8_t calibration_value = 0;
    static uint8_t current_value = 40;
    static uint32_t calibration_frequency = 0;
    static uint32_t proficio_configured = 0;
    uint8_t tune;
    unsigned long setLO_status = 0;
    static int record = 0;
    char mode = 'n';
    static uint8_t opcode_data_8_bit;
    static short int *opcode_data_short;
    static short int s_opcode_data;
    static int *op_code_data_32;
    static int i_opcode_data;
    
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
            //ModeChanged('T');
            mode = 'T';
            //status = Radio_send_parameters(SET_CW_MODE, mode, 1);
            //Sleep(50);
            //status = Radio_send_parameters(SET_CW_MODE, mode, 1);
            //Sleep(50);
            setLO_status = freq_queue_add(calibration_frequency);
            Sleep(50);
            Initialize_power_calibration();
            //G_Proficio_Allow_Temp_Check = FALSE; Restore this for production
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_BAND . Band: %d, Power_Value: %d\n", line_number++,
                    G_power_stack[record].band, G_power_stack[record].power_level);
            Gui_send_param(CMD_GET_BAND_POWER, G_power_stack[record].power_level);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_BAND . Calling SDRcore_recv_send_param \n", line_number++);
            SDRcore_recv_send_param(CMD_SET_SDR_CORE_BAND, Tranceiver_band);
            SDRcore_recv_send_param(CMD_SET_MAIN_MODE, MODE_TUNE); //Set the SDRcore processes into TUNE mode
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_BAND . Calling SDRcore_trans_send_param \n", line_number++);
            SDRcore_trans_send_param(CMD_SET_SDR_CORE_BAND, Tranceiver_band); 
            SDRcore_trans_send_param(CMD_SET_MAIN_MODE, MODE_TUNE); //Set the SDRcore processes into TUNE mode
            SDRcore_trans_send_param(CMD_SET_BAND_POWER_BAND, Tranceiver_band);
            SDRcore_trans_send_param(CMD_SET_BAND_POWER_POWER, G_power_stack[record].power_level);
            SDRcore_trans_send_param(CMD_SET_MAIN_MODE, MODE_TUNE); //Set the SDRcore processes into TUNE mode
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
                G_power_stack[record].power_level = calibration_value;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_POWER . Calling Update_power_ini_file \n", line_number++);
                status = Update_power_ini_file();
                if (status == 1) {
                    Sleep(100);
                    SDRcore_trans_send_param(CMD_SET_SDR_CORE_BAND, Tranceiver_band);
                    SDRcore_trans_send_param(CMD_SET_MAIN_MODE, MODE_TUNE); //Set the SDRcore processes into TUNE mode
                    SDRcore_trans_send_param(CMD_SET_BAND_POWER_BAND, Tranceiver_band);
                    SDRcore_trans_send_param(CMD_SET_BAND_POWER_POWER, G_power_stack[record].power_level);
                    SDRcore_trans_send_param(CMD_SET_MAIN_MODE, MODE_TUNE); //Set the SDRcore processes into TUNE mode
                }
            } else {
                Gui_send_param(CMD_GET_BAND_POWER, 0);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_POWER . Transceiver was not configured properly\n", line_number++);
                //MessageBoxA(NULL, "Logic Error in Power Calibration Routine. Send Logs to Multus SDR, LLC. \r\n ",
                //	"MS-SDR", MB_OK | MB_ICONASTERISK);
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_POWER . Finished \n", line_number++);
            break;

        case CMD_SET_BAND_VOLUME_DEFAULTS:
            print_time(1);
            fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_VOLUME_DEFAULTS . Called\n", line_number++);
            status = Delete_power_ini_file();
            if (status == 1) {
                Create_power_ini_file();
            }
            Initialize_power_calibration();
            Gui_send_param(CMD_GET_BAND_POWER, G_power_stack[record].power_level);
            Sleep(50);
            SDRcore_trans_send_param(CMD_SET_SDR_CORE_BAND, Tranceiver_band);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_VOLUME_DEFAULTS . Finished \n", line_number++);
            break;

        case CMD_GET_BAND_POWER:
            print_time(1);
            if (proficio_configured) {
                fprintf(G_fp_logfile, "[%d] CMD_GET_BAND_POWER . Called . Sending: %d \n", line_number++, current_value);
                current_value = G_power_stack[record].power_level;
                Gui_send_param(CMD_GET_BAND_POWER, current_value);
            } else {
                Gui_send_param(CMD_GET_BAND_POWER, 0);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] CMD_SET_BAND_POWER_POWER . Transceiver was not configured properly\n", line_number++);
                //MessageBoxA(NULL, "Logic Error in Power Calibration Routine. Send Logs to Multus SDR, LLC. \r\n ",
                //	"MS-SDR", MB_OK | MB_ICONASTERISK);
            }
            break;

        case CMD_CALIBRATION_MASTER_RESET:
            print_time(1);
            fprintf(G_fp_logfile, "[%d] CMD_CALIBRATION_MASTER_RESET . Called . Initializing all variables \n", line_number++);
            Tranceiver_band = 0;
            calibration_value = 0;
            current_value = 0;
            calibration_frequency = 0;
            proficio_configured = 0;
            G_calibration_mode = 0;
            G_Proficio_Allow_Temp_Check = TRUE;
            SDRcore_trans_send_param(CMD_SET_SDR_CORE_BAND, Tranceiver_band);
            break;

        case CMD_CALIBRATION_TUNE:
            print_time(1);
            tune = opcode_data_8_bit;
            fprintf(G_fp_logfile, "[%d] CMD_CALIBRATION_TUNE . Tune: %d \n", line_number++, tune);
            Cal_tune_button(tune);
            break;
    }
    return status;
}