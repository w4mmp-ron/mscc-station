// Iambino - Iambic Keyer for Arduino
// Copyright (C) 2013 David Turnbull AE9RB
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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



//These are associated with the CW_Control Control Register

#define CFG_LAG_MAX 25
#define CFG_WEIGHT_DIST 0.50
#define CW_DEFAULT_HOLD_TIME 50

// Morse code timings.
#define DIT 1
#define DAH 3

// Keying modes
#define CFG_MODE_IAMBIC 0
#define CFG_MODE_ULTIMATIC 1
#define CFG_MODE_BUG 2
#define CFG_MODE_STRAIGHT 3

//Set Iambic mode on or off
#define CFG_IAMBIC_OFF 0
#define	CFG_IAMBIC_ON 1

// Keying memory
#define CFG_MEMORY_TYPE_A 0
#define CFG_MEMORY_TYPE_DAH 1
#define CFG_MEMORY_TYPE_DIT 2
#define CFG_MEMORY_TYPE_B 3

// Keying spacing
#define CFG_SPACING_NONE 0
#define CFG_SPACING_EL 1
#define CFG_SPACING_CHAR 2
#define CFG_SPACING_WORD 3

// Paddle reversal
#define CFG_PADDLE_NORMAL 0
#define CFG_PADDLE_REVERSE 1

// External Sound
#define CFG_EXTERNAL_SOUND_OFF 0
#define CFG_EXTERNAL_SOUND_ON 1

// Character dit/dah weight - Use with SET_WEIGHT - increment or decrement by 10 to change weight
//#define CFG_WEIGHT_DIST 50

//Iambic speed tuning - allows fine tuning of the Iambic keying speed
#define CFG_IAMBIC_CALIBRATION_DEFAULT 120

//Amount of time the radio is held in TX mode - Use with SET_TX_HOLD. Set this to zero(0) for full QSK keying
#define CFG_TX_HOLD_DEFAULT 127

//Semi Break In on or off - Use with SET_SEMI_BREAKIN
#define CFG_SEMI_BREAKIN_OFF 0
#define CFG_SEMI_BREAKIN_ON 1

//Set all CW parameters to default (CW Mode is set to Off)   - Use with SET_CW_DEFAULTS
#define CFG_CW_DEFAULTS_OFF 0
#define CFG_CW_DEFAULTS_ON 1

//Use with SET_SEMI_CONTROL For backwards compatiblility with previous versions of the firmware
#define CFG_SEMI_USE_ATU 0
#define CFG_SEMI_USE_AMP 1

//Use with SET_CW_INTERFACE_METHOD - Determines how the OSB goes into transmit mode
#define CFG_INTERFACE_DIRECT 0
#define CFG_INTERFACE_COMM_PORT 1


// Debounce straight key both up and down.
#define KEY_DEBOUNCE_SRAIGHT 8000

// Start looking for key input 2ms early in case the key is bouncing
// up at the moment we're suppose to start a new dit or dah.
#define KEY_DEBOUNCE_IAMBIC 2000


#define IAMBIC_CALIBRATION_SPEED  1200000
#define RATIO_FACTOR 10000


#define EE_FIRMWARE_VERSION 0u
#define EE_PADDLE_REGISTER 1u
#define EE_KEY_TYPE_REGISTER 2u
#define EE_SPACING_REGISTER 3u
#define EE_SIDE_TONE_FREQ_REGISTER 4u
#define EE_MEMORY_REGISTER 5u
#define EE_CW_INTERFACE_METHOD 6u
#define EE_EXTERNAL_SOUND_REGISTER 7u
#define EE_CW_MODE_REGISTER 8u
#define EE_IAMBIC_MODE_REGISTER 9u
#define EE_SEMI_BREAKIN_REGISTER 10u
#define EE_TX_HOLD_REGISTER 11u
#define EE_WPM_REGISTER 12u
#define EE_WEIGHT_REGISTER 13u
#define EE_IAMBIC_TUNING_REGISTER 14u
#define EE_SEMI_CONTROL_REGISTER 15u

#define SEMI_USE_ATU 0
#define SEMI_USE_AMP 1


