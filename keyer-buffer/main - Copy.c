/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.8
        Device            :  PIC16F18326
        Driver Version    :  2.00
 */

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
 */
#include "mcc_generated_files/mcc.h"
#include "defines.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>


#define TRUE 1
#define FALSE 0
#define COMMAND_QUEUE_SIZE 16
#define LINE_QUEUE_SIZE 16
#define QUEUE_EMPTY 199

#define KEYER_STRAIGHT 0
#define KEYER_MODE_A 1
#define KEYER_MODE_B 2

#define SET_KEYER_MODE 0x71
#define SET_CW_PADDLE 0x73
#define SET_SPACING 0x75
#define SET_MEMORY_TYPE 0x76
#define SET_WEIGHT 0x77
#define SET_WPM 0x7B
#define SET_IAMBIC_TUNING 0x7C
#define SET_SIDE_TONE 0x7F

#define CW_DELAY_CONSTANT 2750
#define KEY_ON 1
#define KEY_OFF 0

#define SIDE_TONE_400 0
#define SIDE_TONE_600 1
#define SIDE_TONE_800 2
#define SIDE_TONE_1000 3

enum {
    CHECK = 0,
    PREDOT,
    PREDASH,
    SENDDOT,
    SENDDASH,
    DOTDELAY,
    DASHDELAY,
    DOTHELD,
    DASHHELD,
    LETTERSPACE,
    EXITLOOP
};

static int dot_memory = 0;
static int dash_memory = 0;
static int key_state = 0;
static int kdelay = 0;
static int dot_delay = 0;
static int dash_delay = 0;
static int kcwl = 0;
static int kcwr = 0;
static int *kdot;
static int *kdash;
static int dash_count = 0;
static int dot_count = 0;

static int cw_keyer_speed = 18;
static int cw_keyer_weight = 50;
static int cw_keyer_mode = KEYER_MODE_B;
static int cw_keyer_spacing = 0;
static int cw_keys_reversed = 1;
static int cw_active_state = 0;
static int32_t cw_loop_delay = 0;

static int cw_keyer_sidetone_frequency = 700;
static int cw_keyer_sidetone_gain = 10;
static int cw_keyer_sidetone_envelope = 5;

static int running, keyer_out = 0;

__eeprom int cw_keyer_speed__eeprom = 18;
__eeprom int cw_keyer_weight__eeprom = 50;
__eeprom int cw_keyer_mode__eeprom = KEYER_MODE_B;
__eeprom int cw_keyer_spacing__eeprom = 0;
__eeprom int cw_keys_reversed__eeprom = 1;
__eeprom int32_t cw_loop_delay_calibration__eeprom = 0;
__eeprom uint8_t NCO1INCH__eeprom = 0;
__eeprom uint8_t NCO1INCL__eeprom = 0x27;

volatile uint8_t LineDataRow = 0;
volatile uint8_t message[16], SlaveAddress, SlaveR_W, cursor_row = 0, cursor_column = 0, Scan_Row_Index = 0,
        SlaveInit, SlaveTransmit[1], msg, Line_Data[LINE_QUEUE_SIZE][20], LineDataIndex, first_pass = TRUE;
volatile uint8_t tmpSlaveAddr;
volatile uint8_t msg_size_read = 0;
volatile uint8_t cmd_read = 0;
volatile uint8_t param1 = 0;
volatile uint8_t param2 = 0;

volatile uint8_t CMD_Queue[COMMAND_QUEUE_SIZE][3] = {0};
volatile int16_t CMD_queue_front = -1;
volatile int16_t CMD_queue_rear = -1;
volatile uint8_t CMD_add_queue_busy = 0;
volatile uint8_t CMD_dequeue_busy = 0;
volatile uint8_t CMD_queue_count = 0;
volatile uint8_t Queue_Overrun = 0;
volatile uint8_t Return_Queue_count = 0;

volatile uint8_t audio_time_out = 0;
volatile uint32_t audio_time_count = 3200;
static uint8_t tx_active = FALSE;

