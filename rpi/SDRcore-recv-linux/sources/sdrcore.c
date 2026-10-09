/**************** CORE DSP FOR HF SDR RECEIVER **************
 *               (1992 - 2018)  James L Barber				*
 *************************************************************/
#include "extern.h"
/*#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sdrcore.h"
#include "filter.h"
#include "dsputils.h"
#include "wsfirgen.h"
#include "blanker.h"*/

static int i, j;
static float tempflt;
static float gainmult = 1.0f;
static float windowtaps[8192];
static sp_cplx cptmp;
static sp_cplx input_save[2048]; //allocate for max # of filter taps
static int ifShiftSamps = 0.0f;
static int firstentry = 0;
static sp_float hfilt[32768];
static sp_cplx filt[32768];
static sp_cplx samp[32768];
/* NFM discriminator + de-emphasis (~750 µs US amateur) + DC block */
/* S-meter: average power of each block, smoothed in dB (fast rise, slow fall) */
#define METER_RISE_ALPHA 0.5f   // share of an upward step taken per block (about 2 blocks)
#define METER_FALL_SEC   0.3f   // fall time constant, seconds
#define METER_MIN_POWER  1.0e-20f // floor, avoids log10(0) on an all-zero block
static sp_float meter_db = 0.0f;
static int meter_started = 0;
static sp_float fm_prev_i = 0.0f;
static sp_float fm_prev_q = 0.0f;
static sp_float fm_deemp = 0.0f;
static sp_float fm_dc_x1 = 0.0f;
static sp_float fm_dc_y1 = 0.0f;
static int fm_demod_inited = 0;

/******* External C&C structs, defined in sdrcore-recv.c *******/
extern state mystate;
extern calstate mycalstate;
extern panadapter_buffer panbuffer;
extern cs_state csstate;
extern blanker_state nbstate;

void initDSP()
{

        if (firstentry == 0) {
                firstentry = 1; // only this block at startup!

                // Calculate FFT length order (what power of 2)
                mystate.fftOrder = log((sp_float) mystate.nfft) / log(2.0f);

                // allocate memory for panadapter window coefficients
                panbuffer.winbuf = calloc(mystate.nfft, sizeof(sp_float));

                // Init window for panadapter data
                wHamming(panbuffer.winbuf, mystate.nfft);

                // Init full complex mixer NCO's
                csstate.loFreq = 12000.0f;
                csstate.phaseInc = TPI / (mystate.samplerate / csstate.loFreq);
                csstate.phaseAccum = 0.0f;

                // Init AGC struct
                initAGC(1000.0f);

                // Init noise blanker
                initBlanker(FALSE, 0, 0, 0); // Set noise blanker to default values
        }

        /*  Generate "IF" filter using limits set in state struct
                Note this will run at startup, and any time initDSP is called therafter. 
         */
        memset(hfilt, 0, sizeof(sp_cplx) * mystate.nfft);
        memset(filt, 0, sizeof(sp_cplx) * mystate.nfft);

        wsfirBP(hfilt, mystate.filtertaps, W_HANNING, mystate.lastHRFiltLow / mystate.samplerate,
                mystate.lastHRFiltHigh / mystate.samplerate);

        tempflt = 1.0f / mystate.nfft;

        for (i = 0; i < mystate.filtertaps; i++)
                filt[i].real = tempflt * hfilt[i];

        //fft(filt, mystate.fftOrder);
        jimfft(filt, mystate.nfft);
        mystate.initDSPflag = FALSE;

}

