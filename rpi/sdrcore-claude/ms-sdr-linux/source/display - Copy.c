#include <math.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <limits.h>
//#include <linux/unistd.h>
#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <string.h>
//#include <netdb.h>
//#include <arpa/inet.h>
#include <errno.h>
#include <sys/timeb.h>
//#include <sys/time.h>
#include "usbavrcmd.h"
#include "SRDLL.h"
#include "extern.h"
#include "version.h"
#include "band_stack.h"
#include "last_used.h"
#include "iq.h"
//#include <wiringPi.h>

extern uint8_t DISPLAY_Position(uint8_t row, uint8_t column);
extern uint8_t DISPLAY_PrintString(uint8_t const string[]);
extern int DISPLAY_PutChar(uint8_t character);
extern void DISPLAY_Init(void);

//extern int LCD1602DispChar(int devFD, unsigned char x, unsigned char y, unsigned char data);
//int LCD1602DispStr(int devFD, unsigned char position, unsigned char line, char *str);
//int LCD1602DispLines(int devFD, uint8_t display_1, uint8_t display_2, char* line1, char* line2);
//int LCD1602Init(int devFD);
//int LCD1602Clear(int devFD);

#define INITIALIZE_COUNT 300000
#define OPERATION_COMPLETED 3
#define OPERATION_PENDING 2 
#define OPERATION_STARTED 1
#define OPERATION_IDLE 0
#define OPERATION_FAILED 4
#define FREQUENCY_POSITION 3
#define MODE_POSITION 14
#define STEP_POSITION 15

#define NONE  0
#define STEP  1
#define PTT  2
#define TUNE  3
#define MODE  4
#define RIT  5
#define BAND  6
#define VOLUME 7

#define KNOB 0
#define BUTTON_A 1
#define BUTTON_B 2
#define BUTTON_C 3  
#define BUTTON_A_POSITION 0
#define BUTTON_B_POSITION 6
#define BUTTON_C_POSITION 12

#define MAX_COMMAND_QUEUE 50
#define QUEUE_FAILED 2000
#define QUEUE_SUCCESS 1
#define QUEUE_BUSY 2
#define QUEUE_CHECK_DELAY 1
#define QUEUE_PROCESS_DELAY 10
#define DISPLAY_COUNT 3

static char freq_display[17] = {0};
uint8_t display_size = sizeof (freq_display);
static char previous_freq_display[17] = {0};
static char mode_string[4][4] = {"A", "L", "U", "C"};

char button_Text[][4] = {"NON", "STP", "PTT", "TUN", "MOD", "RIO", "BND", "F/V", "CBW", "HBW", "FAV", "RST"};
char step_size[][2] = {"6", "5", "4", "3", "2", "1"};
char ptt_star[][3] = {"* ", "  "};
char s_meter_value[][4] = {"S0 ", "S1 ", "S2 ", "S3 ", "S4 ", "S5 ", "S6 ", "S7 ", "S8 ", "S9 ", "10 ", "20 ", "30 "};

uint8_t E_buttons = 0;
uint8_t E_star = 0;
uint8_t E_step = 0;
uint8_t E_display_addr = 0;
uint8_t E_s_meter = 0;
uint8_t E_band = 0;
uint32_t E_freq = 0;
char E_host_mode = 'N';

int8_t E_Display_freq_queue_front = -1;
int8_t E_Display_freq_queue_rear = -1;
uint32_t E_Display_freq_queue[MAX_COMMAND_QUEUE] = {0};
int E_Display_queue_count = 0;

#define MASTER_TIMER 20
#define CONTROL_TIMER 20
#define END_TIMER 5

extern uint32_t E_master_timer;
extern uint32_t E_end_timer;
extern uint32_t E_control_timer;
const char *Display_thread_name;
byte G_Display_allow = TRUE;

#define BAND_10M 9
#define BAND_12M 8   
#define BAND_15M 7
#define BAND_17M 6
#define BAND_20M 5
#define BAND_30M 4
#define BAND_40M 3
#define BAND_60M 2    
#define BAND_80M 1 
#define BAND_160M 0

void Display_freq_queue_add(uint32_t command) {
    if (G_Display_attached) {
        if (E_Display_freq_queue_front == (E_Display_freq_queue_rear + 1) % MAX_COMMAND_QUEUE) {
            E_Display_queue_count = 0;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Display_freq_queue_add. QUEUE OVERFLOW \n", line_number++);
        }
        else {
            if (E_Display_freq_queue_front == -1) {
                E_Display_freq_queue_front = E_Display_freq_queue_rear = 0;
            }
            else {
                E_Display_freq_queue_rear = (E_Display_freq_queue_rear + 1) % MAX_COMMAND_QUEUE;
            }
            E_Display_freq_queue[E_Display_freq_queue_rear] = command;
            E_Display_queue_count++;
        }
    }
}

