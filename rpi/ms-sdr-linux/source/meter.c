//===================================================================
//	MC9S08QG8 Wattmeter main	12/04/2010
//	Written by John H. Fisher - K5JHF
// Modified by Ron Patton (W4MMP) for the mWattmeter-III
//===================================================================
/*
    i2ctransfer.c - A user-space program to send concatenated i2c messages
    Copyright (C) 2015-17 Wolfram Sang <wsa@sang-engineering.com>
    Copyright (C) 2015-17 Renesas Electronics Corporation

    Based on i2cget.c:
    Copyright (C) 2005-2012  Jean Delvare <jdelvare@suse.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 */



#include <stdio.h>
#include <string.h>
#include <math.h>
//#include <linux/unistd.h>
//#include <sys/unistd.h>
#include <stdlib.h>
//#include <sys/ioctl.h>
#include <errno.h>
//#include <unistd.h>
//#include <linux/i2c.h>
//#include <unistd.h>
#include <fcntl.h>
//#include <sys/ioctl.h>
//#include <linux/i2c-dev.h>
//#include "i2cbusses.h"
#include "util.h"
#include "extern.h"
#include "usbavrcmd.h"
#include "mcp3422.h"
#include "typedefs.h"
//#include <wiringPi.h>
//#include "Flash_stuff.h"


#define CALIBRATE_OFFSET 0.0011
#define DISPLAY_DELAY 0
#define F_ERROR 0x01;
#define R_ERROR 0x02;
#define READ_TIME_OUT 0x03
#define OVER_VOLTS 0x04
#define CALIBRATION_COUNT 29
#define MIA_COUNT_LIMIT 2
#define CALIBRATING_VOLTAGE_LIMIT 0.8f
#define NORMAL_VOLTAGE_LIMIT 2.2f

#define POWER_SCALE 5.0
#define HIGH_POWER_LIMIT 10
#define SWR_SHUTDOWN_LIMIT 10
#define I2C_FAILURE_LIMIT 100
#define METER_HOLD_LIMIT 1024

#define READ_RETRY_LIMIT 3
#define N_DECIMAL_POINTS_PRECISION_METER (1000)



//===================================================================
////

//change to 11 bands by KAT 12/15/2012 for mWattmeter II
//all coefficients changed by KAT 12/30/2012 for mWattmeter-II 
//

//double	const	Coefficient [ 11 ] [ 3 ] [ 4 ]	=	{
double const Coefficient [ 10 ] [ 3 ] [ 2 ] = {

    { //	Band 0	"10m"
        {0.6183, 6.5067},
        {0.5886, 5.8384},
        {0.8485, 5.3000}
    },

    { //	Band 1	"12m"
        {0.6128, 6.4354},
        {0.5885, 5.7868},
        {0.8352, 5.5000}
    },

    { //	Band 2	"15m"
        {0.6156, 6.1901},
        {0.5895, 5.8145},
        {0.8246, 5.2000}
    },

    { //	Band 3	"17m"
        {0.6037, 6.3604},
        {0.5795, 5.7837},
        {0.9758, 5.1994}
    },

    { //	Band 4	"20m"
        {0.6037, 6.3604},
        {0.5796, 5.7962},
        {0.9341, 5.2839}
    },

    { //	Band 5	"30m"
        {0.5858, 6.3799},
        {0.3273, 7.1370},
        {0.8462, 5.3000}
    },

    { //	Band 6 "40m"
        {0.6265, 6.6363},
        {0.5852, 4.9456},
        {0.8035, 5.3000}
    },

    { //	Band 7 "60m"
        {0.6164, 6.5842},
        {0.5097, 4.9761},
        {0.7106, 5.3000}
    },

    { //	Band 8 "80m"
        {0.6156, 6.1901},
        {0.4761, 4.6737},
        {0.6436, 5.2000}
    },

    { //	Band 9 "160m"
        {0.6156, 6.1901},
        {0.4263, 4.1454},
        {0.8528, 4.7500} //0.8528     // 3.6359
    }
};

