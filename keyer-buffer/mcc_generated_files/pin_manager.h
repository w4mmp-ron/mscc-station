/**
  @Generated Pin Manager Header File

  @Company:
    Microchip Technology Inc.

  @File Name:
    pin_manager.h

  @Summary:
    This is the Pin Manager file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  @Description
    This header file provides APIs for driver for .
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.8
        Device            :  PIC16F18326
        Driver Version    :  2.11
    The generated drivers are tested against the following:
        Compiler          :  XC8 2.36 and above
        MPLAB 	          :  MPLAB X 6.00	
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#ifndef PIN_MANAGER_H
#define PIN_MANAGER_H

/**
  Section: Included Files
*/

#include <xc.h>

#define INPUT   1
#define OUTPUT  0

#define HIGH    1
#define LOW     0

#define ANALOG      1
#define DIGITAL     0

#define PULL_UP_ENABLED      1
#define PULL_UP_DISABLED     0

// get/set IO_RA1 aliases
#define IO_RA1_TRIS                 TRISAbits.TRISA1
#define IO_RA1_LAT                  LATAbits.LATA1
#define IO_RA1_PORT                 PORTAbits.RA1
#define IO_RA1_WPU                  WPUAbits.WPUA1
#define IO_RA1_OD                   ODCONAbits.ODCA1
#define IO_RA1_ANS                  ANSELAbits.ANSA1
#define IO_RA1_SetHigh()            do { LATAbits.LATA1 = 1; } while(0)
#define IO_RA1_SetLow()             do { LATAbits.LATA1 = 0; } while(0)
#define IO_RA1_Toggle()             do { LATAbits.LATA1 = ~LATAbits.LATA1; } while(0)
#define IO_RA1_GetValue()           PORTAbits.RA1
#define IO_RA1_SetDigitalInput()    do { TRISAbits.TRISA1 = 1; } while(0)
#define IO_RA1_SetDigitalOutput()   do { TRISAbits.TRISA1 = 0; } while(0)
#define IO_RA1_SetPullup()          do { WPUAbits.WPUA1 = 1; } while(0)
#define IO_RA1_ResetPullup()        do { WPUAbits.WPUA1 = 0; } while(0)
#define IO_RA1_SetPushPull()        do { ODCONAbits.ODCA1 = 0; } while(0)
#define IO_RA1_SetOpenDrain()       do { ODCONAbits.ODCA1 = 1; } while(0)
#define IO_RA1_SetAnalogMode()      do { ANSELAbits.ANSA1 = 1; } while(0)
#define IO_RA1_SetDigitalMode()     do { ANSELAbits.ANSA1 = 0; } while(0)

// get/set RX_CW aliases
#define RX_CW_TRIS                 TRISAbits.TRISA2
#define RX_CW_LAT                  LATAbits.LATA2
#define RX_CW_PORT                 PORTAbits.RA2
#define RX_CW_WPU                  WPUAbits.WPUA2
#define RX_CW_OD                   ODCONAbits.ODCA2
#define RX_CW_ANS                  ANSELAbits.ANSA2
#define RX_CW_SetHigh()            do { LATAbits.LATA2 = 1; } while(0)
#define RX_CW_SetLow()             do { LATAbits.LATA2 = 0; } while(0)
#define RX_CW_Toggle()             do { LATAbits.LATA2 = ~LATAbits.LATA2; } while(0)
#define RX_CW_GetValue()           PORTAbits.RA2
#define RX_CW_SetDigitalInput()    do { TRISAbits.TRISA2 = 1; } while(0)
#define RX_CW_SetDigitalOutput()   do { TRISAbits.TRISA2 = 0; } while(0)
#define RX_CW_SetPullup()          do { WPUAbits.WPUA2 = 1; } while(0)
#define RX_CW_ResetPullup()        do { WPUAbits.WPUA2 = 0; } while(0)
#define RX_CW_SetPushPull()        do { ODCONAbits.ODCA2 = 0; } while(0)
#define RX_CW_SetOpenDrain()       do { ODCONAbits.ODCA2 = 1; } while(0)
#define RX_CW_SetAnalogMode()      do { ANSELAbits.ANSA2 = 1; } while(0)
#define RX_CW_SetDigitalMode()     do { ANSELAbits.ANSA2 = 0; } while(0)

