/*
 * Minimal PortAudio stub when libportaudio is not installed.
 * Build with: make PA_STUB=1
 * Enough for UDP bring-up; no real audio I/O.
 */
#if defined(PA_STUB)

#include <stddef.h>
#include <string.h>
#include "portaudio_stub.h"

static PaDeviceInfo g_dev = {
    .structVersion = 2,
    .name = "Null (PA_STUB)",
    .hostApi = 0,
    .maxInputChannels = 2,
    .maxOutputChannels = 2,
    .defaultLowInputLatency = 0.01,
    .defaultLowOutputLatency = 0.01,
    .defaultHighInputLatency = 0.05,
    .defaultHighOutputLatency = 0.05,
    .defaultSampleRate = 48000.0
};

static PaHostApiInfo g_api = {
    .structVersion = 1,
    .type = paInDevelopment,
    .name = "Null",
    .deviceCount = 1,
    .defaultInputDevice = 0,
    .defaultOutputDevice = 0
};

static PaVersionInfo g_ver = {
    .versionMajor = 19,
    .versionMinor = 0,
    .versionSubMinor = 0,
    .versionControlRevision = "stub",
    .versionText = "PortAudio stub (no audio)"
};

PaError Pa_Initialize(void) { return paNoError; }
PaError Pa_Terminate(void) { return paNoError; }
int Pa_GetVersion(void) { return 0x00130000; }
const PaVersionInfo *Pa_GetVersionInfo(void) { return &g_ver; }
const char *Pa_GetErrorText(PaError e) {
    (void)e;
    return "PA_STUB";
}
PaHostApiIndex Pa_GetHostApiCount(void) { return 1; }
const PaHostApiInfo *Pa_GetHostApiInfo(PaHostApiIndex i) {
    (void)i;
    return &g_api;
}
PaDeviceIndex Pa_GetDeviceCount(void) { return 1; }
const PaDeviceInfo *Pa_GetDeviceInfo(PaDeviceIndex i) {
    (void)i;
    return &g_dev;
}
PaDeviceIndex Pa_GetDefaultInputDevice(void) { return 0; }
PaDeviceIndex Pa_GetDefaultOutputDevice(void) { return 0; }

PaError Pa_OpenStream(PaStream **stream,
    const PaStreamParameters *inputParameters,
    const PaStreamParameters *outputParameters,
    double sampleRate,
    unsigned long framesPerBuffer,
    PaStreamFlags streamFlags,
    PaStreamCallback *streamCallback,
    void *userData)
{
    (void)inputParameters; (void)outputParameters; (void)sampleRate;
    (void)framesPerBuffer; (void)streamFlags; (void)streamCallback; (void)userData;
    if (stream) *stream = (PaStream *)(intptr_t)1;
    return paNoError;
}

PaError Pa_CloseStream(PaStream *stream) { (void)stream; return paNoError; }
PaError Pa_StartStream(PaStream *stream) { (void)stream; return paNoError; }
PaError Pa_StopStream(PaStream *stream) { (void)stream; return paNoError; }
PaError Pa_AbortStream(PaStream *stream) { (void)stream; return paNoError; }
PaError Pa_IsStreamActive(PaStream *stream) { (void)stream; return 0; }
PaError Pa_IsStreamStopped(PaStream *stream) { (void)stream; return 1; }

#endif /* PA_STUB */