/*if ( Volts >= 0.450 )	{
              Watts	=	Volts * (Coefficient [ Band ] [ 2 ] [ 0 ] + 
                                        Volts * (Coefficient [ Band ] [ 2 ] [ 1 ] ));
 */

//===================================================================

//byte HIGH_LOW = 1;
byte G_swr_shutdown = FALSE;
byte G_hi_low = 0;
byte G_high_power_flag = FALSE;
//byte G_Meter_allow = FALSE;
int G_Meter_hold = METER_HOLD_LIMIT;
byte G_Meter_hi_low = 0;
byte G_Meter_Calibrated = FALSE;
int Mensuro_file = 0;
byte G_Start_Mensuro_Calibrate = FALSE;

//byte OVER_POWER = 1;
byte Band = 0;
//byte Error;

double Volts = 0.0f;
double Forward_Power = 0.0f;
double Reverse_Power = 0.0f;
double Rho = 0.0f;
double SWR = 0.0f;
double Previous_Forward_Power;
double F_Volts = 0.0f;
double R_Volts = 0.0f;
double F_Volts_Cal = 0.0f;
double R_Volts_Cal = 0.0f;
double F_Volts_Avg_Cal = 0.0f;
double R_Volts_Avg_Cal = 0.0f;

MCP3422_Config Config;
MCP3422_Data Read_Data;
MCP3422_Read_data Received_Data;

byte avg_count = 0;
byte status;

//byte previous_hi_low = 0;
int high_power_count = 0;
int swr_shutdown_count = 0;
//byte over_volt_count_limit = 0;
//int process_toggle = FALSE;

byte G_MCP_buffer[256] = {0};

enum parse_state {
    PARSE_GET_DESC,
    PARSE_GET_DATA,
};

#define PRINT_STDERR (1 << 0)
#define PRINT_READ_BUF (1 << 1)
#define PRINT_WRITE_BUF (1 << 2)
#define PRINT_HEADER (1 << 3)

//extern char G_MCP_buffer[256];

int Open_Mensuro() {
    char *filename = (char *) "/dev/i2c-6";
    int status = 0;
    byte reset = 0x06;
    int write_count = 0;

    if ((Mensuro_file = open(filename, O_RDWR)) < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Open_Mensuro. Buss Open. FAILED. file_i2c: %d,\n", line_number++, Mensuro_file);
        status = -1;
    } else {
        int addr = 0x00; //<<<<<The I2C address of the slave
        if (ioctl(Mensuro_file, I2C_SLAVE, addr) < 0) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Open_Mensuro. Set general address FAILED. address: 0x:%0X\n", line_number++, addr);
            status = -1;
        } else {
            write_count = write(Mensuro_file, &reset, 1);
            if (write_count != 1) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Open_Mensuro. write FAILED. error:%d\n", line_number++, write_count);
                status = -1;
            } else {
                int addr = METER_ADDRESS; //<<<<<The I2C address of the slave
                if (ioctl(Mensuro_file, I2C_SLAVE, addr) < 0) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Open_Mensuro. Set address FAILED. address: 0x:%0X\n", line_number++, addr);
                    status = -1;
                }
            }
        }
    }
    return status;
}

int Write_Mensuro() {
    int status = 0;
    int write_count = 0;
    int buffer_size = 0;

    buffer_size = sizeof (Config.Byte);
    write_count = write(Mensuro_file, &Config.Byte, buffer_size);
    if (write_count != buffer_size) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Write_Mensuro. FAILED. error:%d\n", line_number++, write_count);
        status = -1;
    }
    return status;

}

