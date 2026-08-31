/**
 * Shared radio globals (E_* from PSoC main / basic-plus).
 */
#ifndef RADIO_STATE_H
#define RADIO_STATE_H

#include <stdint.h>
#include "proficio_config.h"

/* Band indicators */
#define BAND_160M 0
#define BAND_80M  1
#define BAND_60M  2
#define BAND_40M  3
#define BAND_30M  4
#define BAND_20M  5
#define BAND_17M  6
#define BAND_15M  7
#define BAND_12M  8
#define BAND_10M  9

extern uint8_t  E_host_mode;       /* 'U' USB audio / 'C' CW */
extern uint8_t  E_TX_Hold;
extern uint8_t  E_key_0;
extern uint8_t  E_key_1;
extern uint8_t  E_key_down;
extern uint8_t  E_cw_hold;
extern uint8_t  E_keyer_mode;
extern uint8_t  E_wpm;
extern uint8_t  E_spacing;
extern uint8_t  E_mem_text_wpm;
extern uint8_t  E_weight;
extern uint8_t  E_side_tone;
extern uint8_t  E_paddle;
extern uint8_t  E_keyer_installed;
extern uint8_t  E_QSK;
extern uint8_t  E_Amplifier;
extern uint8_t  E_transverter;
extern uint8_t  E_pcb_version;
extern uint8_t  E_band;
extern uint8_t  E_dll_version;
extern volatile uint8_t E_PTT;

extern volatile uint8_t E_keyer_mem_pkt[2];
extern volatile uint8_t E_keyer_mem_q[KEYER_MEM_Q_SIZE];
extern volatile uint8_t E_keyer_mem_q_head;
extern volatile uint8_t E_keyer_mem_q_tail;
extern volatile uint8_t E_keyer_mem_q_count;

extern uint32_t E_current_LO_freq;
extern uint32_t E_current_rit_freq;
extern volatile uint32_t Si570_LO;   /* host SET_FREQ / GET_FREQ (Hz, host format) */
extern uint8_t  TX_Request;
extern uint8_t  TX_Inhibit;

extern int16_t  E_cw_pitch_freq;
extern volatile uint8_t E_cw_pitch;
extern uint32_t CW_LO_Freq;

extern int8_t   ee_ppm_int;
extern int8_t   ee_ppm_dec;
extern int32_t  E_calibration_int;
extern int32_t  E_calibration_dec;
extern int32_t  E_ppm;
extern uint8_t  E_PPM_needs_updated;
extern uint8_t  E_PPM_needs_set;
extern volatile uint8_t E_smooth;

extern int32_t  E_transceiver_temp;
extern uint8_t  E_si5351_status;

void radio_state_init(void);

#endif /* RADIO_STATE_H */
