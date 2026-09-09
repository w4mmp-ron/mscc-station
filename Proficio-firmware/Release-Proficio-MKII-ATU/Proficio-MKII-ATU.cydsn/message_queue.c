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
#include <iambino.h>
#include <usbvend.h>

//extern struct band_volume E_band_and_volume[10];

volatile uint8   ee_firmware_version,ee_iambic_type,ee_memory,ee_spacing,ee_paddle,
        ee_lag = 0,ee_semi_breakin,ee_tx_hold,ee_weight,ee_wpm,
        ee_iambic_tuning,ee_semi_control;
volatile uint8 ee_external_sound;
volatile uint8 ee_cw_mode;
volatile uint8 ee_iambic_mode = 0;
volatile uint8 ee_cw_interface_method = 0;

volatile uint8 ee_side_tone_freq = 60;
volatile uint8 ee_cw_message[16];

struct message {
  uint8_t length;
  uint8_t cw_message[111];
} message;

struct cfg {
  int8_t  mode;
  int8_t  memory;
  int8_t  spacing;
  float   weight;
  uint8_t paddle;
  uint8_t lag;
} cfg;

static uint8 pa_on = FALSE;  //Flag to indicate the state of the Power Amplifier Finals - On or Off
static long cfg_speed_micros;
static long tx_timer;
static uint8 cw_tx = 0;



double get_cw_delay(){
    double l_cw_delay_period;
    double element_time;
    
    //element_time = 1200 / (double)ee_wpm;
    element_time = (double)1200.0f / 18.0f;
  	l_cw_delay_period = (double)((double)ee_tx_hold * element_time * 10);
    if(l_cw_delay_period >= 49999.0){
        l_cw_delay_period = 49999.0;
    }
    return l_cw_delay_period;
}
    
void cfg_set_speed(float wpm) {
     cfg_speed_micros = ((long)((long)ee_iambic_tuning * RATIO_FACTOR) / (long)wpm);
}


uint8_t get_cw_command_from_host(){
    uint8_t ret = 0;
    if(E_command_queue_front == -1){
        ret = 0;
    }else{
        ret = E_command_queue[E_command_queue_front];
        E_command_queue[E_command_queue_front] = 0;
        if(E_command_queue_front == E_command_queue_rear){
            E_command_queue_front = E_command_queue_rear = -1;
        }else{
            E_command_queue_front = (E_command_queue_front + 1)%MAX_COMMAND_QUEUE;
        }
    }
    return ret;
}


void process_cw_command_from_host(){
    double l_cw_delay_period = 0.0;
    uint8_t l_cw_command;
   
    uint8 error = 0;
    reg8 *RegPointer;
    uint8 pcb_version;
 
    l_cw_command = get_cw_command_from_host();
	if(l_cw_command != FALSE){
        switch(l_cw_command){
            case CMD_SET_PCB_VERSION:
                RegPointer = (reg8 *) (CYDEV_EE_BASE + CYDEV_EEPROM_ROW_SIZE);
                pcb_version = RegPointer[EEPROM_PCB_VERSION_LOCATION];
                break;
 
            case SET_TX_HOLD:
                l_cw_delay_period = get_cw_delay();
                //CW_Hold_Timer_WritePeriod((uint16)l_cw_delay_period);
			    break;
		
            case SET_CW_MODE:
                switch(E_host_mode){
                    case 'C':
			            E_cw_mode = TRUE;
                        E_cw_message_toggle = TRUE;
                        break;
                    default:
                        E_cw_mode = FALSE;
                        E_cw_first_pass = FALSE;
                        E_cw_toggle = FALSE;
                        Control_Write(Control_Read() | CONTROL_CW);
                        Control_Write(Control_Read() & ~CONTROL_LED);
                }
			    break;
				
		    
		    case SET_CW_DEFAULTS:
			    if(E_cw_defaults == CFG_CW_DEFAULTS_ON){
				    set_cw_params(18.0,CFG_PADDLE_NORMAL,CFG_MODE_IAMBIC,CFG_SPACING_EL,
									CFG_WEIGHT_DIST,CFG_EXTERNAL_SOUND_ON,CFG_MEMORY_TYPE_B,
										FALSE,FALSE,FALSE,FALSE,FALSE,CFG_TX_HOLD_DEFAULT,CFG_SEMI_USE_ATU); 
				    get_cw_params();
                    Control_Write(Control_Read() & ~CONTROL_LED);   // Set the LED to steady on
			    }
			    break;                     	
		    }//End of Switch (E_cw_command)
    }//if(E_cw_command != FALSE)
}

