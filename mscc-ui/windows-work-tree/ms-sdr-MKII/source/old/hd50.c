#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
//#include <windows.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);

#include <stdio.h>
#include <conio.h>		// gives _cprintf()
#include <ShlObj.h>
#include <KnownFolders.h>
#include <direct.h>
#pragma comment(lib, "Ws2_32.lib")

#include "port_defines.h"
#include "usbavrcmd.h"

#include "lusb0_usb.h"	// LIBUSB-Win32
#include "SRDLL.h"
#include "extern.h"
#include "version.h"
#include "hr50.h"


#include "pthread.h"
#include "sched.h"
#include "semaphore.h"

#define QUEUE_BUFFER_LIMIT 20
#define QUEUE_ELEMENTS 50
#define QUEUE_SIZE (QUEUE_ELEMENTS + 1)
int Queue[QUEUE_SIZE];
int QueueIn, QueueOut;

pthread_t HR50_Write_thread;
int HR50_Write_thread_rc;
HANDLE Win32_HR50_Writethread;
int delete_hr50_comm_port_ini();
uint8_t HR_Queue_thread_run = 0;

int hr50_comm_port_open = 0;
uint8_t hr50_comport_number = 0;
char hr50_port_number[80];
char hr50_comm_port[20];
hr50_Serial_port_open = 0;

struct {
	char buffer[80];
	int size;
} queue_buffer[20];


HANDLE hr50_hSerial;
DWORD hr50_com_time_out = 20;
DCB hr50_dcbSerialParams = { 0 };
COMMTIMEOUTS hr50_timeouts = { 0 };
char hr50_lastError[1024];
int hr50_baud_rate = 9600;
int hr50_parity = 0;
int hr50_stop_bits = 0;
int hr50_data_bits = 8;

int hr50_baud_rates[8] = { 1200,2400,4800,9600,19200,38400,57600,115200 };
int hr50_parity_values[3] = { PARITY_NONE,PARITY_EVEN,PARITY_ODD };
int hr50_stop_bit_values[2] = { ONESTOPBIT,TWOSTOPBITS };
int hr50_data_bit_values[3] = { 7,8,9 };

int hr50_comm_name_index = 0;
int hr50_baud_rate_index = 3;
int hr50_parity_index = 0;
int hr50_stop_bit_index = 0;
int hr50_data_bit_index = 1;

int hr50_write_to_comm_port(uint8_t *buffer, int size) {
	int status = 0;

	DWORD bytes_to_write;
	DWORD bytes_written;
	BOOL error;

	//print_time(0);
	//fprintf(G_fp_logfile, "[%d] hr50_write_to_comm_port\n", line_number++);
	bytes_to_write = size;
	error = WriteFile(hr50_hSerial, buffer, bytes_to_write, &bytes_written, NULL);
	if (error == 0) {
		FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			hr50_lastError,
			1024,
			NULL);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] hr50_write_to_comm_port -> Error writing to comm port: %s \n", line_number++, hr50_lastError);
		status = 1;
		MessageBoxA(NULL, " hr50_write_to_comm_port FAILED. Closing Comm Port, Please forward log files to Multus SDR, LLC ", "ms-sdr",
			MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
		hr50_Serial_port_open = 0;
		if (hr50_hSerial != NULL) {
			CloseHandle(hr50_hSerial);
		}
		delete_hr50_comm_port_ini();
		Gui_send_param(CMD_GET_SET_HR50_COMM_NAME_INDEX,200);
	}
	//print_time(0);
	//fprintf(G_fp_logfile, "[%d] hr50_write_to_comm_port-> bytes written: %d\n", line_number++, bytes_written);
	return status;
}

int QueuePut(int new)
{
	if (QueueIn == ((QueueOut - 1 + QUEUE_SIZE) % QUEUE_SIZE))
	{
		return -1; /* Queue Full*/
	}

	Queue[QueueIn] = new;

	QueueIn = (QueueIn + 1) % QUEUE_SIZE;

	return 0; // No errors
}

int QueueGet(int *old)
{
	if (QueueIn == QueueOut)
	{
		return -1; /* Queue Empty - nothing to get*/
	}

	*old = Queue[QueueOut];

	QueueOut = (QueueOut + 1) % QUEUE_SIZE;

	return 0; // No errors
}

