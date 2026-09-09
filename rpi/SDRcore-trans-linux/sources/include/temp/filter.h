#ifndef FILTER_HEADER
#define FILTER_HEADER

void fft(sp_cplx *x, int m);
void ifft(sp_cplx *x, int m);
void dcblock(sp_cplx *samps, state *state, int nsamps);


#endif // FILTER_HEADER