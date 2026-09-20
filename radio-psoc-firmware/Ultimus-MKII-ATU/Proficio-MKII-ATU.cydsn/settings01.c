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
