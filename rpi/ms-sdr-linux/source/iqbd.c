#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "extern.h"
#include "smbus.h"
#include "i2cbusses.h"
#include "util.h"
#include "usbavrcmd.h"

#define PRINT_STDERR (1 << 0)
#define PRINT_READ_BUF (1 << 1)
#define PRINT_WRITE_BUF (1 << 2)
#define PRINT_HEADER (1 << 3)
#define AVERAGE_LIMIT 100
#define MAX_DELAY 400
#define IQ_START 0
#define IQ_OFFSET_MAX 200
#define IQ_OFFSET_MIN -199
#define IQ_COARSE 10
#define IQ_FINE 1;
#define STRUCT_CENTER 199
#define ITERATION_COUNT 3
#define IQ_AVERAGE_ARRAY_LIMIT 3
#define IQBD_ARRAY_SIZE 401
#define N_DECIMAL_POINTS_PRECISION (100)
#define IQBD_REPORT_DELAY (IQBD_ARRAY_SIZE * 2)

byte G_IQBD_buffer[256];
uint32_t IQBD_Array[IQBD_ARRAY_SIZE];
int16_t IQBD_Final_Offset[ITERATION_COUNT];

enum iqbd_parse_state {
    PARSE_GET_DESC,
    PARSE_GET_DATA,
};

uint32_t G_iqbd_value = 0;
int previous_power = 0;
int G_Cancel_Calibration = 0;
int16_t G_lowest_iq_value = 999;
int G_IQBD_Address = 0x49;

static int IQBD_check_funcs_direct(int file) {
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

static void IQBD_print_msgs(struct i2c_msg *msgs, __u32 nmsgs, unsigned flags) {
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

                G_IQBD_buffer[j] = msgs[i].buf[j];
            }
            G_IQBD_buffer[j] = msgs[i].buf[j];
        } else if (flags & PRINT_HEADER) {
            //fprintf(output, "\n");
        }
    }
}

static int IQBD_confirm(const char *filename, struct i2c_msg *msgs, __u32 nmsgs) {
    fprintf(G_fp_logfile, "WARNING! This program can confuse your I2C bus, cause data loss and worse!\n");
    fprintf(G_fp_logfile, "I will send the following messages to device file %s:\n", filename);
    IQBD_print_msgs(msgs, nmsgs, PRINT_STDERR | PRINT_HEADER | PRINT_WRITE_BUF);

    fprintf(G_fp_logfile, "Continue? [y/N] ");
    fflush(G_fp_logfile);
    if (!user_ack(0)) {
        fprintf(G_fp_logfile, "Aborting on user request.\n");
        return 0;
    }
    return 1;
}

int IQBD_err_out(char *message) {
    int status = -1;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] IQBD_err_out . err_out.  ERROR: %s\n", line_number++, message);
    return status;
}