int Read_Mensuro(MCP3422_Data *read_data, char control) {
    int status = 0;
    //unsigned char control_string[20] = {0};
    //int count = 0;
    byte first_data_byte = 0;
    byte second_data_byte = 0;
    byte third_data_byte = 0;
    byte control_byte = 0;
    int read_count = 0;
    int length = 0;

    memset(G_MCP_buffer, 0, sizeof (G_MCP_buffer));
    length = sizeof (G_MCP_buffer);
    read_count = read(Mensuro_file, G_MCP_buffer, length);
    if (read_count != length) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Read_Mensuro. FAILED, read_count: %d, length: %d\n", line_number++, read_count, length);
        status = -1;
    } else {
        first_data_byte = G_MCP_buffer[0] & 0x03;
        second_data_byte = G_MCP_buffer[1];
        third_data_byte = G_MCP_buffer[2];
        control_byte = G_MCP_buffer[3];
        read_data->data.ADC_Data = 0;

        Received_Data.data.b[3] = 0;
        Received_Data.data.b[2] = first_data_byte;
        Received_Data.data.b[1] = second_data_byte;
        Received_Data.data.b[0] = third_data_byte;

        read_data->status.Byte = control_byte;
    }
    return status;
}

int Read_Power(byte channel, double voltage_limit) {
    double Watts;
    double forward_ADC_Data = 0.0f;
    double reverse_ADC_Data = 0.0f;
    int loop_count = 0;
    int status = 0;
    int volts_status = 0;
    char message[PATH_MAX] = {0};
    static byte message_sent = FALSE;
    static byte over_volts_count = 0;

    Config.MergedBits.Chan = channel;
    Config.Bits._RDY = Start_Conv;

    //Read_MCP3422(&Read_Data, Config.Byte);
    status = Write_Mensuro();
    if (status == -1) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Read_Power.  Write_Mensuro FAILED. %d\n", line_number++, status);
        return status;
    }
    Sleep(50);
    Read_Mensuro(&Read_Data, Config.Byte);
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Read_Power . Status Bits._RDY: %d\n", line_number++, Read_Data.status.Bits._RDY);
    while (Read_Data.status.Bits._RDY == 1) {
        //Read_MCP3422(&Read_Data, Config.Byte);
        Read_Mensuro(&Read_Data, Config.Byte);
        Sleep(50);
        if (loop_count > 7) {
            status = READ_TIME_OUT;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . Read FAILED on Loop Count: %d\n", line_number++, __func__, loop_count);
            strcpy(message, "Mensuro: Read on Loop Count: FAILED");
            Gui_Add_Message(message);
            return status;
        }
        loop_count++;
    }
    switch (channel) {
        case Channel1: //Forward Channel
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] Read_Power . Forward . ADC_DATA: %d\n", line_number++, Read_Data.data.ADC_Data);
            F_Volts = (double) Received_Data.data.ADC_Data * 15.625E-6;
            forward_ADC_Data = (double) Received_Data.data.ADC_Data;
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] Read_Power . forward_ADC_Data: %f\n", line_number++, forward_ADC_Data);
            F_Volts = (F_Volts - F_Volts_Cal);
            if (F_Volts <= 0.0000000) F_Volts = 0.00001;
            Volts = F_Volts;
            break;
        default: //Reverse Channel
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] Read_Power . Reverse .  ADC_DATA: %d\n", line_number++, Read_Data.data.ADC_Data);
            R_Volts = (double) Received_Data.data.ADC_Data * 15.625E-6;
            reverse_ADC_Data = (double) Received_Data.data.ADC_Data;
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] Read_Power . Reverse Volts: %f\n", line_number++, R_Volts);
            R_Volts = (R_Volts - R_Volts_Cal);
            if (R_Volts <= 0.00000000) R_Volts = 0.00001;
            Volts = R_Volts;
    }
    if ((Volts > voltage_limit)) {
        switch (channel) {
            case Channel1:
                volts_status |= F_ERROR;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Meter_main Read_Power . Forward Error: %d, Volts: %f\n",
                        line_number++, volts_status, Volts);
                break;

            case Channel0:
                volts_status |= R_ERROR;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Meter_main Read_Power . Reverse Error: %d, Volts: %f\n",
                        line_number++, volts_status, Volts);
                break;
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Meter_main Read_Power Read_Power . Over VOLTAGE. voltage_limit:%f \n",
                line_number++, voltage_limit);
        if (++over_volts_count > 0) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Meter_main . Over VOLTAGE. over_volts_count: %d \n",
                    line_number++, over_volts_count);
            /*if (message_sent == FALSE) {
                strcpy(message, "Mensuro. over_volts ERROR COUNT REACHED");
                message_queue_add(CMD_SET_SERVER_ERROR, message, strlen(message));
                message_sent = TRUE;
            }*/
            over_volts_count = 0;
            message_sent = FALSE;
            status = -1;
            return status;
        }
    }
    if (status == 0) {
        if (Volts >= 0.450) {
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] Read_Power . Volts GREATER THAN 0.450.  Volts: %f, Band: %d \n", line_number++, Volts, Band);
            Watts = Volts * (Coefficient [ Band ] [ 2 ] [ 0 ] +
                    Volts * (Coefficient [ Band ] [ 2 ] [ 1 ]));
        } else
            if (Volts >= 0.045) {
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] Read_Power . GREATER THAN 0.045.  Volts: %f, Band: %d\n", line_number++, Volts, Band);
            Watts = Volts * (Coefficient [ Band ] [ 1 ] [ 0 ] +
                    Volts * (Coefficient [ Band ] [ 1 ] [ 1 ]));
        } else {
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] Read_Power . Volts LESS THAN 0.045.  Volts: %f, Band: %d \n", line_number++, Volts, Band);
            Watts = Volts * (Coefficient [ Band ] [ 0 ] [ 0 ] +
                    Volts * (Coefficient [ Band ] [ 0 ] [ 1 ]));
        }
        //print_time(0);
        switch (channel) {
            case Channel1:
                Forward_Power = Watts;
                //fprintf(G_fp_logfile, "[%d] Read_Power . FORWARD WATTS: %f\n", line_number++, Watts);
                break;
            default:
                Reverse_Power = Watts;
                //fprintf(G_fp_logfile, "[%d] Read_Power . REVERSE WATTS: %f\n", line_number++, Watts);
        }
    }
    return status;
}

