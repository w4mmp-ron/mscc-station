/*******************************************************************************
* File Name: QSK_Pop_Filter.h
* Version 2.80
*
*  Description:
*     Contains the function prototypes and constants available to the timer
*     user module.
*
*   Note:
*     None
*
********************************************************************************
* Copyright 2008-2017, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
********************************************************************************/

#if !defined(CY_TIMER_QSK_Pop_Filter_H)
#define CY_TIMER_QSK_Pop_Filter_H

#include "cytypes.h"
#include "cyfitter.h"
#include "CyLib.h" /* For CyEnterCriticalSection() and CyExitCriticalSection() functions */

extern uint8 QSK_Pop_Filter_initVar;

/* Check to see if required defines such as CY_PSOC5LP are available */
/* They are defined starting with cy_boot v3.0 */
#if !defined (CY_PSOC5LP)
    #error Component Timer_v2_80 requires cy_boot v3.0 or later
#endif /* (CY_ PSOC5LP) */


/**************************************
*           Parameter Defaults
**************************************/

#define QSK_Pop_Filter_Resolution                 16u
#define QSK_Pop_Filter_UsingFixedFunction         1u
#define QSK_Pop_Filter_UsingHWCaptureCounter      0u
#define QSK_Pop_Filter_SoftwareCaptureMode        0u
#define QSK_Pop_Filter_SoftwareTriggerMode        0u
#define QSK_Pop_Filter_UsingHWEnable              0u
#define QSK_Pop_Filter_EnableTriggerMode          0u
#define QSK_Pop_Filter_InterruptOnCaptureCount    0u
#define QSK_Pop_Filter_RunModeUsed                1u
#define QSK_Pop_Filter_ControlRegRemoved          0u

#if defined(QSK_Pop_Filter_TimerUDB_sCTRLReg_SyncCtl_ctrlreg__CONTROL_REG)
    #define QSK_Pop_Filter_UDB_CONTROL_REG_REMOVED            (0u)
#elif  (QSK_Pop_Filter_UsingFixedFunction)
    #define QSK_Pop_Filter_UDB_CONTROL_REG_REMOVED            (0u)
#else 
    #define QSK_Pop_Filter_UDB_CONTROL_REG_REMOVED            (1u)
#endif /* End QSK_Pop_Filter_TimerUDB_sCTRLReg_SyncCtl_ctrlreg__CONTROL_REG */


/***************************************
*       Type defines
***************************************/


/**************************************************************************
 * Sleep Wakeup Backup structure for Timer Component
 *************************************************************************/
typedef struct
{
    uint8 TimerEnableState;
    #if(!QSK_Pop_Filter_UsingFixedFunction)

        uint16 TimerUdb;
        uint8 InterruptMaskValue;
        #if (QSK_Pop_Filter_UsingHWCaptureCounter)
            uint8 TimerCaptureCounter;
        #endif /* variable declarations for backing up non retention registers in CY_UDB_V1 */

        #if (!QSK_Pop_Filter_UDB_CONTROL_REG_REMOVED)
            uint8 TimerControlRegister;
        #endif /* variable declaration for backing up enable state of the Timer */
    #endif /* define backup variables only for UDB implementation. Fixed function registers are all retention */

}QSK_Pop_Filter_backupStruct;


/***************************************
*       Function Prototypes
***************************************/

void    QSK_Pop_Filter_Start(void) ;
void    QSK_Pop_Filter_Stop(void) ;

void    QSK_Pop_Filter_SetInterruptMode(uint8 interruptMode) ;
uint8   QSK_Pop_Filter_ReadStatusRegister(void) ;
/* Deprecated function. Do not use this in future. Retained for backward compatibility */
#define QSK_Pop_Filter_GetInterruptSource() QSK_Pop_Filter_ReadStatusRegister()

#if(!QSK_Pop_Filter_UDB_CONTROL_REG_REMOVED)
    uint8   QSK_Pop_Filter_ReadControlRegister(void) ;
    void    QSK_Pop_Filter_WriteControlRegister(uint8 control) ;
