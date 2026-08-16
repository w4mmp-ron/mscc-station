#include "radio_state.h"
#include "usbvend.h"

uint8_t  E_host_mode = 'U';
uint8_t  E_TX_Hold = CW_DEFAULT_HOLD_TIME;
uint8_t  E_key_0 = TRUE;
uint8_t  E_key_1 = TRUE;
uint8_t  E_key_down = FALSE;
uint8_t  E_cw_hold = FALSE;
uint8_t  E_keyer_mode = 1;
uint8_t  E_wpm = 18;
uint8_t  E_spacing = 0;
uint8_t  E_mem_text_wpm = 0;
uint8_t  E_weight = 50;
uint8_t  E_side_tone = 0;
uint8_t  E_paddle = 0;
uint8_t  E_keyer_installed = TRUE;
uint8_t  E_QSK = FALSE;
uint8_t  E_Amplifier = FALSE;
uint8_t  E_transverter = 0;
uint8_t  E_pcb_version = 2;
uint8_t  E_band = BAND_20M;
uint8_t  E_dll_version = SI5351_DLL;
volatile uint8_t E_PTT = FALSE;

volatile uint8_t E_keyer_mem_pkt[2] = {0, 0};
volatile uint8_t E_keyer_mem_q[KEYER_MEM_Q_SIZE];
volatile uint8_t E_keyer_mem_q_head = 0;
volatile uint8_t E_keyer_mem_q_tail = 0;
volatile uint8_t E_keyer_mem_q_count = 0;

uint32_t E_current_LO_freq = 14000000UL;
uint32_t E_current_rit_freq = 0;
volatile uint32_t Si570_LO = 14000000UL;
uint8_t  TX_Request = 0;
uint8_t  TX_Inhibit = 0;

int16_t  E_cw_pitch_freq = 600;
volatile uint8_t E_cw_pitch = 0;
uint32_t CW_LO_Freq = 14000000UL;

int8_t   ee_ppm_int = 0;
int8_t   ee_ppm_dec = 0;
int32_t  E_calibration_int = 0;
int32_t  E_calibration_dec = 0;
int32_t  E_ppm = 0;
uint8_t  E_PPM_needs_updated = 0;
uint8_t  E_PPM_needs_set = 0;
volatile uint8_t E_smooth = TRUE;

int32_t  E_transceiver_temp = 0;
uint8_t  E_si5351_status = 0;

void radio_state_init(void)
{
    E_host_mode = 'U';
    E_TX_Hold = CW_DEFAULT_HOLD_TIME;
    E_current_LO_freq = 14000000UL;
    Si570_LO = 14000000UL;
    CW_LO_Freq = 14000000UL;
    E_keyer_mem_q_head = 0;
    E_keyer_mem_q_tail = 0;
    E_keyer_mem_q_count = 0;
    E_keyer_mem_pkt[0] = 0;
    E_keyer_mem_pkt[1] = 0;
}
