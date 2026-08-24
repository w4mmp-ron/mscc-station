/********************** SDRdemo *************************
 *	SDR Demo using Multus SDR LLC Hardware				*
 *	SDRcore engine inclusions provided under license	*
 *	from James L Barber, AKA Silicon Pixels, Spokane	*
 *	Radio et al											*
 *********************************************************/
#define RON
#include "extern.h"
#include "mscc_resampler.h"
#include "commands.h"
#include "remote_mic.h"
//#include "sdrcoretx.h"
//#include "dsputils.h"


#define PA_SAMPLE_TYPE      paFloat32
#define MAX_NULL 38
typedef float SAMPLE;
int modeChanged = FALSE;
int inputchannels = 0;
int myAudioAPItype = 0;
sp_float *inbuffer, *outbuffer;
sp_float wold = 0;
sp_cplx incplx[4097];
sp_cplx outcplx[4096];
float volumeLevel = -0.0f; //Start with the volume muted.

char G_l_path[MAX_PATH] = { 0 };

state mystate;
PaStream *stream;
alc_state alcstate;
micproc_state micstate;

const PaDeviceInfo* lpInfo;
PaStreamParameters inputParameters, outputParameters;

#ifdef RON
#define MAX_INPUT_DEVICES 50
//For the UDP_Thread
pthread_t p_UDP_thread;
int UDP_thread_rc;

//For the flusher Thread
pthread_t p_Flusher_thread;
int Flusher_thread_rc;

int G_all_threads_run = 0;
uint8_t G_network_initialized = 0;

pthread_t p_Overdriven_thread;
int Overdriven_thread_rc;


pthread_t p_ALC_Meter_thread;
int ALC_Meter_thread_rc;

pthread_t p_Drive_thread;
int Drive_thread_rc;

FILE *G_fp_logfile;
int line_number = 0;
int G_input_device_index = NO_INPUT_DEVICE;
int G_digital_input_device_index = NO_INPUT_DEVICE;
struct input_devices G_input_devices[MAX_INPUT_DEVICES];
struct input_devices G_digital_input_devices[MAX_INPUT_DEVICES];

sp_float G_mic_volume = 0.5f;
uint16_t G_VU_value = 0;
sp_float G_peak = 0.0f;
sp_float iMult = 1.0f;
sp_float qMult = 1.0f;
uint8_t G_DSP_Busy = FALSE;
int G_null_count = 0;
uint8_t transmit = TRUE;
#endif

/****** NOTE: Declared external in other modules and used as a global ******/
state mystate;

static int sdrAudioCallback(const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData);
static int sdrMicOnlyCallback(const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData);
static int sdrIqPlayOnlyCallback(const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData);

static int gNumNoInputs = 0;

/*
 * Dual-stream (Pulse digi mic in + ALSA Proficio I/Q out): PortAudio rejects
 * mixed-host full duplex (-9993). Mic capture → ring @ 96k; I/Q play reads ring + DSP.
 * Non-96k operator mic: open capture at device rate, Oboe upsample into ring at 96k.
 */
#define MIC_RING_FRAMES 16384
static float g_mic_ring[MIC_RING_FRAMES * 2];
static volatile unsigned g_mic_w = 0;
static volatile unsigned g_mic_r = 0;
static int g_split_streams = 0;
static PaStream *stream_mic = NULL; /* capture-only when split; stream = I/Q play */
static MsccResampler *g_mic_resampler = NULL; /* NULL = identity (mic @ 96k) */
static double g_mic_rate = 0.0;

static void mic_ring_reset(void)
{
    g_mic_w = 0;
    g_mic_r = 0;
}

static void mic_ring_write(const SAMPLE *stereo, unsigned long frames, int ch)
{
    unsigned long i;
    for (i = 0; i < frames; i++) {
        unsigned next = (g_mic_w + 1u) % MIC_RING_FRAMES;
        if (next == g_mic_r)
            break;
        if (ch >= 2) {
            g_mic_ring[g_mic_w * 2u] = stereo[i * 2u];
            g_mic_ring[g_mic_w * 2u + 1u] = stereo[i * 2u + 1u];
        } else {
            g_mic_ring[g_mic_w * 2u] = stereo[i];
            g_mic_ring[g_mic_w * 2u + 1u] = stereo[i];
        }
        g_mic_w = next;
    }
}

static void mic_ring_put_frame(const float frame[2], void *userdata)
{
    (void)userdata;
    unsigned next = (g_mic_w + 1u) % MIC_RING_FRAMES;
    if (next == g_mic_r)
        return; /* full — drop */
    g_mic_ring[g_mic_w * 2u] = frame[0];
    g_mic_ring[g_mic_w * 2u + 1u] = frame[1];
    g_mic_w = next;
}

static void mic_ring_read(SAMPLE *stereo, unsigned long frames)
{
    unsigned long i;
    for (i = 0; i < frames; i++) {
        if (g_mic_r == g_mic_w) {
            stereo[i * 2u] = 0.0f;
            stereo[i * 2u + 1u] = 0.0f;
        } else {
            stereo[i * 2u] = g_mic_ring[g_mic_r * 2u];
            stereo[i * 2u + 1u] = g_mic_ring[g_mic_r * 2u + 1u];
            g_mic_r = (g_mic_r + 1u) % MIC_RING_FRAMES;
        }
    }
}