void Audio_interrupt_time_out(void) {
    //TMR3IF = 0;
    //if (audio_time_count-- == 0) {
    if (audio_time_out == TRUE) {
        RX_CW_SetLow();
        audio_time_out = FALSE;
    } else {
        RX_CW_SetHigh();
        audio_time_out = TRUE;
    }
    audio_time_count = 3200;
    //}
}

uint8_t CMD_dequeue(uint8_t *param1, uint8_t *param2) {
    uint8_t ret = QUEUE_EMPTY;

    if (CMD_add_queue_busy == 0) {
        CMD_dequeue_busy = 1;
        if (CMD_queue_front == -1) {
            ret = QUEUE_EMPTY;
        } else {
            ret = CMD_Queue[CMD_queue_front][0];
            *param1 = CMD_Queue[CMD_queue_front][1];
            *param2 = CMD_Queue[CMD_queue_front][2];
            CMD_Queue[CMD_queue_front][0] = QUEUE_EMPTY;
            if (CMD_queue_front == CMD_queue_rear) {
                CMD_queue_front = CMD_queue_rear = -1;
            } else {
                CMD_queue_front = (CMD_queue_front + 1) % COMMAND_QUEUE_SIZE;
            }
        }
        if (ret != QUEUE_EMPTY) {
            CMD_queue_count--;
        }
        CMD_dequeue_busy = 0;
    }
    return ret;
}

void CMD_queue_add(uint8_t command, uint8_t param1, uint8_t param2) {

    CMD_add_queue_busy = 1;
    if (CMD_queue_front == (CMD_queue_rear + 1) % COMMAND_QUEUE_SIZE) {
        Queue_Overrun = 1;
        __delay_ms(3);
        CMD_queue_front = -1;
        CMD_queue_rear = -1;
        CMD_queue_count = 0;
    } else {
        if (CMD_queue_front == -1) {
            CMD_queue_front = CMD_queue_rear = 0;
        } else {
            CMD_queue_rear = (CMD_queue_rear + 1) % COMMAND_QUEUE_SIZE;
        }
        CMD_Queue[CMD_queue_rear][0] = command;
        CMD_Queue[CMD_queue_rear][1] = param1;
        CMD_Queue[CMD_queue_rear][2] = param2;
        CMD_queue_count++;
        if (CMD_queue_count > COMMAND_QUEUE_SIZE) {
            CMD_queue_count = COMMAND_QUEUE_SIZE;
        }
    }
    CMD_add_queue_busy = 0;
}

void I2C_SlaveAddressCallbackHandler() {

    tmpSlaveAddr = I2C1_Read(); /* Address received from Master, indicate New transfer starting. */
    SlaveAddress = tmpSlaveAddr >> 1u; /* 7 bit I2C address. */
    SlaveR_W = tmpSlaveAddr & 0x01u; /* May also need to know whether this is Read or Write transfer. */

    if (SlaveR_W == 0) { /* R/W bit is 0, makes Slave Receiver */
        SlaveInit = 1; /* Next transfer will be first Data byte from Master. */
    } else // (SlaveR_W == 1) /*Read bit from Master, makes Slave Transmitter, shall return first value immediately. */				
    {
        Return_Queue_count = CMD_queue_count;
        if (Return_Queue_count > 32) {
            Return_Queue_count = 32;
        }
    }
}

void I2C_SlaveReceiveCallbackHandler() {

    if (SlaveInit) {
        SlaveInit--;
        LineDataIndex = 0;
        msg_size_read = 0;
        cmd_read = 0;
        param1 = 0;
        param2 = 0;

        if (first_pass == TRUE) {
            msg = I2C1_Read(); //Get message type
            first_pass = FALSE;
        } else {
            param1 = I2C1_Read();
            first_pass = TRUE;
            CMD_queue_add(msg, param1, param2);
        }
    }

}

void I2C_SlaveTransmitCallbackHandler() {
    I2C1_Write(Return_Queue_count);
}

void I2C_SlaveCollisionCallbackHandler() {

}

