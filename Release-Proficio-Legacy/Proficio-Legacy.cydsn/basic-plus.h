// Copyright 2013 David Turnbull AE9RB
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 10/01/2016 Added support for Proficio
// 02/18/2017 Added support for native CW
// Copyright © 2015-2024 Omnia SDR

// Variable naming conventions:     E_<variable> -> Externally defined global
//                                  ff_<variable> -> Externally defined global stored in flash memory
//                                  ee_<variable> -> Externally defined global to be stored in EEPROM memory
//                                  l_<variable> -> locally defined variable
//                                  All UPPERCASE -> Define


#ifndef PEABERRY_H
#define PEABERRY_H

#include <device.h>
    
#define TRUE 1
#define FALSE 0   
#define MAX_COMMAND_QUEUE 50

// {Model}.{[M]M}{Release Number}  (the bytes are reversed)  
// Model 0-OSB, 1-Proficio, 2-Geminus,3-MKII-PTT,4-MKII-ATU,5-Proficio-PTT,6-Proficio-ATU
// Month is the number of months from the starting point of 09/24. 01/25 is 13, 02/25 is 14, etc.
// Release number is in the range of 0 to 9



#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 231
#define FIRMWARE_VERSION ((((FIRMWARE_VERSION_MINOR) << 8) & 0xff00) | ((FIRMWARE_VERSION_MAJOR) & 0x00ff))
#define EEPROM_PCB_VERSION_LOCATION 11
        
typedef uint16 uint16_t;
typedef uint8 uint8_t;
typedef int8 int8_t;    
    
// Status register bits
#define STATUS_KEY_0  0x01
#define STATUS_KEY_1  0x02
#define STATUS_BOOT   0x04
#define STATUS_BEAT   0x08
#define STATUS_ATU_0  0x10

// Control register bits
#define CONTROL_LED      0x01
#define CONTROL_RX       0x02
#define CONTROL_DIN      0x04
#define CONTROL_AMP      0x08
#define CONTROL_DOUT     0x10
#define CONTROL_ATU_0    0x20
#define CONTROL_ATU_0_OE 0x40
#define CONTROL_ATU_1    0x80
    
    
    // Band Control Register Bits
#define CONTROL_BAND_160   0x05 //Y5 Not Connected BS0=1 BS1=1 BS2=1  
#define CONTROL_BAND_80    0x04 //Y4    BS0=0 BS1=0 BS2=1  
#define CONTROL_BAND_40_60 0x03 //Y3    BS0=1 BS1=1 BS2=0  
#define CONTROL_BAND_20_30 0x02 //Y2    BS0=0 BS1=1 BS2=0  
#define CONTROL_BAND_15_17 0x01 //Y1    BS0=1 BS1=0 BS2=0  
#define CONTROL_BAND_10_12 0x00 //Y0    BS0=0 BS1=0 BS2=0    
    
// Band Indicators
#define BAND_10M 9
#define BAND_12M 8   
#define BAND_15M 7
#define BAND_17M 6
#define BAND_20M 5
#define BAND_30M 4
#define BAND_40M 3
#define BAND_60M 2    
#define BAND_80M 1 
#define BAND_160M 0
    
#define E_PPM_NEEDS_SET_STEP_1 2
#define E_PPM_NEEDS_SET_STEP_2 3

// Max buffer size for 1ms
#define I2S_BUF_SIZE (96u * 2 * 2)

// Unvisible stuff from Cypress that they expect us to use and don't export
uint8 USBFS_InitControlRead(void);
uint8 USBFS_InitControlWrite(void);
extern volatile T_USBFS_TD USBFS_currentTD;
extern volatile T_USBFS_EP_CTL_BLOCK USBFS_EP[];
extern uint8 USBFS_initVar;
extern uint8 USBFS_DmaTd[USBFS_MAX_EP];
extern uint8 USBFS_DmaChan[USBFS_MAX_EP];

// main.c
uint32 swap32(uint32) CYREENTRANT;
int32 swap32_int(int32 original) CYREENTRANT ;
uint16 swap16(uint16 original) CYREENTRANT ;
void ERROR(char* msg);

