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

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "i2cbusses.h"
#include "util.h"
#include "extern.h"
//#include "../version.h"

enum parse_state {
        PARSE_GET_DESC,
        PARSE_GET_DATA,
};

#define PRINT_STDERR (1 << 0)
#define PRINT_READ_BUF (1 << 1)
#define PRINT_WRITE_BUF (1 << 2)
#define PRINT_HEADER (1 << 3)

extern char G_MCP_buffer[256];

static int check_funcs_direct(int file)
{
        unsigned long funcs;

        /* check adapter functionality */
        if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
                fprintf(G_fp_logfile, "Error: Could not get the adapter "
                        "functionality matrix: %s\n", strerror(errno));
                return -1;
        }

        if (!(funcs & I2C_FUNC_I2C)) {
                fprintf(G_fp_logfile, MISSING_FUNC_FMT, "I2C transfers");
                return -1;
        }

        return 0;
}

static void print_msgs(struct i2c_msg *msgs, __u32 nmsgs, unsigned flags)
{
        FILE *output = G_fp_logfile;
        unsigned i;
        __u16 j;

        for (i = 0; i < nmsgs; i++) {
                int read = msgs[i].flags & I2C_M_RD;
                int print_buf = (read && (flags & PRINT_READ_BUF)) ||
                        (!read && (flags & PRINT_WRITE_BUF));

                if (flags & PRINT_HEADER)
                        fprintf(output, "msg %u: addr 0x%02x, %s, len %u",
                        i, msgs[i].addr, read ? "read" : "write", msgs[i].len);

                if (msgs[i].len && print_buf) {
                        if (flags & PRINT_HEADER)
                                fprintf(output, ", buf ");
                        for (j = 0; j < msgs[i].len - 1; j++) {
                                //fprintf(output, "0x%02x ", msgs[i].buf[j]);
                                G_MCP_buffer[j] = msgs[i].buf[j];
                        }
                        /* Print final byte with newline */
                        //fprintf(output, "0x%02x\n", msgs[i].buf[j]);
                        G_MCP_buffer[j] = msgs[i].buf[j];
                } else if (flags & PRINT_HEADER) {
                        //fprintf(output, "\n");
                }

        }
}

static int Meter_confirm(const char *filename, struct i2c_msg *msgs, __u32 nmsgs)
{
        fprintf(G_fp_logfile, "WARNING! This program can confuse your I2C bus, cause data loss and worse!\n");
        fprintf(G_fp_logfile, "I will send the following messages to device file %s:\n", filename);
        print_msgs(msgs, nmsgs, PRINT_STDERR | PRINT_HEADER | PRINT_WRITE_BUF);

        fprintf(G_fp_logfile, "Continue? [y/N] ");
        fflush(G_fp_logfile);
        if (!user_ack(0)) {
                fprintf(G_fp_logfile, "Aborting on user request.\n");
                return 0;
        }

        return 1;
}

/*int err_out_with_arg()
{
        int status = -1;

        print_time(0);
        fprintf(G_fp_logfile, "[%d] Input argument error\n", line_number++);
        return status;
}*/

int err_out(char *message)
{
        int status = -1;

        print_time(0);
        fprintf(G_fp_logfile, "[%d] Meter -> err_out.  ERROR: %s\n", line_number++,message);
        return status;
}

