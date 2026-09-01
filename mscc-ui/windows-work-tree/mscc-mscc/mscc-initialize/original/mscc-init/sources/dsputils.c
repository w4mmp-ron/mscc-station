#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include "sdrcore.h"
#include "wsfirgen.h"
#include "dsputils.h"
#include "sdrcore.h"
#include "blanker.h"
#include "extern.h"

extern state mystate;
extern calstate mycalstate;
extern panadapter_buffer panbuffer;
extern agc_state agcstate;
extern cs_state csstate;
extern blanker_state nbstate;
extern anotch_state anstate;
extern uint8_t G_Panadapter_Blocks;
sp_float mag, phase;
uint8_t AGC_Initializing = 0;

sp_float adelay(sp_float samp, long amount);

void initAGC(sp_float releaseOverride)
{
        int j;
        AGC_Initializing = 1;
        agcstate.attackms = 15.0f;
        //agcstate.attackms = 1.0f;
        agcstate.releasems = 1000.0f;
        agcstate.releasems = releaseOverride;
        agcstate.envattackms = 250.0f;

        // gain negative ramp
        agcstate.negramp = (float) pow(0.01f, 1.0f / (agcstate.attackms * mystate.samplerate * 0.001f));

        // gain positive ramp
        agcstate.posramp = (float) pow(0.01f, 1.0f / (agcstate.releasems * mystate.samplerate * 0.001f));
        agcstate.posramp = (1.0f / agcstate.posramp);

        // envelope negative ramp
        agcstate.envramp = (float) pow(0.01f, 1.0f / (agcstate.envattackms * mystate.samplerate * 0.001f));

        agcstate.maxgain = 1000.0f; // 50dB max AGC gain
        agcstate.maxval = 1.0f;
        agcstate.iindex = MAXDELAY - 1;
        agcstate.oindex = 0;
        agcstate.gain = 1.0f;
        agcstate.targetlevel = 0.5f;

        for (j = 0; j < MAXDELAY; j++) {
                agcstate.idelay[j] = 0.01f;
                agcstate.qdelay[j] = 0.01f;
        }
        AGC_Initializing = 0;
}

void doAGC(sp_cplx *samps, int nframes)
{
        sp_float ioutput, qoutput, isamp, qsamp, wk;
        sp_float peak = 0.0f;
        int j;
#if WINDBG > 0
        static int blocks = 0;
        char buf[255];
#endif
        if (AGC_Initializing)return;
        for (j = 0; j < nframes; j++) {
                // Calculate envelope
                isamp = samps[j].real;
                agcstate.idelay[agcstate.iindex] = isamp;

                qsamp = samps[j].imag;
                agcstate.qdelay[agcstate.iindex] = qsamp;

                wk = fabs(isamp);
                if (wk > agcstate.maxval) agcstate.maxval = wk;
                        // if current peak is greater than target, decay gain
                else agcstate.maxval *= agcstate.envramp;

                // decide which way to drive gain
                if ((agcstate.maxval * agcstate.gain) >= agcstate.targetlevel) agcstate.gain *= agcstate.negramp;
                else agcstate.gain *= agcstate.posramp;

                // set gain to limits, if outside
                if (agcstate.gain > agcstate.maxgain) agcstate.gain = agcstate.maxgain;
                if (agcstate.gain < 0.01f) agcstate.gain = 0.01f;

                ioutput = (agcstate.idelay[agcstate.oindex] * agcstate.gain);
                qoutput = (agcstate.qdelay[agcstate.oindex] * agcstate.gain);

                agcstate.iindex++;
                if (agcstate.iindex == MAXDELAY) agcstate.iindex = 0;
                agcstate.oindex++;
                if (agcstate.oindex == MAXDELAY) agcstate.oindex = 0;

                if (fabs(ioutput) > peak) peak = fabs(ioutput);

                samps[j].real = ioutput;
                samps[j].imag = qoutput;
        }
#if WINDBG > 0
        blocks++;
        if (blocks == 10) {
                blocks = 0;
                sprintf(buf, "%14.8f Peak RX Signal\n", peak);
                OutputDebugStringA(buf);
                sprintf(buf, "%14.8f AGC Gain\n\n", agcstate.gain);
                OutputDebugStringA(buf);
        }
#endif
}

