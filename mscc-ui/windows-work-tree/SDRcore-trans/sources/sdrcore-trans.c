/********************** SDRdemo *************************
*	SDR Demo using Multus SDR LLC Hardware				*
*	SDRcore engine inclusions provided under license	*
*	from James L Barber, AKA Silicon Pixels, Spokane	*
*	Radio et al											*
*********************************************************/
#define _CRT_SECURE_NO_WARNINGS 1

#define RON 1
#ifdef RON
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#include "version.h"
#endif
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <windows.h>
#include "sdrcoretx.h"
#include "portaudio.h"
#include "dsputils.h"

#ifdef RON
	#include "pthread.h"
	#include "sched.h"
	#include "semaphore.h"
	#include "extern.h"
	#include "commands.h"
#endif

#define PA_SAMPLE_TYPE      paFloat32
typedef float SAMPLE;
int	modeChanged = FALSE;
int inputchannels = 0;
int myAudioAPItype = 0;
sp_float *inbuffer, *outbuffer;
sp_float wold = 0;
sp_cplx incplx[4097];
sp_cplx outcplx[4096];
float volumeLevel = -0.0f; //Start with the volume muted.

state mystate;
PaStream *stream;
alc_state alcstate;
micproc_state micstate;

const PaDeviceInfo* lpInfo;
PaStreamParameters inputParameters, outputParameters;
char G_l_path[MAX_PATH] = { 0 };

#define MAX_INPUT_DEVICES 50
#define NO_INPUT_DEVICE 100
//For the UDP_Thread
pthread_t p_UDP_thread;
int UDP_thread_rc;
HANDLE UDP_thread_Win32;
//For the flusher Thread
pthread_t p_Flusher_thread;
int Flusher_thread_rc;
HANDLE Flusher_thread_Win32;
int G_all_threads_run = 0;
uint8_t G_network_initialized = 0;
int sound_device_index = -1;

pthread_t p_Overdriven_thread;
int Overdriven_thread_rc;
HANDLE Overdriven_thread_Win32;

pthread_t p_ALC_report_thread;
int ALC_report_thread_rc;
HANDLE ALC_thread_Win32;

pthread_t p_Drive_thread;
int Drive_thread_rc;
HANDLE Drive_thread_Win32;

FILE *G_fp_logfile;
int line_number = 0;
struct input_devices G_input_devices[MAX_INPUT_DEVICES];
int G_input_device_index = NO_INPUT_DEVICE;

sp_float G_mic_volume = 0.5f;
uint16_t G_VU_value = 0;
sp_float G_peak = 0.0f;
sp_float iMult = 1.0f;
sp_float qMult = 1.0f;
uint8_t G_DSP_Busy = FALSE;

/****** NOTE: Declared external in other modules and used as a global ******/
state mystate;

static int sdrAudioCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData);

static int gNumNoInputs = 0;
/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int sdrAudioCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{
	SAMPLE *out = (SAMPLE*)outputBuffer;
	const SAMPLE *in = (const SAMPLE*)inputBuffer;
	unsigned int i;
	(void)timeInfo; /* Prevent unused variable warnings. */
	(void)statusFlags;
	(void)userData;

	G_DSP_Busy = TRUE;
	if (inputBuffer == NULL)
	{
		for (i = 0; i<framesPerBuffer; i++)
		{
			*out++ = 0;  /* left - silent */
			*out++ = 0;  /* right - silent */
		}
		gNumNoInputs += 1;
	}
	else
	{
		inbuffer = (sp_float*)inputBuffer;
		outbuffer = (sp_float*)outputBuffer;

		/*
			Mux samples into complex struct array. The "custom" typedef is to avoid consistency issues
			across compilers with COMPLEX. 
		*/
		framesToComplex(inbuffer, incplx, outcplx, framesPerBuffer, inputchannels);
		
		if ((mystate.opmode == MODE_AM) || (mystate.opmode == MODE_LSB) || (mystate.opmode == MODE_USB))
			doMicProc(incplx, framesPerBuffer);

		/**************************** Two-tone test *****************************************/
		if (mystate.twoToneFlag)
		{
			ssb_modulate(incplx);
			twoTone(incplx);		// overwrites any mic audio data in buffer
		}
		else
		{
			/**************************** AM and SSB modulators *********************************/
			if (mystate.opmode == MODE_AM) am_modulate(incplx);
			if (mystate.opmode == MODE_LSB) ssb_modulate(incplx);
			if (mystate.opmode == MODE_USB) ssb_modulate(incplx);
			if (mystate.opmode == MODE_TUNE) tune_modulate(incplx);
			if (mystate.opmode == MODE_CW) tune_modulate(incplx);
		}

		//null_modulate(incplx);		// no mode, zero out IQ stream for no output
		
		/**************************** DO THE RADIO THING *********************************/
		fastconv(incplx, outcplx, (int)framesPerBuffer);
		/*
			De-mux samples back into packed I-Q-I-Q-I-Q format. In case there's any question,
			Q should lead I by 90 degrees.
		*/
		for (i = 0; i < framesPerBuffer; i++)
		{
			*outbuffer = outcplx[i].real * iMult;
			outbuffer++;
			*outbuffer = outcplx[i].imag * qMult;
			outbuffer++;
		}
	}
	G_DSP_Busy = FALSE;
	return paContinue;
}

