#include <math.h>
#include <windows.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdint.h>

#include <stdio.h>

#include "pthread.h"
#include "sched.h"
#include "semaphore.h"

#include "extern.h"
#include "commands.h"

//#define MAX_AVERAGE 4
int G_band_marker_low = 0;
int G_band_marker_high = 0;

extern panadapter_buffer panbuffer;
#pragma pack(1)

struct {
	uint8_t opcode;
	uint8_t sequence;
	uint16_t Y[400];
	
}panbuffer_temp;

struct {
	struct {
		uint8_t opcode;
		uint8_t sequence;
		uint16_t output_buffer[400];
		}avg_buffer_output;
	struct {
		uint16_t avg_buffer_input[400];
	}buffer_input[4];
}panadaper_average[2];

void *Panadapter_thread(void *t){
	int x = 0;
	int send_size = 0;
	int Y_source_index = 0;
	int marker_low = 0;
	int marker_high = 0;
	int marker_center = 0;
	int i = 0;
	int average_count = 0;
	int sequence = 0;

	send_size = sizeof(panadaper_average->avg_buffer_output);
	print_time();
	fprintf(G_fp_logfile, "[%d] Panadapter_thread -> Started -> Delaying 5 seconds before the start of Panadapter Data\n", line_number++);
	Sleep(5000);  //Let the subsystem set up parameters before sending panadapter data
	print_time();
	fprintf(G_fp_logfile, "[%d] Panadapter_thread -> Started -> Panadapter Data Processing Started \n",line_number++);
	
	panbuffer.panReady = 0;
	while (1) {
		marker_low = 0;
		marker_high = 0;
		marker_center = 0;
		sequence = 0;
		//panbuffer.Y is 800 uint16_t or 1600 bytes. This is too long to send over the Internet without the network segmenting the packet
		//Segmenting may cause packets to arrive out of order. The panbuffer.Y buffer is split into two 800 byte packets and sent individually
		//Each with a unique segment indicator. MSCC will discard both segments if they arrive out of order.

		//Build the first 800 byte packet
		if (panbuffer.panReady) {//Process the panbuffer.Y buffer if panbufer.panReady is true
			panadaper_average[sequence].avg_buffer_output.opcode = CMD_GET_SET_PANADAPTER;
			panadaper_average[sequence].avg_buffer_output.sequence = sequence;
			for (x = 0; x < MAX_X; x++) {
				if (x == G_band_marker_low) {
					marker_low = x;
				}
				else {
					panbuffer_temp.Y[x] = panbuffer.Y[x];
					}
			}
			for (i = 0; i < MAX_X; i++) {
				panadaper_average[sequence].avg_buffer_output.output_buffer[i] = 0;
			}
			for (average_count = 0; average_count < G_Smoothing; average_count++) {
				for (i = 0; i < MAX_X; i++) {
					panadaper_average[sequence].avg_buffer_output.output_buffer[i] += 
						panadaper_average[sequence].buffer_input[average_count].avg_buffer_input[i];
				}
			}
			for (i = 0; i < MAX_X; i++) {
				panadaper_average[sequence].avg_buffer_output.output_buffer[i] = (panadaper_average[sequence].avg_buffer_output.output_buffer[i] +
					panbuffer_temp.Y[i]) / G_Smoothing;
			}

			panadaper_average[sequence].avg_buffer_output.output_buffer[DISPLAY_CENTER] = MAX_MARKER_SIZE;
			panadaper_average[sequence].avg_buffer_output.output_buffer[marker_low] = MAX_MARKER_SIZE;
			//Send the first 800 byte packet.
			if (sendto(ms_sdr_s, (char *)&panadaper_average[sequence].avg_buffer_output,send_size, 0, (struct sockaddr *)&si_ms_sdr, slen) == SOCKET_ERROR)
			{
				print_time();
				fprintf(G_fp_logfile, "[%d] Panadapter_thread -> sentto failed with error code : %d\n", line_number++, WSAGetLastError());
			}
			for (average_count = 0; average_count < (G_Smoothing - 1); average_count++) {
				memcpy(panadaper_average[sequence].buffer_input[average_count].avg_buffer_input,
					panadaper_average[sequence].buffer_input[(average_count + 1)].avg_buffer_input, (MAX_X * 2));
			}
			memcpy(panadaper_average[sequence].buffer_input[average_count].avg_buffer_input, panbuffer_temp.Y, (MAX_X * 2));

			//Now build the second packet
			sequence = 1;
			Y_source_index = MAX_X;
			for (x = 0; x < MAX_X; x++, Y_source_index++) {
				if (Y_source_index == G_band_marker_high) {
					marker_high = x;
				}
				else {
					panbuffer_temp.Y[x] = panbuffer.Y[Y_source_index];
				}
			}
			for (i = 0; i < MAX_X; i++) {
				panadaper_average[1].avg_buffer_output.output_buffer[i] = 0;
			}
			for (average_count = 0; average_count < G_Smoothing; average_count++) {
				for (i = 0; i < MAX_X; i++) {
					panadaper_average[sequence].avg_buffer_output.output_buffer[i] += panadaper_average[sequence].buffer_input[average_count].avg_buffer_input[i];

				}
			}
			for (i = 0; i < MAX_X; i++) {
				panadaper_average[sequence].avg_buffer_output.output_buffer[i] = (panadaper_average[sequence].avg_buffer_output.output_buffer[i] +
					panbuffer_temp.Y[i]) / G_Smoothing;
			}
			panadaper_average[sequence].avg_buffer_output.output_buffer[marker_high] = MAX_MARKER_SIZE;
			panadaper_average[sequence].avg_buffer_output.opcode = CMD_GET_SET_PANADAPTER;
			panadaper_average[sequence].avg_buffer_output.sequence = sequence;
			//Send the second packet
			if (sendto(ms_sdr_s, (char *)&panadaper_average[1].avg_buffer_output, send_size, 0, (struct sockaddr *)&si_ms_sdr, slen) == SOCKET_ERROR)
			{
				print_time();
				fprintf(G_fp_logfile, "[%d] Panadapter_thread -> sentto failed with error code : %d\n", line_number++, WSAGetLastError());
			}
			for (average_count = 0; average_count < (G_Smoothing - 1); average_count++) {
				memcpy(panadaper_average[1].buffer_input[average_count].avg_buffer_input,
					panadaper_average[1].buffer_input[(average_count + 1)].avg_buffer_input, (MAX_X * 2));
			}
			memcpy(panadaper_average[1].buffer_input[average_count].avg_buffer_input, panbuffer_temp.Y, (MAX_X * 2));
			panbuffer.panReady = 0;
		}
		Sleep(30);//This iteration finished.
	}
	return (NULL);
}