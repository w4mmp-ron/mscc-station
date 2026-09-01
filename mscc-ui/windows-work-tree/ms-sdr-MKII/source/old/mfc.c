
//#define TRUE 1
//#define FALSE 0
#define true 1
#define false 0
#include "extern.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include </usr/include/ftd2xx.h>
#include "usbavrcmd.h"

typedef unsigned char byte;
typedef signed char sbyte;
typedef signed char bool;
typedef int BOOL;

#define NO_MFC_DEVICE -1
#define LOOP_LIMIT 250

//These are for the rotory 
#define FREQUENCY_FUNCTION 0
#define VOLUME_FUNCTION 1
#define RIT_FUNCTION 2

#define BUTTON_PROCESSING 3
#define BUTTON_FINISHED 4

#define NONE 0
#define  STEP 1
#define  PTT 2
#define  TUNE 3
#define  MODE 4
#define  RIT_MODE 5
#define  BAND 6
#define  FREQ_VOL 7
#define  CW_BW 8
#define  HI_BW 9
#define  RIT 10
#define  FAVS 11

int G_Version = 0;
int G_State = 1;
int G_Rotory_function = FREQUENCY_FUNCTION;
int G_Freq_digit = 1;
int G_Auto_zero = 0;
uint8_t G_Speaker_volume = 50;
BOOL G_process_user_control_status = FALSE;

byte MFC_state = 1;
byte Knob_switch_function = STEP;
const byte Knob_switch = 0x80;

byte Button_left_function = BAND;
const byte Button_left_switch = 0x10;

byte Button_middle_function = TUNE;
const byte Button_middle_switch = 0x08;

byte Button_right_function = FREQ_VOL;
const byte Button_right_switch = 0x04;

byte PTT_switch_function = PTT;
const byte PTT_switch = 0x01;

DWORD VID = 0x0403;
DWORD PID = 0x1008;
DWORD numDevs;
FT_STATUS ftStatus;
FT_HANDLE ftHandle;
DWORD baudRate = 9600;
bool encoder_A = false;
bool encoder_B = false;
sbyte rec_enc_table[] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0};
sbyte previous_switch_table[] = {0, 0, 0, 0, 0};
bool Continue = true;
byte prevNextCode = 0;
uint32_t Freq = 14074000;
uint16_t G_Multiplier = 3;
int multiplier = 1;
int read_rotary_switch_loop_count = LOOP_LIMIT;
const char *thread_name;
static int speaker_volume = 0;

/*Tuning_Knob_Controls.Freq_Function.Active && !Tuning_Knob_Controls.Volume_Function.Volume_active*/

void MFC_Favs_Function()
{
        byte extended_data[4];
        byte fav = 1;

        memcpy(&extended_data[0], &fav, sizeof(fav));
        Gui_send_param_extended(CMD_SET_FAVS, extended_data, sizeof(fav));
}

bool MFC_Band_Function()
{
        bool status = true;
        byte band = 0;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s -> %s -> G_band_normal: %d\n", line_number++, thread_name, __func__, G_band_normal);
        switch (G_band_normal) {
        case 160:
                band = 80;
                break;

        case 80:
                band = 60;
                break;

        case 60:
                band = 40;
                break;

        case 40:
                band = 30;
                break;

        case 30:
                band = 20;
                break;

        case 20:
                band = 17;
                break;

        case 17:
                band = 15;
                break;

        case 15:
                band = 12;
                break;

        case 12:
                band = 10;
                break;

        case 10:
                band = 200;
                break;

        default:
                band = 160;
                break;
        }
        G_band_normal = band;
        Gui_send_param(CMD_GET_SET_STARTUP_BAND, G_band_normal);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s -> %s -> G_band_normal: %d\n", line_number++, thread_name, __func__, G_band_normal);
        return status;
}

