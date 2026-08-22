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

uint32 result;
uint8 E_dll_version = SI570_DLL;
/* Discard buffer for reboot OUT payloads (host often sends sizeof(int)=4). */
static uint8 E_reboot_pad[4];

// Maps PSoC registers into one that looks like the AVR
uint8 emulated_register(void) {
    uint8 reg = 0, key;
    if (TX_Request) reg |= 0x10;
    key = Status_Read();
    if (key & STATUS_KEY_0) reg |= 0x20;
    if (key & STATUS_KEY_1) reg |= 0x02;
    return reg;
}

uint8 USBFS_HandleVendorRqst(void)  //This called by the USBFS component.  This is an interrupt handler. 
{                                   //This Interrupt handler can not call other routines.  
    uint8 requestHandled = USBFS_FALSE;
    uint8 reqType, reqCmd;
    
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
                if (CY_GET_REG8(USBFS_wValueLo) & 0x01)
                    TX_Request = 1;
                else
                    TX_Request = 0;
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
            case CMD_GET_KEY_DOWN:
                USBFS_currentTD.pData = (void *)&E_key_down;
                USBFS_currentTD.count = 1;
                requestHandled  = USBFS_InitControlRead();
                break;
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
            case CMD_SET_XTAL_INT: // CMD_SET_XTAL
                USBFS_currentTD.pData = (void *)&E_calibration;
                USBFS_currentTD.count = sizeof(E_calibration);
                requestHandled  = USBFS_InitControlWrite();
                break;
            case CMD_SET_XTAL_DEC: // CMD_SET_XTAL
                USBFS_currentTD.pData = (void *)&E_calibration_dec;
                USBFS_currentTD.count = sizeof(E_calibration_dec);
                requestHandled  = USBFS_InitControlWrite();
                break;
            case DLL_VERSION: 
                USBFS_currentTD.pData = (void *)&E_dll_version;
                USBFS_currentTD.count = sizeof(E_dll_version);
                requestHandled  = USBFS_InitControlWrite();
                break;    
            case CMD_SET_TUNE: // Get the current TUNE frequency
                USBFS_currentTD.pData = (void *)&E_tune_freq_from_host;
                USBFS_currentTD.count = sizeof(E_tune_freq_from_host);
                requestHandled  = USBFS_InitControlWrite();
                break;
            case CMD_SET_RIT: // Get the current TUNE frequency
                USBFS_currentTD.pData = (void *)&E_new_rit_freq;
                USBFS_currentTD.count = sizeof(E_new_rit_freq);
                requestHandled  = USBFS_InitControlWrite();
                break;
            case CMD_SET_BAND_VOLUME_BAND: // Get the first part of setting the band volume - The band
                USBFS_currentTD.pData = (void *)&E_band_volume_band;
                USBFS_currentTD.count = sizeof(E_band_volume_band);
                requestHandled  = USBFS_InitControlWrite();
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = CMD_SET_BAND_VOLUME_BAND;
                }
                break;
            case CMD_SET_BAND_VOLUME_VOLUME: // Get the first part of setting the band volume - The band
                USBFS_currentTD.pData = (void *)&E_band_volume_volume;
                USBFS_currentTD.count = sizeof(E_band_volume_volume);
                requestHandled  = USBFS_InitControlWrite();
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = CMD_SET_BAND_VOLUME_VOLUME;
                }
                break;
                       
        //The following receive CW Mode setting commands and Parameters from the host computer
        //The variables set by these commands will be examined by process_cw_command_from_host() in iambic.c
            case SET_CW_MODE:
                USBFS_currentTD.pData = (void *)&E_host_mode;
                USBFS_currentTD.count = sizeof(E_host_mode);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_CW_MODE;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_CW_MODE;
                }
                break;
                                
            case SET_CW_RECORD_MESSAGE:
                USBFS_currentTD.pData = (void *)&E_cw_message;
                USBFS_currentTD.count = sizeof(E_cw_message);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_CW_RECORD_MESSAGE;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_CW_RECORD_MESSAGE;
                }
                break;
                
            case SET_CW_PLAY_MSG:
                USBFS_currentTD.pData = (void *)&E_cw_play_msg;
                USBFS_currentTD.count = sizeof(E_cw_play_msg);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_CW_PLAY_MSG;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_CW_PLAY_MSG;
                }
                break;
                
            case SET_CW_STOP_MSG:
                USBFS_currentTD.pData = (void *)&E_cw_play_msg;
                USBFS_currentTD.count = sizeof(E_cw_play_msg);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_CW_STOP_MSG;
               if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_CW_STOP_MSG;
                }
                break;
                
            case SET_IAMBIC_MODE:
                USBFS_currentTD.pData = (void *)&ee_iambic_mode;
                USBFS_currentTD.count = sizeof(ee_iambic_mode);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_IAMBIC_MODE;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_IAMBIC_MODE;
                }
                break;
                
            case SET_SIDE_TONE:
                USBFS_currentTD.pData = (void *)&ee_external_sound;
                USBFS_currentTD.count = sizeof(ee_external_sound);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_SIDE_TONE_VOLUME;
               if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_SIDE_TONE;
                }
                break;
                
            case SET_CW_PADDLE:
                USBFS_currentTD.pData = (void *)&ee_paddle;
                USBFS_currentTD.count = sizeof(ee_paddle);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_CW_PADDLE;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_CW_PADDLE;
                }
                break;
                                
            case SET_IAMBIC_TYPE:
                USBFS_currentTD.pData = (void *)&ee_iambic_type;
                USBFS_currentTD.count = sizeof(ee_iambic_type);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_KEY_TYPE;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_IAMBIC_TYPE;
                }
                break;    
                                
            case SET_SPACING:
                USBFS_currentTD.pData = (void *)&ee_spacing;
                USBFS_currentTD.count = sizeof(ee_spacing);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_SPACING;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_SPACING;
                }
                break;    
                                
            case SET_MEMORY_TYPE:
                USBFS_currentTD.pData = (void *)&ee_memory;
                USBFS_currentTD.count = sizeof(ee_memory);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_MEMORY_TYPE;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_MEMORY_TYPE;
                }
                break;    
                
             case SET_WEIGHT:
                USBFS_currentTD.pData = (void *)&ee_weight;
                USBFS_currentTD.count = sizeof(ee_weight);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_WEIGHT;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_WEIGHT;
                }
                break;
                
            case SET_SEMI_BREAKIN:
                USBFS_currentTD.pData = (void *)&ee_semi_breakin;
                USBFS_currentTD.count = sizeof(ee_semi_breakin);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_SEMI_BREAKIN;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_SEMI_BREAKIN;
                }
                break;                
                
            case SET_SEMI_CONTROL:
                USBFS_currentTD.pData = (void *)&ee_semi_control;
                USBFS_currentTD.count = sizeof(ee_semi_control);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_SEMI_CONTROL;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_SEMI_CONTROL;
                }
                break;                
                
            case SET_TX_HOLD:
                USBFS_currentTD.pData = (void *)&ee_tx_hold;
                USBFS_currentTD.count = sizeof(ee_tx_hold);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_TX_HOLD;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_TX_HOLD;
                }
                break;
                
            case SET_WPM:
                USBFS_currentTD.pData = (void *)&ee_wpm;
                USBFS_currentTD.count = sizeof(ee_wpm);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_WPM;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_WPM;
                }
                break;
               
            case SET_IAMBIC_TUNING:
                USBFS_currentTD.pData = (void *)&ee_iambic_tuning;
                USBFS_currentTD.count = sizeof(ee_iambic_tuning);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_IAMBIC_TUNING;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_IAMBIC_TUNING;
                }
                break;
                
            case SET_CW_DEFAULTS:
                USBFS_currentTD.pData = (void *)&E_cw_defaults;
                USBFS_currentTD.count = sizeof(E_cw_defaults);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_CW_DEFAULTS;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_CW_DEFAULTS;
                }
                break;        
                
            case SET_CW_INTERFACE_METHOD:
                USBFS_currentTD.pData = (void *)&ee_cw_interface_method;
                USBFS_currentTD.count = sizeof(ee_cw_interface_method);
                requestHandled  = USBFS_InitControlWrite();
                //E_cw_command = SET_CW_INTERFACE_METHOD;
                if(E_command_queue_front == (E_command_queue_rear +1) %MAX_COMMAND_QUEUE) {
                    ERROR ("C O M M A N D  Q U E U E   ");
                }else{
                    if(E_command_queue_front == -1) {
                        E_command_queue_front = E_command_queue_rear = 0;
                    }else{
                        E_command_queue_rear = (E_command_queue_rear + 1)%MAX_COMMAND_QUEUE;
                    }
                    E_command_queue[E_command_queue_rear] = SET_CW_INTERFACE_METHOD;
                }
                break;

            /*
             * Soft reboot / bootloader — do NOT call CySoftwareReset / Bootloadable_Load
             * here (USB ISR). Set flag; main loop performs the reset.
             * Host typically OUT 4 bytes (ms-sdr Radio_send_parameters int); payload ignored.
             */
            case CMD_REBOOT_APP:
                USBFS_currentTD.pData = (void *)&E_reboot_pad;
                USBFS_currentTD.count = sizeof(E_reboot_pad);
                requestHandled = USBFS_InitControlWrite();
                E_reboot_request = REBOOT_REQ_APP;
                break;

            case CMD_ENTER_BOOTLOADER:
                USBFS_currentTD.pData = (void *)&E_reboot_pad;
                USBFS_currentTD.count = sizeof(E_reboot_pad);
                requestHandled = USBFS_InitControlWrite();
                E_reboot_request = REBOOT_REQ_BOOTLOADER;
                break;

        }//End switch (reqCmd)
    }//End if ((reqType & USBFS_RQST_DIR_MASK) == USBFS_RQST_DIR_D2H)
    return(requestHandled);
}

