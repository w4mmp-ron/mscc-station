
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
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <fcntl.h>
#include <wiringPi.h>
#include <errno.h>
#include "usbavrcmd.h"

int MFC_Read(char *buffer, int length);

typedef unsigned char byte;
typedef signed char sbyte;
typedef signed char bool;
typedef int BOOL;

#define NO_MFC_DEVICE -1
#define LOOP_LIMIT 250

//These are for the rotory 
#define MAX_FREQUENCY 32000000
#define FREQUENCY_FUNCTION 0
#define VOLUME_FUNCTION 1
#define RIT_FUNCTION 2
#define NO_FUNCTION 3

#define BUTTON_PROCESSING 3
#define BUTTON_FINISHED 4

#define  NONE 0
#define  STEP 1
#define  PTT 2
#define  TUNE 3
#define  MODE 4
#define  RIT_MODE 5
#define  BAND 6
#define  FREQ_VOL 7
#define  CW_BW 8
#define  HI_BW 9
#define  FAVS 10
#define RIT_RESET 11

int MFC_file;
int G_Version = 0;
int G_State = 1;
int G_Rotory_function = FREQUENCY_FUNCTION;
int G_Freq_digit = 1;
int G_Auto_zero = 0;
uint8_t G_Speaker_volume = 50;
BOOL G_process_user_control_status = FALSE;
int G_MFC_Mode = 0;
long G_Button_Down_Count = 0;
byte G_MFC_Active = 0;
int G_MFC_Address = 0;
byte MFC_GP_A = {0};
byte MFC_GP_B = {0};

byte MFC_state = 1;

const byte Encoder_A = 0x01;
const byte Encoder_B = 0x02;

byte Knob_switch_function = STEP;
const byte Knob_switch = 0x04;

byte Button_left_function = BAND;
const byte Button_left_switch = 0x08;

byte Button_middle_function = TUNE;
const byte Button_middle_switch = 0x10;

byte Button_right_function = FREQ_VOL;
const byte Button_right_switch = 0x20;

byte PTT_switch_function = PTT;
const byte PTT_switch = 0x40;

const byte Knob_switch_star = 0x10;
const byte Button_left_switch_star = 0x20;
const byte Button_middle_switch_star = 0x30;
const byte Button_right_switch_star = 0x40;

bool encoder_A = false;
bool encoder_B = false;
sbyte rec_enc_table[] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0};
sbyte previous_switch_table[] = {0, 0, 0, 0, 0};
bool Continue = true;
byte prevNextCode = 0;
uint32_t Freq = 14074000;
int G_Multiplier = 3;
int multiplier = 1;
int read_rotary_switch_loop_count = LOOP_LIMIT;
const char *MFC_thread_name;
static int speaker_volume = 0;
uint32_t rit_offset = 0;
byte master_function_toggle;
int knob_previous_mode = 0;
static bool freq_volume_toggle = FALSE;

int In_band(uint32_t freq) {
    uint32_t offset = 0;
    int status = FALSE;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] In_band . Frequency: %ld \n", line_number++, freq);
    if (freq >= (1800000 - offset) && freq <= (2000000 + offset)) status = TRUE;
    if (freq >= 3500000 - offset && freq <= 4000000 + offset) status = TRUE;
    if (freq >= 5330500 - offset && freq <= 5403500 + offset) status = TRUE;
    if (freq >= 7000000 - offset && freq <= 7300000 + offset) status = TRUE;
    if (freq >= 10100000 - offset && freq <= 10150000 + offset) status = TRUE;
    if (freq >= 14000000 - offset && freq <= 14350000 + offset) status = TRUE;
    if (freq >= 18068000 - offset && freq <= 18168000 + offset) status = TRUE;
    if (freq >= 21000000 - offset && freq <= 21450000 + offset) status = TRUE;
    if (freq >= 24890000 - offset && freq <= 24990000 + offset) status = TRUE;
    if (freq >= 28000000 - offset && freq <= 29700000 + offset) status = TRUE;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] In_band . Finished . status: %d\n", line_number++, status);
    return status;
}

