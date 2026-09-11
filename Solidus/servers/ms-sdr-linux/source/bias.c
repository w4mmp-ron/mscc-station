#include <math.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "usbavrcmd.h"
#include "extern.h"
#include "version.h"
#include "band_stack.h"
#include "last_used.h"
#include "iq.h"
#include "smbus.h"
#include "i2cbusses.h"
#include "util.h"

#define PRINT_STDERR (1 << 0)
#define PRINT_READ_BUF (1 << 1)
#define PRINT_WRITE_BUF (1 << 2)
#define PRINT_HEADER (1 << 3)

//#define AVERAGE_LIMIT 10
#define BIAS_AVERAGE_LIMIT 40
#define BIAS_RESET_COUNT 2
#define BIAS_CENTER_TEMPERATURE 40
#define BIAS_SCALING 30.75f 

char G_Bias_buffer[256];
uint16_t G_Bias_value;
float average_bias_array[BIAS_AVERAGE_LIMIT];
float E_bias = 0.0f;
uint8_t G_calibrate_tune_power = 10;
unsigned long next = 1;
byte INA219_buffer[256] = {0};

enum parse_state {
    PARSE_GET_DESC,
    PARSE_GET_DATA,
};

static int INA219_check_funcs_direct(int file) {
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

static void INA219_print_msgs(struct i2c_msg *msgs, __u32 nmsgs, unsigned flags) {
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
                INA219_buffer[j] = msgs[i].buf[j];
            }
            /* Print final byte with newline */
            //fprintf(output, "0x%02x\n", msgs[i].buf[j]);
            INA219_buffer[j] = msgs[i].buf[j];
        } else if (flags & PRINT_HEADER) {
            //fprintf(output, "\n");
        }

    }
}

static int INA219_confirm(const char *filename, struct i2c_msg *msgs, __u32 nmsgs) {
    fprintf(G_fp_logfile, "WARNING! This program can confuse your I2C bus, cause data loss and worse!\n");
    fprintf(G_fp_logfile, "I will send the following messages to device file %s:\n", filename);
    INA219_print_msgs(msgs, nmsgs, PRINT_STDERR | PRINT_HEADER | PRINT_WRITE_BUF);

    fprintf(G_fp_logfile, "Continue? [y/N] ");
    fflush(G_fp_logfile);
    if (!user_ack(0)) {
        fprintf(G_fp_logfile, "Aborting on user request.\n");
        return 0;
    }
    return 1;
}

int INA219_err_out(char *message) {
    int status = -1;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Meter . err_out.  ERROR: %s\n", line_number++, message);
    return status;
}