int meter_initialize() {
    int status = TRUE;
    FILE *meter_ini;
    char file_name[PATH_MAX] = {0};
    //const char *homedir;
    char init_record[300];
    char *record;
    float mynumber;
    float forward = 0;
    float reverse = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] meter_initialize . Called.\n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] meter_initialize . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/meter.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] meter_initialize. Filename: %s\n", line_number++, file_name);
        meter_ini = fopen(file_name, "r");
        if (meter_ini != NULL) {
            while (fgets(init_record, sizeof (init_record), meter_ini) != NULL) {
                record = strstr(init_record, "FORWARD");
                if (record != NULL) {
                    mynumber = atof((record + sizeof ("FORWARD")));
                    F_Volts_Cal = mynumber;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] meter_initialize. F_Volts_Cal: %f\n", line_number++, F_Volts_Cal);
                }
                record = strstr(init_record, "REVERSE");
                if (record != NULL) {
                    mynumber = atof((record + sizeof ("REVERSE")));
                    R_Volts_Cal = mynumber;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] meter_initialize. R_Volts_Cal: %f\n", line_number++, R_Volts_Cal);
                }
            }
            fclose(meter_ini);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] meter_initialize . FAILED.\n", line_number++);
            fflush(G_fp_logfile);
            status = FALSE;
        }
    }
    return status;
}

int meter_update_init_file() {
    int status = TRUE;
    FILE *meter_ini;
    char file_name[PATH_MAX] = {0};
    const char *homedir;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] meter_update_init_file. Called.\n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] meter_update_init_file. Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/meter.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] meter_update_init_file. Filename: %s\n", line_number++, file_name);
        meter_ini = fopen(file_name, "w");
        if (meter_ini != NULL) {
            fprintf(meter_ini, "FORWARD=%f\n", F_Volts_Cal);
            fprintf(meter_ini, "REVERSE=%f\n", R_Volts_Cal);
            fclose(meter_ini);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] meter_update_init_file . FAILED.\n", line_number++);
            fflush(G_fp_logfile);
            status = FALSE;
        }
    }
    return status;
}