static void mic_resampler_destroy(void)
{
    if (g_mic_resampler != NULL) {
        mscc_resampler_destroy(g_mic_resampler);
        g_mic_resampler = NULL;
    }
    g_mic_rate = 0.0;
}

/*
 * Prefer I/Q rate (96k) for mic capture. If unsupported, device default then
 * common rates. Returns a rate > 0 (last resort = iq_rate).
 */
static double pick_mic_sample_rate(const PaStreamParameters *in_params,
                                   const PaDeviceInfo *idi,
                                   double iq_rate)
{
    PaStreamParameters probe = *in_params;
    PaError chk;

    chk = Pa_IsFormatSupported(&probe, NULL, iq_rate);
    if (chk == paFormatIsSupported)
        return iq_rate;

    if (idi != NULL && idi->defaultSampleRate > 0.0) {
        chk = Pa_IsFormatSupported(&probe, NULL, idi->defaultSampleRate);
        if (chk == paFormatIsSupported)
            return idi->defaultSampleRate;
    }

    {
        static const double candidates[] = { 48000.0, 44100.0, 32000.0, 16000.0 };
        size_t i;
        for (i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
            if (candidates[i] == iq_rate)
                continue;
            chk = Pa_IsFormatSupported(&probe, NULL, candidates[i]);
            if (chk == paFormatIsSupported)
                return candidates[i];
        }
    }
    return iq_rate;
}

/* Mic (or silence) → modulate → I/Q stereo out. */
static void process_mic_to_iq(const SAMPLE *in, SAMPLE *out, unsigned long framesPerBuffer, int in_ch)
{
    unsigned int i;

    outbuffer = (sp_float *)out;
    if (in == NULL) {
        gNumNoInputs += 1;
        for (i = 0; i < framesPerBuffer; i++) {
            incplx[i].real = 0.0f;
            incplx[i].imag = 0.0f;
            outcplx[i].real = 0.0f;
            outcplx[i].imag = 0.0f;
        }
    } else {
        inbuffer = (sp_float *)in;
        framesToComplex(inbuffer, incplx, outcplx, framesPerBuffer, in_ch);
        if ((mystate.opmode == MODE_AM) || (mystate.opmode == MODE_LSB) || (mystate.opmode == MODE_USB))
            doMicProc(incplx, framesPerBuffer);
    }

    if (G_mode != 'T' && G_null_count++ < MAX_NULL) {
        null_modulate(incplx);
    } else {
        if (mystate.twoToneFlag) {
            ssb_modulate(incplx);
            twoTone(incplx);
        } else {
            if (mystate.opmode == MODE_AM) am_modulate(incplx);
            if (mystate.opmode == MODE_LSB) ssb_modulate(incplx);
            if (mystate.opmode == MODE_USB) ssb_modulate(incplx);
            if (mystate.opmode == MODE_TUNE) tune_modulate(incplx);
            if (mystate.opmode == MODE_CW) tune_modulate(incplx);
        }
    }

    fastconv(incplx, outcplx, (int)framesPerBuffer);

    for (i = 0; i < framesPerBuffer; i++) {
        *outbuffer = outcplx[i].real * iMult;
        outbuffer++;
        *outbuffer = outcplx[i].imag * qMult;
        outbuffer++;
    }
}

/* Full-duplex: mic in + Proficio I/Q out (same host API). */
static int sdrAudioCallback(const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData) {
    SAMPLE *out = (SAMPLE*) outputBuffer;
    static SAMPLE remote_buf[4096 * 2];
    unsigned int i;
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;

    G_DSP_Busy = TRUE;
    /*
     * REMOTE_AUDIO (CMD_SET_AUDIO_DEVICE=2) → MSA1 UDP mic.
     * Digital (0) always uses PortAudio digi capture; Phones (1) local mic.
     */
    if (G_audio_mode == REMOTE_AUDIO && remote_mic_ready() &&
        framesPerBuffer <= 4096u) {
        remote_mic_fill_stereo_96k(remote_buf, (unsigned)framesPerBuffer);
        process_mic_to_iq(remote_buf, out, framesPerBuffer, 2);
        G_DSP_Busy = FALSE;
        return paContinue;
    }
    /*
     * TUNE/CW synthesize a carrier and do not need mic samples. On Linux (ALSA loopback /
     * virtual cable with no writer, or rare PortAudio underflows) inputBuffer can be NULL
     * while the output (I/Q) half of the duplex stream still runs.
     */
    if (inputBuffer == NULL &&
        mystate.opmode != MODE_TUNE &&
        mystate.opmode != MODE_CW) {
        for (i = 0; i < framesPerBuffer; i++) {
            *out++ = 0;
            *out++ = 0;
        }
        gNumNoInputs += 1;
    } else {
        process_mic_to_iq((const SAMPLE *)inputBuffer, out, framesPerBuffer,
            inputBuffer ? inputchannels : 0);
    }
    G_DSP_Busy = FALSE;
    return paContinue;
}

