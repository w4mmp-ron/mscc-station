#ifndef WSFIRGEN_HDR
#define WFFIRGEN_HDR
/*
Windowed Sinc FIR Generator
Modified from Bob's musicdsp.org contribution - JL Barber

Usage:
Lowpass:	wsfirLP(h, N, WINDOW, fc)
Highpass:	wsfirHP(h, N, WINDOW, fc)
Bandpass:	wsfirBP(h, N, WINDOW, fc1, fc2)
Bandstop:	wsfirBS(h, N, WINDOW, fc1, fc2)

where:
h (float[N]):	filter coefficients will be written to this array
N (int):		number of taps
WINDOW (int):	Window (W_BLACKMAN, W_HANNING, or W_HAMMING)
fc (float):	cutoff (0 < fc < 0.5, fc = f/fs)
--> for fs=48kHz and cutoff f=12kHz, fc = 12k/48k => 0.25

fc1 (float):	low cutoff (0 < fc < 0.5, fc = f/fs)
fc2 (float):	high cutoff (0 < fc < 0.5, fc = f/fs)


Windows included here are Blackman, Hanning, and Hamming. Other windows	can be
added by doing the following:
1. "Window type constants" section: Define a global constant for the new window.
2. wsfirLP() function: Add the new window as a case in the switch() statement.
3. Create the function for the window

For windows with a design parameter, such as Kaiser, some modification
will be needed for each function in order to pass the parameter.
*/

#include <math.h>

// Function prototypes
void wsfirLP(sp_float h[], int N, int WINDOW, sp_float fc);
void wsfirHP(sp_float h[], int N, int WINDOW, sp_float fc);
void wsfirBS(sp_float h[], int N, int WINDOW, sp_float fc1, sp_float fc2);
void wsfirBP(sp_float h[], int N, int WINDOW, sp_float fc1, sp_float fc2);
void genSinc(sp_float sinc[], int N, sp_float fc);
void wBlackman(sp_float w[], int N);
void wHanning(sp_float w[], int N);
void wHamming(sp_float w[], int N);

// Window type contstants
#define W_BLACKMAN		1
#define W_HANNING		2
#define W_HAMMING		3

#endif	// WSFIRGEN_HEADER