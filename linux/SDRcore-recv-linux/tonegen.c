#include "extern.h"

#define PA_SAMPLE_TYPE      paFloat32
typedef float SAMPLE;

extern state mystate;
extern calstate mycalstate;
extern panadapter_buffer panbuffer;
extern cs_state csstate;
extern blanker_state nbstate;
extern int line_number;
extern FILE* G_fp_logfile;

float G_side_tone = 600.0f;
uint8_t G_key_down = 0;
static sp_cplx t_samp[32768];
static int t_ifShiftSamps = 0.0f;
static float t_tempflt;
static sp_cplx t_filt[32768];
static sp_cplx t_input_save[2048]; //allocate for max # of filter taps
static float t_gainmult = 1.0f;
static int G_NumNoInputs = 0;
sp_float *t_inbuffer, *t_outbuffer;
sp_float t_wold = 0;
sp_cplx t_incplx[4097];
sp_cplx t_outcplx[4096];
anotch_state t_anstate;

void t_fastconv(sp_cplx *in, sp_cplx *out, int frames)
{
        float tmp;
        int isamps = 0;
        int osamps = 0;
        int sbcut = 0;
        static int meterblocks = 0;
        static int i, j;
        sp_float magaccum = 0.0f;
        sp_float mag;
        sp_float fsbcut = 0.0f;
        sp_float peakmag = 0.0f;

        // re-init DSP if requested
        if (mystate.initDSPflag) initDSP();

        /* do FFT of samples */
        jimfft(t_samp, mystate.nfft); // No dynamic allocation in this version of the FFT

        /**************** IF SHIFTER **************************************/
        t_ifShiftSamps = mystate.nfft / 4; // 1/4 of FFT size @ 4096, shift 1/4 or 12 kHz @ Fs = 48K

        /********** FFT masks for unwanted sideband removal *********/
        fsbcut = (mystate.filtHighHz / mystate.samplerate) * (sp_float) mystate.nfft;
        sbcut = (int) fsbcut + 50;

        // remove the opposite sideband
        if (mystate.opmode == MODE_USB) {
                for (i = sbcut; i < mystate.nfft; i++) {
                        t_samp[i].real = 0.0f;
                        t_samp[i].imag = 0.0f;
                }
        }

        // remove the opposite sideband
        if ((mystate.opmode == MODE_LSB)) {
                for (i = 0; i < sbcut; i++) {
                        t_samp[i].real = 0.0f;
                        t_samp[i].imag = 0.0f;
                }

                for (i = sbcut * 2; i < mystate.nfft / 2; i++) {
                        t_samp[i].real = 0.0f;
                        t_samp[i].imag = 0.0f;
                }
        }
        /* Multiply the two transformed sequences */
        /* swap the real and imag outputs to allow a forward FFT instead of inverse FFT */

        for (i = 0; i < mystate.nfft; i++) {
                t_tempflt = t_samp[i].real * t_filt[i].real
                        - t_samp[i].imag * t_filt[i].imag;
                t_samp[i].real = t_samp[i].real * t_filt[i].imag
                        + t_samp[i].imag * t_filt[i].real;
                t_samp[i].imag = t_tempflt;
        }

        /* Inverse fft the multiplied sequences */
        jimfft(t_samp, mystate.nfft);


        /* Write the result out. because a forward FFT was used for the inverse FFT, the output is in the imag part */

        osamps = 0;
        for (i = mystate.filtertaps; i < mystate.nfft; i++) {
                mag = sqrt((t_samp[i].real * t_samp[i].real) + (t_samp[i].imag * t_samp[i].imag));
                magaccum += (mag * 100000.0f);

                // capture peak magnitude for s-meter
                if (mag > peakmag) peakmag = mag;

                // Simple AM demod - more work required here
                if (mystate.opmode == MODE_AM) {
                        t_samp[i].real = mag;
                        t_samp[i].imag = mag;
                }

                out[osamps].real = t_samp[i].real;
                out[osamps].imag = t_samp[i].real;
                osamps++;
        }
        // Insert a Side Tone if in CW mode.  side_tone will check for KEY DOWN (G_key_down)
        if (G_mode == 'C') {
                side_tone(t_samp);
        }
        
        /*meterblocks++;
        if (meterblocks >= 2) {
                magaccum /= ((sp_float) mystate.nfft - mystate.filtertaps);
                mystate.avgRxSignalMag = magaccum;
                magaccum = 0.0f;
                meterblocks = 0;
        }*/

        /* overlap the last FILTER_LENGTH-1 input data points in the next FFT */
        for (i = 0; i < mystate.filtertaps; i++) {
                t_samp[i].real = t_input_save[i].real;
                t_samp[i].imag = t_input_save[i].imag;
        }

        isamps = 0;
        for (; i < mystate.nfft - mystate.filtertaps; i++) {
                t_samp[i].real = in[isamps].real * t_gainmult;
                t_samp[i].imag = in[isamps].imag * t_gainmult;
                isamps++;

                if (mystate.iqReversed) // <----- reverse I/Q input channels if reverse flag set.
                {
                        tmp = t_samp[i].real;
                        t_samp[i].real = t_samp[i].imag;
                        t_samp[i].imag = tmp;
                }

        }

        /* save the last FILTER_LENGTH points for next time */
        for (j = 0; j < mystate.filtertaps; j++, i++) {
                t_samp[i].real = in[isamps].real * t_gainmult;
                t_samp[i].imag = in[isamps].imag * t_gainmult;
                isamps++;

                if (mystate.iqReversed) // <----- reverse I/Q input channels if reverse flag set.
                {
                        tmp = t_samp[i].real;
                        t_samp[i].real = t_samp[i].imag;
                        t_samp[i].imag = tmp;
                }

                t_input_save[j].real = t_samp[i].real;
                t_input_save[j].imag = t_samp[i].imag;
        }

        mystate.peakRxSignalDbm = 20.0f * log10(peakmag) - 20.0f;

}