/* Split: digi/operator mic capture only (e.g. Pulse VirtualB.monitor or 48k phones). */
static int sdrMicOnlyCallback(const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData) {
    int ch = inputchannels > 0 ? inputchannels : 2;
    (void)outputBuffer;
    (void)timeInfo;
    (void)statusFlags;
    (void)userData;
    /* Remote: I/Q callback pulls MSA1; do not mix local mic into the ring. */
    if (G_audio_mode == REMOTE_AUDIO)
        return paContinue;
    if (inputBuffer == NULL || framesPerBuffer > 4096u)
        return paContinue;
    if (g_mic_resampler != NULL) {
        mscc_resampler_push_in(g_mic_resampler,
            (const float *)inputBuffer, (int)framesPerBuffer, ch,
            mic_ring_put_frame, NULL);
    } else {
        mic_ring_write((const SAMPLE *)inputBuffer, framesPerBuffer, ch);
    }
    return paContinue;
}

/* Split: Proficio I/Q play — mic from ring (or silence for TUNE/CW). */
static int sdrIqPlayOnlyCallback(const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData) {
    static SAMPLE micbuf[4096 * 2];
    (void)inputBuffer;
    (void)timeInfo;
    (void)statusFlags;
    (void)userData;

    if (outputBuffer == NULL || framesPerBuffer > 4096u)
        return paContinue;

    G_DSP_Busy = TRUE;
    if (mystate.opmode == MODE_TUNE || mystate.opmode == MODE_CW) {
        process_mic_to_iq(NULL, (SAMPLE *)outputBuffer, framesPerBuffer, 0);
    } else if (G_audio_mode == REMOTE_AUDIO && remote_mic_ready()) {
        remote_mic_fill_stereo_96k(micbuf, (unsigned)framesPerBuffer);
        process_mic_to_iq(micbuf, (SAMPLE *)outputBuffer, framesPerBuffer, 2);
    } else {
        mic_ring_read(micbuf, framesPerBuffer);
        process_mic_to_iq(micbuf, (SAMPLE *)outputBuffer, framesPerBuffer, 2);
    }
    G_DSP_Busy = FALSE;
    return paContinue;
}

static void manage_stream_close_all(void)
{
    if (stream != NULL) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        stream = NULL;
    }
    if (stream_mic != NULL) {
        Pa_StopStream(stream_mic);
        Pa_CloseStream(stream_mic);
        stream_mic = NULL;
    }
    g_split_streams = 0;
    mic_ring_reset();
    mic_resampler_destroy();
}

/*
 * Open+start once.
 * device < 0 → output-only (Proficio I/Q). Used for DIGITAL+TUNE/CW: idle
 * digi capture can stall full-duplex so callbacks never run → no RF.
 * Mixed Pulse mic + ALSA Proficio → dual stream.
 * Rate mismatch (48k mic + 96k I/Q) → dual + Oboe upsample into 96k ring.
 */
