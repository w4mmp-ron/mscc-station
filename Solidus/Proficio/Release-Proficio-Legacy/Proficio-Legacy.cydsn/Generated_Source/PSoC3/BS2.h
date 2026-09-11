/*******************************************************************************
* File Name: BS2.h  
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

#if !defined(CY_PINS_BS2_H) /* Pins BS2_H */
#define CY_PINS_BS2_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "BS2_aliases.h"


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    BS2_Write(uint8 value) ;
void    BS2_SetDriveMode(uint8 mode) ;
uint8   BS2_ReadDataReg(void) ;
uint8   BS2_Read(void) ;
void    BS2_SetInterruptMode(uint16 position, uint16 mode) ;
uint8   BS2_ClearInterrupt(void) ;
/** @} general */

/***************************************
*           API Constants        
***************************************/

/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the BS2_SetDriveMode() function.
     *  @{
     */
        /* Drive Modes */
        #define BS2_DM_ALG_HIZ         PIN_DM_ALG_HIZ   /**< \brief High Impedance Analog   */
        #define BS2_DM_DIG_HIZ         PIN_DM_DIG_HIZ   /**< \brief High Impedance Digital  */
        #define BS2_DM_RES_UP          PIN_DM_RES_UP    /**< \brief Resistive Pull Up       */
        #define BS2_DM_RES_DWN         PIN_DM_RES_DWN   /**< \brief Resistive Pull Down     */
        #define BS2_DM_OD_LO           PIN_DM_OD_LO     /**< \brief Open Drain, Drives Low  */
        #define BS2_DM_OD_HI           PIN_DM_OD_HI     /**< \brief Open Drain, Drives High */
        #define BS2_DM_STRONG          PIN_DM_STRONG    /**< \brief Strong Drive            */
        #define BS2_DM_RES_UPDWN       PIN_DM_RES_UPDWN /**< \brief Resistive Pull Up/Down  */
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define BS2_MASK               BS2__MASK
#define BS2_SHIFT              BS2__SHIFT
#define BS2_WIDTH              1u

/* Interrupt constants */
#if defined(BS2__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in BS2_SetInterruptMode() function.
     *  @{
     */
        #define BS2_INTR_NONE      (uint16)(0x0000u)   /**< \brief Disabled             */
        #define BS2_INTR_RISING    (uint16)(0x0001u)   /**< \brief Rising edge trigger  */
        #define BS2_INTR_FALLING   (uint16)(0x0002u)   /**< \brief Falling edge trigger */
        #define BS2_INTR_BOTH      (uint16)(0x0003u)   /**< \brief Both edge trigger    */
        /** @} intrMode */
/** @} group_constants */

    #define BS2_INTR_MASK      (0x01u)
#endif /* (BS2__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define BS2_PS                     (* (reg8 *) BS2__PS)
/* Data Register */
#define BS2_DR                     (* (reg8 *) BS2__DR)
/* Port Number */
#define BS2_PRT_NUM                (* (reg8 *) BS2__PRT) 
/* Connect to Analog Globals */                                                  
#define BS2_AG                     (* (reg8 *) BS2__AG)                       
/* Analog MUX bux enable */
#define BS2_AMUX                   (* (reg8 *) BS2__AMUX) 
/* Bidirectional Enable */                                                        
#define BS2_BIE                    (* (reg8 *) BS2__BIE)
/* Bit-mask for Aliased Register Access */
#define BS2_BIT_MASK               (* (reg8 *) BS2__BIT_MASK)
/* Bypass Enable */
#define BS2_BYP                    (* (reg8 *) BS2__BYP)
/* Port wide control signals */                                                   
#define BS2_CTL                    (* (reg8 *) BS2__CTL)
/* Drive Modes */
#define BS2_DM0                    (* (reg8 *) BS2__DM0) 
#define BS2_DM1                    (* (reg8 *) BS2__DM1)
#define BS2_DM2                    (* (reg8 *) BS2__DM2) 
/* Input Buffer Disable Override */
#define BS2_INP_DIS                (* (reg8 *) BS2__INP_DIS)
/* LCD Common or Segment Drive */
#define BS2_LCD_COM_SEG            (* (reg8 *) BS2__LCD_COM_SEG)
/* Enable Segment LCD */
#define BS2_LCD_EN                 (* (reg8 *) BS2__LCD_EN)
/* Slew Rate Control */
#define BS2_SLW                    (* (reg8 *) BS2__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define BS2_PRTDSI__CAPS_SEL       (* (reg8 *) BS2__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define BS2_PRTDSI__DBL_SYNC_IN    (* (reg8 *) BS2__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define BS2_PRTDSI__OE_SEL0        (* (reg8 *) BS2__PRTDSI__OE_SEL0) 
#define BS2_PRTDSI__OE_SEL1        (* (reg8 *) BS2__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define BS2_PRTDSI__OUT_SEL0       (* (reg8 *) BS2__PRTDSI__OUT_SEL0) 
#define BS2_PRTDSI__OUT_SEL1       (* (reg8 *) BS2__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define BS2_PRTDSI__SYNC_OUT       (* (reg8 *) BS2__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(BS2__SIO_CFG)
    #define BS2_SIO_HYST_EN        (* (reg8 *) BS2__SIO_HYST_EN)
    #define BS2_SIO_REG_HIFREQ     (* (reg8 *) BS2__SIO_REG_HIFREQ)
    #define BS2_SIO_CFG            (* (reg8 *) BS2__SIO_CFG)
    #define BS2_SIO_DIFF           (* (reg8 *) BS2__SIO_DIFF)
#endif /* (BS2__SIO_CFG) */

/* Interrupt Registers */
#if defined(BS2__INTSTAT)
    #define BS2_INTSTAT             (* (reg8 *) BS2__INTSTAT)
    #define BS2_SNAP                (* (reg8 *) BS2__SNAP)
    
	#define BS2_0_INTTYPE_REG 		(* (reg8 *) BS2__0__INTTYPE)
#endif /* (BS2__INTSTAT) */

#endif /* End Pins BS2_H */


/* [] END OF FILE */