uint32_t Display_dequeue_freq() {
    uint32_t ret = 0;
    if (E_Display_freq_queue_front == -1) {
        ret = 0;
    } else {
        ret = E_Display_freq_queue[E_Display_freq_queue_front];
        E_Display_freq_queue[E_Display_freq_queue_front] = 0;
        if (E_Display_freq_queue_front == E_Display_freq_queue_rear) {
            E_Display_freq_queue_front = E_Display_freq_queue_rear = -1;
        } else {
            E_Display_freq_queue_front = (E_Display_freq_queue_front + 1) % MAX_COMMAND_QUEUE;
        }
    }
    if (ret != 0) {
        E_Display_queue_count--;
    }
    return ret;
}

uint8_t format_periods(uint32_t n, char *out) {
    static int c;
    static char temp_buf[20] = {0};
    static char *p;
    static uint8_t state = 0;
    static int divide = 0;
    static int modResult;
    static int length = 0;
    static int isNegative = 0;
    static int string_lenth = 0;
    static uint32_t copyOfNumber;
    uint8_t status = OPERATION_PENDING;

    switch (state) {
        case 0:
            if (n < 10000000) {
                isNegative = 1;
                length++;
            }
            copyOfNumber = n;
            while (copyOfNumber != 0) {
                length++;
                copyOfNumber /= 10;
            }
            state++;
            break;
        case 1:
            for (divide = 0; divide < length; divide++) {
                modResult = n % 10;
                n = n / 10;
                temp_buf[length - (divide + 1)] = modResult + '0';
            }
            state++;
            break;
        case 2:
            if (isNegative) {
                temp_buf[0] = ' ';
            }
            temp_buf[length] = '\0';
            while ((temp_buf[string_lenth]) != 0) {
                string_lenth++;
            }
            state++;
            break;
        case 3:
            c = 2 - string_lenth % 3;
            for (p = temp_buf; *p != 0; p++) {
                *out++ = *p;
                if (c == 1) {
                    *out++ = '.';
                }
                c = (c + 1) % 3;
            }
            *--out = 0;
            state = c = divide = modResult = length = isNegative = string_lenth = copyOfNumber = 0;
            status = OPERATION_COMPLETED;
            break;
    }
    return status;
}

uint8_t get_position(char *old_freq, char *new_freq) {
    uint8_t index = 0;
    uint8_t stop = FALSE;

    do {
        if (old_freq[index] != new_freq[index]) {
            stop = TRUE;
        } else {
            index++;
            if (index > 16) {
                stop = TRUE;
            }
        }
    } while (stop == FALSE);
    return index;
}

uint8_t write_display_freq(uint32_t previous_freq, uint32_t freq) {
    uint8_t status = OPERATION_PENDING;
    static uint8_t state = 0;
    static uint8_t pos;
    static uint8_t previous_band = 0;
    int display_status = 0;

    switch (state) {
        case 0:
            status = format_periods(freq, freq_display);
            if (status == OPERATION_COMPLETED) {
                status = OPERATION_PENDING;
                state++;
            }
            break;
        case 1:
            status = format_periods(previous_freq, previous_freq_display);
            if (status == OPERATION_COMPLETED) {
                status = OPERATION_PENDING;
                state++;
            }
            break;
        case 2:
            status = OPERATION_PENDING;
            if (E_band < BAND_30M && previous_band != E_band) {
                previous_band = E_band;
                pos = 0;
            } else {
                pos = get_position(previous_freq_display, freq_display);
            }
            state++;
            break;
        case 3:
            display_status = DISPLAY_Position(0, pos + FREQUENCY_POSITION);
            //LCD1602DispStr(0, pos + FREQUENCY_POSITION, 0, &freq_display[pos]);
            state++;
            break;
        case 4:
            display_status = DISPLAY_PrintString(&freq_display[pos]);
            memset(freq_display, 0, display_size);
            status = OPERATION_COMPLETED;
            state = 0;
            break;
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] write_display_freq. -> state: %d, status: %d\n", line_number++, state, status);
    return status;
}

