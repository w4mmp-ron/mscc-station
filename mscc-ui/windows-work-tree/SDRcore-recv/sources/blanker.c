#include "extern.h"

extern blanker_state nbstate;
extern state mystate;

/*
 * Automatic noise blanker (recv 3.152, Ron 2026-11-01).
 *
 * Runs on the raw 96 kHz I/Q before the frequency shift (dsputils.c complex_shift), where
 * an impulse is still one short, tall spike.
 *
 * How it decides what a pulse is:
 *   - It keeps a running MEAN of the I/Q magnitude over about 2 ms (10x an impulse, but
 *     fast enough to follow a signal keying on in a few ms). Pulse samples are left out, so
 *     a pulse does not raise the level it is judged against.
 *   - A "pulse" longer than the pulse width (at most 2 ms) is a signal, not a pulse: the
 *     mean jumps to its level and blanking stops. Without this a signal starting faster
 *     than the mean could follow was taken as one endless pulse and blanked for good.
 *   - A sample is a pulse when it is more than K times that mean. K is RELATIVE, so the same
 *     setting works on a quiet band and on a noisy one (the old blanker used an absolute
 *     amount above a 0.05 ms envelope, which nearly followed the pulse itself).
 *     Band noise: the chance of a noise peak above K x mean is exp(-0.785 K^2), so at
 *     K = 4 it is about 3 per million samples (~0.3 false blanks per second at 96 kHz),
 *     at K = 6 practically never.
 *   - K comes from the client's threshold slider (no client change): NB_Threshold 0..9.99
 *     -> K = 4 (most sensitive) .. 8 (least). Default (no client value) K = 6.
 *   - Self-protection: if it blanks more than BLANK_RATE_HIGH pulses per second, K is raised
 *     step by step (up to 4x), and lowered again when the rate drops. Real impulse noise
 *     (power line, ignition) is well below that; a too-low K on a strong signal is not.
 *   - The first 50 ms after a (re)start only learn the level; nothing is blanked.
 *
 * How it blanks (the gain):
 *   - The I/Q is delayed by half the pulse width (half) plus the longest pulse (maxrun),
 *     e.g. 15 + 30 samples = 0.47 ms at width 30. The gain at the output is set by the
 *     distance to the nearest pulse sample, ahead (still in the delay) or behind:
 *     0 within +/- hold samples (hold = half / 3), a raised-cosine ramp back to 1 at
 *     half. The extra maxrun of delay means a "pulse" that turns out to be a signal is
 *     un-marked before the gain starts to fall: signals starting fast are never blanked.
 *     So the gain is already down when the pulse arrives, pulses close together
 *     keep the gain down (the old ramp restarted and jumped up), and there is no step.
 *   - Width: the client slider is in MICROSECONDS (labelled "uS", 10..510). Before 3.152
 *     recv used the number as 96 kHz samples, 10.4x longer than the label (510 = 5.3 ms).
 *     Now converted: samples = us x 96000 / 1e6 (200 us = 19 samples). Ron 2026-11-01.
 *
 * Log: every 10 s while on, pulses per second, K in use and the mean level.
 */

#define NB_MEAN_MS       2.0f     // running mean time constant
#define NB_MAXRUN_MS     2.0f     // longest pulse; longer = a signal
#define NB_WARMUP_MS     50.0f    // learn the level first, no blanking
#define NB_K_MIN         4.0f     // slider at most sensitive
#define NB_K_MAX         8.0f     // slider at least sensitive
#define NB_K_DEFAULT     6.0f     // before the client sends a threshold
#define NB_WIDTH_US_DEFAULT 200.0f // before the client sends a width
#define BLANK_RATE_HIGH  400.0f   // pulses/s: above this, raise K (self-protection)
#define BLANK_RATE_LOW   200.0f   // pulses/s: below this, let K come back down
#define NB_KBOOST_MAX    4.0f
#define NB_RATE_STEP_S   0.1f     // self-protection check interval
#define NB_LOG_S         10.0f    // log interval
#define NB_NEAR_FRAC     0.67f    // near miss: above 0.67 x K (4x at K 6), below K

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