void keyer_update() {
    dot_delay = 1200 / cw_keyer_speed;
    cw_loop_delay = CW_DELAY_CONSTANT / cw_keyer_speed;
    // will be 3 * dot length at standard weight
    dash_delay = (dot_delay * 3 * cw_keyer_weight) / 50;
    if (cw_keys_reversed) {
        kdot = &kcwr;
        kdash = &kcwl;
    } else {
        kdot = &kcwl;
        kdash = &kcwr;
    }
}

void clear_memory() {
    dot_memory = 0;
    dash_memory = 0;
}

void set_keyer_out(int state) {

    if (keyer_out != state) {
        keyer_out = state;
        if (tx_active == FALSE) {
            tx_active = TRUE;
            RX_CW_SetHigh();
            TX_CW_SetLow();
        }
        if (state) {
            KEY_0A_SetLow();
            NCO1CONbits.N1EN = 1;
            //Audio output logic
        } else {
            KEY_0A_SetHigh();
            NCO1CONbits.N1EN = 0;
            //Audio output logic
        }
    }
}

void keyer() {
    int32_t loop_count = 0;
    static uint32_t rx_delay_count = 30;

    key_state = CHECK;
    while (key_state != EXITLOOP) {
        kcwl = !(KEY_0_GetValue());
        kcwr = !(KEY_1_GetValue());
        switch (key_state) {
            case CHECK: // check for key press
                if (cw_keyer_mode == KEYER_STRAIGHT) { // Straight/External key or bug
                    if (*kdash) { // send manual dashes
                        set_keyer_out(KEY_ON);
                        key_state = EXITLOOP;
                    } else if (*kdot) // and automatic dots
                        key_state = PREDOT;
                    else {
                        set_keyer_out(KEY_OFF);
                        key_state = EXITLOOP;
                    }
                } else {
                    if (*kdot)
                        key_state = PREDOT;
                    else if (*kdash)
                        key_state = PREDASH;
                    else {
                        set_keyer_out(KEY_OFF);
                        key_state = EXITLOOP;
                    }
                }
                break;
            case PREDOT: // need to clear any pending dots or dashes
                clear_memory();
                key_state = SENDDOT;
                break;
            case PREDASH:
                clear_memory();
                key_state = SENDDASH;
                break;

                // dot paddle  pressed so set keyer_out high for time dependant on speed
                // also check if dash paddle is pressed during this time
            case SENDDOT:
                set_keyer_out(KEY_ON);
                if (kdelay == dot_delay) {
                    kdelay = 0;
                    set_keyer_out(KEY_OFF);
                    key_state = DOTDELAY; // add inter-character spacing of one dot length
                } else kdelay++;

                // if Mode A and both paddels are relesed then clear dash memory
                if (cw_keyer_mode == KEYER_MODE_A) {
                    if (!*kdot & !*kdash)
                        dash_memory = 0;
                    else if (*kdash) // set dash memory
                        dash_memory = 1;
                }
                break;

                // dash paddle pressed so set keyer_out high for time dependant on 3 x dot delay and weight
                // also check if dot paddle is pressed during this time
            case SENDDASH:
                set_keyer_out(KEY_ON);
                if (kdelay == dash_delay) {
                    kdelay = 0;
                    set_keyer_out(KEY_OFF);
                    key_state = DASHDELAY; // add inter-character spacing of one dot length
                } else kdelay++;

                // if Mode A and both padles are relesed then clear dot memory
                if (cw_keyer_mode == KEYER_MODE_A) {
                    if (!*kdot & !*kdash)
                        dot_memory = 0;
                    else if (*kdot) // set dot memory
                        dot_memory = 1;
                }
                break;

                // add dot delay at end of the dot and check for dash memory, then check if paddle still held
            case DOTDELAY:
                if (kdelay == dot_delay) {
                    kdelay = 0;
                    if (!*kdot && cw_keyer_mode == KEYER_STRAIGHT) // just return if in bug mode
                        key_state = EXITLOOP;
                    else if (dash_memory) // dash has been set during the dot so service
                        key_state = PREDASH;
                    else key_state = DOTHELD; // dot is still active so service
                } else kdelay++;

                if (*kdash) // set dash memory
                    dash_memory = 1;
                break;

                // add dot delay at end of the dash and check for dot memory, then check if paddle still held
            case DASHDELAY:
                if (kdelay == dot_delay) {
                    kdelay = 0;

                    if (dot_memory) // dot has been set during the dash so service
                        key_state = PREDOT;
                    else key_state = DASHHELD; // dash is still active so service
                } else kdelay++;

                if (*kdot) // set dot memory
                    dot_memory = 1;
                break;

                // check if dot paddle is still held, if so repeat the dot. Else check if Letter space is required
            case DOTHELD:
                if (*kdot) // dot has been set during the dash so service
                    key_state = PREDOT;
                else if (*kdash) // has dash paddle been pressed
                    key_state = PREDASH;
                else if (cw_keyer_spacing) { // Letter space enabled so clear any pending dots or dashes
                    clear_memory();
                    key_state = LETTERSPACE;
                } else key_state = EXITLOOP;
                break;

                // check if dash paddle is still held, if so repeat the dash. Else check if Letter space is required
            case DASHHELD:
                if (*kdash) // dash has been set during the dot so service
                    key_state = PREDASH;
                else if (*kdot) // has dot paddle been pressed
                    key_state = PREDOT;
                else if (cw_keyer_spacing) { // Letter space enabled so clear any pending dots or dashes
                    clear_memory();
                    key_state = LETTERSPACE;
                } else key_state = EXITLOOP;
                break;

                // Add letter space (3 x dot delay) to end of character and check if a paddle is pressed during this time.
                // Actually add 2 x dot_delay since we already have a dot delay at the end of the character.
            case LETTERSPACE:
                if (kdelay == 2 * dot_delay) {
                    kdelay = 0;
                    if (dot_memory) // check if a dot or dash paddle was pressed during the delay.
                        key_state = PREDOT;
                    else if (dash_memory)
                        key_state = PREDASH;
                    else key_state = EXITLOOP; // no memories set so restart
                } else kdelay++;

                // save any key presses during the letter space delay
                if (*kdot) dot_memory = 1;
                if (*kdash) dash_memory = 1;
                break;

            default:
                key_state = EXITLOOP;
        }
        //loop_delay = cw_loop_delay;
        loop_count = 0;
        while (loop_count++ < cw_loop_delay) {
            __delay_us(1);
        }
    }
    if(tx_active == TRUE){
        tx_active = FALSE;
        RX_CW_SetLow();
        TX_CW_SetHigh();
        rx_delay_count = 30000;
    }
}

