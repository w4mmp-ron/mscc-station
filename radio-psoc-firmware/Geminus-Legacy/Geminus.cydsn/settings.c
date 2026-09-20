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

#include <basic-plus.h>

#define XTAL_MIN (114.285 - 2)
#define XTAL_MAX (114.285 + 2)

uint8 buffer[CYDEV_EEPROM_ROW_SIZE];

#define XTAL_DATA(mem) (*(reg32*)(mem+0))
#define REVERSE_DATA(mem) (*(reg8*)(mem+4))

#define PPM_INT 0
#define PPM_DEC 1
#define AUDIO_CHANNELS 2
#define CALIBRATION_TEMPERATURE 3

void Settings_Init(){
    reg8  *RegPointer;
    
    EEPROM_Start();
    CyDelay(100);
    RegPointer = (reg8 *) (CYDEV_EE_BASE + 0u);
    ee_ppm_int = RegPointer[PPM_INT];
    ee_ppm_dec = RegPointer[PPM_DEC];
    Audio_IQ_Channels = RegPointer[AUDIO_CHANNELS];
    E_temp_at_calibration = RegPointer[CALIBRATION_TEMPERATURE];
}

void Settings_Main(){
    volatile int8_t l_ppm_int;
    volatile int8_t l_ppm_dec;
    volatile uint8 l_audio_channels;
    volatile int8 l_temperature;
    reg8 *RegPointer;
    uint8 update = FALSE;
    cystatus  error = 0;
    volatile int8 ee_temp = 0;
        
    RegPointer = (reg8 *) (CYDEV_EE_BASE + 0u);
    l_ppm_int = RegPointer[PPM_INT];
    if(l_ppm_int != ee_ppm_int) update = TRUE;
    l_ppm_dec = RegPointer[PPM_DEC];
    if(l_ppm_dec != ee_ppm_dec) update = TRUE;
    l_audio_channels = RegPointer[AUDIO_CHANNELS];
    if(l_audio_channels != Audio_IQ_Channels) update = TRUE;
    l_temperature = RegPointer[CALIBRATION_TEMPERATURE];
    if(l_temperature != (int8)E_temp_at_calibration){
        update = TRUE;
        ee_temp = (int8)E_temp_at_calibration;
    }
    if(update){
        EEPROM_UpdateTemperature();
        error = EEPROM_ByteWritePos(ee_ppm_int,0u,PPM_INT);
        error = EEPROM_ByteWritePos(ee_ppm_dec,0u,PPM_DEC);
        error = EEPROM_ByteWritePos(Audio_IQ_Channels,0u,AUDIO_CHANNELS);
        error = EEPROM_ByteWritePos(ee_temp,0u,CALIBRATION_TEMPERATURE);
    } 
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
