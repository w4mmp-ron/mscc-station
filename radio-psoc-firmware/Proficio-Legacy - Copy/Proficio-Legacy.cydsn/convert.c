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

#include <device.h>
#include <basic-plus.h>
//#include <si5351.h>
#define N_DECIMAL_POINTS_PRECISION (1000000)

uint32 convert_from_host(uint32 freq){
    volatile float fout;
    volatile uint32 int_freq, freq_in_hz/*,freq_in_hz_mhz*/,freq_in_hz_khz;
    volatile uint32 l_freq_from_host = 0;
    
    fout = (float)swap32(freq) / 0x200000;
    int_freq = (int32)fout;
    freq_in_hz_khz = ((int32)(fout*N_DECIMAL_POINTS_PRECISION)%N_DECIMAL_POINTS_PRECISION);
    freq_in_hz = (int_freq * 1000000) + freq_in_hz_khz;
    //freq_in_hz = freq_in_hz_mhz + freq_in_hz_khz;
    return freq_in_hz;
}
/* [] END OF FILE */
