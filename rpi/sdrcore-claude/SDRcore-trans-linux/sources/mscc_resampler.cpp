/*
 * C wrapper around Oboe freestanding MultiChannelResampler for sdrcore-trans/recv.
 */
#include "mscc_resampler.h"

#include "resampler/MultiChannelResampler.h"

#include <cstdlib>

using mscc::resampler::MultiChannelResampler;

struct MsccResampler {
    MultiChannelResampler *mcr;
};

extern "C" MsccResampler *mscc_resampler_create(int in_rate, int out_rate)
{
    if (in_rate <= 0 || out_rate <= 0)
        return nullptr;
    if (in_rate == out_rate)
        return nullptr;

    MultiChannelResampler *mcr = MultiChannelResampler::make(
        2, in_rate, out_rate, MultiChannelResampler::Quality::Medium);
    if (mcr == nullptr)
        return nullptr;

    MsccResampler *r = (MsccResampler *)std::malloc(sizeof(MsccResampler));
    if (r == nullptr) {
        delete mcr;
        return nullptr;
    }
    r->mcr = mcr;
    return r;
}

extern "C" void mscc_resampler_destroy(MsccResampler *r)
{
    if (r == nullptr)
        return;
    delete r->mcr;
    r->mcr = nullptr;
    std::free(r);
}

extern "C" void mscc_resampler_fill_out(MsccResampler *r,
                                        float *out,
                                        int out_frames,
                                        void (*get_in_frame)(float frame[2], void *userdata),
                                        void *userdata)
{
    if (r == nullptr || r->mcr == nullptr || out == nullptr || out_frames <= 0)
        return;
    if (get_in_frame == nullptr) {
        for (int i = 0; i < out_frames * 2; i++)
            out[i] = 0.0f;
        return;
    }

    MultiChannelResampler *mcr = r->mcr;
    float *dst = out;
    int left = out_frames;

    while (left > 0) {
        if (mcr->isWriteNeeded()) {
            float frame[2];
            get_in_frame(frame, userdata);
            mcr->writeNextFrame(frame);
        } else {
            mcr->readNextFrame(dst);
            dst += 2;
            left--;
        }
    }
}

extern "C" void mscc_resampler_push_in(MsccResampler *r,
                                       const float *in,
                                       int in_frames,
                                       int in_ch,
                                       void (*put_out_frame)(const float frame[2], void *userdata),
                                       void *userdata)
{
    if (r == nullptr || r->mcr == nullptr || in == nullptr || in_frames <= 0)
        return;
    if (put_out_frame == nullptr)
        return;

    MultiChannelResampler *mcr = r->mcr;
    const float *src = in;
    int left = in_frames;
    const int step = (in_ch >= 2) ? 2 : 1;

    while (left > 0) {
        if (mcr->isWriteNeeded()) {
            float frame[2];
            if (in_ch >= 2) {
                frame[0] = src[0];
                frame[1] = src[1];
            } else {
                frame[0] = src[0];
                frame[1] = src[0];
            }
            mcr->writeNextFrame(frame);
            src += step;
            left--;
        } else {
            float outf[2];
            mcr->readNextFrame(outf);
            put_out_frame(outf, userdata);
        }
    }
    /* Drain any remaining output for this input batch */
    while (!mcr->isWriteNeeded()) {
        float outf[2];
        mcr->readNextFrame(outf);
        put_out_frame(outf, userdata);
    }
}