// get/set KEY_1 aliases
#define KEY_1_TRIS                 TRISAbits.TRISA4
#define KEY_1_LAT                  LATAbits.LATA4
#define KEY_1_PORT                 PORTAbits.RA4
#define KEY_1_WPU                  WPUAbits.WPUA4
#define KEY_1_OD                   ODCONAbits.ODCA4
#define KEY_1_ANS                  ANSELAbits.ANSA4
#define KEY_1_SetHigh()            do { LATAbits.LATA4 = 1; } while(0)
#define KEY_1_SetLow()             do { LATAbits.LATA4 = 0; } while(0)
#define KEY_1_Toggle()             do { LATAbits.LATA4 = ~LATAbits.LATA4; } while(0)
#define KEY_1_GetValue()           PORTAbits.RA4
#define KEY_1_SetDigitalInput()    do { TRISAbits.TRISA4 = 1; } while(0)
#define KEY_1_SetDigitalOutput()   do { TRISAbits.TRISA4 = 0; } while(0)
#define KEY_1_SetPullup()          do { WPUAbits.WPUA4 = 1; } while(0)
#define KEY_1_ResetPullup()        do { WPUAbits.WPUA4 = 0; } while(0)
#define KEY_1_SetPushPull()        do { ODCONAbits.ODCA4 = 0; } while(0)
#define KEY_1_SetOpenDrain()       do { ODCONAbits.ODCA4 = 1; } while(0)
#define KEY_1_SetAnalogMode()      do { ANSELAbits.ANSA4 = 1; } while(0)
#define KEY_1_SetDigitalMode()     do { ANSELAbits.ANSA4 = 0; } while(0)

// get/set KEY_0 aliases
#define KEY_0_TRIS                 TRISAbits.TRISA5
#define KEY_0_LAT                  LATAbits.LATA5
#define KEY_0_PORT                 PORTAbits.RA5
#define KEY_0_WPU                  WPUAbits.WPUA5
#define KEY_0_OD                   ODCONAbits.ODCA5
#define KEY_0_ANS                  ANSELAbits.ANSA5
#define KEY_0_SetHigh()            do { LATAbits.LATA5 = 1; } while(0)
#define KEY_0_SetLow()             do { LATAbits.LATA5 = 0; } while(0)
#define KEY_0_Toggle()             do { LATAbits.LATA5 = ~LATAbits.LATA5; } while(0)
#define KEY_0_GetValue()           PORTAbits.RA5
#define KEY_0_SetDigitalInput()    do { TRISAbits.TRISA5 = 1; } while(0)
#define KEY_0_SetDigitalOutput()   do { TRISAbits.TRISA5 = 0; } while(0)
#define KEY_0_SetPullup()          do { WPUAbits.WPUA5 = 1; } while(0)
#define KEY_0_ResetPullup()        do { WPUAbits.WPUA5 = 0; } while(0)
#define KEY_0_SetPushPull()        do { ODCONAbits.ODCA5 = 0; } while(0)
#define KEY_0_SetOpenDrain()       do { ODCONAbits.ODCA5 = 1; } while(0)
#define KEY_0_SetAnalogMode()      do { ANSELAbits.ANSA5 = 1; } while(0)
#define KEY_0_SetDigitalMode()     do { ANSELAbits.ANSA5 = 0; } while(0)

// get/set RC0 procedures
#define RC0_SetHigh()            do { LATCbits.LATC0 = 1; } while(0)
#define RC0_SetLow()             do { LATCbits.LATC0 = 0; } while(0)
#define RC0_Toggle()             do { LATCbits.LATC0 = ~LATCbits.LATC0; } while(0)
#define RC0_GetValue()              PORTCbits.RC0
#define RC0_SetDigitalInput()    do { TRISCbits.TRISC0 = 1; } while(0)
#define RC0_SetDigitalOutput()   do { TRISCbits.TRISC0 = 0; } while(0)
#define RC0_SetPullup()             do { WPUCbits.WPUC0 = 1; } while(0)
#define RC0_ResetPullup()           do { WPUCbits.WPUC0 = 0; } while(0)
#define RC0_SetAnalogMode()         do { ANSELCbits.ANSC0 = 1; } while(0)
#define RC0_SetDigitalMode()        do { ANSELCbits.ANSC0 = 0; } while(0)

