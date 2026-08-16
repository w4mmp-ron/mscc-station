/*
 * Remote operator phones: post-DSP AF → UDP MSA1 packets (Windows player).
 * Digi is NOT on this path.
 */
#ifndef REMOTE_PHONES_H
#define REMOTE_PHONES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Load ~/.local/mscc/remote-phones.ini, open socket, start sender thread. */
void remote_phones_init(void);

/* Stop thread / close socket (process exit). */
void remote_phones_shutdown(void);

/*
 * Real-time safe: push stereo float frames @ I/Q rate (typically 96 kHz).
 * Uses left channel (mono AF after DSP). Decimates to 48 kHz for the wire.
 * Call after AGC/NR/AN, before local volume scale.
 */
void remote_phones_feed(const float *stereo_interleaved, unsigned frames);

/* 1 if enabled and running. */
int remote_phones_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* REMOTE_PHONES_H */