void get_cw_params(){
    long l_cw_delay_period;
        
    l_cw_delay_period = get_cw_delay();
    //CW_Hold_Timer_WritePeriod((uint16)l_cw_delay_period);
  
    cfg_set_speed((float)ee_wpm);
    cfg.paddle = ee_paddle;
    cfg.mode = ee_iambic_type;
    cfg.spacing = ee_spacing;
    cfg.weight = (float)((float)ee_weight / 100);
    cfg.lag = ee_lag;
    cfg.memory = ee_memory;
}

void set_cw_params(float wpm,uint8 paddle,uint8 mode,uint8 spacing,float weight, uint8 l_side_tone_freq,
                                        uint8 memory,uint8 l_sound,uint8 l_cw_mode,uint8 l_iambic_mode,
                                        float l_iambic_tuning,uint8 l_semi_breakin,uint8 l_tx_hold,
                                        uint8 l_semi_control)
{
    ee_paddle = paddle;
    ee_iambic_type = mode;
    ee_spacing = spacing;
    ee_side_tone_freq = l_side_tone_freq;
    ee_memory = memory;
    ee_external_sound = l_sound;
    ee_cw_mode = l_cw_mode;
    ee_iambic_mode = l_iambic_mode;
    ee_semi_breakin = l_semi_breakin;
    ee_tx_hold = l_tx_hold;
    ee_wpm = (uint8) wpm;
    ee_weight = (uint8)(weight * 100);
    ee_iambic_tuning = (uint8) (l_iambic_tuning / RATIO_FACTOR);
    ee_semi_control = l_semi_control;
}


