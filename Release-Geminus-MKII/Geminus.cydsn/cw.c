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

/*
 * Geminus-MKII CW (relay T/R + internal PIC keyer):
 *  - CONTROL_RX / CONTROL_AMP / CONTROL_BAND_TX (BS2): latched for the whole
 *    semi-break-in session. Avoids relay chatter and wear.
 *  - CONTROL_DIN: element keying — I/Q into PCM3060 (mark); clear blanks I/Q.
 *  - CONTROL_DOUT: mute RX audio for the session.
 * PIN-diode Proficio-MKII keyed CONTROL_RX per element; that is wrong here.
 */

#define CW_ST_IDLE          0
#define CW_ST_WAIT_KEY      1
#define CW_ST_SESSION_START 2
#define CW_ST_MARK          3
#define CW_ST_MARK_HOLD     5
#define CW_ST_SPACE         6
#define CW_ST_HANG          7
#define CW_ST_HANG_POLL     10

static uint8_t cw_keys_down(void)
{
    uint8 key;
    uint8 section;

    section = CyEnterCriticalSection();
    key = Status_Read();
    CyExitCriticalSection(section);
    if (key & STATUS_KEY_0)
        E_key_0 = TRUE;
    else
        E_key_0 = FALSE;
    if (key & STATUS_KEY_1)
        E_key_1 = TRUE;
    else
        E_key_1 = FALSE;
    return (uint8_t)(!E_key_0 || !E_key_1);
}

static uint8 cw_session_start(void)
{
    uint8_t c;

    c = Control_Read();
    c = c & ~CONTROL_DOUT;   /* PCM3060 RX out OFF */
    c = c & ~CONTROL_DIN;    /* I/Q blank until mark */
    Control_Write(c);
    return si5351aSetFrequency(CW_LO_Freq + E_cw_pitch_freq);
}

static void cw_mark_on(void)
{
    uint8_t c;

    c = Control_Read();
    c = c & ~CONTROL_AMP;    /* AMP ON (active low) */
    c = c & ~CONTROL_RX;     /* PA / T-R ON (active low) */
    c = c | CONTROL_DIN;     /* I/Q on → RF */
    Control_Write(c);
    Band_Control_Write(Band_Control_Read() | CONTROL_BAND_TX); /* Geminus BS2 TX */
    E_cw_hold = TRUE;
}

static void cw_mark_off(void)
{
    Control_Write(Control_Read() & ~CONTROL_DIN);
    E_cw_hold = TRUE;
    CW_Hold_Control_Write(CW_CONTROL_RESET);
}

static uint8 cw_session_end(void)
{
    uint8_t c;
    uint8 st;

    st = si5351aSetFrequency(CW_LO_Freq);
    if (st != 0)
        return st;
    Band_Control_Write(Band_Control_Read() & ~CONTROL_BAND_TX);
    c = Control_Read();
    /* AMP off, PA off, RX audio on, restore I/Q path for later SSB */
    c = c | CONTROL_AMP | CONTROL_RX | CONTROL_DOUT | CONTROL_DIN;
    Control_Write(c);
    E_key_down = FALSE;
    return 0;
}

void Manage_Paddles_Port(void)  
{
    static uint8_t state = CW_ST_IDLE;
    static uint32 previous_CW_LO_Freq = 0;
    
    switch(state){
        case CW_ST_IDLE:
            if (previous_CW_LO_Freq != CW_LO_Freq) {
                SI5351_status = si5351aSetFrequency(CW_LO_Freq);
                if (SI5351_status == 0)
                    previous_CW_LO_Freq = CW_LO_Freq;
                else
                    break;
            }
            if (!TX_Inhibit && (E_host_mode == 'C'))
                state = CW_ST_WAIT_KEY;
            break;
        case CW_ST_WAIT_KEY:
            if (cw_keys_down()) {
                E_key_down = TRUE;
                state = CW_ST_SESSION_START;
            } else {
                state = CW_ST_IDLE;
            }
            break;
        case CW_ST_SESSION_START:
            SI5351_status = cw_session_start();
            if (SI5351_status == 0)
                state = CW_ST_MARK;
            break;
        case CW_ST_MARK:
            cw_mark_on();
            state = CW_ST_MARK_HOLD;
            break;
        case CW_ST_MARK_HOLD:
            if (!cw_keys_down())
                state = CW_ST_SPACE;
            break;
        case CW_ST_SPACE:
            cw_mark_off();
            state = CW_ST_HANG;
            break;
        case CW_ST_HANG:
            if (E_cw_hold == FALSE) {
                SI5351_status = cw_session_end();
                if (SI5351_status == 0)
                    state = CW_ST_IDLE;
            } else {
                state = CW_ST_HANG_POLL;
            }
            break;
        case CW_ST_HANG_POLL:
            if (cw_keys_down())
                state = CW_ST_MARK;
            else
                state = CW_ST_HANG;
            break;
    }
}
