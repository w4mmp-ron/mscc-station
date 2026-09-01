#include "extern.h"
#include "iq.h"
#include "version.h"


#define AVERAGE_LIMIT 20
#define BAND_LIMIT 10
#define ARRAY_ELEMENTS 25
#define APPLY_LIMIT 1
#define APPLY_COUNT 3
#define BAND_CHANGE_LIMIT 6
#define IN_RUSH_COUNT 2
#define IN_RUSH_REDUCTION 0.98f

extern uint8_t G_DSP_Busy;

uint8_t G_temperature_received = FALSE;
uint8_t G_bias_received = FALSE;
//float G_VU_ratio = 0.0f;

float average_temperature[AVERAGE_LIMIT] = {0.0f};
float drive_level_array[AVERAGE_LIMIT] = {0.0f};
float smoothing_average_array[AVERAGE_LIMIT] = {0.0f};
float drive_average_result_array[AVERAGE_LIMIT] = {0.0f};
uint32_t bias_array[AVERAGE_LIMIT] = {0};

float G_drive_average_result = 0.0f;
float previous_temperature = 0.0f;


uint32_t Get_Bias_average(uint32_t bias) {
    uint32_t Bias_average_result = 0;
    int average_count = 0;
    uint32_t Bias_average = 0;
    int i = 0;
    static uint32_t bias_valid[3] = {0};
    static int invalid_bias_count = 0;

    bias_valid[0] = bias;
    if (((bias_valid[0] >= bias_valid[1]) && (bias_valid[1] >= bias_valid[2])) || //Bias going up
            ((bias_valid[2] >= bias_valid[1]) && (bias_valid[1] >= bias_valid[0]))) { //Bias going down
        bias_array[0] = bias;
        for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
            Bias_average = Bias_average + bias_array[average_count];
        }
        Bias_average_result = Bias_average / average_count;
        for (i = (AVERAGE_LIMIT - 1); i > 0; i--) {
            bias_array[i] = bias_array[(i - 1)];
        }
        //print_time();
        //fprintf(G_fp_logfile, "[%d] Get_Bias_average. Bias: %d \n", line_number++,Bias_average_result);
    } else {
        if (invalid_bias_count++ > 10) {
            print_time();
            fprintf(G_fp_logfile, "[%d] Get_Bias_average. Bias Not Valid \n", line_number++);
            invalid_bias_count = 0;
        }
    }
    bias_valid[2] = bias_valid[1];
    bias_valid[1] = bias_valid[0];
    return Bias_average_result;
}

int Average_Temperature() {
    static int first_pass = TRUE;
    static int count = 0;
    float temperature_average = 0.0f;
    float temperature_average_result = 0.0f;
    int average_count = 0;
    int temperature_i = 0;
    int i = 0;

    if (first_pass) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Drive_Level. First Pass \n", line_number++);
        for (count = 0; count < AVERAGE_LIMIT; count++) {
            average_temperature[count] = G_temperature;
        }
        for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
            temperature_average = temperature_average + average_temperature[average_count];
        }
        temperature_average_result = temperature_average / (float) average_count;
        count = 0;
        first_pass = FALSE;
    } else {
        average_temperature[0] = G_temperature;
        for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
            temperature_average = temperature_average + average_temperature[average_count];
        }
        temperature_average_result = temperature_average / (float) average_count;
        for (i = (AVERAGE_LIMIT - 1); i > 0; i--) {
            average_temperature[i] = average_temperature[(i - 1)];
        }
    }
    temperature_i = (int) temperature_average_result;
    return temperature_i;
}

float Driver_Get_QRO_Power() {
    static float configured_drive = 0.0f;

    switch (G_mode) {
        case 'T':
            configured_drive = get_QRO_power_level(G_band, TUNE_POWER);
            break;
        case 'C':
            configured_drive = get_QRO_power_level(G_band, CW_POWER);
            break;
        case 'L':
            configured_drive = get_QRO_power_level(G_band, LSB_POWER);
            break;
        case 'U':
            configured_drive = get_QRO_power_level(G_band, USB_POWER);
            break;
        case 'A':
            configured_drive = get_QRO_power_level(G_band, LSB_POWER);
            break;
        default:
            print_time();
            fprintf(G_fp_logfile, "[%d] Drive_Manager. Driver_Get_QRO_Power. INVALID MODE\n",line_number++);
            break;
    }
    //G_VU_ratio = G_mic_volume / configured_drive;
    print_time();
    fprintf(G_fp_logfile, "[%d] Drive_Manager. Driver_Get_QRO_Power. configured_drive %f\n",
            line_number++, configured_drive);
    return configured_drive;
}

