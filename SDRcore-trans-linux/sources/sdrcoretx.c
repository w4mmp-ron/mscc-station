/**************** CORE DSP FOR HF SDR RECEIVER **************
 *               (1992 - 2018)  James L Barber				*
 *************************************************************/
#include "extern.h"

static int i, j;
static float tempflt;
static float agcE, agcR, agcA, agcV, agcT;
static float windowtaps[8192];
static sp_cplx cptmp;
static sp_cplx input_save[2048]; //allocate for max # of filter taps
static sp_float hfilt[32768];
static sp_cplx filt[32768];
static sp_cplx samp[32768];

/******* External C&C struct, defined in sdrcore.c *******/
extern state mystate;
extern uint8_t G_DSP_Busy;

void initDSP(int firstrunflag) {
    if (firstrunflag) {
        if (G_network_initialized) {
            print_time();
            fprintf(G_fp_logfile, "[%d] initDSP - Called \n", line_number++);
        }
        // Get simple AGC limiter set up. (more gooder things later)
        agcA = pow(0.01f, 1.0f / (10.0f * mystate.samplerate * 0.001f));
        agcR = pow(0.01f, 1.0f / (3000.0f * mystate.samplerate * 0.001f));
        agcE = 0.0f;
        agcT = 0.001f;

        // Calculate FFT length order (what power of 2)
        mystate.fftOrder = log((sp_float) mystate.nfft) / log(2.0f);

        // Calc TX internal LO phase increment 
        mystate.lo1_phaseinc = TPI / (mystate.samplerate / mystate.lo1_freq);
        mystate.lo1_phaseacc = 0.0f;
        mystate.initDSPflag = FALSE;

        // Initialize ALC
        initALC();

        // Initialize speech processor
        initMicProc(); //init mic proc to 0.0dB (no action)
    }

    /*  Generate "IF" filter using limits set in state struct
            Note this will run at startup, and any time initDSP is called therafter. 
     */
    memset(hfilt, 0, sizeof (sp_cplx) * mystate.nfft);
    memset(filt, 0, sizeof (sp_cplx) * mystate.nfft);

    wsfirBP(hfilt, mystate.filtertaps, W_HANNING, mystate.filtLowHz / mystate.samplerate,
            mystate.filtHighHz / mystate.samplerate);

    tempflt = 1.0f / mystate.nfft;

    for (i = 0; i < mystate.filtertaps; i++) {
        filt[i].real = tempflt * hfilt[i];
        filt[i].imag = tempflt * hfilt[i];
    }

    // This FFT uses only memory allocated off the stack
    jimfft(filt, mystate.nfft);
}

/******************* IF shift, opposite sideband suppression, demo etc ****************/
void fastconv(sp_cplx *in, sp_cplx *out, int frames) {
    float tmp;
    int isamps = 0;
    int osamps = 0;
    int sbcut = 0;
    sp_float fsbcut = 0.0f;

    // re-init DSP if requested
    //The DSP is Busy
    //G_DSP_Busy = TRUE;
    if (mystate.initDSPflag) initDSP(FALSE);
    //if (mystate.initDSPflag) initDSP(TRUE);

    /* do FFT of samples */
    jimfft(samp, mystate.nfft);

    /* Multiply the two transformed sequences */
    /* swap the real and imag outputs to allow a forward FFT instead of inverse FFT */

    for (i = 0; i < mystate.nfft; i++) {
        tempflt = samp[i].real * filt[i].real
                - samp[i].imag * filt[i].imag;
        samp[i].real = samp[i].real * filt[i].imag
                + samp[i].imag * filt[i].real;
        samp[i].imag = tempflt;
    }

    /* Inverse fft the multiplied sequences */
    jimfft(samp, mystate.nfft);

    // Write the result out. Because a forward FFT was used for the inverse FFT, the output is in the imag part
    osamps = 0;
    for (i = mystate.filtertaps; i < mystate.nfft; i++) {
        out[osamps].real = samp[i].real * mystate.txPower;
        out[osamps].imag = samp[i].imag * mystate.txPower;
        //out[osamps].real = samp[i].real * 0.0f;
        //out[osamps].imag = samp[i].imag * 0.0f;

        osamps++;
    }

    if (mystate.opmode == MODE_AM || mystate.opmode == MODE_LSB || mystate.opmode == MODE_USB) {
        if (G_Do_ALC == TRUE) {
            doALC(out, mystate.nfft - mystate.filtertaps);
        }
    }

    /* overlap the last FILTER_LENGTH-1 input data points in the next FFT */
    for (i = 0; i < mystate.filtertaps; i++) {
        samp[i].real = input_save[i].real;
        samp[i].imag = input_save[i].imag;
    }

    isamps = 0;
    for (; i < mystate.nfft - mystate.filtertaps; i++) {
        samp[i].real = in[isamps].real;
        samp[i].imag = in[isamps].imag;
        isamps++;

        if (mystate.iqReversed) // <----- reverse I/Q input channels if reverse flag set.
        {
            tmp = samp[i].real;
            samp[i].real = samp[i].imag;
            samp[i].imag = tmp;
        }

    }

    /* save the last FILTER_LENGTH points for next time */
    for (j = 0; j < mystate.filtertaps; j++, i++) {
        samp[i].real = in[isamps].real;
        samp[i].imag = in[isamps].imag;
        isamps++;

        if (mystate.iqReversed) // <----- reverse I/Q input channels if reverse flag set.
        {
            tmp = samp[i].real;
            samp[i].real = samp[i].imag;
            samp[i].imag = tmp;
        }

        input_save[j].real = samp[i].real;
        input_save[j].imag = samp[i].imag;
    }
    //G_DSP_Busy = FALSE;
}
