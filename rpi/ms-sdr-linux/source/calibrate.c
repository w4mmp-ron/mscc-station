#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>

#include<string.h> //memset
#include<stdlib.h> //exit(0);

#include <stdio.h>


#include "usbavrcmd.h"
#include "extern.h"
#include "version.h"


int G_Calibration_temperature = 0;
int previous_temperature = 0;
uint8_t G_Calibration_reset = FALSE;
uint8_t Cal_Reset = FALSE;
INT32 delta = 0;

static INT32 manual_integer_part = 0;
static INT32 manual_decimal_part = 0;

uint32_t G_calibration_freq = 0;
int G_calibration_element = 0;
int G_calibration_limit;
int G_max_element = MAX_CALIBRATION_ELEMENT;
int G_element_increament = 1;
uint8_t G_Standard_Carrier_number = 0;

//int PPM_file_exists = 0;
uint32_t G_cycle_count = 65536;
uint8_t G_check_calibration = FALSE;
uint8_t mode_number;
uint8_t previous_mode_number;
char previous_mode = 'N';
uint32_t previous_freq;

//const char *homedir;

int Init_PPM() {
    int status = 1;
    FILE *fp_User_ini;
    char file_name[PATH_MAX] = {0};
    char init_record[300];
    int mynumber;

    struct {
        char * Int_ppm;
        char * Dec_ppm;
        char * Delta;
        char * Calibration_temperature;
    } device_record;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Init_PPM. \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {

        print_time(0);
        fprintf(G_fp_logfile, "[%d] Init_PPM. Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/freq_cal.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Init_PPM. freq_cal.ini Path: %s\n", line_number++, file_name);
        fp_User_ini = fopen(file_name, "r");
        if (fp_User_ini != NULL) {
            while (fgets(init_record, sizeof (init_record), fp_User_ini) != NULL) {
                device_record.Int_ppm = strstr(init_record, "PPM_INT");
                if (device_record.Int_ppm != NULL) {
                    mynumber = atoi((device_record.Int_ppm + sizeof ("PPM_INT")));
                    G_int = mynumber;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_PPM. PPM_INT: %d\n", line_number++, G_int);
                }
                device_record.Dec_ppm = strstr(init_record, "PPM_DEC");
                if (device_record.Dec_ppm != NULL) {
                    mynumber = atoi((device_record.Dec_ppm + sizeof ("PPM_DEC")));
                    G_dec = mynumber;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_PPM. PPM_DEC: %d\n", line_number++, G_dec);
                }
                device_record.Delta = strstr(init_record, "DELTA");
                if (device_record.Delta != NULL) {
                    mynumber = atoi((device_record.Delta + sizeof ("DELTA")));
                    delta = mynumber;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_PPM. Delta: %d\n", line_number++, delta);

                }
                device_record.Calibration_temperature = strstr(init_record, "CALIBRATION_TEMPERATURE");
                if (device_record.Calibration_temperature != NULL) {
                    mynumber = atoi((device_record.Calibration_temperature + sizeof ("CALIBRATION_TEMPERATURE")));
                    G_Calibration_temperature = mynumber;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_PPM. G_Calibration_temperature: %d\n", line_number++, G_Calibration_temperature);
                }
            }
            fclose(fp_User_ini);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Init_PPM. Open file failed\n", line_number++);
            status = 0;
        }
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Init_PPM. Finished\n", line_number++);
    return status;
}

int Update_PPM_ini(int int_part, int dec_part, int delta) {
    int status = 0;
    FILE *fp_User_ini;
    int r = 0;
    INT32 iVersion = 0;
    char file_name[PATH_MAX] = {0};

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_PPM_ini. \n", line_number++);
    r = usbControlMsgIN(CMD_GET_TRANSCEIVER_TEMP, 0xA55A, 0, (char*) &iVersion, sizeof (iVersion));
    G_Calibration_temperature = iVersion;
    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_PPM_ini. Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/freq_cal.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_PPM_ini. freq_cal.ini Path: %s\n", line_number++, file_name);
        fp_User_ini = fopen(file_name, "w");
        if (fp_User_ini != NULL) {
            fprintf(fp_User_ini, "PPM_INT=%d,PPM_DEC=%d,DELTA=%d;\n", int_part, dec_part, delta);
            fprintf(fp_User_ini, "CALIBRATION_TEMPERATURE=%d;\n", G_Calibration_temperature);
            fclose(fp_User_ini);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Update_PPM_ini. Update Failed . File Open FAILED\n", line_number++);
        }
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_PPM_ini. Finished.\n", line_number++);
    return status;
}

int Delete_PPM_ini() {
    int status = 0;
    FILE *fp_freq_cal_ini;
    char file_name[PATH_MAX] = {0};

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Delete_PPM_ini. \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(file_name, homedir);
        strcat(file_name, "/freq_cal.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Delete_PPM_ini. freq_cal.ini Path: %s\n", line_number++, file_name);
        fp_freq_cal_ini = fopen(file_name, "r");
        if (fp_freq_cal_ini != NULL) {
            fclose(fp_freq_cal_ini);
            remove(file_name);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Delete_PPM_ini. Delete freq_cal.ini FAILED.\n", line_number++);
            status = 0;
        }
    }
    return status;
}

