/*******************************************************************************
* File Name: Key_Delay_isr.h
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

#if !defined(CY_ISR_Key_Delay_isr_H)
#define CY_ISR_Key_Delay_isr_H

#include <cytypes.h>
#include <cyfitter.h>

/* Interrupt Controller API. */
void Key_Delay_isr_Start(void) ;
void Key_Delay_isr_StartEx(cyisraddress address) ;
void Key_Delay_isr_Stop(void) ;

CY_ISR_PROTO(Key_Delay_isr_Interrupt);

void Key_Delay_isr_SetVector(cyisraddress address) ;
cyisraddress Key_Delay_isr_GetVector(void) ;

void Key_Delay_isr_SetPriority(uint8 priority) ;
uint8 Key_Delay_isr_GetPriority(void) ;

void Key_Delay_isr_Enable(void) ;
uint8 Key_Delay_isr_GetState(void) ;
void Key_Delay_isr_Disable(void) ;

void Key_Delay_isr_SetPending(void) ;
void Key_Delay_isr_ClearPending(void) ;


/* Interrupt Controller Constants */

/* Address of the INTC.VECT[x] register that contains the Address of the Key_Delay_isr ISR. */
#define Key_Delay_isr_INTC_VECTOR            ((reg16 *) Key_Delay_isr__INTC_VECT)

/* Address of the Key_Delay_isr ISR priority. */
#define Key_Delay_isr_INTC_PRIOR             ((reg8 *) Key_Delay_isr__INTC_PRIOR_REG)

/* Priority of the Key_Delay_isr interrupt. */
#define Key_Delay_isr_INTC_PRIOR_NUMBER      Key_Delay_isr__INTC_PRIOR_NUM

/* Address of the INTC.SET_EN[x] byte to bit enable Key_Delay_isr interrupt. */
#define Key_Delay_isr_INTC_SET_EN            ((reg8 *) Key_Delay_isr__INTC_SET_EN_REG)

/* Address of the INTC.CLR_EN[x] register to bit clear the Key_Delay_isr interrupt. */
#define Key_Delay_isr_INTC_CLR_EN            ((reg8 *) Key_Delay_isr__INTC_CLR_EN_REG)

/* Address of the INTC.SET_PD[x] register to set the Key_Delay_isr interrupt state to pending. */
#define Key_Delay_isr_INTC_SET_PD            ((reg8 *) Key_Delay_isr__INTC_SET_PD_REG)

/* Address of the INTC.CLR_PD[x] register to clear the Key_Delay_isr interrupt. */
#define Key_Delay_isr_INTC_CLR_PD            ((reg8 *) Key_Delay_isr__INTC_CLR_PD_REG)



#endif /* CY_ISR_Key_Delay_isr_H */


/* [] END OF FILE */
