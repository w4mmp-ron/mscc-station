/*******************************************************************************
* File Name: USBFS_VBUS.h  
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

#if !defined(CY_PINS_USBFS_VBUS_H) /* Pins USBFS_VBUS_H */
#define CY_PINS_USBFS_VBUS_H

#include "cytypes.h"
#include "cyfitter.h"
#include "cypins.h"
#include "USBFS_VBUS_aliases.h"


/***************************************
*        Function Prototypes             
***************************************/    

/**
* \addtogroup group_general
* @{
*/
void    USBFS_VBUS_Write(uint8 value) ;
void    USBFS_VBUS_SetDriveMode(uint8 mode) ;
uint8   USBFS_VBUS_ReadDataReg(void) ;
uint8   USBFS_VBUS_Read(void) ;
void    USBFS_VBUS_SetInterruptMode(uint16 position, uint16 mode) ;
uint8   USBFS_VBUS_ClearInterrupt(void) ;
/** @} general */

/***************************************
*           API Constants        
***************************************/

/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup driveMode Drive mode constants
     * \brief Constants to be passed as "mode" parameter in the USBFS_VBUS_SetDriveMode() function.
     *  @{
     */
        /* Drive Modes */
        #define USBFS_VBUS_DM_ALG_HIZ         PIN_DM_ALG_HIZ   /**< \brief High Impedance Analog   */
        #define USBFS_VBUS_DM_DIG_HIZ         PIN_DM_DIG_HIZ   /**< \brief High Impedance Digital  */
        #define USBFS_VBUS_DM_RES_UP          PIN_DM_RES_UP    /**< \brief Resistive Pull Up       */
        #define USBFS_VBUS_DM_RES_DWN         PIN_DM_RES_DWN   /**< \brief Resistive Pull Down     */
        #define USBFS_VBUS_DM_OD_LO           PIN_DM_OD_LO     /**< \brief Open Drain, Drives Low  */
        #define USBFS_VBUS_DM_OD_HI           PIN_DM_OD_HI     /**< \brief Open Drain, Drives High */
        #define USBFS_VBUS_DM_STRONG          PIN_DM_STRONG    /**< \brief Strong Drive            */
        #define USBFS_VBUS_DM_RES_UPDWN       PIN_DM_RES_UPDWN /**< \brief Resistive Pull Up/Down  */
    /** @} driveMode */
/** @} group_constants */
    
/* Digital Port Constants */
#define USBFS_VBUS_MASK               USBFS_VBUS__MASK
#define USBFS_VBUS_SHIFT              USBFS_VBUS__SHIFT
#define USBFS_VBUS_WIDTH              1u

/* Interrupt constants */
#if defined(USBFS_VBUS__INTSTAT)
/**
* \addtogroup group_constants
* @{
*/
    /** \addtogroup intrMode Interrupt constants
     * \brief Constants to be passed as "mode" parameter in USBFS_VBUS_SetInterruptMode() function.
     *  @{
     */
        #define USBFS_VBUS_INTR_NONE      (uint16)(0x0000u)   /**< \brief Disabled             */
        #define USBFS_VBUS_INTR_RISING    (uint16)(0x0001u)   /**< \brief Rising edge trigger  */
        #define USBFS_VBUS_INTR_FALLING   (uint16)(0x0002u)   /**< \brief Falling edge trigger */
        #define USBFS_VBUS_INTR_BOTH      (uint16)(0x0003u)   /**< \brief Both edge trigger    */
        /** @} intrMode */
/** @} group_constants */

    #define USBFS_VBUS_INTR_MASK      (0x01u)
#endif /* (USBFS_VBUS__INTSTAT) */


/***************************************
*             Registers        
***************************************/

/* Main Port Registers */
/* Pin State */
#define USBFS_VBUS_PS                     (* (reg8 *) USBFS_VBUS__PS)
/* Data Register */
#define USBFS_VBUS_DR                     (* (reg8 *) USBFS_VBUS__DR)
/* Port Number */
#define USBFS_VBUS_PRT_NUM                (* (reg8 *) USBFS_VBUS__PRT) 
/* Connect to Analog Globals */                                                  
#define USBFS_VBUS_AG                     (* (reg8 *) USBFS_VBUS__AG)                       
/* Analog MUX bux enable */
#define USBFS_VBUS_AMUX                   (* (reg8 *) USBFS_VBUS__AMUX) 
/* Bidirectional Enable */                                                        
#define USBFS_VBUS_BIE                    (* (reg8 *) USBFS_VBUS__BIE)
/* Bit-mask for Aliased Register Access */
#define USBFS_VBUS_BIT_MASK               (* (reg8 *) USBFS_VBUS__BIT_MASK)
/* Bypass Enable */
#define USBFS_VBUS_BYP                    (* (reg8 *) USBFS_VBUS__BYP)
/* Port wide control signals */                                                   
#define USBFS_VBUS_CTL                    (* (reg8 *) USBFS_VBUS__CTL)
/* Drive Modes */
#define USBFS_VBUS_DM0                    (* (reg8 *) USBFS_VBUS__DM0) 
#define USBFS_VBUS_DM1                    (* (reg8 *) USBFS_VBUS__DM1)
#define USBFS_VBUS_DM2                    (* (reg8 *) USBFS_VBUS__DM2) 
/* Input Buffer Disable Override */
#define USBFS_VBUS_INP_DIS                (* (reg8 *) USBFS_VBUS__INP_DIS)
/* LCD Common or Segment Drive */
#define USBFS_VBUS_LCD_COM_SEG            (* (reg8 *) USBFS_VBUS__LCD_COM_SEG)
/* Enable Segment LCD */
#define USBFS_VBUS_LCD_EN                 (* (reg8 *) USBFS_VBUS__LCD_EN)
/* Slew Rate Control */
#define USBFS_VBUS_SLW                    (* (reg8 *) USBFS_VBUS__SLW)

/* DSI Port Registers */
/* Global DSI Select Register */
#define USBFS_VBUS_PRTDSI__CAPS_SEL       (* (reg8 *) USBFS_VBUS__PRTDSI__CAPS_SEL) 
/* Double Sync Enable */
#define USBFS_VBUS_PRTDSI__DBL_SYNC_IN    (* (reg8 *) USBFS_VBUS__PRTDSI__DBL_SYNC_IN) 
/* Output Enable Select Drive Strength */
#define USBFS_VBUS_PRTDSI__OE_SEL0        (* (reg8 *) USBFS_VBUS__PRTDSI__OE_SEL0) 
#define USBFS_VBUS_PRTDSI__OE_SEL1        (* (reg8 *) USBFS_VBUS__PRTDSI__OE_SEL1) 
/* Port Pin Output Select Registers */
#define USBFS_VBUS_PRTDSI__OUT_SEL0       (* (reg8 *) USBFS_VBUS__PRTDSI__OUT_SEL0) 
#define USBFS_VBUS_PRTDSI__OUT_SEL1       (* (reg8 *) USBFS_VBUS__PRTDSI__OUT_SEL1) 
/* Sync Output Enable Registers */
#define USBFS_VBUS_PRTDSI__SYNC_OUT       (* (reg8 *) USBFS_VBUS__PRTDSI__SYNC_OUT) 

/* SIO registers */
#if defined(USBFS_VBUS__SIO_CFG)
    #define USBFS_VBUS_SIO_HYST_EN        (* (reg8 *) USBFS_VBUS__SIO_HYST_EN)
    #define USBFS_VBUS_SIO_REG_HIFREQ     (* (reg8 *) USBFS_VBUS__SIO_REG_HIFREQ)
    #define USBFS_VBUS_SIO_CFG            (* (reg8 *) USBFS_VBUS__SIO_CFG)
    #define USBFS_VBUS_SIO_DIFF           (* (reg8 *) USBFS_VBUS__SIO_DIFF)
#endif /* (USBFS_VBUS__SIO_CFG) */

/* Interrupt Registers */
#if defined(USBFS_VBUS__INTSTAT)
    #define USBFS_VBUS_INTSTAT             (* (reg8 *) USBFS_VBUS__INTSTAT)
    #define USBFS_VBUS_SNAP                (* (reg8 *) USBFS_VBUS__SNAP)
    
	#define USBFS_VBUS_0_INTTYPE_REG 		(* (reg8 *) USBFS_VBUS__0__INTTYPE)
#endif /* (USBFS_VBUS__INTSTAT) */

#endif /* End Pins USBFS_VBUS_H */


/* [] END OF FILE */
