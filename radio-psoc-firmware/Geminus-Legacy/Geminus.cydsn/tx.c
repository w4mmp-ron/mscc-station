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
 * Geminus 2.153 (2026-10-31): TUN / PTT edges shaped with the PCM3060 DAC soft mute, as the
 * Proficio MKII PTT 3.241. Before, the T/R relay, BS2 and AMP switched with RF at full level
 * and the I/Q went straight to full: broadband noise at the TUN / PTT edges.
 *   Key:     I/Q blank (DIN) + DAC mute, T/R + BS2 + AMP on, TX_KEY_DELAY_MS (relays settle,
 *            DAC fully muted), I/Q on into the muted DAC, DAC unmute (ramps up).
 *   Release: DAC mute, TX_DAC_RAMP_MS, AMP + BS2 off, AMP_RELEASE_MS, T/R off, I/Q path and
 *            DAC back for RX.
 */
static uint8 state = 0;
static uint8 start_tick = 0;

/* Main loop: TX_Main back to idle (entering CW, where cw.c owns the DAC mute) */
void TX_Reset(void) {
    state = 0;
}

/* TRUE when TX_Main is receiving (relays off, DAC unmuted) */
uint8 TX_Idle(void) {
    return (state == 0);
}

static void tx_relays_off(void) {
    Control_Write((Control_Read() & ~CONTROL_LED) | CONTROL_RX | CONTROL_DIN);  //T/R off, I/Q path back
    TX_DAC_Mute(FALSE);
}

void TX_Main(void) {
    switch (state) {
        case 0: // receiving
            if (TX_Request && !TX_Inhibit) {
                Control_Write(Control_Read() & ~CONTROL_DIN);   //I/Q blank: no RF while the relays switch
                TX_DAC_Mute(TRUE);
                Control_Write(Control_Read() & ~CONTROL_RX);    //T/R on - Negative logic level
                Band_Control_Write(Band_Control_Read() | CONTROL_BAND_TX);
                Control_Write(Control_Read() & ~CONTROL_AMP);   //AMP on - Negative logic level
                start_tick = E_ms_tick;
                state = 3;
            }
            break;
        case 3: // relays settling, I/Q blank, DAC muting
            if (!TX_Request || TX_Inhibit) {
                Control_Write(Control_Read() | CONTROL_AMP);
                Band_Control_Write(Band_Control_Read() & ~CONTROL_BAND_TX);
                tx_relays_off();                                //No RF was sent
                state = 0;
            } else if ((uint8)(E_ms_tick - start_tick) >= TX_KEY_DELAY_MS) {
                Control_Write(Control_Read() | CONTROL_DIN);    //I/Q into the muted DAC
                TX_DAC_Mute(FALSE);                             //RF ramps up
                state = 10;
            }
            break;
        case 10: // transmitting
            if (!TX_Request && !TX_Inhibit) {
                TX_DAC_Mute(TRUE);                              //Ramp RF down first, relays stay
                start_tick = E_ms_tick;
                state = 13;
            }
            break;
        case 13: // DAC ramping down
            if (TX_Request) {
                TX_DAC_Mute(FALSE);                             //Re-keyed during the ramp
                state = 10;
            } else if ((uint8)(E_ms_tick - start_tick) >= TX_DAC_RAMP_MS) {
                Control_Write(Control_Read() & ~CONTROL_DIN);   //I/Q blank
                Control_Write(Control_Read() | CONTROL_AMP);    //AMP off
                Band_Control_Write(Band_Control_Read() & ~CONTROL_BAND_TX);
                start_tick = E_ms_tick;
                state = 12;
            }
            break;
        case 12: // AMP off, waiting before the T/R relay
            if ((uint8)(E_ms_tick - start_tick) >= AMP_RELEASE_MS) {
                tx_relays_off();
                state = 0;
            }
            break;
        default:
            state = 0;
            break;
    }
}