#endif /* (!QSK_Pop_Filter_UDB_CONTROL_REG_REMOVED) */

uint16  QSK_Pop_Filter_ReadPeriod(void) ;
void    QSK_Pop_Filter_WritePeriod(uint16 period) ;
uint16  QSK_Pop_Filter_ReadCounter(void) ;
void    QSK_Pop_Filter_WriteCounter(uint16 counter) ;
uint16  QSK_Pop_Filter_ReadCapture(void) ;
void    QSK_Pop_Filter_SoftwareCapture(void) ;

#if(!QSK_Pop_Filter_UsingFixedFunction) /* UDB Prototypes */
    #if (QSK_Pop_Filter_SoftwareCaptureMode)
        void    QSK_Pop_Filter_SetCaptureMode(uint8 captureMode) ;
    #endif /* (!QSK_Pop_Filter_UsingFixedFunction) */

    #if (QSK_Pop_Filter_SoftwareTriggerMode)
        void    QSK_Pop_Filter_SetTriggerMode(uint8 triggerMode) ;
    #endif /* (QSK_Pop_Filter_SoftwareTriggerMode) */

    #if (QSK_Pop_Filter_EnableTriggerMode)
        void    QSK_Pop_Filter_EnableTrigger(void) ;
        void    QSK_Pop_Filter_DisableTrigger(void) ;
    #endif /* (QSK_Pop_Filter_EnableTriggerMode) */


    #if(QSK_Pop_Filter_InterruptOnCaptureCount)
        void    QSK_Pop_Filter_SetInterruptCount(uint8 interruptCount) ;
    #endif /* (QSK_Pop_Filter_InterruptOnCaptureCount) */

    #if (QSK_Pop_Filter_UsingHWCaptureCounter)
        void    QSK_Pop_Filter_SetCaptureCount(uint8 captureCount) ;
        uint8   QSK_Pop_Filter_ReadCaptureCount(void) ;
    #endif /* (QSK_Pop_Filter_UsingHWCaptureCounter) */

    void QSK_Pop_Filter_ClearFIFO(void) ;
#endif /* UDB Prototypes */

/* Sleep Retention APIs */
void QSK_Pop_Filter_Init(void)          ;
void QSK_Pop_Filter_Enable(void)        ;
void QSK_Pop_Filter_SaveConfig(void)    ;
void QSK_Pop_Filter_RestoreConfig(void) ;
void QSK_Pop_Filter_Sleep(void)         ;
void QSK_Pop_Filter_Wakeup(void)        ;


/***************************************
*   Enumerated Types and Parameters
***************************************/

/* Enumerated Type B_Timer__CaptureModes, Used in Capture Mode */
#define QSK_Pop_Filter__B_TIMER__CM_NONE 0
#define QSK_Pop_Filter__B_TIMER__CM_RISINGEDGE 1
#define QSK_Pop_Filter__B_TIMER__CM_FALLINGEDGE 2
#define QSK_Pop_Filter__B_TIMER__CM_EITHEREDGE 3
#define QSK_Pop_Filter__B_TIMER__CM_SOFTWARE 4



/* Enumerated Type B_Timer__TriggerModes, Used in Trigger Mode */
#define QSK_Pop_Filter__B_TIMER__TM_NONE 0x00u
#define QSK_Pop_Filter__B_TIMER__TM_RISINGEDGE 0x04u
#define QSK_Pop_Filter__B_TIMER__TM_FALLINGEDGE 0x08u
#define QSK_Pop_Filter__B_TIMER__TM_EITHEREDGE 0x0Cu
#define QSK_Pop_Filter__B_TIMER__TM_SOFTWARE 0x10u


/***************************************
*    Initialial Parameter Constants
***************************************/

