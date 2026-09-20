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
    uint8 iambic_section;
    uint8 eeprom_value = 0;
    uint8 eeprom_byte_position = 0;
    uint8 write_eeprom = TRUE;
    //uint8 cw_control_reg;
    
    error = error;//Keep compiler happy
    l_cw_command = get_cw_command_from_host();
	if(l_cw_command != FALSE){
        switch(l_cw_command){
            case SET_TX_HOLD:
	            l_cw_delay_period = (1969 *  (double)ee_tx_hold / (double)ee_wpm);
                CW_Hold_Timer_WritePeriod((uint16)l_cw_delay_period);
			    eeprom_value = ee_tx_hold;
			    eeprom_byte_position = EE_TX_HOLD_REGISTER;
			    break;
				
		    case SET_CW_PADDLE:
			    cfg.paddle = ee_paddle;
			    eeprom_value = ee_paddle;
			    eeprom_byte_position = EE_PADDLE_REGISTER;
			    break;
                
            case SET_SIDE_TONE:
			    eeprom_value = ee_external_sound;
			    eeprom_byte_position = EE_EXTERNAL_SOUND_REGISTER;
			    break;
                
           case SET_CW_MODE:
                write_eeprom = FALSE;
                switch(E_host_mode){
                    case 'C':
			            E_cw_mode = TRUE;
                        E_cw_message_toggle = TRUE;
                        break;
                    default:
                        E_cw_mode = FALSE;
                        //cw_control_reg = CW_Control_Read();
                        //cw_control_reg = cw_control_reg & ~CW_IQ_INTERNAL;
                        //CW_Control_Write(cw_control_reg);
                        Control_Write(Control_Read() & ~CONTROL_LED);
                }
			    break;
				
		    case SET_WPM:
			    l_cw_delay_period = (1969 *  (double)ee_tx_hold / ee_wpm);
			    CW_Hold_Timer_WritePeriod((uint16)l_cw_delay_period);
			    cfg_set_speed((float)ee_wpm);
			    eeprom_value = ee_wpm;
			    eeprom_byte_position = EE_WPM_REGISTER;
			    break;    
				
		    case SET_IAMBIC_TYPE:
			    cfg.mode = ee_iambic_type;
			    eeprom_value = ee_iambic_type;
			    eeprom_byte_position = EE_KEY_TYPE_REGISTER;
			    break;        
				
		    case SET_SPACING:
			    cfg.spacing = ee_spacing;
			    eeprom_value = ee_spacing;
			    eeprom_byte_position = EE_SPACING_REGISTER;
			    break;          
				
		    case SET_WEIGHT:
			    cfg.weight = (float)((float)ee_weight / 100);
			    eeprom_value = ee_weight;
			    eeprom_byte_position = EE_WEIGHT_REGISTER;
			    break;            
				
		    case SET_MEMORY_TYPE:
			    cfg.memory = ee_memory;
			    eeprom_value = ee_memory;
			    eeprom_byte_position = EE_MEMORY_REGISTER;
			    break;        
				
		    case SET_CW_RECORD_MESSAGE:
                write_eeprom = FALSE;
			    //cw_record_message();
			    break;             
                
            case SET_CW_PLAY_MSG:
                write_eeprom = FALSE;
			    //cw_play_message(E_cw_play_msg);
			    break;             
				
            case SET_CW_STOP_MSG:
                write_eeprom = FALSE;
			    //cw_play_message(0xff);
			    break;             
                
		    case SET_IAMBIC_MODE:  //Turn Iambic mode on / off
			    eeprom_value = ee_iambic_mode;
			    eeprom_byte_position = EE_IAMBIC_MODE_REGISTER;
                E_cw_message_toggle = TRUE;
                break;                  
						
		    case SET_SEMI_BREAKIN:
			    eeprom_value = ee_semi_breakin;
			    eeprom_byte_position = EE_SEMI_BREAKIN_REGISTER;
                E_cw_message_toggle = TRUE;
                break;            
				
		    case SET_IAMBIC_TUNING:
			    eeprom_value = ee_iambic_tuning;
			    cfg_set_speed((float)ee_wpm);
			    eeprom_byte_position = EE_IAMBIC_TUNING_REGISTER;
			    break;            
				
		    case SET_SEMI_CONTROL:
			    eeprom_value = ee_semi_control;
			    eeprom_byte_position = EE_SEMI_CONTROL_REGISTER;
			    break;   
    
		    case SET_CW_DEFAULTS:
			    if(E_cw_defaults == CFG_CW_DEFAULTS_ON){
				    set_params(18.0,CFG_PADDLE_NORMAL,CFG_MODE_IAMBIC,CFG_SPACING_EL,
									CFG_WEIGHT_DIST,CFG_EXTERNAL_SOUND_ON,CFG_MEMORY_TYPE_B,
										FALSE,FALSE,FALSE,FALSE,FALSE,CFG_TX_HOLD_DEFAULT,CFG_SEMI_USE_ATU); 
				    get_cw_params();
                    write_eeprom = FALSE;
				    Control_Write(Control_Read() & ~CONTROL_LED);   // Set the LED to steady on
			    }
			    break;                     	
		    }//End of Switch (E_cw_command)
            if(write_eeprom){
                iambic_section = CyEnterCriticalSection(); 
                EEPROM_UpdateTemperature();
                error = EEPROM_ByteWrite(eeprom_value,1u,eeprom_byte_position);
                CyExitCriticalSection(iambic_section);
            }
        //E_cw_command = FALSE;
    }//if(E_cw_command != FALSE)
}

