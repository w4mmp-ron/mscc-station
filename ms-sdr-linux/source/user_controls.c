#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
#include "extern.h"
#include "usbavrcmd.h"
#include "version.h"

int Create_User_Controls(int force);
int Write_User_Controls();

#define VERSION 13
#define DIGITAL_SOUND_DEVICE 0
#define PHONES_SOUND_DEVICE 1
#define REMOTE_SOUND_DEVICE 2  /* Phones + REMOTE AUDIO; levels use Phones gains */

//const char *homedir;
uint8_t G_QRP = 0;

struct User_Record User_Controls;

uint8_t opcode_data_8_bit;
uint8_t t_opcode_data;
short int *opcode_data;
short int s_opcode_data;
int *op_code_data_32;
int i_opcode_data;
BOOL G_process_user_control_status = FALSE;
uint8_t G_Speaker_volume = 0;

int Parse_User_Controls_record(char *record, char *field) {
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

int Check_Version() {
    int status = -1;
    FILE *fp_User_ini;
    char file_name[PATH_MAX] = {0};
    char init_record[30];
    char field[20] = {0};
    char *record_status = NULL;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_Version . Called.\n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_Version . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/user_controls.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_Version . user_controls.ini Path: %s\n", line_number++, homedir);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_Version . File Open \n", line_number++);
        fp_User_ini = fopen(file_name, "r");
        if (fp_User_ini != NULL) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Check_Version . File Open \n", line_number++);
            record_status = fgets(init_record, 30, fp_User_ini);
            if (record_status != NULL) {
                memset(field, 0, sizeof (field));
                strcpy(field, "VERSION");
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_Version . Calling Parse_User_Controls. field %s\n", line_number++, field);
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Version = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Check_Version . VERSION: %d\n", line_number++, status);
                }
                fclose(fp_User_ini);
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_Version . Read FAILED\n", line_number++);
            }
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Check_Version .  Open FAILED: %s \n", line_number++, homedir);
        }

    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_Version .  SHGetKnownFolderPath FAILED: %s \n", line_number++, homedir);
    }
    return status;
}

int User_Controls_Init() {
    int status = 0;
    int error_flag = 0;
    FILE *fP_User_ini = NULL;
    char init_record[300];
    char field[30] = {0};
    char file_name[PATH_MAX] = {0};
    char *record_status = NULL;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] User_Controls_Init . Called.\n", line_number++);
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] User_Controls_Init . Calling . Check_Version\n", line_number++);
    //status = Check_Version(); //Check the VERSION number in the user_controls.ini file. 
    //if (status == -1) {//If the VERSION parameter is not present, create the user_controls.ini file
    //    Create_User_Controls(TRUE);
    //} else {
    //    if (status != VERSION) {//The Version parameter exists but does not match this version of the source.
    //        Create_User_Controls(TRUE);
     //   }
    //}
    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] User_Controls_Init . Default Path: %s\n", line_number++, homedir);
        strcat(file_name, homedir);
        strcat(file_name, "/user_controls.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] User_Controls_Init . user_controls.ini Path: %s\n", line_number++, file_name);
        User_Controls.Panadapter_Status = 0;
        User_Controls.Smeter_Status = 0;
        fP_User_ini = fopen(file_name, "r");
        if (fP_User_ini != NULL) {
            record_status = fgets(init_record, sizeof (init_record), fP_User_ini);
            while (record_status != NULL) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Init . Source record: %s", line_number++, init_record);
                memset(field, 0, sizeof (field));
                strcpy(field, "VERSION");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Version = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . VERSION: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "NB_STATUS");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.NB = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . NB_STATUS: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof(field));
                strcpy(field, "AUDIO_DEVICE");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Audio_Device = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . AUDIO_DEVICE: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof(field));
                strcpy(field, "DIGITAL_VOLUME_LEVEL");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Digital_Volume_Level = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . DIGITAL_VOLUME_LEVEL: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof(field));
                strcpy(field, "DIGITAL_MIC_GAIN");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Digital_Mic_Gain = status;
                    G_Speaker_volume = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . DIGITAL_MIC_GAIN: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof(field));
                strcpy(field, "PHONES_VOLUME_LEVEL");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Phones_Volume_Level = status;
                    G_Speaker_volume = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . PHONES_VOLUME_LEVEL: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof(field));
                strcpy(field, "PHONES_MIC_GAIN");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Phones_Mic_Gain = status;
                    G_Speaker_volume = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . PHONES_MIC_GAIN: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "SPEAKER_VOLUME");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Speaker_Volume = status;
                    G_Speaker_volume = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . SPEAKER_VOLUME: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "SPEAKER_MUTE");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Speaker_Muted = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . SPEAKER_MUTE: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "MIC_MUTE");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Mic_Muted = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . MIC_MUTE: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof(field));
                strcpy(field, "MODE");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Mode = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . MODE: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "MIC_VOLUME");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Mic_Volume = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . MIC_VOLUME: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "FILTER_LOW_INDEX");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Filter_Low_index = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . FILTER_LOW_INDEX: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "FILTER_HIGH_INDEX");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Filter_High_index = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . FILTER_HIGH_INDEX: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "AGC_INDEX");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.AGC_Index = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . AGC_INDEX: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "ALC_VALUE");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.ALC_Value = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . ALC_VALUE: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "NR_VALUE");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.NR_Value = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . NR_VALUE: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "NB_THRESHOLD");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.NB_Threshold = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . NB_THRESHOLD: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "NB_PULSE_WIDTH");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.NB_Pulse_Width = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . NB_PULSE_WIDTH: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "COMPRESSION_STATUS");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Compression = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . COMPRESSION_STATUS: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "COMPRESSION_LEVEL");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Compression_Level = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . COMPRESSION_LEVEL: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "PANADAPTER_STATUS");

                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Panadapter_Status = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . PANADAPTER_STATUS: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "SMETER_STATUS");

                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Smeter_Status = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . SMETER_STATUS: %d\n", line_number++, status);
                }

                memset(field, 0, sizeof (field));
                strcpy(field, "PANADAPTER_AVERAGE");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Panadapter_Average = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . PANADAPTER_AVERAGE: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "PANADAPTER_FILL");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Panadapter_Fill = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . PANADAPTER_FILL: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "PANADAPTER_LINE");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Panadapter_Line = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . PANADAPTER_LINE: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "PANADAPTER_MARKER");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Panadapter_Marker = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . PANADAPTER_MARKER: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "PANADAPTER_BACKGROUND");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Panadapter_Background = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . PANADAPTER_BACKGROUND: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "CW_PITCH");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.CW_Pitch = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . CW_PITCH: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "PANADAPTER_CURSOR");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Panadapter_Cursor = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . PANADAPTER_CURSOR: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "PANADAPTER_REFRESH");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    /* 0 kills spectrum (panReady never set); heal sticky poison from old clients. */
                    if (status < 1) status = 6;
                    User_Controls.Panadapter_Refresh = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . PANADAPTER_REFRESH: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "PANADAPTER_BASE");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Panadapter_Base = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . PANADAPTER_BASE: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "PANADAPTER_GAIN");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Panadapter_Gain = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . PANADAPTER_GAIN: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "AUTO_NOTCH");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Auto_Notch = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . AUTO_NOTCH: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "FILTER_CW_INDEX");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Filter_CW_Index = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . FILTER_CW_INDEX: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "AUTO_SNAP_STATUS");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Auto_Snap = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . AUTO_SNAP_STATUS: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "AUTO_SNAP_INDEX");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Auto_Snap_Index = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . AUTO_SNAP_INDEX: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "QRP");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.QRP = status;
                    G_QRP = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . QRP: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "MICROPHONE_STATUS");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Microphone_status = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . MICROPHONE_STATUS: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "WATERFALL_STATUS");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Waterfall_status = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . WATERFALL_STATUS: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "WATERFALL_GRID");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Waterfall_grid = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . WATERFALL_GRID: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "WATERFALL_PALLET");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Waterfall_pallet = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . WATERFALL_PALLET: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "WATERFALL_DIRECTION");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Waterfall_direction = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . WATERFALL_DIRECTION: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "WATERFALL_GAIN");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Waterfall_gain = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . WATERFALL_GAIN: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "WATERFALL_ZERO");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Waterfall_zero = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . WATERFALL_ZERO: %d\n", line_number++, status);
                }
                memset(field, 0, sizeof (field));
                strcpy(field, "WATERFALL_SPEED");
                status = Parse_User_Controls_record(init_record, field);
                if (status != -1) {
                    User_Controls.Waterfall_speed = status;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Init . WATERFALL_SPEED: %d\n", line_number++, status);
                }
                record_status = fgets(init_record, sizeof (init_record), fP_User_ini);
            }
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] User_Controls_Init . %s: Open FAILED\n", line_number++, homedir);
            status = 0;
            return status;
        }

    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] User_Controls_Init . Finished\n", line_number++);
    if (fP_User_ini != NULL) {
        fclose(fP_User_ini);
    }
    return status;
}