int Meter_Read_i2c_direct(int argc, char *argv[])
{
        char filename[20];
        int i2cbus, address = -1, file, arg_idx = 1, nmsgs = 0, nmsgs_sent, i;
        int force = 0, yes = 0, version = 0, verbose = 0, all_addrs = 0;
        struct i2c_msg msgs[I2C_RDRW_IOCTL_MAX_MSGS];
        enum parse_state state = PARSE_GET_DESC;
        unsigned buf_idx = 0;
        int status = 0;
        byte count = 0;
        
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Meter_Read_i2c_direct -> Called with: ",line_number++);
        for (count = 0; count < argc; count++) {
                fprintf(G_fp_logfile, "%s,",
                        argv[count]);
        }
        fprintf(G_fp_logfile, "\n ");

        for (i = 0; i < I2C_RDRW_IOCTL_MAX_MSGS; i++)
                msgs[i].buf = NULL;

        /* handle (optional) arg_idx first */
        while (arg_idx < argc && argv[arg_idx][0] == '-') {
                switch (argv[arg_idx][1]) {
                case 'V': version = 1;
                        break;
                case 'v': verbose = 1;
                        break;
                case 'f': force = 1;
                        break;
                case 'y': yes = 1;
                        break;
                case 'a': all_addrs = 1;
                        break;
                default:
                       // fprintf(G_fp_logfile, "Error: Unsupported option \"%s\"!\n",
                        //        argv[arg_idx]);
                        //help();
                        status = err_out("Unsupported option ");
                }
                arg_idx++;
        }

        if (arg_idx == argc) {
                //help();
                status = err_out("arg_idx == argc");
        }

        i2cbus = lookup_i2c_bus(argv[arg_idx++]);
        if (i2cbus < 0) {
                status = err_out("i2cbus < 0");
        } else {

                file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0);
                if (file < 0 || check_funcs_direct(file))
                        status = err_out("file open or check_funcs_direct");
        }
        if (status >= 0) {
                while (arg_idx < argc) {
                        char *arg_ptr = argv[arg_idx];
                        unsigned long len, raw_data;
                        __u16 flags;
                        __u8 data, *buf;
                        char *end;

                        if (nmsgs > I2C_RDRW_IOCTL_MAX_MSGS) {
                                //fprintf(G_fp_logfile, "Error: Too many messages (max: %d)\n",
                                //        I2C_RDRW_IOCTL_MAX_MSGS);
                                status = err_out("Too many messages");
                        }

                        switch (state) {
                        case PARSE_GET_DESC:
                                flags = 0;

                                switch (*arg_ptr++) {
                                case 'r': flags |= I2C_M_RD;
                                        break;
                                case 'w': break;
                                default:
                                        //fprintf(G_fp_logfile, "Error: Invalid direction\n");
                                        status = err_out("Invalid direction");
                                }

                                len = strtoul(arg_ptr, &end, 0);
                                if (len > 0xffff || arg_ptr == end) {
                                        //fprintf(G_fp_logfile, "Error: Length invalid\n");
                                        status = err_out("Length invalid");
                                }

                                arg_ptr = end;
                                if (*arg_ptr) {
                                        if (*arg_ptr++ != '@') {
                                                //fprintf(G_fp_logfile, "Error: Unknown separator after length\n");
                                                status = err_out("Unknown separator after length");
                                        }

                                        /* We skip 10-bit support for now. If we want it,
                                         * it should be marked with a 't' flag before
                                         * the address here.
                                         */

                                        address = parse_i2c_address(arg_ptr, all_addrs);
                                        if (address < 0)
                                                status = err_out("parse_i2c_address: address < 0");

                                        /* Ensure address is not busy */
                                        if (!force && set_slave_addr(file, address, 0))
                                                status = err_out("device busy");
                                } else {
                                        /* Reuse last address if possible */
                                        if (address < 0) {
                                                //fprintf(G_fp_logfile, "Error: No address given\n");
                                                status = err_out("No address given");
                                        }
                                }

                                msgs[nmsgs].addr = address;
                                msgs[nmsgs].flags = flags;
                                msgs[nmsgs].len = len;

                                if (len) {
                                        buf = malloc(len);
                                        if (!buf) {
                                                //fprintf(G_fp_logfile, "Error: No memory for buffer\n");
                                                status = err_out("No memory for buffer");
                                        }
                                        memset(buf, 0, len);
                                        msgs[nmsgs].buf = buf;
                                }

                                if (flags & I2C_M_RD || len == 0) {
                                        nmsgs++;
                                } else {
                                        buf_idx = 0;
                                        state = PARSE_GET_DATA;
                                }

                                break;

                        case PARSE_GET_DATA:
                                raw_data = strtoul(arg_ptr, &end, 0);
                                if (raw_data > 0xff || arg_ptr == end) {
                                        //fprintf(G_fp_logfile, "Error: Invalid data byte\n");
                                        status = err_out("Invalid data byte");
                                }
                                data = raw_data;
                                len = msgs[nmsgs].len;

                                while (buf_idx < len) {
                                        msgs[nmsgs].buf[buf_idx++] = data;

                                        if (!*end)
                                                break;

                                        switch (*end) {
                                                /* Pseudo randomness (8 bit AXR with a=13 and b=27) */
                                        case 'p':
                                                data = (data ^ 27) + 13;
                                                data = (data << 1) | (data >> 7);
                                                break;
                                        case '+': data++;
                                                break;
                                        case '-': data--;
                                                break;
                                        case '=': break;
                                        default:
                                                //fprintf(G_fp_logfile, "Error: Invalid data byte suffix\n");
                                                status = err_out("Invalid data byte suffix");
                                        }
                                }

                                if (buf_idx == len) {
                                        nmsgs++;
                                        state = PARSE_GET_DESC;
                                }

                                break;

                        default:
                                /* Should never happen */
                                //fprintf(G_fp_logfile, "Internal Error: Unknown state in state machine!\n");
                                status = err_out("Unknown state in state machine!");
                        }

                        arg_idx++;
                }

                if (state != PARSE_GET_DESC || nmsgs == 0) {
                        //fprintf(G_fp_logfile, "Error: Incomplete message\n");
                        status = err_out("Incomplete message");
                }

                if (yes || Meter_confirm(filename, msgs, nmsgs)) {
                        struct i2c_rdwr_ioctl_data rdwr;

                        rdwr.msgs = msgs;
                        rdwr.nmsgs = nmsgs;
                        if (status >= 0) {
                                nmsgs_sent = ioctl(file, I2C_RDWR, &rdwr);
                                if (nmsgs_sent < 0) {
                                        //fprintf(G_fp_logfile, "Error: Sending messages failed: %s\n", strerror(errno));
                                        status = err_out("Sending messages failed");
                                } else if (nmsgs_sent < nmsgs) {
                                        print_time(0);
                                        fprintf(G_fp_logfile, "Warning: only %d/%d messages were sent\n", nmsgs_sent, nmsgs);
                                        status = err_out("Number of messages send incorrect");
                                }

                                print_msgs(msgs, nmsgs_sent, PRINT_READ_BUF | (verbose ? PRINT_HEADER | PRINT_WRITE_BUF : 0));
                        }
                }

                if (file >= 0) {
                        close(file);
                        for (i = 0; i < nmsgs; i++)
                                free(msgs[i].buf);
                }
        }
        return status;
}
