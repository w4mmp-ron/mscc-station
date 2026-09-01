#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include "port_defines.h"
#include "usbavrcmd.h"
//#include "SRDLL.h"
#include "extern.h"
#include "version.h"


#define MAX_COMMAND_QUEUE 50
#define QUEUE_FAILED 2000
#define QUEUE_SUCCESS 1
#define QUEUE_BUSY 2
#define QUEUE_CHECK_DELAY 1
#define QUEUE_PROCESS_DELAY 10
#define DISPLAY_COUNT 4

char M_freq_queue[PATH_MAX][MAX_COMMAND_QUEUE] = {0};
int M_freq_queue_front = -1;
int M_freq_queue_rear = -1;
int M_queue_count = 0;
int M_queue_busy = FALSE;
char return_buffer[PATH_MAX];

int message_queue_add(int command, char *message, int size) {
    int status = QUEUE_SUCCESS;

    if (!M_queue_busy) {
        M_queue_busy = TRUE;
        if (M_freq_queue_front == (M_freq_queue_rear + 1) % MAX_COMMAND_QUEUE) {
            M_freq_queue_front = -1;
            M_freq_queue_rear = -1;
            M_queue_count = 0;
            status = QUEUE_FAILED;
        } else {
            if (M_freq_queue_front == -1) {
                M_freq_queue_front = M_freq_queue_rear = 0;
            } else {
                M_freq_queue_rear = (M_freq_queue_rear + 1) % MAX_COMMAND_QUEUE;
            }
            strcpy(M_freq_queue[M_freq_queue_rear], message);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] message_queue_add . MESSAGE ADDED. %s\n",
                    line_number++, M_freq_queue[M_freq_queue_rear]);
            M_queue_count++;
        }
        M_queue_busy = FALSE;
    } else {
        status = QUEUE_BUSY;
    }
    return (status);
}

int message_queue_dequeue(char *buffer) {
    int message_status = 0;

    if (M_freq_queue_front == -1) {
        memset(buffer, 0, sizeof (buffer));
    } else {
        strcpy(buffer, M_freq_queue[M_freq_queue_front]);
        memset(&M_freq_queue[M_freq_queue_front], 0, PATH_MAX);
        if (M_freq_queue_front == M_freq_queue_rear) {
            M_freq_queue_front = M_freq_queue_rear = -1;
        } else {
            M_freq_queue_front = (M_freq_queue_front + 1) % MAX_COMMAND_QUEUE;
        }
        message_status = 1;
    }
    return message_status;
}

void Gui_Add_Message(char * message) {
    char data[PATH_MAX] = {0};
    int status = 0;

    strncpy(data, message,PATH_MAX);
    status = message_queue_add(CMD_SET_SERVER_ERROR, data, strlen(data));
    if (status != QUEUE_SUCCESS) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Gui_Add_Message. Add message FAILED \n",line_number++);
    }
}