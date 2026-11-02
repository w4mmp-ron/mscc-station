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

#include "basic-plus.h"
#include "CharLCD_I2C.h"
#define MASTER_TIMER_VALUE 10000
#define CONTROL_TIMER_VALUE 10000
#define END_TIMER 10000

/* Stores the state of conponent. Indicates wherewer component is 
* in enabled state or not.
*/
uint8 DISPLAY_enableState = 0u;

uint8 DISPLAY_initVar = 0u;
uint8 New_position = FALSE;
uint8 Display_Initializing = FALSE;
uint32 E_master_timer = MASTER_TIMER_VALUE;
uint32 E_end_timer = END_TIMER;
uint32 E_control_timer = CONTROL_TIMER_VALUE;


/*******************************************************************************
* Function Name: DISPLAY_Init
********************************************************************************
*
* Summary:
*  Perform initialization required for components normal work.
*  This function initializes the LCD hardware module as follows:
*        Enable 4-bit interface
*        Clear the display
*        Enable auto cursor increment
*        Resets the cursor to start position
*  Also loads custom character set to LCD if it was defined in the customizer.
*
* Parameters:
*  None.
*
* Return:
*  None.
*
* Reentrant:
*  No.
*
*******************************************************************************/
void DISPLAY_Init(void) 
{
    /* INIT CODE */
    CyDelay(DISPLAY_INIT_DELAY);									/* Delay 20 ms */
    DISPLAY_WrtCntrlUpNib(DISPLAY_DISPLAY_8_BIT_INIT);   	/* Selects 8-bit mode */
    CyDelay(DISPLAY_INIT_UP_NIB_DELAY);							/* Delay 5 ms */
	DISPLAY_WrtCntrlUpNib(DISPLAY_DISPLAY_8_BIT_INIT);  	/* Selects 8-bit mode */                                                       /* Delay 5 ms */
    CyDelay(DISPLAY_INIT_UP_NIB_DELAY);							/* Delay 5 ms */
	DISPLAY_WrtCntrlUpNib(DISPLAY_DISPLAY_8_BIT_INIT);  	/* Selects 8-bit mode */    
	CyDelay(DISPLAY_INIT_UP_NIB_DELAY);							/* Delay 5 ms */
	DISPLAY_WrtCntrlUpNib(DISPLAY_DISPLAY_4_BIT_INIT);   	/* Selects 8-bit mode */    
	CyDelay(DISPLAY_INIT_UP_NIB_DELAY);							/* Delay 5 ms */

	DISPLAY_WriteControl(DISPLAY_DISPLAY_4_BIT_INIT,FALSE);     /* Write 4-bit Mode 2x16 or 4x20 Char */
	CyDelay(DISPLAY_INIT_CMD_DELAY);								/* Delay 5 ms */
    DISPLAY_WriteControl(DISPLAY_CLEAR_DISPLAY,FALSE);          /* Clear LCD Screen */
	CyDelay(DISPLAY_INIT_CMD_DELAY);								/* Delay 5 ms */
    DISPLAY_WriteControl(DISPLAY_CURSOR_AUTO_INCR_ON,FALSE);    /* Incr Cursor One Space to Right After Writes */
	CyDelay(DISPLAY_INIT_CMD_DELAY);								/* Delay 5 ms */
    DISPLAY_WriteControl(DISPLAY_DISPLAY_ON_CURSOR_OFF,FALSE);  /* Turn Display ON, Cursor OFF */
	CyDelay(DISPLAY_INIT_CMD_DELAY);								/* Delay 5 ms */

   
}


/*******************************************************************************
* Function Name: DISPLAY_Enable
********************************************************************************
*
* Summary:
*  Turns on the display.
*
* Parameters:
*  None.
*
* Return:
*  None.
*
* Reentrant:
*  No.
*
* Theory:
*  This finction has no effect when it called first time as
*  DISPLAY_Init() turns on the LCD.
*
*******************************************************************************/
/*void DISPLAY_Enable(void) 
{
    DISPLAY_DisplayOn();
    DISPLAY_enableState = 1u;
}*/


/*******************************************************************************
* Function Name: DISPLAY_Start
********************************************************************************
*
* Summary:
*  Perform initialization required for components normal work.
*  This function initializes the LCD hardware module as follows:
*        Enable 4-bit interface
*        Clear the display
*        Enable auto cursor increment
*        Resets the cursor to start position
*  Also loads custom character set to LCD if it was defined in the customizer.
*  If it was not the first call in this project then it just turns on the
*  display
*
*
* Parameters:
*  DISPLAY_initVar - global variable.
*
* Return:
*  DISPLAY_initVar - global variable.
*
* Reentrant:
*  No.
*
*******************************************************************************/
/*void DISPLAY_Start(void)
{
    
    if(DISPLAY_initVar == 0u)
    {
        DISPLAY_Init();
        DISPLAY_initVar = 1u;
    }

   
    DISPLAY_Enable();
}*/


