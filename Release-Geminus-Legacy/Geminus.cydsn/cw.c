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

#include <STDLIB.H>
#include <STDIO.H>
#include <basic-plus.h>
#include <usbvend.h>
#include <iambino.h>
#include <si5351a.h>

#define KEYER_SLAVE_ADDRESS				0x40
#define CW_CONTROL_RESET 1

static uint8_t hold_time = CW_DEFAULT_HOLD_TIME;
static uint8_t QSK_pop_filter = FALSE;
static uint8_t E_new_session = FALSE;
uint8 TX_State = 0;
uint8 TX_Phase = TX_PHASE_IQTONE_RAMP_UP;
uint32 CW_LO_Freq = 0;
uint8_t SI5351_status = 0;


CY_ISR (CW_interrupt){
    E_cw_hold = FALSE;
}

CY_ISR (KEY_interrupt){
    //E_cw_hold = FALSE;
}

CY_ISR(QSK_interrupt){
    QSK_pop_filter = FALSE;
}

uint8 keyer_write(uint8_t buffer)
{
	uint8_t msg_buffer = 0;
	uint8_t ret_status = 1;
	uint8_t write_status = 0;
    uint8 buffer_written = 0;
        
	msg_buffer = buffer;
	write_status = I2C_DISPLAY_MasterWriteBuf(KEYER_SLAVE_ADDRESS,&msg_buffer,1u,I2C_DISPLAY_MODE_COMPLETE_XFER);
    while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
    buffer_written = I2C_DISPLAY_MasterGetWriteBufSize();
    if(buffer_written != (1u)) {
        ret_status = 0;
    }
    return ret_status;
}

uint8 Configure_CW(){
    static uint8_t state = 0;
    static uint8_t keyer_mode = 0;
    static uint8_t wpm = 0;
    static uint8_t spacing = 0;
    static uint8_t weight = 0;
    static uint8_t side_tone = 0;
    static uint8_t paddle = 0;
    static uint8_t buffer[2];
	static uint8_t send_state = 0;
    uint8 write_status = 0;
           
    switch(state){
        case 0:
        if(keyer_mode != E_keyer_mode){
            buffer[0] = SET_KEYER_MODE;
            buffer[1] = E_keyer_mode;
            keyer_mode = E_keyer_mode;
            state = 10;
        }else{
            state++;
        }
        break;
        
        case 1:
        if(paddle != E_paddle){
            buffer[0] = SET_CW_PADDLE;
            buffer[1] = E_paddle;
            paddle = E_paddle;
            state = 10;
        }else{
            state++;
        }
        break;
        
        case 2:
        if(spacing != E_spacing){
            buffer[0] = SET_SPACING;
            buffer[1] = E_spacing;
            spacing = E_spacing;
            state = 10;
        }else{
            state++;
        }
        break;
        
        case 3:
        if(weight != E_weight){
            buffer[0] = SET_WEIGHT;
            buffer[1] = E_weight;
            weight = E_weight;
            state = 10;
        }else{
            state++;
        }
        break;
        
        case 4:
        if(side_tone != E_side_tone){    //E_cw_pitch is an index for the cw pitch frequency.
            buffer[0] = SET_SIDE_TONE;  
            buffer[1] = E_side_tone;     //The Keyer uses this index to set the side tone frequency.
            side_tone = E_side_tone;
            state = 10;
        }else{
            state++;
        }
        break;
        
        case 5:
        if(wpm != E_wpm){
            buffer[0] = SET_WPM;
            buffer[1] = E_wpm;
            wpm = E_wpm;
            state = 10;
        }else{
            state = 0;
        }
        break;
        
        case 10:
        switch(send_state){
            case 0:
            write_status = keyer_write(buffer[0]);
            if(write_status == 1){
                send_state++;
                state = 10;
            }else{
                state = 0;
                send_state = 0;
                //ERROR("K  ");
                E_keyer_installed = FALSE;
            }
            break;
            
            case 1:
            write_status = keyer_write(buffer[1]);
            if(write_status == 1){
                send_state = 0;;
                state = 0;
            }else{
                state = 0;
                send_state = 0;
                //ERROR("K  ");
                E_keyer_installed = FALSE;
            }
            break;
        }
        break;
        
        default:
            break;
    }
    return state;
}

