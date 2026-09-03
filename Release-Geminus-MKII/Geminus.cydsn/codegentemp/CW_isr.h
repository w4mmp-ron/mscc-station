/*******************************************************************************
* File Name: CW_isr.h
* Version 1.71
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

#if !defined(CY_ISR_CW_isr_H)
#define CY_ISR_CW_isr_H

#include <cytypes.h>
#include <cyfitter.h>

/* Interrupt Controller API. */
void CW_isr_Start(void) ;
void CW_isr_StartEx(cyisraddress address) ;
void CW_isr_Stop(void) ;

CY_ISR_PROTO(CW_isr_Interrupt);

void CW_isr_SetVector(cyisraddress address) ;
cyisraddress CW_isr_GetVector(void) ;

void CW_isr_SetPriority(uint8 priority) ;
uint8 CW_isr_GetPriority(void) ;

void CW_isr_Enable(void) ;
uint8 CW_isr_GetState(void) ;
void CW_isr_Disable(void) ;

void CW_isr_SetPending(void) ;
void CW_isr_ClearPending(void) ;


/* Interrupt Controller Constants */

/* Address of the INTC.VECT[x] register that contains the Address of the CW_isr ISR. */
#define CW_isr_INTC_VECTOR            ((reg16 *) CW_isr__INTC_VECT)

/* Address of the CW_isr ISR priority. */
#define CW_isr_INTC_PRIOR             ((reg8 *) CW_isr__INTC_PRIOR_REG)

/* Priority of the CW_isr interrupt. */
#define CW_isr_INTC_PRIOR_NUMBER      CW_isr__INTC_PRIOR_NUM

/* Address of the INTC.SET_EN[x] byte to bit enable CW_isr interrupt. */
#define CW_isr_INTC_SET_EN            ((reg8 *) CW_isr__INTC_SET_EN_REG)

/* Address of the INTC.CLR_EN[x] register to bit clear the CW_isr interrupt. */
#define CW_isr_INTC_CLR_EN            ((reg8 *) CW_isr__INTC_CLR_EN_REG)

/* Address of the INTC.SET_PD[x] register to set the CW_isr interrupt state to pending. */
#define CW_isr_INTC_SET_PD            ((reg8 *) CW_isr__INTC_SET_PD_REG)

/* Address of the INTC.CLR_PD[x] register to clear the CW_isr interrupt. */
#define CW_isr_INTC_CLR_PD            ((reg8 *) CW_isr__INTC_CLR_PD_REG)



#endif /* CY_ISR_CW_isr_H */


/* [] END OF FILE */
