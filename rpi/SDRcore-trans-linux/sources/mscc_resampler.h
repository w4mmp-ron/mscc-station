/*
 * Thin C API over freestanding Oboe MultiChannelResampler (sources/resampler/).
 * Trans: mic may be device rate; I/Q DSP stays at 96 kHz (upsample into ring).
 * Recv: play may be device rate; AF ring stays at 96 kHz (downsample on play).
 */
#ifndef MSCC_RESAMPLER_H
#define MSCC_RESAMPLER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MsccResampler MsccResampler;

/*
 * Stereo resampler: in_rate -> out_rate.
 * Returns NULL if rates invalid, equal (use bypass), or allocation failed.
 */
MsccResampler *mscc_resampler_create(int in_rate, int out_rate);

void mscc_resampler_destroy(MsccResampler *r);

/*
 * Fixed OUTPUT frames (recv play path): pull input via get_in_frame.
 */
void mscc_resampler_fill_out(MsccResampler *r,
                             float *out,
                             int out_frames,
                             void (*get_in_frame)(float frame[2], void *userdata),
                             void *userdata);

/*
 * Fixed INPUT frames (trans mic path): push in_frames of interleaved stereo
 * (or mono duplicated if in_ch==1); each produced out frame goes to put_out_frame.
 */
void mscc_resampler_push_in(MsccResampler *r,
                            const float *in,
                            int in_frames,
                            int in_ch,
                            void (*put_out_frame)(const float frame[2], void *userdata),
                            void *userdata);

#ifdef __cplusplus
}
#endif

#endif /* MSCC_RESAMPLER_H */