/*
 * Headless split:
 *   User_Controls_Apply_To_Cores() — sdrcore-recv/trans only (no client required)
 *   User_Controls_Send_To_Gui()    — MSCC client only (after session claim)
 *   User_Controls_Send()           — both (legacy / full sync)
 */
int User_Controls_Apply_To_Cores() {
    int status = 0;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] User_Controls_Apply_To_Cores. (sdrcore-recv/trans, no GUI)\n",
        line_number++);

    if (!G_network_initialized) {
        print_time(0);
        fprintf(G_fp_logfile,
            "[%d] User_Controls_Apply_To_Cores. SKIP — network not initialized\n",
            line_number++);
        return -1;
    }

    SDRcore_trans_send_param(CMD_SET_MIC_VOLUME, User_Controls.Mic_Volume);
    SDRcore_trans_send_param(CMD_SET_MIC_MUTE, User_Controls.Mic_Muted);
    SDRcore_recv_send_param(CMD_SET_MAIN_MODE, User_Controls.Mode);
    SDRcore_trans_send_param(CMD_SET_MAIN_MODE, User_Controls.Mode);
    SDRcore_recv_send_param(CMD_SET_BW_LOCUT, User_Controls.Filter_Low_index);
    SDRcore_recv_send_param(CMD_SET_BW_HICUT, User_Controls.Filter_High_index);
    SDRcore_recv_send_param(CMD_SET_CW_BW, User_Controls.Filter_CW_Index);
    SDRcore_trans_send_param(CMD_SET_COMPRESSION_STATE, User_Controls.Compression);
    SDRcore_trans_send_param(CMD_SET_COMPRESSION_LEVEL, User_Controls.Compression_Level);
    SDRcore_recv_send_param(CMD_GET_SET_NB_ENABLE, User_Controls.NB);
    SDRcore_recv_send_param(CMD_GET_SET_NB_PULSE_WIDTH, User_Controls.NB_Pulse_Width);
    SDRcore_recv_send_param(CMD_GET_SET_NB_THRESHOLD, User_Controls.NB_Threshold);
    print_time(0);
    fprintf(G_fp_logfile,
        "[%d] User_Controls_Apply_To_Cores. NB -> recv enable=%d pulse=%d threshold=%d\n",
        line_number++, (int)User_Controls.NB, (int)User_Controls.NB_Pulse_Width,
        (int)User_Controls.NB_Threshold);
    SDRcore_recv_send_param(CMD_SET_CW_PITCH, User_Controls.CW_Pitch);
    SDRcore_recv_send_param(CMD_GET_SET_PANADAPTER_SMOOTHING, User_Controls.Panadapter_Average);
    SDRcore_recv_send_param(CMD_GET_SET_PANADAPTER_REFRESH, User_Controls.Panadapter_Refresh);
    SDRcore_recv_send_param(CMD_GET_SET_AGC, User_Controls.AGC_Index);
    SDRcore_recv_send_param(CMD_SET_AGC_FAST_LEVEL, User_Controls.ALC_Value);
    /* NR: 0 = off, non-zero = on with level (same as live CMD_SET_NR) */
    SDRcore_recv_send_param(CMD_SET_NR, User_Controls.NR_Value);
    print_time(0);
    fprintf(G_fp_logfile,
        "[%d] User_Controls_Apply_To_Cores. CMD_SET_NR -> sdrcore-recv NR_VALUE=%d (%s)\n",
        line_number++, (int)User_Controls.NR_Value,
        User_Controls.NR_Value == 0 ? "off" : "on");
    SDRcore_trans_send_param(CMD_SET_PA_BYPASS, User_Controls.QRP);
    SDRcore_trans_send_param(CMD_SET_MICROPHONE_STATUS, User_Controls.Microphone_status);
    SDRcore_recv_send_param(CMD_GET_SET_AUTO_NOTCH, User_Controls.Auto_Notch);

    /* P/D path + volumes — critical for headless digi without a client */
    SDRcore_trans_send_param(CMD_SET_AUDIO_DEVICE, User_Controls.Audio_Device);
    SDRcore_recv_send_param(CMD_SET_AUDIO_DEVICE, User_Controls.Audio_Device);
    if (User_Controls.Audio_Device == DIGITAL_SOUND_DEVICE) {
        SDRcore_trans_send_param(CMD_SET_MIC_VOLUME, User_Controls.Digital_Mic_Gain);
        SDRcore_recv_send_param(CMD_SET_SPEAKER_VOLUME, User_Controls.Digital_Volume_Level);
    } else {
        SDRcore_trans_send_param(CMD_SET_MIC_VOLUME, User_Controls.Phones_Mic_Gain);
        SDRcore_recv_send_param(CMD_SET_SPEAKER_VOLUME, User_Controls.Phones_Volume_Level);
    }

    G_Speaker_volume = User_Controls.Speaker_Volume;
    G_QRP = User_Controls.QRP;

    print_time(0);
    fprintf(G_fp_logfile,
        "[%d] User_Controls_Apply_To_Cores. Finished. Audio_Device=%d Mode=%d QRP=%d\n",
        line_number++, User_Controls.Audio_Device, User_Controls.Mode, User_Controls.QRP);
    return status;
}