static int t_sdrAudioCallback(const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData);

//static int gNumNoInputs = 0;

/* This routine will be called by the PortAudio engine when audio is needed.
 ** It may be called at interrupt level on some machines so don't do anything
 ** that could mess up the system like calling malloc() or free().
 */
static int t_sdrAudioCallback(const void *inputBuffer, void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData)
{
        SAMPLE *out = (SAMPLE*) outputBuffer;
        const SAMPLE *in = (const SAMPLE*) inputBuffer;
        unsigned int i;
        (void) timeInfo; /* Prevent unused variable warnings. */
        (void) statusFlags;
        (void) userData;

        if (inputBuffer == NULL) {
                for (i = 0; i < framesPerBuffer; i++) {
                        *out++ = 0; /* left - silent */
                        *out++ = 0; /* right - silent */
                }
                G_NumNoInputs += 1;
        } else {
                t_inbuffer = (sp_float*) inputBuffer;
                t_outbuffer = (sp_float*) outputBuffer;

                /*
                        Mux samples into complex struct array. The "custom" typedef is to avoid consistency issues
                        across compilers with COMPLEX. 
                 */
                //framesToComplex(t_inbuffer, t_incplx, t_outcplx, framesPerBuffer);

                /**************************** IF SHIFT -12000 Hz *********************************/
                //ifShiftDown(incplx, framesPerBuffer);
                //complex_shift(t_incplx, framesPerBuffer);

                /********************** Run calibration DSP, if engaged *************************/
                //if (mycalstate.calStart == TRUE) doRxCalibrate(t_incplx, framesPerBuffer);

                /**************************** DO THE RADIO THING *********************************/
                t_fastconv(t_incplx, t_outcplx, (int) framesPerBuffer);

                /********************** Run AGC *************************************************/
                //if (!AGC_Initializing) doAGC(t_outcplx, (int) framesPerBuffer);

                /********************** Run auto-notch *****************************************/
                //if (t_anstate.enabled) anotch(t_outcplx, (int) framesPerBuffer);

                /*
                        De-mux samples back into packed I-Q-I-Q-I-Q format. In case there's any question,
                        Q should lead I by 90 degrees.
                 */
                for (i = 0; i < framesPerBuffer; i++) {
                        t_outcplx[i].real *= volumeLevel;
                        *t_outbuffer = t_outcplx[i].real;
                        t_outbuffer++;
                        t_outcplx[i].imag *= volumeLevel;
                        *t_outbuffer = t_outcplx[i].imag;
                        t_outbuffer++;
                }
        }
        return paContinue;
}

