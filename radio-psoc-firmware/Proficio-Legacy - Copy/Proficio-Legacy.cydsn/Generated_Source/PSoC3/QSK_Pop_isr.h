/*******************************************************************************
* File Name: QSK_Pop_isr.h
* Version 1.70
*
*  Description:
*   Provides the function definitions for the Interrupt Controller.
*
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_ISR_QSK_Pop_isr_H)
#define CY_ISR_QSK_Pop_isr_H

#include <cytypes.h>
#include <cyfitter.h>

/* Interrupt Controller API. */
void QSK_Pop_isr_Start(void) ;
void QSK_Pop_isr_StartEx(cyisraddress address) ;
void QSK_Pop_isr_Stop(void) ;

CY_ISR_PROTO(QSK_Pop_isr_Interrupt);

void QSK_Pop_isr_SetVector(cyisraddress address) ;
cyisraddress QSK_Pop_isr_GetVector(void) ;

void QSK_Pop_isr_SetPriority(uint8 priority) ;
uint8 QSK_Pop_isr_GetPriority(void) ;

void QSK_Pop_isr_Enable(void) ;
uint8 QSK_Pop_isr_GetState(void) ;
void QSK_Pop_isr_Disable(void) ;

void QSK_Pop_isr_SetPending(void) ;
void QSK_Pop_isr_ClearPending(void) ;


/* Interrupt Controller Constants */

/* Address of the INTC.VECT[x] register that contains the Address of the QSK_Pop_isr ISR. */
#define QSK_Pop_isr_INTC_VECTOR            ((reg16 *) QSK_Pop_isr__INTC_VECT)

/* Address of the QSK_Pop_isr ISR priority. */
#define QSK_Pop_isr_INTC_PRIOR             ((reg8 *) QSK_Pop_isr__INTC_PRIOR_REG)

/* Priority of the QSK_Pop_isr interrupt. */
#define QSK_Pop_isr_INTC_PRIOR_NUMBER      QSK_Pop_isr__INTC_PRIOR_NUM

/* Address of the INTC.SET_EN[x] byte to bit enable QSK_Pop_isr interrupt. */
#define QSK_Pop_isr_INTC_SET_EN            ((reg8 *) QSK_Pop_isr__INTC_SET_EN_REG)

/* Address of the INTC.CLR_EN[x] register to bit clear the QSK_Pop_isr interrupt. */
#define QSK_Pop_isr_INTC_CLR_EN            ((reg8 *) QSK_Pop_isr__INTC_CLR_EN_REG)

/* Address of the INTC.SET_PD[x] register to set the QSK_Pop_isr interrupt state to pending. */
#define QSK_Pop_isr_INTC_SET_PD            ((reg8 *) QSK_Pop_isr__INTC_SET_PD_REG)

/* Address of the INTC.CLR_PD[x] register to clear the QSK_Pop_isr interrupt. */
#define QSK_Pop_isr_INTC_CLR_PD            ((reg8 *) QSK_Pop_isr__INTC_CLR_PD_REG)



#endif /* CY_ISR_QSK_Pop_isr_H */


/* [] END OF FILE */