void get_cw_params(void){
    reg8 *RegPointer;
    double l_cw_delay_period;
            
    //Get the address of the Second EEPROM row
    RegPointer = (reg8 *) (CYDEV_EE_BASE + CYDEV_EEPROM_ROW_SIZE);
        
    ee_firmware_version = RegPointer[EE_FIRMWARE_VERSION];
    if(ee_firmware_version != FIRMWARE_VERSION_MINOR){
        set_params(18.0,CFG_PADDLE_NORMAL,CFG_MODE_IAMBIC,CFG_SPACING_EL,CFG_WEIGHT_DIST,
                                                            CFG_EXTERNAL_SOUND_ON,CFG_MEMORY_TYPE_B,
                                                                      FALSE,FALSE,FALSE,FALSE,FALSE,
                                                                           CFG_TX_HOLD_DEFAULT,CFG_SEMI_USE_ATU); 
    }
        
    ee_paddle = RegPointer[EE_PADDLE_REGISTER];
    ee_iambic_type = RegPointer[EE_KEY_TYPE_REGISTER];
    ee_spacing = RegPointer[EE_SPACING_REGISTER];
    ee_side_tone_freq = RegPointer[EE_SIDE_TONE_FREQ_REGISTER];
    ee_memory = RegPointer[EE_MEMORY_REGISTER];
    ee_external_sound = RegPointer[EE_EXTERNAL_SOUND_REGISTER];
    ee_cw_mode = RegPointer[EE_CW_MODE_REGISTER];
    ee_iambic_mode = RegPointer[EE_IAMBIC_MODE_REGISTER];
    ee_semi_breakin = RegPointer[EE_SEMI_BREAKIN_REGISTER];
    ee_tx_hold = RegPointer[EE_TX_HOLD_REGISTER];
    ee_wpm = RegPointer[EE_WPM_REGISTER];
    ee_weight = RegPointer[EE_WEIGHT_REGISTER];
    ee_iambic_tuning = RegPointer[EE_IAMBIC_TUNING_REGISTER];
    ee_semi_control = RegPointer[EE_SEMI_CONTROL_REGISTER];
    
  
    l_cw_delay_period = (1969 *  (double)ee_tx_hold / (double)ee_wpm);
    CW_Hold_Timer_WritePeriod((uint16)l_cw_delay_period);
  
    cfg_set_speed((float)ee_wpm);
    cfg.paddle = ee_paddle;
    cfg.mode = ee_iambic_type;
    cfg.spacing = ee_spacing;
    cfg.weight = (float)((float)ee_weight / 100);
    cfg.lag = ee_lag;
    cfg.memory = ee_memory;
}
//set_params(18.0,CFG_PADDLE_NORMAL,CFG_MODE_IAMBIC,CFG_SPACING_EL,
//									CFG_WEIGHT_DIST,CFG_SIDE_TONE_FREQ_600,CFG_MEMORY_TYPE_B,
//										FALSE,FALSE,FALSE,FALSE,FALSE,CFG_TX_HOLD_DEFAULT,CFG_SEMI_USE_ATU); 