int Create_PPM_ini() {
    int status = 0;
    FILE *fp_freq_cal_ini;
    char file_name[PATH_MAX] = {0};
    //uint8_t count = 0;
    int int_part = 0;
    int dec_part = 0;
    int calibration_temperature = 40;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Create_PPM_ini. \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_PPM_ini. Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/freq_cal.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_PPM_ini. freq_cal.ini Path: %s\n", line_number++, file_name);
        switch (G_pcb_version) {
            case PROFICIO_1:
                int_part = -23;
                dec_part = -50;
                break;
            case PROFICIO_C:
                int_part = -22;
                dec_part = -50;
                break;
            case PROFICIO_4:
                int_part = -29;
                dec_part = -80;
                break;
            case PROFICIO_5:
                int_part = 22;
                dec_part = 10;
                break;
            case GEMINUS:
                int_part = -14;
                dec_part = -80;
                break;
            case MAGNIS_DUO:
                int_part = 25;
                dec_part = 80;
                break;
            case ULTIMUS:
                int_part = 29;
                dec_part = 30;
                break;
            case PROFICIO_B:
                int_part = 28;
                dec_part = 50;
                break;
            case PROFICIO_MKII:
                int_part = -21;
                dec_part = -50;
                break;
            default:
                int_part = -21;
                dec_part = -50;
                break;
        }
        fp_freq_cal_ini = fopen(file_name, "r");
        if (fp_freq_cal_ini == NULL) {
            fp_freq_cal_ini = fopen(file_name, "w");
            if (fp_freq_cal_ini != NULL) {
                fprintf(fp_freq_cal_ini, "PPM_INT=%d,PPM_DEC=%d,DELTA=%d;\n", int_part, dec_part, 0);
                fprintf(fp_freq_cal_ini, "CALIBRATION_TEMPERATURE=%d;", calibration_temperature);
                fclose(fp_freq_cal_ini);
                G_int = int_part;
                G_dec = dec_part;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Create_PPM_ini. Updated freq_cal.ini for Tranceiver type: %d, G_int: %d, G_dec: %d\n",
                        line_number++, G_pcb_version, G_int, G_dec);
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Create_PPM_ini. Create freq_cal.ini FAILED.\n", line_number++);
                status = 0;
            }
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Create_PPM_ini. freq_cal.ini exists. Not creating file. \n", line_number++);
            status = 1;
        }
    }
    return status;
}