uint8_t display_mode() {
    uint8_t status = 0;
    int display_status = 0;
    char mode = 'N';

    mode = G_mode;
    //display_status = LCD1602DispChar(0, MODE_POSITION, 0, G_mode);
    display_status = DISPLAY_Position(0, MODE_POSITION);
    display_status = DISPLAY_PutChar(G_mode);
    status = OPERATION_COMPLETED;
    return status;
}

uint8_t set_buttons(uint8_t button, int index) {
    uint8_t status = OPERATION_COMPLETED;
    int display_status = 0;

    switch (button) {
        case 1:
            display_status = DISPLAY_Position(1, BUTTON_A_POSITION);
            display_status = DISPLAY_PrintString(button_Text[index]);
            //display_status = LCD1602DispStr(0, BUTTON_A_POSITION, 1, button_A[index]);
            break;
        case 2:
            display_status = DISPLAY_Position(1, BUTTON_B_POSITION);
            display_status = DISPLAY_PrintString(button_Text[index]);
            //display_status = LCD1602DispStr(0, BUTTON_B_POSITION, 1, button_B[index]);
            break;
        case 3:
            display_status = DISPLAY_Position(1, BUTTON_C_POSITION);
            display_status = DISPLAY_PrintString(button_Text[index]);
            //display_status = LCD1602DispStr(0, BUTTON_C_POSITION, 1, button_C[index]);
            break;
    }
    return status;
}

uint8_t set_star(uint8_t button, uint8_t on_off) {
    uint8_t status = OPERATION_COMPLETED;
    static uint8_t state;
    int display_status = 0;
    int position = 0;
    int row = 1;

    switch (button) {
        case 0x10:
            return status;
            break;
        case 0x20:
            position = BUTTON_A_POSITION + 3;
            break;
        case 0x30:
            position = BUTTON_B_POSITION + 3;
            break;
        case 0x40:
            position = BUTTON_C_POSITION + 3;
            break;
    }
    display_status = DISPLAY_Position(row, position);
    if (on_off == TRUE) {
        display_status = DISPLAY_PutChar('*');
    } else {
        display_status = DISPLAY_PutChar(' ');
    }
    return status;
}

uint8_t ptt(uint8_t on_off) {
    static uint8_t state = 0;
    uint8_t status = OPERATION_COMPLETED;
    int display_status = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Process_Display_thread -> ptt. on_off: %d\n", line_number++, on_off);

    display_status = DISPLAY_Position(0, 0);
    switch (on_off) {
        case 0:
            status = DISPLAY_PrintString(ptt_star[1]);
            break;
        case 1:
            status = DISPLAY_PrintString(ptt_star[0]);
            break;
    }

    return status;
}

uint8_t set_step(uint8_t step_value) {
    uint8_t status = OPERATION_COMPLETED;
    int display_status = 0;

    display_status = DISPLAY_Position(0, STEP_POSITION);
    display_status = DISPLAY_PrintString(step_size[step_value]);
    //LCD1602DispStr(0, STEP_POSITION, 0, step_size[step_value]);
    return status;
}

uint8_t s_meter(uint8_t s_value) {
    static uint8_t state = 0;
    uint8_t status = OPERATION_COMPLETED;
    int display_status = 0;

    display_status = DISPLAY_Position(0, 0);
    display_status = DISPLAY_PrintString(s_meter_value[s_value]);
    //LCD1602DispStr(0, 0, 0, s_meter_value[s_value]);
    return status;
}

