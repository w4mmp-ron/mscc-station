#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/timeb.h>
#include <sys/time.h>

#include "usbavrcmd.h"
#include <usb.h>
#include "SRDLL.h"
#include "extern.h"
#include "version.h"
#include "band_stack.h"
#include "last_used.h"
#include "iq.h"

#define TRUE 1
#define FALSE 0

#define OPERATION_COMPLETED 3
#define OPERATION_PENDING 2 
#define OPERATION_STARTED 1
#define OPERATION_IDLE 0
#define OPERATION_FAILED 4


static unsigned char RS = 0x01; //Clear Screen
//static unsigned char  RW = 0x02; //Cursor Return
static unsigned char En = 0x04; //Display Cursor
static unsigned char BL = 0x08; //Function Set
uint8_t devFD = 0x27;

void format_periods_1(uint32_t n, char *out) {
    int c;
    char temp_buf[20] = {0};
    char *p;

    int divide = 0;
    int modResult;
    int length = 0;
    int isNegative = 0;
    int copyOfNumber;

    copyOfNumber = n;
    while (copyOfNumber != 0) {
        length++;
        copyOfNumber /= 10;
    }

    for (divide = 0; divide < length; divide++) {
        modResult = n % 10;
        n = n / 10;
        temp_buf[length - (divide + 1)] = modResult + '0';
    }
    if (isNegative) {
        temp_buf[0] = '-';
    }
    temp_buf[length] = '\0';

    c = 2 - strlen(temp_buf) % 3;
    for (p = temp_buf; *p != 0; p++) {
        *out++ = *p;
        if (c == 1) {
            *out++ = '.';
        }
        c = (c + 1) % 3;
    }
    *--out = 0;
}

int pulseEnable(int devFD, unsigned char data) {
    int tempFD;
    int write_status = 0;
    unsigned char temp_data;
    char filename[PATH_MAX];

    tempFD = devFD;
    temp_data = (data | En);
    Sleep(3);
    write_status = Write_i2c(I2C_BUSS, filename, E_display_addr, 0x00, &temp_data, 1, "pulseEnable(1)");
    if (write_status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] pulseEnable(1). FAILED\n", line_number++);
        return -1;
    }
    Sleep(3);
    temp_data = (data & ~En);
    write_status = Write_i2c(I2C_BUSS, filename, E_display_addr, 0x00, &temp_data, 1, "pulseEnable(2)");
    if (write_status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] pulseEnable(2). FAILED\n", line_number++);
        return -1;
    }
    return 0;
}

int pcf8574WriteCmd4(int devFD, unsigned char command) {
    int write_status = 0;
    unsigned char temp_data;
    char filename[PATH_MAX];

    temp_data = command;
    temp_data = command | BL;

    write_status = Write_i2c(I2C_BUSS, filename, E_display_addr, 0x00, &temp_data, 1, "pcf8574WriteCmd4");
    if (write_status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] pcf8574WriteCmd4. FAILED\n", line_number++);
        return -1;
    }
    if (pulseEnable(devFD, command | BL) == -1) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] pcf8574WriteCmd4. Call to pulseEnable. FAILED\n", line_number++);
        return -1;
    }
    return 0;
}

int pcf8574WriteCmd8(int devFD, unsigned char command) {
    unsigned char command_H = command & 0xf0;
    unsigned char command_L = (command << 4) & 0xf0;
    
    if (pcf8574WriteCmd4(devFD, command_H) == -1) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] pcf8574WriteCmd8(1). Call to pcf8574WriteCmd4 FAILED\n", line_number++);
        return -1;
    }

    if (pcf8574WriteCmd4(devFD, command_L) == -1) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] pcf8574WriteCmd8(2). Call to pcf8574WriteCmd4 FAILED", line_number++);
        return -1;
    }
    return 0;
}

