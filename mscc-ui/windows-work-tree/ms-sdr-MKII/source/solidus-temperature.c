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
#include <math.h>
#include "extern.h"
#include "smbus.h"
#include "i2cbusses.h"
#include "util.h"


#include "usbavrcmd.h"

#include "SRDLL.h"
#include "version.h"
#include "band_stack.h"
#include "last_used.h"
#include "iq.h"
//Pthreads

#define AVERAGE_LIMIT 10
#define DECIMAL_POINTS_PRECISION 1000000
#define WIPER_0 0x00
#define WIPER_1 0x01
#define BIAS_TARGET 750
#define WIPER_LIMIT 255

static unsigned long next = 1;

typedef struct {
    int record;
    int band;
    int power_level;
} amplifier_stack;

uint8_t G_check_potentia = FALSE;
float G_Potentia_Temperature = 0.0f;
uint8_t G_Potentia_Temparature_Average = 0;
uint8_t average_temperature_array[AVERAGE_LIMIT];

int32_t G_Solidus_Temperature = 0;
unsigned char G_temperature_buffer[256];

enum parse_state {
    PARSE_GET_DESC,
    PARSE_GET_DATA,
};

#define PRINT_STDERR (1 << 0)
#define PRINT_READ_BUF (1 << 1)
#define PRINT_WRITE_BUF (1 << 2)
#define PRINT_HEADER (1 << 3)

static int t_check_funcs_direct(int file) {
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

static void t_print_msgs(struct i2c_msg *msgs, __u32 nmsgs, unsigned flags) {
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
                G_temperature_buffer[j] = msgs[i].buf[j];
            }
            /* Print final byte with newline */
            //fprintf(output, "0x%02x\n", msgs[i].buf[j]);
            G_temperature_buffer[j] = msgs[i].buf[j];
        } else if (flags & PRINT_HEADER) {
            //fprintf(output, "\n");
        }

    }
}

static int t_confirm(const char *filename, struct i2c_msg *msgs, __u32 nmsgs) {
    fprintf(G_fp_logfile, "WARNING! This program can confuse your I2C bus, cause data loss and worse!\n");
    fprintf(G_fp_logfile, "I will send the following messages to device file %s:\n", filename);
    t_print_msgs(msgs, nmsgs, PRINT_STDERR | PRINT_HEADER | PRINT_WRITE_BUF);

    fprintf(G_fp_logfile, "Continue? [y/N] ");
    fflush(G_fp_logfile);
    if (!user_ack(0)) {
        fprintf(G_fp_logfile, "Aborting on user request.\n");
        return 0;
    }

    return 1;
}

/*int t_err_out_with_arg()
{
        int status = -1;

        print_time(0);
        fprintf(G_fp_logfile, "[%d] t_err_out_with_arg . Input argument error\n", line_number++);
        return status;
}*/

int t_err_out(char *message) {
    int status = -1;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Solidus Temperature . t_err_out . ERROR %s\n", line_number++, message);
    return status;
}

