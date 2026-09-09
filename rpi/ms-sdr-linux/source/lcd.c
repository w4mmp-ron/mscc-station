/*******************************************************************************
 * File Name: DISPLAY_I2C.c
 * Version `$CY_MAJOR_VERSION`.`$CY_MINOR_VERSION`
 *
 * Description:
 *  This file provides source code for the CharLCD_I2C component's API.
 *
 * Note:
 *
 ********************************************************************************
 * Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
 * You may use this file only in accordance with the license, terms, conditions,
 * disclaimers, and limitations in the end user license agreement accompanying
 * the software package with which this file was provided.
 *******************************************************************************/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/timeb.h>
#include <sys/time.h>

#include "usbavrcmd.h"
#include <usb.h>
#include "SRDLL.h"
#include "extern.h"
#include "version.h"
#include "band_stack.h"
#include "last_used.h"
#include "iq.h"
#include "lcd.h"
#define MASTER_TIMER_VALUE 10000
#define CONTROL_TIMER_VALUE 10000
#define END_TIMER 10000

/* Stores the state of conponent. Indicates wherewer component is 
 * in enabled state or not.
 */
uint8_t DISPLAY_enableState = 0u;

uint8_t DISPLAY_initVar = 0u;
uint8_t New_position = FALSE;
uint8_t Display_Initializing = FALSE;
uint32_t E_master_timer = MASTER_TIMER_VALUE;
uint32_t E_end_timer = END_TIMER;
uint32_t E_control_timer = CONTROL_TIMER_VALUE;





/*******************************************************************************
 * Function Name: DISPLAY_PrintString
 ********************************************************************************
 *
 * Summary:
 *  Writes a zero terminated string to the LCD.
 *
 * Parameters:
 *  string:  pointer to head of char8 array to be written to the LCD module
 *
 * Return:
 *  None.
 *
 *******************************************************************************/


/*******************************************************************************
 *  Function Name: DISPLAY_PutChar
 ********************************************************************************
 *
 * Summary:
 *  Writes a single character to the current cursor position of the LCD module.
 *  Custom character names (`$INTANCE_NAME`_CUSTOM_0 through
 *  `$INTANCE_NAME`_CUSTOM_7) are acceptable as inputs.
 *
 * Parameters:
 *  character:  character to be written to the LCD
 *
 * Return:
 *  None.
 *
 *******************************************************************************/

/*******************************************************************************
 *  Function Name: DISPLAY_WriteData
 ********************************************************************************
 *
 * Summary:
 *   Writes a Control Command to the LCD by sending the Upper Nibble and
 *   the Lower Nibble twice. Once with the E input High and once with the 
 *	E input Low with the the RS input held High and the RW input held Low.
 *	The K input or Back Light (BL) input is also held High.
 *
 *******************************************************************************/

uint8_t DISPLAY_WriteData(uint8_t dByte, uint8_t wait) {
    uint8_t status = OPERATION_COMPLETED;
    static uint8_t state = 0;
    static uint8_t buffer[DISPLAY_BUFFER_SIZE], UPPER_NIB, LOWER_NIB;
    static uint8_t BLH_EH_RWL_RSH, BLH_EL_RWL_RSH;
    int write_status = 0;
    char filename[PATH_MAX];
    UPPER_NIB = dByte & DISPLAY_UPPER_NIB_MASK;
    LOWER_NIB = (dByte & DISPLAY_LOWER_NIB_MASK) << DISPLAY_LOWER_NIB_SHIFT;

    BLH_EH_RWL_RSH = DISPLAY_BLH + DISPLAY_EH + DISPLAY_RSH;
    BLH_EL_RWL_RSH = DISPLAY_BLH + DISPLAY_RSH;

    /* Initialize buffer with packet */

    buffer[DISPLAY_PACKET_0_POS] = UPPER_NIB | BLH_EH_RWL_RSH;
    buffer[DISPLAY_PACKET_1_POS] = UPPER_NIB | BLH_EL_RWL_RSH;
    buffer[DISPLAY_PACKET_2_POS] = LOWER_NIB | BLH_EH_RWL_RSH;
    buffer[DISPLAY_PACKET_3_POS] = LOWER_NIB | BLH_EL_RWL_RSH;
    write_status = Write_i2c(I2C_BUSS, filename, E_display_addr, 0x00, buffer, 2, "DISPLAY_WriteData");
    write_status = Write_i2c(I2C_BUSS, filename, E_display_addr, 0x00, &buffer[2], 2, "DISPLAY_WriteData");
    status = OPERATION_COMPLETED;

    return status;
}

