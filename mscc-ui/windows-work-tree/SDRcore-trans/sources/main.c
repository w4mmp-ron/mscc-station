/********************** SDRdemo *************************
 *	SDR Demo using Multus SDR LLC Hardware				*
 *	SDRcore engine inclusions provided under license	*
 *	from James L Barber, AKA Silicon Pixels, Spokane	*
 *	Radio et al											*
 *********************************************************/
#define RON
#include "extern.h"
//#include "sdrcoretx.h"
//#include "dsputils.h"
//#include "commands.h"


#define PA_SAMPLE_TYPE      paFloat32
#define NO_INPUT_DEVICE 100
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

static int gNumNoInputs = 0;

/* This routine will be called by the PortAudio engine when audio is needed.
 ** It may be called at interrupt level on some machines so don't do anything
 ** that could mess up the system like calling malloc() or free().
 */
static int sdrAudioCallback(const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData) {
    SAMPLE *out = (SAMPLE*) outputBuffer;
    const SAMPLE *in = (const SAMPLE*) inputBuffer;
    unsigned int i;
    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    (void) userData;

    G_DSP_Busy = TRUE;
    if (inputBuffer == NULL) {
        for (i = 0; i < framesPerBuffer; i++) {
            *out++ = 0; /* left - silent */
            *out++ = 0; /* right - silent */
        }
        gNumNoInputs += 1;
    } else {
        inbuffer = (sp_float*) inputBuffer;
        outbuffer = (sp_float*) outputBuffer;
        /*
                Mux samples into complex struct array. The "custom" typedef is to avoid consistency issues
                across compilers with COMPLEX. 
         */
        framesToComplex(inbuffer, incplx, outcplx, framesPerBuffer, inputchannels);
        if ((mystate.opmode == MODE_AM) || (mystate.opmode == MODE_LSB) || (mystate.opmode == MODE_USB))
            doMicProc(incplx, framesPerBuffer);
        //if ((G_tx_mode == 1) || G_QSK) {
        if (G_mode != 'T' && G_null_count++ < MAX_NULL) {
            null_modulate(incplx); // no mode, zero out IQ stream for no output
            //print_time();
            //fprintf(G_fp_logfile, "[%d] Main Thread. sdrAudioCallback. G_null_count %d\n",
            //        line_number++, G_null_count);
        } else { /**************************** Two-tone test *****************************************/
            if (mystate.twoToneFlag) {
                ssb_modulate(incplx);
                twoTone(incplx); // overwrites any mic audio data in buffer
            } else {
                /**************************** AM and SSB modulators *********************************/
                if (mystate.opmode == MODE_AM) am_modulate(incplx);
                if (mystate.opmode == MODE_LSB) ssb_modulate(incplx);
                if (mystate.opmode == MODE_USB) ssb_modulate(incplx);
                if (mystate.opmode == MODE_TUNE) tune_modulate(incplx);
                if (mystate.opmode == MODE_CW) tune_modulate(incplx);
                if (mystate.opmode == MODE_TUNE)tune_modulate(incplx);
            }
        }

        /**************************** DO THE RADIO THING *********************************/
        fastconv(incplx, outcplx, (int) framesPerBuffer);

        /*
                De-mux samples back into packed I-Q-I-Q-I-Q format. In case there's any question,
                Q should lead I by 90 degrees.
         */
        for (i = 0; i < framesPerBuffer; i++) {
            *outbuffer = outcplx[i].real * iMult;
            outbuffer++;
            *outbuffer = outcplx[i].imag * qMult;
            outbuffer++;
        }
    }
    G_DSP_Busy = FALSE;
    return paContinue;
}

