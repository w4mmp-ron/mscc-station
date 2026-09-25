/* include file to define WAV format header */

typedef struct {
    char chunk_id[4];
    unsigned long int chunk_size;
} WAVE_HDR;

typedef struct {
    char form_type[8];
    unsigned long int hdr_size;
} CHUNK_HDR;

typedef unsigned short int WORD;
typedef unsigned long int DWORD;

/* specific waveform format structure for PCM data */
typedef struct waveformat_tag {
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo, etc.) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
    WORD    wBitsPerSample;
} WAVEFORMAT;

typedef struct {
    char data_type[4];
    unsigned long int data_size;
} DATA_HDR;

/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM     1
