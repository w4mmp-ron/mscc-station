#include <math.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <ShlObj.h>
#include <KnownFolders.h>
#include <limits.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/timeb.h>
//#include <KnownFolders.h>


#include "usbavrcmd.h"
#include "lusb0_usb.h"
#include "SRDLL.h"
#include "extern.h"
#include "version.h"
#include "band_stack.h"
#include "last_used.h"
#include "iq.h"
//Pthreads

#include "pthread.h"
#include "sched.h"
#include "semaphore.h"
//#include <wiringPi.h

#pragma warning(disable : 4996)

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif
//#define CONSOLEWND	1	// set to 1 if the console window for capturing _cprintf() output is needed

typedef enum {
    extHw_modeRX = 0
    , extHw_modeTX = 1
} extHw_ModeRxTxT;

uint32_t line_number = 0;

BOOL hasconsole = FALSE;

#define MULTUS_VID 0x16C0
#define MULTUS_PID 0x05DC
#define MULTUS_MAJOR_VERSION_119 119
#define MULTUS_MINOR_VERSION_81 81
#define MULTUS_MAJOR_VERSION_SOLIDUS 121
#define MULTUS_MINOR_VERSION_SOLIDUS 1
//#define SOCKET_ERROR -1
//#define INVALID_SOCKET -1
#define KEY 8
#define KEY_DOWN 0
#define KEY_UP 1


FILE *G_fp_logfile;

char* home;
const char *homedir;
char G_l_path[MAX_PATH] = { 0 };
TCHAR G_serial_number[80] = {0};
char G_Multus_serial_number[80] = {0};
char G_Usb_serial_number[80] = {0};
uint32_t G_tune_freq;
uint8_t G_serial_number_success = 0;
uint8_t G_cw_firmware_option = 0;
int G_Hw_Started = TRUE;
char G_mode = 'U';
static char G_previous_mode;
int G_SetModeRxTX = FALSE;
int G_network_initialized = FALSE;
int G_dll_active = FALSE;
int G_transceiver_initialization_status = TRUE;
//uint16_t G_band_for_power = 0;
uint8_t G_setting_power_band = 0;
uint16_t G_previous_power_band = 0;
uint16_t G_prvious_power_level = 0;
uint16_t G_gui_port = 0;
uint16_t G_ms_sdr_port = 0;
uint16_t G_sdrcore_recv_port = 0;
uint16_t G_sdrcore_trans_port = 0;
uint16_t G_spectrum_port = 0;
uint16_t G_waterfall_port = 0;
uint16_t G_RPI_mscc_port = 0;
char G_gui_IP[MAX_PATH];
char G_ms_sdr_IP[MAX_PATH];
char G_RPI_IP[MAX_PATH];
uint16_t G_band_normal = 0;
uint8_t G_pcb_version = 0;
int G_previous_high_cut = 3000;
uint8_t G_gui_running = 0;
uint8_t G_all_threads_run = 1;
uint8_t G_main_thread_run = 1;
ULONG32 G_thread_sleep_time = THREAD_SLEEP_TIME;
uint8_t G_updating_last_used = 0;
uint8_t G_setting_last_used = 0;
uint8_t G_sdr_core_iterations = 0;
uint8_t G_calibration_mode = 0;
uint8_t G_in_IQ_calibration_mode = 0;
uint8_t G_Speaker_Mute_Status = 0;
ULONG64 G_Heart_beat = 0;
uint8_t G_delete_SDRcore_init_files = 0;
uint8_t G_delete_Comm_port_init_file = 0;
uint8_t G_Message_Box_Displayed = 0;
uint8_t G_Rit_Status = 0;
uint32_t G_Rit_Offset = 0;
uint8_t G_Rit_Step = 0;
uint8_t G_CW_Snap_Running = 0;
int G_CW_Offset = -600;
INT8 G_int = 0;
INT8 G_dec = 0;
int G_Tune_Power = 0;
uint8_t G_TX = 0;
uint8_t G_TX_Inhibit = FALSE;
int G_High_Power_Count = 0;
uint8_t G_High_Power_Toggle = 0;
uint8_t G_Tune = FALSE;
uint8_t G_PTT = FALSE;
uint8_t G_Allow_Log_Write = TRUE;

char G_log_file_name[MAX_PATH] = {0};
int G_comm_port_status = FALSE;
uint16_t G_subsys_initialized = 0x0000;
int G_Key_Up_Threshold = 0;
int ret = 0;

struct band_stack G_band_stack[10];
struct last_used_stack G_last_used_stack[10];

//For the Get_Key_Down_Status
pthread_t G_Get_Key_Down_Status_thread;
int G_Get_Key_Down_Status_rc;
int G_key;
int G_Get_Key_Down_Status_Thread_Running = 0;

//For the CW_Command_Interface_thread
pthread_t G_CW_Command_Interface_thread;
int G_CW_Command_Interface_rc;

//For the flusher thread
pthread_t G_Flusher_thread;
int G_Flusher_thread_rc;

//For the last used updater thread
pthread_t G_Update_last_used_thread;
int G_Udate_last_used_thread_rc;

pthread_t G_Check_temperature_thread;
int G_Check_temperature_rc;

pthread_t G_Power_thread;
int G_Check_Power_rc;

pthread_t G_Bias_thread;
int G_Bias_rc;

pthread_t G_Freq_Queue_thread;
int G_Freq_Queue_rc;

pthread_t G_Master_controller_thread;
int G_Master_controller_rc;

pthread_t G_Meter_main_thread;
int G_Meter_main_rc;

pthread_t G_IQBD_thread;
int G_IQBD_rc;

pthread_t G_CW_thread;
int G_CW_rc;

pthread_t G_MFC_thread;
int G_MFC_rc;

pthread_t G_PWM_thread;
int G_PWM_rc;

pthread_t G_Display_thread;
int G_Display_rc;

extern struct sockaddr_in si_me;
extern struct sockaddr_in si_other;
extern struct sockaddr_in si_gui;
extern struct sockaddr_in si_RPI_mscc;
extern int dll_s;
extern int gui_s;
extern int RPI_mscc_s;
extern int recv_len;

//extern int usleep(__useconds_t __useconds);

void signal_callback_handler(int signum) {
    print_time(0);
    fprintf(G_fp_logfile, "[%d] signal_callback_handler -> signal: %d\n", line_number++, signum);
    pthread_exit(NULL);
    //exit(signum);
}

void sig_term_handler(int signum) {
    print_time(0);
    fprintf(G_fp_logfile, "[%d] sig_term_handler -> Calling stop_all. signal: %d\n", line_number++, signum);
    Stop_all(1, 0);
}

