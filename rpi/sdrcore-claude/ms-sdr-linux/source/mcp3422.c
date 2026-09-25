//===================================================================
//	MC9S08QG8 Wattmeter MCP3422 routines	11/03/2010
//	Written by John H. Fisher - K5JHF
//===================================================================


#include "mcp3422.h"
#include <math.h>
#include <stdio.h>
#include "extern.h"

char G_MCP_buffer[256] = {0};

void Read_MCP3422(MCP3422_Data *read, char control)
{
        int status = 0;
        char filename[PATH_MAX] = {0};

        char *argv[10];
        char dummy[] = "Read_MCP3422";
        char yes[] = "-y";
        char first[] = "w1@0x68";
        //char second[] = "0x9C";
        char third[] = "r4";
        char i2cbuss[] = "6";
        unsigned char control_string[20] = {0};

        //control_string[0] = control;
        sprintf(control_string, "0x%0X", control);
        argv[0] = dummy;
        argv[1] = yes;
        argv[2] = i2cbuss;
        argv[3] = first;
        argv[4] = control_string;
        argv[5] = third;

        status = Meter_Read_i2c_direct(6, argv);

        read->data.b [3] = 0;
        read->data.b [2] = G_MCP_buffer[0]; // Zero
        read->data.b [1] = G_MCP_buffer[1];
        read->data.b [0] = G_MCP_buffer[2];

        read->status.Byte = G_MCP_buffer[3];
 
        if ((read->data.ADC_Data << 8) < 0) read->data.ADC_Data = 0;
}