void MFC_Set_Mode_Function() {
    char mode = 'W';
    int status = 0;

    G_MFC_Mode++;
    if (G_MFC_Mode > 3) {
        G_MFC_Mode = 0;
    }
    switch (G_MFC_Mode) {
        case 0:
            mode = MODE_AM_LETTER;
            break;
        case 1:
            mode = MODE_LSB_LETTER;
            break;
        case 2:
            mode = MODE_USB_LETTER;
            break;
        case 3:
            mode = MODE_CW_LETTER;
            break;
    }
    ModeChanged(mode);
    SDRcore_recv_send_param(CMD_SET_MAIN_MODE, G_MFC_Mode);
    SDRcore_trans_send_param(CMD_SET_MAIN_MODE, G_MFC_Mode);
    Spectrum_Waterfall_send_param(CMD_SET_MAIN_MODE, mode);
    status = Gui_send_param(CMD_SET_MAIN_MODE, mode);
}

void MFC_Favs_Function() {
    byte extended_data[4];
    byte fav = 1;

    memcpy(&extended_data[0], &fav, sizeof (fav));
    Gui_send_param_extended(CMD_MFC_SET_FAVS, extended_data, sizeof (fav));
}

bool MFC_Band_Function() {
    bool status = true;
    byte band = 0;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s . %s . G_band_normal: %d\n", line_number++,
            MFC_thread_name, __func__, G_band_normal);
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
            band = 160;
            break;
            /*case 10:
        band = 200;
        break;
    default:
        band = 160;
        break;*/
    }
    G_band_normal = band;
    Gui_send_param_extended(CMD_MFC_SET_BAND, &band, sizeof (band));
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s . %s . G_band_normal: %d\n", line_number++,
            MFC_thread_name, __func__, G_band_normal);
    return status;
}

int32_t Zero_Frequency(int32_t frequency) {
    int32_t remainder = 0;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] %s . %s . frequency: %d, G_Freq_digit: %d\n",
            line_number++, MFC_thread_name, __func__, frequency, G_Freq_digit);
    switch ((G_Freq_digit)) {
        case 7: //10 Millions
            remainder = frequency % 10000000;
            break;
        case 6: //Millions
            remainder = frequency % 1000000;
            break;
        case 5:
            remainder = frequency % 100000;
            break;
        case 4:
            remainder = frequency % 10000;
            break;
        case 3:
            remainder = frequency % 1000;
            break;
        case 2:
            remainder = frequency % 100;
            break;
        case 1:
            remainder = frequency % 10;
            break;
        default:
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . %s . INVALID G_Freq_digit: %d\n",
                    line_number++, MFC_thread_name, __func__, G_Freq_digit);
            break;
    }
    frequency = frequency - remainder;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s . %s . G_tune_freq: %d, G_Freq_digit: %d\n",
            line_number++, MFC_thread_name, __func__, frequency, G_Freq_digit);
    return frequency;
}

void MFC_Set_Step(int set_by_button) {
    static uint16_t previous_multiplier = 10;

    if (set_by_button == TRUE) {
        G_Multiplier--;
        if (G_Multiplier < 0) {
            G_Multiplier = 5;
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s . %s . G_button_Down_Count: %ld\n",
                line_number++, MFC_thread_name, __func__, G_Button_Down_Count);
    }
    if (previous_multiplier != G_Multiplier) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s . %s . G_Multiplier: %d\n",
                line_number++, MFC_thread_name, __func__, G_Multiplier);
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
        }
        E_step = G_Multiplier;
        Gui_send_param(CMD_SET_STEP_VALUE, G_Multiplier);
        Spectrum_Waterfall_send_param(CMD_SET_STEP_VALUE, G_Multiplier);
        previous_multiplier = G_Multiplier;
    }
}

void Set_Previous_Knob_Function(INT8 function) {
    if (function == FREQ_VOL) {
        knob_previous_mode = FREQ_VOL;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Set_Previous_Knob_Function . previous_mode: %d\n",
                line_number++, knob_previous_mode);
    }
}

