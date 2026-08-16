#include <math.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdint.h>
#include <stdio.h>
#include "extern.h"
#include "dsputils.h"

#define KEY 8
#define KEY_UP 1
#define KEY_DOWN 0
#define PA_SAMPLE_TYPE      paFloat32
typedef float SAMPLE;

extern state mystate;

typedef struct {
    float left_phase;
    float right_phase;
}
paTestData;

PaStream* t_stream;
PaStreamParameters t_inputParameters, t_outputParameters;

float G_side_tone_pitch = 600.0f;
uint8_t G_Pitch_changed = 0;
uint8_t G_key = KEY_UP;
float G_t_volumeLevel = 0.1f;
float call_back_volume = 0.0f;
int beenhere = 0, active = 0, rampdir = 0;
float ramp, level, myfreq, oldfreq = 0.0f, phaseinc, phaseaccum;
float myramp;
float ramptimems = 10.0f; // keying up/down ramp time in ms
float txsamplerate;
int j;

static int t_sdrAudioCallback(const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData);

/* This routine will be called by the PortAudio engine when audio is needed.
 ** It may be called at interrupt level on some machines so don't do anything
 ** that could mess up the system like calling malloc() or free().
 */
static int t_sdrAudioCallback(const void* inputBuffer, void* outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void* userData) {
    unsigned int i;
    float* out = (sp_float*) outputBuffer;
    txsamplerate = mystate.samplerate;

    if (!beenhere) {
        beenhere = 1;
        ramp = 1.0f / (txsamplerate * ramptimems);
        level = 0.0f;
        rampdir = 0;
        active = 0;
        ramp = 0.0f;
    }
    // if key isn't down and shaping isn't previously active, return
    //if ((G_key == KEY_UP) && (active == 0)) return paContinue;
    // new key down
    if ((G_key == KEY_DOWN) && (active == 0)) {
        active = 1;
        rampdir = 1;
        myramp = 1.0f + ramp;
        if (oldfreq != G_side_tone_pitch) {
            phaseinc = (float) TPI / (txsamplerate / G_side_tone_pitch);
            phaseaccum = 0.0f;
            oldfreq = G_side_tone_pitch;
            rampdir = 1; // ramp UP, because this is a new keydown command
        }
    }
    if (G_Pitch_changed) {
        active = 1;
        rampdir = 1;
        myramp = 1.0f + ramp;
        if (oldfreq != G_side_tone_pitch) {
            phaseinc = (float) TPI / (txsamplerate / G_side_tone_pitch);
            phaseaccum = 0.0f;
            oldfreq = G_side_tone_pitch;
            rampdir = 1; // ramp UP, because this is a new keydown command
            G_Pitch_changed = 0;
        }
    }
    //if ((G_key == KEY_UP) && (active == 1)) {
    //    rampdir = 0; // make sure we're in ramp DOWN mode
    //    myramp = ramp;
    //}
    for (i = 0; i < framesPerBuffer; i++) {
        *out++ = (sp_float) sin(phaseaccum) * call_back_volume;
        *out++ = (sp_float) cos(phaseaccum) * call_back_volume;
        phaseaccum += phaseinc;
        phaseaccum = (float) atan2(sin(phaseaccum), cos(phaseaccum));
        //if (rampdir == 0) {
        //    myramp *= ramp;
        //    if (myramp <= 0.000001) {
        //       active = 0;
        //       break; // break out if ramping down and level reaches "close enough" to zero
        //   }
        //}
        // ramping UP, all samples get tone.
        if (rampdir == 1) myramp *= (1.0f + ramp);
    }
    return paContinue;
}