void side_tone(sp_cplx* samps)
{
        static int beenhere = 0, active = 0, rampdir = 0;
        static float ramp, level, myfreq, oldfreq, phaseinc, phaseaccum;
        static float myramp;

        //float txsamplerate = 48000.0f;		// normally gotten from master struct
        float ramptimems = 10.0f; // keying up/down ramp time in ms
        float txsamplerate = mystate.samplerate;
        int j;

        if (!beenhere) {
                beenhere = 1;
                float ramp = 1.0f / (txsamplerate * ramptimems);
                level = 0.0f;
                rampdir = 0;
                active = 0;
                ramp = 0.0f;
                oldfreq = 9999.99f;
        }

        // if key isn't down and shaping isn't previously active, return
        if ((G_key_down == 0) && (active == 0)) return;

        // new key down
        if ((G_key_down == 1) && (active == 0)) {
                active = 1;
                rampdir = 1;
                myramp = 1.0f + ramp;

                if (oldfreq != G_side_tone) {
                        phaseinc = (float) TPI / (txsamplerate / G_side_tone);
                        phaseaccum = 0.0f;
                        oldfreq = G_side_tone;
                        rampdir = 1; // ramp UP, because this is a new keydown command
                }
        }
        // still active from previous operation, ramping DOWN
        if ((G_key_down == 0) && (active == 1)) {
                rampdir = 0; // make sure we're in ramp DOWN mode
                myramp = ramp;
        }
        //print_time();
        //fprintf(G_fp_logfile, "[%d] side_tone -> G_key_down: %d G_mode: %c, G_side_tone: %f,samples: %d\n",
        //        line_number++, G_key_down, G_mode, G_side_tone, mystate.nfft);
        // finally, build tone samples
        for (j = 0; j < mystate.nfft; j++) {
                samps[j].real = (sp_float) sin(phaseaccum) * level;
                samps[j].imag = (sp_float) cos(phaseaccum) * level;

                phaseaccum += phaseinc;
                phaseaccum = (float) atan2(sin(phaseaccum), cos(phaseaccum));
                if (rampdir == 0) {
                        myramp *= ramp;
                        if (myramp <= 0.000001) {
                                active = 0;
                                break; // break out if ramping down and level reaches "close enough" to zero
                        }
                }
                // ramping UP, all samples get tone.
                if (rampdir == 1) myramp *= (1.0f + ramp);

        }
}

int manage_sidetone_stream(int start_stop, int device, int channels)
{
        PaError err;
        int status = 0;
        static int current_device = 0;
        static int current_channels = 0;

        if (start_stop == 0) {
                current_device = device;
                current_channels = channels;
        }

        switch (start_stop) {
        case 1:
                print_time();
                fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream -> Starting Stream.  Device: %d, Channels %d\n", line_number++, current_device, current_channels);
                err = Pa_IsFormatSupported(NULL, &outputParameters, 96000);
                if (err != paNoError) {
                        print_time();
                        fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream ->   Pa_IsFormatSupported FAILED. Device: %d, Channels %d\n",
                                line_number++, current_device, current_channels);
                }
                err = Pa_OpenStream(
                        &stream,
                        NULL,
                        &outputParameters,
                        (int) mystate.samplerate,
                        mystate.frames,
                        0, /* paClipOff, */ /* we won't output out of range samples so don't bother clipping them */
                        t_sdrAudioCallback,
                        NULL);
                if (err != paNoError) {
                        const PaHostErrorInfo* lpError = Pa_GetLastHostErrorInfo();
                        print_time();
                        fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream -> Open Stream Failed: PA ERROR: %s\n",
                                line_number++, lpError->errorText);
                        MessageBoxA(NULL, "Pa Open Stream FAILED. Send logs to Multus SDR, LLC", "SDRcore-recv", MB_ICONASTERISK | MB_OK);
                        status = err;
                } else {
                        err = Pa_StartStream(stream);
                        if (err != paNoError) {
                                status = err;
                                const PaHostErrorInfo* lpError = Pa_GetLastHostErrorInfo();
                                print_time();
                                fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream -> Start Stream Failed: PA ERROR: %s\n",
                                        line_number++, lpError->errorText);
                                MessageBoxA(NULL, "Pa Start Stream FAILED. Send logs to Multus SDR, LLC", "SDRcore-trans", MB_ICONASTERISK | MB_OK);
                        }
                }
                break;
        case 0:
                print_time();
                fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream -> Stoping Stream.  New device: %d, New Channels %d\n", line_number++, device, channels);
                Pa_StopStream(stream);
                Pa_CloseStream(stream);
                if (channels > 2) {
                        print_time();
                        fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream -> Device %d -> Channel Number: %d greater than 2. Setting channels to 2.\n",
                                line_number++, device, channels);
                        channels = 2;
                }
                outputParameters.device = device;
                outputParameters.channelCount = channels;
                outputParameters.sampleFormat = PA_SAMPLE_TYPE;
                outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
                outputParameters.hostApiSpecificStreamInfo = NULL;
                break;
        }
        return status;
}

