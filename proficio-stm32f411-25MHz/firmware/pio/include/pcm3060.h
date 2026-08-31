/**
 * PCM3060 codec — port of PSoC pcm3060.c control path.
 * Data plane is I2S (see i2s_audio.h); this module is I2C register control.
 */
#ifndef PCM3060_H
#define PCM3060_H

#include <stdint.h>

/* PSoC used 0x46 as I2C write address (8-bit) → 7-bit 0x23 */
#define PCM3060_I2C_ADDR7   0x23u

#define USB_AUDIO_BUFS      3u
/* 96 stereo frames × 2 ch × 2 bytes = 384 = 1 ms @ 96 kHz (matches USB max packet) */
#define I2S_BUF_SIZE        (96u * 2u * 2u)
#define AUDIO_SAMPLE_RATE   96000u

uint8_t  PCM3060_SetRegister(uint8_t reg, uint8_t val);
uint8_t  PCM3060_Init(void);
void     PCM3060_Start(void);
uint8_t  PCM3060_Stop(void);
void     PCM3060_Adj_Output_Volume(uint8_t level);

uint8_t *PCM3060_TxBuf(void);  /* host → radio (USB OUT / I2S TX to codec DIN) */
uint8_t *PCM3060_RxBuf(void);  /* radio → host (I2S RX from codec DOUT / USB IN) */

void     PCM3060_SetTxBufAddress(uint8_t *src); /* CW tone inject hook */
void     PCM3060_SetTxBufAddressDefault(void);

/** Triple buffers (USB_AUDIO_BUFS × I2S_BUF_SIZE). */
extern uint8_t RxI2S[USB_AUDIO_BUFS][I2S_BUF_SIZE];
extern uint8_t TxI2S[USB_AUDIO_BUFS][I2S_BUF_SIZE];

/** Index selected by SOF / audio tick (0..USB_AUDIO_BUFS-1). */
uint8_t  audio_buf_index(void);
void     audio_buf_advance(void);

#endif /* PCM3060_H */
