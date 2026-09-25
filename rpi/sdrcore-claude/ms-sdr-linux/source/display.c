#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "usbavrcmd.h"
#include <usb.h>
#include "SRDLL.h"
#include "extern.h"
#include "version.h"
#include "band_stack.h"
#include "last_used.h"
#include "iq.h"
#include "display-driver.h"

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
#define DISPLAY_FREQUENCY_OPERATION 1
#define DISPLAY_MAX_INTERNAL_QUEUE 32
#define DISPLAY_MINIMUM_INTERNAL_QUEUE 14


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
uint8_t display_operation = OPERATION_COMPLETED;
uint8_t refresh_operation = OPERATION_COMPLETED;

int8_t E_Display_freq_queue_front = -1;
int8_t E_Display_freq_queue_rear = -1;
uint32_t E_Display_freq_queue[MAX_COMMAND_QUEUE] = {0};
int E_Display_queue_count = 0;
int E_Display_queue_busy = 0;

#define MASTER_TIMER 20
#define CONTROL_TIMER 20
#define END_TIMER 5

extern uint32_t E_master_timer;
extern uint32_t E_end_timer;
extern uint32_t E_control_timer;
const char *Display_thread_name;
byte check_queue_failed = FALSE;
byte G_Refresh_Display = FALSE;
byte G_Set_I2C_Speed = 0;
int I2C_Error_Count = 0;


char saved_mode = 'N';
char saved_s_meter[4] = {0};
char saved_step[2] = {0};

struct {
    uint8_t button;
    char button_index;
} saved_buttons[3];
char saved_ascii_freq[17] = {0};


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

int Get_Display_Queue(int queue_size_request);
uint8_t set_buttons(uint8_t button, int index);

/*int Elapsed_Time(int reset) {
    static SYSTEMTIME  start_time;
    static SYSTEMTIME  end_time;
    int diff = 0;
    int status = 0;

    if (reset) {
        GetSystemTime(&start_time);
    }
    else {
        GetSystemTime(&end_time);
        diff = end_time.wMinute - start_time.wMinute;
    }
    return diff;
}*/
int Read_I2C_SGL(int i2cbus, char* filename, int address, int daddress, unsigned char* buffer, int buffer_size) {
    int status = 0;
    //status = Read_i2c(i2cbus, filename, address, daddress,buffer, buffer_size);
    status = Display_Read(buffer, buffer_size);
    return status;
}

time_t Elapsed_Time_Seconds(int reset) {
    static time_t start_time;
    static time_t end_time;
    time_t diff = 0;
    int status = 0;

    if (reset) {
        start_time = time(NULL);
    } else {
        end_time = time(NULL);
        diff = end_time - start_time;
    }
    return diff;
}

void Refresh_Display(int type) {
    int status = 0;
    int count = 0;
    char buffer[4] = {0};
    char filename[PATH_MAX] = {0};

    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] %s. %s. STARTED.\n", line_number++, Display_thread_name, __func__);
    refresh_operation = OPERATION_PENDING;
    if (type == 0) {
        if ((Get_Display_Queue(12))) {
            status = DISPLAY_Position(0, 0, 0);
            status = DISPLAY_PrintString("                ");
            status = DISPLAY_Position(0, FREQUENCY_POSITION, 0);
            status = DISPLAY_PrintString(saved_ascii_freq);
            status = DISPLAY_Position(0, STEP_POSITION, 0);
            status = DISPLAY_PrintString(saved_step);
            status = DISPLAY_Position(0, MODE_POSITION, 0);
            status = DISPLAY_PutChar(G_mode);
            status = DISPLAY_Position(0, 0, 1);
            status = DISPLAY_PrintString(saved_s_meter);
            for (count = 0; count < 3; count++) {
                set_buttons(saved_buttons[count].button, saved_buttons[count].button_index);
                Sleep(20);
            }
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s. %s. Get_Display_Queue FAILED.\n", line_number++, Display_thread_name, __func__);
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s. %s. type: %d\n", line_number++, Display_thread_name, __func__, type);
        DISPLAY_Init(RESET_QUEUE);
        Sleep(20);
        if ((Get_Display_Queue(12))) {
            status = DISPLAY_Position(0, 0, 0);
            status = DISPLAY_PrintString("                ");
            status = DISPLAY_Position(0, FREQUENCY_POSITION, 0);
            status = DISPLAY_PrintString(saved_ascii_freq);
            status = DISPLAY_Position(0, STEP_POSITION, 0);
            status = DISPLAY_PrintString(saved_step);
            status = DISPLAY_Position(0, MODE_POSITION, 0);
            status = DISPLAY_PutChar(G_mode);
            status = DISPLAY_Position(0, 0, 1);
            status = DISPLAY_PrintString(saved_s_meter);
            for (count = 0; count < 3; count++) {
                set_buttons(saved_buttons[count].button, saved_buttons[count].button_index);
                Sleep(20);
            }
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] %s. %s. Get_Display_Queue FAILED.\n", line_number++, Display_thread_name, __func__);
        }

    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] %s. %s. FINISHED.\n", line_number++, Display_thread_name, __func__);
    refresh_operation = OPERATION_COMPLETED;
}