void Zero_Frequency()
{
        int32_t remainder = 0;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s -> %s -> G_tune_freq: %d, G_Freq_digit: %d\n", line_number++, thread_name, __func__, G_tune_freq, G_Freq_digit);
        /* switch ((G_Freq_digit)) {
         case 7: //10 Millions
                 remainder = G_tune_freq % 10000000;
                 break;
         case 6: //Millions
                 remainder = G_tune_freq % 1000000;
                 break;
         case 5:
                 remainder = G_tune_freq % 100000;
                 break;
         case 4:
                 remainder = G_tune_freq % 10000;
                 break;
         case 3:
                 remainder = G_tune_freq % 1000;
                 break;
         case 2:
                 remainder = G_tune_freq % 100;
                 break;
         case 1:
                 remainder = G_tune_freq % 10;
                 break;
         default:
                 print_time(0);
                 fprintf(G_fp_logfile, "[%d] %s -> %s -> INVALID G_Freq_digit: %d\n", line_number++, thread_name, __func__, G_Freq_digit);
                 break;
         }
         G_tune_freq = G_tune_freq - remainder;
         */
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s -> %s -> G_tune_freq: %d, G_Freq_digit: %d\n", line_number++, thread_name, __func__, G_tune_freq, G_Freq_digit);
}

void Set_MFC_Increment(int set_by_button)
{
        static uint16_t previous_multiplier = 10;

        if (set_by_button == TRUE) {
                G_Multiplier++;
        }
        if (G_Multiplier > 5) {
                G_Multiplier = 0;
        }
        if (previous_multiplier != G_Multiplier) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s -> %s -> G_Multiplier: %d\n", line_number++, thread_name, __func__, G_Multiplier);
                switch (G_Multiplier) {
                case 0:
                        multiplier = 100000;
                        break;
                case 1:
                        multiplier = 10000;
                        break;
                case 2:
                        multiplier = 1000;
                        break;
                case 3:
                        multiplier = 100;
                        break;
                case 4:
                        multiplier = 10;
                        break;
                case 5:
                        multiplier = 1;
                        break;
                default:
                        multiplier = 1000000;
                        break;
                }
                Gui_send_param(CMD_SET_STEP_VALUE, G_Multiplier);
                previous_multiplier = G_Multiplier;
        }
}

int Initialize_MFC()
{
        int status = TRUE;
        FILE *MFC_ini;
        char file_name[PATH_MAX] = {0};
        const char *homedir;
        char init_record[300];
        char *record;
        INT8 mynumber;

        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_MFC -> Called.\n", line_number++);
        if ((homedir = getenv("HOME")) != NULL) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Initialize_MFC -> Default Path: %s\n", line_number++, homedir);
                strcpy(file_name, homedir);
                strcat(file_name, "/.local/share/mscc/mfc_controller.ini");
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Initialize_MFC -> Filename: %s\n", line_number++, file_name);
                MFC_ini = fopen(file_name, "r");
                if (MFC_ini != NULL) {
                        while (fgets(init_record, sizeof(init_record), MFC_ini) != NULL) {
                                record = strstr(init_record, "VERSION");
                                if (record != NULL) {
                                        mynumber = atoi((record + sizeof("VERSION")));
                                        G_Version = mynumber;
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] Initialize_MFC -> VERSION: %d\n", line_number++, G_Version);
                                }
                                record = strstr(init_record, "STATE");
                                if (record != NULL) {
                                        mynumber = atoi((record + sizeof("STATE")));
                                        G_State = mynumber;
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] Initialize_MFC -> STATE: %d\n", line_number++, G_State);
                                }
                                record = strstr(init_record, "KNOB_SWITCH");
                                if (record != NULL) {
                                        mynumber = atoi((record + sizeof("KNOB_SWITCH")));
                                        Knob_switch_function = mynumber;
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] Initialize_MFC -> KNOB_SWITCH: %d\n", line_number++, Knob_switch_function);
                                }
                                record = strstr(init_record, "LEFT_SWITCH");
                                if (record != NULL) {
                                        mynumber = atoi((record + sizeof("LEFT_SWITCH")));
                                        Button_left_function = mynumber;
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] Initialize_MFC -> LEFT_SWITCH: %d\n", line_number++, Button_left_function);
                                }
                                record = strstr(init_record, "MIDDLE_SWITCH");
                                if (record != NULL) {
                                        mynumber = atoi((record + sizeof("MIDDLE_SWITCH")));
                                        Button_middle_function = mynumber;
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] Initialize_MFC -> MIDDLE_SWITCH: %d\n", line_number++, Button_middle_function);
                                }
                                record = strstr(init_record, "RIGHT_SWITCH");
                                if (record != NULL) {
                                        mynumber = atoi((record + sizeof("RIGHT_SWITCH")));
                                        Button_right_function = mynumber;
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] Initialize_MFC -> RIGHT_SWITCH: %d\n", line_number++, Button_right_function);
                                }
                                record = strstr(init_record, "PTT_SWITCH");
                                if (record != NULL) {
                                        mynumber = atoi((record + sizeof("PTT_SWITCH")));
                                        PTT_switch_function = mynumber;
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] Initialize_MFC -> PTT_SWITCH: %d\n", line_number++, PTT_switch_function);
                                }
                        }
                        fclose(MFC_ini);
                } else {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Initialize_MFC -> FAILED.\n", line_number++);
                        fflush(G_fp_logfile);
                        status = FALSE;
                }
        }
        return status;
}

