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
// Copyright © 2015-2017 Omnia SDR

// Variable naming conventions:     E_<variable> -> Externally defined global
//                                  ff_<variable> -> Externally defined global stored in flash memory
//                                  ee_<variable> -> Externally defined global to be stored in EEPROM memory
//                                  l_<variable> -> locally defined variable
//                                  All UPPERCASE -> Define


#include <basic-plus.h>
#include "si5351a.h"

#define XTAL_MIN (114.285 - 2)
#define XTAL_MAX (114.285 + 2)

uint8 buffer[CYDEV_EEPROM_ROW_SIZE];


#define XTAL_DATA(mem) (*(reg32*)(mem+0))
#define REVERSE_DATA(mem) (*(reg8*)(mem+4))

#define PPM_INT 0
#define PPM_DEC 1
#define PPM_RECEIVED 0x03
#define AUDIO_CHANNELS 2
#define CALIBRATION_TEMPERATURE 3
#define DELAY_LIMIT 200
#define PPM_INT_SIGN 3
#define PPM_DEC_SIGN 4



void Settings_Init(){
    reg8  *RegPointer;
    int8_t int_sign = 0;
    int8_t dec_sign = 0;
    
    EEPROM_Start();
    CyDelay(100);
    RegPointer = (reg8 *) (CYDEV_EE_BASE + 0u);
    ee_ppm_int = RegPointer[PPM_INT];
    ee_ppm_dec = RegPointer[PPM_DEC];
    int_sign = RegPointer[PPM_INT_SIGN];
    dec_sign = RegPointer[PPM_DEC_SIGN];
    
    if(int_sign == 1){
        ee_ppm_int = ee_ppm_int * -1;
    }
    if(dec_sign == 1){
        ee_ppm_dec = ee_ppm_dec * -1;
    }
  
    Audio_IQ_Channels = RegPointer[AUDIO_CHANNELS];
    Audio_IQ_Channels = 0;
}

int Set_PPM(void ) {
    volatile cystatus status = 0;
    reg8 *RegPointer;
    uint8_t i = 0;
    uint8_t freq_status =0;
          
    RegPointer = (reg8 *) (CYDEV_EE_BASE + 0u);
    status = EEPROM_QueryWrite();
    if(status != CYRET_STARTED){
      for (i=5; i < CYDEV_EEPROM_ROW_SIZE; i++) {
            buffer[i] = 0;
        }
        if(ee_ppm_int < 0){
            buffer[PPM_INT] = (ee_ppm_int * -1);
            buffer[PPM_INT_SIGN] = 1;
        }else{
            buffer[PPM_INT] = ee_ppm_int;
            buffer[PPM_INT_SIGN] = 0;
        }
        if(ee_ppm_dec < 0){
            buffer[PPM_DEC] = (ee_ppm_dec * -1);
            buffer[PPM_DEC_SIGN] = 1;
        }else{
            buffer[PPM_DEC] = ee_ppm_dec;
            buffer[PPM_DEC_SIGN] = 0;
        }
        buffer[AUDIO_CHANNELS] = Audio_IQ_Channels;
        EEPROM_StartWrite(buffer, 0);
        E_PPM_needs_updated = 0;
        E_PPM_needs_set = E_PPM_NEEDS_SET_STEP_1;
    }
    return status;
  }



void Settings_Main(){
    //volatile uint8 l_audio_channels;
    reg8 *RegPointer;
    cystatus  error = 0;
    //uint8 critical_section;
    volatile int8 l_calibration_int = 0;
    volatile int8 l_calibration_dec = 0;
    
    
    RegPointer = (reg8 *) (CYDEV_EE_BASE + 0u);
    l_calibration_int = ee_ppm_int;
    l_calibration_dec = ee_ppm_dec;
    if(E_PPM_needs_updated == PPM_RECEIVED){
        l_calibration_int = (E_calibration_int);
        l_calibration_dec = (E_calibration_dec);
        ee_ppm_int = (l_calibration_int);   
        ee_ppm_dec = (l_calibration_dec);
        Set_PPM();
    }
        
    /*l_audio_channels = RegPointer[AUDIO_CHANNELS];
    if(l_audio_channels != Audio_IQ_Channels){
        critical_section = CyEnterCriticalSection();
        error = EEPROM_ByteWritePos(Audio_IQ_Channels,0u,AUDIO_CHANNELS);
        CyExitCriticalSection(critical_section);
    }*/
}    

