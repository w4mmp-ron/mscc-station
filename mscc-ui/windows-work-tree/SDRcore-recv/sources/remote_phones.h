/*
 * Remote operator phones: post-DSP AF → UDP MSA1 packets (Windows player).
 * Digi is NOT on this path.
 */
#ifndef REMOTE_PHONES_H
#define REMOTE_PHONES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Load %LOCALAPPDATA%\MSCC-NET9\remote-phones.ini, open socket, start sender. */
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

/* 1 = keep radio speaker (Monitor at radio). Default 0 from INI MONITOR=. */
int remote_phones_monitor(void);

/* 1 = remote RX on and monitor off — caller should zero local play. */
int remote_phones_mute_local(void);

/* Live opcodes (no process restart). ip_nbo = IPv4 in network byte order. */
void remote_phones_set_host(unsigned int ip_nbo);
/* packed: bits 0-15 port (0=keep), bit16 enable, bit17 monitor-at-radio. */
void remote_phones_set_ctrl(unsigned int packed);

#ifdef __cplusplus
}
#endif

#endif /* REMOTE_PHONES_H */
