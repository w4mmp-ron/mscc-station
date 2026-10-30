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
#include <si5351.h>
#include <si5351a.h>

#define TX_LED_BLINK_ON_MS 40
#define TX_LED_BLINK_OFF_MS 50

uint8 TX_Request = 0;
uint8 TX_Inhibit = 0;

/*
 * Non-QSK amp sequencing, same as CW: AMP on, wait E_key_delay_ms (from TX hold) for the
 * amp relays to settle, then PA on. On release PA off, wait AMP_RELEASE_MS, then AMP off.
 * QSK amp (E_QSK TRUE) keeps the original order.
 *
 * 3.241 non-QSK: PCM3060 DAC soft mute (reg 0x44) around the PA edges, so TX_HI never
 * switches with RF at full level (broadband splatter at TUN / PTT release).
 *   Key:     DAC mute, AMP on, key delay (>= 20 ms > 16.7 ms ramp), PA on, DAC unmute.
 *   Release: DAC mute, TX_DAC_RAMP_MS, PA off, AMP_RELEASE_MS, AMP off, DAC unmute.
 */
static uint8 state = 0;

/* Main loop: force TX_Main back to idle (entering CW, where cw.c owns the DAC mute) */
void TX_Reset(void) {
    state = 0;
}

/* 3.242: TRUE when TX_Main is receiving (PA and AMP off, DAC unmuted) */
uint8 TX_Idle(void) {
    return (state == 0);
}

void TX_Main(void) {
    static uint8 start_tick = 0;
    static uint8 delay_ms = 0;

    switch (state) {
        case 0: // receiving
            if (TX_Request && !TX_Inhibit) {
                if(E_QSK == TRUE){
                    state = 2;
                    Control_Write(Control_Read() & ~CONTROL_RX);
                }else{
                    TX_DAC_Mute(TRUE);                              //Ramp DAC down during the key delay
                    Control_Write(Control_Read() & ~CONTROL_AMP);   //AMP on first - Negative logic level
                    delay_ms = E_key_delay_ms;
                    start_tick = E_ms_tick;
                    state = 3;
                }
            }
            break;
        case 2:
            Control_Write(Control_Read() & ~CONTROL_AMP);
            state = 10;
            break;
        case 3: // Non-QSK: AMP on, waiting for the amp relays before PA on
            if (!TX_Request || TX_Inhibit) {
                Control_Write(Control_Read() | CONTROL_AMP);    //PTT dropped before PA on. AMP off
                TX_DAC_Mute(FALSE);
                state = 0;
            }else if ((uint8)(E_ms_tick - start_tick) >= delay_ms) {
                Control_Write(Control_Read() & ~CONTROL_RX);    //PA on - Negative logic level
                TX_DAC_Mute(FALSE);                             //Then ramp the DAC up (~3 ms 10-90 %)
                state = 10;
            }
            break;
        case 10: // transmitting
            if (!TX_Request && !TX_Inhibit) {
                if(E_QSK == TRUE){
                    Control_Write(Control_Read() & ~(CONTROL_LED) | CONTROL_AMP);
                    state = 11;
                }else{
                    TX_DAC_Mute(TRUE);                          //Ramp RF down first, PA stays on
                    start_tick = E_ms_tick;
                    state = 13;
                }
             }
           break;
        case 13: // Non-QSK: DAC ramping down, PA still on
            if (TX_Request) {
                TX_DAC_Mute(FALSE);                             //Re-keyed during the ramp
                state = 10;
            }else if ((uint8)(E_ms_tick - start_tick) >= TX_DAC_RAMP_MS) {
                Control_Write(Control_Read() & ~(CONTROL_LED) | CONTROL_RX);    //PA off
                start_tick = E_ms_tick;
                state = 12;
            }
            break;
        case 11:
            Control_Write(Control_Read() | CONTROL_RX);
            state = 0;
            break;
        case 12: // Non-QSK: PA off, waiting for RF to die before AMP off
            if ((uint8)(E_ms_tick - start_tick) >= AMP_RELEASE_MS) {
                Control_Write(Control_Read() | CONTROL_AMP);
                TX_DAC_Mute(FALSE);                             //DAC back at full for RX / next over
                state = 0;
            }
            break;
    }

}
