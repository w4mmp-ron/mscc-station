#ifndef DSP_UTILS_HEADER
#define DSP_UTILS_HEADER

typedef struct {
	sp_float phaseinc;
	sp_float accum;
} cmstate;

extern uint8_t AGC_Initializing;
void complex_shift(sp_cplx *samps, int nsamps);
void initAGC(float releaseOverride);
void doAGC(sp_cplx *samps, int nframes);
void doPanadapter(sp_cplx *fftbuf, int nframes);
void doRxCalibrate(sp_cplx *incomplex, int nframes);
sp_float goertzel_mag(int nsamps, sp_float freq, sp_cplx data[]);
void setFilterOffsets(sp_float filterSetLow, sp_float filterSetHigh);
void framesToComplex(sp_float *inframes, sp_cplx *incomplex, sp_cplx *outcomplex, int nframes);
void ifShiftDown(sp_cplx *incomplex, int nframes);
void rect2polar(sp_cplx *samps, int nsamps);
void polar2rect(sp_cplx *samps, int nsamps);
//void complex_mix(sp_cplx *samps, cmstate *state, int nsamps);
void rotateArray(sp_cplx a[], int n, int d);
void rotateByOneRight(sp_cplx a[], int n);
void rotateByOneLeft(sp_cplx a[], int n);
void anotch(sp_cplx *samps, int n);
void jimfft(sp_cplx *samps, int n);

#endif