int Update_MFC_ini()
{
        int status = TRUE;
        FILE *MFC_ini;
        char file_name[PATH_MAX] = {0};
        const char *homedir;

        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_MFC_ini -> Called.\n", line_number++);
        if ((homedir = getenv("HOME")) != NULL) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Update_MFC_ini -> Default Path: %s\n", line_number++, homedir);
                strcpy(file_name, homedir);
                strcat(file_name, "/.local/share/mscc/mfc_controller.ini");
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Update_MFC_ini -> Filename: %s\n", line_number++, file_name);
                MFC_ini = fopen(file_name, "w");
                if (MFC_ini != NULL) {
                        fprintf(MFC_ini, "VERSION=%d;\n", 1);
                        fprintf(MFC_ini, "STATE=%d;\n", MFC_state);
                        fprintf(MFC_ini, "KNOB_SWITCH=%d;\n", Knob_switch_function);
                        fprintf(MFC_ini, "LEFT_SWITCH=%d;\n", Button_left_function);
                        fprintf(MFC_ini, "MIDDLE_SWITCH=%d;\n", Button_middle_function);
                        fprintf(MFC_ini, "RIGHT_SWITCH=%d;\n", Button_right_function);
                        fprintf(MFC_ini, "PTT_SWITCH=%d;\n", PTT_switch_function);
                        fclose(MFC_ini);
                } else {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Update_MFC_ini -> FAILED.\n", line_number++);
                        status = FALSE;
                }
        }
        return status;
}

int Create_MFC_ini()
{
        int status = TRUE;
        FILE *MFC_ini;
        char file_name[PATH_MAX] = {0};
        const char *homedir;

        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_MFC_ini -> Called.\n", line_number++);
        if ((homedir = getenv("HOME")) != NULL) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Create_MFC_ini -> Default Path: %s\n", line_number++, homedir);
                strcpy(file_name, homedir);
                strcat(file_name, "/.local/share/mscc/mfc_controller.ini");
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Create_MFC_ini ->   Filename: %s\n", line_number++, file_name);
                MFC_ini = fopen(file_name, "r");
                if (MFC_ini == NULL) {
                        MFC_ini = fopen(file_name, "w");
                        if (MFC_ini != NULL) {
                                fprintf(MFC_ini, "VERSION=%d;\n", 1);
                                fprintf(MFC_ini, "STATE=%d;\n", 1);
                                fprintf(MFC_ini, "KNOB_SWITCH=%d;\n", Knob_switch_function);
                                fprintf(MFC_ini, "LEFT_SWITCH=%d;\n", Button_left_function);
                                fprintf(MFC_ini, "MIDDLE_SWITCH=%d;\n", Button_middle_function);
                                fprintf(MFC_ini, "RIGHT_SWITCH=%d;\n", Button_right_function);
                                fprintf(MFC_ini, "PTT_SWITCH=%d;\n", PTT_switch_function);
                                fclose(MFC_ini);
                        } else {
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] Create_MFC_ini ->  FAILED.\n", line_number++);
                                status = FALSE;
                        }
                } else {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Create_MFC_ini ->  Not creating file. \n", line_number++);
                        status = TRUE;
                }
        }
        return status;
}

