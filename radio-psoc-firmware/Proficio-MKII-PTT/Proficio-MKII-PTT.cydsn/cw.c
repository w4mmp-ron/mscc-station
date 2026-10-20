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

/*
 * Non-QSK amp keying delay. AMP port goes on at first key down; the key lines are
 * sampled every 1 ms into key_delay_buf and played to the PA (CONTROL_RX) key_delay_ms late
 * so the amp relays settle before RF. Element lengths are unchanged.
 * key_delay_ms is latched from E_key_delay_ms (set from E_TX_Hold in main.c) at the start
 * of each over, so a change never alters an element mid-over.
 * Key_Delay_Timer: fixed-function, CW_Clock (10 kHz), period 10 -> 1 ms tc.
 * ISR uses the register macros, not Status_Read()/Control_Read()/Control_Write():
 * Keil 8051 functions are not reentrant and the main loop calls those.
 * Main loop Control_Write() read-modify-writes may briefly undo CONTROL_RX;
 * the ISR rewrites it every tick, so worst case is a 1 ms edge error.
 */
#define KEY_DELAY_BUF_SIZE 64u       /* ms of key history, power of 2, > KEY_DELAY_MAX_MS */
static uint8 key_delay_buf[KEY_DELAY_BUF_SIZE];
static volatile uint8 key_delay_idx = 0;
static volatile uint8 key_delay_ms = 0;        /* delay for this over */
static volatile uint8 key_up_ms = 0;           /* ms since last key down, stops at 255 */
static volatile uint8 key_delay_active = FALSE;
static volatile uint8 key_delay_busy = FALSE;   /* TRUE until delayed RF of last key down is done */
static volatile uint8 key_delay_lo_ready = FALSE; /* LO on TX freq. PA held off until TRUE */
volatile uint8 E_ms_tick = 0;                   /* free running 1 ms count, used by TX_Main() amp delay */

CY_ISR(KEY_DELAY_interrupt){
    uint8 reg;
    uint8 key;

    E_ms_tick++;
    if(!key_delay_active) return;
    if(TX_Inhibit || (E_host_mode != 'C')){ //Left CW or inhibited mid-over: PA off, stop playing
        Control_Control |= CONTROL_RX;
        key_delay_busy = FALSE;
        key_delay_active = FALSE;
        return;
    }
    reg = Status_Status;
    key = (!(reg & STATUS_KEY_0) || !(reg & STATUS_KEY_1));
    key_delay_buf[key_delay_idx] = key;
    if(key){
        key_up_ms = 0;
    }else if(key_up_ms < 255u){
        key_up_ms++;
    }
    reg = Control_Control;
    if(key_delay_lo_ready && key_delay_buf[(uint8)(key_delay_idx - key_delay_ms) & (KEY_DELAY_BUF_SIZE - 1u)]){
        reg &= ~CONTROL_RX;     //Turn ON PA - Negative logic level
    }else{
        reg |= CONTROL_RX;      //Turn OFF PA - Negative logic level
    }
    Control_Control = reg;
    key_delay_idx = (key_delay_idx + 1u) & (KEY_DELAY_BUF_SIZE - 1u);
    key_delay_busy = (key_up_ms <= key_delay_ms);
}