char *My_getenv(char *myenv) {

    //FILE* Multus_ini;
    WCHAR path[MAX_PATH];
    PWSTR lpPath = path;
    int lenght = 0;

    memset(G_l_path, 0, sizeof(G_l_path));
    
    HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &lpPath);
    //fprintf("get_serial_number -> Get folder path \n");
    if (SUCCEEDED(hr)) {
        WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpPath, -1, G_l_path, sizeof(G_l_path), NULL, NULL);
        strcat(G_l_path, "/MSCC-PW");
        //printf("getenv: %s\n", G_l_path);
    }
    //$HOME = G_l_path;
    return G_l_path;
}
char mode_to_letter(int number) {
    char mode = 'N';

    switch (number) {
        case MODE_AM:
            mode = 'A';
            break;
        case MODE_LSB:
            mode = 'L';
            break;
        case MODE_USB:
            mode = 'U';
            break;
        case MODE_CW:
            mode = 'C';
            break;
        case MODE_TUNE:
            mode = 'T';
            break;
        case 5:
            mode = 'E';
            break;
        case 6:
            mode = 'D';
            break;
    }
    return mode;
}

int mode_to_number(char mode) {
    int mode_number = 9;

    switch (mode) {
        case MODE_AM_LETTER:
            mode_number = MODE_AM;
            break;
        case MODE_LSB_LETTER:
            mode_number = MODE_LSB;
            break;
        case MODE_USB_LETTER:
            mode_number = MODE_USB;
            break;
        case MODE_CW_LETTER:
            mode_number = MODE_CW;
            break;
        case MODE_TUNE_LETTER:
            mode_number = MODE_TUNE;
            break;
    }
    return mode_number;
}

int Open_log_file() {
    FILE *log_file_dir;
    char log_file__dir_name[MAX_PATH] = {0};
    char log_file_record[MAX_PATH] = {0};

    struct {
        char *start;
        char *end;
        int size;
    } log_file_dir_field;
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(log_file__dir_name, homedir);
        strcat(log_file__dir_name, "/.local/share/mscc/log_file_dir.ini");
        log_file_dir = fopen(log_file__dir_name, "r");
        if (log_file_dir != NULL) {
            fgets(log_file_record, sizeof (log_file_record), log_file_dir);
            log_file_dir_field.start = strstr(log_file_record, "LOGFILE_DIRECTORY");
            log_file_dir_field.start = log_file_dir_field.start + 18;
            //Now find the ending point
            log_file_dir_field.end = strstr(log_file_dir_field.start, ";");
            log_file_dir_field.size = (log_file_dir_field.end - (log_file_dir_field.start));
            strcpy(G_log_file_name, homedir);
            strncat(G_log_file_name, log_file_dir_field.start, log_file_dir_field.size);
            strcat(G_log_file_name, "/ms-sdr.log");
            printf("[%d] Open_log_file -> User File Name: %s\n", line_number++, G_log_file_name);
            fclose(log_file_dir);
        }
        G_fp_logfile = fopen(G_log_file_name, "w");
        if (G_fp_logfile == NULL) {
            printf("[%d] Open_log_file -> File Open Failed: %s, error: %s\n", line_number++, G_log_file_name,strerror(errno));
            return 0;
        }
        printf("[%d] Open_log_file -> Finished\n", line_number++);
        fprintf(G_fp_logfile, "[%d] Logfile Opened.  Logfile: %s\n", line_number++, G_log_file_name);
    }
    return 1;
}

void Reset_Logfile() {
    time_t tim;
    struct tm *local_time;
    static byte log_reset = FALSE;

    tim = time(NULL);
    local_time = localtime(&tim);
    if (local_time->tm_hour == 0 && log_reset == FALSE) {
        G_Allow_Log_Write = FALSE;
        log_reset = TRUE;
        line_number = 0;
        fflush(G_fp_logfile);
        fclose(G_fp_logfile);
        Open_log_file();
        G_Allow_Log_Write = TRUE;
    }
    if (local_time->tm_hour != 0 && log_reset == TRUE) {
        log_reset = FALSE;
    }
}

void print_time(int print_new_line) {
    time_t tim;
    //unsigned long usec;
    tim = time(NULL);
    struct tm *now;
    struct timeb my_start;
    unsigned short mills = 0;

    if (G_Allow_Log_Write == TRUE) {
        now = gmtime(&tim);
        ftime(&my_start);
        mills = my_start.millitm;
        if (print_new_line == 1)fprintf(G_fp_logfile, "\n");
        fprintf(G_fp_logfile, "[%02d:%02d:%02d:%02d:%03lu]", now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, mills);
    }
}

void *Flusher_thread(void *t) {
    int gui_count = 0;
    int servers_count = 0;
    static ULONG64 previous_heart_beat = 0;
    static ULONG64 previous_sdrcore_recv_keep_alive = 0;
    static ULONG64 previous_sdrcore_trans_keep_alive = 0;
    int servers_failed = 0;
    //char buf[50];
    //int slen = sizeof (si_gui);
    uint8_t op_code;
    int op_data;
    //static uint8_t gui_initialized = 0;
    //byte iq_extended_data[4];
    int comm_port_status = FALSE;
    byte comm_port_retry = 0;
    int thread_status = 2;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Flusher_thread -> thread started \n", line_number++);
    op_code = CMD_CHECK_GUI_STATUS;
    op_data = 1;
    while (G_all_threads_run) {
        Reset_Logfile();
        if (G_Allow_Log_Write == TRUE) {
            fflush(G_fp_logfile);
            Sleep(3000);
            if (G_network_initialized) {
                SDRcore_recv_send_param(CMD_SET_KEEP_ALIVE, 1);
                SDRcore_trans_send_param(CMD_SET_KEEP_ALIVE, 1);
                if (comm_port_status == FALSE && comm_port_retry < 3) {
                    //comm_port_status = set_comm_port_ini();
                    if (comm_port_status == FALSE) {
                        comm_port_retry++;
                        if (comm_port_retry > 2) {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Flusher_thread -> set_comm_port_ini FAILED after retry count:%d  \n",
                                    line_number++, comm_port_retry);
                        }
                    }
                }
            }
            if (G_MSCC_Initialized == 1) { //G_MSCC_Initialized
                Gui_send_param(CMD_SET_KEEP_ALIVE, 1);
                if (gui_count++ >= 6) {
                    if (G_Heart_beat > previous_heart_beat) {
                        previous_heart_beat = G_Heart_beat;
                    } else {
                        servers_failed = 1;
                    }
                    gui_count = 0;
                }
            }
            if (servers_count++ >= 6) {
                if (G_sdrcore_recv_keep_alive > previous_sdrcore_recv_keep_alive) {
                    previous_sdrcore_recv_keep_alive = G_sdrcore_recv_keep_alive;
                } else {
                    servers_failed = 2;
                }
                if (G_sdrcore_trans_keep_alive > previous_sdrcore_trans_keep_alive) {
                    previous_sdrcore_trans_keep_alive = G_sdrcore_trans_keep_alive;
                } else {
                    servers_failed = 3;
                }
                servers_count = 0;
            }
            if (servers_failed > 0) {
                print_time(0);
                switch (servers_failed) {
                    case 1:
                        fprintf(G_fp_logfile, "[%d] Flusher_thread -> MSCC Client FAILED: %d\n",
                                line_number++, servers_failed);
                        break;
                    case 2:
                        fprintf(G_fp_logfile, "[%d] Flusher_thread -> Sdrcore-recv FAILED: %d\n",
                                line_number++, servers_failed);
                        //Gui_Send_Message("NO RESPONSE FROM Sdrcore-recv\n MSCC WILL NOW STOP");
                        break;
                    case 3:
                        fprintf(G_fp_logfile, "[%d] Flusher_thread -> Sdrcore-trans FAILED: %d\n",
                                line_number++, servers_failed);
                        //Gui_Send_Message("NO RESPONSE FROM Sdrcore-trans\n MSCC WILL NOW STOP");
                        break;
                }
                Sleep(5000);
                Stop_all(1, 0);
            }
            /*if (G_MSCC_Initialized == TRUE) {
                if (thread_status == 2) {
                    thread_status = Check_Threads();
                    if (thread_status < 0) {
                        Stop_all(1, 0);
                    }
                }
            }*/
            /*if (G_TX == TRUE && G_High_Power_Toggle == FALSE && G_high_power_flag == TRUE) {
                G_High_Power_Count = 0;
                G_High_Power_Toggle = TRUE;
            } else {
                if (G_TX == TRUE && G_High_Power_Toggle == TRUE && G_high_power_flag == TRUE) {
                    G_High_Power_Count++;
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Flusher_thread -> G_High_Power_Count: %d \n",
                            line_number++, G_High_Power_Count);
                } else {
                    if (G_TX == FALSE && G_High_Power_Toggle == TRUE && G_high_power_flag == FALSE) {
                        G_High_Power_Count = 0;
                        G_High_Power_Toggle = FALSE;
                    }
                }
            }*/
        }
    }
    print_time(0);
    if (servers_failed <= 0) {
        fprintf(G_fp_logfile, "[%d] Flusher_thread -> Normal Exit \n", line_number++);
    } else {
        fprintf(G_fp_logfile, "[%d] Flusher_thread -> ABNORMAL Exit \n", line_number++);
    }
    fflush(G_fp_logfile);
    pthread_exit(0);
    return NULL;
}