int User_Controls_Send_To_Gui() {
    int status = 0;
    char extended_data[4] = { 0 };

    print_time(1);
    fprintf(G_fp_logfile, "[%d] User_Controls_Send_To_Gui. (client only)\n", line_number++);

    memcpy(&extended_data[0], &User_Controls.Waterfall_speed, sizeof (User_Controls.Waterfall_speed));
    Gui_send_param_extended(CMD_SET_WATERFALL_SPEED, (byte *) extended_data, sizeof (User_Controls.Waterfall_speed));

    memcpy(&extended_data[0], &User_Controls.Waterfall_gain, sizeof (User_Controls.Waterfall_gain));
    Gui_send_param_extended(CMD_SET_WATERFALL_GAIN, (byte *) extended_data, sizeof (User_Controls.Waterfall_gain));

    memcpy(&extended_data[0], &User_Controls.Waterfall_zero, sizeof (User_Controls.Waterfall_zero));
    Gui_send_param_extended(CMD_SET_WATERFALL_ZERO, (byte *) extended_data, sizeof (User_Controls.Waterfall_zero));

    extended_data[0] = User_Controls.Waterfall_status;
    Gui_send_param_extended(CMD_SET_WATERFALL_DISPLAY, (byte *) extended_data, sizeof (User_Controls.Waterfall_status));

    extended_data[0] = User_Controls.Waterfall_grid;
    Gui_send_param_extended(CMD_SET_WATERFALL_GRID, (byte *) extended_data, sizeof (User_Controls.Waterfall_grid));

    extended_data[0] = User_Controls.Waterfall_direction;
    Gui_send_param_extended(CMD_SET_WATERFALL_DIRECTION, (byte *) extended_data, sizeof (User_Controls.Waterfall_direction));

    extended_data[0] = User_Controls.Waterfall_pallet;
    Gui_send_param_extended(CMD_SET_WATERFALL_PALET, (byte *) extended_data, sizeof (User_Controls.Waterfall_pallet));
    Gui_send_param(CMD_SET_MIC_VOLUME, User_Controls.Mic_Volume);

    Gui_send_param(CMD_SET_MIC_MUTE, User_Controls.Mic_Muted);
    G_Speaker_volume = User_Controls.Speaker_Volume;
    Gui_send_param(CMD_SET_BW_LOCUT_DEFAULT, User_Controls.Filter_Low_index);
    Gui_send_param(CMD_SET_BW_HICUT_DEFAULT, User_Controls.Filter_High_index);
    Gui_send_param(CMD_SET_CW_BW_DEFAULT, User_Controls.Filter_CW_Index);
    Gui_send_param(CMD_SET_COMPRESSION_STATE, User_Controls.Compression);
    Gui_send_param(CMD_SET_COMPRESSION_LEVEL, User_Controls.Compression_Level);
    Gui_send_param(CMD_GET_SET_NB_ENABLE, User_Controls.NB);
    Gui_send_param(CMD_GET_SET_NB_PULSE_WIDTH, User_Controls.NB_Pulse_Width);
    Gui_send_param(CMD_GET_SET_NB_THRESHOLD, User_Controls.NB_Threshold);
    /* NR: 0 = off; non-zero = on with level (same opcode client uses) */
    Gui_send_param(CMD_SET_NR, User_Controls.NR_Value);
    print_time(0);
    fprintf(G_fp_logfile,
        "[%d] User_Controls_Send_To_Gui. CMD_SET_NR -> client NR_VALUE=%d (%s)\n",
        line_number++, (int)User_Controls.NR_Value,
        User_Controls.NR_Value == 0 ? "off" : "on");
    Gui_send_param(CMD_SET_CW_PITCH, User_Controls.CW_Pitch);
    Gui_send_param(CMD_GET_SET_AGC, User_Controls.AGC_Index);
    Gui_send_param(CMD_SET_AGC_FAST_LEVEL, User_Controls.ALC_Value);
    Gui_send_param(CMD_GET_SET_AUTO_NOTCH, User_Controls.Auto_Notch);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] User_Controls_Send_To_Gui . QRP: %d\n", line_number++, User_Controls.QRP);
    Gui_send_param(CMD_SET_PA_BYPASS, User_Controls.QRP);
    Gui_send_param(CMD_SET_PANADAPTER_DISPLAY, User_Controls.Panadapter_Status);
    Gui_send_param(CMD_SET_SMETER_DISPLAY, User_Controls.Smeter_Status);
    Gui_send_param(CMD_SET_MICROPHONE_STATUS, User_Controls.Microphone_status);
    Gui_send_param(CMD_SET_MAIN_MODE, User_Controls.Mode);
    Gui_send_param(CMD_SET_PHONES_VOLUME_LEVEL, User_Controls.Phones_Volume_Level);
    Gui_send_param(CMD_SET_PHONES_MIC_GAIN_LEVEL, User_Controls.Phones_Mic_Gain);
    Gui_send_param(CMD_SET_DIGITAL_VOLUME_LEVEL, User_Controls.Digital_Volume_Level);
    Gui_send_param(CMD_SET_DIGITAL_MIC_GAIN_LEVEL, User_Controls.Digital_Mic_Gain);
    Gui_send_param(CMD_SET_AUDIO_DEVICE, User_Controls.Audio_Device);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] User_Controls_Send_To_Gui . Finished\n", line_number++);
    return status;
}

int User_Controls_Send() {
    int status = 0;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] User_Controls_Send. (cores + GUI)\n", line_number++);
    status = User_Controls_Apply_To_Cores();
    User_Controls_Send_To_Gui();
    print_time(0);
    fprintf(G_fp_logfile, "[%d] User_Controls_Send . Finished\n", line_number++);
    return status;
}

