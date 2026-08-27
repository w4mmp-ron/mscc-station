/*******************************************************************************
* File Name: BS0.h  
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

#if !defined(CY_PINS_BS0_H) /* Pins BS0_H */
#define CY_PINS_BS0_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "BS0_aliases.h"


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    BS0_Write(uint8 value) ;
void    BS0_SetDriveMode(uint8 mode) ;
uint8   BS0_ReadDataReg(void) ;
uint8   BS0_Read(void) ;
void    BS0_SetInterruptMode(uint16 position, uint16 mode) ;
uint8   BS0_ClearInterrupt(void) ;
/** @} general */

/***************************************
*           API Constants        
***************************************/

/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the BS0_SetDriveMode() function.
     *  @{
     */
        /* Drive Modes */
        #define BS0_DM_ALG_HIZ         PIN_DM_ALG_HIZ   /**< \brief High Impedance Analog   */
        #define BS0_DM_DIG_HIZ         PIN_DM_DIG_HIZ   /**< \brief High Impedance Digital  */
        #define BS0_DM_RES_UP          PIN_DM_RES_UP    /**< \brief Resistive Pull Up       */
        #define BS0_DM_RES_DWN         PIN_DM_RES_DWN   /**< \brief Resistive Pull Down     */
        #define BS0_DM_OD_LO           PIN_DM_OD_LO     /**< \brief Open Drain, Drives Low  */
        #define BS0_DM_OD_HI           PIN_DM_OD_HI     /**< \brief Open Drain, Drives High */
        #define BS0_DM_STRONG          PIN_DM_STRONG    /**< \brief Strong Drive            */
        #define BS0_DM_RES_UPDWN       PIN_DM_RES_UPDWN /**< \brief Resistive Pull Up/Down  */
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define BS0_MASK               BS0__MASK
#define BS0_SHIFT              BS0__SHIFT
#define BS0_WIDTH              1u

/* Interrupt constants */
#if defined(BS0__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in BS0_SetInterruptMode() function.
     *  @{
     */
        #define BS0_INTR_NONE      (uint16)(0x0000u)   /**< \brief Disabled             */
        #define BS0_INTR_RISING    (uint16)(0x0001u)   /**< \brief Rising edge trigger  */
        #define BS0_INTR_FALLING   (uint16)(0x0002u)   /**< \brief Falling edge trigger */
        #define BS0_INTR_BOTH      (uint16)(0x0003u)   /**< \brief Both edge trigger    */
        /** @} intrMode */
/** @} group_constants */

    #define BS0_INTR_MASK      (0x01u)
#endif /* (BS0__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define BS0_PS                     (* (reg8 *) BS0__PS)
/* Data Register */
#define BS0_DR                     (* (reg8 *) BS0__DR)
/* Port Number */
#define BS0_PRT_NUM                (* (reg8 *) BS0__PRT) 
/* Connect to Analog Globals */                                                  
#define BS0_AG                     (* (reg8 *) BS0__AG)                       
/* Analog MUX bux enable */
#define BS0_AMUX                   (* (reg8 *) BS0__AMUX) 
/* Bidirectional Enable */                                                        
#define BS0_BIE                    (* (reg8 *) BS0__BIE)
/* Bit-mask for Aliased Register Access */
#define BS0_BIT_MASK               (* (reg8 *) BS0__BIT_MASK)
/* Bypass Enable */
#define BS0_BYP                    (* (reg8 *) BS0__BYP)
/* Port wide control signals */                                                   
#define BS0_CTL                    (* (reg8 *) BS0__CTL)
/* Drive Modes */
#define BS0_DM0                    (* (reg8 *) BS0__DM0) 
#define BS0_DM1                    (* (reg8 *) BS0__DM1)
#define BS0_DM2                    (* (reg8 *) BS0__DM2) 
/* Input Buffer Disable Override */
#define BS0_INP_DIS                (* (reg8 *) BS0__INP_DIS)
/* LCD Common or Segment Drive */
#define BS0_LCD_COM_SEG            (* (reg8 *) BS0__LCD_COM_SEG)
/* Enable Segment LCD */
#define BS0_LCD_EN                 (* (reg8 *) BS0__LCD_EN)
/* Slew Rate Control */
#define BS0_SLW                    (* (reg8 *) BS0__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define BS0_PRTDSI__CAPS_SEL       (* (reg8 *) BS0__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define BS0_PRTDSI__DBL_SYNC_IN    (* (reg8 *) BS0__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define BS0_PRTDSI__OE_SEL0        (* (reg8 *) BS0__PRTDSI__OE_SEL0) 
#define BS0_PRTDSI__OE_SEL1        (* (reg8 *) BS0__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define BS0_PRTDSI__OUT_SEL0       (* (reg8 *) BS0__PRTDSI__OUT_SEL0) 
#define BS0_PRTDSI__OUT_SEL1       (* (reg8 *) BS0__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define BS0_PRTDSI__SYNC_OUT       (* (reg8 *) BS0__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(BS0__SIO_CFG)
    #define BS0_SIO_HYST_EN        (* (reg8 *) BS0__SIO_HYST_EN)
    #define BS0_SIO_REG_HIFREQ     (* (reg8 *) BS0__SIO_REG_HIFREQ)
    #define BS0_SIO_CFG            (* (reg8 *) BS0__SIO_CFG)
    #define BS0_SIO_DIFF           (* (reg8 *) BS0__SIO_DIFF)
#endif /* (BS0__SIO_CFG) */

/* Interrupt Registers */
#if defined(BS0__INTSTAT)
    #define BS0_INTSTAT             (* (reg8 *) BS0__INTSTAT)
    #define BS0_SNAP                (* (reg8 *) BS0__SNAP)
    
	#define BS0_0_INTTYPE_REG 		(* (reg8 *) BS0__0__INTTYPE)
#endif /* (BS0__INTSTAT) */

#endif /* End Pins BS0_H */


/* [] END OF FILE */
