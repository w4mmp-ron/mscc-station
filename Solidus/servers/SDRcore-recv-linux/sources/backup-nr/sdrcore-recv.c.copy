/********************** SDRdemo *************************
*	SDR Demo using Multus SDR LLC Hardware				*
*	SDRcore engine inclusions provided under license	*
*	from James L Barber, AKA Silicon Pixels, Spokane	*
*	Radio et al											*
*********************************************************/
#define _CRT_SECURE_NO_WARNINGS 1
#define RON 1
#ifdef RON
//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#include "version.h"
#endif
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <windows.h>
#include "sdrcore.h"
#include "portaudio.h"
#include "dsputils.h"
#include "blanker.h"

#ifdef RON

	#include "pthread.h"
	#include "sched.h"
	#include "semaphore.h"
	#include "extern.h"
#endif

#define PA_SAMPLE_TYPE      paFloat32
typedef float SAMPLE;
int	keyChanged = 0;
sp_float *inbuffer, *outbuffer;
sp_float wold = 0;
sp_cplx incplx[4097];
sp_cplx outcplx[4096];
float volumeLevel = 0.0f; //Start with the volume muted.

/****** NOTE: Declared external in other modules and used as globals ******/
state mystate;
calstate mycalstate;
panadapter_buffer panbuffer;
agc_state agcstate;
cs_state csstate;
blanker_state nbstate;
anotch_state anstate;
/**************************************************************************/

PaStream *stream;
const PaDeviceInfo* lpInfo;
PaStreamParameters inputParameters, outputParameters;

#ifdef RON
#define MAX_OUTPUT_DEVICES 50
//For the UDP_Thread
pthread_t p_UDP_thread;
int UDP_thread_rc;
HANDLE UDP_thread_Win32;

//For the flusher Thread
pthread_t p_Flusher_thread;
int Flusher_thread_rc;
HANDLE Flusher_thread_Win32;

//For the S_meter Thread
pthread_t p_Smeter_thread;
int Smeter_thread_rc;
HANDLE Smeter_thread_Win32;

//For the Panadapter Thread
pthread_t p_Panadapter_thread;
int Panadapter_thread_rc;
HANDLE Panadapter_thread_Win32;

FILE *G_fp_logfile;
int G_all_threads_run = 0;
int line_number = 0;
long long G_keep_alive = 0;

struct output_devices G_output_devices[MAX_OUTPUT_DEVICES];
#endif


//int main(void);

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
		framesToComplex(inbuffer, incplx, outcplx, framesPerBuffer);
				
		/**************************** IF SHIFT -12000 Hz *********************************/
		//ifShiftDown(incplx, framesPerBuffer);
		complex_shift(incplx, framesPerBuffer);

		/********************** Run calibration DSP, if engaged *************************/
		if (mycalstate.calStart == TRUE) doRxCalibrate(incplx, framesPerBuffer);
		
		/**************************** DO THE RADIO THING *********************************/
		fastconv(incplx, outcplx, (int)framesPerBuffer);

		/********************** Run AGC *************************************************/
		if(!AGC_Initializing) doAGC(outcplx, (int)framesPerBuffer);

		/********************** Run auto-notch *****************************************/
		if (anstate.enabled) anotch(outcplx, (int)framesPerBuffer);

		/*
			De-mux samples back into packed I-Q-I-Q-I-Q format. In case there's any question,
			Q should lead I by 90 degrees.
		*/
	
		for (i = 0; i < framesPerBuffer; i++)
		{
			outcplx[i].real *= volumeLevel;
			*outbuffer = outcplx[i].real;
			outbuffer++;
			outcplx[i].imag *= volumeLevel;
			*outbuffer = outcplx[i].imag;
			outbuffer++;
		}
	}

	return paContinue;
}

int manage_stream(int start_stop,int device,int channels) {
	PaError err;
	int status = 0;
	static int current_device = 0;
	static int current_channels = 0;

	if (start_stop == 0) {
		current_device = device;
		current_channels = channels;
	}

	switch (start_stop) {
	case 1:
		print_time();
		fprintf(G_fp_logfile,"[%d] Main Thread -> manage_stream -> Starting Stream.  Device: %d, Channels %d\n", line_number++,current_device, current_channels);
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
			fprintf(G_fp_logfile,"[%d] Main Thread -> manage_stream -> Open Stream Failed: PA ERROR: %s\n", 
				line_number++,lpError->errorText);
			MessageBoxA(NULL, "Pa Open Stream FAILED. Send logs to Multus SDR, LLC", "SDRcore-recv", MB_ICONASTERISK | MB_OK);
			status = err;
		}
		else {
			err = Pa_StartStream(stream);
			if (err != paNoError) {
				status = err;
				const PaHostErrorInfo* lpError = Pa_GetLastHostErrorInfo();
				print_time();
				fprintf(G_fp_logfile,"[%d] Main Thread -> manage_stream -> Start Stream Failed: PA ERROR: %s\n", 
					line_number++,lpError->errorText);
				MessageBoxA(NULL, "Pa Start Stream FAILED. Send logs to Multus SDR, LLC", "SDRcore-trans", MB_ICONASTERISK | MB_OK);
			}
		}
		break;
	case 0:
		print_time();
		fprintf(G_fp_logfile,"[%d] Main Thread -> manage_stream -> Stoping Stream.  New device: %d, New Channels %d\n",line_number++,device, channels);
		Pa_StopStream(stream);
		Pa_CloseStream(stream);
		if (channels > 2) {
			print_time();
			fprintf(G_fp_logfile, "[%d] Main Thread -> manage_stream -> Device %d -> Channel Number: %d greater than 2. Setting channels to 2.\n",
				line_number++, device, channels);
			channels = 2;
		}
		outputParameters.device = device;
		outputParameters.channelCount = channels;      
		outputParameters.sampleFormat = PA_SAMPLE_TYPE;
		outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
		outputParameters.hostApiSpecificStreamInfo = NULL;
		break;
	}
	return status;
}