int Initialize_MFC() {
    int status = TRUE;
    FILE *MFC_ini;
    char file_name[PATH_MAX] = {0};
    //const char *homedir;
    char init_record[300];
    char *record;
    INT8 mynumber;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Initialize_MFC . Called.\n", line_number++);
    if ((homedir = getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_MFC . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/.local/share/mscc/mfc_controller.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Initialize_MFC . Filename: %s\n", line_number++, file_name);
        MFC_ini = fopen(file_name, "r");
        if (MFC_ini != NULL) {
            while (fgets(init_record, sizeof (init_record), MFC_ini) != NULL) {
                record = strstr(init_record, "VERSION");
                if (record != NULL) {
                    mynumber = atoi((record + sizeof ("VERSION")));
                    G_Version = mynumber;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Initialize_MFC . VERSION: %d\n", line_number++, G_Version);
                }
                record = strstr(init_record, "STATE");
                if (record != NULL) {
                    mynumber = atoi((record + sizeof ("STATE")));
                    G_State = mynumber;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Initialize_MFC . STATE: %d\n", line_number++, G_State);
                }
                record = strstr(init_record, "KNOB_SWITCH");
                if (record != NULL) {
                    mynumber = atoi((record + sizeof ("KNOB_SWITCH")));
                    Knob_switch_function = mynumber;
                    Set_Previous_Knob_Function(mynumber);
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Initialize_MFC . KNOB_SWITCH: %d\n", line_number++, Knob_switch_function);
                }
                record = strstr(init_record, "LEFT_SWITCH");
                if (record != NULL) {
                    mynumber = atoi((record + sizeof ("LEFT_SWITCH")));
                    Button_left_function = mynumber;
                    Set_Previous_Knob_Function(mynumber);
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Initialize_MFC . LEFT_SWITCH: %d\n", line_number++, Button_left_function);
                }
                record = strstr(init_record, "MIDDLE_SWITCH");
                if (record != NULL) {
                    mynumber = atoi((record + sizeof ("MIDDLE_SWITCH")));
                    Button_middle_function = mynumber;
                    Set_Previous_Knob_Function(mynumber);
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Initialize_MFC . MIDDLE_SWITCH: %d\n", line_number++, Button_middle_function);
                }
                record = strstr(init_record, "RIGHT_SWITCH");
                if (record != NULL) {
                    mynumber = atoi((record + sizeof ("RIGHT_SWITCH")));
                    Button_right_function = mynumber;
                    Set_Previous_Knob_Function(mynumber);
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Initialize_MFC . RIGHT_SWITCH: %d\n", line_number++, Button_right_function);
                }
                record = strstr(init_record, "PTT_SWITCH");
                if (record != NULL) {
                    mynumber = atoi((record + sizeof ("PTT_SWITCH")));
                    PTT_switch_function = mynumber;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Initialize_MFC . PTT_SWITCH: %d\n", line_number++, PTT_switch_function);
                }
            }
            fclose(MFC_ini);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Initialize_MFC . FAILED.\n", line_number++);
            fflush(G_fp_logfile);
            status = FALSE;
        }
    }
    return status;
}

int Update_MFC_ini() {
    int status = TRUE;
    FILE *MFC_ini;
    char file_name[PATH_MAX] = {0};
    const char *homedir;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_MFC_ini . Called.\n", line_number++);
    if ((homedir = getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_MFC_ini . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/.local/share/mscc/mfc_controller.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_MFC_ini . Filename: %s\n", line_number++, file_name);
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
            fprintf(G_fp_logfile, "[%d] Update_MFC_ini . FAILED.\n", line_number++);
            status = FALSE;
        }
    }
    return status;
}

int Create_MFC_ini() {
    int status = TRUE;
    FILE *MFC_ini;
    char file_name[PATH_MAX] = {0};
    const char *homedir;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Create_MFC_ini . Called.\n", line_number++);
    if ((homedir = getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_MFC_ini . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/.local/share/mscc/mfc_controller.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_MFC_ini .   Filename: %s\n", line_number++, file_name);
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
                fprintf(G_fp_logfile, "[%d] Create_MFC_ini .  FAILED.\n", line_number++);
                status = FALSE;
            }
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Create_MFC_ini .  Not creating file. \n", line_number++);
            status = TRUE;
        }
    }
    return status;
}

