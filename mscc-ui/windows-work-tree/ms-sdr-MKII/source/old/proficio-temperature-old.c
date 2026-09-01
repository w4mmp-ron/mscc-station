#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "usbavrcmd.h"
#include "SRDLL.h"
#include "extern.h"
#include "version.h"
#include "band_stack.h"
#include "last_used.h"
#include "iq.h"



#define TEMP_REFRESH 20000
#define TEMPERATURE_DRIFT_PPM_VERSION_4   0.24990000f
#define TEMPERATURE_DRIFT_PPM_VERSION_5   0.18000000f
#define STARTUP_DELAY 80000
#define AVERAGE_LIMIT 10
#define REFRESH_COUNT 3
#define STANDARD_CARRIER_BASE 10
#define TX_DELAY 10
#define NORMAL_BANDS 10
#define MAX_BANDS NORMAL_BANDS + STANDARD_CARRIER_LIMIT

INT32 E_temp = 0;
INT32 RPi_temp = 0;

struct {
    uint32_t delta_temp_array[AVERAGE_LIMIT];
    INT8 delta_average_temp;
    byte populated;
    int drift_array[AVERAGE_LIMIT];
    INT8 drift;
    byte drift_populated;
} band_data[MAX_BANDS];

INT8 G_delta_drift_int = 0;
uint8_t G_Proficio_temperature = FALSE;
uint8_t G_Proficio_Allow_Temp_Check = TRUE;
static unsigned long next = 1;

#define AVERAGE_DELTAS 0
#define APPLY_TEMP_DELTA 1
#define CHECK_TEMPERATURE 2

byte Set_band_index() {
    byte band_index = FREQ_OUT_OF_RANGE;

    if (G_band_normal != FREQ_OUT_OF_RANGE) {
        switch (G_band_normal) {
            case 160:
                band_index = 0;
                break;
            case 80:
                band_index = 1;
                break;
            case 60:
                band_index = 2;
                break;
            case 40:
                band_index = 3;
                break;
            case 30:
                band_index = 4;
                break;
            case 20:
                band_index = 5;
                break;
            case 17:
                band_index = 6;
                break;
            case 15:
                band_index = 7;
                break;
            case 12:
                band_index = 8;
                break;
            case 10:
                band_index = 9;
                break;
            default:
                band_index = STANDARD_CARRIER_BASE + G_Standard_Carrier_number;
                break;
        }
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Check_temperature -> Set_band_index ->  G_band_normal: %d, band_index: %d\n", line_number++, G_band_normal, band_index);
    return band_index;
}

INT8 average_drift(INT8 drift, byte band) {

    static uint8_t count = 0;
    static byte previous_band = FREQ_OUT_OF_RANGE;
    uint8_t average_count = 0;
    INT32 average = 0;
    int i = 0;

    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Check_temperature -> average_deltas -> Called with Delta: %d\n", line_number++,temp);
    if (band_data[band].drift_populated == 1) {
        band_data[band].drift_array[0] = drift;
        for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
            average = average + band_data[band].drift_array[average_count];
        }
        band_data[band].drift = average / average_count;
        for (i = (AVERAGE_LIMIT - 1); i > 0; i--) {
            band_data[band].drift_array[i] = band_data[band].drift_array[(i - 1)];
        }
    } else {
        for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
            band_data[band].drift_array[average_count] = drift;
        }
        band_data[band].drift = drift;
        band_data[band].drift_populated = 1;
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Check_temperature -> average_drift -> Finished. drift Received: %d,  average drift %d\n",
    //       line_number++, drift, band_data[band].drift);
    return (band_data[band].drift);
}

INT32 average_deltas(INT32 temp, byte band) {
    uint8_t average_count = 0;
    INT32 average = 0;
    int i = 0;

    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Check_temperature -> average_deltas -> Called with Delta: %d\n", line_number++,temp);
    if (band_data[band].populated == 1) {
        band_data[band].delta_temp_array[0] = temp;

        for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
            average = average + band_data[band].delta_temp_array[average_count];
        }
        band_data[band].delta_average_temp = average / average_count;
        for (i = (AVERAGE_LIMIT - 1); i > 0; i--) {
            band_data[band].delta_temp_array[i] = band_data[band].delta_temp_array[(i - 1)];
        }
    } else {
        for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
            band_data[band].delta_temp_array[average_count] = temp;
        }
        band_data[band].populated = 1;
        band_data[band].delta_average_temp = temp;
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Check_temperature -> average_deltas -> Finished. temp: %d,  Delta %d\n",
    //         line_number++, temp, band_data[band].delta_average_temp);
    return (band_data[band].delta_average_temp);
}

