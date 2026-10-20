/*******************************************************************************
* File Name: BS1.h  
* Version 2.20
*
* Description:
*  This file contains Pin function prototypes and register defines
*
* Note:
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_PINS_BS1_H) /* Pins BS1_H */
#define CY_PINS_BS1_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "BS1_aliases.h"


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    BS1_Write(uint8 value) ;
void    BS1_SetDriveMode(uint8 mode) ;
uint8   BS1_ReadDataReg(void) ;
uint8   BS1_Read(void) ;
void    BS1_SetInterruptMode(uint16 position, uint16 mode) ;
uint8   BS1_ClearInterrupt(void) ;
/** @} general */

/***************************************
*           API Constants        
***************************************/

/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the BS1_SetDriveMode() function.
     *  @{
     */
        /* Drive Modes */
        #define BS1_DM_ALG_HIZ         PIN_DM_ALG_HIZ   /**< \brief High Impedance Analog   */
        #define BS1_DM_DIG_HIZ         PIN_DM_DIG_HIZ   /**< \brief High Impedance Digital  */
        #define BS1_DM_RES_UP          PIN_DM_RES_UP    /**< \brief Resistive Pull Up       */
        #define BS1_DM_RES_DWN         PIN_DM_RES_DWN   /**< \brief Resistive Pull Down     */
        #define BS1_DM_OD_LO           PIN_DM_OD_LO     /**< \brief Open Drain, Drives Low  */
        #define BS1_DM_OD_HI           PIN_DM_OD_HI     /**< \brief Open Drain, Drives High */
        #define BS1_DM_STRONG          PIN_DM_STRONG    /**< \brief Strong Drive            */
        #define BS1_DM_RES_UPDWN       PIN_DM_RES_UPDWN /**< \brief Resistive Pull Up/Down  */
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define BS1_MASK               BS1__MASK
#define BS1_SHIFT              BS1__SHIFT
#define BS1_WIDTH              1u

/* Interrupt constants */
#if defined(BS1__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in BS1_SetInterruptMode() function.
     *  @{
     */
        #define BS1_INTR_NONE      (uint16)(0x0000u)   /**< \brief Disabled             */
        #define BS1_INTR_RISING    (uint16)(0x0001u)   /**< \brief Rising edge trigger  */
        #define BS1_INTR_FALLING   (uint16)(0x0002u)   /**< \brief Falling edge trigger */
        #define BS1_INTR_BOTH      (uint16)(0x0003u)   /**< \brief Both edge trigger    */
        /** @} intrMode */
/** @} group_constants */

    #define BS1_INTR_MASK      (0x01u)
#endif /* (BS1__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define BS1_PS                     (* (reg8 *) BS1__PS)
/* Data Register */
#define BS1_DR                     (* (reg8 *) BS1__DR)
/* Port Number */
#define BS1_PRT_NUM                (* (reg8 *) BS1__PRT) 
/* Connect to Analog Globals */                                                  
#define BS1_AG                     (* (reg8 *) BS1__AG)                       
/* Analog MUX bux enable */
#define BS1_AMUX                   (* (reg8 *) BS1__AMUX) 
/* Bidirectional Enable */                                                        
#define BS1_BIE                    (* (reg8 *) BS1__BIE)
/* Bit-mask for Aliased Register Access */
#define BS1_BIT_MASK               (* (reg8 *) BS1__BIT_MASK)
/* Bypass Enable */
#define BS1_BYP                    (* (reg8 *) BS1__BYP)
/* Port wide control signals */                                                   
#define BS1_CTL                    (* (reg8 *) BS1__CTL)
/* Drive Modes */
#define BS1_DM0                    (* (reg8 *) BS1__DM0) 
#define BS1_DM1                    (* (reg8 *) BS1__DM1)
#define BS1_DM2                    (* (reg8 *) BS1__DM2) 
/* Input Buffer Disable Override */
#define BS1_INP_DIS                (* (reg8 *) BS1__INP_DIS)
/* LCD Common or Segment Drive */
#define BS1_LCD_COM_SEG            (* (reg8 *) BS1__LCD_COM_SEG)
/* Enable Segment LCD */
#define BS1_LCD_EN                 (* (reg8 *) BS1__LCD_EN)
/* Slew Rate Control */
#define BS1_SLW                    (* (reg8 *) BS1__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define BS1_PRTDSI__CAPS_SEL       (* (reg8 *) BS1__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define BS1_PRTDSI__DBL_SYNC_IN    (* (reg8 *) BS1__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define BS1_PRTDSI__OE_SEL0        (* (reg8 *) BS1__PRTDSI__OE_SEL0) 
#define BS1_PRTDSI__OE_SEL1        (* (reg8 *) BS1__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define BS1_PRTDSI__OUT_SEL0       (* (reg8 *) BS1__PRTDSI__OUT_SEL0) 
#define BS1_PRTDSI__OUT_SEL1       (* (reg8 *) BS1__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define BS1_PRTDSI__SYNC_OUT       (* (reg8 *) BS1__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(BS1__SIO_CFG)
    #define BS1_SIO_HYST_EN        (* (reg8 *) BS1__SIO_HYST_EN)
    #define BS1_SIO_REG_HIFREQ     (* (reg8 *) BS1__SIO_REG_HIFREQ)
    #define BS1_SIO_CFG            (* (reg8 *) BS1__SIO_CFG)
    #define BS1_SIO_DIFF           (* (reg8 *) BS1__SIO_DIFF)
#endif /* (BS1__SIO_CFG) */

/* Interrupt Registers */
#if defined(BS1__INTSTAT)
    #define BS1_INTSTAT             (* (reg8 *) BS1__INTSTAT)
    #define BS1_SNAP                (* (reg8 *) BS1__SNAP)
    
	#define BS1_0_INTTYPE_REG 		(* (reg8 *) BS1__0__INTTYPE)
#endif /* (BS1__INTSTAT) */

#endif /* End Pins BS1_H */


/* [] END OF FILE */