int Get_Display_Queue(int queue_size_request) {
    int status = TRUE;
    char buffer[4] = {0};
    char filename[PATH_MAX] = {0};
    int queue = 0;
    int check_count = 0;
    int sleep_time = 1;
    static byte error_reported = FALSE;
    static byte timer_started = FALSE;
    time_t timer_result = 0;
    static int I2C_speed_changed = FALSE;

    do {
        Sleep(sleep_time++);
        //status = Read_I2C_SGL(6, filename, E_display_addr, 0, buffer, 1);
        status = Display_Read(buffer, 1);
        queue = DISPLAY_MAX_INTERNAL_QUEUE - buffer[0];
    } while (queue < queue_size_request && check_count++ < DISPLAY_MAX_INTERNAL_QUEUE);
    if (check_count >= DISPLAY_MAX_INTERNAL_QUEUE) {
        I2C_Error_Count++;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s. %s. FAILED. queue: %d, check_count: %d, I2C_Error_Count: %d\n",
                line_number++, Display_thread_name, __func__, queue, check_count, I2C_Error_Count);
        if (I2C_speed_changed == FALSE) {
            if (timer_started == FALSE) {
                timer_started = TRUE;
                Elapsed_Time_Seconds(timer_started);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s. %s. FAILED. TIMER STARTED \n",
                        line_number++, Display_thread_name, __func__);
            } else {
                timer_result = Elapsed_Time_Seconds(0);
                if (timer_result > 2 && I2C_Error_Count > 4) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] %s. %s. FAILED. timer_result: %I64d, I2C_Error_Count: %d \n",
                            line_number++, Display_thread_name, __func__, timer_result, I2C_Error_Count);
                    I2C_Error_Count = 0;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] %s. %s. CALLING Initialize_i2c_mode(1) \n",
                            line_number++, Display_thread_name, __func__);
                    timer_started = FALSE;
                    I2C_speed_changed = TRUE;
                } else {
                    if (timer_result > 2 && I2C_Error_Count <= 4) {
                        timer_started = FALSE;
                        I2C_Error_Count = 0;
                    }
                }
            }
        }
        check_queue_failed = TRUE;
        status = FALSE;
    }
    /*else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s. %s. queue: %d \n", line_number++, Display_thread_name, __func__, queue);
    }*/
    return status;
}

void Display_freq_queue_add(uint32_t command) {
    int token_status = 0;

    if (G_Display_attached) {
        token_status = pthread_mutex_trylock(&Display_Queue_Token_available);
        if (token_status != EBUSY) {
            E_Display_queue_busy = TRUE;
            if (E_Display_freq_queue_front == (E_Display_freq_queue_rear + 1) % MAX_COMMAND_QUEUE) {
                E_Display_queue_count = 0;
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Display_freq_queue_add. QUEUE OVERFLOW \n", line_number++);
            } else {
                if (E_Display_freq_queue_front == -1) {
                    E_Display_freq_queue_front = E_Display_freq_queue_rear = 0;
                } else {
                    E_Display_freq_queue_rear = (E_Display_freq_queue_rear + 1) % MAX_COMMAND_QUEUE;
                }
                E_Display_freq_queue[E_Display_freq_queue_rear] = command;
                //print_time(0);
                //fprintf(G_fp_logfile, "[%d] Display_freq_queue_add. command: %d \n", line_number++,command);
                E_Display_queue_count++;
            }
            E_Display_queue_busy = FALSE;
            pthread_mutex_unlock(&Display_Queue_Token_available);
        }
    }
}

