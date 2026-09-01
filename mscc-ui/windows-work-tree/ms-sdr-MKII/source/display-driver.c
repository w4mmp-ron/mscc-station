#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/timeb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "usbavrcmd.h"
#include "SRDLL.h"
#include "extern.h"
#include "version.h"
#include "band_stack.h"
#include "last_used.h"
#include "iq.h"
#include "display-driver.h"

#define MASTER_TIMER_VALUE 10000
#define CONTROL_TIMER_VALUE 10000
#define END_TIMER 10000
#define SLEEP_TIME 10

extern byte crcSlow(byte* message, int size);

int file_i2c;
int length;
unsigned char buffer[60] = {0};

int Display_Open(int display_address) {
    char *filename = (char *) "/dev/i2c-6";
    int status = 0;

    if ((file_i2c = open(filename, O_RDWR)) < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Display_Open. Buss Open. FAILED. file_i2c: %d,\n", line_number++, file_i2c);
        status = -1;
    } else {
        int addr = display_address; //<<<<<The I2C address of the slave
        if (ioctl(file_i2c, I2C_SLAVE, addr) < 0) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Display_Open. Set address FAILED. address: 0x:%0X\n", line_number++, addr);
            status = -1;
        }
    }
    return status;
}

int Display_Read(char *buffer, int length) {
    int status = 0;
    if (read(file_i2c, buffer, length) != length) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Display_Read. FAILED\n", line_number++, length);
        status = -1;
    }
}

int Write_I2C_AD0(int i2cbus, char* filename, int address, int daddress, unsigned char* buffer, int buffer_size, char* caller) {
    int status = 0;

    if (write(file_i2c, buffer, buffer_size) != buffer_size) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Write_I2C_AD0. FAILED\n", line_number++, buffer_size);
        status = -1;
    }
    return status;
}

void DISPLAY_Init(int type) {
    uint8_t buffer[6];
    char filename[PATH_MAX] = {0};
    int status = 0;

    buffer[0] = type;
    /*switch (type) {
    case 4:
        buffer[0] = CLEAR_SCREEN;
        break;
    case 5:
        buffer[0] = RESET_DISPLAY;
        break;
    case 3:
        buffer[0] = RESET_QUEUE;
        break;
    }
     */
    status = Write_I2C_AD0(6, filename, E_display_addr, 0, buffer, 1, "DISPLAY_Init");
}

int8_t DISPLAY_Position(uint8_t row, uint8_t column, int caller) {
    int status = 0;
    uint8_t buffer[6];
    char filename[PATH_MAX] = {0};
    
    buffer[0] = SET_CURSOR_ROW_COLUMN;
    buffer[1] = row;
    buffer[2] = column;
    status = Write_I2C_AD0(6, filename, E_display_addr, 0, buffer, 3, "DISPLAY_Position");
    return status;
}

int8_t DISPLAY_PrintString(uint8_t const string[]) {
    int indexU8 = 0u;
    static uint8_t temp_string[40] = {'A'};
    int status = 0;
    int count = 0;
    char filename[PATH_MAX] = {0};
    byte crc_result = 0;

    if (string[0] != 0) {
        indexU8 = 2;
        while ((string[(indexU8 - 2)]) != 0) {
            temp_string[indexU8] = string[(indexU8 - 2)];
            indexU8++;
        }
        indexU8 = strlen(string);
        count = indexU8;
        temp_string[0] = DATA;
        temp_string[1] = count;
        status = Write_I2C_AD0(6, filename, E_display_addr, 0, temp_string, (2 + count), "DISPLAY_PrintString");
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] DISPLAY_PrintString. size: %d, crc_result: %d:0x%X\n", line_number++, (count + 2),crc_result,
        //                                                            crc_result);
        //if ((count + 2) == 18) {
        //    print_time(0);
        //    fprintf(G_fp_logfile, "[%d] DISPLAY_PrintString. string: %s\n", line_number++,string);
        //}
        memset(temp_string, 0, sizeof (temp_string));
        indexU8 = 0;
    }
    return status;
}

int DISPLAY_PutChar(char character) {
    int status = 0;
    uint8_t buffer[6];
    char filename[PATH_MAX] = {0};
    byte crc_result = 0;

    buffer[0] = DATA;
    buffer[1] = 1;
    buffer[2] = character;
    status = Write_I2C_AD0(6, filename, E_display_addr, 0, buffer, (3), "DISPLAY_PutChar");
    return status;
}