int INA219_Read_i2c_direct(int argc, char *argv[]) {
    char filename[20];
    int i2cbus, address = -1, file, arg_idx = 1, nmsgs = 0, nmsgs_sent, i;
    int force = 0, yes = 0, version = 0, verbose = 0, all_addrs = 0;
    struct i2c_msg msgs[I2C_RDRW_IOCTL_MAX_MSGS];
    enum parse_state state = PARSE_GET_DESC;
    unsigned buf_idx = 0;
    int status = 0;
    byte count = 0;

    /*print_time(0);
    fprintf(G_fp_logfile, "[%d] Meter_Read_i2c_direct . Called with: ", line_number++);
    for (count = 0; count < argc; count++) {
            fprintf(G_fp_logfile, "%s,",
                    argv[count]);
    }
    fprintf(G_fp_logfile, "\n ");
     */

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
                status = INA219_err_out("Unsupported option ");
        }
        arg_idx++;
    }

    if (arg_idx == argc) {
        //help();
        status = INA219_err_out("arg_idx == argc");
    }

    i2cbus = lookup_i2c_bus(argv[arg_idx++]);
    if (i2cbus < 0) {
        status = INA219_err_out("i2cbus < 0");
    } else {

        file = open_i2c_dev(i2cbus, filename, sizeof (filename), 0);
        if (file < 0 || INA219_check_funcs_direct(file))
            status = INA219_err_out("file open or check_funcs_direct");
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
                status = INA219_err_out("Too many messages");
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
                            status = INA219_err_out("Invalid direction");
                    }

                    len = strtoul(arg_ptr, &end, 0);
                    if (len > 0xffff || arg_ptr == end) {
                        //fprintf(G_fp_logfile, "Error: Length invalid\n");
                        status = INA219_err_out("Length invalid");
                    }

                    arg_ptr = end;
                    if (*arg_ptr) {
                        if (*arg_ptr++ != '@') {
                            //fprintf(G_fp_logfile, "Error: Unknown separator after length\n");
                            status = INA219_err_out("Unknown separator after length");
                        }

                        /* We skip 10-bit support for now. If we want it,
                         * it should be marked with a 't' flag before
                         * the address here.
                         */

                        address = parse_i2c_address(arg_ptr, all_addrs);
                        if (address < 0)
                            status = INA219_err_out("parse_i2c_address: address < 0");

                        /* Ensure address is not busy */
                        if (!force && set_slave_addr(file, address, 0))
                            status = INA219_err_out("device busy");
                    } else {
                        /* Reuse last address if possible */
                        if (address < 0) {
                            //fprintf(G_fp_logfile, "Error: No address given\n");
                            status = INA219_err_out("No address given");
                        }
                    }

                    msgs[nmsgs].addr = address;
                    msgs[nmsgs].flags = flags;
                    msgs[nmsgs].len = len;

                    if (len) {
                        buf = malloc(len);
                        if (!buf) {
                            //fprintf(G_fp_logfile, "Error: No memory for buffer\n");
                            status = INA219_err_out("No memory for buffer");
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
                        status = INA219_err_out("Invalid data byte");
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
                                status = INA219_err_out("Invalid data byte suffix");
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
                    status = INA219_err_out("Unknown state in state machine!");
            }

            arg_idx++;
        }

        if (state != PARSE_GET_DESC || nmsgs == 0) {
            //fprintf(G_fp_logfile, "Error: Incomplete message\n");
            status = INA219_err_out("Incomplete message");
        }

        if (yes || INA219_confirm(filename, msgs, nmsgs)) {
            struct i2c_rdwr_ioctl_data rdwr;

            rdwr.msgs = msgs;
            rdwr.nmsgs = nmsgs;
            if (status >= 0) {
                nmsgs_sent = ioctl(file, I2C_RDWR, &rdwr);
                if (nmsgs_sent < 0) {
                    //fprintf(G_fp_logfile, "Error: Sending messages failed: %s\n", strerror(errno));
                    status = INA219_err_out("Sending messages failed");
                } else if (nmsgs_sent < nmsgs) {
                    print_time(0);
                    fprintf(G_fp_logfile, "Warning: only %d/%d messages were sent\n", nmsgs_sent, nmsgs);
                    status = INA219_err_out("Number of messages send incorrect");
                }

                INA219_print_msgs(msgs, nmsgs_sent, PRINT_READ_BUF | (verbose ? PRINT_HEADER | PRINT_WRITE_BUF : 0));
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

int Read_INA219(byte *read) {
    int status = 0;
    char filename[PATH_MAX] = {0};

    char *argv[10];
    char dummy[] = "Read_INA219";
    char yes[] = "-y";
    char first[] = "w1@0x40";
    //char second[] = "0x9C";
    char third[] = "r2";
    char i2cbuss[] = "6";
    unsigned char control_string[20] = {0};
    int count = 0;

    char control = 0x01;
    sprintf(control_string, "0x%0X", control);
    argv[0] = dummy;
    argv[1] = yes;
    argv[2] = i2cbuss;
    argv[3] = first;
    argv[4] = control_string;
    argv[5] = third;

    status = INA219_Read_i2c_direct(6, argv);


    return status;
}

int INA219_Init() {
    char filename[PATH_MAX] = {0};
    int status = 0;
    unsigned char buffer[2];
   
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s -. Called\n", line_number++, __func__);
    buffer[0] = 0x1F;
    buffer[1] = 0xC5;
    status = Write_i2c(I2C_BUSS, filename, CURRENT_SENSOR_ADDRESS, 0x00, buffer, 2,"INA219_Init");
    if (status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s -. Write_i2c FAILED: %s\n", line_number++, __func__, strerror(errno));
    }
    return status;
}

int Bias_Read() {
    char filename[PATH_MAX] = {0};
    int status = 0;
    unsigned char buffer[2];
    uint8_t sign_bit = 0;
    uint8_t upper_byte;
    uint8_t lower_byte;
    uint16_t power = 0;
    uint16_t power_temp = 0;
    int16_t negative = 0;
    uint16_t n_power = 0;
    uint16_t n_power_temp = 0;
    uint8_t n_upper_byte;
    uint8_t n_lower_byte;
    float n_result = 0.0f;

    status = Read_INA219(buffer);
    if (status >= 0) {
        upper_byte = INA219_buffer[0];
        lower_byte = INA219_buffer[1];
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] %s . upper_byte: %X, lower_byte: %X\n", line_number++, __func__, upper_byte, lower_byte);
        if (upper_byte == 0x00 && lower_byte == 0x6B) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s . value NOT valid\n", line_number++, __func__);
        } else {
            n_upper_byte = INA219_buffer[0];
            n_lower_byte = INA219_buffer[1];
            sign_bit = n_upper_byte & 0x80;
            if (sign_bit == 0x80) {
                n_power_temp = n_upper_byte;
                n_power = n_power_temp << 8;
                n_power = n_power + n_lower_byte;
                //print_time(0);
                //fprintf(G_fp_logfile, "[%d] %s . sign_bit: %X , n_power: %02X \n", line_number++, __func__, sign_bit, n_power);
                negative = ~n_power;
                negative = negative + 1;
                n_result = ((float) negative) * -1.0f;
                //print_time(0);
                //fprintf(G_fp_logfile, "[%d] %s . sign_bit: %X , negative: %02X, negative: %d, n_result: %f \n",line_number++,
                //        __func__, sign_bit, negative, negative, n_result);
                E_bias = n_result;
            } else {
                power = upper_byte;
                power = power << 8;
                power = power + lower_byte;
                E_bias = (float) power;
                //print_time(0);
                //fprintf(G_fp_logfile, "[%d] %s . upper_byte: %X, lower_byte: %X, power_temp: %04X \n",
                //        line_number++, __func__, upper_byte, lower_byte, power_temp);
            }
            if (E_bias <= 0.0f) {
                E_bias = 0.0f;
            }
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] %s . E_bias: %f\n",
            //       line_number++, __func__, E_bias);
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s . INA219_Read FAILED: %d\n", line_number++, __func__, status);
    }
    return status;
}