void Relay_States_to_bool(int switch_A, int switch_B)
{
        switch (switch_A) {
        case 0:
                encoder_A = false;
                break;
        default:
                encoder_A = true;
                break;
        }

        switch (switch_B) {
        case 0:
                encoder_B = false;
                break;
        default:
                encoder_B = true;
                break;
        }
}

sbyte Decode_Rotary()
{
        sbyte status = 0;
        static uint16_t store = 0;

        prevNextCode <<= 2;
        if (encoder_A) {
                prevNextCode |= 0x02;
        }
        if (encoder_B) {
                prevNextCode |= 0x01;
        }
        prevNextCode &= 0x0f;
        if (rec_enc_table[prevNextCode] == 1) {
                store <<= 4;
                store |= prevNextCode;
                if ((store & 0xff) == 0x2b) status = -1;
                if ((store & 0x17) == 0x17) status = 1;
        }
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] %s -> %s -> status: %d\n",
        //        line_number++, thread_name, __func__, status);
        return status;
}

void Select_Rotary_Function(int rotary_value)
{
        int increment = 0;
        uint32_t frequency;

        char speaker_volume_char = 0;

        switch (G_Rotory_function) {
        case FREQUENCY_FUNCTION:
                increment = rotary_value * multiplier;
                frequency = G_tune_freq + increment;
                G_tune_freq = frequency;
                if (G_Auto_zero == 1) {
                        Zero_Frequency();
                }
                freq_queue_add(frequency);
                Gui_send_param(CMD_SET_DISPLAY_FREQ, G_tune_freq);
                break;
        case VOLUME_FUNCTION:
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s -> %s -> START -> rotary_value: %d, speaker_volume: %d, speaker_volume_char: %d\n",
                        line_number++, thread_name, __func__, rotary_value, speaker_volume, speaker_volume_char);
                if (G_process_user_control_status == FALSE) {
                        speaker_volume = speaker_volume + rotary_value;
                        if (speaker_volume < 0) {
                                speaker_volume = 0;
                        } else {
                                if (speaker_volume >= 99) {
                                        speaker_volume = 99;
                                }
                        }
                        speaker_volume_char = speaker_volume;
                        G_Speaker_volume = speaker_volume_char;
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] %s -> %s -> MIDDLE -> rotary_value: %d, speaker_volume: %d, speaker_volume_char: %d\n",
                                line_number++, thread_name, __func__, rotary_value, speaker_volume, speaker_volume_char);
                        SDRcore_recv_send_param(CMD_SET_SPEAKER_VOLUME, speaker_volume_char);
                        //User_Controls_Process(CMD_SET_SPEAKER_VOLUME, &speaker_volume_char, 0);
                        Gui_send_param(CMD_SET_SPEAKER_VOLUME, G_Speaker_volume);
                        print_time(0);
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] %s -> %s -> FINISH -> rotary_value: %d, speaker_volume: %d, speaker_volume_char: %d\n",
                                line_number++, thread_name, __func__, rotary_value, speaker_volume, speaker_volume_char);
                } else {
                        speaker_volume = G_Speaker_volume + rotary_value;
                        fprintf(G_fp_logfile, "[%d] %s -> %s ->  G_process_user_control_status NOT FALSE\n", line_number++);
                }
                break;
        case RIT_FUNCTION:
                break;
        }
}

