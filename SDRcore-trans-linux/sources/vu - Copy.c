#include "extern.h"
#include "commands.h"
#include "version.h"
#include "dsputils.h"
#include "pthread.h"


//#define AVERAGE 100
extern sp_float G_peak;
extern alc_state alcstate;

#define AVERAGE_LIMIT 200
#define N_DECIMAL_POINTS_PRECISION (1000) // n = 3. Three decimal points.
#define VU_CALIBRATION_FACTOR (0.85f)
#define ALC_CALIBRATION_FACTOR (1.5f)
#define OVERDRIVEN 254
#define ZERO_COUNT_LIMIT 20

byte G_Run_ALC = FALSE;

//float VU_average_buffer[AVERAGE_LIMIT] = {0};
float ALC_average_buffer[AVERAGE_LIMIT] = {0};

float ALC_average(uint8_t reset, float ALC_Value) {
    float ALC_average_result = 0;
    int i = 0;
    uint8_t average_count = 0;
    float ALC_average = 0;

    if (reset == FALSE) {
        ALC_average_buffer[0] = ALC_Value;
        for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
            ALC_average = ALC_average + ALC_average_buffer[average_count];
        }
        ALC_average_result = ALC_average / (float) average_count;
        for (i = (AVERAGE_LIMIT - 1); i > 0; i--) {
            ALC_average_buffer[i] = ALC_average_buffer[(i - 1)];
        }
    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] ALC_average RESET\n", line_number);
        for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
            ALC_average_buffer[average_count] = ALC_Value;
            ALC_average = ALC_average + ALC_average_buffer[average_count];
        }
        ALC_average_result = ALC_average / (float) average_count;
    }
    ALC_average = 0.0f;
    return ALC_average_result;
}

/*float Peak_average(uint8_t reset) {
    float VU_average_result = 0.0f;
    int i = 0;
    uint8_t average_count = 0;
    float VU_average = 0.0f;
    float VU_value = 0.0f;
    float peak = 0.0f;

    peak = G_peak * G_VU_ratio;
    if (reset == FALSE) {
        VU_average_buffer[0] = peak;
        for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
            VU_average = VU_average + VU_average_buffer[average_count];
        }
        VU_average_result = VU_average / (float) average_count;
        for (i = (AVERAGE_LIMIT - 1); i > 0; i--) {
            VU_average_buffer[i] = VU_average_buffer[(i - 1)];
        }
        //print_time();
        //fprintf(G_fp_logfile, "[%d] Peak_average -> G_peak: %f, G_VU_ratio: %f, peak: %f, average_result: %f\n", 
        //	line_number, G_peak, G_VU_ratio, peak, VU_average_result);
    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] Peak_average RESET\n", line_number);
        for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
            VU_average_buffer[average_count] = VU_value;
            VU_average = VU_average + VU_average_buffer[average_count];
        }
        VU_average_result = VU_average / (float) average_count;
    }
    VU_average = 0.0f;
    return VU_average_result;
}*/

void *ALC_thread(void *t) {
    char send_buf[20];
    unsigned long sleep_time = 10;
    unsigned long loop_sleep_time = 10;
    int peakblocks = 0;
    uint8_t reset = FALSE;
    float alc = 0.0f;
    uint16_t alc_i = 0;
    uint16_t previous_alc_i = 0;
    float alc_average_value = 0;
    uint8_t zero_count = 0;

    Sleep(5000); //Let the subsystem initialize before processing VU
    print_time();
    fprintf(G_fp_logfile, "[%d] ALC_thread. Thread STARTED.\n", line_number++);
    while (G_all_threads_run) {
        if (G_tx_mode == 1 && G_Run_ALC == TRUE) {
            if (reset == TRUE) {
                zero_count = 0;
                alc = 1.0000f;
                alc_average_value = ALC_average(reset, alc);
                send_buf[0] = CMD_SET_ALC;
                alc_i = 0;
                memcpy(&send_buf[1], &alc_i, 2);
                if (sendto(ms_sdr_s, (char *) &send_buf, 5, 0, (struct sockaddr *) &si_ms_sdr, slen) == SOCKET_ERROR) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] ALC_thread. sentto FAILED. error code : %s\n",
                            line_number++, strerror(errno));
                }
                reset = FALSE;
            } else {
                if (G_ALC_gain < 1.000f) {
                    alc = G_ALC_gain;
                } else {
                    alc = 1.0000f;
                }
                alc_average_value = ALC_average(reset, alc);
            }
            alc_average_value = 1.0000f - alc_average_value;
            alc_average_value = alc_average_value * ALC_CALIBRATION_FACTOR;
            alc_i = ((uint16_t) (alc_average_value * N_DECIMAL_POINTS_PRECISION) % N_DECIMAL_POINTS_PRECISION);
            if (previous_alc_i != alc_i && zero_count++ >= ZERO_COUNT_LIMIT) {
                if (G_Do_ALC == TRUE) {
                    send_buf[0] = CMD_SET_ALC;
                    memcpy(&send_buf[1], &alc_i, 2);
                    if (sendto(ms_sdr_s, (char *) &send_buf, 5, 0, (struct sockaddr *) &si_ms_sdr, slen) == SOCKET_ERROR) {
                        print_time();
                        fprintf(G_fp_logfile, "[%d] ALC_thread. sentto FAILED. error code : %s\n", line_number++, strerror(errno));
                    }
                }
                previous_alc_i = alc_i;
            }
            Sleep(sleep_time);
        }
        peakblocks = 0;
        mystate.overdriven = 0;
        reset = TRUE;
        /*if (G_alc_count > 0) {
            print_time();
            fprintf(G_fp_logfile, "[%d] ALC_thread. G_alc_count: %ld \n",
                    line_number++, G_alc_count);
            G_alc_count = 0;
        }*/
        if (zero_count >= ZERO_COUNT_LIMIT) {
            alc_i = 0;
            send_buf[0] = CMD_SET_ALC;
            memcpy(&send_buf[1], &alc_i, 2);
            if (sendto(ms_sdr_s, (char *) &send_buf, 5, 0, (struct sockaddr *) &si_ms_sdr, slen) == SOCKET_ERROR) {
                print_time();
                fprintf(G_fp_logfile, "[%d] ALC_thread. sentto FAILED. error code : %s\n",
                        line_number++, strerror(errno));
            }
        }
        Sleep(loop_sleep_time);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] ALC_thread. Exit\n", line_number++);
    pthread_exit(0);
    return (NULL);
}