uint8 keyer_write(uint8_t buffer)
{
	uint8_t msg_buffer = 0;
	uint8_t ret_status = 0;
	uint8_t write_status = 0;
    uint8 buffer_written = 0;
    uint8 attempt;

    /*
     * PIC may NACK while busy (e.g. EEPROM write on STORE_END). Retry a few
     * times so a following PLAY is not dropped and keyer not marked missing.
     */
    msg_buffer = buffer;
    for (attempt = 0; attempt < 8u; attempt++) {
        write_status = I2C_DISPLAY_MasterWriteBuf(KEYER_SLAVE_ADDRESS, &msg_buffer, 1u,
            I2C_DISPLAY_MODE_COMPLETE_XFER);
        while ((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u) {
        }
        buffer_written = I2C_DISPLAY_MasterGetWriteBufSize();
        if (buffer_written == 1u) {
            ret_status = 1;
            break;
        }
        CyDelay(5u);
    }
    return ret_status;
}

uint8 Configure_CW(){
    static uint8_t state = 0;
    static uint8_t keyer_mode = 0;
    static uint8_t wpm = 0;
    static uint8_t spacing = 0;
    static uint8_t mem_text_wpm = 0;
    static uint8_t weight = 0;
    static uint8_t side_tone = 0;
    static uint8_t paddle = 0;
    static uint8_t buffer[2];
	static uint8_t send_state = 0;
    static uint8_t last_mem_seq = 0;
    uint8 write_status = 0;
    uint8_t p0, p1, s0, s1;

    /*
     * Always poll USB mem packet (even while I2C-sending state 10).
     * Double-read so we do not catch a half-updated [param,seq] mid-write.
     * Queue holds params until I2C can send (one 0x9C pair at a time).
     */
    p0 = E_keyer_mem_pkt[0];
    s0 = E_keyer_mem_pkt[1];
    p1 = E_keyer_mem_pkt[0];
    s1 = E_keyer_mem_pkt[1];
    if (s0 != 0 && s0 == s1 && p0 == p1 && s0 != last_mem_seq) {
        last_mem_seq = s0;
        if (E_keyer_mem_q_count < KEYER_MEM_Q_SIZE) {
            E_keyer_mem_q[E_keyer_mem_q_head] = p0;
            E_keyer_mem_q_head = (E_keyer_mem_q_head + 1) % KEYER_MEM_Q_SIZE;
            E_keyer_mem_q_count++;
        }
    }
           
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
            state++;
        }
        break;

        case 6:
        /* SET_MEM_TEXT_WPM 0x76 — push to PIC when host changes (PIC may no-op until Farnsworth) */
        if(mem_text_wpm != E_mem_text_wpm){
            buffer[0] = SET_MEM_TEXT_WPM;
            buffer[1] = E_mem_text_wpm;
            mem_text_wpm = E_mem_text_wpm;
            state = 10;
        }else{
            state++;
        }
        break;

        case 7:
        /* Start one queued CQ-memory I2C transfer if idle path reached here */
        if (E_keyer_mem_q_count > 0) {
            buffer[0] = CMD_SET_KEYER_MEMORY;
            buffer[1] = E_keyer_mem_q[E_keyer_mem_q_tail];
            E_keyer_mem_q_tail = (E_keyer_mem_q_tail + 1) % KEYER_MEM_Q_SIZE;
            E_keyer_mem_q_count--;
            state = 10;
        } else {
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
    uint8 i;
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
                if(E_QSK == FALSE){ //Non-QSK: AMP on and start recording the key now, before the LO change, so the first element is not clipped
                    for(i = 0; i < KEY_DELAY_BUF_SIZE; i++) key_delay_buf[i] = 0; //ISR not active yet so no race
                    key_delay_ms = E_key_delay_ms;
                    key_up_ms = 0;          //Key is down now
                    key_delay_busy = TRUE;
                    key_delay_lo_ready = FALSE;
                    key_delay_active = TRUE;
                    Control_Write(Control_Read() & ~CONTROL_AMP);   //Turn ON the AMP port - Negative logic level
                }
                state++;
            }else{
                state = 0;//Key is not down return to idle state
            }
            break;
        case 2: //A KEY is down and the CW Hold timer is not running
            Control_Write(Control_Read() & ~CONTROL_DOUT);          //Turn OFF output from PCM3060. Do NOT Receive the Audio
            SI5351_status = si5351aSetFrequency(CW_LO_Freq + E_cw_pitch_freq); //Set the LO to the CW TX frequency
            if(SI5351_status == 0){
                state++;   
            }
            break;
        case 3:
            control_status = Control_Read();
            if(E_QSK == TRUE){//The Potentia 50 / 100 is attached.  They are QSK. Turn on the AMP port and the PA now.
                control_status = control_status & ~CONTROL_AMP;     //Turn ON the AMP port - Negative logic level
                control_status = control_status & ~CONTROL_RX;      //Turn ON PA - Negative logic level. PA controls CW ON/OFF 
                E_cw_hold = TRUE;                                   /* hang timer starts on key-up (case 6) */
                state = 5;
            }else{  //Non-QSK amp. AMP on and ISR recording since case 1. LO is now on TX freq, let the ISR key the PA
                key_delay_lo_ready = TRUE;
                state = 20;
            }
            Control_Write(control_status);
            break;
        case 4:
            Control_Write(Control_Read() & ~CONTROL_RX);        //Turn ON PA - Negative logic level. PA controls CW ON/OFF 
            E_cw_hold = TRUE;                                   /* hang timer starts on key-up (case 6) */
            state++;                                            //KEY is DOWN and PA output is now ON
            break;
        case 5: //Now check if KEY is UP
            paddles_section = CyEnterCriticalSection();
            key = Status_Read();
            CyExitCriticalSection(paddles_section);
            if (key & STATUS_KEY_0)  E_key_0 = TRUE; else E_key_0 = FALSE;
            if (key & STATUS_KEY_1)  E_key_1 = TRUE; else E_key_1 = FALSE;
            if((E_key_0 && E_key_1)){ //If KEY is UP go to state 6.  If NOT then the KEY is still down and nothing changes.
                state++;
            }
            break;
        case 6: //KEY is UP — start semi-break-in hang (not on key-down)
            Control_Write(Control_Read() | CONTROL_RX);         //Turn OFF PA - Negative logic level
            E_cw_hold = TRUE;
            CW_Hold_Control_Write(CW_CONTROL_RESET);
            state++;
            break;
        case 7: //Check CW hold timer.  KEY is UP
            if(E_cw_hold == FALSE){ //KEY is UP and timer expired.
                SI5351_status = si5351aSetFrequency(CW_LO_Freq);    //Set the LO to the CW RX dial frequency
                if(SI5351_status == 0){
                    control_status = Control_Read();
                    control_status = control_status | CONTROL_AMP;  //Turn OFF the AMP port - Negative logic level
                    control_status = control_status | CONTROL_DOUT; //Turn ON output from PCM3060
                    Control_Write(control_status);
                    E_key_down = FALSE;
                    state = 0;                                   //Return to idle state
                }                                  
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
        case 20: //Non-QSK: ISR is keying the PA. Wait until key UP and delayed RF finished
            if(!key_delay_active){  //ISR stopped (left CW or TX_Inhibit)
                state = 22;
            }else if(!key_delay_busy){
                E_cw_hold = TRUE;   //Start semi-break-in hang from end of RF
                CW_Hold_Control_Write(CW_CONTROL_RESET);
                state = 21;
            }
            break;
        case 21: //Non-QSK: hang timer running, PA off, AMP still on
            if(!key_delay_active){
                state = 22;
            }else if(key_delay_busy){
                state = 20;         //KEY DOWN again during hang
            }else if(E_cw_hold == FALSE){
                key_delay_active = FALSE;
                state = 22;
            }
            break;
        case 22: //Non-QSK: back to RX, same as case 7
            Control_Write(Control_Read() | CONTROL_RX);         //Turn OFF PA - Negative logic level
            SI5351_status = si5351aSetFrequency(CW_LO_Freq);    //Set the LO to the CW RX dial frequency
            if(SI5351_status == 0){
                control_status = Control_Read();
                control_status = control_status | CONTROL_AMP;  //Turn OFF the AMP port - Negative logic level
                control_status = control_status | CONTROL_DOUT; //Turn ON output from PCM3060
                Control_Write(control_status);
                E_key_down = FALSE;
                state = 0;
            }
            break;
    }
}
  