BOOL Set_Calibration_Freq(long freq) {
    int r = 0;

    G_dll_active = TRUE;
    print_time(1);
    fprintf(G_fp_logfile, "[%d] Set_Standard_Carrier. Frequency: %ld\n", line_number++, freq);
    G_tune_freq = freq;
    freq_queue_add(G_tune_freq);
    Sleep(50);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Set_Standard_Carrier. Finished\n", line_number++);
    G_dll_active = FALSE;
    return r;
}

float Convert_PPM_To_Float(int Int_part, int Dec_part) {
    float ppm_value = 0.0f;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Convert_PPM_To_Float. Int_part: %d, Dec_part: %d \n",
            line_number++, Int_part, Dec_part);
    ppm_value = (float) Int_part;
    ppm_value = ppm_value + (float) ((float) Dec_part / 100.0f);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Convert_PPM_To_Float. Finished. ppm_value: %f, Int_part: %d, Dec_part: %d \n",
            line_number++, ppm_value, Int_part, Dec_part);
    return ppm_value;
}

void Convert_PPM_To_Int(float ppm_f, int *int_part, int *dec_part) {
    float temp_ppm = 0.0;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Convert_PPM_To_Int. fppm: %f \n", line_number++, ppm_f);
    *int_part = (int) ppm_f;
    temp_ppm = ppm_f - (float) *int_part;
    if (temp_ppm < 0.00000) {
        temp_ppm = temp_ppm * -1;
    }
    if (temp_ppm < 0.10000000) {
        *dec_part = 0;
    } else {
        *dec_part = ((INT32) (ppm_f * N_DECIMAL_POINTS_PRECISION) % N_DECIMAL_POINTS_PRECISION);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Convert_PPM_To_Int. ppm_f: %.3f integer_part %d, decimal_part %d\n",
            line_number++, ppm_f, *int_part, *dec_part);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Convert_PPM_To_Int. Finished\n", line_number++);
}

float calculate_ppm(uint32_t frequency) {
    int32_t real_Hz = 0;
    float f_ppm = 0.0f;

    real_Hz = frequency;
    delta = real_Hz - G_tune_freq;
    delta = (delta * -1);
    print_time(1);
    fprintf(G_fp_logfile, "[%d] calculate_ppm. Real Frequency: %d, Tune Frequency: %d, delta: %d\n",
            line_number++, real_Hz, G_tune_freq, delta);
    f_ppm = (float) ((float) ((1000000 * delta)) / (float) G_tune_freq);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] calculate_ppm. f_ppm: %.3f \n",
            line_number++, f_ppm);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] calculate_ppm. Finished\n", line_number++);
    return f_ppm;
}

int Calibrate_Si5351_Set_PPM(uint32_t frequency) {//This is called by Process_Frequency_Calibration()
    int status = 0;
    float coarse_ppm = 0.0f;
    float fine_ppm = 0.0f;
    float total_ppm = 0.0f;
    int int_part = 0;
    int dec_part = 0;
    int r = 0;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Set_PPM. offset freq: %d\n", line_number++, frequency);
    coarse_ppm = Convert_PPM_To_Float(G_int, G_dec);
    fine_ppm = calculate_ppm(frequency);
    total_ppm = coarse_ppm + fine_ppm;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Set_PPM. coarse_ppm: %f, fine_ppm: %f, total_ppm: %f\n",
            line_number++, coarse_ppm, fine_ppm, total_ppm);
    Convert_PPM_To_Int(total_ppm, &int_part, &dec_part);
    status = Freq_Set_Transceiver_Calibration(int_part, dec_part);
    Update_PPM_ini(int_part, dec_part, delta);
    Init_PPM();
    G_Calibration_reset = TRUE;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Set_PPM. total_int_part: %d,total_dec_part: %d, Delta: %d\n", line_number++,
            int_part, dec_part, delta);
    G_mode = previous_mode;
    G_tune_freq = previous_freq;
    ModeChanged(previous_mode);
    freq_queue_add(G_tune_freq);
    SDRcore_recv_send_param(CMD_SET_MAIN_MODE, previous_mode_number);
    SDRcore_trans_send_param(CMD_SET_MAIN_MODE, previous_mode_number);
    Gui_send_param(CMD_SET_CALIBRATION_FINISHED, CALIBRATION_SUCCESS);
    Gui_send_param(CMD_GET_SET_CAL_FREQ_DELTA, delta);
    //Cal_Reset = FALSE;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Set_PPM. FINISHED\n", line_number++);
    return status;
}