int manage_stream(int start_stop, int device, int channels) {
    PaError err = 0;
    int status = 0;
    static int stream_running = FALSE;

    if (channels > 2) {
        print_time();
        fprintf(G_fp_logfile, "[%d] Main Thread. manage_stream. Device %d. Channel Number: %d greater than 2. Setting channels to 2.\n",
            line_number++, device, channels);
        channels = 2;
    }
    if (start_stop == TRUE) {
        if (stream_running == FALSE) {
            print_time();
            fprintf(G_fp_logfile, "[%d] Main Thread. manage_stream. STARTED. stream_running: %d Device: %d, Channels %d\n",
                line_number++, stream_running,device, channels);
            print_time();
            fprintf(G_fp_logfile, "[%d] Main Thread. manage_stream. starting stream.  Device: %d, Channels %d\n",
                line_number++, device, channels);
            inputParameters.device = device;
            inputParameters.channelCount = channels;
            inputParameters.sampleFormat = PA_SAMPLE_TYPE;
            inputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
            inputParameters.hostApiSpecificStreamInfo = NULL;
            inputchannels = channels;
            err = Pa_IsFormatSupported(&inputParameters, &outputParameters, 96000);
            if (err != paNoError) {
                const PaHostErrorInfo* format_error = Pa_GetLastHostErrorInfo();
                print_time();
                fprintf(G_fp_logfile, "[%d] Main Thread.  manage_stream.  Pa_IsFormatSupported FAILED. Device: %d, Channels %d, error: %s\n",
                    line_number++, device, channels, format_error->errorText);
            }
            else {
                err = Pa_OpenStream(
                    &stream,
                    &inputParameters,
                    &outputParameters,
                    (int)mystate.samplerate,
                    mystate.frames,
                    0, /* paClipOff, */ /* we won't output out of range samples so don't bother clipping them */
                    sdrAudioCallback,
                    NULL);
                if (err != paNoError) {
                    const PaHostErrorInfo* lpError = Pa_GetLastHostErrorInfo();
                    print_time();
                    fprintf(G_fp_logfile, "[%d] Main Thread. manage_stream. Open Stream Failed: PA ERROR: %s\n",
                        line_number++, lpError->errorText);
                }
                else {
                    err = Pa_StartStream(stream);
                    if (err != paNoError) {
                        const PaHostErrorInfo* lpError = Pa_GetLastHostErrorInfo();
                        print_time();
                        fprintf(G_fp_logfile, "[%d] Main Thread. manage_stream. Start Stream Failed: PA ERROR: %s\n",
                            line_number++, lpError->errorText);
                    }
                }
            }
        }
        if (err == paNoError) {
            stream_running = TRUE;
        }
    }
    else {
        if (stream_running == TRUE) {
            print_time();
            fprintf(G_fp_logfile, "[%d] Main Thread. manage_stream. STARTED.  stream_running: %d, Device: %d, Channels %d\n",
                line_number++, stream_running,device, channels);
            print_time();
            fprintf(G_fp_logfile, "[%d] Main Thread. manage_stream. Stoping Stream.  New device: %d, New Channels %d\n",
                line_number++, device, channels);
            Pa_StopStream(stream);
            Pa_CloseStream(stream);
            stream_running = FALSE;
        }
        else {
            print_time();
            fprintf(G_fp_logfile, "[%d] Main Thread. manage_stream. STARTED. start_stop: %d, stream_running: %d\n",
                line_number++, start_stop, stream_running);
            err = 0;
        }
    }
    status = err;
    print_time();
    fprintf(G_fp_logfile, "[%d] Main Thread. manage_stream.  FINISHED. stream_running: %d, status: %d\n", 
                        line_number++, stream_running,status);
    return status;
}