void set_params(float wpm,uint8 paddle,uint8 mode,uint8 spacing,float weight, uint8 l_side_tone_freq,
                                        uint8 memory,uint8 l_sound,uint8 l_cw_mode,uint8 l_iambic_mode,
                                        float l_iambic_tuning,uint8 l_semi_breakin,uint8 l_tx_hold,
                                        uint8 l_semi_control)
{
    uint8 error;
    
    //uint8 is_written = 1;
    uint8 iambic_section;
   
    EEPROM_UpdateTemperature();
    iambic_section = CyEnterCriticalSection();  
         
    // This writes variable data to EEPROM
    error = EEPROM_ByteWrite(FIRMWARE_VERSION_MINOR,1u,EE_FIRMWARE_VERSION);
    error = EEPROM_ByteWrite(paddle,1u,EE_PADDLE_REGISTER);
    error = EEPROM_ByteWrite(mode,1u,EE_KEY_TYPE_REGISTER);
    error = EEPROM_ByteWrite(spacing,1u,EE_SPACING_REGISTER);
    error = EEPROM_ByteWrite(l_side_tone_freq,1u,EE_SIDE_TONE_FREQ_REGISTER);
    error = EEPROM_ByteWrite(memory,1u,EE_MEMORY_REGISTER);
    error = EEPROM_ByteWrite(l_sound,1u,EE_EXTERNAL_SOUND_REGISTER);
    error = EEPROM_ByteWrite(l_cw_mode,1u,EE_CW_MODE_REGISTER);
    error = EEPROM_ByteWrite(l_iambic_mode,1u,EE_IAMBIC_MODE_REGISTER);
    error = EEPROM_ByteWrite(l_semi_breakin,1u,EE_SEMI_BREAKIN_REGISTER);
    error = EEPROM_ByteWrite(l_semi_control,1u,EE_SEMI_CONTROL_REGISTER);
    error = EEPROM_ByteWrite(l_tx_hold,1u,EE_TX_HOLD_REGISTER);
    error = EEPROM_ByteWrite(((uint8) (weight * 100)),1u,EE_WEIGHT_REGISTER);
    error = EEPROM_ByteWrite(((uint8)(wpm)),1u,EE_WPM_REGISTER);
    error = EEPROM_ByteWrite(((uint8) (l_iambic_tuning / RATIO_FACTOR)),1u,EE_IAMBIC_TUNING_REGISTER);
       
    CyExitCriticalSection(iambic_section);
    error = error;
   
}


void tx_send(long mark) {
    uint8 cw_timer_status;
       
    //if(ee_semi_breakin){
      //  Control_Write((Control_Read()) | CONTROL_AMP);
        //cw_timer_status = cw_timer(TRUE);
    //}
 
    if(!pa_on){
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
        read_after = mark /*+ KEY_DEBOUNCE_IAMBIC*/;
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
      i = mark /*+ KEY_DEBOUNCE_SRAIGHT*/;
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
      read_after = i /*- KEY_DEBOUNCE_IAMBIC*/;
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

uint8_t message_play() {
    static uint8_t /*message_index,*/ message_pos, message_char, message_char_cnt;
    static uint8_t state = 0;
    static long wait;
    long i,mark;
    uint8_t ret = 0;
  
    mark = Iambic_Counter_ReadCounter();
    if (!message_char_cnt) {
        if (message_pos >= message.length) return 1;
        if (!message_pos) wait = mark;
        ret = message_char = message.cw_message[message_pos];
        message_pos++;
        if (!message_char) {
        message_char_cnt = 1;
        } else {
            message_char_cnt = 8;
            while (!(message_char & 0x01)) {
            message_char >>= 1;
            message_char_cnt -= 1;
        }
        }
    }
  
    if (wait - mark < 0) switch(state) {
    case 0:
        state = 1;
        if (message_char_cnt == 1) {
            wait = mark + 2 * DIT * cfg_speed_micros;
            break;
        }
    //nobreak;
    case 1:
        message_char >>= 1;
        message_char_cnt -= 1;
        if (message_char_cnt) {
            if (message_char & 0x01) i = mark + DAH * cfg_speed_micros;
                else i = mark + DIT * cfg_speed_micros;
            i += DIT * cfg_speed_micros * (cfg.weight * 2 - 1);
            tx_send(i + (long)cfg.lag * 1000);
            wait = i + DIT * cfg_speed_micros * (2.0 - cfg.weight * 2);
        } else {
            wait = mark + 2 * DIT * cfg_speed_micros;
            if (message_pos >= message.length) ret = 0x80;
        }
        if (!message_char_cnt) state = 0;
        break;
    }
    return ret;
}


void iambic(){
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
            cw_timer_status = cw_timer(FALSE);
            if(cw_timer_status){ 
                Control_Write((Control_Read()) & ~CONTROL_AMP);
            }
        }
    }
}
  