// get/set RC1 procedures
#define RC1_SetHigh()            do { LATCbits.LATC1 = 1; } while(0)
#define RC1_SetLow()             do { LATCbits.LATC1 = 0; } while(0)
#define RC1_Toggle()             do { LATCbits.LATC1 = ~LATCbits.LATC1; } while(0)
#define RC1_GetValue()              PORTCbits.RC1
#define RC1_SetDigitalInput()    do { TRISCbits.TRISC1 = 1; } while(0)
#define RC1_SetDigitalOutput()   do { TRISCbits.TRISC1 = 0; } while(0)
#define RC1_SetPullup()             do { WPUCbits.WPUC1 = 1; } while(0)
#define RC1_ResetPullup()           do { WPUCbits.WPUC1 = 0; } while(0)
#define RC1_SetAnalogMode()         do { ANSELCbits.ANSC1 = 1; } while(0)
#define RC1_SetDigitalMode()        do { ANSELCbits.ANSC1 = 0; } while(0)

// get/set SIDE_TONE aliases
#define SIDE_TONE_TRIS                 TRISCbits.TRISC2
#define SIDE_TONE_LAT                  LATCbits.LATC2
#define SIDE_TONE_PORT                 PORTCbits.RC2
#define SIDE_TONE_WPU                  WPUCbits.WPUC2
#define SIDE_TONE_OD                   ODCONCbits.ODCC2
#define SIDE_TONE_ANS                  ANSELCbits.ANSC2
#define SIDE_TONE_SetHigh()            do { LATCbits.LATC2 = 1; } while(0)
#define SIDE_TONE_SetLow()             do { LATCbits.LATC2 = 0; } while(0)
#define SIDE_TONE_Toggle()             do { LATCbits.LATC2 = ~LATCbits.LATC2; } while(0)
#define SIDE_TONE_GetValue()           PORTCbits.RC2
#define SIDE_TONE_SetDigitalInput()    do { TRISCbits.TRISC2 = 1; } while(0)
#define SIDE_TONE_SetDigitalOutput()   do { TRISCbits.TRISC2 = 0; } while(0)
#define SIDE_TONE_SetPullup()          do { WPUCbits.WPUC2 = 1; } while(0)
#define SIDE_TONE_ResetPullup()        do { WPUCbits.WPUC2 = 0; } while(0)
#define SIDE_TONE_SetPushPull()        do { ODCONCbits.ODCC2 = 0; } while(0)
#define SIDE_TONE_SetOpenDrain()       do { ODCONCbits.ODCC2 = 1; } while(0)
#define SIDE_TONE_SetAnalogMode()      do { ANSELCbits.ANSC2 = 1; } while(0)
#define SIDE_TONE_SetDigitalMode()     do { ANSELCbits.ANSC2 = 0; } while(0)

// get/set TX_CW aliases
#define TX_CW_TRIS                 TRISCbits.TRISC3
#define TX_CW_LAT                  LATCbits.LATC3
#define TX_CW_PORT                 PORTCbits.RC3
#define TX_CW_WPU                  WPUCbits.WPUC3
#define TX_CW_OD                   ODCONCbits.ODCC3
#define TX_CW_ANS                  ANSELCbits.ANSC3
#define TX_CW_SetHigh()            do { LATCbits.LATC3 = 1; } while(0)
#define TX_CW_SetLow()             do { LATCbits.LATC3 = 0; } while(0)
#define TX_CW_Toggle()             do { LATCbits.LATC3 = ~LATCbits.LATC3; } while(0)
#define TX_CW_GetValue()           PORTCbits.RC3
#define TX_CW_SetDigitalInput()    do { TRISCbits.TRISC3 = 1; } while(0)
#define TX_CW_SetDigitalOutput()   do { TRISCbits.TRISC3 = 0; } while(0)
#define TX_CW_SetPullup()          do { WPUCbits.WPUC3 = 1; } while(0)
#define TX_CW_ResetPullup()        do { WPUCbits.WPUC3 = 0; } while(0)
#define TX_CW_SetPushPull()        do { ODCONCbits.ODCC3 = 0; } while(0)
#define TX_CW_SetOpenDrain()       do { ODCONCbits.ODCC3 = 1; } while(0)
#define TX_CW_SetAnalogMode()      do { ANSELCbits.ANSC3 = 1; } while(0)
#define TX_CW_SetDigitalMode()     do { ANSELCbits.ANSC3 = 0; } while(0)