float Average_bias(float bias, uint8_t reset) {
    float average_bias = 0.0;
    uint8_t average_count = 0;
    float average = 0;
    int i = 0;
    static uint8_t bias_reset = FALSE;
    static int bias_reset_count = BIAS_RESET_COUNT;

    if (reset == TRUE) {
        for (average_count = 0; average_count < BIAS_AVERAGE_LIMIT; average_count++) {
            average_bias_array[average_count] = 0.0f;
        }
        average_bias = 0.0f;
        bias_reset = TRUE;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_Potentia_Bias . Average_bias . bias array set to zero (0)\n", line_number++);
    } else {
        if (bias_reset == TRUE) {
            if (--bias_reset_count == 0) {
                for (average_count = 0; average_count < BIAS_AVERAGE_LIMIT; average_count++) {
                    average_bias_array[average_count] = bias;
                }
                for (average_count = 0; average_count < BIAS_AVERAGE_LIMIT; average_count++) {
                    average = average + average_bias_array[average_count];
                }
                average_bias = average / average_count;
                bias_reset_count = BIAS_RESET_COUNT;
                bias_reset = FALSE;
            }
        } else {
            average_bias_array[0] = bias;
            for (average_count = 0; average_count < BIAS_AVERAGE_LIMIT; average_count++) {
                average = average + average_bias_array[average_count];
            }
            average_bias = average / (float) average_count;
            for (i = (BIAS_AVERAGE_LIMIT - 1); i > 0; i--) {
                average_bias_array[i] = average_bias_array[(i - 1)];
            }
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] Check_Potentia_Bias . Average_bias . Called with: Bias: %f, average_bias: %f, temperature: %f\n", 
            //	line_number++,bias, average_bias, G_Potentia_Temperature);
        }
    }
    return average_bias;
}

