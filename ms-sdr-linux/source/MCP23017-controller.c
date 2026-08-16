/*
    i2cget.c - A user-space program to read an I2C register.
    Copyright (C) 2005-2012  Jean Delvare <jdelvare@suse.de>

    Based on i2cset.c:
    Copyright (C) 2001-2003  Frodo Looijaard <frodol@dds.nl>, and
                             Mark D. Studebaker <mdsxyz123@yahoo.com>
    Copyright (C) 2004-2005  Jean Delvare

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA 02110-1301 USA.
 */

#include "extern.h"
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/errno.h>

#include "smbus.h"
#include "i2cbusses.h"
#include "util.h"



//char buffer[20] = {0};
unsigned char G_controller_buffer[256] = {0};

int MCP23017_Controller_main(unsigned char GPIO_A, unsigned char GPIO_B) {
    char filename[PATH_MAX] = {0};
    int status = 0;

    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] MCP23017_Controller_main . GPIOA: 0x%02X, GPIOB: 0x%02X,\n", line_number++, GPIO_A, GPIO_B);
    status = Write_i2c(I2C_BUSS, filename, MCP23017_ADDRESS, 0x12, &GPIO_A, 1,"MCP23017_Controller_main");
    if (status >= 0) {
        status = Write_i2c(I2C_BUSS, filename, MCP23017_ADDRESS, 0x13, &GPIO_B, 1,"MCP23017_Controller_main");
    }
    if (status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] MCP23017_Controller_main . Write_i2c FAILED: %d\n", line_number++, status);
    }
    return status;
}

int MCP23008_Controller_main(unsigned char GPIO_A) {
    char filename[PATH_MAX] = {0};
    int status = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] MCP23008_Controller_main . GPIOA: 0x%02X,\n", 
            line_number++, GPIO_A);
    status = Write_i2c(I2C_BUSS, filename, MCP23017_ADDRESS, 0x09, &GPIO_A, 1,"MCP23008_Controller_main");
    if (status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] MCP23008_Controller_main . Write_i2c FAILED: %d\n", line_number++, status);
    }
    return status;
}

int MCP_Init() {
    char filename[PATH_MAX] = {0};
    int status = 0;
    unsigned char buffer;

    switch (G_Transceiver_type) {
        case SOLIDUS:
            buffer = 0x1F;
            status = Write_i2c(I2C_BUSS, filename, MCP23017_ADDRESS, 0x05, &buffer, 1,"MCP_Init");
            if (status >= 0) {
                buffer = 0x00;
                status = Write_i2c(I2C_BUSS, filename, MCP23017_ADDRESS, 0x00, &buffer, 1,"MCP_Init");
                if (status >= 0) {
                    buffer = 0x00;
                    status = Write_i2c(I2C_BUSS, filename, MCP23017_ADDRESS, 0x01, &buffer, 1,"MCP_Init");
                }
            }
            break;
        case FORTIS:
            buffer = 0x00;
            status = Write_i2c(I2C_BUSS, filename, MCP23017_ADDRESS, 0x00, &buffer, 1,"MCP_Init");
            break;
    }
    if (status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] MCP_Init . Write_i2c FAILED: %d\n", line_number++, status);
    }
    return status;
}