// get/set KEY_1A aliases
#define KEY_1A_TRIS                 TRISCbits.TRISC4
#define KEY_1A_LAT                  LATCbits.LATC4
#define KEY_1A_PORT                 PORTCbits.RC4
#define KEY_1A_WPU                  WPUCbits.WPUC4
#define KEY_1A_OD                   ODCONCbits.ODCC4
#define KEY_1A_ANS                  ANSELCbits.ANSC4
#define KEY_1A_SetHigh()            do { LATCbits.LATC4 = 1; } while(0)
#define KEY_1A_SetLow()             do { LATCbits.LATC4 = 0; } while(0)
#define KEY_1A_Toggle()             do { LATCbits.LATC4 = ~LATCbits.LATC4; } while(0)
#define KEY_1A_GetValue()           PORTCbits.RC4
#define KEY_1A_SetDigitalInput()    do { TRISCbits.TRISC4 = 1; } while(0)
#define KEY_1A_SetDigitalOutput()   do { TRISCbits.TRISC4 = 0; } while(0)
#define KEY_1A_SetPullup()          do { WPUCbits.WPUC4 = 1; } while(0)
#define KEY_1A_ResetPullup()        do { WPUCbits.WPUC4 = 0; } while(0)
#define KEY_1A_SetPushPull()        do { ODCONCbits.ODCC4 = 0; } while(0)
#define KEY_1A_SetOpenDrain()       do { ODCONCbits.ODCC4 = 1; } while(0)
#define KEY_1A_SetAnalogMode()      do { ANSELCbits.ANSC4 = 1; } while(0)
#define KEY_1A_SetDigitalMode()     do { ANSELCbits.ANSC4 = 0; } while(0)

// get/set KEY_0A aliases
#define KEY_0A_TRIS                 TRISCbits.TRISC5
#define KEY_0A_LAT                  LATCbits.LATC5
#define KEY_0A_PORT                 PORTCbits.RC5
#define KEY_0A_WPU                  WPUCbits.WPUC5
#define KEY_0A_OD                   ODCONCbits.ODCC5
#define KEY_0A_ANS                  ANSELCbits.ANSC5
#define KEY_0A_SetHigh()            do { LATCbits.LATC5 = 1; } while(0)
#define KEY_0A_SetLow()             do { LATCbits.LATC5 = 0; } while(0)
#define KEY_0A_Toggle()             do { LATCbits.LATC5 = ~LATCbits.LATC5; } while(0)
#define KEY_0A_GetValue()           PORTCbits.RC5
#define KEY_0A_SetDigitalInput()    do { TRISCbits.TRISC5 = 1; } while(0)
#define KEY_0A_SetDigitalOutput()   do { TRISCbits.TRISC5 = 0; } while(0)
#define KEY_0A_SetPullup()          do { WPUCbits.WPUC5 = 1; } while(0)
#define KEY_0A_ResetPullup()        do { WPUCbits.WPUC5 = 0; } while(0)
#define KEY_0A_SetPushPull()        do { ODCONCbits.ODCC5 = 0; } while(0)
#define KEY_0A_SetOpenDrain()       do { ODCONCbits.ODCC5 = 1; } while(0)
#define KEY_0A_SetAnalogMode()      do { ANSELCbits.ANSC5 = 1; } while(0)
#define KEY_0A_SetDigitalMode()     do { ANSELCbits.ANSC5 = 0; } while(0)

/**
   @Param
    none
   @Returns
    none
   @Description
    GPIO and peripheral I/O initialization
   @Example
    PIN_MANAGER_Initialize();
 */
void PIN_MANAGER_Initialize (void);

/**
 * @Param
    none
 * @Returns
    none
 * @Description
    Interrupt on Change Handling routine
 * @Example
    PIN_MANAGER_IOC();
 */
void PIN_MANAGER_IOC(void);



#endif // PIN_MANAGER_H
/**
 End of File
*/