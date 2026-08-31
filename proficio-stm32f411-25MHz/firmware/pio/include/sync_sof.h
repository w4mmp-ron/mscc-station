/**
 * USB SOF ↔ I2S clock lock (PSoC SyncSOF / FracN equivalent).
 *
 * PSoC adjusted FracN from SyncSOF frame position. Here we measure samples
 * per USB 1 ms frame and trim PLLI2S so the codec clock tracks the host.
 */
#ifndef SYNC_SOF_H
#define SYNC_SOF_H

#include <stdint.h>

void    sync_sof_init(void);
void    sync_sof_start(void);
void    sync_sof_stop(void);

/** Call from USB SOF interrupt (1 kHz when configured). */
void    sync_sof_on_sof(void);

/** Call when one I2S buffer frame (I2S_BUF_SIZE) completes. */
void    sync_sof_on_i2s_frame(void);

uint8_t sync_sof_running(void);
int16_t sync_sof_last_error(void); /* samples vs 96 frames/ms target */

#endif /* SYNC_SOF_H */