void doPanadapter(sp_cplx *fftbuf, int nframes)
{
        int j, k, bpos = 0;
        int pixels = 800;

        static int panblocks = 8;
        static int panblocks_temp = 1; //One (1) is not a valid value.  The UDP routine will set the correct value. This forces code below to execute
        //at start up.
        uint16_t Y, isamp;
        sp_cplx fbuf[4096];
        sp_cplx dbuf[4096];
        sp_float fpos = 0.0f;
        sp_float maxmag = 16383.0f;
        sp_float mag;
        sp_float fstep = 0.0f;
        sp_float plotframes = 0.0f;

        if (panbuffer.panReady == 1) {
                return; // UDP code hasn't cleared the previous flag, get out
        } else { //Only reset panblocks when Panadapter_thread is NOT processing the buffer
                if (panblocks_temp != G_Panadapter_Blocks) {//Only reset panblocks once when G_Panadapter_Blocks has changed.
                        panblocks_temp = G_Panadapter_Blocks;
                        panblocks = 0;
                }
        }

        // Copy stream data into local FFT buffer
        for (j = 0; j < mystate.nfft; j++) {
                fbuf[j].real = fftbuf[j].real;
                fbuf[j].imag = fftbuf[j].imag;
        }

        // Window data to reduce spectral leakage in display
        for (j = 0; j < mystate.nfft; j++) {
                fbuf[j].real *= panbuffer.winbuf[j];
                fbuf[j].imag *= panbuffer.winbuf[j];
        }

        // Run local FFT on windowed panadapter data
        jimfft(fbuf, mystate.nfft);

        // Shuffle the response from [ 0->N/2 | N/2->N-1 ]  to [N-1->0 | 0->N/2 ]
        // (Same as the "fftshift" function in MatLab

        k = mystate.nfft / 2;
        for (j = 0; j < mystate.nfft / 2; j++) {
                dbuf[j].real = fbuf[k].real;
                dbuf[j].imag = fbuf[k].imag;
                k--;
        }

        k = mystate.nfft - 1;
        for (j = mystate.nfft / 2; j < mystate.nfft; j++) {
                dbuf[j].real = fbuf[k].real;
                dbuf[j].imag = fbuf[k].imag;
                k--;
        }

        // 96 kHz
        bpos = 1024;
        fpos = (sp_float) bpos;
        plotframes = (sp_float) mystate.nfft - 1024;
        fstep = (float) plotframes / (float) pixels;

        for (j = 0; j < pixels; j++) {
                // calculate magnitude of bin
                mag = sqrt((dbuf[bpos].real * dbuf[bpos].real) + (dbuf[bpos].imag * dbuf[bpos].imag));

                // Convert to dBm and offset
                mag = (10.0f * log10(mag));
                mag += 22.0f;
                mag *= 150.0f;

                if (mag < 0.0f) mag = 0.0f;


                // scale and cast normalized sample to uint16_t
                if (mag > maxmag) mag = maxmag;
                isamp = (uint16_t) mag;

                // Histogram routine. Bin magnitude halves each time new sample is less than previous
                if (isamp >= panbuffer.Y[j]) {
                        panbuffer.Y[j] = isamp;
                } else if (panbuffer.Y[j] > 1) panbuffer.Y[j] *= 0.95f;

                fpos += fstep;
                bpos = (int) fpos;
        }

        panblocks++;
        if (panblocks == G_Panadapter_Blocks) {
                panblocks = 0;
                panbuffer.panReady = 1;
        }
}