void *Update_last_used_thread(void *last_used_tread) {
    int status = 0;
    uint32_t previous_startup_freq = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_last_used_thread -> Started\n", line_number++);
    Sleep(3000); //Wait until the remaining subsystems are initialized
    while (G_all_threads_run) {
        if (!G_dll_active && G_MSCC_Initialized == 1) {
            if (!G_setting_power_band) {
                switch (G_vfo) {
                    case VFO_A:
                        status = Update_last_used_VFO_A();
                        break;
                    case VFO_B:
                        status = Update_last_used_VFO_B();
                        break;
                }
                if (status != 0) {
                    if (G_tune_freq != 0) {
                        if (previous_startup_freq != G_tune_freq && G_TX == FALSE) {
                            if (G_vfo == VFO_A) {
                                Update_Startup(G_tune_freq, G_mode);
                                previous_startup_freq = G_tune_freq;
                            }
                        }
                    }
                }
            }
        }
        Sleep(G_thread_sleep_time);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_last_used_thread -> Normal Exit.\n", line_number++);
    pthread_exit(0);
    return NULL;
}

/*int Open_log_file() {
    //char file_name[PATH_MAX] = {0};
    int lenght = 0;
    FILE *log_file_dir;
    char log_file__dir_name[PATH_MAX] = {0};
    char log_file_record[PATH_MAX] = {0};

    struct {
        char *start;
        char *end;
        int size;
    } log_file_dir_field;
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(log_file__dir_name, homedir);
        strcat(log_file__dir_name, "/.local/share/mscc/log_file_dir.ini");
        log_file_dir = fopen(log_file__dir_name, "r");
        if (log_file_dir != NULL) {
            fgets(log_file_record, sizeof (log_file_record), log_file_dir);
            log_file_dir_field.start = strstr(log_file_record, "LOGFILE_DIRECTORY");
            log_file_dir_field.start = log_file_dir_field.start + 18;
            //Now find the ending point
            log_file_dir_field.end = strstr(log_file_dir_field.start, ";");
            log_file_dir_field.size = (log_file_dir_field.end - (log_file_dir_field.start));
            strcpy(G_log_file_name, homedir);
            strncat(G_log_file_name, log_file_dir_field.start, log_file_dir_field.size);
            strcat(G_log_file_name, "/ms-sdr.log");
            printf("[%d] Open_log_file -> User File Name: %s\n", line_number++, G_log_file_name);
            fclose(log_file_dir);
        }
        G_fp_logfile = fopen(G_log_file_name, "w");
        if (G_fp_logfile == NULL) {
            printf("[%d] Open_log_file -> File Open Failed: %s\n", line_number++, G_log_file_name);
            return 0;
        }
        printf("[%d] Open_log_file -> Finished\n", line_number++);
        fprintf(G_fp_logfile, "[%d] Logfile Opened.  Logfile: %s\n", line_number++, G_log_file_name);
    }
    return 1;
}*/

int Get_serial_number() {
    FILE *Multus_ini;
    char file_name[MAX_PATH] = {0};
    int length = 0;
    //int cmd_value;
    char *parameter;
    char *ptr;
    char temp_serial_number[MAX_PATH];
    char record[MAX_PATH];
    int status = 1;

    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Get_serial_number -> Default Path: %s\n", line_number++, homedir);
        strcat(file_name, homedir);
        strcat(file_name, "/.local/share/mscc/Multus_mscc.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Get_serial_number -> Multus.ini Path: %s\n", line_number++, file_name);
        Multus_ini = fopen(file_name, "r");
        if (Multus_ini != NULL) {
            fgets(record, sizeof (record), Multus_ini);
            parameter = strstr(record, "PROFICIO_SERIAL_NUMBER=");
            if (parameter != NULL) {
                length = strlen("PROFICIO_SERIAL_NUMBER=");
                strcpy(temp_serial_number, &parameter[length]);
                ptr = strtok(temp_serial_number, ";");
                strcpy(G_Multus_serial_number, ptr);
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Get_serial_number -> Proficio Serial Number: %s\n", line_number++, G_Multus_serial_number);
            }
            fclose(Multus_ini);
        } else {
            status = 0;
        }
    } else {
        status = 0;
    }
    return status;
}