uint32_t Display_dequeue_freq() {
    uint32_t ret = 0;
    int token_status = 0;

    token_status = pthread_mutex_trylock(&Display_Queue_Token_available);
    if (token_status != EBUSY) {
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
        pthread_mutex_unlock(&Display_Queue_Token_available);
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Display_dequeue_freq. FAILED. TOKEN LOCKED\n", line_number++);
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

uint8_t display_mode() {
    uint8_t status = 0;
    int display_status = OPERATION_PENDING;

    if ((Get_Display_Queue(12))) {
        display_status = DISPLAY_Position(0, MODE_POSITION, 0);
        if (display_status >= 0) {
            display_status = DISPLAY_PutChar(G_mode);
            if (display_status >= 0) {
                status = OPERATION_COMPLETED;
            }
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s. %s. G_mode: %c\n", line_number++, Display_thread_name, __func__, G_mode);
        saved_mode = G_mode;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s. %s. Get_Display_Queue FAILED.\n", line_number++, Display_thread_name, __func__);
        status = OPERATION_PENDING;
    }

    return status;
}

uint8_t set_buttons(uint8_t button, int index) {
    uint8_t status = OPERATION_PENDING;
    int display_status = 0;

    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] %s. %s. button: %d, index: %d\n", line_number++, Display_thread_name, __func__,button,index);
    if ((Get_Display_Queue(12))) {
        switch (button) {
            case 1:
                display_status = DISPLAY_Position(1, BUTTON_A_POSITION, 0);
                if (display_status >= 0) {
                    display_status = DISPLAY_PrintString(button_Text[index]);
                }
                break;

            case 2:
                display_status = DISPLAY_Position(1, BUTTON_B_POSITION, 0);
                if (display_status >= 0) {
                    display_status = DISPLAY_PrintString(button_Text[index]);
                }
                break;
            case 3:
                display_status = DISPLAY_Position(1, BUTTON_C_POSITION, 0);
                if (display_status >= 0) {
                    display_status = DISPLAY_PrintString(button_Text[index]);
                }
                break;
        }
        if (display_status >= 0) {
            status = OPERATION_COMPLETED;
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s. %s. Get_Display_Queue FAILED.\n", line_number++, Display_thread_name, __func__);
    }
    return status;
}

uint8_t set_star(uint8_t button, uint8_t on_off) {
    uint8_t status = OPERATION_PENDING;
    static uint8_t state = 0;
    int display_status = 0;
    int position = 0;
    int row = 1;

    if ((Get_Display_Queue(12))) {
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
        display_status = DISPLAY_Position(row, position, 0);
        if (display_status >= 0) {
            if (on_off == TRUE) {
                display_status = DISPLAY_PutChar('*');
            } else {
                display_status = DISPLAY_PutChar(' ');
            }
        }
        if (display_status >= 0) {
            status = OPERATION_COMPLETED;
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s. %s. Get_Display_Queue FAILED.\n", line_number++, Display_thread_name, __func__);
    }
    return status;
}

uint8_t ptt(uint8_t on_off) {
    static uint8_t state = 0;
    uint8_t status = OPERATION_COMPLETED;
    int display_status = 0;
    char power[4] = {0};

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Process_Display_thread . ptt. on_off: %d\n", line_number++, on_off);
    if ((Get_Display_Queue(12))) {
        display_status = DISPLAY_Position(0, 0, 0);
        switch (on_off) {
            case 0:
                status = DISPLAY_PrintString(ptt_star[1]);
                break;
            case 1:
                status = DISPLAY_PrintString(ptt_star[0]);
                break;
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s. %s. Get_Display_Queue FAILED.\n", line_number++, Display_thread_name, __func__);
    }
    return status;
}

uint8_t set_step(uint8_t step_value) {
    uint8_t status = OPERATION_COMPLETED;
    int display_status = 0;

    if ((Get_Display_Queue(12))) {
        display_status = DISPLAY_Position(0, STEP_POSITION, 0);
        display_status = DISPLAY_PrintString(step_size[step_value]);
        strcpy(saved_step, step_size[step_value]);
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s. %s. Get_Display_Queue FAILED.\n", line_number++, Display_thread_name, __func__);
    }
    return status;
}

uint8_t s_meter(uint8_t s_value) {
    static uint8_t state = 0;
    uint8_t status = OPERATION_PENDING;
    int display_status = 0;
    static uint32_t s_meter_count = 0;

    if ((Get_Display_Queue(12))) {
        display_status = DISPLAY_Position(0, 0, 1);
        if (display_status >= 0) {
            display_status = DISPLAY_PrintString(s_meter_value[s_value]);
        }
        strcpy(saved_s_meter, s_meter_value[s_value]);
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] s_meter. s_meter_count: %d\n", line_number++, s_meter_count);
    }
    if (display_status >= 0) {
        status = OPERATION_COMPLETED;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s. %s. Get_Display_Queue FAILED.\n", line_number++, Display_thread_name, __func__);
    }
    s_meter_count++;
    return status;
}

void Display_Frequency(unsigned long freq) {
    int status = 0;
    char ascii_freq[17] = {0};
    int token_status = EBUSY;
    int retry_count = 0;
    int test = 1;
    int retry_count_limit = 10;
    while (refresh_operation == OPERATION_PENDING);
    display_operation = OPERATION_PENDING;
    do {
        status = format_periods(freq, ascii_freq);
    } while (status == OPERATION_PENDING);
    memset(saved_ascii_freq, 0, sizeof (saved_ascii_freq));
    strcpy(saved_ascii_freq, ascii_freq);
    if ((Get_Display_Queue(12))) {
        while (retry_count++ < retry_count_limit) {
            status = DISPLAY_Position(0, FREQUENCY_POSITION, 0);
            if (status >= 0) {
                status = DISPLAY_PrintString(ascii_freq);
            } else {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s. %s. DISPLAY_Position. FAILED. status:%d \n",
                        line_number++, Display_thread_name, __func__, status);
            }
            memset(ascii_freq, 0, sizeof (ascii_freq));
            if (status < 0) {
                Sleep(20);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] %s. %s. retry_count: %d \n", line_number++, Display_thread_name, __func__, retry_count);
            } else {
                retry_count = retry_count_limit;
            }
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] %s. %s. Get_Display_Queue FAILED.\n", line_number++, Display_thread_name, __func__);
    }
    display_operation = OPERATION_COMPLETED;
}

void *Process_Display_thread(void *t) {
    uint8_t status = OPERATION_COMPLETED;
    uint32_t freq = 0;
    static uint8_t state = 0;
    static uint8_t previous_mode = 'N';
    static uint8_t previous_buttons = 100;
    static int button_index = 0;
    static uint8_t button = 0;
    static uint8_t previous_star = 0;
    static uint8_t star_status = FALSE;
    static uint8_t previous_step = 100;
    static uint8_t previous_s_meter = 100;
    uint8_t first_pass_clear = FALSE;
    int display_status = 0;
    static uint8_t previous_tx_mode = 0;
    static uint8_t tx_state = FALSE;
    int pass_count = 0;
    int sleep_time = 0;
    int token_status = 0;
    uint8_t test = 1;
    //DWORD dwError = 0;
    byte previous_Set_I2C_Speed = 100;
    long long previous_lock_failed = 0;
    long long lock_failed_diff = 0;
    long long previous_lock_failed_diff = 0;
    long long lock_failed_limit = 350;
    long long lock_failed = 0;
    time_t start_time = 0;
    time_t end_time = 0;

    start_time = time(NULL);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Process_Display_thread. STARTED. Sleeping for 3 seconds\n", line_number++);
    Sleep(3000);
    end_time = time(NULL);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Process_Display_thread. Seconds Delayed: %lld\n", line_number++, (end_time - start_time));
    Display_thread_name = __func__;

    while (G_all_threads_run) {
        if (G_MSCC_Initialized == TRUE) {
            if (first_pass_clear == FALSE) {
                DISPLAY_Init(CLEAR_SCREEN);
                first_pass_clear = TRUE;
            }
            token_status = pthread_mutex_trylock(&Master_Device_Token_available);
            if (token_status == 0) {
                switch (state) {
                    case 0:
                        if (G_TX == FALSE) {
                            if (previous_mode != G_mode) {
                                status = display_mode();
                                if (status == OPERATION_COMPLETED) {
                                    previous_mode = G_mode;
                                    state++;
                                }
                            } else {
                                state++;
                            }
                        } else {
                            state++;
                        }
                        break;
                    case 1:
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
                    case 2:
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
                    case 3:
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
                            switch (button) {
                                case 1:
                                    saved_buttons[0].button = button;
                                    saved_buttons[0].button_index = button_index;
                                    break;
                                case 2:
                                    saved_buttons[1].button = button;
                                    saved_buttons[1].button_index = button_index;
                                    break;
                                case 3:
                                    saved_buttons[2].button = button;
                                    saved_buttons[2].button_index = button_index;
                            }
                            status = set_buttons(button, button_index);
                            if (status == OPERATION_COMPLETED) {
                                previous_buttons = E_buttons;
                                state++;
                            }
                        } else {
                            state++;
                        }
                        break;
                    case 4:
                        if (previous_star != E_star) {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Process_Display_thread . E_star: %x\n", line_number++, E_star);
                            star_status = E_star & 0x01;
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Process_Display_thread . E_star: %x, star_status: %x\n",
                                    line_number++, E_star, star_status);
                            if (star_status == 0x01) {
                                star_status = TRUE;
                            } else {
                                star_status = FALSE;
                            }
                            button = E_star & 0x70;
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Process_Display_thread . E_star: %x, star_status: %x, button: %x\n",
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
                    case 5:
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
                        if (G_TX == FALSE) {
                            if (G_Refresh_Display == TRUE && display_operation == OPERATION_COMPLETED) {
                                Refresh_Display(0);
                                pass_count = 0;
                                G_Refresh_Display = FALSE;
                            }
                        }
                        if (check_queue_failed == TRUE) {
                            Refresh_Display(1);
                            check_queue_failed = FALSE;
                        }
                        if (previous_Set_I2C_Speed != G_Set_I2C_Speed) {
                            previous_Set_I2C_Speed = G_Set_I2C_Speed;
                        }
                        state++;
                        break;
                    case 6:
                        freq = Display_dequeue_freq();
                        if (freq != 0 && E_Display_queue_count < DISPLAY_MAX_QUEUE) {
                            Display_Frequency(freq);
                        } else {
                            if (E_Display_queue_count >= DISPLAY_MAX_QUEUE) {
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] %s. E_Display_queue_count: %d\n",
                                        line_number++, __func__, E_Display_queue_count);
                                while (E_Display_queue_count >= DISPLAY_MAX_QUEUE) {
                                    if (!E_Display_queue_busy) {
                                        freq = Display_dequeue_freq();
                                    }
                                }
                            }
                        }
                        state = 0;
                        break;
                }
                pthread_mutex_unlock(&Master_Device_Token_available);
            } else {
                lock_failed++;
                lock_failed_diff = lock_failed - previous_lock_failed;
                if (lock_failed_diff > LOCK_FAILED_LIMIT) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] %s. lock_failed: %I64d, previouslock_failed: %I64d, lock_failed_diff: %I64d\n",
                            line_number++, __func__, lock_failed, previous_lock_failed, lock_failed_diff);
                }
                previous_lock_failed = lock_failed;
                previous_lock_failed_diff = lock_failed_diff;
            }
        }
        Sleep(Get_random_time());
    }
    DISPLAY_Init(RESET_DISPLAY);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] %s. NORMAL EXIT  \n", line_number++, __func__);
    pthread_exit(0);
    return NULL;
}
/* [] END OF FILE */


