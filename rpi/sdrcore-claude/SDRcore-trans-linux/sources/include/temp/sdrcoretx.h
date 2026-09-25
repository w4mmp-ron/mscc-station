/************************************************
* Realtime DSP System - Much used and abused	*
* (1992 - 2018) James L Barber					*
*************************************************/

#ifndef SDRCORE_HEADER
#define SDRCORE_HEADER

#define PI					(3.14159265358979323846)
#define TPI					(6.283185307179586476925286766559)
#define FPI					(12.566370614359172953850573533118)
#define SAMPLERATE			96000.0f
#define MODE_LSB 1
#define MODE_USB 2
#define MODE_AM 3
#define MODE_TUNE 4
#define MODE_CW 5

#define MAXDELAY		1000		// 20.833 mS @ Fs = 48K

#ifdef WIN32
#ifndef TRUE
#define TRUE			1
#define FALSE			0
#endif
#endif

#define WINDBG			0
#define TWOTONE			0

typedef float sp_float;

/* sp_cplx STRUCTURE */
typedef struct {
    float real, imag;
} sp_cplx;

typedef struct {
	uint8_t overdriven;
	uint8_t twoToneFlag;
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
	sp_float rxAgcEnvelope;
	sp_float txPower;
	sp_float amCarrier;
} state;

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
} alc_state;

typedef struct {
	uint8_t enabled;
	sp_float attackms;
	sp_float releasems;
	sp_float negramp;
	sp_float posramp;
	sp_float pregain;
	sp_float maxgain;
	sp_float maxval;
	sp_float gain;
	sp_float targetlevel;
} micproc_state;


/* real-time DSP C code include file */
//void initDSP(int firstRunFlag);
void initDSP();
void fastconv(sp_cplx *in, sp_cplx *out, int frames);

#endif	// SDRCORE_HEADER