/*
    i2cset.c - A user-space program to write an I2C register.
    Copyright (C) 2001-2003  Frodo Looijaard <frodol@dds.nl>, and
                             Mark D. Studebaker <mdsxyz123@yahoo.com>
    Copyright (C) 2004-2012  Jean Delvare <jdelvare@suse.de>

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

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include "extern.h"
#include "i2cbusses.h"
#include "util.h"

static int check_funcs(int file, int size, int pec) {
    unsigned long funcs;

    /* check adapter functionality */
    if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "Error: Could not get the adapter "
                "functionality matrix: %s\n", strerror(errno));
        return -1;
    }

    switch (size) {
        case I2C_SMBUS_BYTE:
            if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE)) {
                print_time(0);
                fprintf(G_fp_logfile, MISSING_FUNC_FMT, "SMBus send byte");
                return -1;
            }
            break;

        case I2C_SMBUS_BYTE_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE_DATA)) {
                print_time(0);
                fprintf(G_fp_logfile, MISSING_FUNC_FMT, "SMBus write byte");
                return -1;
            }
            break;

        case I2C_SMBUS_WORD_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_WRITE_WORD_DATA)) {
                print_time(0);
                fprintf(G_fp_logfile, MISSING_FUNC_FMT, "SMBus write word");
                return -1;
            }
            break;

        case I2C_SMBUS_BLOCK_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_WRITE_BLOCK_DATA)) {
                print_time(0);
                fprintf(G_fp_logfile, MISSING_FUNC_FMT, "SMBus block write");
                return -1;
            }
            break;
        case I2C_SMBUS_I2C_BLOCK_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)) {
                print_time(0);
                fprintf(G_fp_logfile, MISSING_FUNC_FMT, "I2C block write");
                return -1;
            }
            break;
    }

    if (pec
            && !(funcs & (I2C_FUNC_SMBUS_PEC | I2C_FUNC_I2C))) {
        print_time(0);
        fprintf(G_fp_logfile, "Warning: Adapter does "
                "not seem to support PEC\n");
    }
    return 0;
}

static int check_funcs_read(int file, int size, int daddress, int pec) {
    unsigned long funcs;

    /* check adapter functionality */
    if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "Error: Could not get the adapter "
                "functionality matrix: %s\n", strerror(errno));
        return -1;
    }

    switch (size) {
        case I2C_SMBUS_BYTE:
            if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE)) {
                print_time(0);
                fprintf(G_fp_logfile, MISSING_FUNC_FMT, "SMBus receive byte");
                return -1;
            }
            if (daddress >= 0
                    && !(funcs & I2C_FUNC_SMBUS_WRITE_BYTE)) {
                print_time(0);
                fprintf(G_fp_logfile, MISSING_FUNC_FMT, "SMBus send byte");
                return -1;
            }
            break;

        case I2C_SMBUS_BYTE_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
                print_time(0);
                fprintf(G_fp_logfile, MISSING_FUNC_FMT, "SMBus read byte");
                return -1;
            }
            break;

        case I2C_SMBUS_WORD_DATA:
            if (!(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA)) {
                print_time(0);
                fprintf(G_fp_logfile, MISSING_FUNC_FMT, "SMBus read word");
                return -1;
            }
            break;
    }

    if (pec
            && !(funcs & (I2C_FUNC_SMBUS_PEC | I2C_FUNC_I2C))) {
        print_time(0);
        fprintf(G_fp_logfile, "Warning: Adapter does "
                "not seem to support PEC\n");
    }
    return 0;
}

static int controller_confirm(const char *filename, int address, int size, int daddress, int value, int vmask, const unsigned char *block, int len, int pec) {
    //int dont = 0;

    return 1;
}