int t_Read_i2c_direct(int argc, char *argv[]) {
    char filename[20];
    int i2cbus, address = -1, file, arg_idx = 1, nmsgs = 0, nmsgs_sent, i;
    int force = 0, yes = 0, version = 0, verbose = 0, all_addrs = 0;
    struct i2c_msg msgs[I2C_RDRW_IOCTL_MAX_MSGS];
    enum parse_state state = PARSE_GET_DESC;
    unsigned buf_idx = 0;
    int status = 0;
    byte count = 0;

    //print_time(0);
    // fprintf(G_fp_logfile, "[%d] t_Read_i2c_direct . Called with: ", line_number++);
    // for (count = 0; count < argc; count++) {
    //       fprintf(G_fp_logfile, "%s,",
    //               argv[count]);
    //}
    //fprintf(G_fp_logfile, "\n ");

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
                fprintf(G_fp_logfile, "t_Read_i2c_direct . Error: Unsupported option \"%s\"!\n",
                        argv[arg_idx]);
                //help();
                status = t_err_out("Error: Unsupported option");
        }
        arg_idx++;
    }

    if (arg_idx == argc) {
        //help();
        status = t_err_out("arg_idx == argc");
    }

    i2cbus = lookup_i2c_bus(argv[arg_idx++]);
    if (i2cbus < 0) {
        status = t_err_out("i2cbuss < 0");
    } else {

        file = open_i2c_dev(i2cbus, filename, sizeof (filename), 0);
        if (file < 0 || t_check_funcs_direct(file))
            status = t_err_out("file open or check_funcs_direct");
    }
    if (status >= 0) {
        while (arg_idx < argc) {
            char *arg_ptr = argv[arg_idx];
            unsigned long len, raw_data;
            __u16 flags;
            __u8 data, *buf;
            char *end;

            if (nmsgs > I2C_RDRW_IOCTL_MAX_MSGS) {
                fprintf(G_fp_logfile, "t_Read_i2c_direct . Error: Too many messages (max: %d)\n",
                        I2C_RDRW_IOCTL_MAX_MSGS);
                status = t_err_out("I2C_RDRW_IOCTL_MAX_MSGS");
            }

            switch (state) {
                case PARSE_GET_DESC:
                    flags = 0;

                    switch (*arg_ptr++) {
                        case 'r': flags |= I2C_M_RD;
                            break;
                        case 'w': break;
                        default:
                            fprintf(G_fp_logfile, "t_Read_i2c_direct . Error: Invalid direction\n");
                            status = t_err_out("Invalid direction");
                    }

                    len = strtoul(arg_ptr, &end, 0);
                    if (len > 0xffff || arg_ptr == end) {
                        fprintf(G_fp_logfile, "t_Read_i2c_direct . Error: Length invalid\n");
                        status = t_err_out("Length invalid");
                    }

                    arg_ptr = end;
                    if (*arg_ptr) {
                        if (*arg_ptr++ != '@') {
                            fprintf(G_fp_logfile, "t_Read_i2c_direct . Error: Unknown separator after length\n");
                            status = t_err_out("Unknown separator after length");
                        }

                        /* We skip 10-bit support for now. If we want it,
                         * it should be marked with a 't' flag before
                         * the address here.
                         */

                        address = parse_i2c_address(arg_ptr, all_addrs);
                        if (address < 0)
                            status = t_err_out("parse_i2c_address. address < 0");

                        /* Ensure address is not busy */
                        if (!force && set_slave_addr(file, address, 0))
                            status = t_err_out("address busy");
                    } else {
                        /* Reuse last address if possible */
                        if (address < 0) {
                            fprintf(G_fp_logfile, "t_Read_i2c_direct . Error: No address given\n");
                            status = t_err_out("No address given");
                        }
                    }

                    msgs[nmsgs].addr = address;
                    msgs[nmsgs].flags = flags;
                    msgs[nmsgs].len = len;

                    if (len) {
                        buf = malloc(len);
                        if (!buf) {
                            fprintf(G_fp_logfile, "t_Read_i2c_direct . Error: No memory for buffer\n");
                            status = t_err_out("No memory for buffer");
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
                        fprintf(G_fp_logfile, "t_Read_i2c_direct . Error: Invalid data byte\n");
                        status = t_err_out("Invalid data byte");
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
                                fprintf(G_fp_logfile, "t_Read_i2c_direct . Error: Invalid data byte suffix\n");
                                status = t_err_out("Invalid data byte suffix");
                        }
                    }

                    if (buf_idx == len) {
                        nmsgs++;
                        state = PARSE_GET_DESC;
                    }

                    break;

                default:
                    /* Should never happen */
                    fprintf(G_fp_logfile, "Internal Error: Unknown state in state machine!\n");
                    status = t_err_out("Unknown state in state machine!");
            }

            arg_idx++;
        }

        if (state != PARSE_GET_DESC || nmsgs == 0) {
            fprintf(G_fp_logfile, "t_Read_i2c_direct . Error: Incomplete message\n");
            status = t_err_out("Incomplete message");
        }

        if (yes || t_confirm(filename, msgs, nmsgs)) {
            struct i2c_rdwr_ioctl_data rdwr;

            rdwr.msgs = msgs;
            rdwr.nmsgs = nmsgs;
            if (status >= 0) {
                nmsgs_sent = ioctl(file, I2C_RDWR, &rdwr);
                if (nmsgs_sent < 0) {
                    fprintf(G_fp_logfile, "t_Read_i2c_direct . Error: Sending messages failed: %s\n", strerror(errno));
                    status = t_err_out("Sending messages failed");
                } else if (nmsgs_sent < nmsgs) {
                    fprintf(G_fp_logfile, "t_Read_i2c_direct . Warning: only %d/%d messages were sent\n", nmsgs_sent, nmsgs);
                }

                t_print_msgs(msgs, nmsgs_sent, PRINT_READ_BUF | (verbose ? PRINT_HEADER | PRINT_WRITE_BUF : 0));
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

int Read_solidus_temperature() {
    int status = 0;
    char filename[PATH_MAX] = {0};
    uint8_t upper_byte;
    uint8_t lower_byte;
    int32_t temperature = 0;
    unsigned char buffer[2];
    //INT32 potentia_temp = 0;
    float temp_temp = 0.0f;
    char *argv[10];
    char dummy[] = "Read_solidus_temperature";
    char yes[] = "-y";
    char first[] = "w1@0x18";
    //char second[] = "0x05";
    char third[] = "r2";
    char i2cbuss[] = "6";
    unsigned char control_string[20] = {0};
    char control = 0x05;

    //control_string[0] = control;
    sprintf(control_string, "0x%0X", control);
    argv[0] = dummy;
    argv[1] = yes;
    argv[2] = i2cbuss;
    argv[3] = first;
    argv[4] = control_string;
    argv[5] = third;

    status = t_Read_i2c_direct(6, argv);
    buffer[0 ] = G_temperature_buffer[0];
    buffer[1] = G_temperature_buffer[1];
    upper_byte = buffer[0];
    lower_byte = buffer[1];
    upper_byte = upper_byte & 0x0F;
    temperature = upper_byte;
    temperature = temperature << 8;
    temperature = temperature + lower_byte;
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Read_solidus_temperature: %d\n", line_number++, temperature);

    upper_byte = temperature >> 8;
    lower_byte = (uint8_t) temperature;
    temp_temp = (float) lower_byte / 16.0f;
    temp_temp = ((float) upper_byte * 16) + temp_temp;
    G_Solidus_Temperature = temp_temp;
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Read_solidus_temperature: %d\n", line_number++, G_Solidus_Temperature);
    return status;
}

uint8_t Average_temperature(uint8_t temperature) {
    uint8_t average_temp = 0;
    uint8_t average_count = 0;
    uint16_t average = 0;
    int i = 0;

    average_temperature_array[0] = temperature;
    for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
        average = average + average_temperature_array[average_count];
    }
    average_temp = average / average_count;
    for (i = (AVERAGE_LIMIT - 1); i > 0; i--) {
        average_temperature_array[i] = average_temperature_array[(i - 1)];
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Check_Potentia . Average_temperature . temperature: %d, average: %d, average_temp: %d, count:%d\n", 
    //	line_number++, temperature,average,average_temp,average_count);
    return average_temp;
}

int myrand_potentia(void) {
    next = next * 1103515245 + 12345;
    return ((unsigned) (next / 65536) % 32768);
}

void *Check_Potentia_Temperature(void *t) {
    int sleep_time = 3000;
    INT32 potentia_temp = 0;
    float temp_temp = 0.0f;
    int random = 0;
    uint8_t average_temp = 0;
    int first_run_count = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_Solidus_Temperature . Started. Sleeping for 7 Seconds \n", line_number++);
    Sleep(7000);
    //G_subsys_initialized = G_subsys_initialized | READ_SOLIDUS_TEMPERATURE_INITIALIZED;
    while (G_all_threads_run) {
        if (G_TEMP_allow == TRUE) {
            Read_solidus_temperature();
            if (first_run_count < AVERAGE_LIMIT) {
                sleep_time = 450;
                first_run_count++;
            } else {
                sleep_time = 3000;
            }
            if (G_Transceiver_Busy == TRUE && G_Proficio_temperature == TRUE) {
                random = myrand_potentia();
                if (random > 3000) {
                    random = 1500;
                }
                Sleep(random);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_Solidus_Temperature . Sleeping for: %d ms \n", line_number++, random);
            } else {
                potentia_temp = G_Solidus_Temperature;
                G_check_potentia = TRUE;
                if (potentia_temp >= 0) {
                    G_Potentia_Temperature = G_Solidus_Temperature;
                    average_temp = Average_temperature((uint8_t) G_Solidus_Temperature);
                    G_Potentia_Temparature_Average = average_temp;
                    //print_time(0);
                    //fprintf(G_fp_logfile, "[%d] Check_Solidus_Temperature . Temperature: %d\n", line_number++, average_temp);
                }
                G_check_potentia = FALSE;
            }
            if (first_run_count >= AVERAGE_LIMIT) {
                //SDRcore_trans_send_param(CMD_GET_POTENTIA_TEMPERATURE, potentia_temp);
                Gui_send_param(CMD_GET_POTENTIA_TEMPERATURE, average_temp);
            }
            G_TEMP_allow = FALSE;
        }
        Sleep(sleep_time);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_Solidus_Temperature . NORMAL EXIT\n", line_number++);
    pthread_exit(0);
    return NULL;
}