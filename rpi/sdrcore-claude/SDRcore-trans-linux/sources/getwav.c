#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <conio.h>
#include "wavfmt.h"
//#include "rtdspc.h"

/* code to get samples from a WAV type file format */

/* getinput - get one sample from disk to simulate realtime input */

/* input WAV format header with null init */
    WAVE_HDR win = { "", 0L };
    CHUNK_HDR cin = { "", 0L };
    DATA_HDR din = { "", 0L };
    WAVEFORMAT wavin = { 1, 1, 0L, 0L, 1, 8 };

/* global number of samples in data set */
    unsigned long int number_of_samples = 0;

float getinput()
{

    static FILE *fp_getwav = NULL;
    static channel_number = 0;
    short int int_data[4];         /* max of 4 channels can be read */
    unsigned char byte_data[4];    /* max of 4 channels can be read */
    short int j;
    int i;

/* open input file if not done in previous calls */
    if(!fp_getwav) {
        //char s[80];
        //printf("\nEnter input .WAV file name ? ");
        //gets(s);

		char *s = { "B:\\JUNK\\20mSSBmix.wav" };
        fp_getwav = fopen(s,"rb");
        if(!fp_getwav) {
            printf("\nError opening *.WAV input file in GETINPUT\n");
            exit(1);
        }

/* read and display header information */
        fread(&win,sizeof(WAVE_HDR),1,fp_getwav);
        printf("\n%c%c%c%c",
            win.chunk_id[0],win.chunk_id[1],win.chunk_id[2],win.chunk_id[3]);
        printf("\nChunkSize = %ld bytes",win.chunk_size);
        if(_strnicmp(win.chunk_id,"RIFF",4) != 0) {
            printf("\nError in RIFF header\n");
            exit(1);
        }

        fread(&cin,sizeof(CHUNK_HDR),1,fp_getwav);
        printf("\n");
        for(i = 0 ; i < 8 ; i++) printf("%c",cin.form_type[i]);
        printf("\n");
        if(_strnicmp(cin.form_type,"WAVEfmt ",8) != 0) {
            printf("\nError in WAVEfmt header\n");
            exit(1);
        }

        if(cin.hdr_size != sizeof(WAVEFORMAT)) {
            printf("\nError in WAVEfmt header\n");
            exit(1);
        }

        fread(&wavin,sizeof(WAVEFORMAT),1,fp_getwav);
        if(wavin.wFormatTag != WAVE_FORMAT_PCM) {
            printf("\nError in WAVEfmt header - not PCM\n");
            exit(1);
        }
        printf("\nNumber of channels = %d",wavin.nChannels);
        printf("\nSample rate = %ld",wavin.nSamplesPerSec);
        printf("\nBlock size of data = %d bytes",wavin.nBlockAlign);
        printf("\nBits per Sample = %d\n",wavin.wBitsPerSample);

/* check channel number and block size are good */
        if(wavin.nChannels > 4 || wavin.nBlockAlign > 8) {
            printf("\nError in WAVEfmt header - Channels/BlockSize\n");
            exit(1);
        }

        fread(&din,sizeof(DATA_HDR),1,fp_getwav);
        printf("\n%c%c%c%c",
            din.data_type[0],din.data_type[1],din.data_type[2],din.data_type[3]);
        printf("\nData Size = %ld bytes",din.data_size);

/* set the number of samples (global) */
        number_of_samples = din.data_size/wavin.nBlockAlign;
        printf("\nNumber of Samples per Channel = %ld\n",number_of_samples);

        if(wavin.nChannels > 1) {
          do {
            printf("\nError Channel Number [0..%d] - ",wavin.nChannels-1);
            i = _getche() - '0';
            if(i < (4-'0')) exit(1);
          } while(i < 0 || i >= wavin.nChannels);
          channel_number = i;
        }
    }

/* read data until end of file */
    if(wavin.wBitsPerSample == 16) {
        if(fread(int_data,wavin.nBlockAlign,1,fp_getwav) != 1) {
            flush(); /* flush the output when input runs out */
            exit(1);
        }
        j = int_data[channel_number];
    }
    else {
        if(fread(byte_data,wavin.nBlockAlign,1,fp_getwav) != 1) {
            flush(); /* flush the output when input runs out */
            exit(1);
        }
        j = byte_data[channel_number];
        j ^= 0x80;
        j <<= 8;
    }

    return((float)j);
}