INT8 Apply_PPM(uint8_t band, INT32 delta_temp) {
    float current_freq = 0;
    INT8 drift = 0;
    float drift_ppm = 0.0f;

    //This determines the freq in Hz adjustment for temperature drift. 
    //It assumes a linear drift based on temperature    
    //print_time(0);
    //fprintf(G_fp_logfile,"[%d] Check_temperature -> Apply_PPM -> Called. delta_temp: %d\n", line_number++, delta_temp);
    //print_time(0);
    //fprintf(G_fp_logfile,"[%d] Check_temperature -> Apply_PPM -> G_pcb_version : % d\n", line_number++, G_pcb_version);
    switch (G_pcb_version) {
        case 4:
            drift_ppm = TEMPERATURE_DRIFT_PPM_VERSION_4;
            break;
        case 5:
            drift_ppm = TEMPERATURE_DRIFT_PPM_VERSION_5;
            break;
        default:
            drift_ppm = TEMPERATURE_DRIFT_PPM_VERSION_5;
            break;
    }
    current_freq = (float) G_tune_freq / 1000000;
    current_freq = current_freq * (float) ((drift_ppm * (float) delta_temp));
    //Convert float to int using C short cut
    drift = (INT8) (current_freq);
    if (drift <= -128 && drift >= 128) {
        drift = 0;
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Check_temperature -> Apply_PPM -> Finished. drift: %d\n", line_number++, drift);
    return drift;
}

/*int myrand(void) {
    int num = 0;
    num = (rand() % (1500 - 100 + 1)) + 100;
    return num;
}*/
int myrand(void) {
    int num = 0;
    num = (rand() % (1500 - 100 + 1)) + 100;
    return num;
}

void Initialize_band_data() {
    int band = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_temperature -> Initialize_band_data -> Called. \n", line_number++);
    for (band = 0; band < MAX_BANDS; band++) {
        band_data[band].delta_average_temp = 0;
        memset(band_data[band].delta_temp_array, 0, AVERAGE_LIMIT);
        memset(band_data[band].drift_array, 0, AVERAGE_LIMIT);
        band_data[band].populated = 0;
        band_data[band].drift_populated = 0;
    }
}

void *Check_temperature(void *t) {
    INT32 iTemperature = 0;
    int sleep_time = 3000;
    uint8_t band = 201;
    uint8_t previous_band = 201;
    uint8_t transceiver_read = FALSE;
    int random = 0;
    int r;
    INT32 delta_temp = 0;
    INT32 previous_delta_temp = 0;
    INT32 average_delta_temp = 0;
    INT8 previous_drift = 100;
    int tx_delay = 0;
    INT8 drift = 0;
    INT8 drift_average = 0;
    uint8_t update_flag = 0;
    FILE *RPi_temperature_file;
    double T;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_temperature -> Started. \n", line_number++);
    Sleep(7000); //Allow other subsystems to start and stabilize
    next = 3000;
    Initialize_band_data();
    while (G_all_threads_run) {
        if (!G_dll_active) {
            if (G_Calibration_reset == TRUE) {
                Initialize_band_data();
                G_Calibration_reset = FALSE;
            }
            iTemperature = 0;
            if (G_Transceiver_Busy == TRUE && G_check_potentia == TRUE) {
                random = myrand();
                if (random > 3000) {
                    random = 1500;
                }
                Sleep(random);
                transceiver_read = FALSE;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_temperature -> Sleeping for: %d ms \n", line_number++, random);
            } else {
                r = usbControlMsgIN(CMD_GET_TRANSCEIVER_TEMP, 0xA55A, 0, (char*) &iTemperature, sizeof (iTemperature));
                transceiver_read = TRUE;
                G_Proficio_temperature = TRUE;
            }
            if (transceiver_read == TRUE && G_Proficio_Allow_Temp_Check == TRUE) {
                E_temp = iTemperature;
                band = Set_band_index();
                if (band != previous_band) {
                    previous_delta_temp = 128;
                    previous_drift = 128;
                    previous_band = band;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Check_temperature. band: %d \n", line_number++, band);
                }
                if (band != FREQ_OUT_OF_RANGE) {
                    if (E_temp > 0) {
                        if (E_temp < G_Calibration_temperature) {
                            delta_temp = (G_Calibration_temperature - E_temp) * -1;
                        } else {
                            delta_temp = (E_temp - G_Calibration_temperature);
                        }
                        Gui_send_param(CMD_GET_TRANSCEIVER_TEMP, E_temp);
                    }
                    if (previous_delta_temp != delta_temp) {
                        average_delta_temp = average_deltas(delta_temp, band); // Average the delta temperature values
                        drift = Apply_PPM(band, average_delta_temp); //Apply the PPM value to the average delta temperature and receive a drift value
                        drift_average = average_drift(drift, band); //Average the drift values
                        G_delta_drift_int = drift_average;
                        if (previous_drift != G_delta_drift_int) {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Check_temperature -> previous_drift: %d, G_delta_drift_int %d\n",
                                    line_number++, previous_drift, G_delta_drift_int);
                            cw_send_parameters(CMD_SET_DRIFT, G_delta_drift_int, 1);
                            Sleep(100);
                            cw_send_parameters(CMD_SET_DRIFT, G_delta_drift_int, 1);
                            Sleep(100);
                            Gui_send_param(CMD_SET_DRIFT, G_delta_drift_int);
                            freq_queue_add(G_tune_freq);
                            previous_drift = G_delta_drift_int;
                            if (update_flag == TRUE) {
                                update_flag = FALSE;
                            }
                        }
                        previous_delta_temp = delta_temp;
                    }
                }
            } else {
                if (G_Proficio_Allow_Temp_Check == FALSE) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Check_temperature -> G_Proficio_Allow_Temp_Check: %d\n",
                            line_number++, G_Proficio_Allow_Temp_Check);
                    update_flag = TRUE;
                }
            }
            G_Proficio_temperature = FALSE;
        } else {
            if (G_SetModeRxTX) {
                tx_delay = TX_DELAY;
            } else {
                if (G_dll_active) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Check_temperature -> DLL is Active\n", line_number++);
                } else {
                    if (tx_delay != 0) {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Check_temperature -> tx_delay not zero. tx_delay: %d\n", line_number++, tx_delay);
                    }
                }
            }
        }
        RPi_temperature_file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
        if (RPi_temperature_file != NULL) {
            fscanf(RPi_temperature_file, "%lf", &T);
            T /= 1000;
            RPi_temp = (INT32) T;
            Gui_send_param(CMD_RPI_SET_TEMPERATURE, RPi_temp);
            fclose(RPi_temperature_file);
        }
        Sleep(sleep_time);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_temperature -> Normal Exit\n", line_number++);
    pthread_exit(0);
    return NULL;
}

/* [] END OF FILE */