static PaError open_start_stream_once(int device, int channels) {
    PaError err;
    const PaDeviceInfo *inInfo = NULL;
    const PaDeviceInfo *outInfo;
    const PaHostApiInfo *in_api = NULL;
    const PaHostApiInfo *out_api = NULL;
    int output_only = (device < 0);
    int different_host = 0;
    int need_dual = 0;
    int need_resample = 0;
    double iq_rate = (double)mystate.samplerate;
    double mic_rate = iq_rate;
    unsigned long mic_frames;

    outInfo = Pa_GetDeviceInfo(outputParameters.device);
    if (outInfo == NULL) {
        print_time();
        if (G_fp_logfile)
            fprintf(G_fp_logfile,
                "[%d] manage_stream. invalid Pa output device %d\n",
                line_number++, (int)outputParameters.device);
        return paInvalidDevice;
    }
    out_api = Pa_GetHostApiInfo(outInfo->hostApi);

    if (!output_only) {
        inInfo = Pa_GetDeviceInfo((PaDeviceIndex)device);
        if (inInfo == NULL) {
            print_time();
            if (G_fp_logfile)
                fprintf(G_fp_logfile,
                    "[%d] manage_stream. invalid Pa input device %d\n",
                    line_number++, device);
            return paInvalidDevice;
        }
        in_api = Pa_GetHostApiInfo(inInfo->hostApi);
        if (inInfo->hostApi != outInfo->hostApi)
            different_host = 1;
        inputParameters.device = device;
        inputParameters.channelCount = channels;
        inputParameters.sampleFormat = PA_SAMPLE_TYPE;
        inputParameters.suggestedLatency = inInfo->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = NULL;
        inputchannels = channels;
    } else {
        inputchannels = 0;
    }

    /*
     * Mixed host APIs (Pulse digi mic + ALSA Proficio) cannot share one stream.
     * Rate mismatch cannot full-duplex (one rate per stream).
     * Output-only (TUNE/CW) stays I/Q @ 96k only.
     */
    if (output_only) {
        print_time();
        if (G_fp_logfile)
            fprintf(G_fp_logfile,
                "[%d] manage_stream. OUTPUT-ONLY I/Q out_dev=%d out_api='%s' name='%s' rate=%.0f\n",
                line_number++, (int)outputParameters.device,
                (out_api && out_api->name) ? out_api->name : "?",
                outInfo->name ? outInfo->name : "?",
                iq_rate);
        mic_resampler_destroy();
        err = Pa_OpenStream(&stream, NULL, &outputParameters, iq_rate,
            mystate.frames, 0, sdrAudioCallback, NULL);
        if (err != paNoError) {
            print_time();
            if (G_fp_logfile)
                fprintf(G_fp_logfile,
                    "[%d] manage_stream. output-only OpenStream FAILED PA %d '%s'\n",
                    line_number++, err, Pa_GetErrorText(err));
            return err;
        }
        err = Pa_StartStream(stream);
        if (err != paNoError) {
            manage_stream_close_all();
            return err;
        }
        g_split_streams = 0;
        return paNoError;
    }

    mic_rate = pick_mic_sample_rate(&inputParameters, inInfo, iq_rate);
    need_resample = (mic_rate != iq_rate) ? 1 : 0;
    need_dual = (different_host || need_resample) ? 1 : 0;
    mic_frames = (unsigned long)mystate.frames;
    if (need_resample && mic_rate > 0.0 && iq_rate > 0.0) {
        mic_frames = (unsigned long)(0.5 + (double)mystate.frames * mic_rate / iq_rate);
        if (mic_frames < 64ul)
            mic_frames = 64ul;
        if (mic_frames > 4096ul)
            mic_frames = 4096ul;
    }

    print_time();
    if (G_fp_logfile)
        fprintf(G_fp_logfile,
            "[%d] manage_stream. rate plan: iq=%.0f mic=%.0f dual=%d resample=%d "
            "mic_frames=%lu host_mix=%d in_default=%.0f\n",
            line_number++, iq_rate, mic_rate, need_dual, need_resample,
            (unsigned long)mic_frames, different_host,
            inInfo->defaultSampleRate);

    if (need_dual) {
        PaStreamParameters in_only = inputParameters;
        PaStreamParameters out_only = outputParameters;

        print_time();
        if (G_fp_logfile)
            fprintf(G_fp_logfile,
                "[%d] manage_stream. DUAL STREAM: "
                "in_dev=%d in_api='%s' in_name='%s' @%.0f | "
                "out_dev=%d out_api='%s' out_name='%s' @%.0f ch=%d%s\n",
                line_number++, device,
                (in_api && in_api->name) ? in_api->name : "?",
                (inInfo && inInfo->name) ? inInfo->name : "?",
                mic_rate,
                (int)outputParameters.device,
                (out_api && out_api->name) ? out_api->name : "?",
                outInfo->name ? outInfo->name : "?",
                iq_rate,
                channels,
                need_resample ? " (Oboe upsample)" : "");
        mic_ring_reset();
        mic_resampler_destroy();
        stream = NULL;
        stream_mic = NULL;
        in_only.hostApiSpecificStreamInfo = NULL;
        out_only.hostApiSpecificStreamInfo = NULL;

        if (need_resample) {
            g_mic_resampler = mscc_resampler_create(
                (int)(mic_rate + 0.5), (int)(iq_rate + 0.5));
            if (g_mic_resampler == NULL) {
                print_time();
                if (G_fp_logfile)
                    fprintf(G_fp_logfile,
                        "[%d] manage_stream. dual: resampler create FAILED "
                        "(%.0f -> %.0f)\n",
                        line_number++, mic_rate, iq_rate);
                return paUnanticipatedHostError;
            }
            g_mic_rate = mic_rate;
        } else {
            g_mic_rate = mic_rate;
        }

        err = Pa_OpenStream(&stream_mic, &in_only, NULL, mic_rate, mic_frames, 0,
            sdrMicOnlyCallback, NULL);
        if (err != paNoError) {
            print_time();
            if (G_fp_logfile)
                fprintf(G_fp_logfile,
                    "[%d] manage_stream. dual: mic OpenStream FAILED PA %d '%s' "
                    "rate=%.0f frames=%lu\n",
                    line_number++, err, Pa_GetErrorText(err),
                    mic_rate, (unsigned long)mic_frames);
            manage_stream_close_all();
            return err;
        }
        err = Pa_OpenStream(&stream, NULL, &out_only, iq_rate, mystate.frames, 0,
            sdrIqPlayOnlyCallback, NULL);
        if (err != paNoError) {
            print_time();
            if (G_fp_logfile)
                fprintf(G_fp_logfile,
                    "[%d] manage_stream. dual: I/Q OpenStream FAILED PA %d '%s'\n",
                    line_number++, err, Pa_GetErrorText(err));
            manage_stream_close_all();
            return err;
        }
        err = Pa_StartStream(stream_mic);
        if (err == paNoError)
            err = Pa_StartStream(stream);
        if (err != paNoError) {
            print_time();
            if (G_fp_logfile)
                fprintf(G_fp_logfile,
                    "[%d] manage_stream. dual: StartStream FAILED PA %d '%s'\n",
                    line_number++, err, Pa_GetErrorText(err));
            manage_stream_close_all();
            return err;
        }
        g_split_streams = 1;
        print_time();
        if (G_fp_logfile)
            fprintf(G_fp_logfile,
                "[%d] manage_stream. DUAL STREAM OK "
                "(mic @%.0f + I/Q @%.0f%s)\n",
                line_number++, mic_rate, iq_rate,
                need_resample ? ", upsample" : ", ring bypass");
        return paNoError;
    }

    print_time();
    if (G_fp_logfile)
        fprintf(G_fp_logfile,
            "[%d] manage_stream. FULL-DUPLEX (same API, rate=%.0f): "
            "in_dev=%d in_api='%s' | out_dev=%d out_api='%s' ch=%d\n",
            line_number++, iq_rate, device,
            (in_api && in_api->name) ? in_api->name : "?",
            (int)outputParameters.device,
            (out_api && out_api->name) ? out_api->name : "?",
            channels);
    mic_resampler_destroy();
    err = Pa_OpenStream(&stream, &inputParameters, &outputParameters,
        iq_rate, mystate.frames, 0, sdrAudioCallback, NULL);
    if (err != paNoError) {
        print_time();
        if (G_fp_logfile)
            fprintf(G_fp_logfile,
                "[%d] manage_stream. full-duplex OpenStream FAILED PA %d '%s'\n",
                line_number++, err, Pa_GetErrorText(err));
        stream = NULL;
        return err;
    }
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        manage_stream_close_all();
        return err;
    }
    g_split_streams = 0;
    g_mic_rate = iq_rate;
    print_time();
    if (G_fp_logfile)
        fprintf(G_fp_logfile,
            "[%d] manage_stream. FULL-DUPLEX OK (%.0f Hz, no resample)\n",
            line_number++, iq_rate);
    return paNoError;
}