void *HR_Queue_thread(void *param) {
	int queue = 0;
	int queue_status = 0;
	
	print_time(0);
	fprintf(G_fp_logfile, "[%d] HR_Queue_thread -> Started.\n", line_number++);
	while(HR_Queue_thread_run) {
		queue_status = QueueGet(&queue);
		if (queue_status != -1) {
			//print_time(0);
			//fprintf(G_fp_logfile, "[%d] HR_Queue_thread -> command_buffer: %s, size %d\n",
			//	line_number++, queue_buffer[queue].buffer, queue_buffer[queue].size);
			hr50_write_to_comm_port(queue_buffer[queue].buffer, queue_buffer[queue].size);
		}
		Sleep(10);
	}
	print_time(0);
	fprintf(G_fp_logfile, "[%d] HR_Queue_thread -> Stopped.\n", line_number++);
	return NULL;
}

int open_hr50_comm_port() {
	int status = TRUE;
	DWORD read_timeout = 0;
	char windows_comm_port[20] = { 0 };
	char error_message[80] = { 0 };

	sprintf(windows_comm_port, "\\\\.\\%s", hr50_comm_port);
	hr50_hSerial = CreateFileA(windows_comm_port,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (hr50_hSerial == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			print_time(0);
			fprintf(G_fp_logfile, "[%d] open_hr50_comm_port ->COM Port: %s Open Failed \n", line_number++, windows_comm_port);
		}
		FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			ERROR_FILE_NOT_FOUND,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			hr50_lastError,
			1024,
			NULL);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] open_hr50_comm_port -> Error Opening Com Port: %s %s\n", line_number++, windows_comm_port, hr50_lastError);
		MessageBoxA(NULL, "MS-SDR -> Hardrock 50 Comm Port OPEN FAILED. Please forward log files to Multus SDR, LLC ", "MS-SDR",
			MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
		status = FALSE;
	}
	if (status) {
		hr50_dcbSerialParams.DCBlength = sizeof(hr50_dcbSerialParams);
		if (!GetCommState(hr50_hSerial, &hr50_dcbSerialParams)) {
			FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				hr50_lastError,
				1024,
				NULL);
			print_time(0);
			fprintf(G_fp_logfile, "[%d] open_hr50_comm_port -> Error Retrieving serial port state: %s %s\n", line_number++, hr50_comm_port, hr50_lastError);
			MessageBoxA(NULL, "MS-SDR -> ERROR Retrieving Hardrock 50 Comm Port State.  Please forward log files to Multus SDR, LLC ",
				"MS-SDR", MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
			status = FALSE;
		}
	}
	hr50_dcbSerialParams.BaudRate = hr50_baud_rate;
	hr50_dcbSerialParams.ByteSize = hr50_data_bits;
	hr50_dcbSerialParams.StopBits = hr50_stop_bits;
	hr50_dcbSerialParams.Parity = hr50_parity;
	if (status) {
		if (!SetCommState(hr50_hSerial, &hr50_dcbSerialParams)) {
			FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				hr50_lastError,
				1024,
				NULL);
			print_time(0);
			fprintf(G_fp_logfile, "[%d] open_hr50_comm_port -> Error Setting serial port state: %s %s\n", line_number++, hr50_comm_port, hr50_lastError);
			print_time(0);
			fprintf(G_fp_logfile, "[%d] open_hr50_comm_port -> BaudRate: %d, ByteSize: %d, StopBits: %d, Parity: %d\n", line_number++,
				hr50_dcbSerialParams.BaudRate, hr50_dcbSerialParams.ByteSize, hr50_dcbSerialParams.StopBits, hr50_dcbSerialParams.Parity);
			MessageBoxA(NULL, "MS-SDR -> ERROR Setting Hardrock 50 Comm Port Parameters.  Please forward log files to Multus SDR, LLC ",
				"MS-SDR", MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
			status = FALSE;
			CloseHandle(hr50_hSerial);
		}
	}
	hr50_timeouts.ReadTotalTimeoutMultiplier = hr50_com_time_out;
	hr50_timeouts.ReadIntervalTimeout = hr50_com_time_out;
	hr50_timeouts.ReadTotalTimeoutConstant = (hr50_com_time_out * 80);

	hr50_timeouts.WriteTotalTimeoutConstant = 50;
	hr50_timeouts.WriteTotalTimeoutMultiplier = 15;
	if (status) {
		if (!SetCommTimeouts(hr50_hSerial, &hr50_timeouts)) {
			FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				hr50_lastError,
				1024,
				NULL);
			print_time(0);
			fprintf(G_fp_logfile, "[%d] open_hr50_comm_port -> Error Setting serial port timeouts: %s %s\n", line_number++, hr50_comm_port, hr50_lastError);
			CloseHandle(hr50_hSerial);
			status = FALSE;
		}
	}
	if (status == TRUE) {
		hr50_Serial_port_open = TRUE;
	}
	else {
		hr50_Serial_port_open = FALSE;
		Gui_send_param(CMD_GET_SET_HR50_COMM_NAME_INDEX, 200);
	}
	print_time(0);
	fprintf(G_fp_logfile, "[%d] open_hr50_comm_port -> Finished\n", line_number++);
	return status;
}

