#include "extern.h"
//#include "version.h"
#include <math.h>

#define RON 1
#define ALC_DEGLITCH_LIMIT 1
extern state mystate;
extern alc_state alcstate;
extern micproc_state micstate;

sp_float mag, phase;
sp_float gain;

#ifdef RON
extern sp_float G_mic_volume;
extern sp_float G_peak;
sp_float G_ALC_gain = 0.0f;
sp_float G_ALC_limit = 0.0f;
long int G_alc_count = 0;
sp_float G_ALC_multiplier = (float)11.0 / 10.0f;
#endif

//#define ALC_TARGET 0.85000f

//#define ALC_TARGET 0.9f
#define ALC_TARGET (G_ALC_limit * G_ALC_multiplier)

// Initialize speech processor state struct

void initMicProc() {
    int j;

    // Use these lines in your UDP code to change parameters on the fly
    // micstate.pregain = pow(10.0f, (sp_float)integerLevel / 20.0f);
    // micstate.enabled = (uint8_t) enabledState;
    // Don't call initMicProc() directly. It's called at init time and shouldn't be 
    // called again.

    micstate.enabled = TRUE;
    micstate.pregain = pow(10.0f, 20.0f / 20.0f); // convert db to linear (FIELD, not POWER)

    micstate.attackms = 75.0f;
    micstate.releasems = 800.0f;

    // gain negative ramp
    micstate.negramp = (float) pow(0.01f, 1.0f / (micstate.attackms * mystate.samplerate * 0.001f));

    // gain positive ramp
    micstate.posramp = (float) pow(0.01f, 1.0f / (micstate.releasems * mystate.samplerate * 0.001f));
    micstate.posramp = (1.0f / micstate.posramp);

    micstate.maxgain = 1.0f;
    micstate.maxval = 1.0f;
    micstate.gain = 1.0f;
    micstate.targetlevel = 0.9f;
}

/* Speech processor - straighforward gain then compress */
void doMicProc(sp_cplx *samps, int nframes) {
    sp_float ioutput, qoutput, isamp, qsamp, wk;
    sp_float peak = 0.0f;
    int j;
#if WINDBG > 0
    static int blocks = 0;
    char buf[255];
#endif
    if (micstate.enabled == TRUE) // is this the best place for the switch?
    {
        for (j = 0; j < nframes; j++) {
            isamp = samps[j].real;
            qsamp = samps[j].imag;

            isamp *= micstate.pregain;
            qsamp *= micstate.pregain;

            wk = fabs(isamp);
            if (wk > micstate.maxval) micstate.maxval = wk;
                // if current peak is greater than target, decay gain
            else micstate.maxval *= micstate.negramp;

            // decide which way to drive gain
            if ((micstate.maxval * micstate.gain) >= micstate.targetlevel) micstate.gain *= micstate.negramp;
            else micstate.gain *= micstate.posramp;

            // set gain to limits, if outside
            if (micstate.gain > micstate.maxgain) micstate.gain = alcstate.maxgain;
            if (micstate.gain < 0.01f) micstate.gain = 0.01f;

            ioutput = isamp * micstate.gain;
            qoutput = qsamp * micstate.gain;

            if (fabs(ioutput) > peak) peak = fabs(ioutput);

            samps[j].real = ioutput;
            samps[j].imag = qoutput;
        }
    }
#if WINDBG > 0
    if (peak > 1.0f) {
        sprintf(buf, "Peak Mic sample level %12.4f\n", peak);
        OutputDebugStringA(buf);
        sprintf(buf, "Current Mic Proc Gain %12.4f\n\n", alcstate.gain);
        OutputDebugStringA(buf);
    }
#endif
}

// Set "real" TX filter parameters from simple band edge parameters - provides a standard interface

void setFilterOffsets(sp_float filterSetLow, sp_float filterSetHigh) {
    sp_float bw;

    // Store the last-used "human readable" filter parameters, because we'll need them when 
    // switching modes. 
    mystate.lastHRFiltHigh = filterSetHigh;
    mystate.lastHRFiltLow = filterSetLow;

    switch (mystate.opmode) {
        case MODE_AM:
            bw = filterSetHigh - filterSetLow;
            mystate.filtHighHz = mystate.lo1_freq + bw;
            mystate.filtLowHz = mystate.lo1_freq - bw;
            mystate.lo1_freq = 12000.0f;
            break;

        case MODE_TUNE:
            bw = filterSetHigh - filterSetLow;
            mystate.filtHighHz = mystate.lo1_freq + bw;
            mystate.filtLowHz = mystate.lo1_freq - bw;
            mystate.lo1_freq = 12000.0f;
            break;

#ifdef RON
        case MODE_CW:
            bw = filterSetHigh - filterSetLow;
            mystate.filtHighHz = mystate.lo1_freq + bw;
            mystate.filtLowHz = mystate.lo1_freq - bw;
            mystate.lo1_freq = 12000.0f;
            break;
#endif

        case MODE_LSB:
            mystate.filtHighHz = mystate.lo1_freq - filterSetLow;
            mystate.filtLowHz = mystate.lo1_freq - filterSetHigh;
            mystate.lo1_freq = 12000.0f;
            break;

        case MODE_USB:
            mystate.filtHighHz = mystate.lo1_freq + filterSetHigh;
            mystate.filtLowHz = mystate.lo1_freq + filterSetLow;
            mystate.lo1_freq = 12000.0f;
            break;

        default:
            break;
    }

    mystate.initDSPflag = TRUE;
}