// self-protection and log counters (audio thread only)
static int nb_step_count = 0;     // samples in this check interval
static int nb_step_pulses = 0;    // pulses in this check interval
static float nb_rate = 0.0f;      // smoothed pulses per second
static int nb_log_count = 0;      // samples since last log
static long nb_log_pulses = 0;    // pulses since last log
// diagnostics in the log (Ron 2026-11-01: one 54x click passed with NB on; which cause?)
static long nb_log_toolong = 0;   // "pulses" longer than maxrun, taken as signals (not blanked)
static long nb_log_near = 0;      // near misses: peaks between NB_NEAR_FRAC x K and K x mean
static float nb_log_pkpulse = 0;  // biggest pulse / mean (blanked)
static float nb_log_pklong = 0;   // biggest too-long run / mean (not blanked)
static float nb_log_pknear = 0;   // biggest near miss / mean (not blanked)
static float nb_run_peak = 0;     // biggest ratio in the current run
static uint8_t nb_near_prev = 0;  // last sample was a near miss (count events, not samples)
static void nbLog(float secs);

/*
 * Slider moved while the blanker runs: keep the learned level and the audio in the delay
 * ring (before, every step restarted it: the ring was zeroed = up to 4 ms of silence at
 * a large width, and 50 ms of learning; Ron's slider test 2026-11-01). Only a width
 * change resizes the ring: shorter drops the oldest delayed samples, longer repeats the
 * oldest stretch once. Either is one small jump in time, no silence.
 */
static sp_cplx nb_tmp[10000];
static uint8_t nb_tmpflag[10000];

static void nbResizeRing(int newlen)
{
        int oldD = nbstate.ringlen - 1, newD = newlen - 1;
        int p = nbstate.widx, n;

        // pending samples (not yet output), oldest first: slots widx+1 .. widx-1
        for (n = 0; n < oldD; n++) {
                if (++p >= nbstate.ringlen) p = 0;
                nb_tmp[n] = nbstate.delay[p];
                nb_tmpflag[n] = nbstate.flag[p];
        }
        // new ring: slot 0 = already output, slots 1..newD = pending, oldest first
        for (n = 0; n < newD; n++) {
                int src;
                if (newD <= oldD) src = oldD - newD + n;        // newest newD samples
                else if (n < newD - oldD) src = n % oldD;        // repeat the oldest stretch
                else src = n - (newD - oldD);                    // then all of them
                nbstate.delay[n + 1] = nb_tmp[src];
                nbstate.flag[n + 1] = nb_tmpflag[src];
        }
        nbstate.delay[0].real = nbstate.delay[0].imag = 0.0f;
        nbstate.flag[0] = 0;
        nbstate.widx = 0;
        nbstate.ringlen = newlen;
        nbstate.delaylen = newD;

        nbstate.ahead = -1;                                      // next pulse in the ring
        for (n = 1; n <= newD; n++) {
                if (nbstate.flag[n]) {
                        nbstate.ahead = n;
                        break;
                }
        }
}