void Relay_States_to_bool(int switch_A, int switch_B) {
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

sbyte Decode_Rotary() {
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
    //fprintf(G_fp_logfile, "[%d] %s . %s . status: %d\n",
    //        line_number++, thread_name, __func__, status);
    return status;
}

void Select_Rotary_Function(int rotary_value) {
    uint32_t increment = 0;
    uint32_t frequency;

    speaker_volume = G_Speaker_volume;
    switch (G_Rotory_function) {
        case FREQUENCY_FUNCTION:
            if (G_tune_freq < MAX_FREQUENCY) {
                increment = rotary_value * multiplier;
                frequency = G_tune_freq + increment;
                if (G_band_normal != GENERAL_BAND) {
                    if ((In_band(frequency)) == TRUE) {
                        if (G_Auto_zero == 1) {
                            frequency = Zero_Frequency(frequency);
                        }
                        G_tune_freq = frequency;
                        freq_queue_add(G_tune_freq);
                        Gui_send_param(CMD_SET_DISPLAY_FREQ, G_tune_freq);
                    }
                } else {
                    if (G_Auto_zero == 1) {
                        frequency = Zero_Frequency(frequency);
                    }
                    G_tune_freq = frequency;
                    freq_queue_add(G_tune_freq);
                    Gui_send_param(CMD_SET_DISPLAY_FREQ, G_tune_freq);
                }
                //print_time(0);
                //fprintf(G_fp_logfile, "[%d] %s . %s . rotary_value: %d, multiplier: %d, increment: %d, frequency: %d\n",
                //        line_number++, MFC_thread_name, __func__, rotary_value, multiplier, increment, frequency);
                E_freq = G_tune_freq;
                Display_freq_queue_add(G_tune_freq);
                Spectrum_Waterfall_send_param(CMD_SET_DISPLAY_FREQ, G_tune_freq);
            }
            break;
        case VOLUME_FUNCTION:
            if (G_process_user_control_status == FALSE) {
                speaker_volume = speaker_volume + rotary_value;
                if (speaker_volume < 0) {
                    speaker_volume = 0;
                } else {
                    if (speaker_volume >= 99) {
                        speaker_volume = 99;
                    }
                }
                G_Speaker_volume = speaker_volume;
                SDRcore_recv_send_param(CMD_SET_SPEAKER_VOLUME, G_Speaker_volume);
                Gui_send_param(CMD_SET_VOLUME_BY_SERVER, G_Speaker_volume);
            } else {
                speaker_volume = G_Speaker_volume + rotary_value;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s . %s .  G_process_user_control_status NOT FALSE\n",
                        MFC_thread_name, __func__, line_number++);
            }
            break;
        case RIT_FUNCTION:
            G_Rit_Offset = G_Rit_Offset + (rotary_value * G_Rit_Step);
            if (G_Rit_Status == TRUE) {
                freq_queue_add(G_tune_freq);
            }
            Gui_send_param(CMD_SET_RIT_FREQ, G_Rit_Offset);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . %s .  RIT_FUNCTION . G_Rit_Offset: %d \n",
                    line_number++, MFC_thread_name, __func__, G_Rit_Offset);
            break;
    }
}

int Read_Rotary_Switch() {
    int loop_count = 0;
    sbyte rotary_state = 0;
    static byte previous_encoder_A = 0xFF;
    static byte previous_encoder_B = 0xFF;
    int curModeA = 0;
    int curModeB = 0;
    //int static first_pass = TRUE;
    int read_status = 0;

    read_status = MFC_Read(&MFC_GP_A, 1);
    if (read_status >= 0) {
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] %s . %s MFC_GP_A: %d\n", line_number++, MFC_thread_name, __func__, MFC_GP_A);
        if (previous_encoder_A != MFC_GP_A) {
            while (loop_count++ < LOOP_LIMIT) {
                curModeA = (MFC_GP_A & Encoder_A);
                curModeB = (MFC_GP_A & Encoder_B);
                Relay_States_to_bool(curModeA, curModeB);
                rotary_state = Decode_Rotary();
                if (rotary_state != 0) {
                    loop_count = 0;
                    Select_Rotary_Function(rotary_state);
                }
                read_status = MFC_Read(&MFC_GP_A, 1);
            }
            loop_count = 0;
            previous_encoder_A = MFC_GP_A;
        }
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] %s . %s MFC_GP_A: %d\n", line_number++, MFC_thread_name, __func__, MFC_GP_A);
    return read_status;
}