int User_Controls_Update_Init_File() {
    int status = 0;
    FILE *fp_User_ini;
    char file_name[PATH_MAX] = {0};

    print_time(0);
    fprintf(G_fp_logfile, "[%d] User_Controls_Update_Init_File . Called.\n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] User_Controls_Update_Init_File . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/user_controls.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] User_Controls_Update_Init_File . user_controls.ini Path: %s\n", line_number++, file_name);
        fp_User_ini = fopen(file_name, "w");
        if (fp_User_ini != NULL) {
            fprintf(fp_User_ini,
                "VERSION=%d;\n"
                "SPEAKER_VOLUME=%d;\n"
                "SPEAKER_MUTE=%d;\n"
                "MIC_VOLUME=%d;\n"
                "MIC_MUTE=%d;\n"
                "AUDIO_DEVICE=%d;\n"
                "DIGITAL_VOLUME_LEVEL=%d;\n"
                "DIGITAL_MIC_GAIN=%d;\n"
                "PHONES_VOLUME_LEVEL=%d;\n"
                "PHONES_MIC_GAIN=%d;\n"
                "MODE=%d; \n"
                "FILTER_LOW_INDEX=%d;\n"
                "FILTER_HIGH_INDEX=%d;\n"
                "AGC_INDEX=%d;\n"
                "ALC_VALUE=%d;\n"
                "NR_VALUE=%d;\n"
                "NB_STATUS=%d;\n"
                "NB_THRESHOLD=%d;\n"
                "NB_PULSE_WIDTH=%d;\n"
                "COMPRESSION_STATUS=%d;\n"
                "COMPRESSION_LEVEL=%d;\n"
                "PANADAPTER_AVERAGE=%d;\n"
                "PANADAPTER_FILL=%d;\n"
                "PANADAPTER_LINE=%d;\n"
                "PANADAPTER_MARKER=%d;\n"
                "PANADAPTER_BACKGROUND=%d;\n"
                "CW_PITCH=%d;\n"
                "PANADAPTER_CURSOR=%d;\n"
                "PANADAPTER_REFRESH=%d;\n"
                "PANADAPTER_BASE=%d;\n"
                "PANADAPTER_GAIN=%d;\n"
                "AUTO_NOTCH=%d;\n"
                "FILTER_CW_INDEX=%d;\n"
                "AUTO_SNAP_STATUS=%d;\n"
                "AUTO_SNAP_INDEX=%d;\n"
                "QRP=%d;\n"
                "PANADAPTER_STATUS=%d;\n"
                "SMETER_STATUS=%d;\n"
                "MICROPHONE_STATUS=%d;\n"
                "WATERFALL_STATUS=%d\n"
                "WATERFALL_GRID=%d;\n"
                "WATERFALL_PALLET=%d;\n"
                "WATERFALL_DIRECTION=%d;\n"
                "WATERFALL_GAIN=%d;\n"
                "WATERFALL_ZERO=%d;\n"
                "WATERFALL_SPEED=%d;\n"
                ,
                User_Controls.Version,
                User_Controls.Speaker_Volume,
                User_Controls.Speaker_Muted,
                User_Controls.Mic_Volume,
                User_Controls.Mic_Muted,
                User_Controls.Audio_Device,
                User_Controls.Digital_Volume_Level,
                User_Controls.Digital_Mic_Gain,
                User_Controls.Phones_Volume_Level,
                User_Controls.Phones_Mic_Gain,
                User_Controls.Mode,
                User_Controls.Filter_Low_index,
                User_Controls.Filter_High_index,
                User_Controls.AGC_Index,
                User_Controls.ALC_Value,
                User_Controls.NR_Value,
                User_Controls.NB,
                User_Controls.NB_Threshold,
                User_Controls.NB_Pulse_Width,
                User_Controls.Compression,
                User_Controls.Compression_Level,
                User_Controls.Panadapter_Average,
                User_Controls.Panadapter_Fill,
                User_Controls.Panadapter_Line,
                User_Controls.Panadapter_Marker,
                User_Controls.Panadapter_Background,
                User_Controls.CW_Pitch,
                User_Controls.Panadapter_Cursor,
                User_Controls.Panadapter_Refresh,
                User_Controls.Panadapter_Base,
                User_Controls.Panadapter_Gain,
                User_Controls.Auto_Notch,
                User_Controls.Filter_CW_Index,
                User_Controls.Auto_Snap,
                User_Controls.Auto_Snap_Index,
                User_Controls.QRP,
                User_Controls.Panadapter_Status,
                User_Controls.Smeter_Status,
                User_Controls.Microphone_status,
                User_Controls.Waterfall_status,
                User_Controls.Waterfall_grid,
                User_Controls.Waterfall_pallet,
                User_Controls.Waterfall_direction,
                User_Controls.Waterfall_gain,
                User_Controls.Waterfall_zero,
                User_Controls.Waterfall_speed),
                fclose(fp_User_ini);
        }
        else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] User_Controls_Update_Init_File . Update Failed . File Open FAILED\n", line_number++);
        }
    }
    /*print_time(0);
    fprintf(G_fp_logfile, "[%d] "
            "VERSION=%d;"
            "SPEAKER_VOLUME=%d;"
            "SPEAKER_MUTE=%d;"
            "MIC_VOLUME=%d;"
            "MIC_MUTE=%d;"
            "FILTER_LOW_INDEX=%d;"
            "FILTER_HIGH_INDEX=%d;"
            "AGC_INDEX=%d;"
            "ALC_VALUE=%d;"
            "NR_VALUE=%d;"
            "NB_STATUS=%d;"
            "NB_THRESHOLD=%d;"
            "NB_PULSE_WIDTH=%d;"
            "COMPRESSION_STATUS=%d;"
            "COMPRESSION_LEVEL=%d;"
            "PANADAPTER_AVERAGE=%d;"
            "PANADAPTER_FILL=%d;"
            "PANADAPTER_LINE=%d;"
            "PANADAPTER_MARKER=%d;"
            "PANADAPTER_BACKGROUND=%d;"
            "CW_PITCH=%d;"
            "PANADAPTER_CURSOR=%d;"
            "PANADAPTER_REFRESH=%d;"
            "PANADAPTER_BASE=%d;"
            "PANADAPTER_GAIN=%d;"
            "AUTO_NOTCH=%d;"
            "FILTER_CW_INDEX=%d;"
            "AUTO_SNAP_STATUS=%d;"
            "AUTO_SNAP_INDEX=%d;"
            "QRP=%d;\n"
            "PANADAPTER_STATUS=%d;"
            "SMETER_STATUS=%d;\n"
            "MICROPHONE_STATUS=%d;\n"
            "WATERFALL_STATUS=%d;\n"
            "WATERFALL_GRID=%d;\n"
            "WATERFALL_PALLET=%d;\n"
            "WATERFALL_DIRECTION=%d;\n"
            "WATERFALL_GAIN=%d;\n"
            "WATERFALL_ZERO=%d;\n"
            "WATERFALL_SPEED=%d;\n"
            "G_Speaker_volume=%d;\n"
            ,
            line_number++,
            User_Controls.Version,
            User_Controls.Speaker_Volume,
            User_Controls.Speaker_Muted,
            User_Controls.Mic_Volume,
            User_Controls.Mic_Muted,
            User_Controls.Filter_Low_index,
            User_Controls.Filter_High_index,
            User_Controls.AGC_Index,
            User_Controls.ALC_Value,
            User_Controls.NR_Value,
            User_Controls.NB,
            User_Controls.NB_Threshold,
            User_Controls.NB_Pulse_Width,
            User_Controls.Compression,
            User_Controls.Compression_Level,
            User_Controls.Panadapter_Average,
            User_Controls.Panadapter_Fill,
            User_Controls.Panadapter_Line,
            User_Controls.Panadapter_Marker,
            User_Controls.Panadapter_Background,
            User_Controls.CW_Pitch,
            User_Controls.Panadapter_Cursor,
            User_Controls.Panadapter_Refresh,
            User_Controls.Panadapter_Base,
            User_Controls.Panadapter_Gain,
            User_Controls.Auto_Notch,
            User_Controls.Filter_CW_Index,
            User_Controls.Auto_Snap,
            User_Controls.Auto_Snap_Index,
            User_Controls.QRP,
            User_Controls.Panadapter_Status,
            User_Controls.Smeter_Status,
            User_Controls.Microphone_status,
            User_Controls.Waterfall_status,
            User_Controls.Waterfall_grid,
            User_Controls.Waterfall_pallet,
            User_Controls.Waterfall_direction,
            User_Controls.Waterfall_gain,
            User_Controls.Waterfall_zero,
            User_Controls.Waterfall_speed,
            G_Speaker_volume);*/
    print_time(0);
    fprintf(G_fp_logfile, "[%d] User_Controls_Update_Init_File . Finished.\n", line_number++);
    return status;
}