static void applyBlanker(uint8_t overridedefaults, uint8_t enabled, float threshold, int16_t width)
{
        float fs = (float) mystate.samplerate;
        static int nb_started = 0;                 // ring and mean are valid
        int wasOn = nbstate.enabled && nb_started;
        int oldlen = nbstate.ringlen;

        // presets (startup default: enabled until the client sends its setting, as before)
        nbstate.enabled = TRUE;
        nbstate.kbase = NB_K_DEFAULT;
        nbstate.rampwidth = (int) (NB_WIDTH_US_DEFAULT * fs / 1.0e6f + 0.5f);
        int width_us = (int) NB_WIDTH_US_DEFAULT;

        if (overridedefaults) {
                nbstate.enabled = enabled;
                // NB_Threshold from udp_thread.c: 0 (slider max) .. 9.99 (slider min)
                if (threshold < 0.0f) threshold = 0.0f;
                if (threshold > 10.0f) threshold = 10.0f;
                nbstate.kbase = NB_K_MIN + (NB_K_MAX - NB_K_MIN) * threshold / 10.0f;
                width_us = width;
                nbstate.rampwidth = (int) ((float) width * fs / 1.0e6f + 0.5f); // us -> samples
        }

        // ring holds delaylen + 1 samples, arrays are 10000 long (max 4999 + 192 + 1)
        if (nbstate.rampwidth < 4) nbstate.rampwidth = 4;
        if (nbstate.rampwidth > 9998) nbstate.rampwidth = 9998;

        nbstate.half = nbstate.rampwidth / 2;
        nbstate.maxrun = nbstate.rampwidth;
        if (nbstate.maxrun > (int) (fs * NB_MAXRUN_MS * 0.001f))
                nbstate.maxrun = (int) (fs * NB_MAXRUN_MS * 0.001f);
        nbstate.hold = nbstate.half / 3;
        if (nbstate.hold < 1) nbstate.hold = 1;
        nbstate.run = 0;

        if (wasOn && nbstate.enabled) {
                // running and stays on: keep level, rate guard and audio
                int newlen = nbstate.half + nbstate.maxrun + 1;
                if (newlen != oldlen) nbResizeRing(newlen);
                if (nbstate.since > nbstate.half) nbstate.since = nbstate.half;
                print_time();
                fprintf(G_fp_logfile, "[%d] Blanker . setting: K %.2f, width %d us = %d samples (delay %d)\n",
                        line_number++, nbstate.kbase, width_us, nbstate.rampwidth, nbstate.delaylen);
                return;
        }
        if (wasOn && !nbstate.enabled && nb_log_count > 0)
                nbLog((float) nb_log_count / fs); // NB turned off: log the part interval
        nb_started = nbstate.enabled;

        nbstate.delaylen = nbstate.half + nbstate.maxrun;
        nbstate.ringlen = nbstate.delaylen + 1;

        for (int j = 0; j < nbstate.ringlen; j++) {
                nbstate.delay[j].real = 0.0f;
                nbstate.delay[j].imag = 0.0f;
                nbstate.flag[j] = 0;
        }
        nbstate.widx = 0;
        nbstate.ahead = -1;
        nbstate.since = nbstate.half; // gain 1
        nbstate.prevflag = 0;

        nbstate.kboost = 1.0f;
        nbstate.mean = 0.0f;
        nbstate.meancoef = 1.0f / (fs * NB_MEAN_MS * 0.001f);
        nbstate.warmup = (int) (fs * NB_WARMUP_MS * 0.001f);

        nb_step_count = 0;
        nb_step_pulses = 0;
        nb_rate = 0.0f;
        nb_log_count = 0;
        nb_log_pulses = 0;
        nb_log_toolong = nb_log_near = 0;
        nb_log_pkpulse = nb_log_pklong = nb_log_pknear = nb_run_peak = 0.0f;
        nb_near_prev = 0;

        print_time();
        fprintf(G_fp_logfile, "[%d] Blanker . enabled %d, K %.2f, width %d us = %d samples (delay %d, ramp %d, hold %d, max pulse %d)\n",
                line_number++, nbstate.enabled, nbstate.kbase, width_us, nbstate.rampwidth,
                nbstate.delaylen, nbstate.half, nbstate.hold, nbstate.maxrun);
}

// UDP thread: store the request, applied by blankerPoll() in the audio thread
void initBlanker(uint8_t overridedefaults, uint8_t enabled, float threshold, int16_t width)
{
        nb_req_override = overridedefaults;
        nb_req_enabled = enabled;
        nb_req_threshold = threshold;
        nb_req_width = width;
        nb_request = 1;
}

// Audio thread, before doBlanker: apply a pending request
void blankerPoll(void)
{
        if (!nb_request) return;
        nb_request = 0; // clear first: a request made while applying is applied next block
        applyBlanker(nb_req_override, nb_req_enabled, nb_req_threshold, nb_req_width);
}

