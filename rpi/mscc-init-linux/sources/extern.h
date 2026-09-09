#pragma once
#define _CRT_SECURE_NO_WARNINGS 1
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib,"OneCore.lib")
#pragma comment(lib, "winspool.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "WbemUuid.lib")
#pragma comment(lib, "msports.lib")

#include <Windows.h>
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

#define MAX_OUTPUT_DEVICES 50
#define MAX_PIXELS 800
#define DISPLAY_CENTER 399
#define DISPLAY_BANDWIDTH 72000
#define MAX_X 400
#define MAX_MARKER_SIZE 4000
#define MARKER_OUT_OF_RANGE 1000;
#define TRUE 1
#define FALSE 0
#define PATH_MAX MAX_PATH

extern PaStream *stream;
extern PaStreamParameters inputParameters, outputParameters;
extern const PaDeviceInfo* lpInfo;
extern const PaDeviceInfo* lpInfo_Pulse;
extern state mystate;
extern PaDeviceIndex G_Pulse_Proficio_Device;
extern PaDeviceIndex G_Pulse_Output_Device;
extern int G_output_device_index;
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
extern uint8_t G_PCB_Version;
extern uint8_t G_tx_mode;
extern FILE *G_fp_logfile;
extern char G_mode;
extern int line_number;
extern uint8_t G_Monitor;
extern uint8_t G_tx_band_pass;
//extern float G_side_tone_pitch;
extern float t_volumeLevel;

extern struct sockaddr_in si_me, si_ms_sdr;
extern int sdrcore_s, ms_sdr_s, recv_len;
extern int slen;

extern int num_output_devices_found;
extern state mystate;
extern calstate mycalstate;

extern char* My_getenv(char* myenv);

extern float G_side_tone_pitch;
extern float G_t_volumeLevel;
extern uint8_t G_Pitch_changed;