extern uint8 E_host_mode;

//extern uint8 E_cw_message_toggle;
extern uint8 E_key_0;
extern uint8 E_key_1;
extern uint8 E_key_down;
extern uint8 E_cw_hold;

extern uint8_t E_keyer_mode;
extern uint8_t E_wpm;
extern uint8_t E_spacing;
extern uint8_t E_mem_text_wpm; /* SET_MEM_TEXT_WPM 0x76: 0=off, else memory-play text WPM */
extern uint8_t E_weight;
extern uint8_t E_side_tone;
extern uint8_t E_paddle;
extern uint8_t E_keyer_installed;
/*
 * Keyer CQ memory USB packet (control write, 2 bytes — same style as other CW ops):
 *   E_keyer_mem_pkt[0] = param (0=play, 1=begin, 2=end, 0x20-0x7E=ASCII)
 *   E_keyer_mem_pkt[1] = sequence (host increments each transfer; never 0)
 * Configure_CW: stable-read on seq change → ring queue → I2C one pair at a time.
 */
#define KEYER_MEM_Q_SIZE 80
extern volatile uint8_t E_keyer_mem_pkt[2];
extern volatile uint8_t E_keyer_mem_q[KEYER_MEM_Q_SIZE];
extern volatile uint8_t E_keyer_mem_q_head;
extern volatile uint8_t E_keyer_mem_q_tail;
extern volatile uint8_t E_keyer_mem_q_count;
//extern uint8 E_cw_defaults;

extern uint32 E_current_rit_freq;
extern uint32 E_new_rit_freq;
extern uint8 E_transverter;
extern uint8 E_pcb_version;
extern uint8 E_AMP_Bypass;
extern uint8 E_AMP_Value;
extern uint8_t E_TX_Hold;
extern uint8_t E_Amplifier;
extern uint8_t E_QSK;
extern volatile uint8_t E_PTT;
extern uint8_t E_band;
extern uint8 E_PPM_needs_updated;
extern uint8 E_PPM_needs_set;
extern volatile uint8 E_smooth;

// morse.c
void Morse_Main(char* msg);

// audio.c
extern uint8 Audio_IQ_Channels;
void Audio_Start(void);
void Audio_Main(void);
extern uint8 E_Audio_running;


// sync.c
void Sync_Start(void);
void Sync_Main(void);
extern uint8 E_sync_running;

// band.c
uint8 Band_Main(void);
extern uint8 E_meter_band;

// si570.c
#define SI570_STARTUP_FREQ 56.32
extern volatile uint32 Si570_Xtal, Si570_LO;
extern volatile uint32 E_tune_freq;
extern volatile uint32 E_CW_LO_freq;
extern volatile uint8_t E_cw_pitch;
extern uint32 Current_LO;
extern uint8 Si570_Buf[], Si570_Factory[], Si570_OLD[];
uint8 Si570_Init(void);
void Si570_Main(void);
void Si570_Fake_Reset(void);
#define STARTUP_LO 0x713D0A07 // 56.32 MHz in byte reversed 11.21 bits (14.080)
#define MAX_LO 160.0 // maximum for CMOS Si570
#define MIN_LO 4.0 
#define SI570_SMOOTH_PPM 3500
#define SI570_ADDR 0x55
#define SI570_DCO_MIN 4850.0
#define SI570_DCO_MAX 5670.0
#define SI570_DCO_CENTER ((SI570_DCO_MIN + SI570_DCO_MAX) / 2)

// pcm3060.c
uint8 PCM3060_Init(void);
void PCM3060_Start(void);
uint8 PCM3060_Stop(void);
uint8* PCM3060_TxBuf(void);
uint8* PCM3060_RxBuf(void);
void PCM3060_Adj_Output_Volume(uint8 level);
extern void PCM3060_SetTxBufAddress(uint16 source);
extern void PCM3060_SetTxBufAddressDefault();
//void PCM3060_Adj_Input_Volume(uint8 level);

