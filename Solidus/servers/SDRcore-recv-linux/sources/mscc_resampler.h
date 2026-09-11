/*
 * Thin C API over freestanding Oboe MultiChannelResampler (sources/resampler/).
 * I/Q DSP stays at 96 kHz; operator play may open at a different device rate.
 */
#ifndef MSCC_RESAMPLER_H
#define MSCC_RESAMPLER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MsccResampler MsccResampler;

/*
 * Stereo resampler: in_rate (e.g. 96000) -> out_rate (device rate).
 * Returns NULL if rates invalid or allocation failed.
 * Caller should not create when in_rate == out_rate (use ring bypass).
 */
MsccResampler *mscc_resampler_create(int in_rate, int out_rate);

void mscc_resampler_destroy(MsccResampler *r);

/*
 * Produce out_frames of interleaved stereo float into out.
 * When the resampler needs input, calls get_in_frame(frame[2], userdata)
 * once per input frame (L,R). Use silence if the AF ring is empty.
 */
void mscc_resampler_fill_out(MsccResampler *r,
                             float *out,
                             int out_frames,
                             void (*get_in_frame)(float frame[2], void *userdata),
                             void *userdata);

#ifdef __cplusplus
}
#endif

#endif /* MSCC_RESAMPLER_H */