int manage_stream(int start_stop, int device, int channels) {
	PaError err;
	int status = 0;
	static int current_device = 0;
	static int current_channels = 0;

	print_time();
	fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream -> Called.  New device: %d, New Channels %d\n",
		line_number++, device, channels);
	switch (start_stop) {
	case 1:
		print_time();
		fprintf(G_fp_logfile, "[%d] Main thread -> manage_stream -> Starting Stream. \n", line_number++);
		err = Pa_OpenStream(
			&stream,
			&inputParameters,
			&outputParameters,
			(int)mystate.samplerate,
			mystate.frames,
			0, /* paClipOff, */  /* we won't output out of range samples so don't bother clipping them */
			sdrAudioCallback,
			NULL);
		if (err != paNoError)
		{
			const PaHostErrorInfo* lpError = Pa_GetLastHostErrorInfo();
			print_time();
			status = err;
			fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream -> Open Stream Failed: status: %d,PA ERROR: %s\n", 
				line_number++, status,lpError->errorText);
			
			MessageBoxA(NULL, "OPEN RECORD DEVICE FAILED","SDRcore - trans", MB_OK | MB_ICONEXCLAMATION);
		}
		else {
			err = Pa_StartStream(stream);
			if (err != paNoError) {
				status = err;
				const PaHostErrorInfo* lpError = Pa_GetLastHostErrorInfo();
				print_time();
				fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream -> Start Stream Failed: PA ERROR: %s\n", line_number++,
					lpError->errorText);
				MessageBoxA(NULL, "START RECORD AUDIO STREAM FAILED ","SDRcore-trans", MB_OK | MB_ICONEXCLAMATION);
			}
		}
		break;
	case 0:
		print_time();
		fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream -> Stoping Stream.  New device: %d, New Channels %d\n", line_number++, device, channels);
		Pa_StopStream(stream);
		Pa_CloseStream(stream);
		if (channels > 2) {
			print_time();
			fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream -> Device %d -> Channel Number: %d greater than 2. Setting channels to 2.\n",
				line_number++, device, channels);
			channels = 2;
		}
		if (device == 100) {
			channels = 2;
			inputParameters.device = NULL;
			print_time();
			fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream -> Device set to NULL\n", line_number++);
		}
		else {
			inputParameters.device = device;
		}
		//inputParameters.device = NULL;
		//print_time();
		//fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream -> Device set to NULL\n", line_number++);
		inputParameters.channelCount = channels;
		inputParameters.sampleFormat = PA_SAMPLE_TYPE;
		inputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
		inputParameters.hostApiSpecificStreamInfo = NULL;
		inputchannels = channels;
		break;
	}
	return status;
}

char* My_getenv(char* myenv) {
	WCHAR path[MAX_PATH] = { 0 };
	PWSTR lpPath = path;
	int lenght = 0;

	memset(G_l_path, 0, sizeof(G_l_path));
	HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &lpPath);
	//fprintf("get_serial_number -> Get folder path \n");
	if (SUCCEEDED(hr)) {
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpPath, -1, G_l_path, sizeof(G_l_path), NULL, NULL);
		strcat(G_l_path, "/MSCC-PW");
		//printf("My_getenv: %s\n", G_l_path);
	}
	return G_l_path;
}