/*******************************************************************************
* Function Name: DISPLAY_Stop
********************************************************************************
*
* Summary:
*  Turns off the display of the LCD screen.
*
* Parameters:
*  None.
*
* Return:
*  None.
*
* Reentrant:
*  No.
*
*******************************************************************************/
/*void DISPLAY_Stop(void) 
{
    
    DISPLAY_DisplayOff();
    DISPLAY_enableState = 0u;
}*/


/*******************************************************************************
*  Function Name: DISPLAY_Position
********************************************************************************
*
* Summary:
*  Moves active cursor location to a point specified by the input arguments
*
* Parameters:
*  row:     Specific row of LCD module to be written
*  column:  Column of LCD module to be written
*
* Return:
*  None.
*
* Note:
*  This only applies for LCD displays which use the 2X40 address mode.
*  This results in Row 2 offset from row one by 0x28.
*  When there are more than 2 rows, each row must be fewer than 20 characters.
*
*******************************************************************************/
uint8 DISPLAY_Position(uint8 row, uint8 column)
{
    
    static uint8 state = 0;
    uint8 status = OPERATION_PENDING;
    static uint32 control_timer = 0;
    
        switch(state){
            case 0:
            control_timer = E_control_timer;
                switch (row)
                {
                    case (uint8)0:
                        status = DISPLAY_WriteControl(DISPLAY_ROW_0_START + column,TRUE);
                        break;
                    case (uint8) 1:
                        status = DISPLAY_WriteControl(DISPLAY_ROW_1_START + column,TRUE);
                        break;
                    case (uint8) 2:
                        status = DISPLAY_WriteControl(DISPLAY_ROW_2_START + column,TRUE);
                        break;
                    case (uint8) 3:
                        status = DISPLAY_WriteControl(DISPLAY_ROW_3_START + column,TRUE);
                        break;
                    default:
                        /* if default case is hit, invalid row argument was passed.*/
                        break;
                }
                if(status == OPERATION_COMPLETED){
                    state = 1;
                    status = OPERATION_PENDING;
                }
                break;
            case 1:
                if(control_timer-- == 0){
                    state = 0;
                    status = OPERATION_COMPLETED;
                }
                break;
        }
    return status;
}


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
uint8 DISPLAY_PrintString(char8 const string[]) 
{
    static uint8 indexU8 = 0u;
    static char8 temp_string[17] = {0};
    static uint8 status = 0;
    static uint8 state = 0;
    static uint8 end = 0;
    static uint32 end_timer = 0;
    static uint32 master_timer = 0;
    
    
    switch(state){
        case 0:
            master_timer = E_master_timer;
            end_timer = E_end_timer;
            while((string[indexU8]) != 0){
                temp_string[indexU8] = string[indexU8];
                indexU8++;
            }
            end = indexU8;
            indexU8 = 0u;
            state++;
            status = OPERATION_PENDING;
            break;
        case 1:
            if(master_timer-- == 0){
                status = DISPLAY_WriteData(temp_string[indexU8],TRUE);
                if(status == OPERATION_COMPLETED){
                    indexU8++;
                    if(indexU8 == end){
                        state++;
                    }
                }
                master_timer = E_master_timer;
            }
            status = OPERATION_PENDING;
            break;
        case 2:
            if(end_timer-- == 0){
                memset(temp_string,0,sizeof(temp_string));
                indexU8 = 0;
                status = OPERATION_COMPLETED;
                state = 0;
            }
            break;
    }
    return status;
}


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
void DISPLAY_PutChar(char8 character) 
{
    DISPLAY_WriteData((uint8)character,TRUE);
}


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