int Read_Rotary_Switch()
{
        int loop_count = 0;
        sbyte rotary_state = 0;
        UCHAR relay_State = 0xFF;
        UCHAR relay_State_inverted = 0;
        static UCHAR Previous_relay_State = 0xFF;
        int curModeA = 0;
        int curModeB = 0;
        int status = TRUE;
        int static first_pass = TRUE;
        FT_STATUS read_status = FT_OK;

        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] Read_MFC Called \n", line_number++);
        read_status = FT_GetBitMode(ftHandle, &relay_State);
        if (read_status != FT_OK) {
                read_status = FALSE;
        }
        if (status == TRUE) {
                if (Previous_relay_State != relay_State) {
                        while (loop_count++ < read_rotary_switch_loop_count) {
                                curModeA = (relay_State & 32);
                                curModeB = (relay_State & 64);
                                Relay_States_to_bool(curModeA, curModeB);
                                rotary_state = Decode_Rotary();
                                if (rotary_state != 0) {
                                        Select_Rotary_Function(rotary_state);
                                }
                                read_status = FT_GetBitMode(ftHandle, &relay_State);
                                if (read_status != FT_OK) {
                                        status = FALSE;
                                }
                        }
                        loop_count = 0;
                        Previous_relay_State = relay_State;
                        //push_button_pressed = FALSE;
                }
        }
        //Tuning_Knob_Controls.Freq_Function.Active = false;
        return status;
}

int Manage_Knob_Switch()
{
        static bool freq_volume_toggle = FALSE;

        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s -> %s -> %d\n", line_number++, thread_name, __func__);
        switch (Knob_switch_function) {
        case NONE:
                break;
        case STEP:
                Set_MFC_Increment(TRUE);
                break;
        case PTT:
                break;
        case TUNE:
                break;
        case MODE:
                break;
        case RIT_MODE:
                break;
        case BAND:
                MFC_Band_Function();
                break;
        case FREQ_VOL:
                if (freq_volume_toggle == FALSE) {
                        G_Rotory_function = VOLUME_FUNCTION;
                        freq_volume_toggle = TRUE;
                } else {
                        G_Rotory_function = FREQUENCY_FUNCTION;
                        freq_volume_toggle = FALSE;
                }
                break;
        case CW_BW:
                break;
        case HI_BW:
                break;
        case RIT:
                break;
        case FAVS:
                MFC_Favs_Function();
                break;
        }
}

int Manage_Left_Switch()
{
        static bool freq_volume_toggle = FALSE;

        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s -> %s -> Button_left_function: %d\n", line_number++, thread_name, __func__, Button_left_function);
        switch (Button_left_function) {
        case NONE:
                break;
        case STEP:
                Set_MFC_Increment(TRUE);
                break;
        case PTT:
                break;
        case TUNE:
                break;
        case MODE:
                break;
        case RIT_MODE:
                break;
        case BAND:
                MFC_Band_Function();
                break;
        case FREQ_VOL:
                if (freq_volume_toggle == FALSE) {
                        G_Rotory_function = VOLUME_FUNCTION;
                        freq_volume_toggle = TRUE;
                } else {
                        G_Rotory_function = FREQUENCY_FUNCTION;
                        freq_volume_toggle = FALSE;
                }
                break;
        case CW_BW:
                break;
        case HI_BW:
                break;
        case RIT:
                break;
        case FAVS:
                MFC_Favs_Function();
                break;
        }
}

int Manage_Middle_Switch()
{
        static bool freq_volume_toggle = FALSE;

        switch (Button_middle_function) {
        case NONE:
                break;
        case STEP:
                Set_MFC_Increment(TRUE);
                break;
        case PTT:
                break;
        case TUNE:
                break;
        case MODE:
                break;
        case RIT_MODE:
                break;
        case BAND:
                MFC_Band_Function();
                break;
        case FREQ_VOL:
                if (freq_volume_toggle == FALSE) {
                        G_Rotory_function = VOLUME_FUNCTION;
                        freq_volume_toggle = TRUE;
                } else {
                        G_Rotory_function = FREQUENCY_FUNCTION;
                        freq_volume_toggle = FALSE;
                }
                break;
        case CW_BW:
                break;
        case HI_BW:
                break;
        case RIT:
                break;
        case FAVS:
                MFC_Favs_Function();
                break;
        }
}