void Set_band_normal(uint32_t freq, uint8_t apply_offset) {
    uint32_t offset = 0;

    if (apply_offset) {
        offset = 12000;
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Set_band_normal -> Frequency: %ld, Offset: %ld \n", line_number++, freq, offset);
    G_dll_active = 1;
    G_band_normal = FREQ_OUT_OF_RANGE;
    if (freq >= 500000 - offset && freq <= 30000000 + offset) G_band_normal = GENERAL_BAND;
    if (freq >= (1800000 - offset) && freq <= (2000000 + offset)) G_band_normal = 160;
    if (freq >= 3500000 - offset && freq <= 4000000 + offset) G_band_normal = 80;
    if (freq >= 5330500 - offset && freq <= 5403500 + offset) G_band_normal = 60;
    if (freq >= 7000000 - offset && freq <= 7300000 + offset) G_band_normal = 40;
    if (freq >= 10100000 - offset && freq <= 10150000 + offset) G_band_normal = 30;
    if (freq >= 14000000 - offset && freq <= 14350000 + offset) G_band_normal = 20;
    if (freq >= 18068000 - offset && freq <= 18168000 + offset) G_band_normal = 17;
    if (freq >= 21000000 - offset && freq <= 21450000 + offset) G_band_normal = 15;
    if (freq >= 24890000 - offset && freq <= 24990000 + offset) G_band_normal = 12;
    if (freq >= 28000000 - offset && freq <= 29700000 + offset) G_band_normal = 10;
    if (freq >= 135700 - offset && freq <= 137800 + offset) G_band_normal = 2200;
    if (freq >= 472000 - offset && freq <= 479000 + offset) G_band_normal = 630;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Set_band_normal -> Finished -> Band: %ld\n", line_number++, G_band_normal);
    G_dll_active = 0;
}

void Set_PTT(int on_off) {

    if (G_TX_Inhibit == FALSE) {
        G_dll_active = TRUE;
        //print_time(0);
        //fprintf(G_fp_logfile, "[%d] Set_PTT called -> on_off: %d\n", line_number++, on_off);
        G_PTT = on_off;
        SetModeRxTx(on_off, 0);
        Spectrum_Waterfall_send_param(CMD_SET_TX_ON, on_off);
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Set_PTT -> G_TX_Inhibit: %d\n", line_number++, G_TX_Inhibit);
    }
    G_dll_active = FALSE;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Set_PTT was CALLED. FINISHED. Status: %d\n", line_number++, on_off);
}

int Tune_button(int on_off) {

    if (G_TX_Inhibit == FALSE && G_check_bias == FALSE) {
        G_dll_active = TRUE;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Tune_button called -> on_off: %d, previous_mode: %c,G_mode: %c\n",
                line_number++, on_off, G_previous_mode, G_mode);
        G_Tune = on_off;
        SetModeRxTx(on_off, 0);
        Spectrum_Waterfall_send_param(CMD_SET_TX_ON, on_off);
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Tune_button -> G_TX_Inhibit: %d, G_check_bias: %d\n", line_number++, G_TX_Inhibit, G_check_bias);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Tune_button -> Finished. Status: %d\n", line_number++, on_off);
    G_dll_active = FALSE;
    return on_off;
}

int TX_Hold_Timer(int period, int reset) {
    int status = FALSE;
    static int end = 0;
    static int counter = 0;
    static int key_up_count = 0;
    static int threshold = 0;

    if (reset) {
        counter = period * 100;
        end = 0;
        key_up_count = 0;
        threshold = G_Key_Up_Threshold * 5; //G_Key_Up_Threshold is one element time. Wait five elements
    } else {
        ++end;
        if (end >= counter) {
            ++key_up_count;
        }
        if (end >= counter && key_up_count >= threshold) {
            status = TRUE;
        }
    }
    return status;
}

void *Get_Key_Down_Status(void *my_parm) {
    int r = 0;
    static int tx_mode = FALSE;
    int key = FALSE;
    int timer_expired = 0;
    int sleep_time = 100;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Get_Key_Down_Status called -> In Semi Break-in mode -> Polling for key down.\n",
            line_number++);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Get_Key_Down_Status called -> G_Key_Up_Threshold: %d\n",
            line_number++, G_Key_Up_Threshold);
    while (G_Get_Key_Down_Status_Thread_Running) {
        if (G_TX_Inhibit == FALSE) {
            //key = digitalRead(KEY);
            if (key == KEY_DOWN) {
                timer_expired = TX_Hold_Timer(G_TX_Hold, TRUE);
                if (!tx_mode) {
                    SetModeRxTx(1, 1);
                    SDRcore_recv_send_param(CMD_SET_SPEAKER_MUTE, 1);
                    SDRcore_trans_send_param(CMD_SET_TX_ON, 1);
                    SDRcore_recv_send_param(CMD_SET_TX_ON, 1);
                    Spectrum_Waterfall_send_param(CMD_SET_TX_ON, 1);
                    tx_mode = TRUE;
                    Gui_send_param(CMD_SET_TX_SET_BY_SERVER, tx_mode);
                    sleep_time = 1000;
                }
            } else {
                timer_expired = TX_Hold_Timer(G_TX_Hold, FALSE);
                if (tx_mode) {
                    if (timer_expired) {
                        system("pkill --signal SIGIO -x solidus-cw");
                        SetModeRxTx(0, 1);
                        SDRcore_trans_send_param(CMD_SET_TX_ON, 0);
                        SDRcore_recv_send_param(CMD_SET_TX_ON, 0);
                        Spectrum_Waterfall_send_param(CMD_SET_TX_ON, 0);
                        if (G_Speaker_Mute_Status != MUTED) {
                            SDRcore_recv_send_param(CMD_SET_SPEAKER_MUTE, 0);
                        }
                        tx_mode = FALSE;
                        Gui_send_param(CMD_SET_TX_SET_BY_SERVER, tx_mode);
                        sleep_time = 102;
                    }
                }
            }
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Get_Key_Down_Status -> G_TX_Inhibit: %d\n", line_number++, G_TX_Inhibit);
        }
        Sleep(sleep_time);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Get_Key_Down_Status Normal Exit ->  key: %d\n",
            line_number++, key);
    pthread_exit(NULL);
    return (NULL);
}

/*int SetModeRxTx(int transmit, int tx_delay) {
    int key = 0;
    int ret = TRUE;
    uint8_t delay_time = 4; //The T/R relays have a 3ms time to activate

    print_time(1);
    fprintf(G_fp_logfile, "[%d] SetModeRxTx Called -> transmit %d \n", line_number++, transmit);
    G_SetModeRxTX = transmit;
    G_dll_active = TRUE;
    if (G_gui_running) {
        if (transmit == 1) {
            G_TX = TRUE;
            if (tx_delay == 0) {
                Sleep(delay_time); //Allow the Solidus T/R relays to activate
                print_time(0);
                fprintf(G_fp_logfile, "[%d] SetModeRxTx -> Switch delay: %d\n",
                        line_number++, delay_time);
            }
            freq_queue_add(G_tune_freq);
            ret = srSetPTTGetCWInp(transmit, &key);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] SetModeRxTx -> srSetPTTGetCWinp Called -> Transmit: %d -> key: %d, ret: %d\n",
                    line_number++, transmit, key, ret);
        } else {
            ret = srSetPTTGetCWInp(transmit, &key);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] SetModeRxTx -> srSetPTTGetCWinp Called -> Transmit: %d -> key: %d, ret: %d\n",
                    line_number++, transmit, key, ret);
            freq_queue_add(G_tune_freq);
            if (tx_delay == 0) {
                Sleep(delay_time); //Allow time for Proficio to turn of TX
                print_time(0);
                fprintf(G_fp_logfile, "[%d] SetModeRxTx -> Switch delay: %d\n",
                        line_number++, delay_time);
            }
            G_TX = FALSE;
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] SetModeRxTx -> GUI NOT Running.\n", line_number++);
    }
    if (ret == FALSE) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] SetModeRxTx -> FAILED key: %d, ret: %d. Calling Stop_all\n",
                line_number++, key, ret);
        Stop_all(0, STOP_PROFICIO_COMMS);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] SetModeRxTx Finished.\n", line_number++);
    G_dll_active = FALSE;
    return TRUE;
}*/