int Freq_Set_Transceiver_Calibration(int int_part, int dec_part) {
    //This is used to adjust the Si5351 frequency. The value is in Hertz. The value is stored in the PSoC EEPROM
    int r0, r1 = 0;

    G_dll_active = TRUE;
    print_time(1);
    fprintf(G_fp_logfile, "[%d] Freq_Set_Transceiver_Calibration. Calling Radio_send_parameters. int_part:%d\n",
            line_number++, int_part);
    r0 = Radio_send_parameters(CMD_SET_XTAL_INT, int_part, 1);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Freq_Set_Transceiver_Calibration. Calling Radio_send_parameters. dec_part: %d\n",
            line_number++, dec_part);
    r1 = Radio_send_parameters(CMD_SET_XTAL_DEC, dec_part, 1);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Freq_Set_Transceiver_Calibration. int_part: %d, dec_part: %d, FINISHED\n", line_number++,
            int_part, dec_part);
    G_dll_active = FALSE;
    return ((r0 | r1));
}

void Report_Calibration(int calibration_increment) {
    static int increment = 0;

    if (Cal_Reset == FALSE) {
        increment = 0;
        Gui_send_param(CMD_SET_CALIBRATIION_PROGRESS, calibration_increment);
        print_time(1);
        fprintf(G_fp_logfile, "[%d] Report_Calibration. calibration_increment %d \n",
                line_number++, calibration_increment);
    } else {
        print_time(1);
        fprintf(G_fp_logfile, "[%d] Report_Calibration. increment %d \n",
                line_number++, increment);
        Gui_send_param(CMD_SET_CALIBRATIION_PROGRESS, increment++);

    }
}

int Calibrate_Si5351_Initialize() //This is called by Process_Frequency_Calibration()
{
    int status = TRUE;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Initialize. \n", line_number++);
    previous_mode = G_mode;
    previous_freq = G_tune_freq;
    previous_mode_number = mode_to_number(G_mode);
    ModeChanged('C');
    Sleep(50);
    G_delta_drift_int = 0;
    mode_number = mode_to_number('C');
    SDRcore_recv_send_param(CMD_SET_MAIN_MODE, mode_number);
    SDRcore_trans_send_param(CMD_SET_MAIN_MODE, mode_number);
    G_calibration_freq = G_tune_freq - (G_max_element / 2);
    G_calibration_element = 0;
    G_calibration_limit = G_max_element;
    status = CALIBRATION_INITIALIZED;
    Sleep(100);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Initialize. host_calibration_freq: %d, G_calibration_freq: %d, FINISHED \n",
            line_number++, G_tune_freq, G_calibration_freq);
    return status;
}

int Calibrate_Si5351_Continue() { //This is called by Process_Frequency_Calibration
    int status = 0;
    print_time(1);
    fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Continue.\n", line_number++);
    if (G_calibration_element <= G_calibration_limit) {
        G_delta_drift_int = 0;
        freq_queue_add(G_calibration_freq);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Continue. Calling SDRcore_recv_send_param. Param: %X, Element %d, Calibration Freq: %d\n",
                line_number++, CMD_SET_CALIBRATION_START, G_calibration_element, G_calibration_freq);
        SDRcore_recv_send_param(CMD_SET_CALIBRATION_START, G_calibration_freq);
        //Spectrum_Waterfall_send_param(CMD_SET_SPECTRUM_WATERFALL_FREQ, G_calibration_freq);
        G_calibration_freq = G_calibration_freq + G_element_increament;
        G_calibration_element = G_calibration_element + G_element_increament;
        status = CALIBRATION_RUNNING;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Continue. Element %d, Calibration Freq: %d\n",
                line_number++, G_calibration_element, G_calibration_freq);
    } else {
        status = CALIBRATION_COMPLETE;
        SDRcore_recv_send_param(CMD_SET_CALIBRATION_FINISHED, 1);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Continue. CALIBRATION_COMPLETE. Element %d, Calibration Freq: %d\n",
                line_number++, G_calibration_element, G_calibration_freq);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Continue. Element %d, Calibration Freq: %d. FINISHED\n",
            line_number++, G_calibration_element, G_calibration_freq);
    return status;
}