void Audio_Device_Error(PaError err) {
	char send_buf[20] = { 0 };
	Pa_Terminate();
	G_all_threads_run = 0;
	print_time();
	fprintf(G_fp_logfile, "[%d] Audio_Device_Error. err: %d\n", line_number++, err);
	send_buf[0] = CMD_SET_STOP;
	send_buf[1] = 1;
	//memcpy(&send_buf[1],1, 1);
	if (sendto(ms_sdr_s, (char*)&send_buf, 5, 0, (struct sockaddr*)&si_ms_sdr, slen) == SOCKET_ERROR) {
		print_time();
		fprintf(G_fp_logfile, "[%d] ALC_thread -> sentto failed with error code : %s\n",
			line_number++, strerror(errno));
	}
	switch (err) {
	case PROFICIO_SET_AS_DEFAULT_DEVICE:
		print_time();
		fprintf(G_fp_logfile, "[%d] Audio_Device_Error. Transceiver set as default device.\n", line_number++);
		MessageBoxA(NULL, "TRANSCEIVER SET AS DEFAULT RECORDING DEVICE\r\nTHIS IS NOT PERMITTED\r\nCONSULT THE OPERATORS GUIDE",
			"SDRcore-trans", MB_OK | MB_ICONEXCLAMATION);
		break;
	case NO_PROFICIO:
		print_time();
		fprintf(G_fp_logfile, "[%d] No Multus SDR TX Device found.\n", line_number++);
		MessageBoxA(NULL, "THE TRANSCEIVER HAS NOT BEEN FOUND",
			"SDRcore-trans", MB_OK | MB_ICONEXCLAMATION);
		break;
	case NO_DEFAULT_DEVICE:
		print_time();
		fprintf(G_fp_logfile, "[%d] Audio_Device_Error. NO DEFAULT RECORD DEVICE FOUND.\n", line_number++);
		MessageBoxA(NULL, "NO DEFAULT RECORD DEVICE FOUND", "SDRcore-trans", MB_OK | MB_ICONEXCLAMATION);
		break;
	case NO_DEVICE:
		print_time();
		fprintf(G_fp_logfile, "[%d] Audio_Device_Error. No INPUT device found\n", line_number++);
		MessageBoxA(NULL, "NO RECORDING DEVICE FOUND.", "SDRcore-trans",
			MB_OK | MB_ICONEXCLAMATION);
		break;
	default:
		print_time();
		fprintf(G_fp_logfile, "[%d] Audio_Device_Error. Error message: %s\n", line_number++, Pa_GetErrorText(err));
		MessageBoxA(NULL, "INITIALIZATION FAILED.", "SDRcore-trans",
			MB_OK | MB_ICONEXCLAMATION);
	}
	exit(0);
}