float Driver_Get_QRP_Power() {
    static float configured_drive = 0.0f;

    switch (G_mode) {
        case 'T':
            configured_drive = get_QRP_power_level(G_band, TUNE_POWER);
            break;
        case 'C':
            configured_drive = get_QRP_power_level(G_band, CW_POWER);
            break;
        case 'L':
            configured_drive = get_QRP_power_level(G_band, LSB_POWER);
            break;
        case 'U':
            configured_drive = get_QRP_power_level(G_band, USB_POWER);
            break;
        case 'A':
            configured_drive = get_QRP_power_level(G_band, LSB_POWER);
            break;
        default:
            print_time();
            fprintf(G_fp_logfile, "[%d] Drive_Manager. Driver_Get_QRP_Power. INVALID MODE\n",line_number++);
    }
    //G_VU_ratio = G_mic_volume / configured_drive;
    print_time();
    fprintf(G_fp_logfile, "[%d] Drive_Manager. Driver_Get_QRP_Power. configured_drive %f\n",
            line_number++, configured_drive);
    return configured_drive;
}

float Driver_Get_ALC_Limit() {
    static float alc_limit = 0.0f;

    switch (G_mode) {
        case 'T':
            alc_limit = get_QRP_power_level(G_band, TUNE_POWER);
            break;
        case 'C':
            alc_limit = get_QRP_power_level(G_band, CW_POWER);
            break;
        case 'L':
            alc_limit = get_QRP_power_level(G_band, LSB_POWER);
            break;
        case 'U':
            alc_limit = get_QRP_power_level(G_band, USB_POWER);
            break;
        case 'A':
            alc_limit = get_QRP_power_level(G_band, LSB_POWER);
            break;
        default:
            print_time();
            fprintf(G_fp_logfile, "[%d] Drive_Manager. Driver_Get_ALC_Limit. INVALID MODE\n",line_number++);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Drive_Manager. Driver_Get_ALC_Limit. ALC_Limit %f\n",
            line_number++, alc_limit);
    return alc_limit;
}

void *Drive_Manager(void *t) {
    uint32_t Bias_average = 0;
    uint32_t Average_Temp = 0;
    char mode = 10;
    uint8_t qrp_mode = 10;
    uint8_t band = 20;
    float mic_volume = 0.0f;
    float drive = 0.0f;

    print_time();
    fprintf(G_fp_logfile, "[%d] Drive_Manager. Started. Sleeping for 6 seconds.\n", line_number++);
    Sleep(6000);
    while (G_all_threads_run) {
        if (G_Power_Values_initialized == TRUE) {
            if (G_power_file_needs_updated == TRUE) {
                Update_Proficio_User_Power_ini();
                Update_amplifier_calibration();
                Init_Power_All();
                qrp_mode = 10;
                mode = 'N';
                G_power_file_needs_updated = FALSE;
            }
            if (qrp_mode != G_QRP_mode || mode != G_mode || band != G_band || mic_volume != G_mic_volume) {
                switch (G_QRP_mode) {
                    case 0:
                        print_time();
                        fprintf(G_fp_logfile, "[%d] Drive_Manager. In QRO mode \n", line_number++);
                        drive = Driver_Get_QRO_Power();
                        break;
                    case 1:
                        print_time();
                        fprintf(G_fp_logfile, "[%d] Drive_Manager. In QRP mode \n", line_number++);
                        drive = Driver_Get_QRP_Power();
                        break;
                }
                //if (mystate.txPower != drive) {
                mystate.txPower = drive;
                //}
                //G_ALC_limit = Driver_Get_ALC_Limit();
                G_ALC_limit = drive;
                print_time();
                fprintf(G_fp_logfile, "[%d] Drive_Manager. G_ALC_limit: %f \n", line_number++, G_ALC_limit);
                qrp_mode = G_QRP_mode;
                mode = G_mode;
                band = (uint8_t) G_band;
                mic_volume = G_mic_volume;
            }

        }
        if (G_bias_received == TRUE) {
            Bias_average = Get_Bias_average(G_potentia_bias);
            G_bias_received = FALSE;
        }
        if (G_temperature_received == TRUE) {
            Average_Temp = Average_Temperature();
            G_temperature_received = FALSE;
        }

        Sleep(1);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Drive_Manager. Normal Exit\n", line_number++);
    pthread_exit(0);
    return (NULL);
}