int meter_calibrate(void) {
    int forward_status = 0;
    int reverse_status = 0;
    int status = 0;
    int error_count = 0;
    int token_status = 0;
    int token_error = 0;
    char message[PATH_MAX] = {0};

    //G_Meter_Processing = TRUE;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] meter_calibrate STARTED\n", line_number++);
    forward_status = Read_Power(Forward_Channel, CALIBRATING_VOLTAGE_LIMIT); //Forward.  Toss the first meter read after startup. It is bogus
    reverse_status = Read_Power(Reverse_Channel, CALIBRATING_VOLTAGE_LIMIT); //Reverse
    forward_status = 0;
    reverse_status = 0;
    for (avg_count = 0; avg_count < CALIBRATION_COUNT; avg_count++) {
        token_status = pthread_mutex_trylock(&Master_Device_Token_available);
        if (token_status == 0) {
            forward_status = Read_Power(Forward_Channel, CALIBRATING_VOLTAGE_LIMIT); //Forward
            reverse_status = Read_Power(Reverse_Channel, CALIBRATING_VOLTAGE_LIMIT); //Reverse
            if (forward_status == 0 && reverse_status == 0) {
                F_Volts_Avg_Cal += F_Volts;
                R_Volts_Avg_Cal += R_Volts;
            } else {
                error_count++;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] meter_calibrate. error_count:%d\n", line_number++, error_count);
                if (error_count >= 10) {
                    status = -1;
                    return status;
                }
            }
            pthread_mutex_unlock(&Master_Device_Token_available);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] meter_calibrate. pthread_mutex_trylock FAILED\n", line_number++);
            token_error++;
            if (token_error >= CALIBRATION_COUNT - 1) {
                status = -1;
                return status;
            }
        }
        Sleep(Get_random_time());
    }
    if (status == 0) {
        F_Volts_Cal = (F_Volts_Avg_Cal / (double) (--avg_count));
        R_Volts_Cal = (R_Volts_Avg_Cal / (double) (--avg_count));
        meter_update_init_file();
        G_Meter_Calibrated = TRUE;
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Meter_main. meter_calibrate FINISHED. status: %d, token_error: %d, F_Volts_Cal: %f,  R_Volts_Cal: %f\n",
            line_number++, status, token_error, F_Volts_Cal, R_Volts_Cal);
    return status;
}

void Set_band() {
    static uint8_t previous_band = 0;

    if (previous_band != G_band_normal) {
        switch (G_band_normal) {
            case 160:
                Band = 9;
                break;
            case 80:
                Band = 8;
                break;
            case 60:
                Band = 7;
                break;
            case 40:
                Band = 6;
                break;
            case 30:
                Band = 5;
                break;
            case 20:
                Band = 4;
                break;
            case 17:
                Band = 3;
                break;
            case 15:
                Band = 2;
                break;
            case 12:
                Band = 1;
                break;
            case 10:
                Band = 0;
                break;
            default:
                Band = 180;
        }
        previous_band = G_band_normal;
    }
}

void Send_data() {
    uint8_t swr_send;
    uint32_t forward_send;
    uint32_t reverse_send;
    byte extended_data[16];
    uint32_t frac_part;
    uint32_t int_part;
    float decimal_part_float = 0.0f;
    unsigned int decimal_part_int = 0;

    //Send the SWR
    swr_send = (uint8_t) (SWR * 10.0f);
    memcpy(&extended_data[0], &swr_send, sizeof (swr_send));
    Gui_send_param_extended(CMD_SET_SWR, extended_data, sizeof (swr_send));

    //Send the Forward Power
    frac_part = (int) Forward_Power;
    decimal_part_float = (float) Forward_Power - frac_part;
    decimal_part_float *= 1000;
    int_part = ((uint32_t) Forward_Power) * 1000;
    decimal_part_int = (int) decimal_part_float;
    forward_send = (int_part + (int) decimal_part_int);
    memcpy(&extended_data[0], &forward_send, sizeof (forward_send));
    Gui_send_param_extended(CMD_SET_FORWARD_POWER, extended_data, sizeof (forward_send));
    SDRcore_trans_send_param(CMD_SET_FOWARD_POWER_VALUE, forward_send);

    //Send the Reverse Power
    frac_part = (int) Reverse_Power;
    decimal_part_float = (float) Reverse_Power - frac_part;
    decimal_part_float *= 1000;
    int_part = ((uint32_t) Reverse_Power) * 1000;
    decimal_part_int = (int) decimal_part_float;
    reverse_send = (int_part + (int) decimal_part_int);
    memcpy(&extended_data[0], &reverse_send, sizeof (reverse_send));
    Gui_send_param_extended(CMD_SET_REVERSE_POWER, extended_data, sizeof (reverse_send));
}