/******************* IF shift, opposite sideband suppression, demo etc ****************/
void fastconv(sp_cplx *in, sp_cplx *out, int frames)
{
        float tmp;
        int isamps = 0;
        int osamps = 0;
        int sbcut = 0;
        static int meterblocks = 0;
        sp_float magaccum = 0.0f;
        sp_float mag;
        sp_float fsbcut = 0.0f;
        sp_float powaccum = 0.0f;
        sp_float blockpow;
        sp_float blockdb;
        sp_float blocksec;

        // re-init DSP if requested
        if (mystate.initDSPflag) initDSP();

        /* do FFT of samples */
        jimfft(samp, mystate.nfft); // No dynamic allocation in this version of the FFT

        /**************** IF SHIFTER **************************************/
        ifShiftSamps = mystate.nfft / 4; // 1/4 of FFT size @ 4096, shift 1/4 or 12 kHz @ Fs = 48K

        /********** FFT masks for unwanted sideband removal *********/
        fsbcut = (mystate.filtHighHz / mystate.samplerate) * (sp_float) mystate.nfft;
        sbcut = (int) fsbcut + 50;

        // remove the opposite sideband
        if (mystate.opmode == MODE_USB) {
                for (i = sbcut; i < mystate.nfft; i++) {
                        samp[i].real = 0.0f;
                        samp[i].imag = 0.0f;
                }
        }

        // remove the opposite sideband
        if ((mystate.opmode == MODE_LSB)) {
                for (i = 0; i < sbcut; i++) {
                        samp[i].real = 0.0f;
                        samp[i].imag = 0.0f;
                }

                for (i = sbcut * 2; i < mystate.nfft / 2; i++) {
                        samp[i].real = 0.0f;
                        samp[i].imag = 0.0f;
                }
        }

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
        /* Write the result out. because a forward FFT was used for the inverse FFT, the output is in the imag part */

        osamps = 0;
        for (i = mystate.filtertaps; i < mystate.nfft; i++) {
                mag = sqrt((samp[i].real * samp[i].real) + (samp[i].imag * samp[i].imag));
                magaccum += (mag * 100000.0f);

                // sum the power for the s-meter (average, not peak: a peak reads noise high)
                powaccum += mag * mag;

                // Simple AM demod - more work required here
                if (mystate.opmode == MODE_AM) {
                        samp[i].real = mag;
                        samp[i].imag = mag;
                }

                /* NFM: atan2 discr + 750 µs de-emphasis + DC block.
                 * Amplitude-independent (no AGC needed after). Fixed AF scale ~SSB loudness. */
                if (mystate.opmode == MODE_FM) {
                        sp_float ii = samp[i].real;
                        sp_float qq = samp[i].imag;
                        sp_float disc;
                        sp_float alpha;
                        sp_float af;
                        if (!fm_demod_inited) {
                                fm_prev_i = ii;
                                fm_prev_q = qq;
                                fm_deemp = 0.0f;
                                fm_dc_x1 = 0.0f;
                                fm_dc_y1 = 0.0f;
                                fm_demod_inited = 1;
                        }
                        disc = atan2f(fm_prev_i * qq - fm_prev_q * ii,
                                      fm_prev_i * ii + fm_prev_q * qq);
                        fm_prev_i = ii;
                        fm_prev_q = qq;
                        disc *= 0.85f;
                        if (disc > 1.0f) disc = 1.0f;
                        if (disc < -1.0f) disc = -1.0f;
                        alpha = 1.0f / (1.0f + (mystate.samplerate * 750.0e-6f));
                        fm_deemp += alpha * (disc - fm_deemp);
                        af = fm_deemp - fm_dc_x1 + 0.995f * fm_dc_y1;
                        fm_dc_x1 = fm_deemp;
                        fm_dc_y1 = af;
                        if (af > 1.0f) af = 1.0f;
                        if (af < -1.0f) af = -1.0f;
                        samp[i].real = af;
                        samp[i].imag = af;
                }

                out[osamps].real = samp[i].real;
                out[osamps].imag = samp[i].real;
                osamps++;
        }

        if (mystate.opmode != MODE_FM)
                fm_demod_inited = 0;

        meterblocks++;
        if (meterblocks >= 2) {
                magaccum /= ((sp_float) mystate.nfft - mystate.filtertaps);
                mystate.avgRxSignalMag = magaccum;
                magaccum = 0.0f;
                meterblocks = 0;
        }

        /* overlap the last FILTER_LENGTH-1 input data points in the next FFT */
        for (i = 0; i < mystate.filtertaps; i++) {
                samp[i].real = input_save[i].real;
                samp[i].imag = input_save[i].imag;
        }

        isamps = 0;
        for (; i < mystate.nfft - mystate.filtertaps; i++) {
                samp[i].real = in[isamps].real * gainmult;
                samp[i].imag = in[isamps].imag * gainmult;
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
                samp[i].real = in[isamps].real * gainmult;
                samp[i].imag = in[isamps].imag * gainmult;
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

        /* S-meter. Average power of this block in dB; a steady carrier reads the same as
         * the old peak did (same -20 constant). Smoothed in dB: up fast, down slowly. */
        blockpow = powaccum / ((sp_float) mystate.nfft - mystate.filtertaps);
        if (blockpow < METER_MIN_POWER) blockpow = METER_MIN_POWER;
        blockdb = 10.0f * log10(blockpow) - 20.0f;
        if (!meter_started) {
                meter_db = blockdb;
                meter_started = 1;
        } else if (blockdb > meter_db) {
                meter_db += (blockdb - meter_db) * METER_RISE_ALPHA;
        } else {
                blocksec = ((sp_float) mystate.nfft - mystate.filtertaps) / mystate.samplerate;
                meter_db += (blockdb - meter_db) * (blocksec / (METER_FALL_SEC + blocksec));
        }
        mystate.peakRxSignalDbm = meter_db;

}