void MFC_TUNE_Function() {
    byte extended_data[4];
    static byte tune_toggle = 0;

    if (tune_toggle == 0) {
        tune_toggle = 1;
    } else {
        tune_toggle = 0;
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] MFC_TUNE_Function . tune_toggle: %d\n", line_number++, tune_toggle);

    memcpy(&extended_data[0], &tune_toggle, sizeof (tune_toggle));
    Gui_send_param_extended(CMD_MFC_SET_TUNE, extended_data, sizeof (tune_toggle));
}

byte MFC_RIT_Function(byte which_switch) {
    byte extended_data[4];
    static byte rit_toggle = 0;
    byte star = 0;

    rit_toggle = G_Rit_Status;
    if (rit_toggle == 0) {
        rit_toggle = 1;
        G_Rotory_function = RIT_FUNCTION;
        star = which_switch | 0x01;
    } else {
        star = which_switch & 0xFE;
        rit_toggle = 0;
        if (freq_volume_toggle == FALSE) {
            G_Rotory_function = FREQUENCY_FUNCTION;
        } else {
            G_Rotory_function = VOLUME_FUNCTION;
        }
    }
    memcpy(&extended_data[0], &rit_toggle, sizeof (rit_toggle));
    E_star = star;
    Gui_send_param_extended(CMD_MFC_SET_RIT_MODE, extended_data, sizeof (rit_toggle));
    Gui_send_param_extended(CMD_SET_GUI_STAR, &star, sizeof (star));
    print_time(0);
    fprintf(G_fp_logfile, "[%d] MFC_RIT_Function . rit_toggle: %d, star: %X, G_Rotory_function: %d\n", line_number++,
            rit_toggle, star, G_Rotory_function);
    return rit_toggle;
}

void MFC_RIT_RESET_Function() {
    G_Rit_Offset = 0;
    if (G_Rit_Status == TRUE) {
        freq_queue_add(G_tune_freq);
    }
    Gui_send_param(CMD_SET_RIT_FREQ, G_Rit_Offset);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s . %s . G_Rit_Status: %d\n",
            line_number++, MFC_thread_name, __func__, G_Rit_Status);
}

int Manage_Knob_Switch() {
    byte star = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s . %s . %d\n", line_number++, MFC_thread_name, __func__, Knob_switch_function);
    switch (Knob_switch_function) {
        case RIT_RESET:
            MFC_RIT_RESET_Function();
            break;
        case NONE:
            break;
        case STEP:
            MFC_Set_Step(TRUE);
            break;
        case PTT:
            break;
        case TUNE:
            MFC_TUNE_Function();
            break;
        case MODE:
            MFC_Set_Mode_Function();
            break;
        case RIT_MODE:
            MFC_RIT_Function(Knob_switch_star);
            break;
        case BAND:
            MFC_Band_Function();
            break;
        case FREQ_VOL:
            if (freq_volume_toggle == FALSE) {
                G_Rotory_function = VOLUME_FUNCTION;
                freq_volume_toggle = TRUE;
                star = Knob_switch_star | 0x01;
            } else {
                G_Rotory_function = FREQUENCY_FUNCTION;
                freq_volume_toggle = FALSE;
                star = Knob_switch_star & 0xFE;
            }
            E_star = star;
            Gui_send_param_extended(CMD_SET_GUI_STAR, &star, sizeof (star));
            break;
        case CW_BW:
            break;
        case HI_BW:
            break;
        case FAVS:
            MFC_Favs_Function();
            break;
    }
}

