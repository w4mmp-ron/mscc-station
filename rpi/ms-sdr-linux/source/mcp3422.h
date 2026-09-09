//===================================================================
//	MC9S08QG8 Wattmeter MCP3422 routines	11/03/2010
//	Written by John H. Fisher - K5JHF
//===================================================================
#include "typedefs.h"
//#include <stdint-gcc.h>
#define MCP3422_Slave_Address 0x68

#define x1   0b00
#define x2   0b01
#define x4   0b10
#define x8   0b11

#define _12bit  0b00
#define _14bit  0b01
#define _16bit  0b10
#define _18bit  0b11

#define One_Shot 0b0 
#define Cont  0b1

#define Channel0 0b00
#define  Reverse_Channel 0b00
#define Channel1 0b01
#define  Forward_Channel 0b01


#define Start_Conv 0b1

/*======================================================================*/

typedef union {
    byte Byte;

    struct {
        byte G0 : 1;
        byte G1 : 1;
        byte S0 : 1;
        byte S1 : 1;
        byte C_O : 1;
        byte Ch0 : 1;
        byte Ch1 : 1;
        byte _RDY : 1;
    } Bits;

    struct {
        byte Gain : 2;
        byte SPS : 2;
byte:
        1;
        byte Chan : 2;
byte:
        1;
    } MergedBits;

} MCP3422_Config;

/*======================================================================*/

typedef struct {
    union {
        uint32_t ADC_Data;
        byte b[4];
    } data;
    MCP3422_Config status;
} MCP3422_Data;

typedef struct {
    union {
        uint32_t ADC_Data;
        byte b[4];
    } data;
} MCP3422_Read_data;

/*======================================================================*/

//	extern	unsigned char	MCP3422_status;

/*======================================================================*/

void Write_MCP3422_Config(char data);

void Read_MCP3422(MCP3422_Data *read, char control);

// long	Read_MCP3422_Pressure ( void );

// double Altitude_ft ( double Pressure_hPa, double Pressure_sea_level_hPa );

// double Absolute_Pressure_hPa ( void );

/*======================================================================*/
