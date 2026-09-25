#ifndef DSP_UTILS_HEADER
#define DSP_UTILS_HEADER

typedef struct {
	sp_float phaseinc;
	sp_float accum;
} cmstate;

void twoTone(sp_cplx *samps);
void cw_modulate(sp_cplx *samps);
void initMicProc();
void doMicProc(sp_cplx *samps, int nframes);
void initALC();
void doALC(sp_cplx *samps, int nframes);
void setFilterOffsets(sp_float filterSetLow, sp_float filterSetHigh);
void framesToComplex(sp_float *inframes, sp_cplx *incomplex, sp_cplx *outcomplex, int nframes, int inputchannels);
void ifShiftDown(sp_cplx *incomplex, int nframes);
void rect2polar(sp_cplx *samps, int nsamps);
void polar2rect(sp_cplx *samps, int nsamps);
void tune_modulate(sp_cplx *samps);
void ssb_modulate(sp_cplx *samps);
void am_modulate(sp_cplx *samps);
void null_modulate(sp_cplx *samps);
void rotateArray(sp_cplx a[], int n, int d);
void rotateByOneRight(sp_cplx a[], int n);
void rotateByOneLeft(sp_cplx a[], int n);
void jimfft(sp_cplx *samps, int n);

#endif