int Manage_Left_Switch() {
    byte star = 0;
    static byte rit_mode = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s . %s . Button_left_function: %d\n",
            line_number++, MFC_thread_name, __func__, Button_left_function);
    switch (Button_left_function) {
        case RIT_RESET:
            MFC_RIT_RESET_Function();
            break;
        case NONE:
            break;
        case STEP:
            MFC_Set_Step(TRUE);
            break;
        case PTT:
            break;
        case TUNE:
            MFC_TUNE_Function();
            break;
        case MODE:
            MFC_Set_Mode_Function();
            break;
        case RIT_MODE:
            rit_mode = MFC_RIT_Function(Button_left_switch_star);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . %s . rit_mode: %d\n",
                    line_number++, MFC_thread_name, __func__, rit_mode);
            break;
        case BAND:
            MFC_Band_Function();
            break;
        case FREQ_VOL:
            if (freq_volume_toggle == FALSE) {
                G_Rotory_function = VOLUME_FUNCTION;
                freq_volume_toggle = TRUE;
                star = Button_left_switch_star | 0x01;
            } else {
                G_Rotory_function = FREQUENCY_FUNCTION;
                freq_volume_toggle = FALSE;
                star = Button_left_switch_star & 0xFE;
            }
            E_star = star;
            Gui_send_param_extended(CMD_SET_GUI_STAR, &star, sizeof (star));
            break;
        case CW_BW:
            break;
        case HI_BW:
            break;
        case FAVS:
            MFC_Favs_Function();
            break;
    }
}

int Manage_Middle_Switch() {
    byte star = 0;
    static byte rit_mode = 0;

    switch (Button_middle_function) {
        case RIT_RESET:
            MFC_RIT_RESET_Function();
            break;
        case NONE:
            break;
        case STEP:
            MFC_Set_Step(TRUE);
            break;
        case PTT:
            break;
        case TUNE:
            MFC_TUNE_Function();
            break;
        case MODE:
            MFC_Set_Mode_Function();
            break;
        case RIT_MODE:
            rit_mode = MFC_RIT_Function(Button_middle_switch_star);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . %s . rit_mode: %d\n",
                    line_number++, MFC_thread_name, __func__, rit_mode);
            break;
        case BAND:
            MFC_Band_Function();
            break;
        case FREQ_VOL:
            if (freq_volume_toggle == FALSE) {
                G_Rotory_function = VOLUME_FUNCTION;
                freq_volume_toggle = TRUE;
                star = Button_middle_switch_star | 0x01;
            } else {
                G_Rotory_function = FREQUENCY_FUNCTION;
                freq_volume_toggle = FALSE;
                star = Button_middle_switch_star & 0xFE;
            }
            E_star = star;
            Gui_send_param_extended(CMD_SET_GUI_STAR, &star, sizeof (star));
            break;
        case CW_BW:
            break;
        case HI_BW:
            break;
        case FAVS:
            MFC_Favs_Function();
            break;
    }
}

int Manage_Right_Switch() {
    byte star = 0;
    static byte rit_mode = 0;

    switch (Button_right_function) {
        case RIT_RESET:
            MFC_RIT_RESET_Function();
            break;
        case NONE:
            break;
        case STEP:
            MFC_Set_Step(TRUE);
            break;
        case PTT:
            break;
        case TUNE:
            MFC_TUNE_Function();
            break;
        case MODE:
            MFC_Set_Mode_Function();
            break;
        case RIT_MODE:
            rit_mode = MFC_RIT_Function(Button_right_switch_star);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . %s . rit_mode: %d\n",
                    line_number++, MFC_thread_name, __func__, rit_mode);
            break;
        case BAND:
            MFC_Band_Function();
            break;
        case FREQ_VOL:
            if (freq_volume_toggle == FALSE) {
                G_Rotory_function = VOLUME_FUNCTION;
                freq_volume_toggle = TRUE;
                star = Button_right_switch_star | 0x01;
            } else {
                G_Rotory_function = FREQUENCY_FUNCTION;
                freq_volume_toggle = FALSE;
                star = Button_right_switch_star & 0xFE;
            }
            E_star = star;
            Gui_send_param_extended(CMD_SET_GUI_STAR, &star, sizeof (star));
            break;
        case CW_BW:
            break;
        case HI_BW:
            break;
        case FAVS:
            MFC_Favs_Function();
            break;
    }
}

int Manage_PTT_Switch() {
    static bool freq_volume_toggle = FALSE;

    switch (PTT_switch_function) {
        case NONE:
            break;
        case STEP:
            MFC_Set_Step(TRUE);
            break;
        case PTT:
            break;
        case TUNE:
            MFC_TUNE_Function();
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
        case FAVS:
            MFC_Favs_Function();
            break;
    }
}