// This version expects a mono incoming stream and stereo (I/Q) outgoing.
// 6dB boost put in for mono devices

void framesToComplex(sp_float *inframes, sp_cplx *incomplex, sp_cplx *outcomplex, int nframes, int inputchannels) {
    int i;


    if ((mystate.opmode == MODE_CW) || (mystate.opmode == MODE_TUNE)) gain = 0.0f;
    else {
        if (inputchannels == 1) gain = 3.16228f;
        else gain = 6.324f; // give mono devices a 10dB kick
    }

    for (i = 0; i < nframes; i++) {
        //incomplex[i].real = (*inframes * gain) * G_mic_volume;
        incomplex[i].real = (*inframes * gain) * G_mic_volume;
        incomplex[i].imag = incomplex[i].real; // copy left mic channel to right - force mono, since the stereo stream is used for I/Q
        inframes++;
        if (inputchannels == 2) inframes++; // skip over 2nd channel in input stream if stereo

        outcomplex[i].real = 0.0f;
        outcomplex[i].imag = 0.0f;
    }
}

// Initialize ALC state struct

void initALC() {
    int j;
    alcstate.attackms = 1.0f;
    alcstate.releasems = 3000.0f;
    alcstate.envattackms = 1.0f;

    // gain negative ramp
    alcstate.negramp = (float) pow(0.01f, 1.0f / (alcstate.attackms * mystate.samplerate * 0.001f));

    // gain positive ramp
    alcstate.posramp = (float) pow(0.01f, 1.0f / (alcstate.releasems * mystate.samplerate * 0.001f));
    alcstate.posramp = (1.0f / alcstate.posramp);

    // envelope negative ramp
    alcstate.envramp = (float) pow(0.01f, 1.0f / (alcstate.envattackms * mystate.samplerate * 0.001f));

    alcstate.pregain = 1.0f;
    alcstate.maxgain = 1.0f;
    alcstate.maxval = 1.0f;
    alcstate.gain = 1.0f;
    alcstate.targetlevel = ALC_TARGET;
    //alcstate.targetlevel = 0.50000000f;		// 0.9f


}

/* ALC - negative-going only. Use in conjunction with an over-gain input stage */
void doALC(sp_cplx *samps, int nframes) {
    sp_float ioutput, qoutput, isamp, qsamp, wk;
    sp_float peak = 0.0f;
    int j;
    static int peakblocks = 0;

#if WINDBG > 0
    static int blocks = 0;
    char buf[255];
#endif

    for (j = 0; j < nframes; j++) {
        // Calculate envlope
        isamp = samps[j].real;
        qsamp = samps[j].imag;

        if (mystate.opmode != MODE_AM) {
            isamp *= alcstate.pregain;
            qsamp *= alcstate.pregain;
        }

        wk = fabs(isamp);
        if (wk > alcstate.maxval) alcstate.maxval = wk;
            // if current peak is greater than target, decay gain
        else alcstate.maxval *= alcstate.envramp;
        alcstate.targetlevel = ALC_TARGET;
        // decide which way to drive gain
        if ((alcstate.maxval * alcstate.gain) >= alcstate.targetlevel) {
            G_alc_count++;
            alcstate.gain *= alcstate.negramp;
        } else {
            alcstate.gain *= alcstate.posramp;
        }

        // set gain to limits, if outside
        if (alcstate.gain > alcstate.maxgain) {
            alcstate.gain = alcstate.maxgain;
        }
        if (alcstate.gain < 0.1f) {
            alcstate.gain = 0.1f;
        }
        G_ALC_gain = alcstate.gain;
        ioutput = isamp * alcstate.gain;
        qoutput = qsamp * alcstate.gain;

        if (fabs(ioutput) > peak) peak = fabs(ioutput);
        samps[j].real = ioutput;
        samps[j].imag = qoutput;

    }

    G_peak = peak; ///Test

    /*if (peak > 1.0f)
    {
            mystate.overdriven = 1;
            peakblocks = 20;
    }
    else
    {
            peakblocks--;
            if (peakblocks < 1)
            {
                    peakblocks = 0;
                    mystate.overdriven = 0;
            }
    }
     */

#if WINDBG > 0
    blocks = 0;
    if (peak > 1.0f) {
        print_time();
        fprintf(G_fp_logfile, "[%d] doALC. Peak TX sample level %12.4f\n", line_number++, peak);
        print_time();
        fprintf(G_fp_logfile, "[%d] doALC. Current ALC gain %12.4f\n\n", line_number++, alcstate.gain);
        //sprintf(buf, "Peak TX sample level %12.4f\n", peak);
        //OutputDebugStringA(buf);
        //sprintf(buf, "Current ALC gain %12.4f\n\n", alcstate.gain);
        //OutputDebugStringA(buf);
    }
#endif
}