void *Process_Display_thread(void *t) {
    uint8_t status = OPERATION_COMPLETED;
    static uint32_t previous_freq = 0;
    static uint32_t dequeue_freq = 0;
    static uint8_t freq_processing = FALSE;
    static uint8_t state = 0;
    static uint8_t previous_mode = 'N';
    static uint8_t previous_buttons = 0;
    static int button_index = 0;
    static uint8_t button = 0;
    static uint8_t previous_star = 0;
    static uint8_t star_status = FALSE;
    static uint8_t previous_step = 100;
    static uint8_t previous_s_meter = 0;
    uint8_t first_pass_clear = FALSE;
    int display_status = 0;
    static uint8_t previous_tx_mode = 0;
    static uint8_t tx_state = FALSE;
    

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Process_Display_thread -> STARTED. Sleeping for 3 seconds\n", line_number++);
    //Sleep(3000);
    Display_thread_name = __func__;
    DISPLAY_Init();
    display_status = DISPLAY_Position(0, 2);
    display_status = DISPLAY_PrintString("MSCC STARTING");
    while (G_all_threads_run) {
        if (G_MSCC_Initialized == TRUE) {
            if (first_pass_clear == FALSE) {
                display_status = DISPLAY_Position(0, 2);
                display_status = DISPLAY_PrintString("               ");
                first_pass_clear = TRUE;
            }
            if (G_Display_allow == TRUE) {
                switch (state) {
                    case 0:
                        if (freq_processing == FALSE) {
                            dequeue_freq = Display_dequeue_freq();
                        }
                        if (dequeue_freq != 0 && E_Display_queue_count < DISPLAY_COUNT) {
                            freq_processing = TRUE;
                            status = write_display_freq(previous_freq, dequeue_freq);
                            if (status == OPERATION_COMPLETED) {
                                state++;
                                previous_freq = dequeue_freq;
                                freq_processing = FALSE;
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] Process_Display_thread -> FINISHED. freq: %ld\n",
                                        line_number++, dequeue_freq);
                            }
                        } else {
                            while (E_Display_queue_count >= DISPLAY_COUNT) {
                                dequeue_freq = Display_dequeue_freq();
                            }
                            state++;
                        }
                        break;
                    case 1:
                        if (G_TX == FALSE) {
                            if (previous_mode != G_mode) {
                                status = display_mode();
                                previous_mode = G_mode;
                            }
                        }
                        state++;
                        break;
                    case 2:
                        if (previous_step != E_step) {
                            status = set_step(E_step);
                            if (status == OPERATION_COMPLETED) {
                                previous_step = E_step;
                                state++;
                            }
                        } else {
                            state++;
                        }
                        break;
                    case 3:
                        if (G_TX == FALSE) {
                            if (previous_s_meter != E_s_meter) {
                                status = s_meter(E_s_meter);
                                if (status == OPERATION_COMPLETED) {
                                    previous_s_meter = E_s_meter;
                                    state++;
                                }
                            } else {
                                state++;
                            }
                        } else {
                            state++;
                        }
                        break;
                    case 4:
                        if (previous_buttons != E_buttons) {
                            button_index = E_buttons & 0x0F;
                            if (E_buttons >= 0x30) {
                                button = 3;
                            } else {
                                if (E_buttons >= 0x20) {
                                    button = 2;
                                } else {
                                    if (E_buttons >= 0x10) {
                                        button = 1;
                                    }
                                }
                            }
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Process_Display_thread -> START. button: %d, button_index: %d\n",
                                    line_number++, button, button_index);
                            status = set_buttons(button, button_index);
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Process_Display_thread -> FINISH. button: %d, button_index: %d\n",
                                    line_number++, button, button_index);
                            if (status == OPERATION_COMPLETED) {
                                previous_buttons = E_buttons;
                                state++;
                            }
                        } else {
                            state++;
                        }
                        break;
                    case 5:
                        if (previous_star != E_star) {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Process_Display_thread -> E_star: %x\n", line_number++, E_star);
                            star_status = E_star & 0x01;
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Process_Display_thread -> E_star: %x, star_status: %x\n",
                                    line_number++, E_star, star_status);
                            if (star_status == 0x01) {
                                star_status = TRUE;
                            } else {
                                star_status = FALSE;
                            }
                            button = E_star & 0x70;
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Process_Display_thread -> E_star: %x, star_status: %x, button: %x\n",
                                    line_number++, E_star, star_status, button);
                            status = set_star(button, star_status);
                            if (status == OPERATION_COMPLETED) {
                                previous_star = E_star;
                                state++;
                            }
                        } else {
                            state++;
                        }
                        break;
                    case 6:
                        if (previous_tx_mode != G_TX) {
                            previous_tx_mode = G_TX;
                            ptt(G_TX);
                            if (G_TX == TRUE) {
                                tx_state = TRUE;
                            }
                        } else {
                            if (G_TX == FALSE) {
                                if (tx_state == TRUE) {
                                    previous_s_meter = 0;
                                    tx_state = FALSE;
                                }
                            }
                        }
                        state = 0;
                        break;
                }
                G_Display_allow = FALSE;
            }
        }
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] Process_Display_thread -> G_tune_freq: %ld\n", line_number++, G_tune_freq);
        Sleep(1);
    }
    display_status = DISPLAY_Position(0, 0);
    //                        0123456789ABCDEF
    display_status = DISPLAY_PrintString("  MSCC STOPPED  ");
    display_status = DISPLAY_Position(1, 0);
    display_status = DISPLAY_PrintString("                ");
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s -> Normal Exit  \n", line_number++, __func__);
    pthread_exit(0);
    return NULL;
}
/* [] END OF FILE */

