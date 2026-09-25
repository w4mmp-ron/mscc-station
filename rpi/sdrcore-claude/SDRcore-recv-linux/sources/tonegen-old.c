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

extern state mystate;
extern uint8_t G_mode;
extern int line_number;
extern FILE* G_fp_logfile;

float G_side_tone = 600.0f;
uint8_t G_key_down = 0;
void side_tone(sp_cplx* samps)
{
	static int beenhere = 0, active = 0, rampdir = 0;
	static float ramp, level, myfreq, oldfreq, phaseinc, phaseaccum;
	static float myramp;

	//float txsamplerate = 48000.0f;		// normally gotten from master struct
	float ramptimems = 10.0f;			// keying up/down ramp time in ms
	float txsamplerate = mystate.samplerate;
	int j;

	if (!beenhere)
	{
		beenhere = 1;
		float ramp = 1.0f / (txsamplerate * ramptimems);
		level = 0.0f;
		rampdir = 0;
		active = 0;
		ramp = 0.0f;
		oldfreq = 9999.99f;
	}

	// if key isn't down and shaping isn't previously active, return
	if ((G_key_down == 0) && (active == 0)) return;

	// new key down
	if ((G_key_down == 1) && (active == 0))
	{
		active = 1;
		rampdir = 1;
		myramp = 1.0f + ramp;

		if (oldfreq != G_side_tone)
		{
			phaseinc = (float)TPI / (txsamplerate / G_side_tone);
			phaseaccum = 0.0f;
			oldfreq = G_side_tone;
			rampdir = 1;				// ramp UP, because this is a new keydown command
		}
	}

	// still active from previous operation, ramping DOWN
	if ((G_key_down == 0) && (active == 1))
	{
		rampdir = 0;					// make sure we're in ramp DOWN mode
		myramp = ramp;
	}
	print_time();
	fprintf(G_fp_logfile, "[%d] side_tone -> G_key_down: %d G_mode: %c, G_side_tone: %f,samples: %d\n",
		line_number++, G_key_down, G_mode, G_side_tone,mystate.nfft);
	// finally, build tone samples
	for (j = 0; j < mystate.nfft; j++)
	{
		samps[j].real = (sp_float)sin(phaseaccum) * level;
		samps[j].imag = (sp_float)cos(phaseaccum) * level;

		phaseaccum += phaseinc;
		phaseaccum = (float)atan2(sin(phaseaccum), cos(phaseaccum));

		if (rampdir == 0)
		{
			myramp *= ramp;
			if (myramp <= 0.000001)
			{
				active = 0;
				break;		// break out if ramping down and level reaches "close enough" to zero
			}
		}

		// ramping UP, all samples get tone.
		if (rampdir == 1) myramp *= (1.0f + ramp);

	}
}
