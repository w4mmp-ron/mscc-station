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
#include <usbvend.h>
#define PPM_INT 0
#define PPM_DEC 1
#define PPM_INT_RECEIVED 0x01
#define PPM_DEC_RECEIVED 0x02

//extern struct band_volume E_band_and_volume[10];
uint32 result;
uint8 E_dll_version = SI570_DLL;
static uint8_t ppm_start = 0;
static int32 proficio_temperature = 0;



int32 swap32_int(int32 original) CYREENTRANT {
    uint8 *r, *o;
    int32 ret;
    r = (void*)&ret;
    o = (void*)&original;
    r[0] = o[3];
    r[1] = o[2];
    r[2] = o[1];
    r[3] = o[0];
    return ret;
}


// Maps PSoC registers into one that looks like the AVR
uint8 emulated_register(void) {
    uint8 reg = 0, key;
    if (TX_Request) reg |= 0x10;
    //key = Status_Read();
    key = E_key_down;
    if (key & STATUS_KEY_0) reg |= 0x20;
    if (key & STATUS_KEY_1) reg |= 0x02;
    return reg;
}

uint8 USBFS_HandleVendorRqst(void)  //This called by the USBFS component.  This is an interrupt handler. 
{                                   //This Interrupt handler can not call other routines.  
    uint8 requestHandled = USBFS_FALSE;
    uint8 reqType, reqCmd;
    static int32 l_dec = 0;
    static int32 l_int = 0;
    uint8_t E_Potentia_temperature;
    static int8_t int_value = 0;
    
     
    reqType = CY_GET_REG8(USBFS_bmRequestType);
    reqCmd = CY_GET_REG8(USBFS_bRequest);
    
    //These send DATA TO the HOST 
    if ((reqType & USBFS_RQST_DIR_MASK) == USBFS_RQST_DIR_D2H)
    {
        switch (reqCmd)
        {
            case 0x00: // CMD_GET_VERSION
                *(uint16*)&result = FIRMWARE_VERSION;
                USBFS_currentTD.pData = (void *)&result;
                USBFS_currentTD.count = 2;
                requestHandled  = USBFS_InitControlRead();
                break;
            case 0x02: // CMD_GET_PIN
                // used on the test tab in cfgsr
                *(uint8*)&result = emulated_register();
                result |= 0x08000000; // 3.3v indicator
                USBFS_currentTD.pData = (void *)&result;
                USBFS_currentTD.count = 1;
                requestHandled  = USBFS_InitControlRead();
                break;
            case 0x3A: // CMD_GET_FREQ
                USBFS_currentTD.pData = (void *)&Si570_LO;
                USBFS_currentTD.count = sizeof(Si570_LO);
                requestHandled  = USBFS_InitControlRead();
                break;
            case 0x3C: // CMD_GET_STARTUP
                result = swap32(SI570_STARTUP_FREQ * 0x0200000);
                USBFS_currentTD.pData = (void *)&result;
                USBFS_currentTD.count = sizeof(result);
                requestHandled  = USBFS_InitControlRead();
                break;
            case 0x3D: // CMD_GET_XTAL
                USBFS_currentTD.pData = (void *)&Si570_Xtal;
                USBFS_currentTD.count = sizeof(Si570_Xtal);
                requestHandled  = USBFS_InitControlRead();
                E_dll_version = SI570_DLL;
                break;
            case 0x3F: // CMD_GET_SI570
                USBFS_currentTD.pData = (void *)(Si570_Buf+2);
                USBFS_currentTD.count = 6;
                requestHandled  = USBFS_InitControlRead();
                break;
            case 0x50: // CMD_SET_USRP1
                if (CY_GET_REG8(USBFS_wValueLo) & 0x01){
                    TX_Request = 1;
                }
                else{
                    TX_Request = 0;
                }
                    
                //nobreak, returns key value
            case 0x51: // CMD_GET_CW_KEY
                *(uint8*)&result = emulated_register();
                USBFS_currentTD.pData = (void *)&result;
                USBFS_currentTD.count = 1;
                requestHandled  = USBFS_InitControlRead();
                break;
            case 0x20: // CMD_SET_SI570
                // Fake a reset!  Used for calibration.
                if (CY_GET_REG8(USBFS_wValueHi) == 0x87 && CY_GET_REG16(USBFS_wIndex) == 0x01) {
                    //Si570_Fake_Reset();
                    *(uint8*)&result = 0;
                    USBFS_currentTD.pData = (void *)&result;
                    USBFS_currentTD.count = 1;
                    requestHandled  = USBFS_InitControlRead();
                }
                break;
            case CMD_GET_KEY_DOWN:
                USBFS_currentTD.pData = (void *)&E_key_down;
                USBFS_currentTD.count = 1;
                requestHandled  = USBFS_InitControlRead();
                break;
                
            case CMD_GET_PTT:
                USBFS_currentTD.pData = (void *)&E_PTT;
                USBFS_currentTD.count = 1;
                requestHandled  = USBFS_InitControlRead();
                break;
                               
            case CMD_GET_TRANSCEIVER_TEMP:
                proficio_temperature = swap32_int(E_transceiver_temp);
                //*(uint16*)&result = FIRMWARE_VERSION;
                USBFS_currentTD.pData = (void *)&proficio_temperature;
                USBFS_currentTD.count = 4;
                requestHandled  = USBFS_InitControlRead();
                break;
                
            case CMD_GET_POTENTIA_TEMPERATURE:
                USBFS_currentTD.pData = (void *)&E_Potentia_temperature;
                USBFS_currentTD.count = 4;
                requestHandled  = USBFS_InitControlRead();
                break;
                
            case CMD_SET_XTAL_INT:
                int_value = ee_ppm_int;
                USBFS_currentTD.pData = (void *)&int_value;
                USBFS_currentTD.count = sizeof(int_value);
                requestHandled  = USBFS_InitControlRead();
                break;
                
            /*case CMD_SET_XTAL_DEC:
                USBFS_currentTD.pData = (void *)&ee_ppm_int;
                USBFS_currentTD.count = 1;
                requestHandled  = USBFS_InitControlRead();
                break;
                */
                
        }
    }
    
    //These receive DATA FROM the HOST
    if ((reqType & USBFS_RQST_DIR_MASK) == USBFS_RQST_DIR_H2D)
    {
        switch (reqCmd)
        {
            case CMD_SET_FREQ_REG: // CMD_SET_FREQ_REG
                USBFS_currentTD.pData = (void *)&Si570_OLD;
                USBFS_currentTD.count = 6;
                requestHandled  = USBFS_InitControlWrite();
                break;
            case CMD_SET_FREQ: // CMD_SET_FREQ
                USBFS_currentTD.pData = (void *)&Si570_LO;
                USBFS_currentTD.count = sizeof(Si570_LO);
                requestHandled  = USBFS_InitControlWrite();
                break;
            case CMD_SET_XTAL_INT: // CMD_SET_XTAL_INT Integer Part
                USBFS_currentTD.pData = (void *)&E_calibration_int;
                USBFS_currentTD.count = sizeof(E_calibration_int);
                requestHandled  = USBFS_InitControlWrite();
                l_int = E_calibration_int;
                E_PPM_needs_updated = E_PPM_needs_updated | PPM_INT_RECEIVED;
                break;
            case CMD_SET_XTAL_DEC: // CMD_SET_XTAL_DEC  Decimal Part
                USBFS_currentTD.pData = (void *)&E_calibration_dec;
                USBFS_currentTD.count = sizeof(E_calibration_dec);
                requestHandled  = USBFS_InitControlWrite();
                l_dec = E_calibration_dec;
                E_PPM_needs_updated = E_PPM_needs_updated | PPM_DEC_RECEIVED;
                //Set_PPM in settings.c will set to 0 (zero) when the PPM has been set in EEPROM
                break;
            case CMD_SET_PPM:
                USBFS_currentTD.pData = (void *)&E_ppm;
                USBFS_currentTD.count = sizeof(E_ppm);
                requestHandled  = USBFS_InitControlWrite();
                break;
            case DLL_VERSION: 
                USBFS_currentTD.pData = (void *)&E_dll_version;
                USBFS_currentTD.count = sizeof(E_dll_version);
                requestHandled  = USBFS_InitControlWrite();
                break;    
            case CMD_SET_TRANSVERTER: // Allow TX to 30MHz
                USBFS_currentTD.pData = (void *)&E_transverter;
                USBFS_currentTD.count = sizeof(E_transverter);
                requestHandled  = USBFS_InitControlWrite();
                //command_queue_add(CMD_SET_TRANSVERTER);
                break;
            case CMD_SET_PCB_VERSION: 
                USBFS_currentTD.pData = (void *)&E_pcb_version;
                USBFS_currentTD.count = sizeof(E_pcb_version);
                requestHandled  = USBFS_InitControlWrite();
                break;    
           case CMD_SET_PA_BYPASS: 
                USBFS_currentTD.pData = (void *)&E_Amplifier;
                USBFS_currentTD.count = sizeof(E_Amplifier);
                requestHandled  = USBFS_InitControlWrite();
                break;    
 
        //The following receive CW Mode setting commands and Parameters from the host computer
            case SET_CW_MODE:
                USBFS_currentTD.pData = (void *)&E_host_mode;
                USBFS_currentTD.count = sizeof(E_host_mode);
                requestHandled  = USBFS_InitControlWrite();
                break;
                
            case SET_QSK:
                USBFS_currentTD.pData = (void *)&E_QSK;
                USBFS_currentTD.count = sizeof(E_QSK);
                requestHandled  = USBFS_InitControlWrite();
                break;
                
            case SET_TX_HOLD:
                USBFS_currentTD.pData = (void *)&E_TX_Hold;
                USBFS_currentTD.count = sizeof(E_TX_Hold);
                requestHandled  = USBFS_InitControlWrite();
                break;
                        
            case CMD_SET_TRANSCEIVER_CW_PITCH: // Get the CW Pitch Index
                USBFS_currentTD.pData = (void *)&E_cw_pitch;
                USBFS_currentTD.count = sizeof(E_cw_pitch);
                requestHandled  = USBFS_InitControlWrite();
                break;
                
            case SET_KEYER_MODE:
                USBFS_currentTD.pData = (void *)&E_keyer_mode;
                USBFS_currentTD.count = sizeof(E_keyer_mode);
                requestHandled  = USBFS_InitControlWrite();
                break;
                
            case SET_CW_PADDLE:
                USBFS_currentTD.pData = (void *)&E_paddle;
                USBFS_currentTD.count = sizeof(E_paddle);
                requestHandled  = USBFS_InitControlWrite();
                break;
                
            case SET_SPACING:
                USBFS_currentTD.pData = (void *)&E_spacing;
                USBFS_currentTD.count = sizeof(E_spacing);
                requestHandled  = USBFS_InitControlWrite();
                break;
                
            case SET_WEIGHT:
                USBFS_currentTD.pData = (void *)&E_weight;
                USBFS_currentTD.count = sizeof(E_weight);
                requestHandled  = USBFS_InitControlWrite();
                break;
                
            case SET_WPM:
                USBFS_currentTD.pData = (void *)&E_wpm;
                USBFS_currentTD.count = sizeof(E_wpm);
                requestHandled  = USBFS_InitControlWrite();
                break;
                
            case SET_SIDE_TONE:
                USBFS_currentTD.pData = (void *)&E_side_tone;
                USBFS_currentTD.count = sizeof(E_side_tone);
                requestHandled  = USBFS_InitControlWrite();
                break;
                
            case SET_KEYER_INSTALLED:
                USBFS_currentTD.pData = (void *)&E_keyer_installed;
                USBFS_currentTD.count = sizeof(E_keyer_installed);
                requestHandled  = USBFS_InitControlWrite();
                break;
           
          
        }//End switch (reqCmd)
    }//End if ((reqType & USBFS_RQST_DIR_MASK) == USBFS_RQST_DIR_D2H)
    return(requestHandled);
}