char* My_getenv(char* myenv) {

    //FILE* Multus_ini;
    WCHAR path[MAX_PATH] = { 0 };
    PWSTR lpPath = path;
    int lenght = 0;

    memset(G_l_path, 0, sizeof(G_l_path));
    //printf("My_getenv \n");
    HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &lpPath);
    //fprintf("get_serial_number -> Get folder path \n");
    if (SUCCEEDED(hr)) {
        WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpPath, -1, G_l_path, sizeof(G_l_path), NULL, NULL);
        strcat(G_l_path, "/MSCC-NET9");
        //printf("getenv: %s\n", G_l_path);
    }
    //$HOME = G_l_path;
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
        MessageBox(NULL, L"Log file open failed. SDRcore-trans is terminating", L"SDRcore-recv", MB_OK);
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
    if (err != paNoError) return -1; //goto error;

    // Get # of host API's supported on THIS machine at THIS time.
    const PaHostApiInfo* lpApiInfo;
    PaHostApiIndex hostApiCount;
    int apiTypeMME = 9999;
    int apiTypeDSOUND = 9999;
    int apiTypeWDMKS = 9999;
    int apiTypeWASAPI = 9999;
    int apiTypeASIO = 9999;
    int apiTypeALSA = 9999;

    hostApiCount = Pa_GetHostApiCount();

    for (j = 0; j < hostApiCount; j++) {
        lpApiInfo = Pa_GetHostApiInfo(j);
        if (!strncmp("MME", lpApiInfo->name, 3)) {
            apiTypeMME = j;
        }
    }

    /***** Set audio API type here. Eventually this should be abstracted to a choice on the GUI *****/
    //myAudioAPItype = apiTypeALSA;

    PaDeviceIndex devCount = Pa_GetDeviceCount();

    PaDeviceIndex recordDevice = 0;
    PaDeviceIndex playDevice = 0;

    // Get device count, go through devices looking for desired output device
    for (int j = 0; j < devCount; j++) {
        lpInfo = Pa_GetDeviceInfo((PaDeviceIndex) j);
        if (lpInfo->hostApi == apiTypeMME) {
            //print_time();
            //fprintf(G_fp_logfile, "[%d] main. Pa_GetDeviceInfo. j: %d, name: %s, channels: %d\n",
            //    line_number++, j, lpInfo->name, lpInfo->maxInputChannels);
            build_input_devices((PaDeviceIndex)j);
            build_digital_input_devices((PaDeviceIndex)j);
        }
        if (strstr(lpInfo->name, "Multus")) {
            if (lpInfo->maxOutputChannels >= 2) {
                if (lpInfo->hostApi == apiTypeMME) {
                    print_time();
                    fprintf(G_fp_logfile, "[%d] main. Proficio found: %s\n", line_number++, lpInfo->name);
                    playDevice = (PaDeviceIndex) j;
#ifndef RON

                    break;
#endif
                }
            }
        }
    }

    if (playDevice == 0) {
        print_time();
        fprintf(G_fp_logfile, "[%d] main. Mark II NOT FOUND (0).\n", line_number++);
        MessageBoxA(NULL, "THE TRANSCEIVER I/Q AUDIO DEVICE HAS NOT BEEN FOUND \
							\r\nIs the Transceiver Powered ON? \
							\r\nHas the Transceiver been power cycled? \
							\r\nCheck the USB Connection \
							\r\nThen restart MSCC \r\n",
        					"SDRcore-trans", MB_OK | MB_ICONEXCLAMATION);
        goto error;
    }

    inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
    if (inputParameters.device == paNoDevice) {
        print_time();
        fprintf(G_fp_logfile, "[%d] main. NO DEFAULT MICROPHONE DEVICE FOUND.\n", line_number++);
        //MessageBoxA(NULL, "NO DEFAULT MICROPHONE DEVICE FOUND \
		//					\r\nUNCHECK THE Microphone ON Check Box on the Audio Tab\
		//					\r\nThen restart MSCC \r\n",
        //					"SDRcore-trans", MB_OK | MB_ICONEXCLAMATION);
        goto error;
    }

    inputParameters.channelCount = inputchannels;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = 0.025f; // Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = playDevice;
    if (outputParameters.device == paNoDevice) {
        print_time();
        fprintf(G_fp_logfile, "[%d] main. Proficio Not found (2).\n", line_number++);
        MessageBoxA(NULL, "NO DEFAULT MICROPHONE DEVICE FOUND \r\n",
        					"SDRcore-trans", MB_OK | MB_ICONEXCLAMATION);
        goto error;
    }
    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = 0.025f; // Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    //Initialize the output devices structure
    //Now Start the audio stream
    if (G_input_device_index == NO_INPUT_DEVICE) {
        print_time();
        fprintf(G_fp_logfile, "[%d] main. No input device found\n", line_number++);
        MessageBoxA(NULL, "NO MICROPHONE DEVICE FOUND \r\n",
            "SDRcore-trans", MB_OK | MB_ICONEXCLAMATION);
        goto error;
    } else {
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
    while (G_all_threads_run) {
        Sleep(100);
    }
    err = Pa_CloseStream(stream);
    if (err != paNoError) goto error;
    Sleep(100);
    Pa_Terminate();
    exit(0);

error:
    Pa_Terminate();
    print_time();
    fprintf(G_fp_logfile, "[%d] An error occured while using the portaudio stream\n", line_number++);
    print_time();
    fprintf(G_fp_logfile, "[%d] Error number: %d\n", line_number++, err);
    print_time();
    fprintf(G_fp_logfile, "[%d] Error message: %s\n", line_number++, Pa_GetErrorText(err));
    MessageBoxA(NULL, "SDRcore-trans initialization FAILED.  Send logs to Multus SDR, LLC", "SDRcore-trans", 
    	MB_OK | MB_ICONEXCLAMATION);
    exit(0);
}

