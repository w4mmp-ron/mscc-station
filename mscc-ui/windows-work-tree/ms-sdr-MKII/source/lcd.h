/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   lcd.h
 * Author: mscc
 *
 * Created on January 3, 2022, 8:58 AM
 */

#ifndef LCD_H
#define LCD_H

#ifdef __cplusplus
extern "C" {
#endif




#ifdef __cplusplus
}
#endif

#endif /* LCD_H */

/*******************************************************************************
* File Name: DISPLAY_I2C.h
* Version `$CY_MAJOR_VERSION`.`$CY_MINOR_VERSION`
*
* Description:
*  This header file contains registers and constants associated with the
* CharLCD_I2C component.
*
* Note:
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/



/***************************************
*   Conditional Compilation Parameters
***************************************/

#define DISPLAY_CONVERSION_ROUTINES     (`$ConversionRoutines_DEF`u)
#define DISPLAY_CUSTOM_CHAR_SET         (`$CustomCharDefines_API_GEN`u)

/* Custom character set types */
#define DISPLAY_NONE                     (0u)    /* No Custom Fonts      */
#define DISPLAY_HORIZONTAL_BG            (1u)    /* Horizontal Bar Graph */
#define DISPLAY_VERTICAL_BG              (2u)    /* Vertical Bar Graph   */
#define DISPLAY_USER_DEFINED             (3u)    /* User Defined Fonts   */


/***************************************
*     Data Struct Definitions
***************************************/

/* Sleep Mode API Support */
typedef struct
{
    uint8_t enableState;
} 
DISPLAY_BACKUP_STRUCT;

#define DISPLAY_PosPrintString(row, col, string) DISPLAY_Position(row, col); DISPLAY_PrintString(string); 
#define DISPLAY_PosPutChar(row, col, character) DISPLAY_Position(row, col); DISPLAY_PutChar(character); 
/* Clear Macro */
#define DISPLAY_ClearDisplay() DISPLAY_WriteControl(DISPLAY_CLEAR_DISPLAY,FALSE)
/* Off Macro */
#define DISPLAY_DisplayOff() DISPLAY_WriteControl(DISPLAY_DISPLAY_CURSOR_OFF,FALSE)
/* On Macro */
#define DISPLAY_DisplayOn() DISPLAY_WriteControl(DISPLAY_DISPLAY_ON_CURSOR_OFF,FALSE)


/***************************************
*           Global Variables
***************************************/

//extern uint8_t DISPLAY_initVar;
//extern uint8_t DISPLAY_enableState;
//extern uint8_t const DISPLAY_customFonts[64u];


/***************************************
*           API Constants
***************************************/

/* Full Byte Commands Sent as Two Nibbles */

/* 2 Lines 5x7 Characters 8 bit mode*/
#define DISPLAY_DISPLAY_8_BIT_INIT       (0x38u)
/* 2 lines 5x7 Charcaters 4 bit mode*/
#define DISPLAY_DISPLAY_4_BIT_INIT       (0x28u)

#define DISPLAY_DISPLAY_CURSOR_OFF       (0x08u)
#define DISPLAY_CLEAR_DISPLAY            (0x01u)
#define DISPLAY_CURSOR_AUTO_INCR_ON      (0x06u)
#define DISPLAY_DISPLAY_CURSOR_ON        (0x0Eu)
#define DISPLAY_DISPLAY_ON_CURSOR_OFF    (0x0Cu)
#define DISPLAY_DISPLAY_2_LINES_5x10     (0x2Cu)

#define DISPLAY_RESET_CURSOR_POSITION    (0x03u)
#define DISPLAY_CURSOR_WINK              (0x0Du)
#define DISPLAY_CURSOR_BLINK             (0x0Fu)
#define DISPLAY_CURSOR_SH_LEFT           (0x10u)
#define DISPLAY_CURSOR_SH_RIGHT          (0x14u)
#define DISPLAY_CURSOR_HOME              (0x02u)
#define DISPLAY_CURSOR_LEFT              (0x04u)
#define DISPLAY_CURSOR_RIGHT             (0x06u)