int manage_stream(int start_stop, int device, int channels) {
    PaError err = 0;
    int status = 0;
    static int stream_running = FALSE;

    if (device >= 0) {
        if (channels < 1)
            channels = 1;
        if (channels > 2) {
            print_time();
            if (G_fp_logfile)
                fprintf(G_fp_logfile,
                    "[%d] Main Thread. manage_stream. Device %d. Channel Number: %d > 2 (ALSA inflate). Using 2 ch.\n",
                    line_number++, device, channels);
            channels = 2;
        }
    }
    if (start_stop == TRUE) {
        if (stream_running == FALSE) {
            print_time();
            if (G_fp_logfile) {
                if (device < 0)
                    fprintf(G_fp_logfile,
                        "[%d] Main Thread. manage_stream. starting OUTPUT-ONLY I/Q rate=96000 (no capture)\n",
                        line_number++);
                else
                    fprintf(G_fp_logfile,
                        "[%d] Main Thread. manage_stream. starting FULL-DUPLEX in_dev=%d ch=%d rate=96000\n",
                        line_number++, device, channels);
            }

            err = open_start_stream_once(device, channels);
            if (err != paNoError && device >= 0 && channels == 1) {
                print_time();
                if (G_fp_logfile)
                    fprintf(G_fp_logfile,
                        "[%d] manage_stream. retry open with 2 channels\n", line_number++);
                err = open_start_stream_once(device, 2);
            }

            if (err == paNoError) {
                stream_running = TRUE;
                if (stream != NULL && !Pa_IsStreamActive(stream)) {
                    print_time();
                    if (G_fp_logfile)
                        fprintf(G_fp_logfile,
                            "[%d] manage_stream. WARNING: stream not active after Start\n",
                            line_number++);
                }
            }
        }
    }
    else {
        if (stream_running == TRUE) {
            print_time();
            if (G_fp_logfile)
                fprintf(G_fp_logfile,
                    "[%d] Main Thread. manage_stream. Stopping Stream. device: %d, channels %d (split=%d)\n",
                    line_number++, device, channels, g_split_streams);
            manage_stream_close_all();
            stream_running = FALSE;
            Sleep(80);
        }
        else {
            print_time();
            if (G_fp_logfile)
                fprintf(G_fp_logfile,
                    "[%d] Main Thread. manage_stream. stop: already stopped\n", line_number++);
            err = 0;
        }
    }
    status = err;
    print_time();
    if (G_fp_logfile) {
        fprintf(G_fp_logfile,
            "[%d] Main Thread. manage_stream.  FINISHED. stream_running: %d, status: %d\n",
            line_number++, stream_running, status);
        fflush(G_fp_logfile);
    }
    return status;
}

char* My_getenv(char* myenv)
{
    memset(G_l_path, 0, sizeof(G_l_path));

#if defined(__linux__) || defined(__APPLE__)
    {
        const char *home = getenv("HOME");
        (void)myenv;

        if (home == NULL || home[0] == '\0') {
            home = "/tmp";
        }

        snprintf(G_l_path, sizeof(G_l_path), "%s/.local/mscc", home);
    }
#else
    // Windows
    {
        WCHAR path[MAX_PATH] = { 0 };
        PWSTR lpPath = path;
        HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &lpPath);
        (void)myenv;

        if (SUCCEEDED(hr)) {
            WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpPath, -1, 
                                G_l_path, sizeof(G_l_path), NULL, NULL);
            strcat(G_l_path, "\\MSCC");
        } else {
            strcpy(G_l_path, "C:\\Temp\\MSCC");   // fallback
        }
    }