#define WAIT_LIMIT 20

int SetModeRxTx(int transmit, int tx_delay) {
    int key = 0;
    int ret = TRUE;
    int delay_time = 1;
    int wait_count = 0;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] SetModeRxTx Called -> transmit %d \n", line_number++, transmit);
    G_SetModeRxTX = transmit;
    G_dll_active = TRUE;
    if (G_gui_running) {
        if (transmit == 1) {
            G_TX = TRUE;
            if (G_QRP == TRUE) { //G_QRP in inverted.  If G_QRP == true,  we are in QRO mode
                //while (G_Solidus_TX_Relay_ON == FALSE && wait_count++ < WAIT_LIMIT) { //Wait until Master_controller has turned ON the Solidus TX Relay
                //    Sleep(delay_time);
                //}
                if (wait_count >= WAIT_LIMIT) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] SetModeRxTx -> wait_count exceeded WAIT_LIMIT : %d\n",
                            line_number++, wait_count);
                    Stop_all(0, STOP_MFC);
                }
                Sleep(delay_time); //Sleep additional delay to be certain the relay in ON
            }
            if (G_Split == FALSE) {
                freq_queue_add(G_tune_freq);
            } else {
                freq_queue_add(G_Split_TX_FREQ);
            }
            ret = srSetPTTGetCWInp(transmit, &key);
            //system("pkill --signal SIGURG -x solidus-cw");
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] SetModeRxTx -> srSetPTTGetCWinp Called -> Transmit: %d -> key: %d, ret: %d\n",
            //        line_number++, transmit, key, ret);
        } else {
            ret = srSetPTTGetCWInp(transmit, &key);
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] SetModeRxTx -> srSetPTTGetCWinp Called -> Transmit: %d -> key: %d, ret: %d\n",
            //        line_number++, transmit, key, ret);
            G_TX = FALSE;
            if (G_QRP == TRUE) { //G_QRP in inverted.  If G_QRP == true,  we are in QRO mode
                //while (G_Solidus_TX_Relay_ON == TRUE && wait_count++ < WAIT_LIMIT) { //Wait until Master_controller has turned OFF the Solidus TX Relay
                //    Sleep(delay_time);
                //}
                if (wait_count >= WAIT_LIMIT) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] SetModeRxTx -> wait_count exceeded WAIT_LIMIT : %d\n",
                            line_number++, wait_count);
                    Stop_all(0, STOP_MFC);
                }
                Sleep(delay_time); //Sleep additional delay to be certain the relay in OFF
                //system("pkill --signal SIGURG -x solidus-cw");
            }
            if (G_Split == FALSE) {
                freq_queue_add(G_tune_freq);
            } else {
                freq_queue_add(G_Split_RX_FREQ);
            }
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] SetModeRxTx -> GUI NOT Running.\n", line_number++);
    }
    if (ret == FALSE) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] SetModeRxTx -> FAILED key: %d, ret: %d. Calling Stop_all\n",
                line_number++, key, ret);
        Stop_all(0, STOP_PROFICIO_COMMS);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] SetModeRxTx. FINISHED. wait_count: %d\n", line_number++, wait_count);
    G_dll_active = FALSE;
    return TRUE;
}

int SetHWLO_queue(long tune_freq) {
    double offset_lo = 0.0;
    int status = 0;
    static long previous_freq = 0;
    long delta_freq = 0;
    byte mute_active = FALSE;
    int set_last_used_status = FALSE;
    static uint16_t previous_band = FREQ_OUT_OF_RANGE;

    delta_freq = (labs) ((previous_freq - tune_freq));
    G_dll_active = TRUE;
    previous_freq = tune_freq;
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] SetHWLO_queue -> Called -> tune_freq %ld, G_delta_drift: %d\n",
    //        line_number++, tune_freq, G_delta_drift_int);
    switch (G_mode) {
        case 'A':
            if (G_SetModeRxTX) {
                offset_lo = (double) (tune_freq + AM_OFFSET_TX + G_delta_drift_int);
                //print_time(0);
                //fprintf(G_fp_logfile, "[%d] SetHWLO_queue -> Mode A -> TX mode: %d, tune_freq: %ld, Offset: %f \n", line_number++, G_SetModeRxTX,
                //        tune_freq, offset_lo);
            } else {
                offset_lo = (double) (tune_freq + AM_OFFSET_RX + G_delta_drift_int);
                //print_time(0);
                //fprintf(G_fp_logfile, "[%d] SetHWLO_queue -> Mode A -> TX mode: %d, tune_freq: %ld, Offset: %f \n",
                //        line_number++, G_SetModeRxTX, tune_freq, offset_lo);
            }
            break;
        case 'C':
            if (G_SetModeRxTX) {
                offset_lo = (double) (tune_freq + SSB_OFFSET + G_delta_drift_int);
            } else {
                offset_lo = (double) (tune_freq + SSB_OFFSET + (G_CW_Offset) + G_delta_drift_int);
            }
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] SetHWLO_queue -> Mode C -> TX mode: %d, tune_freq: %ld, Offset: %f \n",
            //        line_number++, G_SetModeRxTX, tune_freq, offset_lo);
            break;
        default:
            offset_lo = (double) (tune_freq + SSB_OFFSET + G_delta_drift_int);
            //print_time(0);
            //fprintf(G_fp_logfile, "[%d] SetHWLO_queue -> Mode SSB -> TX mode: %d, tune_freq: %ld, Offset: %f \n",
            //        line_number++, G_SetModeRxTX, tune_freq, offset_lo);
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] SetHWLO_queue -> Calling srSetFreq with Si5351 Frequency: %f \n", line_number++, offset_lo);
    //Mute the Speaker when the frequency change is 100000Hz or greater. Prevents pop noise cause by PLL reset of the Si5351
    if (delta_freq >= 100000) {
        if (!G_Speaker_Mute_Status) {
            mute_active = TRUE;
            //SDRcore_recv_send_param(CMD_SET_SPEAKER_MUTE, 1);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] SetHWLO_queue -> CMD_SET_SPEAKER_MUTE -> mute_active: %d, Delta Freq %ld\n",
                    line_number++, mute_active, delta_freq);
        }
    }
    status = srSetFreq(offset_lo, 27);
    if (status == FALSE) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] SetHWLO_queue. srSetFreq -> FAILED.  status: %d. Calling Stop_all\n",
                line_number++, status);
        Stop_all(0, STOP_PROFICIO_COMMS);
    }
    if (mute_active) {
        mute_active = FALSE;
        //SDRcore_recv_send_param(CMD_SET_SPEAKER_MUTE, 0);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] SetHWLO_queue -> CMD_SET_SPEAKER_MUTE -> mute_active: %d, Delta Freq %ld\n",
                line_number++, mute_active, delta_freq);
    }
    G_dll_active = FALSE;
    Set_band_normal(G_tune_freq, TRUE);
    if (previous_band != G_band_normal) {
        SDRcore_trans_send_param(CMD_SET_SDR_CORE_BAND, G_band_normal);
        SDRcore_recv_send_param(CMD_SET_SDR_CORE_BAND, G_band_normal);
        previous_band = G_band_normal;
    }
    if (G_band_normal != GENERAL_BAND) {
        switch (G_vfo) {
            case VFO_A:
                set_last_used_status = Set_last_used_VFO_A(G_tune_freq, G_mode);
                if (set_last_used_status == FALSE) { // Try again.  Update_Last_Used is busy
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] SetHWLO_queue -> set_last_used-> BUSY. set_last_used_status: %d \n",
                            line_number++, set_last_used_status);
                    Sleep(100);
                    set_last_used_status = Set_last_used_VFO_A(G_tune_freq, G_mode);
                }
                break;
            case VFO_B:
                set_last_used_status = Set_last_used_VFO_B(G_tune_freq, G_mode);
                if (set_last_used_status == FALSE) { // Try again.  Update_Last_Used is busy
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] SetHWLO_queue -> set_last_used-> BUSY. set_last_used_status: %d \n",
                            line_number++, set_last_used_status);
                    Sleep(100);
                    set_last_used_status = Set_last_used_VFO_B(G_tune_freq, G_mode);
                }
        }
    }
    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] SetHWLO_queue -> Finished-> tune_freq: %ld, offset_lo: %f,  status: %d \n",
    //        line_number++, tune_freq, offset_lo, status);
    return status;
}