void Process_extended(uint8_t command, char *buf) {
    uint8_t t_opcode_data;
    uint8_t update_flag = TRUE;
    //short int *opcode_data;
    //short int s_opcode_data;
    int *op_code_data_32;
    int i_opcode_data;

    op_code_data_32 = (int*) &buf[0];
    memcpy(&i_opcode_data, op_code_data_32, 4);
    t_opcode_data = buf[0];
    switch (command) {
        case CMD_SET_WATERFALL_GAIN:
            User_Controls.Waterfall_gain = i_opcode_data;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Process_extended . CMD_SET_WATERFALL_GAIN . VALUE: %d \n",
                    line_number++, i_opcode_data);
            break;
        case CMD_SET_WATERFALL_DIRECTION:
            User_Controls.Waterfall_direction = t_opcode_data;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Process_extended . CMD_SET_WATERFALL_DIRECTION . VALUE: %d \n",
                    line_number++, t_opcode_data);
            break;
        case CMD_SET_WATERFALL_DISPLAY:
            User_Controls.Waterfall_status = t_opcode_data;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Process_extended . CMD_SET_WATERFALL_DISPLAY . VALUE: %d \n",
                    line_number++, t_opcode_data);
            break;
        case CMD_SET_WATERFALL_GRID:
            User_Controls.Waterfall_grid = t_opcode_data;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Process_extended . CMD_SET_WATERFALL_GRID . VALUE: %d \n",
                    line_number++, t_opcode_data);
            break;
        case CMD_SET_WATERFALL_PALET:
            User_Controls.Waterfall_pallet = t_opcode_data;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Process_extended . CMD_SET_WATERFALL_PALET . VALUE: %d \n",
                    line_number++, t_opcode_data);
            break;
        case CMD_SET_WATERFALL_ZERO:
            User_Controls.Waterfall_zero = i_opcode_data;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Process_extended . CMD_SET_WATERFALL_ZERO . VALUE: %d \n",
                    line_number++, i_opcode_data);
            break;
        case CMD_SET_WATERFALL_SPEED:
            User_Controls.Waterfall_speed = i_opcode_data;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Process_extended . CMD_SET_WATERFALL_SPEED . VALUE: %d \n",
                    line_number++, i_opcode_data);
            break;
        default:
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Process_extended . Invalid Command . VALUE: %d \n",
                    line_number++, i_opcode_data);
            update_flag = FALSE;
            break;
    }
    if (update_flag == TRUE) {
        User_Controls_Update_Init_File();
    }
}