#endif

    return G_l_path;
}
/************************************* MAIN *********************************************/
int main(int argc, char **argv) {

    PaError err;
    int key = 0;
    float temp;
    int status;
    long t = 0;
    int log_status;
    int sound_ini_status = 0;
    int power_ini_status = 0;
    int ini_status = 0;
    int j = 0;
    /************ DSP parameters *****************/
    mystate.overdriven = 0; // TX overdriven flag
    mystate.nfft = 2048; // # of FFT bins
    mystate.filtertaps = 1000; // # of taps in IF filterl
    mystate.samplerate = SAMPLERATE;
    mystate.frames = mystate.nfft - mystate.filtertaps; // frames per buffer
    mystate.opmode = MODE_LSB;
    mystate.txPower = 1.0f; // TX power 0-1.0 = 0-100%
    mystate.lo1_freq = 12000.0f; // MUST be set before setFilterOffsets() is called
    setFilterOffsets(75.0f, 2700.0f); // Set default TX filter parms
    mystate.iqReversed = TRUE; // Yes Virginia, we're reversed...
    mystate.amCarrier = 0.25f; // Starting value for AM carrier
    temp = 0.0f;

    //************** SET # OF CHANNELS TO USE ON TX HERE **************//
    inputchannels = 1;

    /******* Init DSP ******/
    initDSP(TRUE); // 1st-time startup

    log_status = Open_log_file();
    if (log_status) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Main Thread. SDRcore Log file opened\n", line_number++);
    } else {
        MessageBoxA(NULL, "Log file open failed. SDRcore-trans is terminating", "SDRcore-trans", MB_OK);
        G_all_threads_run = 0;
        exit(1);
    }
    G_all_threads_run = 1;
    print_time();
    fprintf(G_fp_logfile, "[%d] Main Thread - sdrcore-trans. starting. Compile Date %s, Compile Time %s \n", line_number++,
            COMPILE_DATE, COMPILE_TIME);

    power_ini_status = check_for_power_ini_file();
    if (power_ini_status == 1) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Main Thread. power.ini file exists\n", line_number++);
    } else {
        //Power.ini does not exist. create it.
        print_time();
        fprintf(G_fp_logfile, "[%d] Main Thread. power.ini file does not exist. File will be created\n", line_number++);
        build_power_levels();
        Update_Proficio_User_Power_ini();
    }
       
    print_time();
    fprintf(G_fp_logfile, "[%d] Main Thread.  Starting UDP Thread\n", line_number++);
    UDP_thread_rc = pthread_create(&p_UDP_thread, NULL, UDP_Thread, (void *) t);
    if (UDP_thread_rc) {
        print_time();
        fprintf(G_fp_logfile, "[%d} Main Thread. Start up of UDP thread failed, return code from pthread_create() is %d\n", line_number++, UDP_thread_rc);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Main Thread. SDRcore Starting Flusher Thread\n", line_number++);
    Flusher_thread_rc = pthread_create(&p_Flusher_thread, NULL, Flusher_thread, (void *) t);
    if (Flusher_thread_rc) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Main Thread. Start up of Flusher thread failed, return code from pthread_create() is %d\n", line_number++, Flusher_thread_rc);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Main Thread. SDRcore Starting Overdriven Thread\n", line_number++);
    Overdriven_thread_rc = pthread_create(&p_Overdriven_thread, NULL, Overdriven_thread, (void *) t);
    if (Overdriven_thread_rc) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Main Thread. Start up of Overdriven thread failed, return code from pthread_create() is %d\n", line_number++, Flusher_thread_rc);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Main Thread. SDRcore Starting VU Thread\n", line_number++);
    ALC_Meter_thread_rc = pthread_create(&p_ALC_Meter_thread, NULL, ALC_Meter_thread, (void *) t);
    if (ALC_Meter_thread_rc) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Main Thread. Start up of VU thread failed, return code from pthread_create() is %d\n", line_number++, Flusher_thread_rc);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] Main Thread. SDRcore Starting Drive_Manager Thread\n", line_number++);
    Drive_thread_rc = pthread_create(&p_Drive_thread, NULL, Drive_Manager, (void *) t);
    if (Drive_thread_rc) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Main Thread. Start up of Drive_Manager thread failed, return code from pthread_create() is %d\n", line_number++, Flusher_thread_rc);
    }
    
    Get_Operator_Sound_Device();
    Get_Digital_Sound_Device();

    /******************************************************************/
    err = Pa_Initialize();
    if (err != paNoError) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Pa_Initialize failed: %s\n", line_number++, Pa_GetErrorText(err));
        goto error;
    }

    {
        const PaHostApiInfo *lpApiInfo;
        PaHostApiIndex hostApiCount;
        int apiTypeId = 9999;
        PaDeviceIndex devCount;
        PaDeviceIndex playDevice = paNoDevice;

        print_time();
        fprintf(G_fp_logfile, "[%d] main -> PortAudio %s\n", line_number++,
            Pa_GetVersionInfo() ? Pa_GetVersionInfo()->versionText : "?");

        hostApiCount = Pa_GetHostApiCount();
        for (j = 0; j < hostApiCount; j++) {
            lpApiInfo = Pa_GetHostApiInfo(j);
            if (lpApiInfo == NULL || lpApiInfo->name == NULL)
                continue;
            print_time();
            fprintf(G_fp_logfile, "[%d] main -> Host API %d: %s\n",
                line_number++, j, lpApiInfo->name);
#if defined(__linux__) || defined(__APPLE__)
            if (apiTypeId == 9999 && strstr(lpApiInfo->name, "ALSA"))
                apiTypeId = j;
#else
            if (!strncmp("MME", lpApiInfo->name, 3))
                apiTypeId = j;
#endif
        }
#if defined(__linux__) || defined(__APPLE__)
        if (apiTypeId == 9999) {
            for (j = 0; j < hostApiCount; j++) {
                lpApiInfo = Pa_GetHostApiInfo(j);
                if (lpApiInfo && lpApiInfo->name &&
                    (strstr(lpApiInfo->name, "Pulse") || strstr(lpApiInfo->name, "JACK"))) {
                    apiTypeId = j;
                    break;
                }
            }
        }
        if (apiTypeId == 9999 && hostApiCount > 0)
            apiTypeId = 0;
#endif
        print_time();
        fprintf(G_fp_logfile, "[%d] main -> Selected hostApi index: %d\n", line_number++, apiTypeId);

        /*
         * Same logic as Windows sdrcore-trans.c:
         *   - operator/digital mics from ini via build_*_input_devices on preferred API
         *   - I/Q TX playDevice = first Multus (Linux also Proficio product string)
         *     with out>=2 on preferred host API (MME / ALSA), then break
         * Do NOT match MSCC cable / virtual devices as the radio.
         */
        devCount = Pa_GetDeviceCount();
        print_time();
        fprintf(G_fp_logfile, "[%d] main -> Device count: %d\n", line_number++, (int)devCount);

        for (j = 0; j < (int)devCount; j++) {
            lpInfo = Pa_GetDeviceInfo((PaDeviceIndex)j);
            if (lpInfo == NULL || lpInfo->name == NULL)
                continue;
            print_time();
            fprintf(G_fp_logfile,
                "[%d] main -> dev %d api=%d in=%d out=%d name='%s'\n",
                line_number++, j, lpInfo->hostApi,
                lpInfo->maxInputChannels, lpInfo->maxOutputChannels, lpInfo->name);

            /* Linux: list inputs from all host APIs so Pulse VirtualB.monitor is visible */
#if defined(__linux__) || defined(__APPLE__)
            if (lpInfo->maxInputChannels > 0) {
                build_input_devices((PaDeviceIndex)j);
                build_digital_input_devices((PaDeviceIndex)j);
            }
#else
            if (lpInfo->hostApi == apiTypeId) {
                if (lpInfo->maxInputChannels > 0) {
                    build_input_devices((PaDeviceIndex)j);
                    build_digital_input_devices((PaDeviceIndex)j);
                }
            }
#endif
            /* Windows: strstr(..., "Multus"); Linux radio enumerates as "Proficio" */
            if (strstr(lpInfo->name, "Multus") != NULL ||
                strstr(lpInfo->name, "Proficio") != NULL) {
                /* Prefer ALSA hw Proficio for I/Q TX, not Pulse wrapper names */
                if (lpInfo->maxOutputChannels >= 2 &&
                    lpInfo->hostApi == apiTypeId &&
                    strstr(lpInfo->name, "hw:") != NULL) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] Multus SDR TX SDR device found: %s\n",
                        line_number++, lpInfo->name);
                    playDevice = (PaDeviceIndex)j;
                } else if (playDevice == paNoDevice &&
                           lpInfo->maxOutputChannels >= 2 &&
                           lpInfo->hostApi == apiTypeId) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] Multus SDR TX SDR device found (fallback): %s\n",
                        line_number++, lpInfo->name);
                    playDevice = (PaDeviceIndex)j;
                }
            }
        }

        if (playDevice == paNoDevice) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] No Multus SDR Device found. Please check hardware and try again.\n",
                line_number++);
            MessageBoxA(NULL,
                "THE TRANSCEIVER I/Q AUDIO DEVICE HAS NOT BEEN FOUND.\r\n"
                "Is the Transceiver Powered ON?\r\n"
                "Is the Multus sound device visible to PortAudio/ALSA?\r\n"
                "Then restart MSCC\r\n",
                "SDRcore-trans", MB_OK | MB_ICONEXCLAMATION);
            goto error;
        }

        if (G_input_device_index == NO_INPUT_DEVICE) {
            print_time();
            fprintf(G_fp_logfile, "[%d] main. NO MICROPHONE DEVICE FOUND.\n", line_number++);
            MessageBoxA(NULL, "NO MICROPHONE DEVICE FOUND \r\n",
                "SDRcore-trans", MB_OK | MB_ICONEXCLAMATION);
            goto error;
        }

        /*
         * Digital mic is optional at startup (P works with operator mic only). If
         * digital-microphone.ini is missing or the name does not match any PortAudio
         * device, keep D mode usable: fall back to the operator mic record so
         * CMD_SET_AUDIO_DEVICE does not index past G_digital_input_devices[] and
         * leave the I/Q stream stopped (no TUNE power in D).
         */
        if (G_digital_input_device_index == NO_INPUT_DEVICE ||
            G_digital_input_device_index < 0 ||
            G_digital_input_device_index >= MAX_INPUT_DEVICES) {
            G_digital_input_device_index = G_input_device_index;
            G_digital_input_devices[G_digital_input_device_index] =
                G_input_devices[G_input_device_index];
            print_time();
            fprintf(G_fp_logfile,
                "[%d] main. Digital mic not found — fallback to operator mic index %d ('%s')\n",
                line_number++, G_digital_input_device_index,
                G_input_devices[G_input_device_index].name);
        } else {
            print_time();
            {
                const PaDeviceInfo *ddi = Pa_GetDeviceInfo(
                    (PaDeviceIndex)G_digital_input_devices[G_digital_input_device_index].device_index);
                const PaHostApiInfo *dhai = ddi ? Pa_GetHostApiInfo(ddi->hostApi) : NULL;
                fprintf(G_fp_logfile,
                    "[%d] main. Digital mic index %d device %d api='%s' '%s'\n",
                    line_number++, G_digital_input_device_index,
                    G_digital_input_devices[G_digital_input_device_index].device_index,
                    (dhai && dhai->name) ? dhai->name : "?",
                    G_digital_input_devices[G_digital_input_device_index].name);
            }
        }

        inputParameters.device = G_input_devices[G_input_device_index].device_index;
        if (inputParameters.device == paNoDevice ||
            Pa_GetDeviceInfo(inputParameters.device) == NULL) {
            print_time();
            fprintf(G_fp_logfile, "[%d] main. Microphone device invalid.\n", line_number++);
            goto error;
        }
        if (inputParameters.device == playDevice) {
            print_time();
            fprintf(G_fp_logfile,
                "[%d] main. Operator mic must not be the Multus/Proficio I/Q device.\n",
                line_number++);
            goto error;
        }
        inputParameters.channelCount = inputchannels;
        inputParameters.sampleFormat = PA_SAMPLE_TYPE;
        inputParameters.suggestedLatency = 0.025f;
        inputParameters.hostApiSpecificStreamInfo = NULL;

        outputParameters.device = playDevice;
        if (outputParameters.device == paNoDevice ||
            Pa_GetDeviceInfo(outputParameters.device) == NULL) {
            print_time();
            fprintf(G_fp_logfile, "[%d] main. I/Q TX device invalid.\n", line_number++);
            goto error;
        }
        outputParameters.channelCount = 2;
        outputParameters.sampleFormat = PA_SAMPLE_TYPE;
        outputParameters.suggestedLatency = 0.025f;
        outputParameters.hostApiSpecificStreamInfo = NULL;

        print_time();
        fprintf(G_fp_logfile,
            "[%d] G_input_device_index: %d, device_index: %d, Multus TX: %d '%s'\n",
            line_number++, G_input_device_index,
            G_input_devices[G_input_device_index].device_index,
            (int)playDevice, Pa_GetDeviceInfo(playDevice)->name);

        status = manage_stream(0, G_input_devices[G_input_device_index].device_index,
                G_input_devices[G_input_device_index].num_channels);
        status = manage_stream(1, G_input_devices[G_input_device_index].device_index,
                G_input_devices[G_input_device_index].num_channels);
        if (status) {
            err = status;
            goto error;
        }
    }

    Init_Power_All();
    remote_mic_init();
    while (G_all_threads_run) {
        Sleep(100);
    }
    remote_mic_shutdown();
    if (stream != NULL) {
        err = Pa_CloseStream(stream);
        if (err != paNoError)
            goto error;
    }
    Sleep(100);
    Pa_Terminate();
    exit(0);

error:
    /*
     * Fatal audio init — always exit here (do not fall through into
     * Init_Power_All / stream loop with invalid devices).
     */
    G_all_threads_run = 0;
    if (G_fp_logfile != NULL) {
        print_time();
        fprintf(G_fp_logfile, "[%d] An error occured while using the portaudio stream\n", line_number++);
        print_time();
        fprintf(G_fp_logfile, "[%d] Error number: %d\n", line_number++, err);
        print_time();
        fprintf(G_fp_logfile, "[%d] Error message: %s\n", line_number++,
            err != paNoError ? Pa_GetErrorText(err) : "(no PortAudio error — missing device)");
        print_time();
        fprintf(G_fp_logfile, "[%d] sdrcore-trans terminating (audio init failed)\n", line_number++);
        fflush(G_fp_logfile);
    }
    MessageBoxA(NULL, "SDRcore-trans initialization FAILED.  Send logs to Multus SDR, LLC", "SDRcore-trans",
        MB_OK | MB_ICONEXCLAMATION);
    remote_mic_shutdown();
    Pa_Terminate();
    exit(1);
}

