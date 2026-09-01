#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "wavfmt.h"
#include "rtdspc.h"

/* code to send samples to a WAV type file format */

/* define BITS16 if want to use 16 bit samples */

/* sendout - send sample to disk to simulate realtime output */

    static FILE *fp_sendwav = NULL;
    static DWORD samples_sent = 0L;  /* used by flush for header */

/* WAV format header init */
    static WAVE_HDR wout = { "RIFF", 0L };  /* fill size at flush */
    static CHUNK_HDR cout = { "WAVEfmt " , sizeof(WAVEFORMAT) };
    static DATA_HDR dout = { "data" , 0L };  /* fill size at flush */
    static WAVEFORMAT wavout = { 1, 1, 0L, 0L, 1, 8 };

    extern WAVE_HDR win;
    extern CHUNK_HDR cin;
    extern DATA_HDR din;
    extern WAVEFORMAT wavin;

void sendout(float x)
{
    int BytesPerSample;
    short int j;

/* open output file if not done in previous calls */
    if(!fp_sendwav) {
        //char s[80];
        //printf("\nEnter output *.WAV file name ? ");
        //gets(s);

		char *s = { "B:\\JUNK\\20mSSBout.wav" };

        fp_sendwav = fopen(s,"wb");
        if(!fp_sendwav) {
            printf("\nError opening output *.WAV file in SENDOUT\n");
            exit(1);
        }
/* write out the *.WAV file format header */

#ifdef BITS16
        wavout.wBitsPerSample = 16;
        wavout.nBlockAlign = 2;
        printf("\nUsing 16 Bit Samples\n");
#else
        wavout.wBitsPerSample = 8;
#endif

        wavout.nSamplesPerSec = SAMPLE_RATE;
        BytesPerSample = (int)ceil(wavout.wBitsPerSample/8.0);
        wavout.nAvgBytesPerSec = BytesPerSample*wavout.nSamplesPerSec;

        fwrite(&wout,sizeof(WAVE_HDR),1,fp_sendwav);
        fwrite(&cout,sizeof(CHUNK_HDR),1,fp_sendwav);
        fwrite(&wavout,sizeof(WAVEFORMAT),1,fp_sendwav);
        fwrite(&dout,sizeof(DATA_HDR),1,fp_sendwav);
    }
/* write the sample and check for errors */

/* clip output to 16 bits */
    j = (short int)x;
    if(x > 32767.0) j = 32767;
    else if(x < -32768.0) j = -32768;

#ifdef BITS16
    j ^= 0x8000;

    if(fwrite(&j,sizeof(short int),1,fp_sendwav) != 1) {
        printf("\nError writing 16 Bit output *.WAV file in SENDOUT\n");
        exit(1);
    }
#else
/* clip output to 8 bits */
    j = j >> 8;
    j ^= 0x80;

    if(fputc(j,fp_sendwav) == EOF) {
        printf("\nError writing output *.WAV file in SENDOUT\n");
        exit(1);
    }
#endif

    samples_sent++;
}

/* routine for flush - must call this to update the WAV header */

void flush()
{
    int BytesPerSample;

    BytesPerSample = (int)ceil(wavout.wBitsPerSample/8.0);
    dout.data_size=BytesPerSample*samples_sent;

    wout.chunk_size=
        dout.data_size+sizeof(DATA_HDR)+sizeof(CHUNK_HDR)+sizeof(WAVEFORMAT);

/* check for an input WAV header and use the sampling rate, if valid */
    if(_strnicmp(win.chunk_id,"RIFF",4) == 0 && wavin.nSamplesPerSec != 0) {
        wavout.nSamplesPerSec = wavin.nSamplesPerSec;
        wavout.nAvgBytesPerSec = BytesPerSample*wavout.nSamplesPerSec;
    }

    fseek(fp_sendwav,0L,SEEK_SET);
    fwrite(&wout,sizeof(WAVE_HDR),1,fp_sendwav);
    fwrite(&cout,sizeof(CHUNK_HDR),1,fp_sendwav);
    fwrite(&wavout,sizeof(WAVEFORMAT),1,fp_sendwav);
    fwrite(&dout,sizeof(DATA_HDR),1,fp_sendwav);
}
