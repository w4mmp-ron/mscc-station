/*
 * Remote operator mic: Windows MsccRemotePhones → Pi (MSA1 UDP listen).
 *
 * When operator selects Phones (P) and ~/.local/mscc/remote-mic.ini has
 * ENABLED=1, AF comes from UDP (not the local operator mic).
 * Digital (D) is never on this path.
 */
#ifndef REMOTE_MIC_H
#define REMOTE_MIC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Load remote-mic.ini; if ENABLED, bind UDP and start receiver. */
void remote_mic_init(void);

void remote_mic_shutdown(void);

/* 1 if INI says remote (ENABLED=1) and listener is running. */
int remote_mic_enabled(void);

/*
 * Fill stereo float @ I/Q rate (96 kHz) from 48 kHz mono MSA ring.
 * Underrun → silence. Call only when remote_mic_enabled() and Phones mode.
 */
void remote_mic_fill_stereo_96k(float *stereo_interleaved, unsigned frames);

#ifdef __cplusplus
}
#endif

#endif /* REMOTE_MIC_H */