long GetHWLO(void) {
    //The return value is the current LO frequency, expressed in units of Hz.
    long LOfreq;
    double pMHz;
    int ret;

    print_time(1);
    G_dll_active = TRUE;
    fprintf(G_fp_logfile, "[%d] GetHWLO Called.\n", line_number++);
    print_time(0);
    ret = srGetFreq(&pMHz);
    LOfreq = (long) pMHz;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] GetHWLO Called-> LOfreq: %ld\n", line_number++, LOfreq);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] GetHWLO Called-> Finished\n", line_number++);
    G_dll_active = FALSE;
    return LOfreq; //LOfreq;
}

void ModeChanged(char mode) {
    int status = 0;
    long t = 0;
    static int signal_sent = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] ModeChanged Called -> mode: %c,G_calibrating: %d\n", line_number++, mode, G_calibration_mode);
    //Disable mode setting if the application is in Power Calibration mode.  
    //Power Calibration has control of the Proficio
    if (!G_calibration_mode) {
        G_dll_active = TRUE;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] ModeChanged -> Calling usbControlMsgOUT \n", line_number++);
        status = cw_send_parameters(SET_CW_MODE, mode, 1);
        Sleep(50);
        status = cw_send_parameters(SET_CW_MODE, mode, 1);
        Sleep(50);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] ModeChanged -> mode: %c, Return Status: %d\n", line_number++, mode, status);
        if (status > 0) {
            G_mode = mode;
            if (mode == 'C') {
                if (signal_sent == 0) {
                    system("pkill --signal SIGUSR2 -x solidus-cw");
                    signal_sent = 1;
                }
                if (G_cw_semi_break_in) {
                    //Now start the Get_Key_Down_Status thread
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] ModeChanged -> Starting Key_Down_Thead -> G_cw_semi_break_in: %d, mode: %c\n", line_number++,
                            G_cw_semi_break_in, mode);
                    if (!G_Get_Key_Down_Status_Thread_Running) {
                        G_Get_Key_Down_Status_Thread_Running = TRUE;
                        G_Get_Key_Down_Status_rc = pthread_create(&G_Get_Key_Down_Status_thread,
                                NULL, Get_Key_Down_Status, (void *) t);
                        if (G_Get_Key_Down_Status_rc) {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] ModeChanged -> Start Get_Key_Down_Status thread failed, return code from pthread_create() is %d\n",
                                    line_number++, G_Get_Key_Down_Status_rc);
                            G_Get_Key_Down_Status_Thread_Running = FALSE;
                        }
                    }
                }
            } else {
                G_Get_Key_Down_Status_Thread_Running = FALSE;
                if (signal_sent == 1) {
                    system("pkill --signal SIGUSR2 -x solidus-cw");
                    signal_sent = 0;
                }
            }
            freq_queue_add(G_tune_freq);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] ModeChanged -> Call to usbControlMsgOUT Failed -> status: %d\n", line_number++, status);
        }
        if (!G_in_IQ_calibration_mode) {
            if (G_band_normal != GENERAL_BAND) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] ModeChanged -> Calling set_last_used for mode %c \n", line_number++, G_mode);
                switch (G_vfo) {
                    case VFO_A:
                        status = Set_last_used_VFO_A(G_tune_freq, G_mode);
                        if (status == FALSE) { // Try again.  Update_Last_Used is busy
                            Sleep(100);
                            status = Set_last_used_VFO_A(G_tune_freq, G_mode);
                        }
                        break;
                    case VFO_B:
                        status = Set_last_used_VFO_B(G_tune_freq, G_mode);
                        if (status == FALSE) { // Try again.  Update_Last_Used is busy
                            Sleep(100);
                            status = Set_last_used_VFO_B(G_tune_freq, G_mode);
                        }
                }
            }
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] ModeChanged Finished \n", line_number++);
        G_dll_active = FALSE;
    }
}

