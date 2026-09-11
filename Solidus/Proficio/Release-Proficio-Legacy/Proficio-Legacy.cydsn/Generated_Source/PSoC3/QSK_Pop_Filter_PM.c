/*******************************************************************************
* File Name: QSK_Pop_Filter_PM.c
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

#include "QSK_Pop_Filter.h"

static QSK_Pop_Filter_backupStruct QSK_Pop_Filter_backup;


/*******************************************************************************
* Function Name: QSK_Pop_Filter_SaveConfig
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
*  QSK_Pop_Filter_backup:  Variables of this global structure are modified to
*  store the values of non retention configuration registers when Sleep() API is
*  called.
*
*******************************************************************************/
void QSK_Pop_Filter_SaveConfig(void) 
{
    #if (!QSK_Pop_Filter_UsingFixedFunction)
        QSK_Pop_Filter_backup.TimerUdb = QSK_Pop_Filter_ReadCounter();
        QSK_Pop_Filter_backup.InterruptMaskValue = QSK_Pop_Filter_STATUS_MASK;
        #if (QSK_Pop_Filter_UsingHWCaptureCounter)
            QSK_Pop_Filter_backup.TimerCaptureCounter = QSK_Pop_Filter_ReadCaptureCount();
        #endif /* Back Up capture counter register  */

        #if(!QSK_Pop_Filter_UDB_CONTROL_REG_REMOVED)
            QSK_Pop_Filter_backup.TimerControlRegister = QSK_Pop_Filter_ReadControlRegister();
        #endif /* Backup the enable state of the Timer component */
    #endif /* Backup non retention registers in UDB implementation. All fixed function registers are retention */
}


/*******************************************************************************
* Function Name: QSK_Pop_Filter_RestoreConfig
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
*  QSK_Pop_Filter_backup:  Variables of this global structure are used to
*  restore the values of non retention registers on wakeup from sleep mode.
*
*******************************************************************************/
void QSK_Pop_Filter_RestoreConfig(void) 
{   
    #if (!QSK_Pop_Filter_UsingFixedFunction)

        QSK_Pop_Filter_WriteCounter(QSK_Pop_Filter_backup.TimerUdb);
        QSK_Pop_Filter_STATUS_MASK =QSK_Pop_Filter_backup.InterruptMaskValue;
        #if (QSK_Pop_Filter_UsingHWCaptureCounter)
            QSK_Pop_Filter_SetCaptureCount(QSK_Pop_Filter_backup.TimerCaptureCounter);
        #endif /* Restore Capture counter register*/

        #if(!QSK_Pop_Filter_UDB_CONTROL_REG_REMOVED)
            QSK_Pop_Filter_WriteControlRegister(QSK_Pop_Filter_backup.TimerControlRegister);
        #endif /* Restore the enable state of the Timer component */
    #endif /* Restore non retention registers in the UDB implementation only */
}


/*******************************************************************************
* Function Name: QSK_Pop_Filter_Sleep
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
*  QSK_Pop_Filter_backup.TimerEnableState:  Is modified depending on the
*  enable state of the block before entering sleep mode.
*
*******************************************************************************/
void QSK_Pop_Filter_Sleep(void) 
{
    #if(!QSK_Pop_Filter_UDB_CONTROL_REG_REMOVED)
        /* Save Counter's enable state */
        if(QSK_Pop_Filter_CTRL_ENABLE == (QSK_Pop_Filter_CONTROL & QSK_Pop_Filter_CTRL_ENABLE))
        {
            /* Timer is enabled */
            QSK_Pop_Filter_backup.TimerEnableState = 1u;
        }
        else
        {
            /* Timer is disabled */
            QSK_Pop_Filter_backup.TimerEnableState = 0u;
        }
    #endif /* Back up enable state from the Timer control register */
    QSK_Pop_Filter_Stop();
    QSK_Pop_Filter_SaveConfig();
}


/*******************************************************************************
* Function Name: QSK_Pop_Filter_Wakeup
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
*  QSK_Pop_Filter_backup.enableState:  Is used to restore the enable state of
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void QSK_Pop_Filter_Wakeup(void) 
{
    QSK_Pop_Filter_RestoreConfig();
    #if(!QSK_Pop_Filter_UDB_CONTROL_REG_REMOVED)
        if(QSK_Pop_Filter_backup.TimerEnableState == 1u)
        {     /* Enable Timer's operation */
                QSK_Pop_Filter_Enable();
        } /* Do nothing if Timer was disabled before */
    #endif /* Remove this code section if Control register is removed */
}


/* [] END OF FILE */
