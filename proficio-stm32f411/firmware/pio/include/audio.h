/**
 * USB ↔ I2S audio path — port of PSoC audio.c
 */
#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

#define RX_ENDPOINT   2u  /* USB IN  0x82 — device → host (RX IQ) */
#define TX_ENDPOINT   3u  /* USB OUT 0x03 — host → device (TX IQ) */
#define RX_INTERFACE  2u
#define TX_INTERFACE  3u

extern uint8_t Audio_IQ_Channels;
extern uint8_t E_Audio_running;

void Audio_Start(void);
void Audio_Main(void);
void Audio_Stop(void);

/** Feed from USB stack when OUT data arrives / IN needs fill. */
void audio_usb_out_packet(const uint8_t *data, uint16_t len);
uint16_t audio_usb_in_packet(uint8_t *data, uint16_t max_len);

#endif /* AUDIO_H */