#define QSK_Pop_Filter_INIT_PERIOD             49u
#define QSK_Pop_Filter_INIT_CAPTURE_MODE       ((uint8)((uint8)0u << QSK_Pop_Filter_CTRL_CAP_MODE_SHIFT))
#define QSK_Pop_Filter_INIT_TRIGGER_MODE       ((uint8)((uint8)0u << QSK_Pop_Filter_CTRL_TRIG_MODE_SHIFT))
#if (QSK_Pop_Filter_UsingFixedFunction)
    #define QSK_Pop_Filter_INIT_INTERRUPT_MODE (((uint8)((uint8)1u << QSK_Pop_Filter_STATUS_TC_INT_MASK_SHIFT)) | \
                                                  ((uint8)((uint8)0 << QSK_Pop_Filter_STATUS_CAPTURE_INT_MASK_SHIFT)))
#else
    #define QSK_Pop_Filter_INIT_INTERRUPT_MODE (((uint8)((uint8)1u << QSK_Pop_Filter_STATUS_TC_INT_MASK_SHIFT)) | \
                                                 ((uint8)((uint8)0 << QSK_Pop_Filter_STATUS_CAPTURE_INT_MASK_SHIFT)) | \
                                                 ((uint8)((uint8)0 << QSK_Pop_Filter_STATUS_FIFOFULL_INT_MASK_SHIFT)))
#endif /* (QSK_Pop_Filter_UsingFixedFunction) */
#define QSK_Pop_Filter_INIT_CAPTURE_COUNT      (2u)
#define QSK_Pop_Filter_INIT_INT_CAPTURE_COUNT  ((uint8)((uint8)(1u - 1u) << QSK_Pop_Filter_CTRL_INTCNT_SHIFT))


/***************************************
*           Registers
***************************************/