int Manage_Right_Switch()
{
        static bool freq_volume_toggle = FALSE;

        switch (Button_right_function) {
        case NONE:
                break;
        case STEP:
                Set_MFC_Increment(TRUE);
                break;
        case PTT:
                break;
        case TUNE:
                break;
        case MODE:
                break;
        case RIT_MODE:
                break;
        case BAND:
                MFC_Band_Function();
                break;
        case FREQ_VOL:
                if (freq_volume_toggle == FALSE) {
                        G_Rotory_function = VOLUME_FUNCTION;
                        freq_volume_toggle = TRUE;
                } else {
                        G_Rotory_function = FREQUENCY_FUNCTION;
                        freq_volume_toggle = FALSE;
                }
                break;
        case CW_BW:
                break;
        case HI_BW:
                break;
        case RIT:
                break;
        case FAVS:
                MFC_Favs_Function();
                break;
        }
}

int Manage_PTT_Switch()
{
        static bool freq_volume_toggle = FALSE;

        switch (PTT_switch_function) {
        case NONE:
                break;
        case STEP:
                Set_MFC_Increment(TRUE);
                break;
        case PTT:
                break;
        case TUNE:
                break;
        case MODE:
                break;
        case RIT_MODE:
                break;
        case BAND:
                MFC_Band_Function();
                break;
        case FREQ_VOL:
                if (freq_volume_toggle == FALSE) {
                        G_Rotory_function = VOLUME_FUNCTION;
                        freq_volume_toggle = TRUE;
                } else {
                        G_Rotory_function = FREQUENCY_FUNCTION;
                        freq_volume_toggle = FALSE;
                }
                break;
        case CW_BW:
                break;
        case HI_BW:
                break;
        case RIT:
                break;
        case FAVS:
                MFC_Favs_Function();
                break;
        }
}

int Read_Buttons()
{
        int status = BUTTON_FINISHED;
        UCHAR switch_state = 0;
        UCHAR switch_state_inverted = 0;
        static UCHAR which_switch = 0;
        static UCHAR previous_switch_state = 0xFF;
        FT_STATUS read_status = FT_OK;
        static byte button_down = FALSE;
        static byte button_up = FALSE;

        read_status = FT_GetBitMode(ftHandle, &switch_state);
        if (read_status == FT_OK) {
                if (previous_switch_state != switch_state) {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] %s -> %s -> START -> button_down: %d, button_up: %d \n", line_number++, thread_name, __func__, button_down, button_up);
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] %s -> %s -> switch_state: 0x%02X\n", line_number++, thread_name, __func__, switch_state);
                        switch_state_inverted = ~switch_state; //Switches are active low.  Invert for active high
                        switch_state_inverted = switch_state_inverted & 0x9F; //Remove the rotary switch
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] %s -> %s -> switch_state_inverted: 0x%02X\n", line_number++, thread_name, __func__, switch_state_inverted);
                        if ((switch_state_inverted == Knob_switch)) {
                                which_switch = Knob_switch;
                                button_down = TRUE;
                        } else
                                if ((switch_state_inverted) == Button_left_switch) {
                                which_switch = Button_left_switch;
                                button_down = TRUE;
                        } else
                                if ((switch_state_inverted) == Button_middle_switch) {
                                which_switch = Button_middle_switch;
                                button_down = TRUE;
                        } else
                                if ((switch_state_inverted) == Button_right_switch) {
                                which_switch = Button_right_switch;
                                button_down = TRUE;
                        } else
                                if ((switch_state_inverted) == PTT_switch) {
                                which_switch = PTT_switch;
                                button_down = TRUE;
                        } else {
                                if (button_down == TRUE) {
                                        button_up = TRUE;
                                }
                        }
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] %s -> %s -> FINISH -> button_down: %d, button_up: %d \n",
                                line_number++, thread_name, __func__, button_down, button_up);
                        if (button_down == TRUE) {
                                status = BUTTON_PROCESSING;
                        }
                        if (button_down == TRUE && button_up == TRUE) {
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] %s -> %s -> buttons TRUE \n", line_number++, thread_name, __func__);
                                switch (which_switch) {
                                case Knob_switch:
                                        Manage_Knob_Switch();
                                        break;
                                case Button_left_switch:
                                        Manage_Left_Switch();
                                        break;
                                case Button_middle_switch:
                                        Manage_Middle_Switch();
                                        break;
                                case Button_right_switch:
                                        Manage_Right_Switch();
                                        break;
                                case PTT_switch:
                                        Manage_PTT_Switch();
                                        break;
                                }
                                button_up = FALSE;
                                button_down = FALSE;
                                status = BUTTON_FINISHED;
                        }
                }
                previous_switch_state = switch_state;
        } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s -> %s -> Read FAILED: \n", line_number++, thread_name, __func__);
                status = FALSE;
        }
        return status;
}