/*void Settings_Init(void) {
    float crystal_freq;
    EEPROM_Start();
    
    crystal_freq = ((float)swap32(*(reg32*)CYDEV_EE_BASE) / 0x01000000);
    
    if (
        crystal_freq < XTAL_MIN || crystal_freq > XTAL_MAX ||
        REVERSE_DATA(CYDEV_EE_BASE) > 3
    ) {
        // New radio! Si570_Start() will compute xtal
        // and it will immediately be stored in eeprom.
        Si570_Xtal = 0;
        Audio_IQ_Channels = 0;
    } else {
        Si570_Xtal = XTAL_DATA(CYDEV_EE_BASE);
        Audio_IQ_Channels = REVERSE_DATA(CYDEV_EE_BASE);
    }
    
}

void Settings_Main(void) {
    uint8 i;
    
    if (
        XTAL_DATA(CYDEV_EE_BASE) != Si570_Xtal ||
        REVERSE_DATA(CYDEV_EE_BASE) != Audio_IQ_Channels
    ) {
        if (EEPROM_QueryWrite() != CYRET_STARTED) {
            for (i=4; i < CYDEV_EEPROM_ROW_SIZE; i++) buffer[i] = 0;
            XTAL_DATA(buffer) = Si570_Xtal;
            REVERSE_DATA(buffer) = Audio_IQ_Channels;
            CySetTemp();
            EEPROM_StartWrite(buffer, 0);
        }
    }

}*/

/* ======================================================================================
 * Geminus 2.152 (2026-10-31): per-band calibration store, copied from Proficio MKII PTT
 * calibration.c (3.238+). Kept in this file so the PSoC Creator project needs no new source.
 * EEPROM rows 8-19 (row 0 = crystal / PPM here; rows 8+ unused on the Geminus).
 * ====================================================================================== */
// Per-band calibration stored in the radio (RX IQ, TX IQ, power). 3.235, 2026-10-21;
// 3.236 adds clear (type 6, in usbvend01.c).
// 3.237: no EEPROM_UpdateTemperature() before a save (as the PPM save in settings01.c).
// 3.238: no save command. Done the PPM way (Ron: "something I know works"): a value set
// from the host is written to EEPROM by the radio itself, ONE row per band, with one
// EEPROM_StartWrite like Set_PPM(). 3.236/3.237 hung USB on Ron's MKII at an audio
// interface change ~1-3 s after a save (9 rows + a no-data control request).
// Same opcode / types / sizes as the Black Pill (CMD_CALIBRATION 0xC8, see basic-plus.h),
// except type 4 write (save) is gone: it stalls here.
// 3.239: clear (type 6) removed: a no-data control request like the 3.236/3.237 save, the
// likely hang cause. Type 6 now stalls.
// The radio only stores the values; the server applies them.
//
// The record lives in RAM (E_cal). usbvend01.c (USB ISR) reads and writes it directly:
// a set lands straight in the record, marks the band valid and marks the band dirty.
// Values are raw little-endian bytes as the host sent them (the 8051 is big endian, so
// the firmware never treats them as numbers).
//
// EEPROM: one 16-byte row per band, rows CAL_ROW_FIRST .. CAL_ROW_FIRST + 11:
//   [0] 'C'  [1] version  [2] band  [3] valid bits (1 RX IQ, 2 TX IQ, 4 power)
//   [4..7] RX IQ  [8..11] TX IQ  [12..13] power  [14] 0  [15] checksum (sum of 0..14)
// Cal_Main() (main loop) writes one dirty row, waits for it to finish (EEPROM_Query
// releases the SPC), then the next. It waits CAL_QUIET_SET passes after the last set so the
// USB data stage has landed, and does not write while transmitting (rows stay dirty).
// Boot: Cal_Init() loads each good row; a bad row = that band 0, not valid.
//
// Variable naming conventions:     E_<variable> -> Externally defined global
//                                  l_<variable> -> locally defined variable
//                                  All UPPERCASE -> Define

#include <usbvend.h>
#include <STRING.H>

#define CAL_ROW_FIRST   8u      // EEPROM row 0 = crystal / PPM / IQ channels (settings01.c), row 1 = keyer
#define CAL_ROW_MAGIC   'C'
#define CAL_ROW_VERSION 2u

#define CAL_ST_IDLE     0u
#define CAL_ST_WAIT     1u

cal_record_t E_cal;
uint8 E_cal_status[2] = {CAL_SAVE_IDLE, 0u};   // [write status, 1 = loaded from EEPROM]
volatile uint8 E_cal_dirty[CAL_BANDS];          // set by the ISR, cleared when the row write starts
volatile uint8 E_cal_quiet = 0;                 // set by the ISR at each set