void CW_Update_Config() {
    uint8_t scan_msg = QUEUE_EMPTY;
    uint8_t param1 = 0;
    uint8_t param2 = 0;

    scan_msg = CMD_dequeue(&param1, &param2);
    if (scan_msg != QUEUE_EMPTY) {
        NCO1CONbits.N1EN = 0;
        switch (scan_msg) {
            case SET_CW_PADDLE:
                cw_keys_reversed = param1;
                cw_keys_reversed__eeprom = param1;
                break;
            case SET_WEIGHT:
                cw_keyer_weight = param1;
                cw_keyer_weight__eeprom = param1;
                break;
            case SET_KEYER_MODE:
                cw_keyer_mode = param1;
                cw_keyer_mode__eeprom = param1;
                break;
            case SET_WPM:
                if (param1 == 5) {
                    param1 = 6;
                }
                cw_keyer_speed = param1;
                cw_keyer_speed__eeprom = param1;
                break;
            case SET_SPACING:
                cw_keyer_spacing = param1;
                cw_keyer_spacing__eeprom = param1;
                break;
            case SET_IAMBIC_TUNING:
                //cw_loop_delay_calibration = param1 * 10;
                //cw_loop_delay_calibration__eeprom = cw_loop_delay_calibration;
                break;
            case SET_SIDE_TONE:
                switch (param1) {
                    case SIDE_TONE_400:
                        NCO1INCH = 0;
                        NCO1INCL = 0x1A;
                        NCO1INCH__eeprom = 0;
                        NCO1INCL__eeprom = 0x1A;
                        break;
                    case SIDE_TONE_600:
                        NCO1INCH = 0;
                        NCO1INCL = 0x27;
                        NCO1INCH__eeprom = 0;
                        NCO1INCL__eeprom = 0x27;
                        break;
                    case SIDE_TONE_800:
                        NCO1INCH = 0;
                        NCO1INCL = 0x34;
                        NCO1INCH__eeprom = 0;
                        NCO1INCL__eeprom = 0x34;
                        break;
                    case SIDE_TONE_1000:
                        NCO1INCH = 0;
                        NCO1INCL = 0x42;
                        NCO1INCH__eeprom = 0;
                        NCO1INCL__eeprom = 0x42;
                        break;
                    default:
                        NCO1INCH = 0;
                        NCO1INCL = 0x27;
                        NCO1INCH__eeprom = 0;
                        NCO1INCL__eeprom = 0x27;
                }
                break;
        }
        keyer_update();
    }
}