/***** 3-freq Goertzel detector - used in Si5351 calibration *****/
void doRxCalibrate(sp_cplx *incomplex, int nframes)
{
        static int index = 0;
        int j;
        sp_float magm, mag, magp;

        for (j = 0; j < nframes; j++) {
                mycalstate.calbuffer[index].real = incomplex[j].real;
                mycalstate.calbuffer[index].imag = incomplex[j].imag;

                index++;
                if (index == mycalstate.Cycle_Count) {
                        mycalstate.calStart = FALSE;
                        index = 0;
                        break;
                }
        }
        if (mycalstate.calStart == TRUE) return; // get out - buffer not filled yet

        // Buffer is filled, let's process it
        magm = goertzel_mag(mycalstate.Cycle_Count, mycalstate.freq_low, mycalstate.calbuffer);
        mag = goertzel_mag(mycalstate.Cycle_Count, mycalstate.freq_center, mycalstate.calbuffer);
        magp = goertzel_mag(mycalstate.Cycle_Count, mycalstate.freq_high, mycalstate.calbuffer);

        mycalstate.calMagLow = (int) (magm * 1000000.0f);
        mycalstate.calMag = (int) (mag * 1000000.0f);
        mycalstate.calMagHigh = (int) (magp * 1000000.0f);

        mycalstate.calReady = TRUE;

}

// Goertel filter/detector (aka DFT) on arbitrary size buffer

sp_float goertzel_mag(int nsamps, sp_float freq, sp_cplx data[])
{
        int k, i;
        sp_float fsamps;
        sp_float omega, sine, cosine, coeff, q0, q1, q2, magnitude, real, imag;
        sp_float scalingFactor = nsamps / 2.0f;

        fsamps = (float) nsamps;
        k = (int) (0.5f + ((fsamps * freq) / mystate.samplerate));
        omega = (2.0f * PI * k) / fsamps;
        sine = sin(omega);
        cosine = cos(omega);
        coeff = 2.0f * cosine;
        q0 = 0.0f;
        q1 = 0.0f;
        q2 = 0.0f;

        for (i = 0; i < nsamps; i++) {
                q0 = coeff * q1 - q2 + data[i].real;
                q2 = q1;
                q1 = q0;
        }

        // calculate the real and imaginary results
        // scaling appropriately
        real = (q1 - q2 * cosine); // / scalingFactor;
        imag = (q2 * sine); // / scalingFactor;

        magnitude = sqrt((real * real) + (imag * imag));
        real = 0.0f;
        return magnitude;
}

// In the TX code, this function translates "human readable" filter settings into those
// needed for the TX DSP. In the RX, however, there is no need for translation as the receiver
// uses them in "human readable" form - ** EXCEPT for AM ** AM is different in RX than TX,
// but in RX is also different than SSB. Go Figure.

void setFilterOffsets(sp_float filterSetLow, sp_float filterSetHigh)
{
        // Store the last-used "human readable" filter parameters, because we'll need them when 
        // switching modes. 
        mystate.lastHRFiltHigh = filterSetHigh;
        mystate.lastHRFiltLow = filterSetLow;

        // As above, RX doesn't need translation except on AM.
        if (mystate.opmode == MODE_AM) {
                mystate.filtLowHz = filterSetLow;
                mystate.filtHighHz = filterSetHigh * 2.0f; // TWO sidebands.
        } else {
                mystate.filtHighHz = filterSetHigh;
                mystate.filtLowHz = filterSetLow;
        }

        mystate.initDSPflag = TRUE;
}

/***** Convert incoming interleaved frames to complex form *****/
void framesToComplex(sp_float *inframes, sp_cplx *incomplex, sp_cplx *outcomplex, int nframes)
{
        int i;

        for (i = 0; i < nframes; i++) {
                incomplex[i].real = *inframes * mystate.iMult;
                inframes++;

                incomplex[i].imag = *inframes * mystate.qMult;
                inframes++;

                incomplex[i].real += 1.0e-18f;
                incomplex[i].real -= 1.0e-18f;

                incomplex[i].imag += 1.0e-18f;
                incomplex[i].imag -= 1.0e-18f;

                outcomplex[i].real = 0.0f;
                outcomplex[i].imag = 0.0f;
        }
}

/*
        Shift a complex sample stream down by a fixed Fs/4.
        Note the number of frames must be evenly divisible by 4 for this to work...
 */