int Read_Buttons() {
    int status = BUTTON_FINISHED;
    static byte state = 0;
    UCHAR switch_state_inverted = 0;
    static UCHAR current_switch = 0xFF;
    //static UCHAR which_switch = 0;
    //static UCHAR previous_which_switch = 0xFF;
    int read_status = 0;
    static byte button_status = FALSE;
    char filename[PATH_MAX] = {0};

    /*G_MFC_Active = 1;
    if (G_MFC_Address == MFC_ADDRESS_1) {
        read_status = Read_i2c(I2C_BUSS, filename, G_MFC_Address, 0x09, &MFC_GP_A, 1);
    } else {
        read_status = Read_i2c(I2C_BUSS, filename, G_MFC_Address, 0x00, &MFC_GP_A, 1);
    }*/
    read_status = MFC_Read(&MFC_GP_A, 1);
    if (read_status >= 0) {
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] %s . %s MFC_GP_A: %d\n", line_number++, MFC_thread_name, __func__, MFC_GP_A);
        switch_state_inverted = ~MFC_GP_A; //Switches are active low.  Invert for active high
        switch_state_inverted = switch_state_inverted & 0xFC; //Remove everything but the button switches
        switch (state) {
            case 0:
                if (switch_state_inverted > 0) {
                    G_Button_Down_Count++;
                    status = BUTTON_PROCESSING;
                    current_switch = switch_state_inverted;
                    button_status = TRUE;
                    state = 0;
                } else {
                    if (button_status == TRUE) {
                        state = 1;
                    }
                }
                //fprintf(G_fp_logfile, "[%d] %s . %s . switch_state_inverted: %d, state: %d \n",
                //        line_number++, MFC_thread_name, __func__, switch_state_inverted, state);
                break;
            case 1:
                switch (current_switch) {
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
                }
                state = 2;
                //fprintf(G_fp_logfile, "[%d] %s . %s . switch_state_inverted: %d, state: %d \n",
                //        line_number++, MFC_thread_name, __func__, switch_state_inverted, state);
                status = BUTTON_PROCESSING;
                break;
            case 2:
                status = BUTTON_FINISHED;
                G_Button_Down_Count = 0;
                button_status = FALSE;
                current_switch = 0xFF;
                state = 0;
                //fprintf(G_fp_logfile, "[%d] %s . %s . switch_state_inverted: %d, state: %d \n",
                //        line_number++, MFC_thread_name, __func__, switch_state_inverted, state);
                break;
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s . %s . Read FAILED: \n", line_number++, MFC_thread_name, __func__);
        status = -1;
    }

    G_MFC_Active = 0;
    return status;
}

int Process_MFC_Controls(uint8_t command, char *buf) {
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
            fprintf(G_fp_logfile, "[%d] %s . CMD_SET_KNOB_SWITCH:%d \n", line_number++, __func__, Knob_switch_function);
            break;

        case CMD_SET_LEFT_SWITCH:
            Button_left_function = t_opcode_data;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . CMD_SET_LEFT_SWITCH %d\n", line_number++, __func__, Button_left_function);
            break;

        case CMD_SET_MIDDLE_SWITCH:
            Button_middle_function = t_opcode_data;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . CMD_SET_MIDDLE_SWITCH: %d \n", line_number++, __func__, Button_middle_function);
            break;

        case CMD_SET_RIGHT_SWITCH:
            Button_right_function = t_opcode_data;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . CMD_SET_RIGHT_SWITCH %d\n", line_number++, __func__, CMD_SET_RIGHT_SWITCH);
            break;

        case CMD_SET_PTT_SWITCH:
            PTT_switch_function = t_opcode_data;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . CMD_SET_PTT_SWITCH %d\n", line_number++, __func__, PTT_switch_function);
            break;
    }
    status = Update_MFC_ini();
    return status;
}

int MFC_Read(char *buffer, int length) {
    int status = 0;
    int read_count = 0;

    read_count = read(MFC_file, buffer, length);
    if (read_count != length) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] MFC_Read. FAILED, read_count: %d, length: %d\n", line_number++, read_count, length);
        status = -1;
    }
    return status;
}

