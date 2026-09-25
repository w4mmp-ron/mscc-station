/************************************************
* Realtime DSP System - Much used and abused	*
* (1992 - 2018) James L Barber					*
*************************************************/

#ifndef SDRCORE_HEADER
#define SDRCORE_HEADER

#include <stdint.h>

#define PI					(3.14159265358979323846)
#define TPI					(6.283185307179586476925286766559)
#define FPI					(12.566370614359172953850573533118)
#define VERY_SMALL_FLOAT	1.0e-30f
#define MODE_LSB 1
#define MODE_USB 2
#define MODE_AM 3
//#define PAN_REFRESH		8		// send a new panadapter buffer every PAN_REFRESH FFT's
#define PAN_REFRESH		4		// send a new panadapter buffer every PAN_REFRESH FFT's

#ifdef WIN32
#ifndef TRUE
#define TRUE			1
#define FALSE			0
#endif
#endif

#define WINDBG			0

/************* Set PERFORMANCELEVEL here. (read below for implications) *******************/
#define PERFORMANCELEVEL  4

#if  PERFORMANCELEVEL == 4		// 96K, max filter performance, lower latency
#define FFTSIZE		4096
#define FILTERTAPS	2048
#define SAMPLERATE	96000
#endif

#if  PERFORMANCELEVEL == 3		// 96K, lower filter performance, lowest latency
#define FFTSIZE		1024
#define FILTERTAPS	512
#define SAMPLERATE	96000
#endif

#if  PERFORMANCELEVEL == 2		// 48K, normal+ filter performance (original SDRcore default performance)
#define FFTSIZE		2048
#define FILTERTAPS	1024
#define SAMPLERATE	48000
#endif

#if  PERFORMANCELEVEL == 1		// 48K, normal filter performance, lower latency
#define FFTSIZE		1024
#define FILTERTAPS	512
#define SAMPLERATE	48000
#endif

#if  PERFORMANCELEVEL == 0		// 48K, lower filter performance, lowest latency available in this build
#define FFTSIZE		512
#define FILTERTAPS	256
#define SAMPLERATE	48000
#endif

#if  PERFORMANCELEVEL > 5		// 48K, lower filter performance, lowest latency available in this build
#define FFTSIZE		2048
#define FILTERTAPS	1024
#define SAMPLERATE	48000
#define STINKER
#endif

#if SAMPLERATE == 48000
#define MAXDELAY		720		// AGC delay line - (15 / 48000) = 720 samples
#else
#define MAXDELAY		1440
#endif


typedef float sp_float;

/* sp_cplx STRUCTURE */
typedef struct {
    float real, imag;
} sp_cplx;

typedef struct {
	int iqReversed;
	int	nfft;
	int filtertaps;
	int frames;
	int opmode;
	int fftOrder;
	int	initDSPflag;
	sp_float samplerate;
	sp_float filtLowHz;			// Note for TX these are NOT direct-set parameters
	sp_float filtHighHz;		// ""
	sp_float lastHRFiltLow;		// Last Human Readable filter LOW 
	sp_float lastHRFiltHigh;	// Last Human Readable filter HIGH
	sp_float lo1_phaseacc;
	sp_float lo1_phaseinc;
	sp_float lo1_freq;
	sp_float avgRxSignalMag;
	sp_float peakRxSignalDbm;
	sp_float rxAgcEnvelope;
	sp_float txPower;
	sp_float iMult;				// I magnitude offset for RX image rejection
	sp_float qMult;				// Q ""
} state;

typedef struct {
	uint32_t Cycle_Count;
	sp_float freq_low;
	sp_float freq_center;
	sp_float freq_high;
	int calStart;				// host intiates cal routine start (set to 1)
	int calReady;				// core sets to 1 when new cal data is ready
	unsigned int calMagLow;		// signal magnitude at 600 Hz minus NN Hz (We'll try 2 Hz first)
	unsigned int calMag;		// signal magnitude at 600 Hz
	unsigned int calMagHigh;	// signal magnitude at 600 Hz plus NN Hz
	sp_cplx *calbuffer;			// pointer to cal buffer (allocated off heap before audio threads start)
} calstate;

typedef struct {
	uint8_t panReady;
	uint8_t opcode;
	uint8_t sequence;
	uint16_t Y[800];
	sp_float *winbuf;
}panadapter_buffer;

typedef struct {
	sp_float attackms;
	sp_float releasems;
	sp_float envattackms;
	sp_float negramp;
	sp_float posramp;
	sp_float envramp;
	sp_float pregain;
	sp_float maxgain;
	sp_float maxval;
	sp_float gain;
	sp_float targetlevel;
	sp_float idelay[MAXDELAY];
	sp_float qdelay[MAXDELAY];
	int		iindex;
	int		oindex;
} agc_state;

typedef struct {
	sp_float loFreq;
	sp_float phaseInc;
	sp_float phaseAccum;
} cs_state;

typedef struct {
	sp_cplx delay[10000];
	sp_float gainramp[10000];
	sp_float threshold;
	sp_float attackms;
	sp_float attack;
	sp_float releasems;
	sp_float release;
	sp_float envelope;
	sp_float gain;
	int iindex;
	int oindex;
	int delaylen;
	int rampwidth;
	uint8_t	enabled;
}blanker_state;

typedef struct {
	uint8_t enabled;
}anotch_state;


/* real-time DSP C code include file */
void initDSP();
void fastconv(sp_cplx *in, sp_cplx *out, int frames);

#endif	// SDRCORE_HEADER