int Write_i2c(int i2cbus, char *filename, int address, int daddress, unsigned char *buffer, int buffer_size, char *caller) {
    //char *end;
    //const char *maskp = NULL;
    int res = 0, size, file;
    int value, vmask = 0;
    //char filename[20];
    int pec = 0;
    //int flags = 0;
    int force = 0;
    int len = 0;
    char type = 'i';
    int status = 0;
    /* handle (optional) flags first */

    switch (type) {
        case 'b': size = I2C_SMBUS_BYTE_DATA;
            break;
        case 'w': size = I2C_SMBUS_WORD_DATA;
            break;
        case 's': size = I2C_SMBUS_BLOCK_DATA;
            break;
        case 'i': size = I2C_SMBUS_I2C_BLOCK_DATA;
            break;
        default:
            //fprintf(stderr, "Error: Invalid mode '%s'!\n", argv[argc - 1]);
            //help();
            break;
    }
    len = 0; /* Must always initialize len since it is passed to confirm() */

    /* read values from command line */
    switch (size) {
        case I2C_SMBUS_BYTE_DATA:
        case I2C_SMBUS_WORD_DATA:
            break;
        case I2C_SMBUS_BLOCK_DATA:
        case I2C_SMBUS_I2C_BLOCK_DATA:
            value = -1;
            break;
        default:
            value = -1;
            break;
    }

    file = open_i2c_dev(i2cbus, filename, sizeof (filename), 0);
    if (file < 0 || check_funcs(file, buffer_size, pec) || set_slave_addr(file, address, force)) {
        status = -1;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Write_i2c . File Open FAILED: %s\n", line_number++, strerror(errno));
    }
    if (status != -1) {
        controller_confirm(filename, address, size, daddress, value, vmask, buffer, buffer_size, pec);
        switch (size) {
            case I2C_SMBUS_BYTE:
                res = i2c_smbus_write_byte(file, daddress);
                break;
            case I2C_SMBUS_WORD_DATA:
                res = i2c_smbus_write_word_data(file, daddress, value);
                break;
            case I2C_SMBUS_BLOCK_DATA:
                res = i2c_smbus_write_block_data(file, daddress, buffer_size, buffer);
                break;
            case I2C_SMBUS_I2C_BLOCK_DATA:
                res = i2c_smbus_write_i2c_block_data(file, daddress, buffer_size, buffer);
                break;
            default: /* I2C_SMBUS_BYTE_DATA */
                res = i2c_smbus_write_byte_data(file, daddress, value);
                break;
        }
        if (res < 0) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Write_i2c . Caller: %s. Write FAILED: %d\n", line_number++, caller, res);
        }
    }
    if (status >= 0) {
        close(file);
    }
    return (res);
}

int Read_i2c(int i2cbus, char *filename, int address, int daddress, unsigned char *buffer, int count) {
    char *end;
    //const char *maskp = NULL;
    int res = 0;
    int size, file;
    int pec = 0;
    int value = 0;
    int force = 0;
    int status = 0;
    char type = 'c';
    int loop_count = 0;
    int read_status = 0;

    switch (type) {
        case 'c': size = I2C_SMBUS_BYTE;
            break;
        default:
            //fprintf(stderr, "Error: Invalid mode '%s'!\n", argv[argc - 1]);
            //help();
            break;
    }
    //len = 0; /* Must always initialize len since it is passed to confirm() */
    file = open_i2c_dev(i2cbus, filename, sizeof (filename), 0);
    if (file < 0 || check_funcs_read(file, size, daddress, pec) || set_slave_addr(file, address, force)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Read_i2c . File Open FAILED: %s\n", line_number++, strerror(errno));
        status = -1;
    }
    //     confirm(filename, address, size, daddress, value, vmask, buffer, buffer_size, pec);
    if (status != -1) {
        switch (size) {
            case I2C_SMBUS_BYTE:
                if (daddress >= 0) {
                    for (loop_count = 0; loop_count < count; loop_count++) {
                        res = i2c_smbus_read_byte_data(file, daddress + loop_count);
                        if (res < 0) {
                            status = res;
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Read_i2c . Read i2c_smbus . FAILED: %d\n",
                                    line_number++, res);
                        } else {
                            buffer[loop_count] = (unsigned char) res;
                        }
                    }
                    default:
                    value = -1;
                    break;
                }
        }
    }
    if (file >= 0) {
        close(file);
    }
    return (status);
}

int Read_i2c_word(int i2cbus, char *filename, int address, int daddress, unsigned char *buffer, int count) {
    char *end;
    //const char *maskp = NULL;
    int res = 0;
    int size, file;
    int pec = 0;
    int value = 0;
    int force = 0;
    int status = 0;
    char type = 'c';
    int loop_count = 0;
    int read_status = 0;

    switch (type) {
        case 'c': size = I2C_SMBUS_BYTE;
            break;
        default:
            //fprintf(stderr, "Error: Invalid mode '%s'!\n", argv[argc - 1]);
            //help();
            break;
    }
    //len = 0; /* Must always initialize len since it is passed to confirm() */
    file = open_i2c_dev(i2cbus, filename, sizeof (filename), 0);
    if (file < 0 || check_funcs_read(file, size, daddress, pec) || set_slave_addr(file, address, force)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Read_i2c . File Open FAILED: %s\n", line_number++, strerror(errno));
        status = -1;
    }
    //     confirm(filename, address, size, daddress, value, vmask, buffer, buffer_size, pec);
    if (status != -1) {
        switch (size) {
            case I2C_SMBUS_BYTE:
                if (daddress >= 0) {
                    for (loop_count = 0; loop_count < count; loop_count++) {
                        res = i2c_smbus_read_byte_data(file, daddress + loop_count);
                        if (res < 0) {
                            status = res;
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Read_i2c . Read i2c_smbus_read_byte_data: %d\n",
                                    line_number++, res);
                        } else {
                            buffer[loop_count] = (unsigned char) res;
                        }
                    }
                    default:
                    value = -1;
                    break;
                }
        }
    }
    if (file >= 0) {
        close(file);
    }
    return (status);
}
