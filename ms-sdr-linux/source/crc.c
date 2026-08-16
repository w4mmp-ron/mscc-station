#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/timeb.h>
#include "extern.h"
 
byte crcSlow(byte* message, int size) {
    unsigned char sbuf[22] = { 0 };
    int count = 0;
    byte data_buffer[22] = { '0' };
    byte y = 0;

    memcpy(data_buffer, message, size);
    for (count = 0; count < size; count++) {
        sbuf[count] = (uint8_t)data_buffer[count];
        y += sbuf[count];
    }
    y = (0 - y) & 0xff;
    return y;
}