int IQBD_Read_i2c_direct(int argc, char *argv[]) {
    char filename[20];
    int i2cbus, address = -1, file, arg_idx = 1, nmsgs = 0, nmsgs_sent, i;
    int force = 0, yes = 0, version = 0, verbose = 0, all_addrs = 0;
    struct i2c_msg msgs[I2C_RDRW_IOCTL_MAX_MSGS];
    enum iqbd_parse_state state = PARSE_GET_DESC;
    unsigned buf_idx = 0;
    int status = 0;
    byte count = 0;

    for (i = 0; i < I2C_RDRW_IOCTL_MAX_MSGS; i++)
        msgs[i].buf = NULL;
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
                status = IQBD_err_out("Unsupported option ");
        }
        arg_idx++;
    }
    if (arg_idx == argc) {
        status = IQBD_err_out("arg_idx == argc");
    }
    i2cbus = lookup_i2c_bus(argv[arg_idx++]);
    if (i2cbus < 0) {
        status = IQBD_err_out("i2cbus < 0");
    } else {

        file = open_i2c_dev(i2cbus, filename, sizeof (filename), 0);
        if (file < 0 || IQBD_check_funcs_direct(file))
            status = IQBD_err_out("file open or check_funcs_direct");
    }
    if (status >= 0) {
        while (arg_idx < argc) {
            char *arg_ptr = argv[arg_idx];
            unsigned long len, raw_data;
            __u16 flags;
            __u8 data, *buf;
            char *end;
            if (nmsgs > I2C_RDRW_IOCTL_MAX_MSGS) {
                status = IQBD_err_out("Too many messages");
            }
            switch (state) {
                case PARSE_GET_DESC:
                    flags = 0;
                    switch (*arg_ptr++) {
                        case 'r': flags |= I2C_M_RD;
                            break;
                        case 'w': break;
                        default:
                            status = IQBD_err_out("Invalid direction");
                    }
                    len = strtoul(arg_ptr, &end, 0);
                    if (len > 0xffff || arg_ptr == end) {
                        status = IQBD_err_out("Length invalid");
                    }
                    arg_ptr = end;
                    if (*arg_ptr) {
                        if (*arg_ptr++ != '@') {
                            status = IQBD_err_out("Unknown separator after length");
                        }
                        address = parse_i2c_address(arg_ptr, all_addrs);
                        if (address < 0)
                            status = IQBD_err_out("parse_i2c_address: address < 0");

                        if (!force && set_slave_addr(file, address, 0))
                            status = IQBD_err_out("device busy");
                    } else {

                        if (address < 0) {
                            status = IQBD_err_out("No address given");
                        }
                    }
                    msgs[nmsgs].addr = address;
                    msgs[nmsgs].flags = flags;
                    msgs[nmsgs].len = len;
                    if (len) {
                        buf = malloc(len);
                        if (!buf) {
                            status = IQBD_err_out("No memory for buffer");
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
                        status = IQBD_err_out("Invalid data byte");
                    }
                    data = raw_data;
                    len = msgs[nmsgs].len;
                    while (buf_idx < len) {
                        msgs[nmsgs].buf[buf_idx++] = data;
                        if (!*end)
                            break;
                        switch (*end) {
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
                                status = IQBD_err_out("Invalid data byte suffix");
                        }
                    }
                    if (buf_idx == len) {
                        nmsgs++;
                        state = PARSE_GET_DESC;
                    }
                    break;
                default:
                    status = IQBD_err_out("Unknown state in state machine!");
            }
            arg_idx++;
        }
        if (state != PARSE_GET_DESC || nmsgs == 0) {
            //fprintf(G_fp_logfile, "Error: Incomplete message\n");
            status = IQBD_err_out("Incomplete message");
        }
        if (yes || IQBD_confirm(filename, msgs, nmsgs)) {
            struct i2c_rdwr_ioctl_data rdwr;
            rdwr.msgs = msgs;
            rdwr.nmsgs = nmsgs;
            if (status >= 0) {
                nmsgs_sent = ioctl(file, I2C_RDWR, &rdwr);
                if (nmsgs_sent < 0) {
                    status = IQBD_err_out("Sending messages failed");
                } else if (nmsgs_sent < nmsgs) {
                    status = IQBD_err_out("Number of messages send incorrect");
                }
                IQBD_print_msgs(msgs, nmsgs_sent, PRINT_READ_BUF | (verbose ? PRINT_HEADER | PRINT_WRITE_BUF : 0));
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

void Read_iqbd() {
    int status = 0;
    char filename[PATH_MAX] = {0};

    char *argv[10];
    char dummy[] = "Read_iqbd";
    char yes[] = "-y";
    char first[20] = {0};
    char third[] = "r2";
    char i2cbuss[] = "6";
    unsigned char control_string[20] = {0};
    char iqbd_address[20] = {0};
   
    char control = 0x05;
    sprintf(control_string, "0x%0X", control);
    
    sprintf(iqbd_address,"0x%0X",G_IQBD_Address);
    strcpy(first,"w1@");
    strcat(first,iqbd_address);
    
    argv[0] = dummy;
    argv[1] = yes;
    argv[2] = i2cbuss;
    argv[3] = first;
    argv[4] = control_string;
    argv[5] = third;

    status = IQBD_Read_i2c_direct(6, argv);
}

void IQBD_Set_Offset(int iq_offset) {
    int status = 0;

    SDRcore_trans_send_param(CMD_SET_IQ_OFFSET, iq_offset);
    Sleep(10);
    G_IQBD_New_Value_Set = TRUE;
}

void IQBD_Set_Tune_Power(byte tune_power) {
    SDRcore_trans_send_param(CMD_SET_TUNE_POWER, tune_power);
}

uint32_t Get_IQBD_Raw() {
    byte upper_byte = 0;
    byte lower_byte = 0;
    byte lower_byte_shifted = 0;
    uint32_t value = 0;
    float f_temp_value = 0.0f;
    uint32_t frac_part = 0;
    uint32_t int_part = 0;
    int32_t return_value = 0;

    Read_iqbd();
    upper_byte = G_IQBD_buffer[0];
    lower_byte = G_IQBD_buffer[1];

    f_temp_value = 3.3f / 1024.0f;
    lower_byte = lower_byte & 0xFC;
    value = upper_byte;
    value = value << 6;
    lower_byte_shifted = lower_byte >> 2;

    value = value + lower_byte_shifted;
    return_value = value / 10;
    f_temp_value = f_temp_value * (float) value;

    frac_part = (uint32_t) (f_temp_value * N_DECIMAL_POINTS_PRECISION) % N_DECIMAL_POINTS_PRECISION;
    int_part = (uint32_t) f_temp_value;
    //ADC_Return_Value = (int_part * 10) + frac_part;
    //print_time(1);
    //fprintf(G_fp_logfile, "[%d] Get_IQBD_Raw . f_temp_value: %f, int_part: %d, frac_part: %d\n",
    //      line_number++, f_temp_value,int_part,frac_part);
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Get_IQBD_Raw . upper_byte: %0X, lower_byte: %0X, value: %d, return_value: %d\n",
    //      line_number++, upper_byte, lower_byte,value,return_value);
    if (return_value > 100) {
        return_value = 100;
    }
    return return_value;
}

uint32_t IQBD_Read_Value(int16_t first_pass, byte band) {

    int16_t iqbd_average = 0;
    uint32_t iqbd_raw = 0;
    static int16_t low_value = 2000;
    int16_t iq_low_value = 0;
    static byte iq_band = 100;
    int buffer_limit = 0;

    iqbd_raw = Get_IQBD_Raw();
    return iqbd_raw;
}

/*int Set_Proficio_IQBD(int16_t iq_offset) {
    int status = 0;
    print_time(1);
    fprintf(G_fp_logfile, "[%d] Set_Proficio_IQBD . CMD_SET_IQ_OFFSET . Calling set_iq_record . iq_offset: %d\n",
            line_number++, iq_offset);
    SDRcore_trans_send_param(CMD_SET_IQ_OFFSET, iq_offset);
    Sleep(10);
    SDRcore_trans_send_param(CMD_SET_COMMIT_IQ, G_iq_band);
    G_IQBD_New_Value_Set = TRUE;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Set_Proficio_IQBD . CMD_SET_IQ_OFFSET . Finished. status: %d\n", line_number++, status);
    return status;
}*/

/*int Set_IQBD_Calibration(int iteration, int stop) {
    int status = 0;
    int count = 0;
    int average_count = 0;
    int16_t iq_low_value = 2000;
    int16_t iq_low_offset = 2000;
    byte iq_extended_data[4];
    int16_t IQBD_final_offset = 0;
    float IQBD_average_total = 0.0f;
    static int16_t IQBD_final_offset_array[3] = {0};

    struct {
        int16_t IQBD_Offset;
        int16_t IQBD_Value;
    } IQBD_Average[6];

    print_time(1);
    fprintf(G_fp_logfile, "[%d] Set_IQBD_Calibration . called . iteration: %d \n",
            line_number++, iteration);
    for (count = 0; count < 6; count++) {
        IQBD_Average[count].IQBD_Offset = 3000;
        IQBD_Average[count].IQBD_Value = 3000;
    }
    if (stop == FALSE) {
        for (count = 10; count < 390; count++) {
            if (IQBD_Struct[count].IQBD_Value > 0 && IQBD_Struct[count].IQBD_Value < 900) {
                if (IQBD_Struct[count].IQBD_Value < iq_low_value) {
                    IQBD_Average[average_count].IQBD_Offset = IQBD_Struct[count].IQBD_Offset;
                    IQBD_Average[average_count].IQBD_Value = IQBD_Struct[count].IQBD_Value;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Set_IQBD_Calibration LOOP . IQBD_Offset:%d, IQBD_Value:%d \n",
                            line_number++, IQBD_Average[average_count].IQBD_Offset, IQBD_Average[average_count].IQBD_Value);
                    average_count++;
                    if (average_count > 5) {
                        average_count = 0;
                    }
                    iq_low_value = IQBD_Struct[count].IQBD_Value;
                }
            }
        }
        iq_low_value = 2000;
        iq_low_offset = 2000;
        for (average_count = 0; average_count < 6; average_count++) {
            if (IQBD_Average[average_count].IQBD_Value < iq_low_value) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Set_IQBD_Calibration . IQBD_Offset: %d, IQBD_Value: %d \n",
                        line_number++, IQBD_Average[average_count].IQBD_Offset, IQBD_Average[average_count].IQBD_Value);
                IQBD_final_offset_array[iteration] = IQBD_Average[average_count].IQBD_Offset;
            }
            iq_low_value = IQBD_Average[average_count].IQBD_Value;
        }
        memcpy(&iq_extended_data[0], &IQBD_final_offset_array[iteration], sizeof (IQBD_final_offset_array[iteration]));
        Gui_send_param_extended(CMD_SET_IQBD_DATA, iq_extended_data, sizeof (IQBD_final_offset_array[iteration]));
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Set_IQBD_Calibration . stop: %d, iteration: %d, offset: %d\n",
                line_number++, stop, iteration, IQBD_final_offset_array[iteration]);
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Set_IQBD_Calibration . stop: %d \n", line_number++, stop);
        iq_low_offset = 2000;
        for (count = 0; count < 3; count++) {
            IQBD_average_total = IQBD_average_total + (float) IQBD_final_offset_array[count];
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Set_IQBD_Calibration . stop: %d, IQBD_final_offset_array: %d\n",
                    line_number++, stop, IQBD_final_offset_array[count]);
        }
        IQBD_final_offset = (int16_t) (IQBD_average_total / 3.0f);
        Set_Proficio_IQBD(IQBD_final_offset);
        memcpy(&iq_extended_data[0], &IQBD_final_offset, sizeof (IQBD_final_offset));
        Gui_send_param_extended(CMD_SET_IQBD_DATA, iq_extended_data, sizeof (IQBD_final_offset));
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Set_IQBD_Calibration . stop: %d, IQBD_final_offset: %d\n",
                line_number++, stop, IQBD_final_offset);
    }
    return status;
}*/

int IQBD_Set_Power(int band) {
    int power = 0;

    switch (band) {
        case 9://160
            power = 50;
            break;
        case 8://80
            power = 100;
            break;
        case 7://60
            power = 100;
            break;
        case 6://40
            power = 40;
            break;
        case 5://30
            power = 100;
            break;
        case 4://20
            power = 100;
            break;
        case 3://17
            power = 30;
            break;
        case 2://15
            power = 100;
            break;
        case 1://12
            power = 100;
            break;
        case 0: //10
            power = 100;
            break;
    }
    return power;
}

int16_t IQBD_Set_Low_Limit(int band) {
    int16_t low = 0;

    switch (band) {
        case 9:
            low = 300;
            break;
        case 8:
            low = 300;
            break;
        case 7:
            low = 300;
            break;
        case 6:
            low = 300;
            break;
        case 5:
            low = 300;
            break;
        case 4:
            low = 300;
            break;
        case 3:
            low = 300;
            break;
        case 2:
            low = 300;
            break;
        case 1:
            low = 300;
            break;
        case 0:
            low = 300;
            break;
    }
    return low;
}

void *iqbd(void *t) {
    byte iq_band = 100;
    uint32_t iq_value = 0;
    byte iq_extended_data[4];
    int previous_power = 0;
    int first_run = TRUE;
    int IQBD_struct_count = 0;
    int average_count = 0;
    uint32_t average = 0;
    uint32_t IQBD_Average = 0;
    int send_data_count = 0;
    int i = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] IQBD . Started. \n", line_number++);
    while (G_all_threads_run) {
        while (G_IQBD_Monitor_ON == TRUE) {
            if (first_run == TRUE) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] IQBD . Processing Started. first_run: %d \n", line_number++, first_run);
                G_Proficio_Allow_Temp_Check = FALSE;
                previous_power = G_Tune_Power;
                for (IQBD_struct_count = 0; IQBD_struct_count < IQBD_ARRAY_SIZE; IQBD_struct_count++) {
                    IQBD_Array[IQBD_struct_count] = 0;
                }
                IQBD_struct_count = 0;
                IQBD_Average = 0;
                first_run = FALSE;
                average = 0;

            }
            iq_value = IQBD_Read_Value(0, 0);
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] %s . iq_value: %d  \n", line_number++, __func__, iq_value);
            IQBD_Array[0] = iq_value;
            for (average_count = 0; average_count < IQBD_ARRAY_SIZE; average_count++) {
                average = average + IQBD_Array[average_count];
            }
            IQBD_Average = average / average_count;
            for (i = (IQBD_ARRAY_SIZE - 1); i > 0; i--) {
                IQBD_Array[i] = IQBD_Array[(i - 1)];
            }
            average = 0;
            if (send_data_count >= IQBD_REPORT_DELAY) {
                memcpy(&iq_extended_data[0], &IQBD_Average, sizeof (IQBD_Average));
                Gui_send_param_extended(CMD_SET_IQBD_DATA, iq_extended_data, sizeof (IQBD_Average));
            } else {
                send_data_count++;
            }
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] %s . iq_value: %d  \n", line_number++, __func__, iq_value);
            //Sleep(100);
        }
        if (first_run == FALSE) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] IQBD . Processing Stopped. first_run: %d \n", line_number++, first_run);
            G_Proficio_Allow_Temp_Check = TRUE;
            first_run = TRUE;
            send_data_count = 0;
        }
        Sleep(100);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s . NORMAL EXIT  \n", line_number++, __func__);
    pthread_exit(0);
    return NULL;
}