int create_hr50_comm_port_ini() {
	FILE *fp_Comm_port_ini;
	WCHAR path[PATH_MAX];
	PWSTR lpPath = path;
	char l_path[PATH_MAX] = { 0 };
	int lenght = 0;
	int record = 0;
	int index = 0;

	print_time(1);
	fprintf(G_fp_logfile, "[%d] create_hr50_comm_port_ini -> Called.\n", line_number++);

#if defined(_WIN32)
	HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &lpPath);
	if (SUCCEEDED(hr)) {
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpPath, -1, l_path, sizeof(l_path), NULL, NULL);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] create_hr50_comm_port_ini -> Default Path: %s\n", line_number++, l_path);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] create_hr50_comm_port_ini -> hr50-comm-port.ini Path: %s\n", line_number++, l_path);

		strcat(l_path, "\\multus-sdr-mfc\\hr50-comm-port.ini");

#endif
		fp_Comm_port_ini = fopen(l_path, "w");
		if (fp_Comm_port_ini != NULL) {
			fprintf(fp_Comm_port_ini, "COMM_PORT_NAME=%s,COMM_PORT_INDEX=%d,BAUD_RATE_INDEX=%d,PARITY_INDEX=%d,DATA_BITS_INDEX=%d,STOP_BITS_INDEX=%d;\n",
				hr50_comm_port, hr50_comm_name_index, hr50_baud_rate_index, hr50_parity_index, hr50_data_bit_index, hr50_stop_bit_index);
		}
		else {
			print_time(0);
			fprintf(G_fp_logfile, "[%d] create_hr50_comm_port_ini -> Open file failed\n", line_number++);
			return 0;
		}
		print_time(0);
		fprintf(G_fp_logfile, "[%d] create_hr50_comm_port_ini -> Finished\n", line_number++);
		fclose(fp_Comm_port_ini);
	}
	return 1;
}

int set_hr50_comm_port_ini() {
	FILE *fp_Comm_port_ini;
	WCHAR path[PATH_MAX];
	PWSTR lpPath = path;
	char l_path[PATH_MAX] = { 0 };
	int lenght = 0;
	int record = 0;
	int index = 0;
	char comm_port_record[132];
	struct {
		char *comm_port_name_start;
		char *comm_port_name_end;
		int comm_port_name_size;
		char *comm_port_index;
		char *baud_rate_index;
		char *parity_index;
		char *data_bits_index;
		char *stop_bits_index;
		char *pin;
	}comm_port_fields;
	int mynumber;
	int status = TRUE;
	int open_port_status = 0;
	long t = 0;


	print_time(1);
	fprintf(G_fp_logfile, "[%d] set_hr50_comm_port_ini -> Called.\n", line_number++);
#if defined(_WIN32)
	HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &lpPath);
	if (SUCCEEDED(hr)) {
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpPath, -1, l_path, sizeof(l_path), NULL, NULL);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] set_hr50_comm_port_ini -> Default Path: %s\n", line_number++, l_path);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] set_hr50_comm_port_ini -> hr50-comm-port.ini Path: %s\n", line_number++, l_path);

		strcat(l_path, "\\multus-sdr-mfc\\hr50-comm-port.ini");