int Open_MFC()
{
        int status = TRUE;
        int i;
        FT_DEVICE_LIST_INFO_NODE *devInfo;
        int MFC_device = NO_MFC_DEVICE;

        ftStatus = FT_SetVIDPID(VID, PID);
        if (ftStatus == FT_OK) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s -> %s -> FT_SetVIDPID OK\n", line_number++, thread_name, __func__);
        } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s -> %s -> FT_SetVIDPID FAILED\n", line_number++, thread_name, __func__);
                status = FALSE;
        }
        if (status == TRUE) {
                ftStatus = FT_CreateDeviceInfoList(&numDevs);
                if (ftStatus == FT_OK) {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] %s -> %s -> FT_CreateDeviceInfoList -> Number of devices is %d\n",
                                line_number++, thread_name, __func__, numDevs);
                } else {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] %s -> %s  -> FT_CreateDeviceInfoList FAILED\n", line_number++, thread_name, __func__);
                        status = FALSE;
                }
        }
        if (status == TRUE) {
                if (numDevs > 0) {
                        // allocate storage for list based on numDevs
                        devInfo =
                                (FT_DEVICE_LIST_INFO_NODE*) malloc(sizeof(FT_DEVICE_LIST_INFO_NODE) * numDevs);
                        // get the device information list
                        ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs);
                        if (ftStatus == FT_OK) {
                                for (int i = 0; i < numDevs; i++) {
                                        if (devInfo[i].ID == 67309576) {
                                                MFC_device = i;
                                        }
                                }
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] %s -> %s -> MFC_device = %d\n", line_number++, thread_name, __func__, MFC_device);
                        } else {
                                status = FALSE;
                        }
                }
        }
        if (status == TRUE) {
                if (MFC_device != NO_MFC_DEVICE) {
                        ftStatus = FT_Open(MFC_device, &ftHandle);
                        if (ftStatus == FT_OK) {
                                ftStatus = FT_SetBitMode(ftHandle, 0x00, /* sets all 8 pins as input */FT_BITMODE_ASYNC_BITBANG);
                                if (ftStatus == FT_OK) {
                                        ftStatus = FT_SetBaudRate(ftHandle, baudRate * 16);
                                }
                                if (ftStatus != FT_OK) {
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] %s -> %s -> FT_SetBaudRate FAILED\n", line_number++, thread_name, __func__);
                                        status = FALSE;
                                }
                        } else {
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] %s -> %s -> Open_MFC FAILED\n", line_number++, thread_name, __func__);
                                status = FALSE;
                        }
                        free(devInfo);
                } else {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] %s -> %s -> NO MFC FOUND\n", line_number++, thread_name, __func__);
                        status = FALSE;
                }
        }
        return status;
}