// settings.c
void Settings_Init(void);
void Settings_Main(void);

// tx.c
extern uint8 TX_Request;
extern uint8 TX_Inhibit;
void TX_Main(void);

// t1.c
extern uint8 T1_Band_Number;
extern uint8 T1_Tune_Request;
void T1_Main(void);

//convert.c
uint32 convert_from_host(uint32 freq);

//si5351a.c
extern uint32 E_l_freq;
extern volatile int8 E_ppm;

//si5351a-cw.c
uint8 si5351aSetFrequency_CW(uint8_t transmit);

//si5351.c
int si5351_init(uint8_t xtal_load_c, uint32 ref_osc_freq);
uint8_t si5351_write_init(uint8_t addr, uint8_t command_data);
uint8 si5351_write_bulk(uint8 addr, uint8 bytes, uint8 *command_data);
uint8_t si5351_read(uint8_t addr, uint8_t *command_data);
void si5351_main(void);
extern volatile uint32 E_current_LO_freq;
extern volatile uint32 E_xtal_freq;
extern volatile int8 E_calibration_int;
extern volatile int8 E_calibration_dec;
extern volatile int8 ee_ppm_int;
extern volatile int8 ee_ppm_dec;
extern volatile int16 PPM;
//extern volatile uint8 E_SI5351_Reset;
extern int8_t E_si5351_queue_status;
extern int16 E_cw_pitch_freq;

//usbvend.c
extern uint8 E_dll_version;

//cw.c
void Manage_Paddles_Port(void);
CY_ISR_PROTO(CW_interrupt);
CY_ISR_PROTO(KEY_interrupt);
CY_ISR_PROTO(QSK_interrupt);
uint8 Configure_CW();
extern uint8 TX_State;
extern uint8 TX_Phase;
extern uint32 CW_LO_Freq;

#define KEY_DOWN 1
#define KEY_UP 0
#define TX_KEY_ACTIVE    0x10
#define TX_PHASE_RECEIVING           0
#define TX_PHASE_TX_ENABLE           1
#define TX_PHASE_AMP_ENABLE          2
// Transmitting PC audio data.
#define TX_PHASE_TXPC                10
// End the transmit of PC audio data.
#define TX_PHASE_TXPC_EXIT           11
// Transmitting an internal shaped IQ tone, keyed by the external key down signal.
#define TX_PHASE_IQTONE_IDLE         20
#define TX_PHASE_IQTONE_RAMP_UP      21
#define TX_PHASE_IQTONE_STEADY       22
#define TX_PHASE_IQTONE_RAMP_DOWN    23
#define TX_PHASE_IQTONE_HANG         24
#define TX_PHASE_IQTONE_END          25
#define TX_PHASE_IQTONE_UNMUTE       26
#define TX_PHASE_IQTONE_EXIT         27

//mia.c
uint8 Mia_Send(uint8 mia_data,uint8 mia_data_type);
uint8 Mia_Init();
uint8 MIA_Refresh();
extern uint8_t E_amp_power;

//temperature.c
void Check_temperature();
extern int32 E_temp;
extern uint8 E_temperature_processing;
extern int32 E_delta_drift_int;
extern volatile int32 E_transceiver_temp;

//power.c
extern int32 E_Potentia_Power;
extern uint8_t E_potentia_power_sensor_attached;
uint8_t Potentia_Power_Init(void);
uint8_t Potentia_Read_Power();

//bias.c
extern uint8_t E_potentia_Bias_Sensor_Attached;
extern uint8_t E_Potentia_Read_Bias;
extern uint16_t E_Potentia_Write_Bias;
uint8_t Potentia_Bias_Init(void);
uint8_t Potentia_Write_Bias(void);
uint8_t Potentia_Read_Bias(void);
#define INCREMENT_WIPER_0 0x04
#define INCREMENT_WIPER_1 0x08
#define DECREMENT_WIPER_0 0x14
#define DECREMENT_WIPER_1 0x18





#endif //PEABERRY_H
