/*******************************************************************************
* File Name: Key_Delay_Timer_PM.c
* Version 2.80
*
*  Description:
*     This file provides the power management source code to API for the
*     Timer.
*
*   Note:
*     None
*
*******************************************************************************
* Copyright 2008-2017, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
********************************************************************************/

#include "Key_Delay_Timer.h"

static Key_Delay_Timer_backupStruct Key_Delay_Timer_backup;


/*******************************************************************************
* Function Name: Key_Delay_Timer_SaveConfig
********************************************************************************
*
* Summary:
*     Save the current user configuration
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  Key_Delay_Timer_backup:  Variables of this global structure are modified to
*  store the values of non retention configuration registers when Sleep() API is
*  called.
*
*******************************************************************************/
void Key_Delay_Timer_SaveConfig(void) 
{
    #if (!Key_Delay_Timer_UsingFixedFunction)
        Key_Delay_Timer_backup.TimerUdb = Key_Delay_Timer_ReadCounter();
        Key_Delay_Timer_backup.InterruptMaskValue = Key_Delay_Timer_STATUS_MASK;
        #if (Key_Delay_Timer_UsingHWCaptureCounter)
            Key_Delay_Timer_backup.TimerCaptureCounter = Key_Delay_Timer_ReadCaptureCount();
        #endif /* Back Up capture counter register  */

        #if(!Key_Delay_Timer_UDB_CONTROL_REG_REMOVED)
            Key_Delay_Timer_backup.TimerControlRegister = Key_Delay_Timer_ReadControlRegister();
        #endif /* Backup the enable state of the Timer component */
    #endif /* Backup non retention registers in UDB implementation. All fixed function registers are retention */
}


/*******************************************************************************
* Function Name: Key_Delay_Timer_RestoreConfig
********************************************************************************
*
* Summary:
*  Restores the current user configuration.
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  Key_Delay_Timer_backup:  Variables of this global structure are used to
*  restore the values of non retention registers on wakeup from sleep mode.
*
*******************************************************************************/
void Key_Delay_Timer_RestoreConfig(void) 
{   
    #if (!Key_Delay_Timer_UsingFixedFunction)

        Key_Delay_Timer_WriteCounter(Key_Delay_Timer_backup.TimerUdb);
        Key_Delay_Timer_STATUS_MASK =Key_Delay_Timer_backup.InterruptMaskValue;
        #if (Key_Delay_Timer_UsingHWCaptureCounter)
            Key_Delay_Timer_SetCaptureCount(Key_Delay_Timer_backup.TimerCaptureCounter);
        #endif /* Restore Capture counter register*/

        #if(!Key_Delay_Timer_UDB_CONTROL_REG_REMOVED)
            Key_Delay_Timer_WriteControlRegister(Key_Delay_Timer_backup.TimerControlRegister);
        #endif /* Restore the enable state of the Timer component */
    #endif /* Restore non retention registers in the UDB implementation only */
}


/*******************************************************************************
* Function Name: Key_Delay_Timer_Sleep
********************************************************************************
*
* Summary:
*     Stop and Save the user configuration
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  Key_Delay_Timer_backup.TimerEnableState:  Is modified depending on the
*  enable state of the block before entering sleep mode.
*
*******************************************************************************/
void Key_Delay_Timer_Sleep(void) 
{
    #if(!Key_Delay_Timer_UDB_CONTROL_REG_REMOVED)
        /* Save Counter's enable state */
        if(Key_Delay_Timer_CTRL_ENABLE == (Key_Delay_Timer_CONTROL & Key_Delay_Timer_CTRL_ENABLE))
        {
            /* Timer is enabled */
            Key_Delay_Timer_backup.TimerEnableState = 1u;
        }
        else
        {
            /* Timer is disabled */
            Key_Delay_Timer_backup.TimerEnableState = 0u;
        }
    #endif /* Back up enable state from the Timer control register */
    Key_Delay_Timer_Stop();
    Key_Delay_Timer_SaveConfig();
}


/*******************************************************************************
* Function Name: Key_Delay_Timer_Wakeup
********************************************************************************
*
* Summary:
*  Restores and enables the user configuration
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  Key_Delay_Timer_backup.enableState:  Is used to restore the enable state of
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void Key_Delay_Timer_Wakeup(void) 
{
    Key_Delay_Timer_RestoreConfig();
    #if(!Key_Delay_Timer_UDB_CONTROL_REG_REMOVED)
        if(Key_Delay_Timer_backup.TimerEnableState == 1u)
        {     /* Enable Timer's operation */
                Key_Delay_Timer_Enable();
        } /* Do nothing if Timer was disabled before */
    #endif /* Remove this code section if Control register is removed */
}


/* [] END OF FILE */