#endif
		fp_Comm_port_ini = fopen(l_path, "r");
		if (fp_Comm_port_ini != NULL) {
			fgets(comm_port_record, sizeof(comm_port_record), fp_Comm_port_ini);
			//Get the name of the comm port
			//Find the starting point
			comm_port_fields.comm_port_name_start = strstr(comm_port_record, "COMM_PORT_NAME");
			comm_port_fields.comm_port_name_start = comm_port_fields.comm_port_name_start + 15;
			//Now find the ending point
			comm_port_fields.comm_port_name_end = strstr(comm_port_fields.comm_port_name_start, ",");
			//Get the size of the comm port name.
			comm_port_fields.comm_port_name_size = (comm_port_fields.comm_port_name_end - (comm_port_fields.comm_port_name_start));
			//Now copy to comm_port
			strncpy(hr50_comm_port, comm_port_fields.comm_port_name_start, comm_port_fields.comm_port_name_size);
					   
			comm_port_fields.comm_port_index = strstr(comm_port_record, "COMM_PORT_INDEX");
			comm_port_fields.baud_rate_index = strstr(comm_port_record, "BAUD_RATE_INDEX");
			comm_port_fields.parity_index = strstr(comm_port_record, "PARITY_INDEX");
			comm_port_fields.data_bits_index = strstr(comm_port_record, "DATA_BITS_INDEX");
			comm_port_fields.stop_bits_index = strstr(comm_port_record, "STOP_BITS_INDEX");
			
			mynumber = atoi((comm_port_fields.comm_port_index + 16));
			hr50_comm_name_index = mynumber;
			mynumber = atoi((comm_port_fields.baud_rate_index + 16));
			hr50_baud_rate_index = mynumber;
			mynumber = atoi((comm_port_fields.parity_index + 13));
			hr50_parity_index = mynumber;
			mynumber = atoi((comm_port_fields.data_bits_index + 16));
			hr50_data_bit_index = mynumber;
			mynumber = atoi((comm_port_fields.stop_bits_index + 16));
			hr50_stop_bit_index = mynumber;
		
			hr50_baud_rate = hr50_baud_rates[hr50_baud_rate_index];
			hr50_data_bits = hr50_data_bit_values[hr50_data_bit_index];
			hr50_stop_bits = hr50_stop_bit_values[hr50_stop_bit_index];
			hr50_parity = hr50_parity_values[hr50_parity_index];

			print_time(0);
			fprintf(G_fp_logfile,
				"[%d] set_hr50_comm_port_ini-> COMM_PORT_NAME=%s,COMM_PORT_INDEX=%d,BAUD_RATE_INDEX=%d,PARITY_INDEX=%d,DATA_BITS_INDEX=%d,STOP_BITS_INDEX=%d\n",
				line_number++, hr50_comm_port, hr50_comm_name_index, hr50_baud_rate_index, hr50_parity_index, hr50_data_bit_index, 
				hr50_stop_bit_index);
			fclose(fp_Comm_port_ini);
			open_port_status = open_hr50_comm_port();
			if(open_port_status == TRUE){
				HR50_Write_thread_rc = pthread_create(&HR50_Write_thread, NULL, HR_Queue_thread, (void *)t);
				if (HR50_Write_thread_rc) {
					print_time(0);
					fprintf(G_fp_logfile, "[%d] MS-SDR -> Start HR_Queue_thread thread failed, return code from pthread_create() is %d\n",
						line_number++, HR50_Write_thread_rc);
				}
				else {
					print_time(0);
					fprintf(G_fp_logfile, "[%d] MS-SDR -> Start HR_Queue_thread thread Started, return code from pthread_create() is %d\n",
						line_number++, HR50_Write_thread_rc);
					HR_Queue_thread_run = 1;
				}
				Gui_send_param(CMD_GET_SET_HR50_COMM_NAME_INDEX, hr50_comm_name_index);
			} else {
				print_time(0);
				fprintf(G_fp_logfile, "[%d] set_hr50_comm_port_ini -> Open Comm Port Failed \n", line_number++);
				delete_hr50_comm_port_ini();
				Gui_send_param(CMD_GET_SET_HR50_COMM_NAME_INDEX, 200);
			}
		}
		else {
			print_time(0);
			fprintf(G_fp_logfile, "[%d] set_hr50_comm_port_ini -> hr50_comm-port.ini does not exist.  Open failed\n", line_number++);
			status = FALSE;
		}

	}
	print_time(0);
	fprintf(G_fp_logfile, "[%d] set_hr50_comm_port_ini -> Finished\n", line_number++);
	return status;
}

int delete_hr50_comm_port_ini() {
	WCHAR path[PATH_MAX];
	PWSTR lpPath = path;
	char l_path[PATH_MAX] = { 0 };

	print_time(1);
	fprintf(G_fp_logfile, "[%d] delete_hr50_comm_port_ini -> Called.\n", line_number++);

#if defined(_WIN32)
	HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &lpPath);
	if (SUCCEEDED(hr)) {
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpPath, -1, l_path, sizeof(l_path), NULL, NULL);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] delete_hr50_comm_port_ini -> Default Path: %s\n", line_number++, l_path);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] delete_hr50_comm_port_ini -> hr50-comm-port.ini Path: %s\n", line_number++, l_path);

		strcat(l_path, "\\multus-sdr-mfc\\hr50-comm-port.ini");

		remove(l_path);
	}