#if (QSK_Pop_Filter_UsingFixedFunction) /* Implementation Specific Registers and Register Constants */


    /***************************************
    *    Fixed Function Registers
    ***************************************/

    #define QSK_Pop_Filter_STATUS         (*(reg8 *) QSK_Pop_Filter_TimerHW__SR0 )
    /* In Fixed Function Block Status and Mask are the same register */
    #define QSK_Pop_Filter_STATUS_MASK    (*(reg8 *) QSK_Pop_Filter_TimerHW__SR0 )
    #define QSK_Pop_Filter_CONTROL        (*(reg8 *) QSK_Pop_Filter_TimerHW__CFG0)
    #define QSK_Pop_Filter_CONTROL2       (*(reg8 *) QSK_Pop_Filter_TimerHW__CFG1)
    #define QSK_Pop_Filter_CONTROL2_PTR   ( (reg8 *) QSK_Pop_Filter_TimerHW__CFG1)
    #define QSK_Pop_Filter_RT1            (*(reg8 *) QSK_Pop_Filter_TimerHW__RT1)
    #define QSK_Pop_Filter_RT1_PTR        ( (reg8 *) QSK_Pop_Filter_TimerHW__RT1)

    #if (CY_PSOC3 || CY_PSOC5LP)
        #define QSK_Pop_Filter_CONTROL3       (*(reg8 *) QSK_Pop_Filter_TimerHW__CFG2)
        #define QSK_Pop_Filter_CONTROL3_PTR   ( (reg8 *) QSK_Pop_Filter_TimerHW__CFG2)
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */
    #define QSK_Pop_Filter_GLOBAL_ENABLE  (*(reg8 *) QSK_Pop_Filter_TimerHW__PM_ACT_CFG)
    #define QSK_Pop_Filter_GLOBAL_STBY_ENABLE  (*(reg8 *) QSK_Pop_Filter_TimerHW__PM_STBY_CFG)

    #define QSK_Pop_Filter_CAPTURE_LSB         (* (reg16 *) QSK_Pop_Filter_TimerHW__CAP0 )
    #define QSK_Pop_Filter_CAPTURE_LSB_PTR       ((reg16 *) QSK_Pop_Filter_TimerHW__CAP0 )
    #define QSK_Pop_Filter_PERIOD_LSB          (* (reg16 *) QSK_Pop_Filter_TimerHW__PER0 )
    #define QSK_Pop_Filter_PERIOD_LSB_PTR        ((reg16 *) QSK_Pop_Filter_TimerHW__PER0 )
    #define QSK_Pop_Filter_COUNTER_LSB         (* (reg16 *) QSK_Pop_Filter_TimerHW__CNT_CMP0 )
    #define QSK_Pop_Filter_COUNTER_LSB_PTR       ((reg16 *) QSK_Pop_Filter_TimerHW__CNT_CMP0 )


    /***************************************
    *    Register Constants
    ***************************************/

    /* Fixed Function Block Chosen */
    #define QSK_Pop_Filter_BLOCK_EN_MASK                     QSK_Pop_Filter_TimerHW__PM_ACT_MSK
    #define QSK_Pop_Filter_BLOCK_STBY_EN_MASK                QSK_Pop_Filter_TimerHW__PM_STBY_MSK

    /* Control Register Bit Locations */
    /* Interrupt Count - Not valid for Fixed Function Block */
    #define QSK_Pop_Filter_CTRL_INTCNT_SHIFT                  0x00u
    /* Trigger Polarity - Not valid for Fixed Function Block */
    #define QSK_Pop_Filter_CTRL_TRIG_MODE_SHIFT               0x00u
    /* Trigger Enable - Not valid for Fixed Function Block */
    #define QSK_Pop_Filter_CTRL_TRIG_EN_SHIFT                 0x00u
    /* Capture Polarity - Not valid for Fixed Function Block */
    #define QSK_Pop_Filter_CTRL_CAP_MODE_SHIFT                0x00u
    /* Timer Enable - As defined in Register Map, part of TMRX_CFG0 register */
    #define QSK_Pop_Filter_CTRL_ENABLE_SHIFT                  0x00u

    /* Control Register Bit Masks */
    #define QSK_Pop_Filter_CTRL_ENABLE                        ((uint8)((uint8)0x01u << QSK_Pop_Filter_CTRL_ENABLE_SHIFT))

    /* Control2 Register Bit Masks */
    /* As defined in Register Map, Part of the TMRX_CFG1 register */
    #define QSK_Pop_Filter_CTRL2_IRQ_SEL_SHIFT                 0x00u
    #define QSK_Pop_Filter_CTRL2_IRQ_SEL                      ((uint8)((uint8)0x01u << QSK_Pop_Filter_CTRL2_IRQ_SEL_SHIFT))

    #if (CY_PSOC5A)
        /* Use CFG1 Mode bits to set run mode */
        /* As defined by Verilog Implementation */
        #define QSK_Pop_Filter_CTRL_MODE_SHIFT                 0x01u
        #define QSK_Pop_Filter_CTRL_MODE_MASK                 ((uint8)((uint8)0x07u << QSK_Pop_Filter_CTRL_MODE_SHIFT))
    #endif /* (CY_PSOC5A) */
    #if (CY_PSOC3 || CY_PSOC5LP)
        /* Control3 Register Bit Locations */
        #define QSK_Pop_Filter_CTRL_RCOD_SHIFT        0x02u
        #define QSK_Pop_Filter_CTRL_ENBL_SHIFT        0x00u
        #define QSK_Pop_Filter_CTRL_MODE_SHIFT        0x00u

        /* Control3 Register Bit Masks */
        #define QSK_Pop_Filter_CTRL_RCOD_MASK  ((uint8)((uint8)0x03u << QSK_Pop_Filter_CTRL_RCOD_SHIFT)) /* ROD and COD bit masks */
        #define QSK_Pop_Filter_CTRL_ENBL_MASK  ((uint8)((uint8)0x80u << QSK_Pop_Filter_CTRL_ENBL_SHIFT)) /* HW_EN bit mask */
        #define QSK_Pop_Filter_CTRL_MODE_MASK  ((uint8)((uint8)0x03u << QSK_Pop_Filter_CTRL_MODE_SHIFT)) /* Run mode bit mask */

        #define QSK_Pop_Filter_CTRL_RCOD       ((uint8)((uint8)0x03u << QSK_Pop_Filter_CTRL_RCOD_SHIFT))
        #define QSK_Pop_Filter_CTRL_ENBL       ((uint8)((uint8)0x80u << QSK_Pop_Filter_CTRL_ENBL_SHIFT))
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */

    /*RT1 Synch Constants: Applicable for PSoC3 and PSoC5LP */
    #define QSK_Pop_Filter_RT1_SHIFT                       0x04u
    /* Sync TC and CMP bit masks */
    #define QSK_Pop_Filter_RT1_MASK                        ((uint8)((uint8)0x03u << QSK_Pop_Filter_RT1_SHIFT))
    #define QSK_Pop_Filter_SYNC                            ((uint8)((uint8)0x03u << QSK_Pop_Filter_RT1_SHIFT))
    #define QSK_Pop_Filter_SYNCDSI_SHIFT                   0x00u
    /* Sync all DSI inputs with Mask  */
    #define QSK_Pop_Filter_SYNCDSI_MASK                    ((uint8)((uint8)0x0Fu << QSK_Pop_Filter_SYNCDSI_SHIFT))
    /* Sync all DSI inputs */
    #define QSK_Pop_Filter_SYNCDSI_EN                      ((uint8)((uint8)0x0Fu << QSK_Pop_Filter_SYNCDSI_SHIFT))

    #define QSK_Pop_Filter_CTRL_MODE_PULSEWIDTH            ((uint8)((uint8)0x01u << QSK_Pop_Filter_CTRL_MODE_SHIFT))
    #define QSK_Pop_Filter_CTRL_MODE_PERIOD                ((uint8)((uint8)0x02u << QSK_Pop_Filter_CTRL_MODE_SHIFT))
    #define QSK_Pop_Filter_CTRL_MODE_CONTINUOUS            ((uint8)((uint8)0x00u << QSK_Pop_Filter_CTRL_MODE_SHIFT))

    /* Status Register Bit Locations */
    /* As defined in Register Map, part of TMRX_SR0 register */
    #define QSK_Pop_Filter_STATUS_TC_SHIFT                 0x07u
    /* As defined in Register Map, part of TMRX_SR0 register, Shared with Compare Status */
    #define QSK_Pop_Filter_STATUS_CAPTURE_SHIFT            0x06u
    /* As defined in Register Map, part of TMRX_SR0 register */
    #define QSK_Pop_Filter_STATUS_TC_INT_MASK_SHIFT        (QSK_Pop_Filter_STATUS_TC_SHIFT - 0x04u)
    /* As defined in Register Map, part of TMRX_SR0 register, Shared with Compare Status */
    #define QSK_Pop_Filter_STATUS_CAPTURE_INT_MASK_SHIFT   (QSK_Pop_Filter_STATUS_CAPTURE_SHIFT - 0x04u)

    /* Status Register Bit Masks */
    #define QSK_Pop_Filter_STATUS_TC                       ((uint8)((uint8)0x01u << QSK_Pop_Filter_STATUS_TC_SHIFT))
    #define QSK_Pop_Filter_STATUS_CAPTURE                  ((uint8)((uint8)0x01u << QSK_Pop_Filter_STATUS_CAPTURE_SHIFT))
    /* Interrupt Enable Bit-Mask for interrupt on TC */
    #define QSK_Pop_Filter_STATUS_TC_INT_MASK              ((uint8)((uint8)0x01u << QSK_Pop_Filter_STATUS_TC_INT_MASK_SHIFT))
    /* Interrupt Enable Bit-Mask for interrupt on Capture */
    #define QSK_Pop_Filter_STATUS_CAPTURE_INT_MASK         ((uint8)((uint8)0x01u << QSK_Pop_Filter_STATUS_CAPTURE_INT_MASK_SHIFT))