/*void tx_send(long mark) {
    uint8 cw_timer_status;
       
    if(!pa_on){
        if(ee_semi_breakin){
            cw_timer_status = cw_timer(CW_TIMER_RESET_START);
            E_key_down = TRUE;
        }
        Control_Write(Control_Read() & ~CONTROL_CW); 
        if(ee_external_sound) Control_Write(Control_Read() | CONTROL_ATU_1); //Turn on the external sound oscillator
        pa_on = TRUE;
    }
  tx_timer = mark;
}

uint8 tx_loop(long mark) {
    static uint8 status = FALSE;
    
    if(pa_on){
        if(tx_timer - mark < 0){
            Control_Write(Control_Read() | CONTROL_CW);
            if(ee_external_sound)Control_Write(Control_Read() & ~CONTROL_ATU_1); //Turn off the external sound oscillator
            pa_on = FALSE;
        }
        if(pa_on == FALSE) {
            status = TRUE;
        }else{
            status = FALSE;
        }
    }
    return status;
}

uint8_t key_read() {
    uint8_t k0,k1;
    
    k0 = E_key_0 ^ 1;
    k1 = E_key_1 ^ 1;
  
    if (cfg.mode == CFG_MODE_STRAIGHT) {
        k0 <<= 1;
        k1 = 0;
    } else if (cfg.paddle == CFG_PADDLE_NORMAL) {
        k1 <<= 1;
    } else {
        k0 <<= 1;
    }
    return (k0|k1);
}

uint8 key_loop(long mark) {
  static uint8_t last, spacing=2, ultimatic, state=3, staged=0, mcode=0x80;
  static long read_after, start_after;
  uint8 k0,k1,ret = 0;
  long i;
  
  k0 = key_read();
  k1 = k0 & 2;
  k0 = k0 & 1;
  
  switch(state) {
  case 1: // waiting until ready for read
    if (cfg.spacing == CFG_SPACING_NONE)
      if ((k0 && last == DIT) || (k1 && last == DAH))
        read_after = mark //+ KEY_DEBOUNCE_IAMBIC
    if (read_after - mark < 0) state = 2;
    break;
  case 2: // waiting and reading
    if (start_after - mark < 0) state = 3;
    if (spacing < 4) break;
    //nobreak;
  case 3: // idle, spacing
    if (start_after - mark < 0) {
      switch (spacing) {
      case 0:
      case 2:
      case 3:
        break;
      case 1:
        ret = mcode;
        mcode=0x80;
        if (cfg.spacing >= CFG_SPACING_CHAR) state = 2;
        break;
      case 4:
        ret = mcode;
        //nobreak
      case 5:
      case 6:
        if (cfg.spacing >= CFG_SPACING_WORD) state = 2;
        break;
      }
      if (spacing < 7) spacing += 1;
      if (cfg.mode == CFG_MODE_BUG) state = 3;
      start_after += DIT * cfg_speed_micros;
    }
    break;
  case 4: // debouncing straight/bug down
    if (start_after - mark < 0) state = 5;
    break;
  case 5: // holding straight/bug
    break;
  case 6: // debouncing straight/bug up
    if (read_after - mark < 0) {
      state = 3;
      staged = 0;
      start_after = mark + DIT * cfg_speed_micros;
      spacing = 0;
      if (mcode & 0x01) {
        mcode = 0xFF;
      } else {
        mcode >>= 1;
        mcode |= 0x80;
      }
    }
    break;
  }

  if (cfg.mode == CFG_MODE_STRAIGHT || cfg.mode == CFG_MODE_BUG) {
    if (k1) {
      i = mark //+ KEY_DEBOUNCE_SRAIGHT
      if (state < 4) {
        state = 4;
        start_after = i;
      }
      if (state < 6) {
        read_after = i;
        tx_send(i);
        ret = TRUE;
      }
      last = DAH;
      staged = 0;
    } 
    else if (state == 5) {
      if (staged == DIT) {
        state = 3;
      } else {
        state = 6;
        tx_send(mark);
        ret = TRUE;
      }
    }
  } else {
    if (state > 3) state = 6;
  }
  
  if (!staged) {
    if (state > 1) {
      if (k0 && k1) {
        if (ultimatic && cfg.mode == CFG_MODE_ULTIMATIC) staged = last;
        else if (last == DIT) staged = DAH;
        else staged = DIT;
        ultimatic = 1;
      } else {
        if (k0) staged = DIT;
        if (k1) staged = DAH;
        ultimatic = 0;
      }
    }
    else if (!ultimatic || cfg.mode != CFG_MODE_ULTIMATIC) {
      if (k0 && (last == DAH || spacing > 0)) {
        if (cfg.memory & CFG_MEMORY_TYPE_DIT) {
          staged = DIT;
          ultimatic = 1;
        }
      }
      if (k1 && (last == DIT || spacing > 0)) {
        if (cfg.memory & CFG_MEMORY_TYPE_DAH) {
          staged = DAH;
          ultimatic = 1;
        }
      }
    }
  }
  
  if (state == 3 && staged) {
    i = mark + (long)staged * cfg_speed_micros;
    i += DIT * cfg_speed_micros * (cfg.weight * 2 - 1);
    read_after = start_after = i + (long)cfg.lag * 1000;
    tx_send(start_after);
    ret = TRUE;
    i += DIT * cfg_speed_micros * (2.0 - cfg.weight * 2);
    if (cfg.spacing >= CFG_SPACING_EL) {
      read_after = i //- KEY_DEBOUNCE_IAMBIC
      start_after = i;
    }
    spacing = 0;
    if (mcode & 0x01) {
      if (mcode != 0x01 || staged==DAH) mcode = 0xFF;
    } else {
      mcode >>= 1;
      if (staged==DAH) mcode |= 0x80;
    }
    last = staged;
    staged = 0;
    state = 1;
  }

  if (cfg.mode == CFG_MODE_STRAIGHT) return 0;
  return ret;
}

*/

/*void iambic(){
    long mark;
    uint8_t key_down = FALSE;
    uint8 tx_loop_status;
    uint8 cw_timer_status;
            
    mark = Iambic_Counter_ReadCounter();
    key_down = key_loop(mark);
    key_down = key_down; //Makes the compiler happy
    tx_loop_status = tx_loop(mark);
    if(ee_semi_breakin){
        if(tx_loop_status){
            cw_timer_status = cw_timer(CW_TIMER_CHECK);
            if(cw_timer_status == CW_TIMER_EXPIRED){ 
                //Control_Write((Control_Read()) & ~CONTROL_AMP);
                E_key_down = FALSE;
            }
        }
    }
}
*/
  




