#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
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

unsigned long E_freq_queue[MAX_COMMAND_QUEUE] = {0};
int E_freq_queue_front = -1;
int E_freq_queue_rear = -1;
int E_queue_count = 0;
int E_queue_busy = FALSE;

unsigned long freq_queue_add(unsigned long freq) {
    unsigned long status = QUEUE_SUCCESS;
    if (freq != 0) {
        if (G_Rit_Status && !G_TX) {
            freq = freq + G_Rit_Offset;
        }
        if (!E_queue_busy) {
            E_queue_busy = TRUE;
            if (E_freq_queue_front == (E_freq_queue_rear + 1) % MAX_COMMAND_QUEUE) {
                E_freq_queue_front = -1;
                E_freq_queue_rear = -1;
                E_queue_count = 0;
                status = QUEUE_FAILED;
            } else {
                if (E_freq_queue_front == -1) {
                    E_freq_queue_front = E_freq_queue_rear = 0;
                } else {
                    E_freq_queue_rear = (E_freq_queue_rear + 1) % MAX_COMMAND_QUEUE;
                }
                E_freq_queue[E_freq_queue_rear] = freq;
                E_queue_count++;
            }
            E_queue_busy = FALSE;
        } else {
            status = QUEUE_BUSY;
        }
    }
    return (status);
}

unsigned long dequeue_freq() {
    unsigned long ret = 0;

    if (E_freq_queue_front == -1) {
        ret = 0;
    } else {
        ret = E_freq_queue[E_freq_queue_front];
        E_freq_queue[E_freq_queue_front] = 0;
        if (E_freq_queue_front == E_freq_queue_rear) {
            E_freq_queue_front = E_freq_queue_rear = -1;
        } else {
            E_freq_queue_front = (E_freq_queue_front + 1) % MAX_COMMAND_QUEUE;
        }
    }
    if (ret != 0) {
        E_queue_count--;
    }
    return ret;
}

void *Freq_Dequeue_thread(void *t) {
    unsigned long freq = 0;
    int process_delay = QUEUE_CHECK_DELAY;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Freq_Dequeue_thread . Started.\n", line_number++);
    while (G_all_threads_run) {
        if (!E_queue_busy) {
            freq = dequeue_freq();
            if (freq != 0 && E_queue_count < DISPLAY_COUNT) {
                SetHWLO_queue(freq);
                process_delay = QUEUE_PROCESS_DELAY;
            } else {
                process_delay = QUEUE_CHECK_DELAY;
                if (E_queue_count >= DISPLAY_COUNT) {
                    while (E_queue_count >= DISPLAY_COUNT) {
                        if (!E_queue_busy) {
                            freq = dequeue_freq();
                        }
                    }
                }
            }
        }
        Sleep(process_delay);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Freq_Dequeue_thread . NORMAL EXIT.\n", line_number++);
    pthread_exit(NULL);
    return NULL;
}
