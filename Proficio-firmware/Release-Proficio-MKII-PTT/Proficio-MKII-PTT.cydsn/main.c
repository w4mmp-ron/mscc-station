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
#include <iambino.h>

uint8 E_host_mode = 'U';
uint8_t E_TX_Hold = CW_DEFAULT_HOLD_TIME;
uint8 E_key_0;
uint8 E_key_1;
uint8 E_key_down = 0;
uint8 E_cw_hold = FALSE;
uint8_t E_keyer_mode = 1;
uint8_t E_wpm = 18;
uint8_t E_spacing = 0;
uint8_t E_mem_text_wpm = 0; /* 0=Farnsworth off until PIC uses it */
uint8_t E_weight = 50;
uint8_t E_side_tone = 0;
uint8_t E_paddle = 0;
uint8_t E_keyer_installed = TRUE;
volatile uint8_t E_keyer_mem_pkt[2] = {0, 0}; /* [0]=param [1]=seq from host */
volatile uint8_t E_keyer_mem_q[KEYER_MEM_Q_SIZE];
volatile uint8_t E_keyer_mem_q_head = 0;
volatile uint8_t E_keyer_mem_q_tail = 0;
volatile uint8_t E_keyer_mem_q_count = 0;
//uint8_t E_side_tone = 0;

uint32 E_current_rit_freq = 0;
uint8 E_transverter = 0;
uint8 E_pcb_version = 2;

uint8 E_PPM_needs_updated = FALSE;

uint8 E_PPM_needs_set = FALSE;
uint8_t E_band;

uint8 E_si5351_status = 0;
uint8_t E_Amplifier = FALSE;
uint8_t E_QSK = FALSE;
volatile uint8_t E_PTT = FALSE;
volatile uint8_t E_reboot_request = REBOOT_REQ_NONE;


void main_init() {
    uint8 si_err, pcm_err;
    uint8_t status = 0;
    uint8_t display_err = 0;
    uint8_t potentia_err = 0;
    uint8_t power_err = 0;
    uint8_t bias_err = 0;
    
    CyDelay(100);
    CyGlobalIntEnable;
    Sync_Start();
    I2C_DISPLAY_Start();
    CW_Hold_Timer_Start();
    CW_isr_StartEx(CW_interrupt);
       
    Settings_Init();
    si_err = si5351_init(SI5351_CRYSTAL_LOAD_10PF,0);
    //si_err = si5351_init(SI5351_CRYSTAL_LOAD_12PF,0);
    if (!si_err) {
        I2C_DISPLAY_Stop();
        I2C_DISPLAY_Start();
        I2C_DISPLAY_MasterClearStatus();
    }
      
    pcm_err = PCM3060_Init();
    if (!si_err && pcm_err) ERROR("E  ");
    if (pcm_err) ERROR("P  ");
    if (!si_err) ERROR("S  ");
    
    Band_Control_Write(CONTROL_BAND_20_30);
    E_current_LO_freq = 14000000;
    //E_SI5351_Reset = FALSE;
    while((si5351aSetFrequency(E_current_LO_freq) != 0));
    CyDelay(1500); //Wait for Keyer MCU to boot
    Control_Write(Control_Read() & ~CONTROL_LED);
    CyDelay(5); //Let things settle down;
}

// A compliant USB device is required to monitor
// vbus voltage and shut down if it disappears.
void main_usb_vbus(void) {
    if (USBFS_VBusPresent()) {
        if(!USBFS_initVar) {
            USBFS_Start(0, USBFS_DWR_VDDD_OPERATION);
            Audio_Start();
            PCM3060_Start();
        }
    } else {
        if(USBFS_initVar) {
            TX_Request = 0;
            PCM3060_Stop();
            USBFS_Stop();
        }
    }
}

