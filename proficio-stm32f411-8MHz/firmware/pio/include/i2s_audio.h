/**
 * I2S2 + I2S2ext full duplex to PCM3060 (Black Pill pin map).
 */
#ifndef I2S_AUDIO_H
#define I2S_AUDIO_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

extern I2S_HandleTypeDef hi2s2;
extern I2S_HandleTypeDef hi2s2ext;

void    i2s_audio_init(void);
void    i2s_audio_start(void);
void    i2s_audio_stop(void);
uint8_t i2s_audio_running(void);

/** Called from DMA complete to rotate buffers / notify USB. */
void    i2s_audio_on_tx_half(void);
void    i2s_audio_on_tx_cplt(void);
void    i2s_audio_on_rx_half(void);
void    i2s_audio_on_rx_cplt(void);

#endif /* I2S_AUDIO_H */