int manage_sidetone_stream(int start_stop, int device, int channels) {
    PaError err;
    int status = 0;
    static int current_device = 0;
    static int current_channels = 0;
    paTestData data;

    if (start_stop == 0) {
        current_device = device;
        current_channels = channels;
    }
    switch (start_stop) {
        case 1:
            print_time();
            fprintf(G_fp_logfile, "[%d] manage_sidetone_stream -> Starting Stream.  Device: %d, Channels %d\n",
                    line_number++, current_device, current_channels);
            call_back_volume = 0.0f;
            err = Pa_IsFormatSupported(NULL, &t_outputParameters, 96000);
            if (err != paNoError) {
                print_time();
                fprintf(G_fp_logfile, "[%d] manage_sidetone_stream ->   Pa_IsFormatSupported FAILED. Device: %d, Channels %d\n",
                        line_number++, current_device, current_channels);
            }
            err = Pa_OpenStream(
                    &t_stream,
                    NULL,
                    &t_outputParameters,
                    (int) mystate.samplerate,
                    mystate.frames,
                    paClipOff, /* paClipOff, */ /* we won't output out of range samples so don't bother clipping them */
                    t_sdrAudioCallback,
                    &data);
            if (err != paNoError) {
                const PaHostErrorInfo* lpError = Pa_GetLastHostErrorInfo();
                print_time();
                fprintf(G_fp_logfile, "[%d] manage_sidetone_stream -> Open Stream Failed: PA ERROR: %s\n",
                        line_number++, lpError->errorText);
                MessageBoxA(NULL, "manage_sidetone_stream -> Pa Open Stream FAILED. Send logs to Multus SDR, LLC", "SDRcore-recv", MB_ICONASTERISK | MB_OK);
                status = err;
            } else {
                err = Pa_StartStream(t_stream);
                if (err != paNoError) {
                    status = err;
                    const PaHostErrorInfo* lpError = Pa_GetLastHostErrorInfo();
                    print_time();
                    fprintf(G_fp_logfile, "[%d] manage_sidetone_stream -> Start Stream Failed: PA ERROR: %s\n",
                            line_number++, lpError->errorText);
                    MessageBoxA(NULL, "manage_sidetone_stream -> Pa Start Stream FAILED. Send logs to Multus SDR, LLC", "SDRcore-trans", MB_ICONASTERISK | MB_OK);
                }
            }
            break;
        case 0:
            print_time();
            fprintf(G_fp_logfile, "[%d] manage_sidetone_stream -> Stoping Stream.  New device: %d, New Channels %d\n", line_number++, device, channels);
            Pa_StopStream(t_stream);
            Pa_CloseStream(t_stream);
            if (channels > 2) {
                print_time();
                fprintf(G_fp_logfile, "[%d] manage_sidetone_stream -> Device %d -> Channel Number: %d greater than 2. Setting channels to 2.\n",
                        line_number++, device, channels);
                channels = 2;
            }
            t_outputParameters.device = device;
            t_outputParameters.channelCount = channels;
            t_outputParameters.sampleFormat = PA_SAMPLE_TYPE;
            t_outputParameters.suggestedLatency = Pa_GetDeviceInfo(t_outputParameters.device)->defaultLowOutputLatency;
            t_outputParameters.hostApiSpecificStreamInfo = NULL;
            break;
    }
    return status;
}

void Side_Tone_Volume_Ramp_Up(float target_volume) {
    float volume = 0.0f;
    print_time();
    fprintf(G_fp_logfile, "[%d] Speaker_Volume_Ramp_Up -> target_volume: %f\n", line_number++, target_volume);
    while (volume < target_volume) {
        Sleep(1);
        volume += 0.01f;
        call_back_volume = volume;
    }
}

void *Read_Key_thread(void *t) {
    uint8_t reported = 0;
    uint8_t key = 0;
    int first_key_count = 0;

    print_time();
    fprintf(G_fp_logfile, "[%d] Read_Key_thread -> Started\n", line_number++);
    G_key = KEY_UP;
    call_back_volume = 0.0f;
    while (G_all_threads_run) {
        if (G_mode == 'C' && G_tx_mode == TRUE) {
            if (reported == 0) {
                print_time();
                fprintf(G_fp_logfile, "[%d] Read_Key_thread -> G_mode: %c, G_tx_mode: %d \n",
                        line_number++, G_mode, G_tx_mode);
                reported = 1;
            }
            Sleep(1);
            //key = digitalRead(KEY);
            if (first_key_count++ > 1000) {
                if (key == KEY_DOWN) {
                    call_back_volume = G_t_volumeLevel;
                }else{
                    call_back_volume = 0.0f;
                }
            }
        } else {
            call_back_volume = 0.0f;
            reported = 0;
            G_key = KEY_UP;
            first_key_count = 0;
            Sleep(1);
        }
    }
    pthread_exit(0);
    return (NULL);
}