int Calibrate_Si5351_Failed(uint32_t frequency) {//This is called by Process_Frequency_Calibration()
    static int status = 0;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Failed. Operation FAILED.\n", line_number++);
    G_check_calibration = 0;
    G_mode = previous_mode;
    G_tune_freq = previous_freq;
    ModeChanged('A');
    freq_queue_add(G_tune_freq);
    SDRcore_recv_send_param(CMD_SET_MAIN_MODE, previous_mode_number);
    SDRcore_trans_send_param(CMD_SET_MAIN_MODE, previous_mode_number);
    status = FALSE;
    print_time(0);
    G_Proficio_Allow_Temp_Check = TRUE;
    Gui_send_param(CMD_SET_CALIBRATION_FINISHED, CALIBRAITON_FAIL);
    Gui_send_param(CMD_GET_SET_CAL_FREQ_DELTA, 10000);
    G_Calibration_temperature = previous_temperature;
    Cal_Reset = FALSE;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Failed. FINISHED\n", line_number++);
    return status;
}

int Check_Calibration_Continue() { //This is called by Process_Frequency_Calibration()
    int status = 0;
    print_time(1);
    fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Continue Called\n", line_number++);
    if (G_calibration_element <= G_calibration_limit) {
        freq_queue_add(G_calibration_freq);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Calibrate_Si5351_Continue. Calling SDRcore_recv_send_param. Param: %X, Element %d, Calibration Freq: %ld\n",
                line_number++, CMD_SET_CALIBRATION_START, G_calibration_element, G_calibration_freq);
        SDRcore_recv_send_param(CMD_SET_CALIBRATION_START, G_calibration_freq);
        //Spectrum_Waterfall_send_param(CMD_SET_SPECTRUM_WATERFALL_FREQ, G_calibration_freq);
        G_calibration_freq = G_calibration_freq + G_element_increament;
        G_calibration_element = G_calibration_element + G_element_increament;
        status = CALIBRATION_RUNNING;
    } else {
        status = CALIBRATION_COMPLETE;
        G_Proficio_Allow_Temp_Check = TRUE;
        SDRcore_recv_send_param(CMD_SET_CALIBRATION_FINISHED, 1);
    }
    return status;
}

int Check_Calibration_PPM(uint32_t frequency) {//This is called by Process_Frequency_Calibration()
    int status = 0;
    float coarse_ppm = 0.0f;
    float fine_ppm = 0.0f;
    float total_ppm = 0.0f;
    int total_int_part = 0;
    int total_dec_part = 0;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Check_Calibration_PPM. delta freq: %d\n", line_number++, frequency);
    coarse_ppm = Convert_PPM_To_Float(G_int, G_dec);
    fine_ppm = calculate_ppm(frequency);
    total_ppm = coarse_ppm + fine_ppm;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_Calibration_PPM. coarse_ppm: %f, fine_ppm: %f, total_ppm: %f\n", line_number++,
            coarse_ppm, fine_ppm, total_ppm);
    Convert_PPM_To_Int(total_ppm, &total_int_part, &total_dec_part);
    G_Calibration_temperature = previous_temperature;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_Calibration_PPM. total_int_part: %d,total_dec_part: %d, delta: %d\n",
            line_number++, total_int_part, total_dec_part, delta);
    G_mode = previous_mode;
    G_tune_freq = previous_freq;
    ModeChanged(previous_mode);
    freq_queue_add(G_tune_freq);
    SDRcore_recv_send_param(CMD_SET_MAIN_MODE, previous_mode_number);
    SDRcore_trans_send_param(CMD_SET_MAIN_MODE, previous_mode_number);
    Gui_send_param(CMD_SET_CALIBRATION_FINISHED, CALIBRATION_SUCCESS);
    Gui_send_param(CMD_GET_SET_CAL_FREQ_DELTA, delta);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_Calibration_PPM. Finished\n", line_number++);
    return status;
}

