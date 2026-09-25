#define _CRT_SECURE_NO_WARNINGS 1
#include "extern.h"
#include <math.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
#include "usbavrcmd.h"
//#include "ExtIODll.h"
//#include "ExtIOFunctions.h"
#include "version.h"


uint8_t CW_Snap_calibration_freq = 0;
int CW_Snap_calibration_element = 0;
int CW_Snap_calibration_limit;
int CW_Snap_max_element = MAX_CALIBRATION_ELEMENT;
int CW_Snap_element_increament = 1;
int CW_Snap_int = 0;
int CW_Snap_dec = 0;
int CW_Snap_PPM_file_exists = 0;

//uint32_t CW_Snap_cycle_count = 65536 / 6;
uint8_t CW_Snap_mode_number;
uint8_t CW_Snap_previous_mode_number;
char CW_Snap_previous_mode = 'N';
uint32_t CW_Snap_previous_freq;


uint8_t CW_Snap_t_opcode_data;
short int *CW_Snap_opcode_data;
short int CW_Snap_s_opcode_data;
uint16_t *CW_Snap_op_code_data_16;
uint16_t CW_Snap_i_opcode_data;
uint32_t CW_Snap_u_int_data;
uint8_t CW_Snap_opcode_data_8_bit;

int CW_Snap_Calibrate_Si5351_Initialize(uint32_t frequency) //This is called by Process_Frequency_Calibration
{
	int status = TRUE;

	CW_Snap_previous_mode = G_mode;
	CW_Snap_previous_freq = G_tune_freq;
	CW_Snap_previous_mode_number = mode_to_number(G_mode);
	CW_Snap_mode_number = mode_to_number('C');
	CW_Snap_calibration_freq = 100;
	CW_Snap_max_element = 100;
	CW_Snap_calibration_element = 1;
	CW_Snap_calibration_limit = CW_Snap_max_element;
	CW_Snap_element_increament = 1;
	
	status = CALIBRATION_INITIALIZED;
	return status;
}

int CW_Snap_Calibrate_Si5351_Continue(uint32_t frequency) { //This is called by Process_Frequency_Calibration
	int status = 0;
	print_time(0);
	fprintf(G_fp_logfile, "[%d] CW_Snap_Calibrate_Si5351_Continue Called\n",line_number++);
	if (CW_Snap_calibration_element <= CW_Snap_calibration_limit) {
		print_time(0);
		fprintf(G_fp_logfile, "[%d] CW_Snap_Calibrate_Si5351_Continue . Calling SDRcore_recv_send_param. Param: %X, Element %d, Delta Freq: %d\n",
			line_number++, CMD_SET_CALIBRATION_START, CW_Snap_calibration_element, CW_Snap_calibration_freq);
		SDRcore_recv_send_param(CMD_CW_SNAP_SET_DELTA, CW_Snap_calibration_freq);
		SDRcore_recv_send_param(CMD_CW_SNAP_START, 1);

		CW_Snap_calibration_freq = CW_Snap_calibration_freq - CW_Snap_element_increament;
		CW_Snap_calibration_element = CW_Snap_calibration_element + CW_Snap_element_increament;
		status = CALIBRATION_RUNNING;
	}
	else {
		status = CALIBRATION_COMPLETE;
		SDRcore_recv_send_param(CMD_CW_SNAP_FINISHED, 1);
	}
	return status;
}

int CW_Snap_Process_Frequency_Calibration(uint8_t command, char *buf) {
	int status = 0;
	static int calibration_state = 0;
	static int calibration_initialized = 0;
	uint8_t sign = 0;
	uint16_t delta = 0;
	//static int calibration_count = 0;
	//static int calibration_increament = 0;

	CW_Snap_op_code_data_16 = (uint16_t *)&buf[2];
	memcpy(&CW_Snap_i_opcode_data, CW_Snap_op_code_data_16, 2);
	CW_Snap_opcode_data_8_bit = (uint8_t)(buf[1]);

	switch (command) {

	case CMD_CW_SNAP_SET_CALIBRATION_DATA: //This is called by SDRcore_recv
		sign = buf[1];
		print_time(1);
		fprintf(G_fp_logfile, "[%d] CW_Snap_Process_Frequency_Calibration . CMD_SET_CALIBRATION_DATA . sign: %d, delta: %d \n", 
			line_number++, sign, CW_Snap_i_opcode_data);
		switch (sign) {
		case 0:
			G_tune_freq = G_tune_freq - CW_Snap_i_opcode_data;
			break;
		case 1:
			G_tune_freq = G_tune_freq + CW_Snap_i_opcode_data;
			break;
		case 2:
			G_tune_freq = G_tune_freq + CW_Snap_i_opcode_data;
		}
		print_time(1);
		fprintf(G_fp_logfile, "[%d] CW_Snap_Process_Frequency_Calibration . CMD_SET_CALIBRATION_DATA . G_tune_freq: %ld \n",
			line_number++, G_tune_freq);
		//SetHWLO(G_tune_freq);
		freq_queue_add(G_tune_freq);
		//G_tune_freq = (uint32_t) CW_Snap_i_opcode_data;
		Gui_send_param(CMD_CW_SNAP_FINISHED, CALIBRATION_SUCCESS);
		Sleep(50);
		Gui_send_param(CMD_SET_CW_SNAP_FREQ, G_tune_freq);
		calibration_state = 0;
		calibration_initialized = 0;
		//calibration_count = 0;
		//calibration_increament = 0;
		G_CW_Snap_Running = 0;
		break;

	case CMD_CW_SNAP_START: //This is called by MSCC
		print_time(1);
		fprintf(G_fp_logfile, "[%d] CW_Snap_Process_Frequency_Calibration . CMD_CW_SNAP_START . Frequency: %ld\n", line_number++, CW_Snap_i_opcode_data);
		//CW_Snap_max_element = MAX_CALIBRATION_ELEMENT / 4;
		
		calibration_initialized = CW_Snap_Calibrate_Si5351_Initialize(CW_Snap_i_opcode_data);
		if (calibration_initialized == CALIBRATION_INITIALIZED) {
			SDRcore_recv_send_param(CDM_SET_CALIBRATE_CYCLE_COUNT, (CW_SNAP_CYCLE_COUNT / 16));
			calibration_state = CW_Snap_Calibrate_Si5351_Continue(CW_Snap_i_opcode_data);
		}
		G_CW_Snap_Running = 1;
		break;
		
	case CMD_CW_SNAP_DATA_PROCESSED:  //This is called by SDRcore_recv
		print_time(1);
		fprintf(G_fp_logfile, "[%d] CW_Snap_Process_Frequency_Calibration . CMD_SET_CAL_DATA_PROCESSED \n", line_number++);
		if (CW_Snap_opcode_data_8_bit == CALIBRATION_SUCCESS) {
			if (calibration_state == CALIBRATION_RUNNING) {
				calibration_state = CW_Snap_Calibrate_Si5351_Continue(CW_Snap_i_opcode_data);
				//calibration_count++;
				//calibration_increament++;
				print_time(0);
				fprintf(G_fp_logfile, "[%d] CW_Snap_Process_Frequency_Calibration . CW_Snap_Calibrate_Si5351_Continue\n", line_number++);
			}
		}
		break;
	}

	return status;
}