/*int Start_Bias_thread() {
    int status = TRUE;
    long last_used_t = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Starting Start_Bias_thread thread \n", line_number++);
    G_Bias_rc = pthread_create(&G_Bias_thread, NULL, Check_Potentia_Bias, (void *) last_used_t);
    if (G_Bias_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Start_Bias_thread thread failed, return code from pthread_create() is %d\n",
                line_number++, G_Bias_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start_Bias_thread thread Started, return code from pthread_create() is %d\n",
                line_number++, G_Bias_rc);
    }
    return status;
}

int Start_check_temperature_thread() {
    int status = TRUE;
    long last_used_t = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Starting Check_temperature thread \n", line_number++);
    G_Check_temperature_rc = pthread_create(&G_Check_temperature_thread, NULL, Check_temperature, (void *) last_used_t);
    if (G_Check_temperature_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Check_temperature thread failed, return code from pthread_create() is %d\n",
                line_number++, G_Check_temperature_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Check_temperature thread Started, return code from pthread_create() is %d\n",
                line_number++, G_Check_temperature_rc);
    }
    return status;
}

int Start_master_controller_thread() {
    int status = TRUE;
    long last_used_t = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Starting Master_Controller thread \n", line_number++);
    G_Master_controller_rc = pthread_create(&G_Master_controller_thread, NULL, Master_controller, (void *) last_used_t);
    if (G_Master_controller_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Master_controller thread failed, return code from pthread_create() is %d\n",
                line_number++, G_Master_controller_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Master_controller thread Started, return code from pthread_create() is %d\n",
                line_number++, G_Master_controller_rc);
    }
    return status;
}

int Start_meter_thread() {
    int status = TRUE;
    long last_used_t = 0;

    /*print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Starting Meter_main thread \n", line_number++);
    G_Meter_main_rc = pthread_create(&G_Meter_main_thread, NULL, Meter_main, (void *) last_used_t);
    if (G_Meter_main_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Meter_main thread failed, return code from pthread_create() is %d\n",
                line_number++, G_Meter_main_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Meter_main thread Started, return code from pthread_create() is %d\n",
                line_number++, G_Meter_main_rc);
    }
    return status;
}

int Start_iqbd_thread() {
    long t;
    int status = 1;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Starting iqbd thread \n", line_number++);
    G_IQBD_rc = pthread_create(&G_IQBD_thread, NULL, iqbd, (void *) t);
    if (G_IQBD_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start iqbd thread failed, return code from pthread_create() is %d\n",
                line_number++, G_IQBD_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start iqbd thread Started, return code from pthread_create() is %d\n",
                line_number++, G_IQBD_rc);
    }
    return status;
}

int Start_Solidus_Temperature_thread() {
    long t;
    int status = 1;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Starting Check_Potentia_Temperature thread \n", line_number++);
    G_Check_Power_rc = pthread_create(&G_Power_thread, NULL, Check_Potentia_Temperature, (void *) t);
    if (G_Check_Power_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Check_Potentia_Temperature thread failed, return code from pthread_create() is %d\n",
                line_number++, G_Check_Power_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Check_Potentia_Temperature thread Started, return code from pthread_create() is %d\n",
                line_number++, G_Check_Power_rc);
    }
    return status;
}

int Start_PWM_thread() {
    long t;
    int status = 1;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Starting Start_PWM_thread thread \n", line_number++);
    G_PWM_rc = pthread_create(&G_PWM_thread, NULL, Fan_PWM, (void *) t);
    if (G_PWM_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Start_PWM_thread thread failed, return code from pthread_create() is %d\n",
                line_number++, G_PWM_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Start_PWM_thread thread Started, return code from pthread_create() is %d\n",
                line_number++, G_PWM_rc);
    }
    return status;
}

int Start_MFC_thread() {
    long t;
    int status = 1;

    /*print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Starting Start_MFC_thread thread \n", line_number++);
    G_MFC_rc = pthread_create(&G_MFC_thread, NULL, MFC_Thread, (void *) t);
    if (G_MFC_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Start_MFC_thread thread failed, return code from pthread_create() is %d\n",
                line_number++, G_MFC_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Start_MFC_thread thread Started, return code from pthread_create() is %d\n",
                line_number++, G_MFC_rc);
    }
    return status;
}

int Start_Display_thread() {
    long t;
    int status = 1;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Starting Start_Display_thread thread \n", line_number++);
    G_Display_rc = pthread_create(&G_Display_thread, NULL, Process_Display_thread, (void *) t);
    if (G_Display_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Start_Display_thread thread failed, return code: %d\n",
                line_number++, G_Display_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Start_Display_thread thread Started, return code: %d\n",
                line_number++, G_Display_rc);
    }
    return status;
}*/

int Start_Threads() {
    int status = 1;
    int check_status = 1;
    long t = 0;

    print_time(1);
    fprintf(G_fp_logfile, "[%d] main -> Start_Threads CALLED, \n", line_number++);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Starting Flusher_thread \n", line_number++);
    G_Flusher_thread_rc = pthread_create(&G_Flusher_thread, NULL, Flusher_thread, (void *) t);
    if (G_Flusher_thread_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Flusher_thread thread failed, return code from pthread_create() is %d\n", line_number++,
                G_Flusher_thread_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Flusher_thread thread started, return code from pthread_create() is %d\n", line_number++,
                G_Flusher_thread_rc);
    }
    //Sleep(2000);
    //status = Start_check_temperature_thread();
    //Sleep(2000);
    //status = Start_PWM_thread();
    //Sleep(2000);

    /*check_status = Check_i2c_devices();
    if (G_MASTER_CONTROLLER_attached == TRUE) {
        status = Start_master_controller_thread();
        Sleep(2000);
    }
    if (G_MFC_attached == TRUE) {
        status = Start_MFC_thread();
        if (status == 1) {
            Sleep(2000);
            if (G_Display_attached == TRUE) {
                Start_Display_thread();
                Sleep(2000);
            }
        }
    }
    if (G_SOLIDUS_TEMP_SENSOR_attached == TRUE) {
        status = Start_Solidus_Temperature_thread();
        Sleep(2000);
    }
    if (G_METER_attached == TRUE) {
        status = Start_meter_thread();
        Sleep(2000);
    }
    if (G_IQBD_attached == TRUE) {
        status = Start_iqbd_thread();
        Sleep(2000);
    }
    if (G_CURRENT_SENSOR_attached == TRUE) {
        status = Start_Bias_thread();
        Sleep(2000);
    }*/
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Starting Command Interface Thread \n", line_number++);
    G_CW_Command_Interface_rc = pthread_create(&G_CW_Command_Interface_thread, NULL, CW_Command_Interface, (void *) t);
    if (G_CW_Command_Interface_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Command_Interface thread failed, return code from pthread_create() is %d\n",
                line_number++, G_CW_Command_Interface_rc);
        //MessageBoxA(NULL,"Command Interface Thread Startup Failed. main will now exit", "main", MB_OK);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Command_Interface thread Started, return code from pthread_create() is %d\n",
                line_number++, G_CW_Command_Interface_rc);
    }
    Sleep(2000);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Starting Update_last_used_thread \n", line_number++);
    G_Udate_last_used_thread_rc = pthread_create(&G_Update_last_used_thread, NULL,
            Update_last_used_thread, (void *) t);
    if (G_Udate_last_used_thread_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Update_last_used_thread thread failed, return code from pthread_create() is %d\n",
                line_number++, G_Udate_last_used_thread_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Update_last_used_thread thread Started, return code from pthread_create() is %d\n",
                line_number++, G_Udate_last_used_thread_rc);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Starting Freq_Dequeue_thread \n", line_number++);
    G_Freq_Queue_rc = pthread_create(&G_Freq_Queue_thread, NULL, Freq_Dequeue_thread, (void *) t);
    if (G_Freq_Queue_rc) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Freq_Dequeue_thread thread failed, return code from pthread_create() is %d\n",
                line_number++, G_Freq_Queue_rc);
        status = -1;
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Start Freq_Dequeue_thread thread Started, return code from pthread_create() is %d\n",
                line_number++, G_Freq_Queue_rc);
    }

    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Start_Threads FINISHED \n\n", line_number++);
    return (status | check_status);
}

/*void Stop_NOW(uint8_t up_date_transceiver, uint8_t shutdown_status) {
    print_time(1);
    fprintf(G_fp_logfile, "[%d] Stop_NOW Called. update_transceiver: %d, shutdown_status: %d\n",
            line_number++, up_date_transceiver, shutdown_status);
    exit(1);
}*/

