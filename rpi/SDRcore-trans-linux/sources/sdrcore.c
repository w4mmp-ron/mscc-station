/**************** CORE DSP FOR HF SDR RECEIVER **************
*               (1992 - 2018)  James L Barber				*
*************************************************************/
#include "extern.h"

static int          i, j;
static float        tempflt;
static float		gainmult = 1.0f;
static float		agcE, agcR, agcA, agcV, agcT;
static float		windowtaps[8192];
static sp_cplx		cptmp;
static sp_cplx		input_save[2048];		//allocate for max # of filter taps
static int			ifShiftSamps = 0.0f;
static int			firstentry = 0;
static sp_float		hfilt[32768];
static sp_cplx		filt[32768];
static sp_cplx		samp[32768];

/******* External C&C struct, defined in sdrcore.c *******/
extern state mystate;

void initDSP()
{

	if (firstentry == 0)
	{
		firstentry = 1;		// only this block at startup!
		// Get simple AGC limiter set up. (more gooder things later)
		agcA = pow(0.01f, 1.0f / (10.0f * mystate.samplerate * 0.001f));
		agcR = pow(0.01f, 1.0f / (3000.0f * mystate.samplerate * 0.001f));
		agcE = 0.0f;
		agcT = 0.001f;

		// Calculate FFT length order (what power of 2)
		mystate.fftOrder = log((sp_float)mystate.nfft) / log(2.0f);
	}

	/*  Generate "IF" filter using limits set in state struct
		Note this will run at startup, and any time initDSP is called therafter. 
	*/
	agcE = 0.001f;			// Stack up AGC envelope to suppress any popping from filter change
	memset(hfilt, 0, sizeof(sp_cplx) * mystate.nfft);
	memset(filt, 0, sizeof(sp_cplx) * mystate.nfft);

	wsfirBP(hfilt, mystate.filtertaps, W_HANNING, mystate.lastHRFiltLow / mystate.samplerate,
		mystate.lastHRFiltHigh / mystate.samplerate);

	tempflt = 1.0f / mystate.nfft;

	for (i = 0; i < mystate.filtertaps; i++)
		filt[i].real = tempflt * hfilt[i];

	//fft(filt, mystate.fftOrder);
	jimfft(filt, mystate.nfft);
	mystate.initDSPflag = FALSE;

}