#endif
	return(1);
}

uint8_t hr50_convert_band(int Proficio_band) {
	uint8_t band;

	switch (Proficio_band) {
	case 160:
		band = 10;
		break;
	case 80:
		band = 9;
		break;
	case 60:
		band = 8;
		break;
	case 40:
		band = 7;
		break;
	case 30:
		band = 6;
		break;
	case 20:
		band = 5;
		break;
	case 15:
		band = 3;
		break;
	case 17:
		band = 4;
		break;
	case 12:
		band = 2;
		break;
	case 10:
		band = 1;
		break;
	default:
		band = 11;
	}
	return band;
};

int hr_format_buffer(char *buffer, char *command, uint32_t value,uint8_t mode) {
	int status = 0;
	char value_buffer[80] = { 0 };

	switch (mode) {
	case 0:
		sprintf(value_buffer, "%d", value);
		break;
	case 1:
		sprintf(value_buffer, "%011ld", value);
		break;
	}
	strcpy(buffer, command);
	strcat(buffer, value_buffer);
	strcat(buffer, ";");
	status = strlen(buffer);
	return status;
}

int Process_hr50(int opcode, char *buf){
	int status = 0;
	uint8_t t_opcode_data;
	short int *opcode_data;
	short int s_opcode_data;
	uint8_t band = 0;
	//char hr50_command_buffer[80];
	int size;
	long t = 0;
	static int queue_count = 0;
	int queue_status = 0;

	//opcode = (uint8_t)buf[0];
	t_opcode_data = (uint8_t)(buf[1]);
	opcode_data = (short int*)&buf[1];
	memcpy(&s_opcode_data, opcode_data, 2);

	switch (opcode) {
	case CMD_GET_SET_HR50_COMM_PORT:
		memset(hr50_comm_port, 0, sizeof(hr50_comm_port));
		memset(hr50_port_number, 0, sizeof(hr50_port_number));
		strcpy(hr50_comm_port, "COM");
		sprintf(hr50_port_number, "%d", t_opcode_data);
		strcat(hr50_comm_port, hr50_port_number);
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_hr50 -> CMD_GET_SET_HR50_COMM_PORT -> Portname: %s, Portnumber: %s\n", line_number++,
			hr50_comm_port, hr50_port_number);
		break;

	case CMD_GET_SET_HR50_COMM_NAME_INDEX:
		hr50_comm_name_index = t_opcode_data;
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_comm_command -> CMD_GET_SET_COMM_NAME_INDEX -> Comm Name index: %d\n", line_number++, hr50_comm_name_index);
		break;

	case CMD_GET_SET_HR50_COMM_START:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_hr50 -> CMD_GET_SET_HR50_COMM_START -> t_opcode_data: %d\n", line_number++, t_opcode_data);
		if (t_opcode_data == 1) {
			create_hr50_comm_port_ini();
			open_hr50_comm_port();
			HR50_Write_thread_rc = pthread_create(&HR50_Write_thread, NULL, HR_Queue_thread, (void *)t);
			if (HR50_Write_thread_rc) {
				print_time(0);
				fprintf(G_fp_logfile, "[%d] MS-SDR -> Start HR_Queue_thread thread failed, return code from pthread_create() is %d\n", 
					line_number++, HR50_Write_thread_rc);
			} else {
				print_time(0);
				fprintf(G_fp_logfile, "[%d] MS-SDR -> Start HR_Queue_thread thread Started, return code from pthread_create() is %d\n", 
					line_number++, HR50_Write_thread_rc);
				HR_Queue_thread_run = 1;
			}
		}
		else {
			if (hr50_Serial_port_open) {
				CloseHandle(hr50_hSerial);
				hr50_hSerial = NULL;
			}
			delete_hr50_comm_port_ini();
			hr50_Serial_port_open = 0;
			HR_Queue_thread_run = 0;
		}
		break;

	case CMD_GET_SET_HR50_COMM_SEND_BAND_INFO:
		//print_time(0);
		//fprintf(G_fp_logfile, "[%d] Process_hr50 -> CMD_GET_SET_HR50_COMM_SEND_BAND_INFO -> t_opcode_data: %d\n", line_number++, t_opcode_data);
		if (hr50_Serial_port_open) {
			band = hr50_convert_band(t_opcode_data);
			//size = hr_format_buffer(hr50_command_buffer, "HRBN", band,0);
			if (queue_count++ < QUEUE_BUFFER_LIMIT) {
				memset(queue_buffer[queue_count].buffer, 0, sizeof(queue_buffer[queue_count].buffer));
				size = hr_format_buffer(queue_buffer[queue_count].buffer, "HRBN",band, 0);
				queue_buffer[queue_count].size = size;
				queue_status = QueuePut(queue_count);
				if (queue_status == -1) {
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Process_hr50 -> QueuePut FAILED.\n", line_number++);
				}
			}
			else {
				queue_count = 0;
			}
			//print_time(0);
			//fprintf(G_fp_logfile, "[%d] Process_hr50 -> CMD_GET_SET_HR50_COMM_SEND_BAND_INFO -> command_buffer: %s, size %d\n", 
				//line_number++, queue_buffer[queue_count].buffer,size);
		}
		else {
			print_time(0);
			fprintf(G_fp_logfile, "[%d] Process_hr50 -> CMD_GET_SET_HR50_COMM_SEND_BAND_INFO -> Comm Port NOT OPEN. Not sending data\n",
				line_number++);
		}
		break;

	case CMD_GET_SET_HR50_COMM_SEND_FREQ_INFO:
		//print_time(0);
		//fprintf(G_fp_logfile, "[%d] Process_hr50 -> CMD_GET_SET_HR50_COMM_SEND_FREQ_INFO -> t_opcode_data: %d\n", line_number++, t_opcode_data);
		if (hr50_Serial_port_open) {
			//size = hr_format_buffer(hr50_command_buffer, "FA", G_tune_freq,1);
			//hr50_write_to_comm_port(hr50_command_buffer, size);
			if (queue_count++ < QUEUE_BUFFER_LIMIT) {
				memset(queue_buffer[queue_count].buffer, 0, sizeof(queue_buffer[queue_count].buffer));
				size = hr_format_buffer(queue_buffer[queue_count].buffer, "FA", G_tune_freq, 1);
				queue_buffer[queue_count].size = size;
				queue_status = QueuePut(queue_count);
				if (queue_status == -1) {
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Process_hr50 -> QueuePut FAILED.\n", line_number++);
				}
			}
			else {
				queue_count = 0;
			}
			//print_time(0);
			//fprintf(G_fp_logfile, "[%d] Process_hr50 -> CMD_GET_SET_HR50_COMM_SEND_FREQ_INFO -> command_buffer: %s, size %d\n",
			//	line_number++, queue_buffer[queue_count].buffer, size);
		}
		else {
			print_time(0);
			fprintf(G_fp_logfile, "[%d] Process_hr50 -> CMD_GET_SET_HR50_COMM_SEND_FREQ_INFO -> Comm Port NOT OPEN. Not sending data\n",
				line_number++);
		}
		break;

	case CMD_GET_SET_HR50_COMM_PASS_THRU:
		//print_time(0);
		//fprintf(G_fp_logfile, "[%d] Process_hr50 -> CMD_GET_SET_HR50_COMM_PASS_THRU -> t_opcode_data: %d\n", line_number++, t_opcode_data);
		if (hr50_Serial_port_open) {
			//size = hr_format_buffer(hr50_command_buffer, "HRMD", t_opcode_data,0);
			//hr50_write_to_comm_port(hr50_command_buffer, size);
			if (queue_count++ < QUEUE_BUFFER_LIMIT) {
				memset(queue_buffer[queue_count].buffer, 0, sizeof(queue_buffer[queue_count].buffer));
				size = hr_format_buffer(queue_buffer[queue_count].buffer, "HRMD", t_opcode_data, 0);
				queue_buffer[queue_count].size = size;
				queue_status = QueuePut(queue_count);
				if (queue_status == -1) {
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Process_hr50 -> QueuePut FAILED.\n", line_number++);
				}
			}
			else {
				queue_count = 0;
			}
			//print_time(0);
			//fprintf(G_fp_logfile, "[%d] Process_hr50 -> CMD_GET_SET_HR50_COMM_PASS_THRU -> command_buffer: %s, size %d\n",
			//	line_number++, queue_buffer[queue_count].buffer, size);
		}
		else {
			print_time(0);
			fprintf(G_fp_logfile, "[%d] Process_hr50 -> CMD_GET_SET_HR50_COMM_PASS_THRU -> Comm Port NOT OPEN. Not sending data\n",
				line_number++);
		}
		break;

	default:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_hr50 -> Unknown Opcode -> opcode: %d\n", line_number++, opcode);
	}
	return status;
}