int main()
{
    uint8 i, beat, beater = 0;
    uint8_t previous_host_mode = 0;
    uint8_t ptt = 0;
    uint16 timer = 0;
    uint8_t control_status = 0;
    static uint8_t previous_TX_hold_time = 0;
    uint8_t ptt_section = 0;
    uint8_t ptt_delay_count = 10;
           
    main_init();
    for(;;) {
        // USB Audio is very high priority
        Audio_Main();
        Sync_Main();
        if(E_host_mode == 'C') {
            if(previous_host_mode != E_host_mode){
                previous_host_mode = E_host_mode;
            }
            Manage_Paddles_Port();
        }
        // Everything else runs twice per millisecond
        // Keep T1 first for timing accuracy
        i = Status_Read() & STATUS_BEAT;
        if (beat != i) {
            switch(beater++) {
            case 0:
                if(--ptt_delay_count == 0){
                    ptt_section = CyEnterCriticalSection();
                    ptt = Status_Read();
                    CyExitCriticalSection(ptt_section);  
                    if (ptt & STATUS_PTT)  E_PTT = TRUE; else E_PTT = FALSE;
                    ptt_delay_count = 10;
                }
                //T1_Main();
                break;
            case 1:
                if(E_host_mode != 'C'){
                    if(previous_host_mode != E_host_mode){
                        Control_Write(Control_Read() & ~CONTROL_LED);   //Restore LED operation to normal
                        Control_Write(Control_Read() | CONTROL_RX);     //Turn OFF PA - Negative logic level
                        control_status = Control_Read();
                        control_status = control_status | CONTROL_AMP;  //Turn OFF the AMP port - Negative logic level
                        control_status = control_status | CONTROL_DOUT; //Turn ON output from PCM3060
                        Control_Write(control_status);
                        TX_Request = 0;
                        previous_host_mode = E_host_mode;
                    }
                    TX_Main();
                }
                break;
            case 2:
                Settings_Main();
                if (previous_TX_hold_time != E_TX_Hold){
                    previous_TX_hold_time = E_TX_Hold;
                    CW_Hold_Timer_WritePeriod((previous_TX_hold_time * 100)); //Configure the CW hold timer
                }
               if(E_keyer_installed == TRUE){
                    Configure_CW();
               }
                break;
            case 3:
                si5351_main();
                break;
            case 4:
                if(E_si5351_status == 0){
                    Band_Main();
                }
                break;
            case 5:
                Check_temperature();
                if (CyXTAL_ReadStatus()) ERROR("C  ");
                break;
            case 6:
                if(E_host_mode != 'C'){//If mode is CW, frequency setting is managed in cw.c
                    E_si5351_status = si5351aSetFrequency(E_current_LO_freq);
                }
                /* Hardware BOOT jumper (existing) */
                if (!(Status_Read() & STATUS_BOOT)) Bootloadable_Load();
                /* USB-requested reboot (flags set in usbvend ISR).
                 * Wait long enough for the control DATA/STATUS stage to finish
                 * on the host — too short → host sees LIBUSB_ERROR_PIPE. */
                if (E_reboot_request == REBOOT_REQ_APP) {
                    E_reboot_request = REBOOT_REQ_NONE;
                    CyDelay(300);
                    CySoftwareReset();
                } else if (E_reboot_request == REBOOT_REQ_BOOTLOADER) {
                    E_reboot_request = REBOOT_REQ_NONE;
                    CyDelay(300);
                    Bootloadable_Load();
                }
                break;
            default:
                main_usb_vbus();
                beater = 0;
                beat = i;
            }
        }
            
    }
}

uint32 swap32(uint32 original) CYREENTRANT {
    uint8 *r, *o;
    uint32 ret;
    r = (void*)&ret;
    o = (void*)&original;
    r[0] = o[3];
    r[1] = o[2];
    r[2] = o[1];
    r[3] = o[0];
    return ret;
}

uint16 swap16(uint16 original) CYREENTRANT {
    uint8 *r, *o;
    uint16 ret;
    r = (void*)&ret;
    o = (void*)&original;
    r[0] = o[1];
    r[1] = o[0];
    return ret;
}
void ERROR(char* msg) {
    uint8 i, beat;
    uint16 timer = 0;
    
    if(USBFS_initVar) USBFS_Stop();
    Control_Write(Control_Read() & CONTROL_LED | CONTROL_AMP | CONTROL_RX);
    Morse_Main(msg);
    
    for(;;) {
        i = Status_Read() & STATUS_BEAT;
        if (beat != i) {
            beat = i;
            if (!timer--) {
                timer = 480; // 5 WPM
                Morse_Main(0);
            }
        }    
    }
}