void Process_Check_Calibration(uint8_t command, char *buf, uint8_t state) {
    static int calibration_state = 0;
    static int calibration_initialized = 0;
    static int calibration_count = 0;
    static int calibration_increament = 0;
    static uint8_t t_opcode_data = 0;
    static short int *opcode_data = 0;
    static short int s_opcode_data = 0;
    static int *op_code_data_32;
    static int i_opcode_data;
    static uint32_t u_int_data = 0;
    static uint8_t opcode_data_8_bit;

    op_code_data_32 = (int*) &buf[1];
    memcpy(&i_opcode_data, op_code_data_32, 4);
    opcode_data_8_bit = (uint8_t) (buf[1]);

    switch (state) {
        case CMD_SET_FREQ_CAL_CHECK:
            //previous_delta = G_delta_drift_int;
            G_Proficio_Allow_Temp_Check = FALSE;
            Init_PPM();
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Check_Calibration. G_int: %d, G_dec: %d\n",
                    line_number++, G_int, G_dec);
            G_max_element = 100;
            G_element_increament = 1;
            calibration_initialized = Calibrate_Si5351_Initialize();
            if (calibration_initialized == CALIBRATION_INITIALIZED) {
                SDRcore_recv_send_param(CDM_SET_CALIBRATE_CYCLE_COUNT, G_cycle_count);
                calibration_state = Check_Calibration_Continue(i_opcode_data);
            }
            break;
        case CMD_SET_CALIBRATION_DATA: //The SDRcore-recv calibration routine has finished
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Check_Calibration. CMD_SET_CALIBRATION_DATA \n", line_number++);
            if (i_opcode_data == 0) {//The SDRcore-recv calibration routine failed
                Calibrate_Si5351_Failed(i_opcode_data);
            } else {
                Check_Calibration_PPM(i_opcode_data);
            }
            calibration_state = 0;
            calibration_initialized = 0;
            calibration_count = 0;
            calibration_increament = 0;
            G_Proficio_Allow_Temp_Check = TRUE;
            break;
        case CMD_SET_CAL_DATA_PROCESSED: //The SDRcore-recv calibration routine is running 
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Check_Calibration. CMD_SET_CAL_DATA_PROCESSED \n", line_number++);
            if (opcode_data_8_bit == CALIBRATION_SUCCESS) { // and returned the current frequency under test
                if (calibration_state == CALIBRATION_RUNNING) {
                    calibration_state = Check_Calibration_Continue(i_opcode_data);
                    Report_Calibration(calibration_increament);
                    calibration_count++;
                    calibration_increament++;
                }
            } else {
                Calibrate_Si5351_Failed(i_opcode_data);
                calibration_state = 0;
                calibration_initialized = 0;
                calibration_count = 0;
                calibration_increament = 0;
                Sleep(100);
                G_check_calibration = FALSE;
            }
    }
}

int Adjust_int_Part(int int_part) {
    int status = 0;

    manual_decimal_part = 0;
    manual_integer_part = G_int + int_part;
    print_time(1);
    fprintf(G_fp_logfile, "[%d] Adjust_int_Part. int_part: %d, manual_integer_part: %d,G_int:%d  \n",
            line_number++, int_part, manual_integer_part, G_int);
    status = Freq_Set_Transceiver_Calibration(manual_integer_part, manual_decimal_part);
    freq_queue_add(G_tune_freq + 10);
    freq_queue_add(G_tune_freq);
    //Spectrum_Waterfall_send_param(CMD_SET_SPECTRUM_WATERFALL_FREQ, G_tune_freq);
    return status;
}