int main(int argc, char **argv) {
    int status;
    int major_version = 0, minor_version = 0;
    int ret_status;
    uint8_t mode;
    long t = 0;
    long gui_t = 0;
    long last_used_t = 0;
    int completion_status = TRUE;
    int dir_configured = 0;


    printf("MSCC STARTING\n");
    ret_status = Open_log_file(dir_configured);
    if (!ret_status) {
        Stop_all(0, STOP_LOGFILE_FAILED);
        exit(1);
    }
    //signal(SIGUSR1, signal_callback_handler);
    //signal(SIGTERM, sig_term_handler);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main starting -> Compile Date %s, Compile Time %s \n", line_number++,
            COMPILE_DATE, COMPILE_TIME);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Calling: Get_serial_number \n", line_number++);
    G_serial_number_success = Get_serial_number();
    G_serial_number_success = TRUE;
    if (G_serial_number_success) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Calling: srOpen -> USB VID: %X, USB PID %X \n", line_number++, MULTUS_VID, MULTUS_PID);
        srOpen(MULTUS_VID, MULTUS_PID, L"", L"", L"", -1, &status);
        if (status) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] G_Multus_serial_number: %s lenght: %d\n", line_number, G_Multus_serial_number, (strlen(G_Multus_serial_number)));
            print_time(0);
            fprintf(G_fp_logfile, "[%d] G_Usb_serial_number: %s lenght: %d\n", line_number, G_Usb_serial_number, (strlen(G_Usb_serial_number)));
            status = strcmp(G_Multus_serial_number, G_Usb_serial_number);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] strcmp result: %d\n", line_number, status);
            if (status != 0) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] main -> Serial Numbers Do Not Match \n", line_number++);
                G_transceiver_initialization_status = FALSE;
                Stop_all(0, STOP_PROFICIO_SERIAL_NUMBER);
            }
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] main -> Open USB ***FAILED***, Status: %d\n", line_number++, status);
            G_transceiver_initialization_status = FALSE;
            Stop_all(0, STOP_PROFICIO);
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Get_serial_number ***FAILED*** \n", line_number++);
        G_transceiver_initialization_status = FALSE;
        Stop_all(0, STOP_PROFICIO_SERIAL_NUMBER);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Calling srGetVersion\n", line_number++);
    ret_status = srGetVersion(&major_version, &minor_version);
    if (ret_status) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> srGetVersion returned Transceiver Firmware Version: %d.%d \n",
                line_number++, major_version, minor_version);
        if ((major_version == MULTUS_MAJOR_VERSION_119 && minor_version == MULTUS_MINOR_VERSION_81) ||
                (major_version == MULTUS_MAJOR_VERSION_SOLIDUS && minor_version == MULTUS_MINOR_VERSION_SOLIDUS)) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] main -> Firmware OK. \n",
                    line_number++);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] main -> Firmware version less than required. ***main WILL NOT FUNCTION***\n",
                    line_number++);
            G_transceiver_initialization_status = FALSE;
            Stop_all(0, STOP_PROFICIO_SERIAL_NUMBER);
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Called to srGetVersion ***FAILED***, Status: %d \n", line_number++, ret_status);
        G_transceiver_initialization_status = FALSE;
        Stop_all(0, STOP_PROFICIO);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Sending Si5351 DLL -> Calling usbControlMsgOUT \n", line_number++);
    mode = SI5351_DLL;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Calling get_cw_params \n", line_number++);
    get_cw_params();
    status = cw_send_parameters(SET_DLL_VERSION, SI5351_DLL, 1);
    Sleep(10);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Sending Si5351 DLL -> Finished-> mode: %d, status: %d\n", line_number++, mode, status);
    if (status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Send Si5351 DLL -> Failed-> mode: %d, status: %d\n", line_number++, mode, status);
        G_transceiver_initialization_status = FALSE;
        Stop_all(0, STOP_PROFICIO);
    }
    Get_RPI_IP();
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Calling Init_PPM \n", line_number++);
    status = Init_PPM();
    if (status == 1) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Calling Multus_Set_Calibration \n", line_number++);
        status = Multus_Set_Calibration(G_int, G_dec);
        if (status < 0) {
            G_transceiver_initialization_status = FALSE;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] main -> Calling Multus_Set_Calibration -> FAILED. status: %d\n", line_number++, status);
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Calling Init_PPM -> File Not Found -> Creating freq_cal.ini status: %d\n",
                line_number++, status);
        Create_PPM_ini();
        status = Multus_Set_Calibration(G_int, G_dec);
        if (status < 0) {
            G_transceiver_initialization_status = FALSE;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] main -> Calling Multus_Set_Calibration -> FAILED. status: %d\n", line_number++, status);
        }
    }
    if (G_transceiver_initialization_status == TRUE) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Calling Init_favorites \n", line_number++);
        Init_favorites();
    }
    if (G_transceiver_initialization_status == TRUE) {
        //Check_Power_Cal_Version();
        status = Create_power_ini_file();
        Check_Amplifier_Version();
        status = Create_amplifier_ini_file();
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Calling Init_last_used_VFO_A \n", line_number++);
    status = Init_last_used_VFO_A();
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Calling Init_last_used_VFO_B \n", line_number++);
    status = Init_last_used_VFO_B();

    /*if (status == 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> init_last_used FAILED. Calling Create_Last_Used \n", line_number++);
        Create_Last_Used();
        status = Init_last_used_VFO_A();
        if (status == 0) {
            //MessageBoxA(NULL, "Last_used.ini file is CORRUPT -> Permanent FAILURE. main will now stop \r\n ",
            //	"main", MB_OK | MB_ICONASTERISK);
            Stop_all(0, 0);
        }
    }*/
    Sleep(10);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Calling Init_User_Controls \n", line_number++);
    Init_User_Controls();
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Calling Init_Master_Comm_Port_File \n", line_number++);
    //status = Init_Master_Comm_Port_File();
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Waiting for network to initialize \n", line_number++);
    if (G_transceiver_initialization_status == FALSE) {
        Sleep(3000);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Transceiver Initialization Failed\n", line_number++);
        Stop_all(0, STOP_PROFICIO);
    }
    if (completion_status == FALSE) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> main initialization failed.  Terminating\n", line_number++);
        Gui_send_param(CMD_SET_STOP, 0);
        //MessageBoxA(NULL, "main did not initialize properly. Send Logs to Multus SDR, LLC.", "ms-sdr", MB_OK | MB_ICONEXCLAMATION);
        Stop_all(1, 0);
    }
    //wiringPiSetupSys();
    status = Start_Threads();
    if (status < 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main -> Threads Start FAILED\n", line_number++);
        Stop_all(0, 0);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Finished Initializing\n", line_number++);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Running \n", line_number++);
    while (G_main_thread_run) {
        Sleep(G_thread_sleep_time);
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] main -> Normal Exit \n", line_number++);
    if (G_Shutdown_Status == STOP_SHUTDOWN) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] main. STOP_SHUTDOWN \n", line_number++);
        if (G_fp_logfile != NULL) {
            fflush(G_fp_logfile);
            fclose(G_fp_logfile);
        }
        Sleep(3000);
        //ret = system("$HOME/mscc/servers/shutdown-scripts/system-shutdown");
    }
    if (G_fp_logfile != NULL) {
        fflush(G_fp_logfile);
        fclose(G_fp_logfile);
    }
    return TRUE;
}