// gain for a distance (samples) to the nearest pulse: 0 inside hold, raised cosine to 1
static inline sp_float nbRamp(int d)
{
        if (d <= nbstate.hold) return 0.0f;
        if (d >= nbstate.half) return 1.0f;
        sp_float x = (sp_float) (d - nbstate.hold) / (sp_float) (nbstate.half - nbstate.hold);
        return 0.5f - 0.5f * cosf((sp_float) PI * x);
}

/*
 * The last nbstate.run ring samples were marked as a pulse but it went on too long: it is a
 * signal. Un-mark them (none has reached the output yet: delaylen >= half + maxrun, so the
 * gain had not started to fall) and find the next real pulse in the ring, if any.
 * Called before the current sample is written: widx = slot output last time.
 */
static void nbUnmarkRun(void)
{
        int p = nbstate.widx;
        for (int n = 0; n < nbstate.run; n++) {
                if (--p < 0) p = nbstate.ringlen - 1;
                nbstate.flag[p] = 0;
        }
        nbstate.run = 0;
        if (nb_step_pulses > 0) nb_step_pulses--; // it was counted as a pulse
        if (nb_log_pulses > 0) nb_log_pulses--;
        nb_log_toolong++;
        if (nb_run_peak > nb_log_pklong) nb_log_pklong = nb_run_peak;
        nb_run_peak = 0.0f;

        // ahead is relative to the last output (slot widx); rescan the ring after it
        nbstate.ahead = -1;
        p = nbstate.widx;
        for (int d = 1; d < nbstate.ringlen; d++) {
                if (++p >= nbstate.ringlen) p = 0;
                if (nbstate.flag[p]) {
                        nbstate.ahead = d;
                        break;
                }
        }
}

// self-protection check and periodic log, once per NB_RATE_STEP_S
static void nbRateStep(float fs)
{
        float r = (float) nb_step_pulses / NB_RATE_STEP_S;

        nb_rate = 0.7f * nb_rate + 0.3f * r;
        if (nb_rate > BLANK_RATE_HIGH) {
                nbstate.kboost *= 1.25f;
                if (nbstate.kboost > NB_KBOOST_MAX) nbstate.kboost = NB_KBOOST_MAX;
        } else if (nb_rate < BLANK_RATE_LOW && nbstate.kboost > 1.0f) {
                nbstate.kboost /= 1.05f;
                if (nbstate.kboost < 1.0f) nbstate.kboost = 1.0f;
        }
        nb_step_count = 0;
        nb_step_pulses = 0;

        if (nb_log_count >= (int) (fs * NB_LOG_S)) nbLog(NB_LOG_S);
}

/*
 * Log line, every NB_LOG_S while on and when NB is turned off. Per second:
 *   pulses   = blanked (biggest = peak / mean)
 *   too long = over K but longer than maxrun: taken as a signal, NOT blanked
 *   near     = peaks between NB_NEAR_FRAC x K and K: NOT blanked
 */
static void nbLog(float secs)
{
        if (secs < 0.1f) secs = 0.1f;
        print_time();
        fprintf(G_fp_logfile, "[%d] Blanker . %.1f s: %.1f pulses/s (biggest %.0fx), too long %.1f/s (biggest %.0fx), "
                "near %.1f/s (biggest %.1fx), K %.2f (base %.2f x %.2f), mean %.6f\n",
                line_number++, secs, (float) nb_log_pulses / secs, nb_log_pkpulse,
                (float) nb_log_toolong / secs, nb_log_pklong,
                (float) nb_log_near / secs, nb_log_pknear,
                nbstate.kbase * nbstate.kboost, nbstate.kbase, nbstate.kboost, nbstate.mean);
        nb_log_count = 0;
        nb_log_pulses = nb_log_toolong = nb_log_near = 0;
        nb_log_pkpulse = nb_log_pklong = nb_log_pknear = 0.0f;
}