int Process_MFC_Controls(uint8_t command, char *buf)
{
        int status = 0;
        uint8_t t_opcode_data;
        uint8_t update_flag = TRUE;
        int *op_code_data_32;
        int i_opcode_data;

        op_code_data_32 = (int*) &buf[0];
        memcpy(&i_opcode_data, op_code_data_32, 4);
        t_opcode_data = buf[0];

        switch (command) {
        case CMD_SET_KNOB_SWITCH:
                Knob_switch_function = t_opcode_data;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s -> CMD_SET_KNOB_SWITCH:%d \n", line_number++, __func__, Knob_switch_function);
                break;

        case CMD_SET_LEFT_SWITCH:
                Button_left_function = t_opcode_data;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s -> CMD_SET_LEFT_SWITCH %d\n", line_number++, __func__, Button_left_function);
                break;

        case CMD_SET_MIDDLE_SWITCH:
                Button_middle_function = t_opcode_data;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s -> CMD_SET_MIDDLE_SWITCH: %d \n", line_number++, __func__, Button_middle_function);
                break;

        case CMD_SET_RIGHT_SWITCH:
                Button_right_function = t_opcode_data;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s -> CMD_SET_RIGHT_SWITCH %d\n", line_number++, __func__, CMD_SET_RIGHT_SWITCH);
                break;

        case CMD_SET_PTT_SWITCH:
                PTT_switch_function = t_opcode_data;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s -> CMD_SET_PTT_SWITCH %d\n", line_number++, __func__, PTT_switch_function);
                break;
        }
        status = Update_MFC_ini();
        return status;
}

void *MFC_Thread(void *param)
{
        int status = TRUE;
        int initialize_status = TRUE;
        int create_status = TRUE;
        int button_status = FALSE;
        int process_status = TRUE;
        char message[PATH_MAX];
        int once = TRUE;

        thread_name = __func__;
        
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s -> Starting -> Sleep Called \n", line_number++, thread_name);
        Sleep(10000);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s -> Started \n", line_number++, __func__);
        initialize_status = Initialize_MFC();
        if (initialize_status == FALSE) {
                create_status = Create_MFC_ini();
                if (create_status == FALSE) {
                        print_time(0);
                        if (create_status < 0) {
                                fprintf(G_fp_logfile, "[%d] %s -> INITIALIZATION FAILED -> status: %d \n", line_number++, __func__, create_status);
                        }
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] %s -> ABNORMAL EXIT -> status: %d \n", line_number++, __func__, create_status);
                        pthread_exit(0);
                        return NULL;
                }
        }
        if (MFC_state == 1) {
                status = Open_MFC();
                if (status == FALSE) {
                        fprintf(G_fp_logfile, "[%d] %s -> Open_MFC -> FAILED -> status: %d\n", line_number++, __func__, status);
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] %s -> ABNORMAL EXIT -> status: %d \n", line_number++, __func__, status);
                        strcpy(message, "Open MFC as FAILED.  MFC is not running");
                        Gui_send_param_extended(CMD_SET_SERVER_ERROR, (byte *) message, strlen(message));
                        pthread_exit(0);
                        return NULL;
                }
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s -> Open_MFC SUCCESS \n", line_number++, __func__);
        Set_MFC_Increment(FALSE);
        speaker_volume = G_Speaker_volume;
        while (G_all_threads_run && process_status == TRUE) {
                Set_MFC_Increment(FALSE);
                button_status = Read_Buttons();
                if (button_status == BUTTON_FINISHED) {
                        status = Read_Rotary_Switch();
                        if (status == FALSE) {
                                process_status = FALSE;
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] %s -> Read_Rotary_Switch -> READ FAILED -> status: %d \n", line_number++, __func__, status);
                        }
                } else {
                        if (button_status == FALSE) {
                                process_status = FALSE;
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] %s -> Read_Buttons -> READ FAILED -> status: %d \n", line_number++, __func__, status);
                        }
                }
                Sleep(1);
        }
        if (process_status == FALSE) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s -> ABNORMAL EXIT -> status: %d \n", line_number++, __func__, status);
                strcpy(message, "ABNORMAL EXIT.  MFC Processing has STOPPED");
                Gui_send_param_extended(CMD_SET_SERVER_ERROR, (byte *) message, strlen(message));
        } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s -> Normal Exit. \n", line_number++, __func__);
        }
        pthread_exit(0);
        return NULL;
}