void Manage_Paddles_Port(void)  
{
    uint8 key;
    uint8 paddles_section;
    static uint8_t state = 0;
    uint8_t control_status = 0;
    static uint32 previous_CW_LO_Freq = 0;
    
    switch(state){  //When in CW mode Manage_Paddles_Port manages the LO freq.
        case 0:     //Idle state
            if(previous_CW_LO_Freq != CW_LO_Freq){
                SI5351_status = si5351aSetFrequency(CW_LO_Freq);
                if(SI5351_status == 0){
                    previous_CW_LO_Freq = CW_LO_Freq;
                    if(!TX_Inhibit && (E_host_mode == 'C')){ //Do not process the CW KEY if TX_Inhibit active and NOT in CW mode
                        state++; //Now check for key down
                    }
                }
            }else{
                if(!TX_Inhibit && (E_host_mode == 'C')){ //Do not process the CW KEY if TX_Inhibit active and NOT in CW mode
                    state++; //CW_LO_Freq did not change. Now check for key down
                }
            }            
            break;
        case 1: //Check for KEY DOWN
            paddles_section = CyEnterCriticalSection();
            key = Status_Read();
            CyExitCriticalSection(paddles_section);
            if (key & STATUS_KEY_0)  E_key_0 = TRUE; else E_key_0 = FALSE;
            if (key & STATUS_KEY_1)  E_key_1 = TRUE; else E_key_1 = FALSE;
            if(!E_key_0 || !E_key_1){ //Key is DOWN   
                E_key_down = TRUE;
                state++;
            }else{
                state = 0;//Key is not down return to idle state
            }
            break;
        case 2: //A KEY is down and the CW Hold timer is not running
            Control_Write(Control_Read() & ~CONTROL_DOUT);          //Turn OFF output from PCM3060. Do NOT Receive the Audio
            state++;
            break;
        case 3:
            control_status = Control_Read();
            if(E_QSK == TRUE){//The Potentia 50 / 100 is attached.  They are QSK. Turn on the AMP port and the PA now.
                control_status = control_status & ~CONTROL_AMP;     //Turn ON the AMP port - Negative logic level
                control_status = control_status & ~CONTROL_RX;      //Turn ON PA - Negative logic level. PA controls CW ON/OFF 
                E_cw_hold = TRUE;                                   //CW_Hold_Control will set this to FALSE when the timer expires
                CW_Hold_Control_Write(CW_CONTROL_RESET);            //Reset and start CW hold timer
                state = 5;
            }else{
                control_status = control_status & ~CONTROL_AMP;     //Turn ON the AMP port - Negative logic level
                state = 4;
            }
            Control_Write(control_status);
            break;
        case 4:
            Control_Write(Control_Read() & ~CONTROL_RX);        //Turn ON PA - Negative logic level. PA controls CW ON/OFF 
            E_cw_hold = TRUE;                                   //CW_Hold_Control will set this to FALSE when the timer expires
            CW_Hold_Control_Write(CW_CONTROL_RESET);            //Reset and start CW hold timer
            state++;                                            //KEY is DOWN and PA output is now ON
            break;
        case 5: //Now check if KEY is UP
            paddles_section = CyEnterCriticalSection();
            key = Status_Read();
            CyExitCriticalSection(paddles_section);
            if (key & STATUS_KEY_0)  E_key_0 = TRUE; else E_key_0 = FALSE;
            if (key & STATUS_KEY_1)  E_key_1 = TRUE; else E_key_1 = FALSE;
            if((E_key_0 && E_key_1)){ //KEY is UP.  If NOT then the KEY is still down and nothing changes.
                state++;
            }
            break;
        case 6: //KEY is UP
            Control_Write(Control_Read() | CONTROL_RX);         //Turn OFF PA - Negative logic level
            state++;
            break;
        case 7: //Check CW hold timer.  KEY is UP
            if(E_cw_hold == FALSE){ //KEY is UP and timer expired.
                control_status = Control_Read();
                control_status = control_status | CONTROL_AMP;  //Turn OFF the AMP port - Negative logic level
                control_status = control_status | CONTROL_DOUT; //Turn ON output from PCM3060
                Control_Write(control_status);
                state = 0;                                      //Return to idle state
            }else{
                state = 10;//KEY is UP but timer is still running. Check for KEY DOWN
            }
            break;
        case 10: //Timer is still running but the KEY is UP.  Check for KEY DOWN
            paddles_section = CyEnterCriticalSection();
            key = Status_Read();
            CyExitCriticalSection(paddles_section);
            if (key & STATUS_KEY_0)  E_key_0 = TRUE; else E_key_0 = FALSE;
            if (key & STATUS_KEY_1)  E_key_1 = TRUE; else E_key_1 = FALSE;
            //Check if KEY is down
            if(!E_key_0 || !E_key_1){ //Key is DOWN
                state = 4; //KEY is DOWN. TURN ON PA in state 4
            }else{
                state = 7; //KEY is UP.  Check timer state;
            }
            break;
    }
}
  