/************************************* MAIN *********************************************/
int main(int argc, char **argv)
{
	//PaStreamParameters inputParameters, outputParameters;
	//PaStream *stream;
	PaError err;
	int j;
	int key = 0;
	float temp;

	/************ DSP parameters *****************/
	// See sdrcore.h for definitions
	mystate.nfft = FFTSIZE;							// # of FFT bins
	mystate.filtertaps = FILTERTAPS;				// # of taps in IF filterl
	mystate.samplerate = SAMPLERATE;				// Overall RX sampling rate 
	mystate.frames = mystate.nfft - mystate.filtertaps;  // frames per buffer
	mystate.opmode = MODE_LSB;
	setFilterOffsets(75.0f, 2700.0f);
	mystate.iqReversed = 1;					// Yes Virginia, we're reversed...
	mystate.iMult = 1.0f;					// I and Q mag adjustments = preset to "no action"
	mystate.qMult = 1.0f;
	mycalstate.freq_high = 598.0f;
	mycalstate.freq_center = 600.0f;
	mycalstate.freq_high = 602.f;
	temp = 0.0f;

	/***** Pre-flight checks *****/
#ifdef STINKER
	// do something obnoxious and get out - compile defines in sdrcore.c not set correctly
#ifdef WIN32
	MessageBoxA(NULL, "Pre-processor DEFINES not set correctly in sdrcore.c . Check PERFORMANCELEVEL", "SDRcore STINKER alert", MB_ICONSTOP);
#elseif
	printf("*** Pre-processor DEFINES not set correctly in sdrcore.c . Check PERFORMANCELEVEL \n***");
	getchar;
#endif
	return -1;
#endif // !STINKER
	
	/******* Init DSP ******/
	mycalstate.Cycle_Count = 65536;//This the default cycle count. 
	mycalstate.calbuffer = calloc(mycalstate.Cycle_Count, sizeof(sp_cplx));	// buffer for LO calibration - NOTE: Allocated before audio threads start, so should not blow up... ... !
	initDSP();
	Sleep(100);

#ifdef RON
	long t = 0;
	int log_status;
	int ini_status;
	HANDLE sdrcore_recv_handle;
	int priority_status;

	sdrcore_recv_handle = NULL;
	sdrcore_recv_handle = GetCurrentProcess();
	if (sdrcore_recv_handle != NULL) {
		priority_status = SetPriorityClass(sdrcore_recv_handle, ABOVE_NORMAL_PRIORITY_CLASS);
	}
	log_status = Open_log_file();
	if (log_status) {
		print_time();
		fprintf(G_fp_logfile,"[%d] SDRcore Log file opened\n",line_number++);
	}
	else {
		MessageBox(NULL, L"Log file open failed. SDRcore-recv is terminating", L"SDRcore-recv", MB_OK);
		exit(1);
	}
	G_all_threads_run = 1;
	print_time(0);
	fprintf(G_fp_logfile, "[%d] sdrcore-recv -> starting -> Compile Date %s, Compile Time %s \n", line_number++, COMPILE_DATE, COMPILE_TIME);

	//Now start the UDP Thread
	print_time();
	fprintf(G_fp_logfile, "[%d] SDRcore Starting UDP Thread\n", line_number++);
	UDP_thread_rc = pthread_create(&p_UDP_thread, NULL, UDP_Thread, (void *)t);
	if (UDP_thread_rc) {
		print_time();
		fprintf(G_fp_logfile, "[%d} Start up of UDP thread failed, return code from pthread_create() is %d\n", line_number++, UDP_thread_rc);
	}

	print_time();
	fprintf(G_fp_logfile,"[%d] SDRcore Starting Flusher Thread\n",line_number++);
	Flusher_thread_rc = pthread_create(&p_Flusher_thread, NULL, Flusher_thread, (void *)t);
	if (Flusher_thread_rc) {
		print_time();
		fprintf(G_fp_logfile,"[%d] Start up of Flusher thread failed, return code from pthread_create() is %d\n",line_number++,Flusher_thread_rc);
	}
	print_time();
	fprintf(G_fp_logfile, "[%d] SDRcore Starting S-meter Thread\n", line_number++);
	Smeter_thread_rc = pthread_create(&p_Smeter_thread, NULL, Send_smeter_thread, (void *)t);
	if (Smeter_thread_rc) {
		print_time();
		fprintf(G_fp_logfile, "[%d] Start up of S-meter thread failed, return code from pthread_create() is %d\n", line_number++, Smeter_thread_rc);
	}
	print_time();
	fprintf(G_fp_logfile, "[%d] SDRcore Starting Panadapter Thread\n", line_number++);
	Panadapter_thread_rc = pthread_create(&p_Panadapter_thread, NULL, Panadapter_thread, (void *)t);
	if (Panadapter_thread_rc) {
		print_time();
		fprintf(G_fp_logfile, "[%d] Start up of Panadapter thread failed, return code from pthread_create() is %d\n", line_number++, Smeter_thread_rc);
	}
	ini_status = check_for_sound_ini_file();
	if (ini_status == 1) {
		print_time();
		fprintf(G_fp_logfile,"[%d] sdrcore_recv.ini file exists\n",line_number++);
	}
	else {
		print_time();
		fprintf(G_fp_logfile, "[%d] sdcore_recv.ini file does not exist. File will be created\n",line_number++);
	}
#endif
	/******************************************************************/
	err = Pa_Initialize();
	if (err != paNoError) return -1; //goto error;

	// Get # of host API's supported on THIS machine at THIS time.
	const PaHostApiInfo* lpApiInfo;
	PaHostApiIndex hostApiCount;
	int apiTypeIdMME = 9999;
	int apiTypeIdDSOUND = 9999;
	hostApiCount = Pa_GetHostApiCount();

	for (j = 0; j < hostApiCount; j++)
	{
		lpApiInfo = Pa_GetHostApiInfo(j);
		if (!strncmp("MME", lpApiInfo->name, 3))
		{
			apiTypeIdMME = j;
		}
	}

	PaDeviceIndex devCount = Pa_GetDeviceCount();
	//const PaDeviceInfo* lpInfo;
	PaDeviceIndex recordDevice = 0;
	PaDeviceIndex playDevice = 0;

	// Get device count, go through devices looking for "Line" as an MME input device.
	for (int j = 0; j < devCount; j++)
	{
		lpInfo = Pa_GetDeviceInfo((PaDeviceIndex)j);
#ifdef RON
		if (lpInfo->hostApi == apiTypeIdMME) {
			if (ini_status == 0) {
				build_output_devices((PaDeviceIndex)j);
			}
		}
#endif

		if (strstr(lpInfo->name, "Multus"))
		{
			if (lpInfo->maxInputChannels == 2)
			{
				if (lpInfo->hostApi == apiTypeIdMME)		// Force MME API - lowest latency sometimes... ?
				{
					print_time();
					fprintf(G_fp_logfile,"[%d] Multus SDR device found: %s\n", line_number++,lpInfo->name);
					recordDevice = (PaDeviceIndex)j;
#ifndef RON
					break;
#endif
				}
			}
		}
	}
		
	if (recordDevice == 0)
	{
		print_time();
		fprintf(G_fp_logfile, "[%d] No Multus SDR Device found. Please check hardware and try again.\n",line_number++);
		fflush(G_fp_logfile);
		goto error;		// use 1 to indicate processing didn't end normally
	}
	
	//inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
	inputParameters.device = recordDevice;
	if (inputParameters.device == paNoDevice) {
		print_time();
		fprintf(G_fp_logfile, "[%d] Error: Multus SDR Device not functioning.\n",line_number++);
		goto error;
	}
	inputParameters.channelCount = 2;       /* stereo input */
	inputParameters.sampleFormat = PA_SAMPLE_TYPE;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
														   //outputParameters.device = playDevice;
	if (outputParameters.device == paNoDevice) {
		print_time();
		fprintf(G_fp_logfile, "[%d] Error: No default output device.\n",line_number++);
		goto error;
	}

#ifdef RON
	//Now update the INI file if it does not exist
	if (ini_status == 0) {
		update_sound_ini();
	}
	//Initialize the output devices structure
	init_sound_ini();

#endif
#ifdef RON
	//Now Start the audio stream
	//status = manage_stream(1, 0, 0);
	//if (status) {
	//	err = status;
	//	goto error;
	//}
#else
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
		printf("PA ERROR: %s\n", lpError->errorText);
		getchar();
		goto error;
	}

	err = Pa_StartStream(stream);
	if (err != paNoError) goto error;
#endif
	
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
	fprintf(G_fp_logfile, "[%d] An error occured while using the portaudio stream\n", line_number++);
	print_time();
	fprintf(G_fp_logfile, "[%d] Error number: %d\n", line_number++, err);
	print_time();
	fprintf(G_fp_logfile, "[%d] Error message: %s\n", line_number++, Pa_GetErrorText(err));
	MessageBoxA(NULL, "SDRcore-recv initialization FAILED.  Send logs to Multus SDR, LLC", "SDRcore-recv",
		MB_OK | MB_ICONEXCLAMATION);
	exit(0);
}

