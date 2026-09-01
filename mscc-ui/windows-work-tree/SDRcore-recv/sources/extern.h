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
#include "sdrcore.h"
#include "iq.h"
#include "sdrcore.h"
#include "wsfirgen.h"
#include "dsputils.h"
#include "blanker.h"
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
#define VERSION_MS_SDRCORE_RECV ((((VERSION_MINOR) << 8) & 0xff00) | ((VERSION_MAJOR) & 0x00ff))

#define MAX_OUTPUT_DEVICES 50
#define MAX_PIXELS 3200
#define DISPLAY_CENTER 399
#define DISPLAY_BANDWIDTH 72000
#define MAX_X 400
#define MAX_PAN_SEGMENTS (MAX_PIXELS / MAX_X)
#define MAX_MARKER_SIZE 4000
#define MARKER_OUT_OF_RANGE 1000;
#define TRUE 1
#define FALSE 0
#define PATH_MAX MAX_PATH
//#define MB_ICONASTERISK 2
//#define MB_OK 1
//#define MB_ICONEXCLAMATION 3
//#define MB_ICONSTOP 4
//#define SOCKET_ERROR -1
//#define INVALID_SOCKET -1

//extern void MessageBoxA(void *nothing, char *message, char * sender,int ok_button_and_symbol);
//extern int usleep(unsigned long usec);
//extern void Sleep(long sleep_time);
extern PaStream *stream;
extern PaStreamParameters inputParameters, outputParameters;
extern const PaDeviceInfo* lpInfo;
extern const PaDeviceInfo* lpInfo_Pulse;
extern state mystate;
extern PaDeviceIndex G_Pulse_Proficio_Device;
extern PaDeviceIndex G_Pulse_Output_Device;
extern int G_output_device_index;
extern int G_digital_output_device_index;
extern int G_num_pulse_output_devices_found;
extern uint8_t G_Image_Check;

extern float G_volumeLevel;
extern int G_band_marker_low;
extern int G_band_marker_high;
extern int G_iq_file_needs_updated;
extern int G_Smoothing;
extern int G_all_threads_run;
extern long long G_keep_alive;
extern uint8_t G_Panadapter_Blocks;
/* Active pan display bins: 800 / 1600 / 3200 (set via CMD_GET_SET_PANADAPTER_REFRESH index 0/1/2). */
extern int G_Panadapter_Pixels;
extern uint8_t G_PCB_Version;
extern uint8_t G_tx_mode;
extern FILE *G_fp_logfile;
extern char G_mode;
extern int line_number;
extern uint8_t G_Monitor;
extern uint8_t G_tx_band_pass;
//extern float G_side_tone_pitch;
extern float t_volumeLevel;
extern uint8_t G_Pause_Panadapter;
/* Set TRUE with G_Pause_Panadapter to re-arm pause (clear history + reset count). */
extern uint8_t G_Pause_Panadapter_Reload;
/* Frames to blank when reload arms (small vs large LO step). */
extern int G_Pause_Panadapter_Cycles;
/* Any MAIN_FREQ change: clear pan smoother history. */
extern uint8_t G_Pan_Flush_History;


struct output_devices {
	int record_number;
	int device_index;
	char name[120];
	int num_channels;
	int default_output_device;
	int selected_output_device;
};


extern struct sockaddr_in si_me, si_ms_sdr;
extern int sdrcore_s, ms_sdr_s, recv_len;
extern int slen;

extern int num_output_devices_found;
extern state mystate;
extern calstate mycalstate;
extern void *UDP_Thread(void *my_param);
extern void *Flusher_thread(void *t);
extern void *Send_smeter_thread(void *param);
extern void *Panadapter_thread(void *t);
extern void *Check_Image_Level_thread(void *param);
////extern void side_tone(sp_cplx* samps);
extern void print_time();
extern int Open_log_file();
extern void setFilterOffsets(sp_float filterSetLow, sp_float filterSetHigh);

extern int manage_stream(int start_stop, int device,int channels);
extern void build_output_devices(int device_index);
extern void build_digital_output_devices(int device_index);
extern void Build_pulse_output_devices(int device_index);
//extern int Update_Pulse_sound_ini(int index);
extern int Get_Sound_Device();
extern int Get_Digital_Sound_Device();
//extern int update_sound_ini();
//extern int check_for_sound_ini_file();
//extern void init_sound_ini();
//extern void set_selected_device(int record_index);
//void print_output_devices(void);
////extern int delete_ini_file();
extern int IQ_Set_All_Band(int iq_value);
extern void IQ_calc(int iq_int);
extern int update_iq_ini_file();
extern int Init_IQ_structure();
extern int Load_IQ_Defaults();
extern int create_iq_ini_file(int force);
extern int Check_Image_Level(int start);
extern char* My_getenv(char* myenv);

//For Side Tone Generator
extern int manage_sidetone_stream(int start_stop, int device, int channels);
//extern void *Read_Key_thread(void *t);
extern float G_side_tone_pitch;
extern float G_t_volumeLevel;
extern uint8_t G_Pitch_changed;