/*******************************************************************************
 *  Function Name: DISPLAY_WriteControl
 ********************************************************************************
 *
 * Summary:
 *   Writes a Control Command to the LCD by sending the Upper Nibble and
 *   the Lower Nibble twice. Once with the E input High and once with the 
 *	E input Low with the the RS and the RW input held Low.
 *	The K input or Back Light (BL) input is also held High.
 *
 *******************************************************************************/

uint8_t DISPLAY_WriteControl(uint8_t cByte, uint8_t wait) {
    uint8_t status = OPERATION_COMPLETED;
    static uint8_t state = 0;
    static uint8_t buffer[DISPLAY_BUFFER_SIZE], UPPER_NIB, LOWER_NIB;
    static uint8_t BLH_EH_RWL_RSL, BLH_EL_RWL_RSL;
    int write_status = 0;
    char filename[PATH_MAX];

    UPPER_NIB = cByte & DISPLAY_UPPER_NIB_MASK;
    LOWER_NIB = (cByte & DISPLAY_LOWER_NIB_MASK) << DISPLAY_LOWER_NIB_SHIFT;

    BLH_EH_RWL_RSL = DISPLAY_BLH + DISPLAY_EH;
    BLH_EL_RWL_RSL = DISPLAY_BLH;

    /* Initialize buffer with packet */

    buffer[DISPLAY_PACKET_0_POS] = UPPER_NIB | BLH_EH_RWL_RSL;
    buffer[DISPLAY_PACKET_1_POS] = UPPER_NIB | BLH_EL_RWL_RSL;
    buffer[DISPLAY_PACKET_2_POS] = LOWER_NIB | BLH_EH_RWL_RSL;
    buffer[DISPLAY_PACKET_3_POS] = LOWER_NIB | BLH_EL_RWL_RSL;
    write_status = Write_i2c(I2C_BUSS, filename, E_display_addr, 0x00, buffer, 2, "DISPLAY_WriteControl");
    write_status = Write_i2c(I2C_BUSS, filename, E_display_addr, 0x00, &buffer[2], 2, "DISPLAY_WriteControl");
    status = OPERATION_COMPLETED;

    return status;
}

/*******************************************************************************
 *  Function Name: DISPLAY_WrtCntrlUpNib
 ********************************************************************************
 *
 * Summary:
 *   Writes a Control Command to the LCD by sending the Upper Nibble twice,
 *	once with the E input High and once with the E input 
 *   Low with the RS input and RW input held Low. 
 *	The K input or Back Light (BL) input is also held High.
 *
 *******************************************************************************/
void DISPLAY_WrtCntrlUpNib(uint8_t cByte) {
    uint8_t status = OPERATION_COMPLETED;
    uint8_t buffer[DISPLAY_UPPER_NIB_BUFFER_SIZE], UPPER_NIB;
    uint8_t BLH_EH_RWL_RSL, BLH_EL_RWL_RSL;
    int write_status = 0;
    char filename[PATH_MAX];


    UPPER_NIB = cByte & DISPLAY_UPPER_NIB_MASK;
    BLH_EH_RWL_RSL = DISPLAY_BLH + DISPLAY_EH;
    BLH_EL_RWL_RSL = DISPLAY_BLH;

    /* Initialize buffer with packet */
    buffer[DISPLAY_PACKET_0_POS] = UPPER_NIB | BLH_EH_RWL_RSL;
    buffer[DISPLAY_PACKET_1_POS] = UPPER_NIB | BLH_EL_RWL_RSL;
    write_status = Write_i2c(I2C_BUSS, filename, E_display_addr, 0x00, buffer,
            DISPLAY_UPPER_NIB_PACKET_SIZE, "DISPLAY_WrtCntrlUpNib");
    usleep(DISPLAY_CMD_DELAY_US);
}

