#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
#include "usbavrcmd.h"
#include "SRDLL.h"
#include "extern.h"


void *S_meter_thread(void *param) {
	int status;

	Sleep(1000);
	status = SDRcore_recv_send_param(CMD_GET_SET_SMETER, 1);

}