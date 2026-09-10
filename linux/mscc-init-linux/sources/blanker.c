#include "extern.h"

extern blanker_state nbstate;
extern state mystate;

void initBlanker(uint8_t overridedefaults, uint8_t enabled, float threshold, int16_t width)
{
        sp_float ga, gb;

        // presets
        nbstate.enabled = TRUE;
        nbstate.threshold = 0.002f;
        nbstate.rampwidth = 30;
        nbstate.attackms = 0.05f;
        nbstate.releasems = 0.05f;
        nbstate.envelope = 0.0f;
        nbstate.gain = 1.0f;
        nbstate.oindex = 0;

        if (overridedefaults) {
                nbstate.enabled = enabled;
                print_time();
                nbstate.threshold = (sp_float) threshold / 1000.0f;
                print_time();
                fprintf(G_fp_logfile, "[%d] initBlanker -> nbstate.threshold: %f\n", line_number++, nbstate.threshold);
                nbstate.rampwidth = width;
        }

        // Calculated from above
        nbstate.delaylen = nbstate.rampwidth / 2;
        nbstate.iindex = nbstate.delaylen - 1;
        nbstate.attack = exp(log(0.01) / (mystate.samplerate * nbstate.attackms * 0.001f));
        nbstate.release = exp(log(0.01) / (mystate.samplerate * nbstate.releasems * 0.001f));

        // Blanking gain ramp coefficients
        ga = 1.0f / (nbstate.rampwidth / 2);
        ga *= 1.5f;
        gb = 1.0f;

        // calculate gain ramp values based on length of ramp
        for (int j = 0; j < nbstate.rampwidth; j++) {
                nbstate.delay[j].real = 0.0f;
                nbstate.delay[j].imag = 0.0f;

                if (j < nbstate.rampwidth / 2) gb -= ga;
                else gb += ga;
                if (gb < 0.0f) nbstate.gainramp[j] = 0.0f;
                else nbstate.gainramp[j] = gb;
        }
}

void doBlanker(sp_cplx *samps, int nframes)
{
        int tripped = 0;
        int i, j, sp;
        sp_float tmp;
        static int tripcounter = 0;

        for (j = 0; j < nframes; j++) {
                // get I and Q samples into delay line
                nbstate.delay[nbstate.iindex].real = samps[j].real;
                nbstate.delay[nbstate.iindex].imag = samps[j].imag;

                // get instantaneous envelope magnitude of complex sample
                tmp = sqrt((samps[j].real * samps[j].real) + (samps[j].imag * samps[j].imag));

                // The "secret sauce" of this algorithm is rather than setting the blanking
                // threshold to a fixed level as compared to the incoming signal samples,
                // it calculates a running signal envelope and sets the blanking threshold
                // at a distance relative to that envelope. That feature allows the threshold
                // to be set much lower (more sensitive) without interfering with strong
                // signals. Even so, though there is a point where it can't be set any lower
                // without blanking desired modulation.

                // apply time constants and update running envelope
                if (tmp > nbstate.envelope)
                        nbstate.envelope = nbstate.attack * (nbstate.envelope - tmp) + tmp;
                else
                        nbstate.envelope = nbstate.release * (nbstate.envelope - tmp) + tmp;

                if (tmp > (nbstate.envelope + nbstate.threshold))
                        tripcounter = nbstate.rampwidth - 1; // new pulse, start at the beginning

                // Process samples by the gain curve. If we're actively working on a pulse,
                // tripcounter is ALWAYS the index into the gain curve array. If we're actively
                // working on a pulse, the gain will be somewhere [ 1.0 .. 0.0 .. 1.0 ] If not,
                // tripcounter will have value of zero, which indexes to a gain of 1.0 . (unity)

                samps[j].real = nbstate.delay[nbstate.oindex].real * nbstate.gainramp[tripcounter];
                samps[j].imag = nbstate.delay[nbstate.oindex].imag * nbstate.gainramp[tripcounter];

                // decrement the index into the gain curve array at the normally sample rate
                if (tripcounter > 0) tripcounter--;

                // rotate the delay line indexes
                if (++nbstate.iindex == nbstate.delaylen) nbstate.iindex = 0;
                if (++nbstate.oindex == nbstate.delaylen) nbstate.oindex = 0;
        }
}
