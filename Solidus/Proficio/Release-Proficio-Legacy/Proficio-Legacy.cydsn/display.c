/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
/*#include "basic-plus.h"
//#include "CharLCD_I2C.h"

#define INITIALIZE_COUNT 300000
#define OPERATION_COMPLETED 3
#define OPERATION_PENDING 2 
#define OPERATION_STARTED 1
#define OPERATION_IDLE 0
#define OPERATION_FAILED 4
#define FREQUENCY_POSITION 3
#define MODE_POSITION 14
#define STEP_POSITION 15

#define NONE  0
#define STEP  1
#define PTT  2
#define TUNE  3
#define MODE  4
#define RIT  5
#define BAND  6
#define VOLUME 7

#define KNOB 0
#define BUTTON_A 1
#define BUTTON_B 2
#define BUTTON_C 3  
#define BUTTON_A_POSITION 0
#define BUTTON_B_POSITION 7
#define BUTTON_C_POSITION 12

//int rem;
//int start;
//int end;
static char freq_display[17] = {0};
uint8 display_size = sizeof(freq_display);
static char previous_freq_display[17] = {0};
static char mode_string[4][4] = {"A","L","U","C"};

char button_A[13][4] = {"NON","STP","PPT","TUN","MOD","ROF","BND","F/V","CBW","HBW","RIT","FAV","VOL"};
char button_B[13][4] = {"NON","STP","PPT","TUN","MOD","ROF","BND","F/V","CBW","HBW","RIT","FAV","VOL"};
char button_C[13][4] = {"NON","STP","PPT","TUN","MOD","ROF","BND","F/V","CBW","HBW","RIT","FAV","VOL"};

char step_size[6][2] = {"6","5","4","3","2","1"};
char ptt_star[2][3] = {"* "," "};
char s_meter_value[13][4] = {"S0 ","S1 ","S2 ","S3 ","S4 ","S5 ","S6 ","S7 ","S8 ","S9 ","10 ","20 ","30 "};

uint8 E_buttons = 0;
uint8 E_star = 0;
uint8 E_step = 0;
uint8 E_display_addr = 0x20;
uint8 E_s_meter = 0;

uint8 format_periods(uint32 n, char *out) CYREENTRANT
{
    static int c;
    static char temp_buf[20] = {0};
    static char *p;
    static state = 0;
    static int divide = 0;
    static int modResult;
    static int length = 0;
    static int isNegative = 0;
    static int string_lenth = 0;
    static uint32 copyOfNumber;
    uint8 status = OPERATION_PENDING;

    switch(state){
        case 0:
            if(n < 10000000){
                isNegative = 1;
                length++;
            }
            copyOfNumber = n;
            while(copyOfNumber != 0)
            {
                length++;
                copyOfNumber /= 10;
            }
            state++;
            break;
        case 1:
            for(divide = 0; divide < length; divide++) {
                modResult = n % 10;
                n    = n / 10;
                temp_buf[length - (divide + 1)] = modResult + '0';
            }
            state++;
            break;
        case 2:
            if(isNegative) {
                temp_buf[0] = ' ';
            }
            temp_buf[length] = '\0';
            while((temp_buf[string_lenth]) != 0){
                string_lenth++;
            }
            state++;
            break;
        case 3:
            c = 2 - string_lenth % 3;
            for (p = temp_buf; *p != 0; p++) {
                *out++ = *p;
                if (c == 1) {
                    *out++ = '.';
                }
                c = (c + 1) % 3;
            }
            *--out = 0;
            state =  c  = divide = modResult = length = isNegative = string_lenth = copyOfNumber = 0;
            status = OPERATION_COMPLETED;
            break;
    }  
    return status;
}

uint8 get_position(char *old_freq, char *new_freq){
    uint8 index = 0;
    uint8 stop = FALSE;
    
    do{
        if(old_freq[index] != new_freq[index]){
            stop = TRUE;
        }else{
            index++;
            if(index > 16){
                stop = TRUE;
            }
        }
    }while (stop == FALSE);
    return index;
}

uint8 write_display_freq(uint32 previous_freq,uint32 freq){
    uint8 status = OPERATION_PENDING;
    static uint8 state = 0;
    static uint8 pos;
    static uint8 previous_band = 0;
    //static uint8 pos_temp = 0;;
   
    switch(state){
        case 0:
            status = format_periods(freq,freq_display);
            if(status == OPERATION_COMPLETED){
                status = OPERATION_PENDING;
                state++;
            }
            break;
        case 1:
            status = format_periods(previous_freq,previous_freq_display);
             if(status == OPERATION_COMPLETED){
                status = OPERATION_PENDING;
                state++;
            }
            break;
        case 2:
            status = OPERATION_PENDING;
            if(E_band < BAND_30M  && previous_band != E_band){
                previous_band = E_band;
                pos = 0;
            }else{
                pos = get_position(previous_freq_display,freq_display);
            }
            state++;
            break;
        case 3:
            status = DISPLAY_Position(0,(pos + FREQUENCY_POSITION));
            if(status == OPERATION_COMPLETED){
                status = OPERATION_PENDING;
                state++;
            }
            break;
        case 4:
            status = DISPLAY_PrintString(&freq_display[pos]);
            if(status == OPERATION_COMPLETED){
                status = OPERATION_PENDING;
                state++;
            }
            break;
        case 5:
            memset(freq_display,0,display_size);
            status = OPERATION_COMPLETED;
            state = 0;
            break;
    }
    return status;
}

uint8 display_mode()
{
    uint8 status = 0;
    static uint8 mode_set = FALSE;
    static uint8 state = 0;
    static uint8 mode_index = 0;
        
    if(mode_set == FALSE){
        switch(E_host_mode){
            case 'A':
                mode_index = 0;
                break;
            case 'U':
                mode_index = 2;
                break;
            case 'L':
                mode_index = 1;
                break;
            case 'C':
                mode_index = 3;
                break;
        }
    }
    mode_set = TRUE;
    switch(state){
        case 0:
            status = DISPLAY_Position(0,MODE_POSITION);
            if(status == OPERATION_COMPLETED){
                status = OPERATION_PENDING;
                state++;
            }
            break;
        case 1:
            status = DISPLAY_PrintString(mode_string[mode_index]);
            if(status == OPERATION_COMPLETED){
                mode_set = FALSE;
                state = 0;
            }
    }
    return status;
}

uint8 set_buttons(uint8 button,int index){
    uint8 status = OPERATION_PENDING;
    static uint8 state;
   
    switch(button){
        case 1:
            switch(state){
                case 0:
                    status = DISPLAY_Position(1,BUTTON_A_POSITION);
                    if(status == OPERATION_COMPLETED){
                        status = OPERATION_PENDING;
                        state++;
                    }
                    break;
                case 1:
                    status = DISPLAY_PrintString(button_A[index]);
                    if(status == OPERATION_COMPLETED){
                        state = 0;
                    }
                    break;
            }
            break;
        case 2:
            switch(state){
                case 0:
                    status = DISPLAY_Position(1,BUTTON_B_POSITION);
                    if(status == OPERATION_COMPLETED){
                        status = OPERATION_PENDING;
                        state++;
                    }
                    break;
                case 1:
                    status = DISPLAY_PrintString(button_B[index]);
                    if(status == OPERATION_COMPLETED){
                        state = 0;
                    }
                    break;
            }
            break;
        case 3:
            switch(state){
                case 0:
                    status = DISPLAY_Position(1,BUTTON_C_POSITION);
                    if(status == OPERATION_COMPLETED){
                        status = OPERATION_PENDING;
                        state++;
                    }
                    break;
                case 1:
                    status = DISPLAY_PrintString(button_C[index]);
                    if(status == OPERATION_COMPLETED){
                        state = 0;
                    }
                    break;
            }
            break;
    }
    return status;
}

uint8 set_star(uint8 button,uint8 on_off){
    uint8 status = OPERATION_PENDING;
    static uint8 state;
   
    switch(state){
        case 0:
            switch(button){
                case 0x10:
                    status = DISPLAY_Position(1,(BUTTON_A_POSITION + 3));
                    break;
                case 0x20:
                    status = DISPLAY_Position(1,(BUTTON_B_POSITION + 3));
                    break;
                case 0x30:
                    status = DISPLAY_Position(1,(BUTTON_C_POSITION + 3));
                    break;
                //default:
                  //  status = OPERATION_COMPLETED;
                    //state = 0;
            }
            if(status == OPERATION_COMPLETED){
                status = OPERATION_PENDING;
                state++;
            }
            break;
        case 1:
            if(on_off == TRUE){
                status = DISPLAY_PrintString("*");
            }else{
                status = DISPLAY_PrintString(" ");
            }
            if(status == OPERATION_COMPLETED){
                state = 0;
            }
            break;
    }
    return status;
}

uint32 dequeue_freq(){
    uint32 ret = 0;
    if(E_freq_queue_front == -1){
        ret = 0;
    }else{
        ret = E_freq_queue[E_freq_queue_front];
        E_freq_queue[E_freq_queue_front] = 0;
        if(E_freq_queue_front == E_freq_queue_rear){
            E_freq_queue_front = E_freq_queue_rear = -1;
        }else{
            E_freq_queue_front = (E_freq_queue_front + 1)%MAX_COMMAND_QUEUE;
        }
    }
    return ret;
}
uint8 ptt(uint8 on_off){
    static uint8 state = 0;
    uint8 status = OPERATION_PENDING;
    switch(state){
        case 0:
            status = DISPLAY_Position(0,0);
            if(status == OPERATION_COMPLETED){
                status = OPERATION_PENDING;
                state++;
            }
            break;
        case 1:
            switch(on_off){
                case 0:
                    status = DISPLAY_PrintString(ptt_star[1]);
                    break;
                case 1:
                    status = DISPLAY_PrintString(ptt_star[0]);
                    break;
            }
            if(status == OPERATION_COMPLETED){
                state = 0;
            }
    }
    return status;
}

uint8 set_step(uint8 step_value){
    static uint8 state = 0;
    uint8 status = OPERATION_PENDING;
    char star[2] = {0};
    
    switch(state){
        case 0:
            status = DISPLAY_Position(0,STEP_POSITION);
            if(status == OPERATION_COMPLETED){
                status = OPERATION_PENDING;
                state++;
            }
            break;
        case 1:
            status = DISPLAY_PrintString(step_size[step_value]);
            if(status == OPERATION_COMPLETED){
                state = 0;
            }
    }
    return status;
}

uint8_t Display_Set_Address(void){
    uint8_t msg_buffer[2];
    uint8_t status = 1;
    uint8_t write_status;
    uint8_t buffer_written;
    
    E_display_addr = 0x27;
    msg_buffer[0] = 0x00;   //IODIR Register
    msg_buffer[1] = 0x00;   //Set IODIR to OUTPUT
    write_status = I2C_DISPLAY_MasterWriteBuf(E_display_addr,msg_buffer,2u,
        I2C_DISPLAY_MODE_COMPLETE_XFER);
    while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
    buffer_written = I2C_DISPLAY_MasterGetWriteBufSize();
    if(buffer_written != (2u)){
        E_display_addr = 0x3F;
        write_status = I2C_DISPLAY_MasterWriteBuf(E_display_addr,msg_buffer,2u,
            I2C_DISPLAY_MODE_COMPLETE_XFER);
        while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
        buffer_written = I2C_DISPLAY_MasterGetWriteBufSize();
        if(buffer_written != (2u)){status = 0;}
    }
    return status;
}
uint8 s_meter(uint8 s_value){
    static uint8 state = 0;
    uint8 status = OPERATION_PENDING;
    
    switch(state){
        case 0:
            status = DISPLAY_Position(0,0);
            if(status == OPERATION_COMPLETED){
                status = OPERATION_PENDING;
                state++;
            }
            break;
        case 1:
            status = DISPLAY_PrintString(s_meter_value[s_value]);
            if(status == OPERATION_COMPLETED){
                state = 0;
            }
    }
    return status;
}

#define MASTER_TIMER 20
#define CONTROL_TIMER 20
#define END_TIMER 5

uint8 Process_Display(){
    uint8 status = OPERATION_COMPLETED; 
    static uint32 previous_freq = 0;
    static uint8 state = 0;   
    static uint8 first_pass = FALSE;
    static uint8 previous_mode = 'N';
    static uint8 previous_buttons = 0;
    static uint32 freq;
    static uint8 processing = FALSE;
    static uint8 previous_ptt = 0;
    static int button_index = 0;
    static uint8 button = 0;
    static uint8 previous_star = 0;
    static uint8 star_status = FALSE;
    static uint8 previous_step = 100;
    static uint8 previous_s_meter = 0;
    
    if(first_pass == FALSE){
        E_master_timer = MASTER_TIMER;
        E_end_timer = END_TIMER;
        E_control_timer = CONTROL_TIMER;
        first_pass = TRUE;
    }
    if(E_si5351_status == FALSE){
        switch(state){
            case 0:
                if(processing == FALSE){
                    freq = dequeue_freq();
                }
                if(freq != 0){
                   processing = TRUE;
                   status = write_display_freq(previous_freq,freq);
                   if(status == OPERATION_COMPLETED){
                        state++;
                        previous_freq = freq;
                        processing = FALSE;
                    }                    
                }else{
                    state++;
                }
                break;
            case 1:
                if(previous_mode != E_host_mode){
                    status = display_mode();
                    if(status == OPERATION_COMPLETED){
                        previous_mode = E_host_mode;
                        state++;
                    }
                }else{
                    state++;
                }
                break;
            case 2:
                if(previous_buttons != E_buttons){
                    button_index = E_buttons & 0x0F;
                    if(E_buttons >= 0x30){
                        button = 3;
                    }else {
                        if (E_buttons >= 0x20){
                            button = 2;
                        }else{
                            if(E_buttons >= 0x10){
                                button = 1;
                            }
                        }
                    }
                    status = set_buttons(button,button_index);
                    if(status == OPERATION_COMPLETED){
                        previous_buttons = E_buttons;
                        state++;
                    }
                }else{
                    state++;
                }
                break;
            case 3:
                if(previous_ptt != TX_Request){
                    status = ptt(TX_Request);
                    if(status == OPERATION_COMPLETED){
                        previous_ptt = TX_Request;
                        state++;
                    }
                }else{
                    state++;
                }
                break;
            case 4:
                if(previous_star != E_star){
                    star_status = E_star & 0x80;
                    if(star_status == 0x80){
                        star_status = TRUE;
                    }else{
                        star_status = FALSE;
                    }
                    button = E_star & 0x30;
                    status = set_star(button,star_status);
                    if(status == OPERATION_COMPLETED){
                        previous_star = E_star;
                        state++;
                    }
                }else{
                    state++;
                }
                break;
            case 5:
                if(previous_step != E_step){
                    status = set_step(E_step);
                    if(status == OPERATION_COMPLETED){
                        previous_step = E_step;
                        state = 6;
                    }
                }else{
                    state = 6;
                }
                break;
            case 6:
                if(previous_s_meter != E_s_meter){
                    if(!TX_Request){
                        status = s_meter(E_s_meter);
                        if(status == OPERATION_COMPLETED){
                            previous_s_meter = E_s_meter;
                            state = 0;
                        }
                    }
                }else{
                    state = 0;
                }
                break;
        }
    }
    return status;
}*/
/* [] END OF FILE */