static uint8 l_state = CAL_ST_IDLE;
static uint8 l_row[CYDEV_EEPROM_ROW_SIZE];

static uint8 Cal_Sum(const uint8 *p) {
    uint8 i, sum = 0;

    for (i = 0; i < (CYDEV_EEPROM_ROW_SIZE - 1u); i++) sum += p[i];
    return sum;
}

static void Cal_Build_Row(uint8 band) {
    uint8 i, l_bit = (uint8)(1u << (band & 7u)), l_hi = band >> 3;

    l_row[0] = CAL_ROW_MAGIC;
    l_row[1] = CAL_ROW_VERSION;
    l_row[2] = band;
    l_row[3] = 0;
    if (E_cal.valid[0][l_hi] & l_bit) l_row[3] |= 1u;
    if (E_cal.valid[1][l_hi] & l_bit) l_row[3] |= 2u;
    if (E_cal.valid[2][l_hi] & l_bit) l_row[3] |= 4u;
    for (i = 0; i < 4u; i++) {
        l_row[4u + i] = E_cal.rx_iq[band][i];
        l_row[8u + i] = E_cal.tx_iq[band][i];
    }
    l_row[12] = E_cal.power[band][0];
    l_row[13] = E_cal.power[band][1];
    l_row[14] = 0;
    l_row[15] = Cal_Sum(l_row);
}

// Call after Settings_Init() (EEPROM_Start)
void Cal_Init(void) {
    reg8 *l_ee;
    uint8 band, i, l_bit, l_hi;

    memset(&E_cal, 0, sizeof(E_cal));
    E_cal_status[1] = 0u;
    for (band = 0; band < CAL_BANDS; band++) {
        E_cal_dirty[band] = 0;
        l_ee = (reg8 *)(CYDEV_EE_BASE + ((uint16)(CAL_ROW_FIRST + band) * CYDEV_EEPROM_ROW_SIZE));
        for (i = 0; i < CYDEV_EEPROM_ROW_SIZE; i++) l_row[i] = l_ee[i];
        if (l_row[0] != CAL_ROW_MAGIC || l_row[1] != CAL_ROW_VERSION || l_row[2] != band ||
            l_row[15] != Cal_Sum(l_row)) continue;
        l_bit = (uint8)(1u << (band & 7u));
        l_hi = band >> 3;
        if (l_row[3] & 1u) E_cal.valid[0][l_hi] |= l_bit;
        if (l_row[3] & 2u) E_cal.valid[1][l_hi] |= l_bit;
        if (l_row[3] & 4u) E_cal.valid[2][l_hi] |= l_bit;
        for (i = 0; i < 4u; i++) {
            E_cal.rx_iq[band][i] = l_row[4u + i];
            E_cal.tx_iq[band][i] = l_row[8u + i];
        }
        E_cal.power[band][0] = l_row[12];
        E_cal.power[band][1] = l_row[13];
        E_cal_status[1] = 1u;
    }
}

// Main loop: writes the rows of the bands the host set, one row at a time
void Cal_Main(void) {
    cystatus l_status;
    uint8 band, l_int;

    switch (l_state) {
    case CAL_ST_IDLE:
        if (E_cal_quiet) {                      // a set just arrived: let the data stage land
            E_cal_quiet--;
            break;
        }
        if (TX_Request || E_PTT || E_key_down) break;   // rows stay dirty until RX
        for (band = 0; band < CAL_BANDS; band++) {
            if (E_cal_dirty[band]) break;
        }
        if (band >= CAL_BANDS) break;
        l_int = CyEnterCriticalSection();       // the ISR may set it again: that write follows
        E_cal_dirty[band] = 0;
        Cal_Build_Row(band);
        CyExitCriticalSection(l_int);
        E_cal_status[0] = CAL_SAVE_BUSY;
        l_status = EEPROM_StartWrite(l_row, (uint8)(CAL_ROW_FIRST + band));
        if (l_status == CYRET_STARTED) {
            l_state = CAL_ST_WAIT;
        } else {
            E_cal_dirty[band] = 1;              // SPC busy (e.g. temperature read): next pass
        }
        break;

    case CAL_ST_WAIT:
        l_status = EEPROM_Query();              // also releases the SPC when done
        if (l_status == CYRET_STARTED) break;   // row still being written
        E_cal_status[0] = (l_status == CYRET_SUCCESS) ? CAL_SAVE_OK : CAL_SAVE_FAILED;
        if (l_status == CYRET_SUCCESS) E_cal_status[1] = 1u;
        l_state = CAL_ST_IDLE;
        break;

    default:
        l_state = CAL_ST_IDLE;
        break;
    }
}