/******************* IF shift, opposite sideband suppression, demo etc ****************/
void fastconv(sp_cplx *in, sp_cplx *out, int frames)
{
	float tmp;
	int isamps = 0;
	int osamps = 0;
	int sbcut = 0;
	static int meterblocks = 0;
	sp_float magaccum = 0.0f;
	sp_float mag;
	sp_float fsbcut = 0.0f;

	// re-init DSP if requested
	if (mystate.initDSPflag) initDSP();

	/* do FFT of samples */
	//fft(samp, mystate.fftOrder);		// uses dynamic allocation, no-no
	jimfft(samp, mystate.nfft);			// alternate version

	/**************** IF SHIFTER **************************************/
	ifShiftSamps = mystate.nfft / 4;			// 1/4 of FFT size @ 4096, shift 1/4 or 12 kHz @ Fs = 48K
	/**************************************************************/

	// bsamps = (3000 / 48000) * 4096
	fsbcut = (mystate.filtHighHz / mystate.samplerate) * (sp_float)mystate.nfft;
	sbcut = (int)fsbcut + 50;

	// remove the opposite sideband
	if (mystate.opmode == MODE_USB)
	{
		for (i = sbcut; i < mystate.nfft; i++)
		{
			samp[i].real = 0.0f;
			samp[i].imag = 0.0f;
		}
	}

	// remove the opposite sideband
	if ((mystate.opmode == MODE_LSB)) 
	{
		for (i = 0; i < sbcut; i++)
		{
			samp[i].real = 0.0f;
			samp[i].imag = 0.0f;
		}

		for (i = sbcut*2; i < mystate.nfft/2 ; i++)
		{
			samp[i].real = 0.0f;
			samp[i].imag = 0.0f;
		}
	}

	if ((mystate.opmode == MODE_AM))
	{
		for (i =0; i< (mystate.nfft/2)-sbcut; i++)
		{
			samp[i].real = 0.0f;
			samp[i].imag = 0.0f;
		}

		for (i = (mystate.nfft/2); i<(mystate.nfft / 2) + sbcut; i++)
		{
			samp[i].real = 0.0f;
			samp[i].imag = 0.0f;
		}
	}
	
	/* Multiply the two transformed sequences */
	/* swap the real and imag outputs to allow a forward FFT instead of inverse FFT */

	for (i = 0; i < mystate.nfft; i++) {
		tempflt = samp[i].real * filt[i].real
			- samp[i].imag * filt[i].imag;
		samp[i].real = samp[i].real * filt[i].imag
			+ samp[i].imag * filt[i].real;
		samp[i].imag = tempflt;
	}

	/* Inverse fft the multiplied sequences */
	//fft(samp, mystate.fftOrder);
	jimfft(samp, mystate.nfft);
	/* Write the result out */
	/* because a forward FFT was used for the inverse FFT,
	the output is in the imag part */
	
	osamps = 0;
	for (i = mystate.filtertaps; i < mystate.nfft; i++)
	{
		mag = sqrt((samp[i].real * samp[i].real) + (samp[i].imag * samp[i].imag));
		magaccum += (mag * 100000.0f);

		/************** SIMPLE AGC PROCESSOR *************/
		agcV = fabs(samp[i].real);
		if (agcV > agcE) agcE = agcA * (agcE - agcV) + agcV;
		else agcE = agcR * (agcE - agcV) + agcV;
		if (agcE < agcT) agcE = agcT;

		mystate.rxAgcEnvelope = agcE;

		// Simple AM demod - more work required here
		if (mystate.opmode == MODE_AM)
		{
			samp[i].real = mag;
			samp[i].imag = mag;
		}

		out[osamps].real = samp[i].real / (agcE * 5.0f);
		out[osamps].imag = samp[i].real / (agcE * 5.0f);
		osamps++;
	}

	meterblocks++;
	if (meterblocks >= 2)
	{
		magaccum /= ((sp_float)mystate.nfft - mystate.filtertaps);
		printf("%14.8f MAG  %14.8f AGC\n", magaccum, agcE);
		mystate.avgRxSignalMag = magaccum;
		magaccum = 0.0f;
		meterblocks = 0;
	}

	/* overlap the last FILTER_LENGTH-1 input data points in the next FFT */
	for (i = 0; i < mystate.filtertaps; i++) {
		samp[i].real = input_save[i].real;
		samp[i].imag = input_save[i].imag;
	}

	isamps = 0;
	for (; i < mystate.nfft - mystate.filtertaps; i++) {
		samp[i].real = in[isamps].real * gainmult;
		samp[i].imag = in[isamps].imag * gainmult;
		isamps++;

		if (mystate.iqReversed)		// <----- reverse I/Q input channels if reverse flag set.
		{
			tmp = samp[i].real;
			samp[i].real = samp[i].imag;
			samp[i].imag = tmp;
		}

	}

	/* save the last FILTER_LENGTH points for next time */
	for (j = 0; j < mystate.filtertaps; j++, i++) {
		samp[i].real = in[isamps].real * gainmult;
		samp[i].imag = in[isamps].imag * gainmult;
		isamps++;

		if (mystate.iqReversed)		// <----- reverse I/Q input channels if reverse flag set.
		{
			tmp = samp[i].real;
			samp[i].real = samp[i].imag;
			samp[i].imag = tmp;
		}

		input_save[j].real = samp[i].real;
		input_save[j].imag = samp[i].imag;
	}

}