/*
        Shift a complex sample stream down by a fixed Fs/4.
        Note the number of frames must be evenly divisible by 4 for this to work...
 */

void ifShiftDown(sp_cplx *incomplex, int nframes) {
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

void rotateArray(sp_cplx a[], int n, int d) {
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

void rotateByOneLeft(sp_cplx a[], int n) {
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

void rotateByOneRight(sp_cplx a[], int n) {
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

void rect2polar(sp_cplx *samps, int nsamps) {
    int i;
    for (i = 0; i < nsamps; i++) {
        mag = (sp_float) sqrt((samps[i].real * samps[i].real) + (samps[i].imag * samps[i].imag));
        if (samps[i].real == 0.0f) samps[i].real = 1e-20; // prevent divide by zero
        phase = (sp_float) atan(samps[i].imag / samps[i].real);

        if ((samps[i].real < 0.0f) && (samps[i].imag < 0.0f)) phase -= PI;
        if ((samps[i].real < 0.0f) && (samps[i].imag >= 0.0f)) phase += PI;

        samps[i].real = mag;
        samps[i].imag = phase;
    }
}

void polar2rect(sp_cplx *samps, int nsamps) {
    int i;
    for (i = 0; i < nsamps; i++) {
        mag = samps[i].real;
        phase = samps[i].imag;

        samps[i].real = mag * cos(phase);
        samps[i].imag = mag * sin(phase);
    }
}

// In-place complex mixers

void ssb_modulate(sp_cplx *samps) {
    int i;

    for (i = 0; i < mystate.nfft - mystate.filtertaps; i++) {

        samps[i].real *= (sp_float) sin(mystate.lo1_phaseacc);
        samps[i].imag *= (sp_float) cos(mystate.lo1_phaseacc);

        mystate.lo1_phaseacc += mystate.lo1_phaseinc;
        mystate.lo1_phaseacc = atan2(sin(mystate.lo1_phaseacc), cos(mystate.lo1_phaseacc));
        //if (mystate.lo1_phaseacc > TPI)  mystate.lo1_phaseacc -= TPI;	// wrap phase accumulator
        //if (mystate.lo1_phaseacc < -TPI) mystate.lo1_phaseacc += TPI;	// accommodate negative frequencies
    }
}

// AM modulator - generate I and Q carriers, then feed them to the balanced modulator
// with a DC offset, which unbalances them and generates the carrier. Modulate that 
// carrier with mic audio.

void am_modulate(sp_cplx *samps) {
    int i;
    sp_float carrierI, carrierQ;
    sp_float gain = 0.4f;

    for (i = 0; i < mystate.nfft - mystate.filtertaps; i++) {
        carrierI = (sp_float) sin(mystate.lo1_phaseacc);
        carrierQ = (sp_float) cos(mystate.lo1_phaseacc);

        // Experimental asymtetric effect
        if (samps[i].real < 0.0f) samps[i].real *= 0.5f;
        if (samps[i].imag < 0.0f) samps[i].imag *= 0.5f;

        // modulate that stream, adding an offset to create the carrier
        samps[i].real *= gain;
        samps[i].real += mystate.amCarrier;
        samps[i].real *= carrierI;

        samps[i].imag *= gain;
        samps[i].imag += mystate.amCarrier;
        samps[i].imag *= carrierQ;

        // increment the DDS phase accumulator by the phase increment
        mystate.lo1_phaseacc += mystate.lo1_phaseinc;
        // rotate the accumulator if past 2PI radians
        mystate.lo1_phaseacc = atan2(sin(mystate.lo1_phaseacc), cos(mystate.lo1_phaseacc));
    }
}

// TUNE mode modulator - generate I and Q carriers, then feed them to the balanced modulator
// with a DC offset, which unbalances it and generates the carrier.

void tune_modulate(sp_cplx *samps) {
    int i;
    sp_float gain = 1.5f;

    sp_float carrierI, carrierQ;
    for (i = 0; i < mystate.nfft - mystate.filtertaps; i++) {
        carrierI = (sp_float) sin(mystate.lo1_phaseacc);
        carrierQ = (sp_float) cos(mystate.lo1_phaseacc);

        // modulate that stream, adding an offset to create the carrier
        samps[i].real = (carrierI * (1.0f + samps[i].real)) * 0.8f; // TUNE at 80% of available power
        samps[i].imag = (carrierQ * (1.0f + samps[i].imag)) * 0.8f;

        // increment the DDS phase accumulator by the phase increment
        mystate.lo1_phaseacc += mystate.lo1_phaseinc;
        // rotate the accumulator if past 2PI radians. (or -2PI radians if negative freq)
        if (mystate.lo1_phaseacc > TPI) mystate.lo1_phaseacc -= TPI; // wrap phase accumulator
        if (mystate.lo1_phaseacc < -TPI) mystate.lo1_phaseacc += TPI; // accommodate negative frequencies
    }
}

void cw_modulate(sp_cplx *samps) {
    int i;
    sp_float gain = 1.5f;

    sp_float carrierI, carrierQ;
    for (i = 0; i < mystate.nfft - mystate.filtertaps; i++) {
        carrierI = (sp_float) sin(mystate.lo1_phaseacc);
        carrierQ = (sp_float) cos(mystate.lo1_phaseacc);

        // modulate that stream, adding an offset to create the carrier
        samps[i].real = (carrierI * (1.0f + samps[i].real)) * 0.8f; // TUNE at 80% of available power
        samps[i].imag = (carrierQ * (1.0f + samps[i].imag)) * 0.8f;

        // increment the DDS phase accumulator by the phase increment
        mystate.lo1_phaseacc += mystate.lo1_phaseinc;
        // rotate the accumulator if past 2PI radians. (or -2PI radians if negative freq)
        if (mystate.lo1_phaseacc > TPI) mystate.lo1_phaseacc -= TPI; // wrap phase accumulator
        if (mystate.lo1_phaseacc < -TPI) mystate.lo1_phaseacc += TPI; // accommodate negative frequencies
    }
}


// "null" modulate, or set IQ sample stream to zeros. (no output)

void null_modulate(sp_cplx *samps) {
    for (int i = 0; i < mystate.nfft - mystate.filtertaps; i++) {
        samps[i].real = 0.0f;
        samps[i].imag = 0.0f;
    }
}

// complex FFT - Steve's original ported to C for Pee Cee's.

void jimfft(sp_cplx *samps, int n) {
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

void jimifft(sp_cplx *samps, int n) {
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

void twoTone(sp_cplx *samps) {
    static sp_float f1 = 700.0f;
    static sp_float f2 = 1900.0;
    static sp_float pinc1 = 0.0f;
    static sp_float pacc1 = 0.0f;
    static sp_float pinc2 = 0.0f;
    static sp_float pacc2 = 0.0f;
    static int beenhere = 0;
    int j;

    if (beenhere == 0) {
        beenhere = 1;
        pinc1 = TPI / (mystate.samplerate / f1);
        pinc2 = TPI / (mystate.samplerate / f2);
    }

    for (j = 0; j < mystate.nfft - mystate.filtertaps; j++) {
        samps[j].real = (sp_float) cos(mystate.lo1_phaseacc);
        samps[j].imag = (sp_float) sin(mystate.lo1_phaseacc);

        mystate.lo1_phaseacc += mystate.lo1_phaseinc;
        mystate.lo1_phaseacc = atan2(sin(mystate.lo1_phaseacc), cos(mystate.lo1_phaseacc));

        samps[j].real *= (sin(pacc1) + sin(pacc2));
        samps[j].imag *= (cos(pacc1) + cos(pacc2));
        pacc1 += pinc1;
        pacc2 += pinc2;
        pacc1 = atan2(sin(pacc1), cos(pacc1));
        pacc2 = atan2(sin(pacc2), cos(pacc2));
    }

    //if (pacc1 > TPI)  pacc1 -= TPI;	// wrap phase accumulator
    //if (pacc1 < -TPI) pacc1 += TPI;	// accommodate negative frequencies

    //if (pacc2 > TPI)  pacc2 -= TPI;	// wrap phase accumulator
    //if (pacc2 < -TPI) pacc2 += TPI;	// accommodate negative frequencies


}