int Meter_HILO_Init() {
    char filename[PATH_MAX] = {0};
    int status = 0;
    unsigned char buffer;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Meter_main. Meter_HILO_Init . STARTED\n", line_number++);
    buffer = 0x00;
    status = Write_i2c(I2C_BUSS, filename, METER_HILO_ADDRESS, 0x03, &buffer, 1, "Meter_HILO");

    if (status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Meter_main. Meter_HILO_Init . Write_i2c FAILED: %d\n", line_number++, status);
        status = -1;
    } else {
        status = 0;
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Meter_main. Meter_HILO_Init . FINISHED\n", line_number++);
    return status;
}

void Set_Mensuro() {
    static byte previous_hi_low = 100;
    unsigned char buffer = 0;
    char filename[PATH_MAX] = {0};
    //static byte startup_delay = 0;

    if (previous_hi_low != G_Meter_hi_low) {
        if (G_Meter_hi_low) { // G_Meter_hi_low = 1, high power mode
            buffer = 0x00;
        } else {
            buffer = 0xFF;
        }
        status = Write_i2c(I2C_BUSS, filename, METER_HILO_ADDRESS, 0x01, &buffer, 1, "Meter_main");
        if (status != -1) {
            status = 0;
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Meter_main. Set_Mensuro. G_hi_low: %d, buffer: 0x%X\n", line_number++, G_Meter_hi_low, buffer);
        previous_hi_low = G_Meter_hi_low;
    }
}

void *Meter_main(void *param) {
    Config.MergedBits.Gain = x1;
    Config.MergedBits.SPS = _18bit;
    Config.Bits.C_O = Cont;
    int status = 0;
    int forward_status = 0;
    int reverse_status = 0;
    byte zero_sent = FALSE;
    int zero_counter = 0;
    int read_retry_count = 0;
    //byte tx_mode = 0;
    byte previous_tx_mode = 10;
    byte previous_qrp = 10;
    unsigned char buffer;
    char filename[PATH_MAX] = {0};
    int token_status = 0;
    long long previous_lock_failed = 0;
    long long lock_failed_diff = 0;
    long long previous_lock_failed_diff = 0;
    long long lock_failed = 0;
    char message[PATH_MAX] = {0};

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Meter_main. STARTED. \n", line_number++);
    //Sleep(10000);
    status = Open_Mensuro();
    if (status >= 0) {
        status = Meter_HILO_Init();
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Meter_main. Open_Mensuro FAILED. SEND LOGS TO MULTUS SDR, LLC.\n", line_number++);
    }
    meter_initialize();
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Meter_main . Now Running. G_Transceiver_type: %d\n", line_number++, G_Transceiver_type);
    while (G_all_threads_run && status == 0) {
        if (G_Start_Mensuro_Calibrate) {
            status = meter_calibrate();
            if (status == -1) {
                //strcpy(message, "Mensuro: CALIBRATION FAILED");
                //message_queue_add(CMD_SET_SERVER_ERROR, message, strlen(message));
                Gui_send_param(CMD_MENSURO_CALIBRATE, 0);
            } else {
                Gui_send_param(CMD_MENSURO_CALIBRATE, 1);
            }
            G_Start_Mensuro_Calibrate = FALSE;
        }
        token_status = pthread_mutex_trylock(&Master_Device_Token_available);
        Set_Mensuro();
        if (token_status == 0) {
            if (G_TX == TRUE) {
                Set_band();
                forward_status = Read_Power(Forward_Channel, NORMAL_VOLTAGE_LIMIT); //Forward
                reverse_status = Read_Power(Reverse_Channel, NORMAL_VOLTAGE_LIMIT); //Reverse
                if (forward_status == 0 && reverse_status == 0) {
                    if (Forward_Power > 0.010000000f) {
                        Rho = sqrt(Reverse_Power / Forward_Power);
                        if (Rho > 0.990000000f) {
                            Rho = 0.99000000000000f;
                        }
                        SWR = (1.0f + Rho) / (1.0f - Rho);
                        if ((SWR <= 1.000000f)) {
                            SWR = 1.000000000f;
                            swr_shutdown_count = 0;
                            G_swr_shutdown = 0;
                        }
                        if ((SWR > 9.90000f)) SWR = 9.9000000f;
                        if (G_QRP) { //G_QRP is inverse 
                            Forward_Power *= 5.000000f;
                            Reverse_Power *= 5.00000000f;
                            if ((SWR > 2.000000f && Forward_Power > 3.0f)) {
                                if (swr_shutdown_count++ >= SWR_SHUTDOWN_LIMIT && G_TX == TRUE) {
                                    G_swr_shutdown = 1;
                                    print_time(0);
                                    fprintf(G_fp_logfile, "[%d] Meter_main . swr_shutdown_count: %d, G_swr_shutdown: %d, SWR: %f\n",
                                            line_number++, swr_shutdown_count, G_swr_shutdown, SWR);
                                }
                            } else {
                                if (Forward_Power <= 3.0f) {
                                    SWR = 1.0f;
                                }
                            }
                            if (Forward_Power > 110.0f && G_swr_shutdown == 0) {
                                if (high_power_count++ >= HIGH_POWER_LIMIT) {
                                    G_high_power_flag = TRUE;
                                    print_time(0);
                                    fprintf(G_fp_logfile, "[%d] Meter_main . high_power_count: %d, G_high_power_flag: %d \n",
                                            line_number++, high_power_count, G_high_power_flag);
                                }
                            } else {
                                if (G_high_power_flag == TRUE) {
                                    G_high_power_flag = FALSE;
                                    high_power_count = 0;
                                    print_time(0);
                                    fprintf(G_fp_logfile, "[%d] Meter_main . G_high_power_flag: %d \n",
                                            line_number++, G_high_power_flag);
                                }
                            }
                        }
                        Send_data();
                        zero_sent = FALSE;
                        zero_counter = 0;
                    }
                }
            }
            pthread_mutex_unlock(&Master_Device_Token_available);
        } else {
            lock_failed++;
            lock_failed_diff = lock_failed - previous_lock_failed;
            if (lock_failed_diff > LOCK_FAILED_LIMIT) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s. G_lock_failed: %I64d, previous_G_lock_failed: %I64d, lock_failed_diff: %I64d\n",
                        line_number++, __func__, lock_failed, previous_lock_failed, lock_failed_diff);
            }
            previous_lock_failed = lock_failed;
            previous_lock_failed_diff = lock_failed_diff;
        }
        if (!G_QRP && G_swr_shutdown == 1) {
            swr_shutdown_count = 0;
            G_swr_shutdown = 0;
        }
        if (previous_tx_mode != G_TX) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Meter_main . previous_tx_mode: %d, G_TX: %d \n",
                    line_number++, previous_tx_mode, G_TX);
            if (G_TX == FALSE && G_swr_shutdown == 0) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Meter_main . G_TX: %d \n",
                        line_number++, G_TX);
                swr_shutdown_count = 0;
                high_power_count = 0;
                G_high_power_flag = FALSE;
                Forward_Power = 0.000f;
                Reverse_Power = 0.000f;
                SWR = 1.0f;
                Send_data();
                zero_sent = TRUE;
                zero_counter = 0;
            }
            previous_tx_mode = G_TX;
        }
        Sleep(Get_random_time());
    }//while (G_all_threads_run && status == 0)
    if (Mensuro_file >= 0) {
        close(Mensuro_file);
    }
    if (status != 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Meter_main. ABNORMAL EXIT \n", line_number++);
        strcpy(message, "Mensuro. ABNORMAL EXIT");
        Gui_Add_Message(message);
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Meter_main. NORMAL EXIT \n", line_number++);
    }
    pthread_exit(0);
    return NULL;
}

//===================================================================