/* Point to Character Generator Ram 0 */
#define DISPLAY_CGRAM_0                  (0x40u)

/* Point to Display Data Ram 0 */
#define DISPLAY_DDRAM_0                  (0x80u)

/* LCD Characteristics */
#define DISPLAY_CHARACTER_WIDTH          (0x05u)
#define DISPLAY_CHARACTER_HEIGHT         (0x08u)

/* Nibble Offset and Mask */
#define DISPLAY_NIBBLE_SHIFT             (0x04u)
#define DISPLAY_NIBBLE_MASK              (0x0Fu)

/* LCD Module Address Constants */
#define DISPLAY_ROW_0_START              (0x80u)
#define DISPLAY_ROW_1_START              (0xC0u)
#define DISPLAY_ROW_2_START              (0x94u)
#define DISPLAY_ROW_3_START              (0xD4u)

/* Custom Character References */
#define DISPLAY_CUSTOM_0                 (0x00u)
#define DISPLAY_CUSTOM_1                 (0x01u)
#define DISPLAY_CUSTOM_2                 (0x02u)
#define DISPLAY_CUSTOM_3                 (0x03u)
#define DISPLAY_CUSTOM_4                 (0x04u)
#define DISPLAY_CUSTOM_5                 (0x05u)
#define DISPLAY_CUSTOM_6                 (0x06u)
#define DISPLAY_CUSTOM_7                 (0x07u)

/* Other constants */
#define DISPLAY_BYTE_UPPER_NIBBLE_SHIFT  (0x04u)
#define DISPLAY_BYTE_LOWER_NIBBLE_MASK   (0x0Fu)
#define DISPLAY_U16_UPPER_BYTE_SHIFT     (0x08u)
#define DISPLAY_U16_LOWER_BYTE_MASK      (0xFFu)
#define DISPLAY_CUSTOM_CHAR_SET_LEN      (0x40u)

#define DISPLAY_CMD_DELAY_US                        (2000)/* 2.0 msec Delay */
//#define DISPLAY_DATA_DELAY_US                       (500)/*  0.50 msec Delay */
//#define DISPLAY_NIB_DELAY_US                        (100)/*  0.10 msec Delay */
#define DISPLAY_INIT_DELAY                          (20000) /*  20.0 msec Delay */
#define DISPLAY_INIT_UP_NIB_DELAY                   (5000) /*   5.0 msec Delay */
#define DISPLAY_INIT_CMD_DELAY                      (5000) /*   5.0 msec Delay */

/**************************************
*             Registers               *
***************************************/
/* I2C 7 bit slave address for the PCF8574A A0=1,A1=1,A2=1*/



/* Buffer and packet size */
#define DISPLAY_UPPER_NIB_BUFFER_SIZE	(2u)
#define DISPLAY_UPPER_NIB_PACKET_SIZE	(DISPLAY_UPPER_NIB_BUFFER_SIZE)
#define DISPLAY_BUFFER_SIZE     		(4u)
#define DISPLAY_PACKET_SIZE     		(DISPLAY_BUFFER_SIZE)

/* Packet positions */
#define DISPLAY_PACKET_0_POS  (0u)
#define DISPLAY_PACKET_1_POS  (1u)
#define DISPLAY_PACKET_2_POS  (2u)
#define DISPLAY_PACKET_3_POS  (3u)

/* Nibble Offset and Mask */

#define DISPLAY_LOWER_NIB_SHIFT        (0x04u)
#define DISPLAY_LOWER_NIB_MASK         (0x0Fu)
#define DISPLAY_UPPER_NIB_MASK         (0xF0u)

/* I2C LOGIC CONSTANTS */

#define DISPLAY_BLH				    (0x08u)/* Back Light On */
#define DISPLAY_EH					(0x04u)/* E Input High */
#define DISPLAY_RWH				    (0x02u)/* RW Input High */
#define DISPLAY_RSH				    (0x01u)/* RS Input High */


#define OPERATION_COMPLETED 3
#define OPERATION_PENDING 2 
#define OPERATION_STARTED 1
#define OPERATION_IDLE 0
#define OPERATION_FAILED 4


/* [] END OF FILE */