int User_Controls_Process(uint8_t command, char *buf, byte extened) {
    int status = 0;
    uint8_t cw_pitch = 0;
    uint8_t update_flag = 1;
    uint8_t pitch_valid = TRUE;

    G_process_user_control_status = TRUE;
    opcode_data_8_bit = (uint8_t) (buf[1]);
    t_opcode_data = (uint8_t) (buf[1]);
    opcode_data = (short int*) &buf[1];
    memcpy(&s_opcode_data, opcode_data, 2);

    if (extened == FALSE) {
        switch (command) {
            case CMD_SET_MAIN_MODE:
                User_Controls.Mode = opcode_data_8_bit;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process. CMD_SET_MAIN_MODE: %d \n",
                    line_number++, opcode_data_8_bit);
                break;
            case CMD_SET_AUDIO_DEVICE:
                G_Sound_Device = opcode_data_8_bit;
                User_Controls.Audio_Device = opcode_data_8_bit;
                SDRcore_recv_send_param(CMD_SET_AUDIO_DEVICE, opcode_data_8_bit);
                SDRcore_trans_send_param(CMD_SET_AUDIO_DEVICE, opcode_data_8_bit);
                if (User_Controls.Audio_Device == DIGITAL_SOUND_DEVICE) {
                    SDRcore_trans_send_param(CMD_SET_MIC_VOLUME, User_Controls.Digital_Mic_Gain);
                    SDRcore_recv_send_param(CMD_SET_SPEAKER_VOLUME, User_Controls.Digital_Volume_Level);
                }
                else {
                    /* Phones (1) or Remote (2): Phones levels */
                    SDRcore_trans_send_param(CMD_SET_MIC_VOLUME, User_Controls.Phones_Mic_Gain);
                    SDRcore_recv_send_param(CMD_SET_SPEAKER_VOLUME, User_Controls.Phones_Volume_Level);
                }
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process. CMD_SET_AUDIO_DEVICE: %d \n",
                    line_number++, opcode_data_8_bit);
                //status = Manage_Sound_Devices(command, opcode_data_8_bit);
                break;

            case CMD_SET_PHONES_VOLUME_LEVEL:
                User_Controls.Phones_Volume_Level = opcode_data_8_bit;
                if (User_Controls.Audio_Device == PHONES_SOUND_DEVICE ||
                    User_Controls.Audio_Device == REMOTE_SOUND_DEVICE) {
                    SDRcore_recv_send_param(CMD_SET_SPEAKER_VOLUME, User_Controls.Phones_Volume_Level);
                }
                update_flag = FALSE;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process. CMD_SET_PHONES_VOLUME_LEVEL: %d \n",
                    line_number++, opcode_data_8_bit);
                break;

            case CMD_SET_PHONES_MIC_GAIN_LEVEL:
                User_Controls.Phones_Mic_Gain = opcode_data_8_bit;
                if (User_Controls.Audio_Device == PHONES_SOUND_DEVICE ||
                    User_Controls.Audio_Device == REMOTE_SOUND_DEVICE) {
                    SDRcore_trans_send_param(CMD_SET_MIC_VOLUME, User_Controls.Phones_Mic_Gain);
                }
                update_flag = FALSE;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process. CMD_SET_PHONES_MIC_GAIN_LEVEL: %d \n",
                    line_number++, opcode_data_8_bit);
                break;

            case CMD_SET_DIGITAL_VOLUME_LEVEL:
                User_Controls.Digital_Volume_Level = opcode_data_8_bit;
                if (User_Controls.Audio_Device == DIGITAL_SOUND_DEVICE) {
                    SDRcore_recv_send_param(CMD_SET_SPEAKER_VOLUME, User_Controls.Digital_Volume_Level);
                }
                update_flag = FALSE;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process. CMD_SET_DIGITAL_VOLUME_LEVEL: %d \n",
                    line_number++, opcode_data_8_bit);
                break;

            case CMD_SET_DIGITAL_MIC_GAIN_LEVEL:
                User_Controls.Digital_Mic_Gain = opcode_data_8_bit;
                if (User_Controls.Audio_Device == DIGITAL_SOUND_DEVICE) {
                    SDRcore_trans_send_param(CMD_SET_MIC_VOLUME, User_Controls.Digital_Mic_Gain);
                }
                update_flag = FALSE;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process. CMD_SET_DIGITAL_MIC_GAIN_LEVEL: %d \n",
                    line_number++, opcode_data_8_bit);
                break;

            case CMD_SET_VOLUME_ATTN:
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_VOLUME_ATTN . VALUE: %d \n",
                        line_number++, t_opcode_data);
                SDRcore_recv_send_param(CMD_SET_VOLUME_ATTN, t_opcode_data);
                break;

            case CMD_SET_DIGITAL_VOLUME_ATTN:
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_VOLUME_ATTN . VALUE: %d \n",
                    line_number++, t_opcode_data);
                SDRcore_recv_send_param(CMD_SET_DIGITAL_VOLUME_ATTN, t_opcode_data);
                update_flag = FALSE;
                break;

            case CMD_SET_MIC_GAIN:
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_MIC_GAIN . VALUE: %d \n",
                        line_number++, t_opcode_data);
                SDRcore_trans_send_param(CMD_SET_MIC_GAIN, t_opcode_data);
                break;

            case CMD_SET_DIGITAL_MIC_GAIN:
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_DIGITAL_MIC_GAIN . VALUE: %d \n",
                    line_number++, t_opcode_data);
                SDRcore_trans_send_param(CMD_SET_DIGITAL_MIC_GAIN, t_opcode_data);
                break;

            case CMD_SET_MICROPHONE_STATUS:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_MICROPHONE_STATUS . VALUE: %d \n",
                        line_number++, t_opcode_data);
                User_Controls.Microphone_status = t_opcode_data;
                SDRcore_trans_send_param(CMD_SET_MICROPHONE_STATUS, t_opcode_data);
                break;

            case CMD_SET_SMETER_DISPLAY:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_SMETER_DISPLAY . VALUE: %d \n",
                        line_number++, t_opcode_data);
                User_Controls.Smeter_Status = t_opcode_data;
                break;

            case CMD_SET_PANADAPTER_DISPLAY:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_PANADAPTER_DISPLAY . VALUE: %d \n",
                        line_number++, t_opcode_data);
                User_Controls.Panadapter_Status = t_opcode_data;
                break;

            case CMD_SET_PA_BYPASS:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_PA_BYPASS . VALUE: %d \n",
                        line_number++, t_opcode_data);
                User_Controls.QRP = t_opcode_data;
                G_QRP = t_opcode_data; //if G_QRP == 1, mode is QRO
                Radio_send_parameters(CMD_SET_PA_BYPASS, t_opcode_data, 1);
                Sleep(100);
                SDRcore_trans_send_param(CMD_SET_PA_BYPASS, t_opcode_data);
                break;

            case CMD_SET_AUTO_SNAP_STATUS:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_AUTO_SNAP_STATUS . VALUE: %d \n",
                        line_number++, t_opcode_data);
                User_Controls.Auto_Snap = t_opcode_data;
                break;

            case CMD_SET_AUTO_SNAP_INDEX:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_AUTO_SNAP_INDEX . VALUE: %d \n",
                        line_number++, t_opcode_data);
                User_Controls.Auto_Snap_Index = t_opcode_data;
                break;

            case CMD_GET_SET_AGC:
                User_Controls.AGC_Index = t_opcode_data;
                SDRcore_recv_send_param(CMD_GET_SET_AGC, User_Controls.AGC_Index);
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_AGC . VALUE: %d \n", line_number++, User_Controls.AGC_Index);
                break;

            case CMD_SET_AGC_FAST_LEVEL:
                User_Controls.ALC_Value = s_opcode_data;
                SDRcore_recv_send_param(CMD_SET_AGC_FAST_LEVEL, s_opcode_data);
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_AGC_FAST_LEVEL . VALUE: %d \n",
                        line_number++, s_opcode_data);
                break;

            case CMD_SET_TWO_TONE:
                SDRcore_trans_send_param(CMD_SET_TWO_TONE, t_opcode_data);
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_TWO_TONE . VALUE: %d \n", line_number++, t_opcode_data);
                update_flag = 0;
                break;

            case CMD_GET_SET_NB_ENABLE:
                {
                    uint8_t prev = User_Controls.NB;
                    SDRcore_recv_send_param(CMD_GET_SET_NB_ENABLE, t_opcode_data);
                    User_Controls.NB = t_opcode_data;
                    print_time(1);
                    if (prev != t_opcode_data) {
                        fprintf(G_fp_logfile,
                            "[%d] User_Controls_Process . CMD_GET_SET_NB_ENABLE CHANGED %d -> %d (%s)\n",
                            line_number++, (int)prev, (int)t_opcode_data,
                            t_opcode_data ? "on" : "off");
                    } else {
                        fprintf(G_fp_logfile,
                            "[%d] User_Controls_Process . CMD_GET_SET_NB_ENABLE . NB=%d (%s)\n",
                            line_number++, (int)t_opcode_data,
                            t_opcode_data ? "on" : "off");
                    }
                }
                break;

            case CMD_GET_SET_NB_THRESHOLD:
                {
                    int prev = User_Controls.NB_Threshold;
                    SDRcore_recv_send_param(CMD_GET_SET_NB_THRESHOLD, s_opcode_data);
                    User_Controls.NB_Threshold = s_opcode_data;
                    print_time(1);
                    if (prev != s_opcode_data) {
                        fprintf(G_fp_logfile,
                            "[%d] User_Controls_Process . CMD_GET_SET_NB_THRESHOLD CHANGED %d -> %d\n",
                            line_number++, prev, (int)s_opcode_data);
                    } else {
                        fprintf(G_fp_logfile,
                            "[%d] User_Controls_Process . CMD_GET_SET_NB_THRESHOLD . value=%d\n",
                            line_number++, (int)s_opcode_data);
                    }
                }
                break;

            case CMD_GET_SET_NB_PULSE_WIDTH:
                {
                    int prev = User_Controls.NB_Pulse_Width;
                    SDRcore_recv_send_param(CMD_GET_SET_NB_PULSE_WIDTH, s_opcode_data);
                    User_Controls.NB_Pulse_Width = s_opcode_data;
                    print_time(1);
                    if (prev != s_opcode_data) {
                        fprintf(G_fp_logfile,
                            "[%d] User_Controls_Process . CMD_GET_SET_NB_PULSE_WIDTH CHANGED %d -> %d\n",
                            line_number++, prev, (int)s_opcode_data);
                    } else {
                        fprintf(G_fp_logfile,
                            "[%d] User_Controls_Process . CMD_GET_SET_NB_PULSE_WIDTH . value=%d\n",
                            line_number++, (int)s_opcode_data);
                    }
                }
                break;

            case CMD_SET_MIC_VOLUME:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_MIC_VOLUME . VALUE: %d \n", line_number++, opcode_data_8_bit);
                SDRcore_trans_send_param(CMD_SET_MIC_VOLUME, opcode_data_8_bit);
                User_Controls.Mic_Volume = opcode_data_8_bit;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_MIC_VOLUME . Finished \n", line_number++);
                break;

            case CMD_SET_SPEAKER_MUTE:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_SPEAKER_MUTE . MUTE: %d \n", line_number++, opcode_data_8_bit);
                SDRcore_recv_send_param(CMD_SET_SPEAKER_MUTE, opcode_data_8_bit);
                User_Controls.Speaker_Muted = opcode_data_8_bit;
                G_Speaker_Mute_Status = opcode_data_8_bit;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_SPEAKER_MUTE . Finished \n", line_number++);
                break;

            case CMD_SET_SPEAKER_VOLUME:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_SPEAKER_VOLUME . VALUE: %d \n", line_number++, opcode_data_8_bit);
                SDRcore_recv_send_param(CMD_SET_SPEAKER_VOLUME, opcode_data_8_bit);
                User_Controls.Speaker_Volume = opcode_data_8_bit;
                G_Speaker_volume = opcode_data_8_bit;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_SPEAKER_VOLUME . Finished .  G_Speaker_volume: %d\n",
                        line_number++, G_Speaker_volume);
                break;

            case CMD_SET_MIC_MUTE:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_MIC_MUTE . MUTE: %d \n", line_number++, opcode_data_8_bit);
                SDRcore_trans_send_param(CMD_SET_MIC_MUTE, opcode_data_8_bit);
                User_Controls.Mic_Muted = opcode_data_8_bit;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_MIC_MUTE . Finished \n", line_number++);
                break;

            case CMD_GET_SET_MIC_DEVICE:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_MIC_DEVICE . sdrcore_trans.ini Record Number: %d \n", line_number++, opcode_data_8_bit);
                SDRcore_trans_send_param(CMD_GET_SET_MIC_DEVICE, opcode_data_8_bit);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_MIC_DEVICE . Finished \n", line_number++);
                break;

            case CMD_GET_SET_SPEAKER_DEVICE:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_SPEAKER_DEVICE . sdrcore_trans.ini Record Number: %d \n", line_number++, opcode_data_8_bit);
                SDRcore_recv_send_param(CMD_GET_SET_SPEAKER_DEVICE, opcode_data_8_bit);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_SPEAKER_DEVICE . Finished \n", line_number++);
                break;

            case CMD_SET_TX_HICUT:
                SDRcore_trans_send_param(CMD_SET_TX_HICUT, opcode_data_8_bit);
                SDRcore_recv_send_param(CMD_SET_TX_HICUT, opcode_data_8_bit);
                //Spectrum_Waterfall_send_param(CMD_SET_TX_HICUT, opcode_data_8_bit);
                User_Controls.TX_hi_cut = opcode_data_8_bit;
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_TX_HICUT: %d \n", line_number++,
                        opcode_data_8_bit);
                break;

            case CMD_SET_BW_LOCUT:
                SDRcore_recv_send_param(CMD_SET_BW_LOCUT, opcode_data_8_bit);
                //Spectrum_Waterfall_send_param(CMD_SET_BW_LOCUT, opcode_data_8_bit);
                User_Controls.Filter_Low_index = opcode_data_8_bit;
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . Command_Interface . CMD_SET_BW_LOCUT: %d \n",
                        line_number++, opcode_data_8_bit);
                break;

            case CMD_SET_BW_HICUT:
                //Spectrum_Waterfall_send_param(CMD_SET_BW_HICUT, opcode_data_8_bit);
                SDRcore_recv_send_param(CMD_SET_BW_HICUT, opcode_data_8_bit);
                User_Controls.Filter_High_index = opcode_data_8_bit;
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . Command_Interface . CMD_SET_BW_HICUT: %d \n",
                        line_number++, opcode_data_8_bit);
                break;

            case CMD_SET_CW_BW:
                SDRcore_recv_send_param(CMD_SET_CW_BW, opcode_data_8_bit);
                User_Controls.Filter_CW_Index = opcode_data_8_bit;
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . Command_Interface . CMD_SET_CW_BW: %d \n",
                        line_number++, opcode_data_8_bit);
                break;

            case CMD_SET_BW_LOCUT_DEFAULT:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . Command_Interface . CMD_SET_BW_LOCUT_DEFAULT Called \n", line_number++);
                User_Controls.Filter_Low_index = opcode_data_8_bit;
                break;

            case CMD_SET_BW_HICUT_DEFAULT:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . Command_Interface . CMD_SET_BW_HICUT_DEFAULT Called \n", line_number++);
                User_Controls.Filter_High_index = opcode_data_8_bit;
                break;

            case CMD_SET_CW_BW_DEFAULT:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . Command_Interface . CMD_SET_CW_BW_DEFAULT Called \n", line_number++);
                User_Controls.Filter_CW_Index = opcode_data_8_bit;
                break;

            case CMD_SET_COMPRESSION_STATE:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_COMPRESSION_STATE . Compression State: %d \n", line_number++, opcode_data_8_bit);
                SDRcore_trans_send_param(CMD_SET_COMPRESSION_STATE, opcode_data_8_bit);
                User_Controls.Compression = opcode_data_8_bit;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_COMPRESSION_STATE . Finished \n", line_number++);
                break;

            case CMD_SET_COMPRESSION_LEVEL:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_COMPRESSION_LEVEL . Compression Level: %d \n", line_number++, opcode_data_8_bit);
                SDRcore_trans_send_param(CMD_SET_COMPRESSION_LEVEL, opcode_data_8_bit);
                User_Controls.Compression_Level = opcode_data_8_bit;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_SET_COMPRESSION_LEVEL . Finished \n", line_number++);
                break;

            case CMD_GET_SET_AUTO_NOTCH:
                print_time(1);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CDM_GET_SET_AUTO_NOTCH . Auto Notch State: %d \n", line_number++,
                        opcode_data_8_bit);
                SDRcore_recv_send_param(CMD_GET_SET_AUTO_NOTCH, opcode_data_8_bit);
                User_Controls.Auto_Notch = opcode_data_8_bit;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_AUTO_NOTCH . Finished \n", line_number++);
                break;

            case CMD_SET_NR:
                /*
                 * 0 = off; non-zero = on with level (byte payload matches live client path).
                 * Persist NR_VALUE so appliance start can re-apply to sdrcore-recv.
                 */
                {
                    uint8_t nr = t_opcode_data;
                    uint8_t prev = User_Controls.NR_Value;
                    User_Controls.NR_Value = nr;
                    SDRcore_recv_send_param(CMD_SET_NR, nr);
                    print_time(1);
                    if (prev != nr) {
                        fprintf(G_fp_logfile,
                            "[%d] User_Controls_Process . CMD_SET_NR CHANGED %d -> %d (%s) - saved to user_controls.ini\n",
                            line_number++, (int)prev, (int)nr,
                            nr == 0 ? "off" : "on");
                    } else {
                        fprintf(G_fp_logfile,
                            "[%d] User_Controls_Process . CMD_SET_NR . NR_VALUE=%d (%s) — unchanged\n",
                            line_number++, (int)nr,
                            nr == 0 ? "off" : "on");
                    }
                }
                break;

            case CMD_SET_CW_PITCH:
                cw_pitch = opcode_data_8_bit;
                switch (cw_pitch) {
                    case 0:
                        G_CW_Offset = -400;
                        break;
                    case 1:
                        G_CW_Offset = -600;
                        break;
                    case 2:
                        G_CW_Offset = -800;
                        break;
                    case 3:
                        G_CW_Offset = -1000;
                        break;
                    default:
                        pitch_valid = FALSE;
                }
                if (pitch_valid == TRUE) {
                    print_time(1);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Process. CMD_SET_CW_PITCH\n", line_number++);
                    Radio_send_parameters(CMD_SET_TRANSCEIVER_CW_PITCH, cw_pitch, 1);
                    if (cw_record.keyer_Installed == TRUE) {
                        Radio_send_parameters(SET_SIDE_TONE, cw_pitch, 1);
                    }
                    Sleep(10);
                    freq_queue_add(G_tune_freq);
                    SDRcore_recv_send_param(CMD_SET_CW_PITCH, cw_pitch);
                    //Spectrum_Waterfall_send_param(CMD_SET_CW_PITCH, cw_pitch);
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Process. CMD_SET_CW_PITCH. G_cw_pitch: %d, Index: %d\n", line_number++,
                            G_CW_Offset, cw_pitch);
                    User_Controls.CW_Pitch = cw_pitch;
                } else {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] User_Controls_Process. CMD_SET_CW_PITCH INVALID cw_pitch: %d\n", line_number++,
                            cw_pitch);
                }
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process. CMD_SET_CW_PITCH FINISHED\n", line_number++);
                break;

            case CMD_GET_SET_PANADAPTER_SMOOTHING:
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_PANADAPTER_SMOOTHING: %d\n", line_number++, t_opcode_data);
                User_Controls.Panadapter_Average = t_opcode_data;
                SDRcore_recv_send_param(CMD_GET_SET_PANADAPTER_SMOOTHING, (t_opcode_data));
                break;

            case CMD_GET_SET_PANADAPTER_GAIN:
                User_Controls.Panadapter_Gain = s_opcode_data;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_PANADAPTER_GAIN: %d\n", line_number++, s_opcode_data);
                break;

            case CMD_GET_SET_PANADAPTER_BASE:
                User_Controls.Panadapter_Base = s_opcode_data;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_PANADAPTER_BASE: %d\n", line_number++, s_opcode_data);
                break;

            case CMD_GET_SET_PANADAPTER_REFRESH:
                /* Blocks between panReady. 0 never equals panblocks++ → silent no spectrum. */
                if (t_opcode_data < 1) {
                    print_time(0);
                    fprintf(G_fp_logfile,
                        "[%d] User_Controls_Process . CMD_GET_SET_PANADAPTER_REFRESH: %d invalid — using 6\n",
                        line_number++, t_opcode_data);
                    t_opcode_data = 6;
                }
                SDRcore_recv_send_param(CMD_GET_SET_PANADAPTER_REFRESH, t_opcode_data);
                User_Controls.Panadapter_Refresh = t_opcode_data;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_PANADAPTER_REFRESH: %d\n", line_number++, t_opcode_data);
                break;

            case CMD_GET_SET_PANADAPTER_FILL:
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_PANADAPTER_FILL: %d\n", line_number++, t_opcode_data);
                User_Controls.Panadapter_Fill = t_opcode_data;
                break;

            case CMD_GET_SET_PANADAPTER_LINE:
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_PANADAPTER_LINE: %d\n", line_number++, t_opcode_data);
                User_Controls.Panadapter_Line = t_opcode_data;
                break;

            case CMD_GET_SET_PANADAPTER_MARKER:
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_PANADAPTER_MARKER: %d\n", line_number++, t_opcode_data);
                User_Controls.Panadapter_Marker = t_opcode_data;
                break;

            case CMD_GET_SET_PANADAPTER_BACKGROUND:
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_PANADAPTER_BACKGROUND: %d\n", line_number++, t_opcode_data);
                User_Controls.Panadapter_Background = t_opcode_data;
                break;

            case CMD_GET_SET_PANADAPTER_CURSOR:
                print_time(0);
                fprintf(G_fp_logfile, "[%d] User_Controls_Process . CMD_GET_SET_PANADAPTER_CURSOR: %d\n", line_number++, t_opcode_data);
                User_Controls.Panadapter_Cursor = t_opcode_data;
                break;

        }
        if (update_flag) {
            User_Controls_Update_Init_File();
        }
    } else {
        Process_extended(command, buf);
    }
    G_process_user_control_status = FALSE;
    return status;
}

