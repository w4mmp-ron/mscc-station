
#define _CRT_SECURE_NO_WARNINGS 1
//#define COMPILE_DATE __DATE__
//#define COMPILE_TIME __TIME__
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "usbavrcmd.h"
#include "SRDLL.h"
#include "extern.h"
#include "band_stack.h"
#include "last_used.h"
#include "iq.h"
#include "port_defines.h"


struct sockaddr_in panadapter_me, si_other;
int panadapter_s,  recv_len;
int socket_len = sizeof(si_other);
int panadapter_port = 0;

//For SDRcore-recv
struct sockaddr_in si_sdrcore_recv;
int sdrcore_s_recv;

int count = 0;

uint8_t opcode;
uint8_t t_opcode_data;
short int *opcode_data;
short int s_opcode_data;
int *op_code_data_32;
int i_opcode_data;
uint8_t buf[49000];

/*int Setup_Panadatper_UDP_Port() {
	int status = 0;

	Sleep(300);
	print_time(0);
	fprintf(G_fp_logfile, "[%d] Setup_Panadatper_UDP_Port -> Initialising Winsock...\n", line_number++);
	if (WSAStartup(MAKEWORD(2, 2), &dll_wsa) != NO_ERROR)
	{
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Setup_Panadatper_UDP_Port -> Initialising Winsock: Failed. Error Code : %d\n", line_number++, WSAGetLastError());
		pthread_exit(NULL);
	}
	print_time(0);
	fprintf(G_fp_logfile, "[%d] Setup_Panadatper_UDP_Port -> Winsock Initialised.\n", line_number++);
	//create the DLL socket
	if ((panadapter_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Setup_Panadatper_UDP_Port -> DLL Create Socket: Failed. Error Code : %d\n", line_number++, WSAGetLastError());
		pthread_exit(NULL);
	}
	else {
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Setup_Panadatper_UDP_Port -> DLL Socket Created\n", line_number++);
	}
	// Set up the DLL port for receiving UDP packets from any IP address
	// Initialize structures
	if (panadapter_port == 0) {
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Setup_Panadatper_UDP_Port -> Invalid DLL Port. Using default: %d\n", line_number++, PANADAPTER_PORT);
		panadapter_port = PANADAPTER_PORT;
	}
	memset((char *)&panadapter_me, 0, sizeof(panadapter_me));
	panadapter_me.sin_family = AF_INET;
	panadapter_me.sin_port = htons(panadapter_port);
	panadapter_me.sin_addr.s_addr = inet_addr(G_dll_IP);

	//bind socket to port
	if (bind(panadapter_s, (struct sockaddr*)&panadapter_me, sizeof(panadapter_me)) != 0)
	{
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Setup_Panadatper_UDP_Port -> Bind Failed. Error Code : %d\n", line_number++, WSAGetLastError());
		pthread_exit(NULL);
	}
	return status;
}*/

/*void *Panadapter_Thread(void *myparam) {

	Setup_Panadatper_UDP_Port();
	print_time(0);
	fprintf(G_fp_logfile, "[%d] Panadapter_Thread -> Calling SDRcore_recv_send_param \n", line_number++);
	SDRcore_recv_send_param(CMD_GET_SET_PANADAPTER, 1);
	while (1) {
		memset(buf, 0, sizeof(buf));
		if ((recv_len = recvfrom(panadapter_s, buf, sizeof(buf), 0, (struct sockaddr *) &si_other, &socket_len)) == SOCKET_ERROR)
		{
			print_time(0);
			fprintf(G_fp_logfile, "[%d] Panadapter_Thread -> recvfrom Failed. Error Code : %d\n", line_number++, WSAGetLastError());
			pthread_exit(NULL);
		}
		Sleep(100);
	}
	Sleep(5000);
	return NULL;
}*/