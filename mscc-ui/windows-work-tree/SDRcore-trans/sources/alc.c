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
    float deficit;
    int meter;

    /* Guard NaN / nonsense */
    if (alc != alc)          /* NaN */
        return 0;
    if (alc >= 1.0f)
        return 0;
    if (alc <= 0.0f)
        return 100;

    /*
     * Analog phones: (1-gain)*1000 → ~1 dB of ALC = full scale.
     * Line-level VAC/WSJT runs doALC down toward 0.1, so that scale pegs
     * red on a clean CAL tone. Digital: 0–20 dB of gain reduction → 0–100
     * (green 0–20, yellow 20–50, red 50+ on the analog face).
     */
    if (G_audio_mode == DIGITAL_AUDIO || G_audio_mode == REMOTE_DIGITAL_AUDIO) {
        float g = alc;
        float db;
        if (g < 0.1f)
            g = 0.1f;
        db = -20.0f * log10f(g); /* 1.0 → 0 dB, 0.1 → 20 dB */
        meter = (int)(db / 20.0f * 100.0f + 0.5f);
        return clamp_int(meter, 0, 100);
    }

    /* 3.149: analog voice: 0-10 dB of ALC gain reduction -> 0-100.
     * Was (1-gain)*1000, full scale at 0.9 dB, so it only ever blipped to 100.
     * Set the mic gain so voice peaks show a little ALC (about 20-40 on the meter). */
    (void) deficit;
    {
        float db = -20.0f * log10f(alc);
        meter = (int)(db / 10.0f * 100.0f + 0.5f);
    }
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
    fprintf(G_fp_logfile, "[%d] ALC_Meter_thread. Thread STARTED. trans 3.149 (ALC 0.45, level meter, TX audio LPF 6k)\n", line_number++);
    while (G_all_threads_run) {
        if (G_tx_mode == TRUE && G_Do_ALC == TRUE && G_Allow_ALC_Send == TRUE) {
            new_session = TRUE;
            alc = G_ALC_gain;
            if (G_audio_mode == DIGITAL_AUDIO || G_audio_mode == REMOTE_DIGITAL_AUDIO) {
                result_average = alc_float_to_meter(alc);
            } else {
                /* trans 3.149: voice ALC meter = peak envelope / ALC ceiling. 100 = the ALC
                 * is just starting to act (clean limit of the PA). Set the mic gain so voice
                 * peaks reach about 70-100. The old meter showed only gain reduction, so it
                 * read 0 until the signal was already at the limit. */
                float target = G_ALC_limit * G_ALC_multiplier;
                float pk = G_ALC_peak_env;
                int m = 0;
                G_ALC_peak_env = 0.0f;
                if (target > 0.0f) m = (int) (pk / target * 100.0f + 0.5f);
                if (m > 100) m = 100;
                if (m < 0) m = 0;
                result_average = (uint16_t) m;
            }
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