uint8 DISPLAY_WriteData(uint8 dByte,uint8 wait) 
{
    uint8 status = OPERATION_COMPLETED;
    static uint8 state = 0;
    static uint8 buffer[DISPLAY_BUFFER_SIZE], UPPER_NIB, LOWER_NIB;
	static uint8 BLH_EH_RWL_RSH, BLH_EL_RWL_RSH;
	
    switch (state){
        case 0:
	        UPPER_NIB = dByte & DISPLAY_UPPER_NIB_MASK;
   	        LOWER_NIB = (dByte & DISPLAY_LOWER_NIB_MASK) << DISPLAY_LOWER_NIB_SHIFT;
	
	        BLH_EH_RWL_RSH = DISPLAY_BLH + DISPLAY_EH + DISPLAY_RSH;
	        BLH_EL_RWL_RSH = DISPLAY_BLH + DISPLAY_RSH;
	
	        /* Initialize buffer with packet */
   
	        buffer[DISPLAY_PACKET_0_POS] = UPPER_NIB | BLH_EH_RWL_RSH;
            buffer[DISPLAY_PACKET_1_POS] = UPPER_NIB | BLH_EL_RWL_RSH;
            buffer[DISPLAY_PACKET_2_POS] = LOWER_NIB | BLH_EH_RWL_RSH;
            buffer[DISPLAY_PACKET_3_POS] = LOWER_NIB | BLH_EL_RWL_RSH;
    
            (void) I2C_DISPLAY_MasterWriteBuf(E_display_addr, buffer, 2, 
                                  I2C_DISPLAY_MODE_COMPLETE_XFER);
            while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
            state = 1;
                status = OPERATION_PENDING;
                if(wait == TRUE){
                    break;
                }
        case 1:
            (void) I2C_DISPLAY_MasterWriteBuf(E_display_addr, &buffer[2], 2, 
                                  I2C_DISPLAY_MODE_COMPLETE_XFER);
            while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
            status = OPERATION_COMPLETED;
            state = 0;
            break;
    }
    //(void) I2C_DISPLAY_MasterWriteBuf(DISPLAY_I2C_SLAVE_ADDR, buffer, DISPLAY_PACKET_SIZE, 
     //                             I2C_DISPLAY_MODE_COMPLETE_XFER);
     //while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};

	//CyDelayUs(DISPLAY_DATA_DELAY_US);
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

uint8 DISPLAY_WriteControl(uint8 cByte,uint8 wait) 
{
    uint8 status = OPERATION_COMPLETED;
    static uint8 state = 0;
    static uint8 buffer[DISPLAY_BUFFER_SIZE], UPPER_NIB, LOWER_NIB;
	static uint8 BLH_EH_RWL_RSL, BLH_EL_RWL_RSL;
   
	switch(state){
        case 0:
	        UPPER_NIB = cByte & DISPLAY_UPPER_NIB_MASK;
   	        LOWER_NIB = (cByte & DISPLAY_LOWER_NIB_MASK) << DISPLAY_LOWER_NIB_SHIFT;
	
	        BLH_EH_RWL_RSL = DISPLAY_BLH + DISPLAY_EH;
	        BLH_EL_RWL_RSL = DISPLAY_BLH;
	
	        /* Initialize buffer with packet */
   
	        buffer[DISPLAY_PACKET_0_POS] = UPPER_NIB | BLH_EH_RWL_RSL;
            buffer[DISPLAY_PACKET_1_POS] = UPPER_NIB | BLH_EL_RWL_RSL;
            buffer[DISPLAY_PACKET_2_POS] = LOWER_NIB | BLH_EH_RWL_RSL;
            buffer[DISPLAY_PACKET_3_POS] = LOWER_NIB | BLH_EL_RWL_RSL;
     

            (void) I2C_DISPLAY_MasterWriteBuf(E_display_addr, buffer, 2, 
                                  I2C_DISPLAY_MODE_COMPLETE_XFER);
            while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
            state = 1;
            status = OPERATION_PENDING;
            if(wait == TRUE){
                break;
            }
        case 1:
            (void) I2C_DISPLAY_MasterWriteBuf(E_display_addr, &buffer[2], 2, 
                                  I2C_DISPLAY_MODE_COMPLETE_XFER);
            while((I2C_DISPLAY_MasterStatus() & I2C_DISPLAY_MSTAT_WR_CMPLT) == 0u){};
            status = OPERATION_COMPLETED;
            state = 0;
            break;
    }
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
void DISPLAY_WrtCntrlUpNib(uint8 cByte)
{
    uint8 status = OPERATION_COMPLETED;
    uint8 buffer[DISPLAY_UPPER_NIB_BUFFER_SIZE], UPPER_NIB;
	uint8 BLH_EH_RWL_RSL, BLH_EL_RWL_RSL;
   
	
	UPPER_NIB = cByte & DISPLAY_UPPER_NIB_MASK;
	
	BLH_EH_RWL_RSL = DISPLAY_BLH + DISPLAY_EH;
	BLH_EL_RWL_RSL = DISPLAY_BLH;
	
	/* Initialize buffer with packet */
   
	buffer[DISPLAY_PACKET_0_POS] = UPPER_NIB | BLH_EH_RWL_RSL;
    buffer[DISPLAY_PACKET_1_POS] = UPPER_NIB | BLH_EL_RWL_RSL;
     

   	
	(void) I2C_DISPLAY_MasterWriteBuf(E_display_addr, buffer, DISPLAY_UPPER_NIB_PACKET_SIZE, \
                                  I2C_DISPLAY_MODE_COMPLETE_XFER);
    
	CyDelayUs(DISPLAY_CMD_DELAY_US);

}
/*******************************************************************************
* Function Name: DISPLAY_IsReady
********************************************************************************
*
* Summary:
*  Polls LCD until the ready bit is set.
*
* Parameters:
*  None.
*
* Return:
*  None.
*
* Note:
*  Changes pins to High-Z.
*
*******************************************************************************/
/*void DISPLAY_IsReady(void) 
{
   
	CyDelay(1u);
	
}*/




/* [] END OF FILE */