int Manual_Calibration_Set(uint8_t status) {

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Manual_Calibration_Set. status: %d \n",
            line_number++, status);
    if (status == TRUE) {
        Update_PPM_ini(manual_integer_part, manual_decimal_part, 0);

    } else {
        status = Freq_Set_Transceiver_Calibration(G_int, G_dec);
        Update_PPM_ini(G_int, G_dec, 0);
    }
    Init_PPM();
    //Spectrum_Waterfall_send_param(CMD_SET_SPECTRUM_WATERFALL_FREQ, G_tune_freq);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Manual_Calibration_Set. G_int: %d,G_dec: %d. FINISHED \n",
            line_number++, G_int, G_dec);
    return status;
}

int Process_Frequency_Calibration(uint8_t command, char *buf) {
    int status = 0;
    static int calibration_state = 0;
    static int calibration_initialized = 0;
    static int calibration_count = 0;
    static int calibration_increament = 0;
    int reset_type = 0;
    uint8_t t_opcode_data = 0;
    static short int *opcode_data = 0;
    static short int s_opcode_data = 0;
    static int *op_code_data_32;
    static int i_opcode_data;
    static uint32_t u_int_data = 0;
    static uint8_t opcode_data_8_bit;

    op_code_data_32 = (int*) &buf[1];
    memcpy(&i_opcode_data, op_code_data_32, 4);
    opcode_data_8_bit = (uint8_t) (buf[1]);
    switch (command) {
        case CMD_SET_CALIBRATION_FINISHED: //This is called by the client
            Manual_Calibration_Set(opcode_data_8_bit);
            break;

        case CMD_SET_CAL_SET_COARSE: //This is called by the client
            Adjust_int_Part(i_opcode_data);
            break;

        case CMD_SET_FORCE_CALIBRATION: //This is called by the client
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_SET_FORCE_CALIBRATION. G_tune_freq: %d\n",
                    line_number++, G_tune_freq);
            previous_freq = G_tune_freq;
            freq_queue_add(G_tune_freq);
            break;

        case CMD_SET_FREQ_CAL_CHECK: //This is called by the client 
            G_check_calibration = opcode_data_8_bit;
            if (G_check_calibration == 1) {
                print_time(1);
                fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_SET_FREQ_CAL_CHECK. Calling Check_Calibration. G_check_only: %d\n",
                        line_number++, G_check_calibration);
                Process_Check_Calibration(command, buf, CMD_SET_FREQ_CAL_CHECK);
                previous_temperature = G_Calibration_temperature;
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_SET_FREQ_CAL_CHECK. G_check_only: %d\n",
                    line_number++, G_check_calibration);

            break;

        case CMD_SET_STANDARD_CARRIER: //This is called by client before starting calibration
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_SET_STANDARD_CARRIER. %d\n",
                    line_number++, i_opcode_data);
            Set_Calibration_Freq((long) i_opcode_data);
            break;

        case CMD_START_CALIBRATE: //This is called by the client
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_START_CALIBRATE. \n", line_number++);
            freq_queue_add(G_tune_freq);
            G_Proficio_Allow_Temp_Check = FALSE;
            G_delta_drift_int = 0;
            previous_temperature = G_Calibration_temperature;
            calibration_initialized = Calibrate_Si5351_Initialize();
            if (calibration_initialized == CALIBRATION_INITIALIZED) {
                SDRcore_recv_send_param(CDM_SET_CALIBRATE_CYCLE_COUNT, G_cycle_count);
                calibration_state = Calibrate_Si5351_Continue();
            }
            break;

        case CMD_SET_CALIBRATION_DATA: //This is called by SDRcore_recv and ends the calibration with success for fail
            if (!G_check_calibration) {
                print_time(1);
                fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_SET_CALIBRATION_DATA \n", line_number++);
                if (i_opcode_data == 0) {//The SDRcore-recv calibration routine failed
                    Calibrate_Si5351_Failed(i_opcode_data);
                } else {//The SDRcore-recv calibration routine finished successfully
                    calibration_state = Calibrate_Si5351_Set_PPM(i_opcode_data);
                }
                calibration_state = 0;
                calibration_initialized = 0;
                calibration_count = 0;
                calibration_increament = 0;
                G_Proficio_Allow_Temp_Check = TRUE;
                Cal_Reset = FALSE;

            } else {
                Process_Check_Calibration(command, buf, CMD_SET_CALIBRATION_DATA);
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_SET_CALIBRATION_DATA. FINISHED \n", line_number++);
            break;

        case CMD_SET_CAL_DATA_PROCESSED: //This is called by SDRcore_recv. Calibration will continue or fail
            if (!G_check_calibration) {
                print_time(1);
                fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_SET_CAL_DATA_PROCESSED \n", line_number++);
                if (opcode_data_8_bit == CALIBRATION_SUCCESS) {
                    if (calibration_state == CALIBRATION_RUNNING) {
                        calibration_state = Calibrate_Si5351_Continue();
                        Report_Calibration(calibration_increament);
                        calibration_count++;
                        calibration_increament++;

                    }
                } else {
                    Calibrate_Si5351_Failed(i_opcode_data);
                    calibration_state = 0;
                    calibration_initialized = 0;
                    calibration_count = 0;
                    calibration_increament = 0;
                    Sleep(100);
                }
            } else {
                Process_Check_Calibration(command, buf, CMD_SET_CAL_DATA_PROCESSED);
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_SET_CAL_DATA_PROCESSED. FINISHED \n", line_number++);
            break;

        case CMD_SET_CAL_MODE: //This is called by the client
            G_check_calibration = 0;
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_SET_CAL_MODE. mode: %d \n",
                    line_number++, opcode_data_8_bit);
            switch (opcode_data_8_bit) {
                case 0: //Course mode
                    G_max_element = 500;
                    G_element_increament = 5;
                    break;
                case 1: //Fine Mode
                    G_max_element = 100;
                    G_element_increament = 1;
                    Freq_Set_Transceiver_Calibration(G_int, G_dec);
                    break;
            }
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_SET_CAL_MODE. mode: %d G_int: %d, G_dec: %d, FINISHED\n",
                    line_number++, opcode_data_8_bit,G_int, G_dec);
            break;

        case CMD_SET_CAL_RESET:
            reset_type = opcode_data_8_bit;
            print_time(1);
            fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_SET_CAL_RESET. reset_type %d \n",
                    line_number++, reset_type);
            Delete_PPM_ini();
            Create_PPM_ini();
            Init_PPM();
            switch (reset_type) {
                case 1:
                    Freq_Set_Transceiver_Calibration(G_int, G_dec);
                    //Spectrum_Waterfall_send_param(CMD_SET_SPECTRUM_WATERFALL_FREQ, G_tune_freq);
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_SET_CAL_RESET. reset_type %d, FINISHED\n",
                            line_number++, reset_type);
                    break;
                case 0:
                    Freq_Set_Transceiver_Calibration(0, 0);
                    Update_PPM_ini(0, 0, 0);
                    break;
            }
            freq_queue_add(G_tune_freq);
            Cal_Reset = TRUE;
            break;

        case CMD_SET_CAL_LOOSE:
            print_time(1);
            SDRcore_recv_send_param(CMD_SET_CAL_LOOSE, opcode_data_8_bit);
            fprintf(G_fp_logfile, "[%d] Process_Frequency_Calibration. CMD_SET_CAL_LOOSE. opcode_data_8_bit: %d. FINISHED\n",
                    line_number++, opcode_data_8_bit);

            break;
    }
    return status;
}




