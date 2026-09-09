#ifndef BLANKER_HEADER
#define BLANKER_HEADER

void initBlanker(uint8_t overridedefaults, uint8_t enabled, float threshold, int16_t width);
void doBlanker(sp_cplx *samps, int nframes);

#endif //BLANKER_HEADER