int Check_Bias(float temperature, uint8_t reset) {
    INT32 iVersion = 0;
    int r;
    float shunt_voltage = 0;
    float shunt_voltage_valid[3] = {0};
    float amplifier_current = 0.0f;
    static float current_average_result = 0.0f;
    static INT32 total_current = 0;
    int read_status = 0;

    Sleep(100);
    read_status = Bias_Read();
    //r = usbControlMsgIN(CMD_GET_POTENTIA_POWER, 0xA55A, 0, (char*) &iVersion, sizeof(iVersion));
    if (read_status >= 0) {
        shunt_voltage = E_bias;
        if (shunt_voltage < 32000.0f) {
            shunt_voltage_valid[0] = shunt_voltage;
            if (((shunt_voltage_valid[0] >= shunt_voltage_valid[1]) && (shunt_voltage_valid[1] >= shunt_voltage_valid[2])) || //Temperature going up
                    ((shunt_voltage_valid[2] >= shunt_voltage_valid[1]) && (shunt_voltage_valid[1] >= shunt_voltage_valid[0]))) { //Temperature going down
                amplifier_current = shunt_voltage / 0.10000f;
                current_average_result = Average_bias(amplifier_current, reset);
                total_current = (int) current_average_result;
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_Potentia_Bias . Check_Bias . Shunt Voltage NOT VALID. Element 0: %f, Element 1: %f, Element 2: %f\n",
                        line_number++, shunt_voltage_valid[0], shunt_voltage_valid[1], shunt_voltage_valid[2]);
            }
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_Potentia_Bias . Check_Bias . INA219_Read FAILED \n");
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] %s . E_bias: %f, shunt_voltage: %f, current_average_result: %f, total_current(int): %d temperature: %f, RESET: %d\n",
    //       line_number++, __func__, E_bias, shunt_voltage, current_average_result, total_current, temperature, reset);
    return total_current;
}

INT32 Normal_Bias_Check(uint8_t tx_mode) {
    static uint8_t bias_reset = FALSE;
    INT32 bias_value;

    if (tx_mode == 1) {
        if (bias_reset == FALSE) {
            bias_value = Check_Bias(G_Potentia_Temperature, TRUE);
            bias_reset = TRUE;
        } else {
            bias_value = Check_Bias(G_Potentia_Temperature, FALSE);
        }
    } else {
        if (bias_reset == TRUE) {
            bias_value = Check_Bias(G_Potentia_Temperature, TRUE);
            bias_reset = FALSE;
        } else {
            bias_value = Check_Bias(G_Potentia_Temperature, FALSE);
        }
    }
    return bias_value;
}