#else   /* UDB Registers and Register Constants */


    /***************************************
    *           UDB Registers
    ***************************************/

    #define QSK_Pop_Filter_STATUS              (* (reg8 *) QSK_Pop_Filter_TimerUDB_rstSts_stsreg__STATUS_REG )
    #define QSK_Pop_Filter_STATUS_MASK         (* (reg8 *) QSK_Pop_Filter_TimerUDB_rstSts_stsreg__MASK_REG)
    #define QSK_Pop_Filter_STATUS_AUX_CTRL     (* (reg8 *) QSK_Pop_Filter_TimerUDB_rstSts_stsreg__STATUS_AUX_CTL_REG)
    #define QSK_Pop_Filter_CONTROL             (* (reg8 *) QSK_Pop_Filter_TimerUDB_sCTRLReg_SyncCtl_ctrlreg__CONTROL_REG )
    
    #if(QSK_Pop_Filter_Resolution <= 8u) /* 8-bit Timer */
        #define QSK_Pop_Filter_CAPTURE_LSB         (* (reg8 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__F0_REG )
        #define QSK_Pop_Filter_CAPTURE_LSB_PTR       ((reg8 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__F0_REG )
        #define QSK_Pop_Filter_PERIOD_LSB          (* (reg8 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__D0_REG )
        #define QSK_Pop_Filter_PERIOD_LSB_PTR        ((reg8 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__D0_REG )
        #define QSK_Pop_Filter_COUNTER_LSB         (* (reg8 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__A0_REG )
        #define QSK_Pop_Filter_COUNTER_LSB_PTR       ((reg8 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__A0_REG )
    #elif(QSK_Pop_Filter_Resolution <= 16u) /* 8-bit Timer */
        #if(CY_PSOC3) /* 8-bit addres space */
            #define QSK_Pop_Filter_CAPTURE_LSB         (* (reg16 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__F0_REG )
            #define QSK_Pop_Filter_CAPTURE_LSB_PTR       ((reg16 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__F0_REG )
            #define QSK_Pop_Filter_PERIOD_LSB          (* (reg16 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__D0_REG )
            #define QSK_Pop_Filter_PERIOD_LSB_PTR        ((reg16 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__D0_REG )
            #define QSK_Pop_Filter_COUNTER_LSB         (* (reg16 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__A0_REG )
            #define QSK_Pop_Filter_COUNTER_LSB_PTR       ((reg16 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__A0_REG )
        #else /* 16-bit address space */
            #define QSK_Pop_Filter_CAPTURE_LSB         (* (reg16 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__16BIT_F0_REG )
            #define QSK_Pop_Filter_CAPTURE_LSB_PTR       ((reg16 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__16BIT_F0_REG )
            #define QSK_Pop_Filter_PERIOD_LSB          (* (reg16 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__16BIT_D0_REG )
            #define QSK_Pop_Filter_PERIOD_LSB_PTR        ((reg16 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__16BIT_D0_REG )
            #define QSK_Pop_Filter_COUNTER_LSB         (* (reg16 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__16BIT_A0_REG )
            #define QSK_Pop_Filter_COUNTER_LSB_PTR       ((reg16 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__16BIT_A0_REG )
        #endif /* CY_PSOC3 */
    #elif(QSK_Pop_Filter_Resolution <= 24u)/* 24-bit Timer */
        #define QSK_Pop_Filter_CAPTURE_LSB         (* (reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__F0_REG )
        #define QSK_Pop_Filter_CAPTURE_LSB_PTR       ((reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__F0_REG )
        #define QSK_Pop_Filter_PERIOD_LSB          (* (reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__D0_REG )
        #define QSK_Pop_Filter_PERIOD_LSB_PTR        ((reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__D0_REG )
        #define QSK_Pop_Filter_COUNTER_LSB         (* (reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__A0_REG )
        #define QSK_Pop_Filter_COUNTER_LSB_PTR       ((reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__A0_REG )
    #else /* 32-bit Timer */
        #if(CY_PSOC3 || CY_PSOC5) /* 8-bit address space */
            #define QSK_Pop_Filter_CAPTURE_LSB         (* (reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__F0_REG )
            #define QSK_Pop_Filter_CAPTURE_LSB_PTR       ((reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__F0_REG )
            #define QSK_Pop_Filter_PERIOD_LSB          (* (reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__D0_REG )
            #define QSK_Pop_Filter_PERIOD_LSB_PTR        ((reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__D0_REG )
            #define QSK_Pop_Filter_COUNTER_LSB         (* (reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__A0_REG )
            #define QSK_Pop_Filter_COUNTER_LSB_PTR       ((reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__A0_REG )
        #else /* 32-bit address space */
            #define QSK_Pop_Filter_CAPTURE_LSB         (* (reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__32BIT_F0_REG )
            #define QSK_Pop_Filter_CAPTURE_LSB_PTR       ((reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__32BIT_F0_REG )
            #define QSK_Pop_Filter_PERIOD_LSB          (* (reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__32BIT_D0_REG )
            #define QSK_Pop_Filter_PERIOD_LSB_PTR        ((reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__32BIT_D0_REG )
            #define QSK_Pop_Filter_COUNTER_LSB         (* (reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__32BIT_A0_REG )
            #define QSK_Pop_Filter_COUNTER_LSB_PTR       ((reg32 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__32BIT_A0_REG )
        #endif /* CY_PSOC3 || CY_PSOC5 */ 
    #endif

    #define QSK_Pop_Filter_COUNTER_LSB_PTR_8BIT       ((reg8 *) QSK_Pop_Filter_TimerUDB_sT16_timerdp_u0__A0_REG )
    
    #if (QSK_Pop_Filter_UsingHWCaptureCounter)
        #define QSK_Pop_Filter_CAP_COUNT              (*(reg8 *) QSK_Pop_Filter_TimerUDB_sCapCount_counter__PERIOD_REG )
        #define QSK_Pop_Filter_CAP_COUNT_PTR          ( (reg8 *) QSK_Pop_Filter_TimerUDB_sCapCount_counter__PERIOD_REG )
        #define QSK_Pop_Filter_CAPTURE_COUNT_CTRL     (*(reg8 *) QSK_Pop_Filter_TimerUDB_sCapCount_counter__CONTROL_AUX_CTL_REG )
        #define QSK_Pop_Filter_CAPTURE_COUNT_CTRL_PTR ( (reg8 *) QSK_Pop_Filter_TimerUDB_sCapCount_counter__CONTROL_AUX_CTL_REG )
    #endif /* (QSK_Pop_Filter_UsingHWCaptureCounter) */


    /***************************************
    *       Register Constants
    ***************************************/

    /* Control Register Bit Locations */
    #define QSK_Pop_Filter_CTRL_INTCNT_SHIFT              0x00u       /* As defined by Verilog Implementation */
    #define QSK_Pop_Filter_CTRL_TRIG_MODE_SHIFT           0x02u       /* As defined by Verilog Implementation */
    #define QSK_Pop_Filter_CTRL_TRIG_EN_SHIFT             0x04u       /* As defined by Verilog Implementation */
    #define QSK_Pop_Filter_CTRL_CAP_MODE_SHIFT            0x05u       /* As defined by Verilog Implementation */
    #define QSK_Pop_Filter_CTRL_ENABLE_SHIFT              0x07u       /* As defined by Verilog Implementation */

    /* Control Register Bit Masks */
    #define QSK_Pop_Filter_CTRL_INTCNT_MASK               ((uint8)((uint8)0x03u << QSK_Pop_Filter_CTRL_INTCNT_SHIFT))
    #define QSK_Pop_Filter_CTRL_TRIG_MODE_MASK            ((uint8)((uint8)0x03u << QSK_Pop_Filter_CTRL_TRIG_MODE_SHIFT))
    #define QSK_Pop_Filter_CTRL_TRIG_EN                   ((uint8)((uint8)0x01u << QSK_Pop_Filter_CTRL_TRIG_EN_SHIFT))
    #define QSK_Pop_Filter_CTRL_CAP_MODE_MASK             ((uint8)((uint8)0x03u << QSK_Pop_Filter_CTRL_CAP_MODE_SHIFT))
    #define QSK_Pop_Filter_CTRL_ENABLE                    ((uint8)((uint8)0x01u << QSK_Pop_Filter_CTRL_ENABLE_SHIFT))

    /* Bit Counter (7-bit) Control Register Bit Definitions */
    /* As defined by the Register map for the AUX Control Register */
    #define QSK_Pop_Filter_CNTR_ENABLE                    0x20u

    /* Status Register Bit Locations */
    #define QSK_Pop_Filter_STATUS_TC_SHIFT                0x00u  /* As defined by Verilog Implementation */
    #define QSK_Pop_Filter_STATUS_CAPTURE_SHIFT           0x01u  /* As defined by Verilog Implementation */
    #define QSK_Pop_Filter_STATUS_TC_INT_MASK_SHIFT       QSK_Pop_Filter_STATUS_TC_SHIFT
    #define QSK_Pop_Filter_STATUS_CAPTURE_INT_MASK_SHIFT  QSK_Pop_Filter_STATUS_CAPTURE_SHIFT
    #define QSK_Pop_Filter_STATUS_FIFOFULL_SHIFT          0x02u  /* As defined by Verilog Implementation */
    #define QSK_Pop_Filter_STATUS_FIFONEMP_SHIFT          0x03u  /* As defined by Verilog Implementation */
    #define QSK_Pop_Filter_STATUS_FIFOFULL_INT_MASK_SHIFT QSK_Pop_Filter_STATUS_FIFOFULL_SHIFT

    /* Status Register Bit Masks */
    /* Sticky TC Event Bit-Mask */
    #define QSK_Pop_Filter_STATUS_TC                      ((uint8)((uint8)0x01u << QSK_Pop_Filter_STATUS_TC_SHIFT))
    /* Sticky Capture Event Bit-Mask */
    #define QSK_Pop_Filter_STATUS_CAPTURE                 ((uint8)((uint8)0x01u << QSK_Pop_Filter_STATUS_CAPTURE_SHIFT))
    /* Interrupt Enable Bit-Mask */
    #define QSK_Pop_Filter_STATUS_TC_INT_MASK             ((uint8)((uint8)0x01u << QSK_Pop_Filter_STATUS_TC_SHIFT))
    /* Interrupt Enable Bit-Mask */
    #define QSK_Pop_Filter_STATUS_CAPTURE_INT_MASK        ((uint8)((uint8)0x01u << QSK_Pop_Filter_STATUS_CAPTURE_SHIFT))
    /* NOT-Sticky FIFO Full Bit-Mask */
    #define QSK_Pop_Filter_STATUS_FIFOFULL                ((uint8)((uint8)0x01u << QSK_Pop_Filter_STATUS_FIFOFULL_SHIFT))
    /* NOT-Sticky FIFO Not Empty Bit-Mask */
    #define QSK_Pop_Filter_STATUS_FIFONEMP                ((uint8)((uint8)0x01u << QSK_Pop_Filter_STATUS_FIFONEMP_SHIFT))
    /* Interrupt Enable Bit-Mask */
    #define QSK_Pop_Filter_STATUS_FIFOFULL_INT_MASK       ((uint8)((uint8)0x01u << QSK_Pop_Filter_STATUS_FIFOFULL_SHIFT))

    #define QSK_Pop_Filter_STATUS_ACTL_INT_EN             0x10u   /* As defined for the ACTL Register */

    /* Datapath Auxillary Control Register definitions */
    #define QSK_Pop_Filter_AUX_CTRL_FIFO0_CLR             0x01u   /* As defined by Register map */
    #define QSK_Pop_Filter_AUX_CTRL_FIFO1_CLR             0x02u   /* As defined by Register map */
    #define QSK_Pop_Filter_AUX_CTRL_FIFO0_LVL             0x04u   /* As defined by Register map */
    #define QSK_Pop_Filter_AUX_CTRL_FIFO1_LVL             0x08u   /* As defined by Register map */
    #define QSK_Pop_Filter_STATUS_ACTL_INT_EN_MASK        0x10u   /* As defined for the ACTL Register */

#endif /* Implementation Specific Registers and Register Constants */

#endif  /* CY_TIMER_QSK_Pop_Filter_H */


/* [] END OF FILE */