/************************************* MAIN *********************************************/
int main(int argc, char** argv) {

	PaError err;
	int key = 0;
	float temp;
	//int status;

	/************ DSP parameters *****************/
	mystate.overdriven = 0;					// TX overdriven flag
	mystate.nfft = 2048;					// # of FFT bins
	mystate.filtertaps = 1000;				// # of taps in IF filterl
	mystate.samplerate = SAMPLERATE;
	mystate.frames = mystate.nfft - mystate.filtertaps;  // frames per buffer
	mystate.opmode = MODE_LSB;
	mystate.txPower = 1.0f;					// TX power 0-1.0 = 0-100%
	mystate.lo1_freq = 12000.0f;			// MUST be set before setFilterOffsets() is called
	setFilterOffsets(75.0f, 2700.0f);		// Set default TX filter parms
	mystate.iqReversed = TRUE;				// Yes Virginia, we're reversed...
	mystate.amCarrier = 0.25f;				// Starting value for AM carrier
	temp = 0.0f;

	//************** SET # OF CHANNELS TO USE ON TX HERE **************//
	inputchannels = 1;

	/******* Init DSP ******/
	initDSP(TRUE);							// 1st-time startup

	long t = 0;
	int log_status;
	int sound_ini_status;
	int power_ini_status;


	log_status = Open_log_file();
	if (log_status) {
		print_time();
		fprintf(G_fp_logfile, "[%d] Main Thread. SDRcore Log file opened\n", line_number++);
	}
	else {
		MessageBox(NULL, L"Log file open failed. SDRcore-trans is terminating", L"SDRcore-recv", MB_OK);
		G_all_threads_run = 0;
		exit(1);
	}
	G_all_threads_run = 1;
	print_time();
	fprintf(G_fp_logfile, "[%d] Main Thread. STARTING -> Compile Date %s, Compile Time %s \n", line_number++,
		__DATE__, __TIME__);

	power_ini_status = check_for_power_ini_file();
	if (power_ini_status == 1) {
		print_time();
		fprintf(G_fp_logfile, "[%d] Main Thread. power.ini file exists\n", line_number++);
	}
	else {
		print_time();
		fprintf(G_fp_logfile, "[%d] Main Thread. power.ini file does not exist. File will be created\n", line_number++);
	}
	//Now update the Power INI file if it does not exist
	if (power_ini_status == 0) {
		build_power_levels();
		Update_Proficio_User_Power_ini();
	}

	//Now start the UDP Thread
	print_time();
	fprintf(G_fp_logfile, "[%d] Main Thread. Starting UDP Thread\n", line_number++);
	UDP_thread_rc = pthread_create(&p_UDP_thread, NULL, UDP_Thread, (void*)t);
	if (UDP_thread_rc) {
		print_time();
		fprintf(G_fp_logfile, "[%d} Main Thread. Start up of UDP thread failed, return code from pthread_create() is %d\n", 
																							line_number++, UDP_thread_rc);
	}
	print_time();
	fprintf(G_fp_logfile, "[%d] Main Thread. SDRcore Starting Flusher Thread\n", line_number++);
	Flusher_thread_rc = pthread_create(&p_Flusher_thread, NULL, Flusher_thread, (void*)t);
	if (Flusher_thread_rc) {
		print_time();
		fprintf(G_fp_logfile, "[%d] Main Thread. Start up of Flusher thread failed, return code from pthread_create() is %d\n", 
														line_number++, Flusher_thread_rc);
	}
	print_time();
	fprintf(G_fp_logfile, "[%d] Main Thread. SDRcore Starting Overdriven Thread\n", line_number++);
	Overdriven_thread_rc = pthread_create(&p_Overdriven_thread, NULL, Overdriven_thread, (void*)t);
	if (Overdriven_thread_rc) {
		print_time();
		fprintf(G_fp_logfile, "[%d] Main Thread. Start up of Overdriven thread failed, return code from pthread_create() is %d\n", 
																	line_number++, Flusher_thread_rc);
	}
	print_time();
	fprintf(G_fp_logfile, "[%d] Main Thread. SDRcore Starting ALC Thread\n", line_number++);
	ALC_report_thread_rc = pthread_create(&p_ALC_report_thread, NULL, ALC_report_thread, (void*)t);
	if (ALC_report_thread_rc) {
		print_time();
		fprintf(G_fp_logfile, "[%d] Main Thread. Start up of ALC_report_thread failed, return code from pthread_create() is %d\n", 
																		line_number++, Flusher_thread_rc);
	}
	print_time();
	fprintf(G_fp_logfile, "[%d] Main Thread. Starting Drive_Manager Thread\n", line_number++);
	Drive_thread_rc = pthread_create(&p_Drive_thread, NULL, Drive_Manager, (void*)t);
	if (Drive_thread_rc) {
		print_time();
		fprintf(G_fp_logfile, "[%d] Main Thread. Start up of Drive_Manager thread failed, return code from pthread_create() is %d\n", 
													line_number++, Flusher_thread_rc);
	}
	sound_ini_status = check_for_sound_ini_file();
	if (sound_ini_status == 1) {
		print_time();
		fprintf(G_fp_logfile, "[%d] Main Thread. sdrcore_trans.ini file exists\n", line_number++);
	}
	else {
		print_time();
		fprintf(G_fp_logfile, "[%d] Main Thread. sdcore_trans.ini file does not exist. File will be created\n", line_number++);
	}

	/******************************************************************/
	err = Pa_Initialize();
	if (err != paNoError) {
		Audio_Device_Error(err);
	}
	// Get # of host API's supported on THIS machine at THIS time.
	const PaHostApiInfo* lpApiInfo;
	PaHostApiIndex hostApiCount;
	int apiTypeMME = 9999;
	int apiTypeDSOUND = 9999;
	int apiTypeWDMKS = 9999;
	int apiTypeWASAPI = 9999;
	int apiTypeASIO = 9999;

	hostApiCount = Pa_GetHostApiCount();

	for (int j = 0; j < hostApiCount; j++)
	{
		lpApiInfo = Pa_GetHostApiInfo(j);
		if (!strncmp("MME", lpApiInfo->name, 3))
		{
			apiTypeMME = j;
		}
		if (strstr(lpApiInfo->name, "WDM-KS"))
		{
			apiTypeWDMKS = j;
		}
		if (strstr(lpApiInfo->name, "WASAPI"))
		{
			apiTypeWASAPI = j;
		}
		if (strstr(lpApiInfo->name, "DirectSound"))
		{
			apiTypeDSOUND = j;
		}
		if (strstr(lpApiInfo->name, "ASIO"))
		{
			apiTypeASIO = j;
		}
	}

	/***** Set audio API type here. Eventually this should be abstracted to a choice on the GUI *****/
	myAudioAPItype = apiTypeMME;

	PaDeviceIndex devCount = Pa_GetDeviceCount();

	PaDeviceIndex recordDevice = 0;
	PaDeviceIndex playDevice = 0;

	// Get device count, go through devices looking for desired output device
	for (int j = 0; j < devCount; j++)
	{
		lpInfo = Pa_GetDeviceInfo((PaDeviceIndex)j);
		if (lpInfo->hostApi == myAudioAPItype) {
			if (sound_ini_status == 0) {
				build_input_devices((PaDeviceIndex)j);
			}
		}
		if (strstr(lpInfo->name, "Multus"))
		{
			if (lpInfo->maxOutputChannels >= 2)
			{
				if (lpInfo->hostApi == myAudioAPItype)
				{
					print_time();
					fprintf(G_fp_logfile, "[%d} Multus SDR TX SDR device found: %s\n", line_number++, lpInfo->name);
					playDevice = (PaDeviceIndex)j;
					break;
				}
			}
		}
	}

	if (playDevice == 0)
	{
		Audio_Device_Error(NO_PROFICIO);
	}

	inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
	if (inputParameters.device == playDevice) {
		Audio_Device_Error(PROFICIO_SET_AS_DEFAULT_DEVICE);
	}
	else {
		if (inputParameters.device == paNoDevice) {
			Audio_Device_Error(NO_DEFAULT_DEVICE);
		}
	}

	inputParameters.channelCount = inputchannels;
	inputParameters.sampleFormat = PA_SAMPLE_TYPE;
	inputParameters.suggestedLatency = 0.025f;	// Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	outputParameters.device = playDevice;
	if (outputParameters.device == paNoDevice) {
		Audio_Device_Error(NO_PROFICIO);
	}
	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = PA_SAMPLE_TYPE;
	outputParameters.suggestedLatency = 0.025f; // Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	//Now update the INI file if it does not exist
	if (sound_ini_status == 0) {
		update_sound_ini(TRUE);
	}
	else {
		//Initialize the input devices structure
		init_sound_ini();
	}
	if (G_input_device_index == NO_INPUT_DEVICE) {
		Audio_Device_Error(NO_DEVICE);
	}
	else {
		print_time();
		fprintf(G_fp_logfile, "[%d] G_input_device_index: %d, G_input_devices[G_input_device_index].device_index: %d\n",
			line_number++, G_input_device_index, G_input_devices[G_input_device_index].device_index);
	}
	//Check the current condition of the audio devices to determine if the Multus device as been set as default
	if (sound_ini_status == 1) {
		for (int j = 0; j < devCount; j++)
		{
			lpInfo = Pa_GetDeviceInfo((PaDeviceIndex)j);
			if (lpInfo->hostApi == myAudioAPItype) {
				Check_default_device((PaDeviceIndex)j);
			}
		}
	}

	while (G_all_threads_run)
	{
		Sleep(100);
	}
	err = Pa_CloseStream(stream);
	if (err != paNoError) goto error;
	Sleep(100);
	Pa_Terminate();
	exit(0);

error:
	Pa_Terminate();
	print_time();
	fprintf(G_fp_logfile, "[%d] An error occured while closing the portaudio stream\n",line_number++);
	print_time();
	fprintf(G_fp_logfile, "[%d] Error number: %d\n",line_number++, err);
	print_time();
	fprintf(G_fp_logfile, "[%d] Error message: %s\n", line_number++,Pa_GetErrorText(err));
	MessageBoxA(NULL, "SDRcore-trans initialization FAILED.  Send logs to Multus SDR, LLC", "SDRcore-trans", 
		MB_OK | MB_ICONEXCLAMATION);
	G_all_threads_run = 0;
	exit(0);
}