int MFC_Open(int mfc_address) {
    int status = 0;
    unsigned char buffer;
    int buffer_size = 1;
    int write_count = 0;
    char *filename = (char *) "/dev/i2c-6";

    if ((MFC_file = open(filename, O_RDWR)) < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] MFC_Open. Buss Open. FAILED. file_i2c: %d,\n", line_number++, MFC_file);
        status = -1;
    } else {
        int addr = mfc_address; //<<<<<The I2C address of the slave
        if (ioctl(MFC_file, I2C_SLAVE, addr) < 0) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] MFC_Open. Set address FAILED. address: 0x:%0X\n", line_number++, addr);
            status = -1;
        } else {
            buffer = 0xFF;
            write_count = write(MFC_file, &buffer, buffer_size);
            if (write_count != buffer_size) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] MFC_Open. FAILED\n", line_number++, buffer_size);
                status = -1;
            }
        }
    }
    return status;
}

void *MFC_Thread(void *param) {
    int status = TRUE;
    int initialize_status = TRUE;
    int create_status = TRUE;
    int button_status = BUTTON_FINISHED;
    char message[PATH_MAX];
    int token_status = 0;
    long long previous_lock_failed = 0;
    long long lock_failed_diff = 0;
    long long previous_lock_failed_diff = 0;
    long long lock_failed_limit = 350;
    long long lock_failed = 0;

    MFC_thread_name = __func__;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s . Started \n", line_number++, __func__);
    //strcpy(message, "MFC STARTED");
    //message_queue_add(CMD_SET_SERVER_ERROR, message, strlen(message));
    initialize_status = Initialize_MFC();
    if (initialize_status == FALSE) {
        create_status = Create_MFC_ini();
        if (create_status == FALSE) {
            print_time(0);
            if (create_status < 0) {
                fprintf(G_fp_logfile, "[%d] %s . INITIALIZATION FAILED . status: %d \n", line_number++, __func__, create_status);
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . ABNORMAL EXIT . status: %d \n", line_number++, __func__, create_status);
            pthread_exit(0);
            return NULL;
        }
    }
    if (G_State == 1) {
        status = MFC_Open(G_MFC_Address);
        if (status < 0) {
            fprintf(G_fp_logfile, "[%d] %s . MFC_Initialize . FAILED . status: %d\n", line_number++, __func__, status);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . ABNORMAL EXIT . status: %d \n", line_number++, __func__, status);
            strcpy(message, "MFC_Initialize FAILED.  MFC is not running");
            Gui_Add_Message(message);
            pthread_exit(0);
            return NULL;
        }
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s . Open_MFC SUCCESS \n", line_number++, __func__);
    MFC_Set_Step(FALSE);
    speaker_volume = G_Speaker_volume;
    while (G_all_threads_run && status >= 0) {
        token_status = pthread_mutex_trylock(&Master_Device_Token_available);
        if (token_status == 0) {
            MFC_Set_Step(FALSE);
            button_status = Read_Buttons();
            if (button_status == BUTTON_FINISHED) {
                status = Read_Rotary_Switch();
                if (status < 0) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] %s . Read_Rotary_Switch . READ FAILED . status: %d \n", line_number++,
                            __func__, status);
                }
            } else {
                if (button_status < 0) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] %s . Read_Buttons . READ FAILED . status: %d \n", line_number++,
                            __func__, status);
                }
            }
            pthread_mutex_unlock(&Master_Device_Token_available);
        } else {
            lock_failed++;
            lock_failed_diff = lock_failed - previous_lock_failed;
            if (lock_failed_diff > LOCK_FAILED_LIMIT) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s. G_lock_failed: %I64d, previous_G_lock_failed: %I64d, lock_failed_diff: %I64d\n",
                        line_number++, __func__, lock_failed, previous_lock_failed, lock_failed_diff);
            }
            previous_lock_failed = lock_failed;
            previous_lock_failed_diff = lock_failed_diff;
        }
        Sleep(Get_random_time());
    }
    if (status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s . ABNORMAL EXIT . status: %d \n", line_number++, __func__, status);
        strcpy(message, "ABNORMAL EXIT.  MFC Processing has STOPPED");
        Gui_Add_Message(message);
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s . NORMAL EXIT. \n", line_number++, __func__);
    }
    pthread_exit(0);
    return NULL;
}