void DISPLAY_Init(void) {
    /* INIT CODE */
    usleep(DISPLAY_INIT_DELAY); /* Delay 20 ms */
    DISPLAY_WrtCntrlUpNib(DISPLAY_DISPLAY_8_BIT_INIT); /* Selects 8-bit mode */
    usleep(DISPLAY_INIT_UP_NIB_DELAY); /* Delay 5 ms */
    DISPLAY_WrtCntrlUpNib(DISPLAY_DISPLAY_8_BIT_INIT); /* Selects 8-bit mode */ /* Delay 5 ms */
    usleep(DISPLAY_INIT_UP_NIB_DELAY); /* Delay 5 ms */
    DISPLAY_WrtCntrlUpNib(DISPLAY_DISPLAY_8_BIT_INIT); /* Selects 8-bit mode */
    usleep(DISPLAY_INIT_UP_NIB_DELAY); /* Delay 5 ms */
    DISPLAY_WrtCntrlUpNib(DISPLAY_DISPLAY_4_BIT_INIT); /* Selects 8-bit mode */
    usleep(DISPLAY_INIT_UP_NIB_DELAY); /* Delay 5 ms */

    DISPLAY_WriteControl(DISPLAY_DISPLAY_4_BIT_INIT, FALSE); /* Write 4-bit Mode 2x16 or 4x20 Char */
    usleep(DISPLAY_INIT_CMD_DELAY); /* Delay 5 ms */
    DISPLAY_WriteControl(DISPLAY_CLEAR_DISPLAY, FALSE); /* Clear LCD Screen */
    usleep(DISPLAY_INIT_CMD_DELAY); /* Delay 5 ms */
    DISPLAY_WriteControl(DISPLAY_CURSOR_AUTO_INCR_ON, FALSE); /* Incr Cursor One Space to Right After Writes */
    usleep(DISPLAY_INIT_CMD_DELAY); /* Delay 5 ms */
    DISPLAY_WriteControl(DISPLAY_DISPLAY_ON_CURSOR_OFF, FALSE); /* Turn Display ON, Cursor OFF */
    usleep(DISPLAY_INIT_CMD_DELAY); /* Delay 5 ms */
}

uint8_t DISPLAY_Position(uint8_t row, uint8_t column) {
    static uint8_t state = 0;
    uint8_t status = OPERATION_PENDING;
    static uint8_t control_timer = 0;

    control_timer = E_control_timer;
    switch (row) {
        case (uint8_t) 0:
            status = DISPLAY_WriteControl(DISPLAY_ROW_0_START + column, TRUE);
            break;
        case (uint8_t) 1:
            status = DISPLAY_WriteControl(DISPLAY_ROW_1_START + column, TRUE);
            break;
        case (uint8_t) 2:
            status = DISPLAY_WriteControl(DISPLAY_ROW_2_START + column, TRUE);
            break;
        case (uint8_t) 3:
            status = DISPLAY_WriteControl(DISPLAY_ROW_3_START + column, TRUE);
            break;
        default:
            /* if default case is hit, invalid row argument was passed.*/
            break;
    }
    status = OPERATION_COMPLETED;
    return status;
}

uint8_t DISPLAY_PrintString(uint8_t const string[]) {
    static uint8_t indexU8 = 0u;
    static uint8_t temp_string[17] = {0};
    static uint8_t status = OPERATION_COMPLETED;
    static uint8_t state = 0;
    static uint8_t end = 0;
    static uint32_t end_timer = 0;
    static uint32_t master_timer = 0;

    master_timer = E_master_timer;
    end_timer = E_end_timer;
    while ((string[indexU8]) != 0) {
        temp_string[indexU8] = string[indexU8];
        status = DISPLAY_WriteData(temp_string[indexU8], TRUE);
        indexU8++;
    }
    memset(temp_string, 0, sizeof (temp_string));
    indexU8 = 0;
    return status;
}

int DISPLAY_PutChar(uint8_t character) {
    int status = 0;
    status = DISPLAY_WriteData((uint8_t) character, TRUE);
    return status;
}
