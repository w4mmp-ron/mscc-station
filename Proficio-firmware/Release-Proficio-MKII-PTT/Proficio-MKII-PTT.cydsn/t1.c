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
#define PULSE_TUNER_TIME 5000;

// Full support for Elecraft T1 Automatic Antenna Tuner

uint8 T1_Band_Number;
uint8 T1_Tune_Request;
uint32 Pulse_Tuner = PULSE_TUNER_TIME

/* void T1_Main(void) {
    static uint8 state = 0, timer, band, send, bits, band_request, band_request_timer;
    static uint16 tune_timer; 
    //static uint8 previous_E_AMP_Bypass = 100;
    //static uint8 band_refresh_count = 3;
    
    if(Pulse_Tuner-- == 0){
        band = 0;
        Pulse_Tuner = PULSE_TUNER_TIME;
    }
    if (tune_timer) {
        tune_timer--;
        if (!tune_timer) {
            Control_Write(Control_Read() & ~CONTROL_ATU_1);
        }
    }

    if (band_request_timer) band_request_timer--;
    else if (T1_Band_Number != band) {
        band_request = 1;
        band_request_timer = 200;
        Pulse_Tuner = PULSE_TUNER_TIME;
    }

    switch(state) {
    case 0: // idle
        if (Status_Read() & STATUS_ATU_0) timer++;
        else {
            if (timer >= 85 && timer <= 115) {
                state = 1;
                timer = 20;
                send = band = T1_Band_Number;
                bits = 4;
                Control_Write(Control_Read() & ~CONTROL_ATU_0 | CONTROL_ATU_0_OE);
            }
            else {
                timer = 0;
                if (band_request || T1_Tune_Request) {
                    Control_Write(Control_Read() | CONTROL_ATU_1);
                    if (T1_Tune_Request) tune_timer = 1000;
                    else tune_timer = 10;
                    band_request = T1_Tune_Request = 0;
                }
            }
        }
        break;
    case 1: // data low
        timer--;
        if (!timer) {
            if (bits) {
                Control_Write(Control_Read() | CONTROL_ATU_0);
                if (send & 0x08) timer = 8;
                else timer = 3;
                send <<= 1;
                bits--;
                state = 2;
            } else {
                Control_Write(Control_Read() & ~(CONTROL_ATU_0 | CONTROL_ATU_0_OE));
                state = 0;
            }
        }
        break;
    case 2: // data high
        timer--;
        if (!timer) {
            if (bits) timer = 3;
            else timer = 10;
            Control_Write(Control_Read() & ~CONTROL_ATU_0);
            state = 1;
        }
        break;
    }   

}
*/