void ifShiftDown(sp_cplx *incomplex, int nframes)
{
        sp_float hh1, hh2;
        int i;

        for (i = 0; i < nframes; i += 4) {
                hh1 = -incomplex[i + 1].imag;
                hh2 = incomplex[i + 1].real;
                incomplex[i + 1].real = hh1;
                incomplex[i + 1].imag = hh2;

                hh1 = -incomplex[i + 2].real;
                hh2 = -incomplex[i + 2].imag;
                incomplex[i + 2].real = hh1;
                incomplex[i + 2].imag = hh2;

                hh1 = incomplex[i + 3].imag;
                hh2 = -incomplex[i + 3].real;
                incomplex[i + 3].real = hh1;
                incomplex[i + 3].imag = hh2;
        }
}

// N = length, D = rotation amount

void rotateArray(sp_cplx a[], int n, int d)
{
        int i, ctr;

        if (d < 0) {
                ctr = abs(d);
                for (i = 0; i < ctr; i++) {
                        rotateByOneLeft(a, n);
                }
        } else {
                for (i = 0; i < d; i++) rotateByOneRight(a, n);
        }
}

void rotateByOneLeft(sp_cplx a[], int n)
{
        int i;
        sp_cplx temp;

        temp.real = a[0].real;
        temp.imag = a[0].imag;

        for (i = 0; i < n - 1; i++) {
                a[i].real = a[i + 1].real;
                a[i].imag = a[i + 1].imag;
        }
        a[n - 1].real = temp.real;
        a[n - 1].imag = temp.imag;
}

void rotateByOneRight(sp_cplx a[], int n)
{
        int i;
        sp_cplx temp;

        temp.real = a[n - 1].real;
        temp.imag = a[n - 1].imag;

        for (i = n - 1; i > 0; i--) {
                a[i].real = a[i - 1].real;
                a[i].imag = a[i - 1].imag;
        }
        a[0].real = temp.real;
        a[0].imag = temp.imag;
}

void rect2polar(sp_cplx *samps, int nsamps)
{
        int i;
        for (i = 0; i < nsamps; i++) {
                mag = (sp_float) sqrt((samps[i].real * samps[i].real) + (samps[i].imag * samps[i].imag));
                if (samps[i].real == 0.0f) samps[i].real = 01e-20; // prevent divide by zero
                phase = (sp_float) atan(samps[i].imag / samps[i].real);

                if ((samps[i].real < 0.0f) && (samps[i].imag < 0.0f)) phase -= PI;
                if ((samps[i].real < 0.0f) && (samps[i].imag >= 0.0f)) phase += PI;

                samps[i].real = mag;
                samps[i].imag = phase;
        }
}

void polar2rect(sp_cplx *samps, int nsamps)
{
        int i;
        for (i = 0; i < nsamps; i++) {
                mag = samps[i].real;
                phase = samps[i].imag;

                samps[i].real = mag * cos(phase);
                samps[i].imag = mag * sin(phase);
        }
}

// In-place complex freq shifter (DO NOT REMOVE - MAY BE USED LATER)

void complex_shift(sp_cplx *samps, int nsamps)
{
        sp_float im1, isum, im2, qm1, qsum, qm2, cnco, snco;
        int i;

        if (nbstate.enabled == TRUE) doBlanker(samps, nsamps);
        doPanadapter(samps, nsamps);


        for (i = 0; i < nsamps; i++) {
                cnco = (sp_float) cos(csstate.phaseAccum);
                im1 = samps[i].real * cnco;
                snco = (sp_float) sin(csstate.phaseAccum);
                im2 = samps[i].imag * snco;
                isum = im1 - im2;

                qm1 = samps[i].imag * cnco;
                qm2 = samps[i].real * snco;
                qsum = qm1 + qm2;

                samps[i].real = isum;
                samps[i].imag = qsum;

                csstate.phaseAccum += csstate.phaseInc;
                if (csstate.phaseAccum > TPI) csstate.phaseAccum -= TPI; // wrap phase accumulator
                if (csstate.phaseAccum < -TPI) csstate.phaseAccum += TPI; // accommodate negative frequencies

        }
}

// complex FFT - Steve's original ported to C for Pee Cee's.