int pcf8574WriteData4(int devFD, unsigned char data) {
    int write_status = 0;
    unsigned char temp_data;
    char filename[PATH_MAX];

    temp_data = (data | RS | BL);
    write_status = Write_i2c(I2C_BUSS, filename, E_display_addr, 0x00, &temp_data, 1, "pcf8574WriteData4");
    if (write_status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] pcf8574WriteData4. FAILED", line_number++);
        return -1;
    }
    if (pulseEnable(devFD, (data | RS | BL)) == -1) {
        return -1;
    }
    return 0;
}

int pcf8574WriteData8(int devFD, unsigned char data) {
    unsigned char data_H = data & 0xf0;
    unsigned char data_L = (data << 4) & 0xf0;
    if (pcf8574WriteData4(devFD, data_H) == -1) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Fail to write dat low 4bit\n", line_number++);
        return -1;
    }
    if (pcf8574WriteData4(devFD, data_L) == -1) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Fail to write dat low 4bit\n", line_number++);
        return -1;
    }
    return 0;
}

int LCD1602DispChar(int devFD, unsigned char x, unsigned char y, unsigned char data) {
    unsigned char addr;
    if (y == 0)
        addr = 0x80 + x;
    else
        addr = 0xc0 + x;

    if (pcf8574WriteCmd8(devFD, addr) == -1) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Fail to write cmd 8bit\n", line_number++);
        return -1;
    }
    if (pcf8574WriteData8(devFD, data) == -1) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Fail to write data 8bit\n", line_number++);
        return -1;
    }
    return 0;
}

int LCD1602DispStr(int devFD, unsigned char position, unsigned char line, char *str) {
    unsigned char addr;
    addr = ((line + 2) * 0x40) + position;

    if (pcf8574WriteCmd8(devFD, addr) == -1) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Fail to write cmd 8bit\n", line_number++);
        return -1;
    }
    while (*str != '\0') {
        if (pcf8574WriteData8(devFD, *str++) == -1) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Fail to write data 8bit\n", line_number++);
            return -1;
        }
    }
    return 0;
}

int LCD1602DispLines(int devFD, uint8_t line_1, uint8_t line_2, char* line1, char* line2) {
    int ret;

    if (line_1) {
        ret = LCD1602DispStr(devFD, 0, 0, line1);
    }
    if (ret != -1 && line_2) {
        ret = LCD1602DispStr(devFD, 0, 1, line2);
    }
    return ret;
}

int LCD1602Init(int devFD) {
    print_time(0);
    fprintf(G_fp_logfile, "[%d] LCD1602Init. CALLED\n", line_number++);
    usleep(100);
    if (pcf8574WriteCmd4(devFD, 0x03 << 4) == -1) {
        return -1;
    }
    usleep(100 * 41);
    if (pcf8574WriteCmd4(devFD, 0x03 << 4) == -1) {
        return -1;
    }
    usleep(100);
    if (pcf8574WriteCmd4(devFD, 0x03 << 4) == -1) {
        return -1;
    }
    usleep(100);
    if (pcf8574WriteCmd4(devFD, 0x02 << 4) == -1) {
        return -1;
    }
    usleep(100);
    if (pcf8574WriteCmd8(devFD, 0x28) == -1) {
        return -1;
    }
    usleep(100);
    if (pcf8574WriteCmd8(devFD, 0x0c) == -1) {
        return -1;
    }
    usleep(2000);
    if (pcf8574WriteCmd8(devFD, 0x06) == -1) {
        return -1;
    }
    usleep(100);
    if (pcf8574WriteCmd8(devFD, 0x02) == -1) {
        return -1;
    }
    usleep(2000);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] LCD1602Init. FINISHED\n", line_number++);
    return 1;
}

int LCD1602Clear(int devFD) {
    if (pcf8574WriteCmd8(devFD, 0x01) == -1) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Fail to pcf8574write cmd 0x01\n", line_number++);
        return -1;
    }
    return 0;
}

