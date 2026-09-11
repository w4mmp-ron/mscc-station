#include "extern.h"
#include "commands.h"
#define _CRT_SECURE_NO_WARNINGS 1

extern sp_float G_peak;
extern alc_state alcstate;

#define AVERAGE_LIMIT 50
#define N_DECIMAL_POINTS_PRECISION (1000) // n = 3. Three decimal points.
#define ALC_CALIBRATION_FACTOR (1.5f)


//float VU_average_buffer[AVERAGE_LIMIT] = {0};
float ALC_average_buffer[AVERAGE_LIMIT] = {0.0f};
uint16_t Result_aveage_buffer[AVERAGE_LIMIT] = { 0 };
uint16_t Average_limit = AVERAGE_LIMIT;


float ALC_average(uint8_t reset, float ALC_Value) {
    float ALC_average_result = 0;
    int i = 0;
    uint16_t average_count = 0;
    float ALC_average = 0;

    if (reset == FALSE) {
        ALC_average_buffer[0] = ALC_Value;
        for (average_count = 0; average_count < Average_limit; average_count++) {
            ALC_average = ALC_average + ALC_average_buffer[average_count];
        }
        ALC_average_result = ALC_average / (float) average_count;
        for (i = (Average_limit - 1); i > 0; i--) {
            ALC_average_buffer[i] = ALC_average_buffer[(i - 1)];
        }
    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] ALC_average RESET\n", line_number);
        for (average_count = 0; average_count < Average_limit; average_count++) {
            ALC_average_buffer[average_count] = ALC_Value;
            ALC_average = ALC_average + ALC_average_buffer[average_count];
        }
        ALC_average_result = ALC_average / (float) average_count;
    }
    ALC_average = 0.0f;
    return ALC_average_result;
}

uint16_t Result_average(uint8_t reset, int16_t ALC_Value) {
    uint32_t ALC_average_result = 0;
    int i = 0;
    uint32_t average_count = 0;
    uint32_t ALC_average = 0;

    if (reset == FALSE) {
        Result_aveage_buffer[0] = ALC_Value;
        for (average_count = 0; average_count < Average_limit; average_count++) {
            ALC_average = ALC_average + Result_aveage_buffer[average_count];
        }
        ALC_average_result = ALC_average / average_count;
        for (i = (Average_limit - 1); i > 0; i--) {
            Result_aveage_buffer[i] = Result_aveage_buffer[(i - 1)];
        }
    }
    else {
        print_time();
        fprintf(G_fp_logfile, "[%d] Result_average RESET\n", line_number);
        memset(Result_aveage_buffer, 0, Average_limit);
        /*for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
            ALC_average_buffer[average_count] = ALC_Value;
            ALC_average = ALC_average + ALC_average_buffer[average_count];
        }
        ALC_average_result = ALC_average / (float)average_count;
        */
    }
    ALC_average = 0;
    return ALC_average_result;
}

static int clamp_int(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int alc_float_to_meter(float alc)
{
    /*
     * Map ALC gain → meter 0..100 over the real doALC gain span.
     * doALC clamps gain to [0.1, 1.0]. Old scale was (1-gain)*1000, so
     * gain 0.99 → 10 and gain 0.90 already pegged at 100 — a tiny step
     * past the ALC threshold looked like a huge meter jump.
     * Linear over [min_gain, 1.0]: just engaged → small reading; hard
     * limiting near 0.1 → full scale.
     */
    const float max_gain = 1.0f;
    const float min_gain = 0.1f; /* matches doALC floor */
    float span;
    float deficit;
    int meter;

    if (alc != alc) /* NaN */
        return 0;
    if (alc >= max_gain)
        return 0;
    if (alc <= min_gain)
        return 100;

    span = max_gain - min_gain; /* 0.9 */
    deficit = max_gain - alc;
    meter = (int)(deficit / span * 100.0f + 0.5f);

    return clamp_int(meter, 0, 100);
}


void* ALC_Meter_thread(void* t) {
    char send_buf[20];
    unsigned long sleep_time = 50;
    uint8_t new_session = FALSE;
    float alc = 0.0f;
    uint16_t alc_i = 0;
    uint16_t previous_alc_i = 0;
    uint16_t result_average = 0;
    float alc_average_value = 0;
    uint16_t previous_average_limit = 0;

    Sleep(1000); //Let the subsystem initialize before processing ALC
    print_time();
    fprintf(G_fp_logfile, "[%d] ALC_Meter_thread. Thread STARTED.\n", line_number++);
    while (G_all_threads_run) {
        if (G_tx_mode == TRUE && G_Do_ALC == TRUE && G_Allow_ALC_Send == TRUE) {
            new_session = TRUE;
            alc = G_ALC_gain;
            result_average = alc_float_to_meter(alc);
            //print_time();
            //fprintf(G_fp_logfile, "[%d] ALC_Meter_thread. CMD_SET_ALC. result_average: %d, alc: %f\n",
            //    line_number++, result_average, alc);
            send_buf[0] = CMD_SET_ALC;
            memcpy(&send_buf[1], &result_average, 2);
            if (sendto(ms_sdr_s, (char*)&send_buf, 5, 0, (struct sockaddr*)&si_ms_sdr, slen) == SOCKET_ERROR) {
                print_time();
                fprintf(G_fp_logfile, "[%d] ALC_Meter_thread. sentto FAILED. error code : %s\n", line_number++, strerror(errno));
            }
        }
        else if (new_session == TRUE) {
            new_session = FALSE;
            send_buf[0] = CMD_SET_ALC;
            result_average = 0;
            memcpy(&send_buf[1], &result_average, 2);
            if (sendto(ms_sdr_s, (char*)&send_buf, 5, 0, (struct sockaddr*)&si_ms_sdr, slen) == SOCKET_ERROR) {
                print_time();
                fprintf(G_fp_logfile, "[%d] ALC_Meter_thread. sentto FAILED. error code : %s\n", line_number++, strerror(errno));
            }
            print_time();
            fprintf(G_fp_logfile, "[%d] ALC_Meter_thread. CMD_SET_ALC. result_average: %d, new_session: %d\n",
                line_number++, result_average, new_session);
        }
        Sleep(sleep_time);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] ALC_Meter_thread. Normal Exit\n", line_number++);
    pthread_exit(0);
    return (NULL);
}