int adjust_bias(int bias) {
    int scaled_bias = 0;
    int temp_i = 0;
    static float bias_temp_low = 0.0f;
    static float bias_temp_high = 0.0f;

    //temp_i = G_Potentia_Temparature_Average;
    if (G_Potentia_Temparature_Average > BIAS_CENTER_TEMPERATURE) {
        temp_i = G_Potentia_Temparature_Average - BIAS_CENTER_TEMPERATURE;
        bias_temp_low = (float) temp_i * BIAS_SCALING;
        scaled_bias = bias - (int) bias_temp_low;
    } else {
        if (G_Potentia_Temparature_Average < BIAS_CENTER_TEMPERATURE) {
            temp_i = BIAS_CENTER_TEMPERATURE - G_Potentia_Temparature_Average;
            bias_temp_high = (float) temp_i * BIAS_SCALING;
            scaled_bias = bias + (int) bias_temp_high;
        } else {
            scaled_bias = bias;
        }
    }
    //scaled_bias = bias;//Test code for Stew
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_Potentia_Bias . adjust_bias . bias: %d, bias_temp_low: %f, bias_temp_high: %f, scaled_bias: %d, Temp: %d\n",
            line_number++, bias, bias_temp_low, bias_temp_high, scaled_bias, G_Potentia_Temparature_Average);
    return scaled_bias;
}

unsigned long myrand_bias(void) {
    next = next * 1103515245 + 12345;
    return ((next / 65536) % 32768);
}

void segvhandler(int signum) {
    fprintf(stderr, "Something failed in Check_Potentia_Bias\n");
    exit(0);
    //signal(signum, SIG_DLF);
}

void *Check_Potentia_Bias(void *t) {
    int bias_value = 0;
    int sleep_time = 1;
    unsigned long random = 0;
    uint8_t calibrate_started = FALSE;
    uint8_t previous_tune_power = 0;
    int status = 0;
    int send_count = 0;
    char message[PATH_MAX];

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Check_Potentia_Bias . Started.\n", line_number++);
    status = INA219_Init();
    while (G_all_threads_run && status >= 0) {
        if (G_BIAS_allow == TRUE) {
            //print_time(1);
            //fprintf(G_fp_logfile, "[%d] Check_Potentia_Bias . Checking Potentia BIAS \n", line_number++);
            if (G_Transceiver_Busy == TRUE && G_Proficio_temperature == TRUE && G_check_potentia == TRUE) {
                random = myrand_bias();
                if (random > 3000) {
                    random = 1500;
                }
                Sleep(random);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Check_Potentia_Bias . Sleeping for: %d seconds. \n", line_number++, random);
            }
            previous_tune_power = G_Tune_Power;
            if (G_check_bias == FALSE) {
                if (calibrate_started == TRUE) {
                    calibrate_started = FALSE;
                    if (previous_tune_power != G_calibrate_tune_power) {
                        SDRcore_trans_send_param(CMD_SET_TUNE_POWER, previous_tune_power);
                        G_calibrate_tune_power = previous_tune_power;
                    }
                }
                bias_value = Normal_Bias_Check(G_SetModeRxTX);
                //print_time(0);
                //fprintf(G_fp_logfile, "[%d] %s . NOT in BIAS MODE \n", line_number++, __func__);
            } else {
                if (G_SetModeRxTX == 1) {
                    calibrate_started = TRUE;
                    bias_value = Normal_Bias_Check(G_SetModeRxTX);
                    //print_time(0);
                    //fprintf(G_fp_logfile, "[%d] Check_Potentia_Bias . IN BIAS MODE \n", line_number++);
                }
            }
            if (send_count++ >= 9) {
                Gui_send_param(CMD_GET_POTENTIA_BIAS, bias_value);
                //SDRcore_trans_send_param(CMD_GET_POTENTIA_BIAS, bias_value);
                send_count = 0;
            }
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] %s . bias_value: %d \n", line_number++, __func__, bias_value);
            G_BIAS_allow = FALSE;
        }
        Sleep(sleep_time);
    }
    if (status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_Potentia_Bias . Abnormal Exit.  Status: %d\n", line_number++, status);
        strcpy(message, "BIAS SENSOR FAILED");
        Gui_Add_Message(message);
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Check_Potentia_Bias . NORMAL EXIT\n", line_number++);
    }
    pthread_exit(0);
    return NULL;
}