void doBlanker(sp_cplx *samps, int nframes)
{
        float fs = (float) mystate.samplerate;
        int stepLen = (int) (fs * NB_RATE_STEP_S);
        sp_float k = nbstate.kbase * nbstate.kboost;

        for (int j = 0; j < nframes; j++) {
                sp_float re = samps[j].real;
                sp_float im = samps[j].imag;
                sp_float m = sqrtf(re * re + im * im);
                uint8_t isPulse = 0;

                // 1. pulse or not, against the running mean (pulses do not update it)
                if (nbstate.warmup > 0) {
                        nbstate.warmup--;
                        nbstate.mean += 5.0f * nbstate.meancoef * (m - nbstate.mean); // learn fast
                } else if (m > k * nbstate.mean + 1e-9f) {
                        sp_float ratio = m / (nbstate.mean + 1e-12f);
                        if (ratio > nb_run_peak) nb_run_peak = ratio;
                        if (nbstate.run < nbstate.maxrun) {
                                isPulse = 1;
                                nbstate.run++;
                        } else {
                                nbstate.mean = m; // too long for a pulse: a signal, take its level
                                nbUnmarkRun();
                        }
                } else {
                        // a run that ended within maxrun was a real (blanked) pulse
                        if (nbstate.run > 0 && nb_run_peak > nb_log_pkpulse) nb_log_pkpulse = nb_run_peak;
                        nb_run_peak = 0.0f;
                        nbstate.run = 0;

                        uint8_t isNear = (m > NB_NEAR_FRAC * k * nbstate.mean);
                        if (isNear) {
                                sp_float ratio = m / (nbstate.mean + 1e-12f);
                                if (!nb_near_prev) nb_log_near++;
                                if (ratio > nb_log_pknear) nb_log_pknear = ratio;
                        }
                        nb_near_prev = isNear;
                        nbstate.mean += nbstate.meancoef * (m - nbstate.mean);
                }
                if (!isfinite(nbstate.mean)) nbstate.mean = 0.0f;

                if (isPulse && !nbstate.prevflag) {
                        nb_step_pulses++;
                        nb_log_pulses++;
                }
                nbstate.prevflag = isPulse;

                // 2. into the ring; it reaches the output delaylen samples from now
                int w = nbstate.widx;
                nbstate.delay[w].real = re;
                nbstate.delay[w].imag = im;
                nbstate.flag[w] = isPulse;

                if (nbstate.ahead >= 0) nbstate.ahead--;   // one sample closer
                if (nbstate.since < nbstate.half) nbstate.since++;
                if (isPulse && nbstate.ahead < 0) nbstate.ahead = nbstate.delaylen;

                // 3. output: the oldest ring sample (written delaylen samples ago)
                int r = w + 1;
                if (r >= nbstate.ringlen) r = 0;

                if (nbstate.ahead == 0) nbstate.since = 0; // this output sample is a pulse

                sp_float ga = (nbstate.ahead < 0) ? 1.0f : nbRamp(nbstate.ahead);
                sp_float gb = nbRamp(nbstate.since);
                sp_float g = (ga < gb) ? ga : gb;

                samps[j].real = nbstate.delay[r].real * g;
                samps[j].imag = nbstate.delay[r].imag * g;

                // 4. pulse at the output passed: find the next one still in the ring
                if (nbstate.ahead == 0) {
                        nbstate.ahead = -1;
                        int p = r;
                        for (int d = 1; d <= nbstate.delaylen; d++) {
                                if (++p >= nbstate.ringlen) p = 0;
                                if (nbstate.flag[p]) {
                                        nbstate.ahead = d;
                                        break;
                                }
                        }
                }

                nbstate.widx = r; // r is the next write position (oldest sample is done)

                // 5. self-protection and log
                nb_log_count++;
                if (++nb_step_count >= stepLen) {
                        nbRateStep(fs);
                        k = nbstate.kbase * nbstate.kboost;
                }
        }
}
