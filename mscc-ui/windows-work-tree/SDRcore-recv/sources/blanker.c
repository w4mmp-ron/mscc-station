#include "extern.h"

extern blanker_state nbstate;
extern state mystate;

/*
 * Settings request. initBlanker() runs in the UDP thread and only stores the request;
 * blankerPoll() applies it in the audio thread before the next block. Before, the UDP
 * thread rebuilt nbstate while doBlanker() used it: a shorter width at the wrong moment
 * left oindex past delaylen, it never wrapped, read past the delay line, and a NaN from
 * there locked the AGC (audio silent until restart; Ron 2026-10-29).
 */
static volatile int nb_request = 0;
static uint8_t nb_req_override;
static uint8_t nb_req_enabled;
static float nb_req_threshold;
static int16_t nb_req_width;

static void applyBlanker(uint8_t overridedefaults, uint8_t enabled, float threshold, int16_t width)
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
                nbstate.threshold = (sp_float) threshold / 1000.0f;
                nbstate.rampwidth = width;
        }

        // delay line and gain ramp are 10000 long; delaylen must be at least 1
        if (nbstate.rampwidth < 2) nbstate.rampwidth = 2;
        if (nbstate.rampwidth > 9998) nbstate.rampwidth = 9998;

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
        // index 0 is the resting gain (not blanking): unity (it was 1 - ga, e.g. 0.7)
        nbstate.gainramp[0] = 1.0f;
}

// UDP thread: store the request, applied by blankerPoll() in the audio thread
void initBlanker(uint8_t overridedefaults, uint8_t enabled, float threshold, int16_t width)
{
        nb_req_override = overridedefaults;
        nb_req_enabled = enabled;
        nb_req_threshold = threshold;
        nb_req_width = width;
        nb_request = 1;

        if (overridedefaults) {
                print_time();
                fprintf(G_fp_logfile, "[%d] initBlanker . nbstate.threshold: %f\n", line_number++,
                        (sp_float) threshold / 1000.0f);
        }
}

// Audio thread, before doBlanker: apply a pending request
void blankerPoll(void)
{
        if (!nb_request) return;
        nb_request = 0; // clear first: a request made while applying is applied next block
        applyBlanker(nb_req_override, nb_req_enabled, nb_req_threshold, nb_req_width);
}

void doBlanker(sp_cplx *samps, int nframes)
{
        int tripped = 0;
        int i, j, sp;
        sp_float tmp;
        static int tripcounter = 0;

        if (tripcounter >= nbstate.rampwidth) tripcounter = 0; // width made shorter

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
                if (++nbstate.iindex >= nbstate.delaylen) nbstate.iindex = 0;
                if (++nbstate.oindex >= nbstate.delaylen) nbstate.oindex = 0;
        }
}
