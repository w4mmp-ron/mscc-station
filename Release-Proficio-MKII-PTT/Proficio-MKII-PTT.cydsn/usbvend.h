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

//These send Data TO the HOST 
#define CMD_GET_VERSION     0x00 // CMD_GET_VERSION
#define CMD_GET_PIN         0x02 // CMD_GET_PIN - Not Needed for Multus - Currently a NOOP 
#define CMD_GET_FREQ        0x3A // CMD_GET_FREQ
#define CMD_GET_STARTUP     0x3C // CMD_GET_STARTUP - Not Needed for Multus - Currently a NOOP 
#define CMD_GET_XTAL        0x3D // CMD_GET_XTAL - Not Needed for Multus - Currently a NOOP 
#define CMD_GET_SI570       0x3F // CMD_GET_SI570 - Not Needed for Multus - Currently a NOOP 
#define CMD_SET_USRP1       0x50 // CMD_SET_USRP1
#define CMD_GET_CW_KEY      0x51 // CMD_GET_CW_KEY
#define CMD_SET_SI570       0x20 // CMD_SET_SI570 - Not Needed for Multus - Currently a NOOP 


//These receive Data FROM the HOST
#define CMD_SET_FREQ_REG    0x30 // CMD_SET_FREQ_REG - Not Needed for Multus - Currently a NOOP 
#define CMD_SET_FREQ        0x32 // CMD_SET_FREQ - Set the LO Frequency
#define CMD_SET_XTAL_INT    0x33 // CMD_SET_XTAL - Used for calibration 
#define	CMD_SET_PPM		    0x35
#define CMD_SET_XTAL_DEC    0x3A // CMD_SET_XTAL - Used for calibration 
#define CMD_GET_PPM_INT     0x94 // Return INTEGER PART OF PPM
#define CMD_GET_PPM_DEC     0x95 // Return DECIMAL PART OF PPM

#define CMD_GET_POTENTIA_POWER 0x05
#define CMD_GET_POTENTIA_TEMPERATURE 0x06
#define CMD_SET_SET_WIPER 0x09


#define SET_CW_MODE 0x70
#define SET_KEYER_MODE 0x71
#define SET_QSK 0x72
#define SET_CW_PADDLE 0x73
#define SET_IAMBIC_TYPE 0x74
#define SET_SPACING 0x75
/* Memory-play Farnsworth text WPM (was SET_MEMORY_TYPE).
 * Param: 0=off; 5–60=text WPM for CQ memory inter-char gaps. Elements use SET_WPM. */
#define SET_MEM_TEXT_WPM 0x76
#define SET_MEMORY_TYPE SET_MEM_TEXT_WPM /* legacy alias */
#define SET_WEIGHT 0x77
#define SET_SEMI_BREAKIN 0x78
#define SET_SEMI_CONTROL 0x79
#define SET_TX_HOLD 0x7A
#define SET_WPM 0x7B
#define SET_IAMBIC_TUNING 0x7C
#define SET_KEYER_INSTALLED 0x7D
#define SET_CW_INTERFACE_METHOD 0x7E
#define SET_SIDE_TONE 0x7F

// CW Message Management (legacy 0x80-0x82; NB uses those on host now)
#define SET_CW_RECORD_MESSAGE 0x80
#define SET_CW_PLAY_MSG 0x81
#define SET_CW_STOP_MSG 0x82

/* PIC keyer CQ memory — match ms-sdr CMD_SET_KEYER_MEMORY.
 * Param: 0=play, 1=store begin, 2=store end, 0x20-0x7E=append ASCII char */
#define CMD_SET_KEYER_MEMORY 0x9C

//Special Commands
#define CMD_SET_TRANSCEIVER_CW_PITCH 0x90           
#define DLL_VERSION 0xA0
#define CMD_SET_BAND_VOLUME_BAND 0xA1
#define CMD_SET_BAND_VOLUME_VOLUME 0xA2
#define CMD_GET_KEY_DOWN 0xA4
#define CMD_GET_PTT 0xA5    
#define CMD_SET_RIG_TUNE 0xA6
#define CMD_SET_CALIBRATE 0xA7
#define CMD_SET_POWER_BAND 0xA8
#define CMD_SET_TRANSVERTER 0xA9
#define CMD_SET_BAND_VOLUME_DEFAULTS 0xAA
#define CMD_GET_BAND_VOLUME 0xB4
#define CMD_SET_TRANSCEIVER_DISPLAY 0xCC
#define CMD_SET_STAR 0xCD
#define CMD_SET_STEP_VALUE 0xCE
#define CMD_SET_S_METER 0xC0
#define CMD_GET_MIA_STATUS 0xBE
#define CMD_GET_TRANSCEIVER_TEMP 0xBF

#define CMD_SET_HDSDR_STATUS 0xF0   //Not used by firmware. Here for documentation purposes
#define CMD_GET_HDSDR_STATUS 0xF1   //Not used by firmware. Here for documentation purposes
#define CMD_SET_PCB_VERSION 0xF3
#define CMD_STOP_GUI 0xFF           //Not used by firmware. Here for documentation purposes
#define CMD_SET_PA_BYPASS 0xF7
#define SI570_DLL 0
#define SI5351_DLL 1

//Volume Calibration
#define CMD_SET_LEFT_VOLUME 0xE0
#define CMD_SET_RIGHT_VOLUME 0xE1

/* [] END OF FILE */
