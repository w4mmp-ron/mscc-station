/*
 * Remote operator mic: Windows MsccRemotePhones → Pi (MSA1 UDP listen).
 *
 * Client sends CMD_SET_AUDIO_DEVICE=2 (REMOTE_AUDIO) when Phones + REMOTE AUDIO.
 * Digital (0) never uses this path. INI supplies PORT only (default 9101).
 */
#ifndef REMOTE_MIC_H
#define REMOTE_MIC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Load remote-mic.ini PORT; bind UDP and start receiver (always, if bind ok). */
void remote_mic_init(void);

void remote_mic_shutdown(void);

/* 1 if UDP listener is running (packets may fill the ring). */
int remote_mic_ready(void);

/*
 * Fill stereo float @ I/Q rate (96 kHz) from 48 kHz mono MSA ring.
 * Underrun → silence. Call when G_audio_mode == REMOTE_AUDIO.
 */
void remote_mic_fill_stereo_96k(float *stereo_interleaved, unsigned frames);

#ifdef __cplusplus
}
#endif

#endif /* REMOTE_MIC_H */