int Write_User_Controls() {
    int status = 0;
    FILE *fp_User_ini;
    char file_name[PATH_MAX] = {0};

    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Write_User_Controls . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/user_controls.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Write_User_Controls . user_controls.ini Path: %s\n", line_number++, homedir);
        fp_User_ini = fopen(file_name, "w");
        if (fp_User_ini != NULL) {
            fprintf(fp_User_ini,
                    "VERSION=%d;\n"
                    "SPEAKER_VOLUME=%d;\n"
                    "SPEAKER_MUTE=%d;\n"
                    "MIC_VOLUME=%d;\n"
                    "MIC_MUTE=%d; \n"
                    "FILTER_LOW_INDEX=%d;\n"
                    "FILTER_HIGH_INDEX=%d;\n"
                    "AGC_INDEX=%d;\n"
                    "ALC_VALUE=%d;\n"
                    "NR_VALUE=%d;\n"
                    "NB_STATUS=%d;\n"
                    "NB_THRESHOLD=%d;\n"
                    "NB_PULSE_WIDTH=%d;\n"
                    "COMPRESSION_STATUS=%d;\n"
                    "COMPRESSION_LEVEL=%d;\n"
                    "PANADAPTER_AVERAGE=%d;\n"
                    "PANADAPTER_FILL=%d;\n"
                    "PANADAPTER_LINE=%d;\n"
                    "PANADAPTER_MARKER=%d;\n"
                    "PANADAPTER_BACKGROUND=%d;\n"
                    "CW_PITCH=%d;\n"
                    "PANADAPTER_CURSOR=%d;\n"
                    "PANADAPTER_REFRESH=%d;\n"
                    "PANADAPTER_BASE=%d;\n"
                    "PANADAPTER_GAIN=%d;\n"
                    "AUTO_NOTCH=%d;\n"
                    "FILTER_CW_INDEX=%d;\n"
                    "AUTO_SNAP_STATUS=%d;\n"
                    "AUTO_SNAP_INDEX=%d;\n"
                    "QRP=%d;\n"
                    "PANADAPTER_STATUS=%d;\n"
                    "SMETER_STATUS=%d;\n"
                    "MICROPHONE_STATUS=%d;\n"
                    "WATERFALL_STATUS=%d;\n"
                    "WATERFALL_GRID=%d;\n"
                    "WATERFALL_PALLET=%d;\n"
                    "WATERFALL_DIRECTION=%d;\n"
                    "WATERFALL_GAIN=%d;\n"
                    "WATERFALL_ZERO=%d;\n"
                    "WATERFALL_SPEED=%d;\n",
                    User_Controls.Version = VERSION,
                    User_Controls.Speaker_Volume = 0,
                    User_Controls.Speaker_Muted = 0,
                    User_Controls.Mic_Volume = 0,
                    User_Controls.Mic_Muted = 0,
                    User_Controls.Filter_Low_index = 0,
                    User_Controls.Filter_High_index = 0,
                    User_Controls.AGC_Index = 0,
                    User_Controls.ALC_Value = 1,
                    User_Controls.NR_Value = 0,
                    User_Controls.NB = 0,
                    User_Controls.NB_Threshold = 10,
                    User_Controls.NB_Pulse_Width = 30,
                    User_Controls.Compression = 0,
                    User_Controls.Compression_Level = 0,
                    User_Controls.Panadapter_Average = 0,
                    User_Controls.Panadapter_Fill = 0,
                    User_Controls.Panadapter_Line = 0,
                    User_Controls.Panadapter_Marker = 0,
                    User_Controls.Panadapter_Background = 0,
                    User_Controls.CW_Pitch = 2,
                    User_Controls.Panadapter_Cursor = 0,
                    User_Controls.Panadapter_Refresh = 6,
                    User_Controls.Panadapter_Base = 0,
                    User_Controls.Panadapter_Gain = 0,
                    User_Controls.Auto_Notch = 0,
                    User_Controls.Filter_CW_Index = 0,
                    User_Controls.Auto_Snap = 0,
                    User_Controls.Auto_Snap_Index = 0,
                    User_Controls.QRP = 0,
                    User_Controls.Panadapter_Status = 0,
                    User_Controls.Smeter_Status = 0,
                    User_Controls.Microphone_status = 1,
                    User_Controls.Waterfall_status = 0,
                    User_Controls.Waterfall_grid = 0,
                    User_Controls.Waterfall_pallet = 0,
                    User_Controls.Waterfall_direction = 0,
                    User_Controls.Waterfall_gain = 2891,
                    User_Controls.Waterfall_zero = 410,
                    User_Controls.Waterfall_speed = 1),
                    fclose(fp_User_ini);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Write_User_Controls . Create user_controls.ini . FAILED: %s \n", line_number++, homedir);
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Write_User_Controls . SHGetKnownFolderPath FAILED \n", line_number++);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Write_User_Controls . Finished \n", line_number++);
    return status;
}

int Create_User_Controls(int force) {
    int status = 0;
    FILE *fp_User_ini;
    char file_name[PATH_MAX] = {0};

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Create_User_Controls . Called.\n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_User_Controls . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/user_controls.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_User_Controls . user_controls.ini Path: %s\n", line_number++, homedir);
        if (!force) {
            fp_User_ini = fopen(file_name, "r");
            if (fp_User_ini == NULL) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Create_User_Controls . Does not exist.. Creating user_controls.ini. Force: %d\n", line_number++, force);
                Write_User_Controls();
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Create_User_Controls . user_controls exists. Not creating user_controls.ini. Force: %d\n", line_number++
                        , force);
                fclose(fp_User_ini);
            }
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Create_User_Controls . force: %d. Forcing create of user_controls.ini\n", line_number++, force);
            Write_User_Controls();
        }
    }
    return status;
}