void CW_Initialize() {

    cw_keyer_speed = cw_keyer_speed__eeprom;
    cw_keys_reversed = cw_keys_reversed__eeprom;
    cw_keyer_weight = cw_keyer_weight__eeprom;
    cw_keyer_mode = cw_keyer_mode__eeprom;
    cw_keyer_spacing = cw_keyer_spacing__eeprom;
    NCO1INCH = NCO1INCH__eeprom;
    NCO1INCL = NCO1INCL__eeprom;
    //NCO1INCH = 0;
    //NCO1INCL = 0x34;
    //NCO1INCH__eeprom = 0;
    //NCO1INCL__eeprom = 0x34;
    NCO1CONbits.N1EN = 0;
    keyer_update();
}

void CW_Initialize_1() {

    cw_keyer_speed = 18;
    cw_keys_reversed = 0;
    cw_keyer_weight = 50;
    cw_keyer_mode = KEYER_MODE_B;
    cw_keyer_spacing = 0;
    NCO1INCH = 0;
    NCO1INCL = 0x27;
    keyer_update();
}

void I2C_Initialize() {
    I2C1_Initialize();
    I2C1_Open();
    I2C1_SlaveSetReadIntHandler(I2C_SlaveReceiveCallbackHandler);
    I2C1_SlaveSetAddrIntHandler(I2C_SlaveAddressCallbackHandler); /* Set I2C Slave Callback pointers. */
    I2C1_SlaveSetWriteIntHandler(I2C_SlaveTransmitCallbackHandler);
    I2C1_SlaveSetBusColIntHandler(I2C_SlaveCollisionCallbackHandler);
}

void main(void) {

    int looper = 3000;
    int looper_1 = 3000;
    uint16_t timer_value = 0;

    __delay_ms(1000); //Wait for Power to stabilize 
    SYSTEM_Initialize();
    I2C_Initialize();
    TMR3_Initialize();
    TMR3_SetInterruptHandler(Audio_interrupt_time_out);
    TMR3_StopTimer();
    TMR3_WriteTimer(0xE000);
    TMR3_Reload();
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();
    //CW_Initialize_1();
    CW_Initialize();
    KEY_0_SetPullup();
    KEY_1_SetPullup();
    KEY_1A_SetPullup();
    KEY_0A_SetPullup();
    KEY_1_SetLow();
    KEY_0_SetLow();
    KEY_1A_SetHigh();
    KEY_0A_SetHigh();
    RX_CW_SetLow();
    TX_CW_SetHigh();

    while (1) {
        keyer();
        /*if (looper-- <= 0) {
            TMR3_StopTimer();
            timer_value = TMR3_ReadTimer();
            timer_value = timer_value + 1;
            TMR3_WriteTimer(timer_value);
            TMR3_Reload();
            TMR3_StartTimer();
            looper_1 = 3000;
            while(looper_1-- >= 3000){}
        }*/
        if (CMD_add_queue_busy == 0) {
            CW_Update_Config();
        }
    }
}
/**
 End of File
 */