void jimfft(sp_cplx *samps, int32_t n)
{
        int nm1, nd2, m, j, i, k, l, le, le2, jm1, ip;
        sp_float tr, ti, ur, ui, sr, si;

        nm1 = n - 1;
        nd2 = n / 2;
        m = (long) (log((sp_float) n) / log(2.0f));
        j = nd2;

        for (i = 1; i < (n - 1); i++) {
                if (i >= j) goto s1190;
                tr = samps[j].real;
                ti = samps[j].imag;
                samps[j].real = samps[i].real;
                samps[j].imag = samps[i].imag;
                samps[i].real = tr;
                samps[i].imag = ti;
s1190:
                k = nd2;

s1200:
                if (k > j) goto s1240;
                j -= k;
                k /= 2;
                goto s1200;
s1240:
                j += k;
        }

        for (l = 1; l < m + 1; l++) {
                le = (long) pow(2.0f, l);
                le2 = le / 2;
                ur = 1.0f;
                ui = 0.0f;
                sr = cos(PI / (sp_float) le2);
                si = -sin(PI / (sp_float) le2);

                for (j = 1; j < le2 + 1; j++) {
                        jm1 = j - 1;

                        for (i = jm1; i < nm1 + 1; i += le) {
                                ip = i + le2;
                                tr = samps[ip].real * ur - samps[ip].imag * ui;
                                ti = samps[ip].real * ui + samps[ip].imag * ur;
                                samps[ip].real = samps[i].real - tr;
                                samps[ip].imag = samps[i].imag - ti;
                                samps[i].real += tr;
                                samps[i].imag += ti;
                        }

                        tr = ur;
                        ur = (tr * sr) - (ui * si);
                        ui = (tr * si) + (ui * sr);
                }

        }

        return;

}

// inverse complex FFT

void jimifft(sp_cplx *samps, int32_t n)
{
        long k, i;
        sp_float fn;

        fn = (sp_float) n;

        for (k = 0; k < n; k++)
                samps[k].imag = -samps[k].imag;

        jimfft(samps, n);

        for (i = 0; i < n; i++) {
                samps[i].real = (samps[i].real / fn);
                samps[i].imag = (-samps[i].imag / fn);
        }

        return;

}

void anotch(sp_cplx *samps, int32_t n)
{
        static long numtaps = 100; /* length of adaptive filter */
        static long ndelay = 300; /* delay line length in samples */
        static sp_float mu = 0.00002f; /* rate of adaptation */
        static sp_float gain = 1.0f; /* processing gain */
        static sp_float alpha = .02f; /* derived value used in coefficient of adaptation equation */
        static sp_float sum;
        static sp_float sigma = 2.0f;
        static sp_float history[200];
        static sp_float taps[200];

        sp_float d, e, mu_e, acc;
        sp_float samp;
        int j, i;

        for (j = 0; j < n; j++) {
                /*
                Roughly modified for 96K sampling 2019-01-09 jlb
                Note these are also specfied for SSB communications audio
                Not -as- critical as for denoise function
                 */

                samp = samps[j].real;

                /* convert a sample to double, multiply by gain, and store it.  (D) */
                d = (samp * gain);

                /* delay it		(X) */
                history[0] = adelay(d, ndelay);
                acc = 0.0L;

                /* filter the sample */
                for (i = 0; i < numtaps; i++) {
                        acc += taps[i] * history[i];
                }

                /* e is the error signal (difference between desired and realized) */
                e = d - acc;

                /* update sigma (coefficient of adaptation) */
                sigma = alpha * (history[0] * history[0]) + (1.0L - alpha) * sigma;
                mu_e = mu * e / sigma;

                /* update the filter coefficients */
                for (i = 0; i < numtaps; i++) {
                        taps[i] += (mu_e * history[i]);
                }

                /* update the history array */
                for (i = numtaps; i >= 1; i--) {
                        history[i] = history[i - 1];
                }

                samps[j].real = e; // Since this is an audio-channel block, force Q (right) channel to follow I (left)
                samps[j].imag = e;

        } /* ANOTCH ENDS */
}

// delay (samp) by (amount)
// Used to delay samples for AUTONOTCH routine

sp_float adelay(sp_float samp, long amount)
{
        static long indelay = 0;
        static long outdelay = 0;
        static sp_float buckets[500];

        buckets[indelay] = samp;

        outdelay--;
        if (outdelay < 0) outdelay = amount - 1;

        indelay--;
        if (indelay < 0) indelay = amount - 1;

        return buckets[outdelay];

}
