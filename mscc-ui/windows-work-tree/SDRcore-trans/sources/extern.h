#define _CRT_SECURE_NO_WARNINGS 1
#pragma comment(lib, "Ws2_32.lib")
#pragma once
//#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <conio.h>
#include <ShlObj.h>
#include <KnownFolders.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <limits.h>
#include <fcntl.h>
#include <malloc.h>
#include "port_defines.h"
#include "version.h"
#include "sdrcoretx.h"
#include "wsfirgen.h"
#include "dsputils.h"
#include "commands.h"
#include "pthread.h"
#include "semaphore.h"
#include "sched.h"
#include "portaudio.h"

// {Model}.{[M]M}{Release Number}  (the bytes are reversed)  
// Model 0-OSB, 1-Proficio, 2-Geminus,3-MKII-PTT,4-MKII-ATU,5-Proficio-PTT,6-Proficio-ATU
// Month is the number of months from the starting point of 09/24. 01/25 is 13, 02/25 is 14, etc.
// Release number is in the range of 0 to 9

#define VERSION_MAJOR 3
#define VERSION_MINOR 140
#define VERSION_MS_SDRCORE_TRANS ((((VERSION_MINOR) << 8) & 0xff00) | ((VERSION_MAJOR) & 0x00ff))

#define MAX_KEEP_ALIVE_COUNT 15
#define MAX_INPUT_DEVICES 50
#define USB_POWER 0
#define LSB_POWER 1
#define AM_POWER 2
#define CW_POWER 3
#define TUNE_POWER 4
#define TRUE 1
#define FALSE 0
#define PATH_MAX MAX_PATH

#define GEMINUS 6

extern PaStream *stream;
extern int G_input_device_index;
extern int G_digital_input_device_index;
extern int G_all_threads_run;
extern uint8_t G_PCB_Version;
extern uint8_t G_network_initialized;
extern int num_input_devices_found;
extern state mystate;
extern micproc_state micstate;
extern sp_float G_ALC_gain;
extern long int G_alc_count;
extern sp_float G_ALC_limit;
extern sp_float G_ALC_multiplier;
extern int8_t G_Do_ALC;
extern int8_t G_Allow_ALC_Send;
extern uint8_t G_audio_mode;
extern void *UDP_Thread(void *my_param);
extern void *Flusher_thread(void *t);
extern void *Overdriven_thread(void *t);
extern void *ALC_Meter_thread(void *t);
extern FILE *G_fp_logfile;
extern uint8_t G_Allow_Log_Write;
extern char G_mode;
extern int G_mode_change;
extern uint16_t G_band;
extern uint8_t G_Mia_Status;
extern uint8_t G_tx_mode;
extern uint8_t G_QSK;
extern int G_null_count;
extern int line_number;
extern void print_time(void);
//extern void Sleep(long sleep_time);
extern int Open_log_file();
extern void Reset_Logfile();
extern void setFilterOffsets(sp_float filterSetLow, sp_float filterSetHigh);
extern int G_watts_int;
extern int G_watts_decimal;
extern float G_watts;
extern PaDeviceIndex Mic_Device;
extern PaDeviceIndex Proficio_Device;

extern sp_float G_mic_volume;
extern sp_float iMult;
extern sp_float qMult;
extern int ms_sdr_s;
extern int slen;
extern struct sockaddr_in si_ms_sdr;
extern sp_float gain;

extern PaStream *stream;
extern PaStreamParameters inputParameters, outputParameters;
extern const PaDeviceInfo* lpInfo;
extern long long G_keep_alive;
//extern const char *homedir;
//extern micproc_state micstate;

extern float volumeLevel;

struct input_devices {
	int record_number;
	int device_index;
	char name[512];
	int num_channels;
	int default_input_device;
	int selected_input_device;
};

extern int G_power_file_needs_updated;

typedef struct  {
	int usb_power;
	int lsb_power;
	int am_power;
	int cw_power;
	int tune_power;
}power_levels;

typedef struct {
	int record;
	int band;
	int power_level;
}power_stack;

typedef struct {
	int record;
	int band;
	int power_level;
}amplifier_stack;

extern int manage_stream(int start_stop, int device, int channels);
extern void build_input_devices(int device_index);
extern void build_digital_input_devices(int device_index);
//extern int update_sound_ini();
//extern int check_for_sound_ini_file();
//extern void init_sound_ini();
//extern void set_selected_device(int record_index);
//extern void print_input_devices(void);
extern int check_for_power_ini_file(void);
extern int Get_Operator_Sound_Device();
extern int Get_Digital_Sound_Device();
extern char* My_getenv(char* myenv);

extern int Update_Proficio_User_Power_ini(void);
extern void set_selected_power(int power_field, int power_value);
extern void build_power_levels(void);
//extern int delete_ini_file();
extern int delete_iq_ini_file();
extern void Init_Proficio_User_power();
extern int Init_Proficio_calibration(uint8_t send_to_transceiver);
extern int Init_amplifier_user_values();
extern int Init_amplifier_calibration();
extern int Update_amplifier_calibration();
extern void Init_Power_All(void);
extern int Create_amplifier_calibration_file(int force);
extern float get_QRO_power_level(int band, int mode);
extern float get_QRP_power_level(int band, int mode);

//For driver.c
//extern float Avg_Potentia_Temp(float temperature);
void *Drive_Manager(void *t);
extern uint8_t G_QRP_mode;
extern float G_temperature;
extern int G_Power_Values_initialized;
extern int G_calibration_index;
extern float G_drive_average_result;
extern uint8_t G_temperature_received;
extern uint8_t G_bias_received;
extern uint32_t G_potentia_bias;
extern float G_VU_ratio;
