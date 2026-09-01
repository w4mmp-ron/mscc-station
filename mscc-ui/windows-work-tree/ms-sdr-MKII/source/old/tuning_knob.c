#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
#include "extern.h"
#include "usbavrcmd.h"
#include "version.h"

int Create_User_Controls(int force);
int Write_User_Controls();


#define VERSION 9

struct  {
	uint8_t Version;
	uint8_t Speaker_Volume;
	uint8_t Mic_Volume;
	uint8_t Speaker_Muted;
	uint8_t Mic_Muted;
	uint8_t Filter_Low_index;
	uint8_t Filter_High_index;
	uint8_t AGC_Index;
	uint8_t ALC_Value;
	uint8_t NR_Value;
	uint8_t Compression;
	uint8_t Compression_Level;
	uint8_t NB;
	int NB_Threshold;
	int NB_Pulse_Width;
	uint8_t Panadapter_Status;
	uint8_t Smeter_status;
	uint8_t Panadapter_Average;
	uint8_t Panadapter_Fill;
	uint8_t Panadapter_Line;
	uint8_t Panadapter_Marker;
	uint8_t Panadapter_Background;
	uint8_t CW_Pitch;
	uint8_t Panadapter_Cursor;
	uint8_t Panadapter_Refresh;
	int Panadapter_Base;
	int Panadapter_Gain;
	uint8_t Auto_Notch;
	uint8_t Filter_CW_Index;
} User_Controls;


uint8_t opcode_data_8_bit;
uint8_t t_opcode_data;
short int *opcode_data;
short int s_opcode_data;
int *op_code_data_32;
int i_opcode_data;

int Parse_User_Controls_record(char *record, char *field) {
	int status = -1;
	char source_field[80] = { 0 };
	char *result_field;
	int size = 0;

	strcpy(source_field, field);
	size = strlen(source_field);

	if ((result_field = strstr(record, source_field)) != NULL) {
		status = atoi((result_field + (size + 1)));
		//print_time(0);
		//fprintf(G_fp_logfile, "[%d] Parse_User_Controls_record. Record: %s, Field: %s, Size: %d, Result: %s\n", line_number++, record, source_field,
		//	size, result_field);
		return status;
	}
	return status;
}

int Check_Version() {
	int status = -1;
	FILE *fp_User_ini;
	WCHAR path[PATH_MAX];
	PWSTR lpPath = path;
	char l_path[PATH_MAX] = { 0 };
	char init_record[30];
	char field[20] = { 0 };
	char *record_status = NULL;

	print_time(0);
	fprintf(G_fp_logfile, "[%d] Check_Version -> Called.\n", line_number++);
#if defined(_WIN32)
	HRESULT hr_1 = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &lpPath);
	if (SUCCEEDED(hr_1)) {
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpPath, -1, l_path, sizeof(l_path), NULL, NULL);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Check_Version -> Default Path: %s\n", line_number++, l_path);
		strcat(l_path, "HOMEDIRECTORY/user_controls.ini");
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Check_Version -> user_controls.ini Path: %s\n", line_number++, l_path);
#endif
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Check_Version -> File Open \n", line_number++);
		fp_User_ini = fopen(l_path, "r");
		if (fp_User_ini != NULL) {
			print_time(0);
			fprintf(G_fp_logfile, "[%d] Check_Version -> File Open \n", line_number++);
			record_status = fgets(init_record,30, fp_User_ini);
			if (record_status != NULL) {
				memset(field, 0, sizeof(field));
				strcpy(field, "VERSION");
				print_time(0);
				fprintf(G_fp_logfile, "[%d] Check_Version -> Calling Parse_User_Controls. field %s\n", line_number++, field);
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Version = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Check_Version -> VERSION: %d\n", line_number++, status);
				}
				fclose(fp_User_ini);
			}
			else {
				print_time(0);
				fprintf(G_fp_logfile, "[%d] Check_Version -> Read FAILED\n", line_number++);
			}
		}
		else {
			print_time(0);
			fprintf(G_fp_logfile, "[%d] Check_Version ->  Open FAILED: %s \n", line_number++, l_path);
		}

	}
	else {
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Check_Version ->  SHGetKnownFolderPath FAILED: %s \n", line_number++, l_path);
	}
	return status;
}

int Init_User_Controls() {
	int status = 0;
	int error_flag = 0;
	FILE *fP_User_ini = NULL;
	char init_record[300];
	char field[30] = { 0 };
	WCHAR path[PATH_MAX];
	PWSTR lpPath = path;
	char l_path[PATH_MAX] = { 0 };
	char *record_status = NULL;

	print_time(0);
	fprintf(G_fp_logfile, "[%d] Init_User_Controls -> Called.\n", line_number++);
	print_time(0);
	fprintf(G_fp_logfile, "[%d] Init_User_Controls -> Calling -> Check_Version\n", line_number++);
	status = Check_Version();//Check the VERSION number in the user_controls.ini file. 
	if (status == -1) {//If the VERSION parameter is not present, create the user_controls.ini file
		Create_User_Controls(TRUE);
	}
	else {
		if (status != VERSION) {//The Version parameter exists but does not match this version of the source.
			Create_User_Controls(TRUE);
		}
	}
		/*else {
			Create_User_Controls(FALSE);//
		}*/
	
#if defined(_WIN32)
	HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &lpPath);
	if (SUCCEEDED(hr)) {
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpPath, -1, l_path, sizeof(l_path), NULL, NULL);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Init_User_Controls -> Default Path: %s\n", line_number++, l_path);
		strcat(l_path,"HOMEDIRECTORY/user_controls.ini");
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Init_User_Controls -> user_controls.ini Path: %s\n", line_number++, l_path);
		//strcpy_s(l_path, "C:\Users\Ron\AppData\Local\multus-sdr\user_controls.ini");
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Init_User_Controls -> user_controls.ini Path: %s\n", line_number++, l_path);
		
#endif
		fP_User_ini = fopen(l_path, "r");
		if (fP_User_ini != NULL) {
			record_status = fgets(init_record, sizeof(init_record), fP_User_ini);
			while (record_status != NULL) {
				print_time(0);
				fprintf(G_fp_logfile, "[%d] Init_User_Controls -> Source record: %s", line_number++, init_record);
				memset(field, 0, sizeof(field));
				strcpy(field, "VERSION");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Version = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> VERSION: %d\n", line_number++, status);
				}
				memset(field, 0, sizeof(field));
				strcpy(field, "NB_STATUS");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.NB = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> NB_STATUS: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "SPEAKER_VOLUME");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Speaker_Volume = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> SPEAKER_VOLUME: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "SPEAKER_MUTE");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Speaker_Muted = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> SPEAKER_MUTE: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "MIC_MUTE");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Mic_Muted = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> MIC_MUTE: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "MIC_VOLUME");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Mic_Volume = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> MIC_VOLUME: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "FILTER_LOW_INDEX");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Filter_Low_index = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> FILTER_LOW_INDEX: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "FILTER_HIGH_INDEX");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Filter_High_index = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> FILTER_HIGH_INDEX: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "AGC_INDEX");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.AGC_Index = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> AGC_INDEX: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "ALC_VALUE");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.ALC_Value = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> ALC_VALUE: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "NR_VALUE");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.NR_Value = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> NR_VALUE: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "NB_THRESHOLD");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.NB_Threshold = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> NB_THRESHOLD: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "NB_PULSE_WIDTH");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.NB_Pulse_Width = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> NB_PULSE_WIDTH: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "COMPRESSION_STATUS");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Compression = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> COMPRESSION_STATUS: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "COMPRESSION_LEVEL");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Compression_Level = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> COMPRESSION_LEVEL: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "PANADAPTER_STATUS");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Panadapter_Status = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> PANADAPTER_STATUS: %d\n", line_number++, status);
				}
				memset(field, 0, sizeof(field));
				strcpy(field, "SMETER_STATUS");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Smeter_status = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> SMETER_STATUS: %d\n", line_number++, status);
				}

				memset(field, 0, sizeof(field));
				strcpy(field, "PANADAPTER_AVERAGE");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Panadapter_Average = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> PANADAPTER_AVERAGE: %d\n", line_number++, status);
				}
				memset(field, 0, sizeof(field));
				strcpy(field, "PANADAPTER_FILL");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Panadapter_Fill = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> PANADAPTER_FILL: %d\n", line_number++, status);
				}
				memset(field, 0, sizeof(field));
				strcpy(field, "PANADAPTER_LINE");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Panadapter_Line = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> PANADAPTER_LINE: %d\n", line_number++, status);
				}
				memset(field, 0, sizeof(field));
				strcpy(field, "PANADAPTER_MARKER");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Panadapter_Marker = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> PANADAPTER_MARKER: %d\n", line_number++, status);
				}
				memset(field, 0, sizeof(field));
				strcpy(field, "PANADAPTER_BACKGROUND");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Panadapter_Background = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> PANADAPTER_BACKGROUND: %d\n", line_number++, status);
				}
				memset(field, 0, sizeof(field));
				strcpy(field, "CW_PITCH");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.CW_Pitch = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> CW_PITCH: %d\n", line_number++, status);
				}
				memset(field, 0, sizeof(field));
				strcpy(field, "PANADAPTER_CURSOR");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Panadapter_Cursor = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> PANADAPTER_CURSOR: %d\n", line_number++, status);
				}
				memset(field, 0, sizeof(field));
				strcpy(field, "PANADAPTER_REFRESH");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Panadapter_Refresh = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> PANADAPTER_REFRESH: %d\n", line_number++, status);
				}
				memset(field, 0, sizeof(field));
				strcpy(field, "PANADAPTER_BASE");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Panadapter_Base = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> PANADAPTER_BASE: %d\n", line_number++, status);
				}
				memset(field, 0, sizeof(field));
				strcpy(field, "PANADAPTER_GAIN");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Panadapter_Gain = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> PANADAPTER_GAIN: %d\n", line_number++, status);
				}
				memset(field, 0, sizeof(field));
				strcpy(field, "AUTO_NOTCH");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Auto_Notch = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> AUTO_NOTCH: %d\n", line_number++, status);
				}
				memset(field, 0, sizeof(field));
				strcpy(field, "FILTER_CW_INDEX");
				status = Parse_User_Controls_record(init_record, field);
				if (status != -1) {
					User_Controls.Filter_CW_Index = status;
					print_time(0);
					fprintf(G_fp_logfile, "[%d] Init_User_Controls -> FILTER_CW_INDEX: %d\n", line_number++, status);
				}
				record_status = fgets(init_record, sizeof(init_record), fP_User_ini);
			}
		}
		else {
			print_time(0);
			fprintf(G_fp_logfile, "[%d] Init_User_Controls -> %s: Open FAILED\n", line_number++,l_path);
			status = 0;
			return status;
		}

	}
	//print_time(0);
	//fprintf(G_fp_logfile, "[%d] Init_User_Controls -> Finished\n", line_number++);
	if (fP_User_ini != NULL) {
		fclose(fP_User_ini);
	}
	return status;
}

int Update_User_Controls() {
	int status = 0;
	
	print_time(1);
	fprintf(G_fp_logfile, "[%d] Update_User_Controls -> Called\n", line_number++);
	Gui_send_param(CMD_SET_MIC_VOLUME, User_Controls.Mic_Volume);
	SDRcore_trans_send_param(CMD_SET_MIC_VOLUME, User_Controls.Mic_Volume);
	Gui_send_param(CMD_SET_MIC_MUTE, User_Controls.Mic_Muted);
	SDRcore_trans_send_param(CMD_SET_MIC_MUTE, User_Controls.Mic_Muted);

	Gui_send_param(CMD_SET_SPEAKER_VOLUME, User_Controls.Speaker_Volume);
	SDRcore_recv_send_param(CMD_SET_SPEAKER_VOLUME, User_Controls.Speaker_Volume);
	Gui_send_param(CMD_SET_SPEAKER_MUTE, User_Controls.Speaker_Muted);
	SDRcore_recv_send_param(CMD_SET_SPEAKER_MUTE, User_Controls.Speaker_Muted);

	Gui_send_param(CMD_SET_BW_LOCUT_DEFAULT, User_Controls.Filter_Low_index);
	SDRcore_recv_send_param(CMD_SET_BW_LOCUT, User_Controls.Filter_Low_index);
	Gui_send_param(CMD_SET_BW_HICUT_DEFAULT, User_Controls.Filter_High_index);
	SDRcore_recv_send_param(CMD_SET_BW_HICUT, User_Controls.Filter_High_index);
	Gui_send_param(CMD_SET_CW_BW_DEFAULT, User_Controls.Filter_CW_Index);
	SDRcore_recv_send_param(CMD_SET_CW_BW, User_Controls.Filter_CW_Index);

	Gui_send_param(CMD_SET_COMPRESSION_STATE, User_Controls.Compression);
	Gui_send_param(CMD_SET_COMPRESSION_LEVEL, User_Controls.Compression_Level);
	SDRcore_trans_send_param(CMD_SET_COMPRESSION_STATE, User_Controls.Compression);
	SDRcore_trans_send_param(CMD_SET_COMPRESSION_LEVEL, User_Controls.Compression_Level);

	Gui_send_param(CMD_GET_SET_NB_ENABLE, User_Controls.NB);
	Gui_send_param(CMD_GET_SET_NB_PULSE_WIDTH, User_Controls.NB_Pulse_Width);
	Gui_send_param(CMD_GET_SET_NB_THRESHOLD, User_Controls.NB_Threshold);
	SDRcore_recv_send_param(CMD_GET_SET_NB_ENABLE, User_Controls.NB);
	SDRcore_recv_send_param(CMD_GET_SET_NB_PULSE_WIDTH, User_Controls.NB_Pulse_Width);
	SDRcore_recv_send_param(CMD_GET_SET_NB_THRESHOLD, User_Controls.NB_Threshold);

	Gui_send_param(CMD_SET_CW_PITCH, User_Controls.CW_Pitch);
	SDRcore_recv_send_param(CMD_SET_CW_PITCH, User_Controls.CW_Pitch);

	SDRcore_recv_send_param(CMD_GET_SET_PANADAPTER_SMOOTHING, User_Controls.Panadapter_Average);
	SDRcore_recv_send_param(CMD_GET_SET_PANADAPTER_REFRESH, User_Controls.Panadapter_Refresh);

	SDRcore_recv_send_param(CMD_GET_SET_AGC, User_Controls.AGC_Index);
	Gui_send_param(CMD_GET_SET_AGC, User_Controls.AGC_Index);

	SDRcore_recv_send_param(CMD_GET_SET_AUTO_NOTCH, User_Controls.Auto_Notch);
	Gui_send_param(CMD_GET_SET_AUTO_NOTCH, User_Controls.Auto_Notch);

	print_time(0);
	fprintf(G_fp_logfile, "[%d] Update_User_Controls -> Finished\n", line_number++);
	return status;
}

int Update_User_Controls_ini() {
	int status = 0;
	FILE *fp_User_ini;
	WCHAR path[PATH_MAX];
	PWSTR lpPath = path;
	char l_path[PATH_MAX] = { 0 };

	print_time(0);
	fprintf(G_fp_logfile, "[%d] Update_User_Controls_ini -> Called.\n", line_number++);
#if defined(_WIN32)
	HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &lpPath);
	if (SUCCEEDED(hr)) {
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpPath, -1, l_path, sizeof(l_path), NULL, NULL);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Update_User_Controls_ini -> Default Path: %s\n", line_number++, l_path);
		strcat(l_path, "HOMEDIRECTORY/user_controls.ini");
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Update_User_Controls_ini -> user_controls.ini Path: %s\n", line_number++, l_path);
#endif
		fp_User_ini = fopen(l_path, "w");
		if (fp_User_ini != NULL) {
			fprintf(fp_User_ini,
				"VERSION=%d;\n"
				"SPEAKER_VOLUME=%d;\n"
				"SPEAKER_MUTE=%d;\n"
				"MIC_VOLUME=%d;\n"
				"MIC_MUTE=%d; \n"
				"FILTER_LOW_INDEX=%d;\n"
				"FILTER_HIGH_INDEX=%d;\n"
				"AGC_INDEX=%d;\n"
				"ALC_VALUE=%d;\n"
				"NR_VALUE=%d;\n"
				"NB_STATUS=%d;\n"
				"NB_THRESHOLD=%d;\n"
				"NB_PULSE_WIDTH=%d;\n"
				"COMPRESSION_STATUS=%d;\n"
				"COMPRESSION_LEVEL=%d;\n"
				"PANADAPTER_AVERAGE=%d;\n"
				"PANADAPTER_FILL=%d;\n"
				"PANADAPTER_LINE=%d;\n"
				"PANADAPTER_MARKER=%d;\n"
				"PANADAPTER_BACKGROUND=%d;\n"
				"CW_PITCH=%d;\n"
				"PANADAPTER_CURSOR=%d;\n"
				"PANADAPTER_REFRESH=%d;\n"
				"PANADAPTER_BASE=%d;\n"
				"PANADAPTER_GAIN=%d;\n"
				"AUTO_NOTCH=%d;\n"
				"FILTER_CW_INDEX=%d;\n",
				User_Controls.Version,
				User_Controls.Speaker_Volume,
				User_Controls.Speaker_Muted,
				User_Controls.Mic_Volume,
				User_Controls.Mic_Muted,
				User_Controls.Filter_Low_index,
				User_Controls.Filter_High_index,
				User_Controls.AGC_Index,
				User_Controls.ALC_Value,
				User_Controls.NR_Value,
				User_Controls.NB,
				User_Controls.NB_Threshold,
				User_Controls.NB_Pulse_Width,
				User_Controls.Compression,
				User_Controls.Compression_Level,
				User_Controls.Panadapter_Average,
				User_Controls.Panadapter_Fill,
				User_Controls.Panadapter_Line,
				User_Controls.Panadapter_Marker,
				User_Controls.Panadapter_Background,
				User_Controls.CW_Pitch,
				User_Controls.Panadapter_Cursor,
				User_Controls.Panadapter_Refresh,
				User_Controls.Panadapter_Base,
				User_Controls.Panadapter_Gain,
				User_Controls.Auto_Notch,
				User_Controls.Filter_CW_Index);
			fclose(fp_User_ini);
		}
		else {
			print_time(0);
			fprintf(G_fp_logfile, "[%d] Update_User_Controls_ini -> Update Failed -> File Open FAILED\n", line_number++);
		}
	}
	print_time(0);
	fprintf(G_fp_logfile, "[%d] "
		"VERSION=%d;"
		"SPEAKER_VOLUME=%d;"
		"SPEAKER_MUTE=%d;"
		"MIC_VOLUME=%d;"
		"MIC_MUTE=%d;"
		"FILTER_LOW_INDEX=%d;"
		"FILTER_HIGH_INDEX=%d;"
		"AGC_INDEX=%d;"
		"ALC_VALUE=%d;"
		"NR_VALUE=%d;"
		"NB_STATUS=%d;"
		"NB_THRESHOLD=%d;"
		"NB_PULSE_WIDTH=%d;"
		"COMPRESSION_STATUS=%d;"
		"COMPRESSION_LEVEL=%d;"
		"PANADAPTER_AVERAGE=%d;"
		"PANADAPTER_FILL=%d;"
		"PANADAPTER_LINE=%d;"
		"PANADAPTER_MARKER=%d;"
		"PANADAPTER_BACKGROUND=%d;"
		"CW_PITCH=%d;"
		"PANADAPTER_CURSOR=%d;"
		"PANADAPTER_REFRESH=%d;"
		"PANADAPTER_BASE=%d;"
		"PANADAPTER_GAIN=%d;\n"
		"AUTO_NOTCH=%d;"
		"FILTER_CW_INDEX=%d;\n",
		line_number++,
		User_Controls.Version,
		User_Controls.Speaker_Volume,
		User_Controls.Speaker_Muted,
		User_Controls.Mic_Volume,
		User_Controls.Mic_Muted,
		User_Controls.Filter_Low_index,
		User_Controls.Filter_High_index,
		User_Controls.AGC_Index,
		User_Controls.ALC_Value,
		User_Controls.NR_Value,
		User_Controls.NB,
		User_Controls.NB_Threshold,
		User_Controls.NB_Pulse_Width,
		User_Controls.Compression,
		User_Controls.Compression_Level,
		User_Controls.Panadapter_Average,
		User_Controls.Panadapter_Fill,
		User_Controls.Panadapter_Line,
		User_Controls.Panadapter_Marker,
		User_Controls.Panadapter_Background,
		User_Controls.CW_Pitch,
		User_Controls.Panadapter_Cursor,
		User_Controls.Panadapter_Refresh,
		User_Controls.Panadapter_Base,
		User_Controls.Panadapter_Gain,
		User_Controls.Auto_Notch,
		User_Controls.Filter_CW_Index);
	print_time(0);
	fprintf(G_fp_logfile, "[%d] Update_User_Controls_ini -> Finished.\n", line_number++);
	return status;
}

int Process_User_Controls(uint8_t command, char *buf){
	int status = 0;
	uint8_t cw_pitch = 0;
	 uint8_t update_flag = 1;
	
	opcode_data_8_bit = (uint8_t)(buf[1]);
	t_opcode_data = (uint8_t)(buf[1]);
	opcode_data = (short int*)&buf[1];
	memcpy(&s_opcode_data, opcode_data, 2);

	switch (command) {
	case CMD_SET_PA_BYPASS:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_PA_BYPASS -> VALUE: %d \n", 
			line_number++, t_opcode_data);
		cw_send_parameters(CMD_SET_PA_BYPASS, t_opcode_data, 1);
		break;

	case CMD_GET_SET_AGC:
		User_Controls.AGC_Index = t_opcode_data;
		SDRcore_recv_send_param(CMD_GET_SET_AGC, User_Controls.AGC_Index);
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_AGC -> VALUE: %d \n", line_number++, User_Controls.AGC_Index);
		break;

	case CMD_SET_TWO_TONE:
		SDRcore_trans_send_param(CMD_SET_TWO_TONE, t_opcode_data);
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_TWO_TONE -> VALUE: %d \n", line_number++,t_opcode_data);
		update_flag = 0;
		break;

	case CMD_GET_SET_NB_ENABLE:
		SDRcore_recv_send_param(CMD_GET_SET_NB_ENABLE, t_opcode_data);
		User_Controls.NB = t_opcode_data;
		break;

	case CMD_GET_SET_NB_THRESHOLD:
		SDRcore_recv_send_param(CMD_GET_SET_NB_THRESHOLD, s_opcode_data);
		User_Controls.NB_Threshold = s_opcode_data;
		break;

	case CMD_GET_SET_NB_PULSE_WIDTH:
		SDRcore_recv_send_param(CMD_GET_SET_NB_PULSE_WIDTH, s_opcode_data);
		User_Controls.NB_Pulse_Width = s_opcode_data;
		break;

	case CMD_SET_MIC_VOLUME:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_MIC_VOLUME -> VALUE: %d \n", line_number++, opcode_data_8_bit);
		SDRcore_trans_send_param(CMD_SET_MIC_VOLUME, opcode_data_8_bit);
		User_Controls.Mic_Volume = opcode_data_8_bit;
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_MIC_VOLUME -> Finished \n", line_number++);
		break;

	case CMD_SET_SPEAKER_MUTE:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_SPEAKER_MUTE -> MUTE: %d \n", line_number++, opcode_data_8_bit);
		SDRcore_recv_send_param(CMD_SET_SPEAKER_MUTE, opcode_data_8_bit);
		User_Controls.Speaker_Muted = opcode_data_8_bit;
		G_Speaker_Mute_Status = opcode_data_8_bit;
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_SPEAKER_MUTE -> Finished \n", line_number++);
		break;

	case CMD_SET_SPEAKER_VOLUME:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_SPEAKER_VOLUME -> VALUE: %d \n", line_number++, opcode_data_8_bit);
		SDRcore_recv_send_param(CMD_SET_SPEAKER_VOLUME, opcode_data_8_bit);
		User_Controls.Speaker_Volume = opcode_data_8_bit;
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_SPEAKER_VOLUME -> Finished \n", line_number++);
		break;

	case CMD_SET_MIC_MUTE:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_MIC_MUTE -> MUTE: %d \n", line_number++, opcode_data_8_bit);
		SDRcore_trans_send_param(CMD_SET_MIC_MUTE, opcode_data_8_bit);
		User_Controls.Mic_Muted = opcode_data_8_bit;
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_MIC_MUTE -> Finished \n", line_number++);
		break;

	case CMD_GET_SET_MIC_DEVICE:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_MIC_DEVICE -> sdrcore_trans.ini Record Number: %d \n", line_number++, opcode_data_8_bit);
		SDRcore_trans_send_param(CMD_GET_SET_MIC_DEVICE, opcode_data_8_bit);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_MIC_DEVICE -> Finished \n", line_number++);
		break;

	case CMD_GET_SET_SPEAKER_DEVICE:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_SPEAKER_DEVICE -> sdrcore_trans.ini Record Number: %d \n", line_number++, opcode_data_8_bit);
		SDRcore_recv_send_param(CMD_GET_SET_SPEAKER_DEVICE, opcode_data_8_bit);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_SPEAKER_DEVICE -> Finished \n", line_number++);
		break;

	case CMD_SET_TX_HICUT:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_TX_HICUT Called \n", line_number++);
		SDRcore_trans_send_param(CMD_SET_TX_HICUT, opcode_data_8_bit);
		break;

	case CMD_SET_BW_LOCUT:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> Command_Interface -> CMD_SET_BW_LOCUT Called \n", line_number++);
		//G_low_cut = (int)s_opcode_data;
		User_Controls.Filter_Low_index = opcode_data_8_bit;
		SDRcore_recv_send_param(CMD_SET_BW_LOCUT, opcode_data_8_bit);
		break;

	case CMD_SET_BW_LOCUT_DEFAULT:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> Command_Interface -> CMD_SET_BW_LOCUT_DEFAULT Called \n", line_number++);
		User_Controls.Filter_Low_index = opcode_data_8_bit;
		break;

	case CMD_SET_BW_HICUT:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> Command_Interface -> CMD_SET_BW_HICUT Called \n", line_number++);
		//G_high_cut = (int)s_opcode_data;
		User_Controls.Filter_High_index = opcode_data_8_bit;
		SDRcore_recv_send_param(CMD_SET_BW_HICUT, opcode_data_8_bit);
		break;

	case CMD_SET_BW_HICUT_DEFAULT:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> Command_Interface -> CMD_SET_BW_HICUT_DEFAULT Called \n", line_number++);
		User_Controls.Filter_High_index = opcode_data_8_bit;
		break;

	case CMD_SET_CW_BW:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> Command_Interface -> CMD_SET_CW_BW Called \n", line_number++);
		SDRcore_recv_send_param(CMD_SET_CW_BW, opcode_data_8_bit);
		break;

	case CMD_SET_CW_BW_DEFAULT:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> Command_Interface -> CMD_SET_CW_BW_DEFAULT Called \n", line_number++);
		User_Controls.Filter_CW_Index = opcode_data_8_bit;
		break;

	case CMD_SET_COMPRESSION_STATE:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_COMPRESSION_STATE -> Compression State: %d \n", line_number++, opcode_data_8_bit);
		SDRcore_trans_send_param(CMD_SET_COMPRESSION_STATE, opcode_data_8_bit);
		User_Controls.Compression = opcode_data_8_bit;
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_COMPRESSION_STATE -> Finished \n", line_number++);
		break;

	case CMD_SET_COMPRESSION_LEVEL:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_COMPRESSION_LEVEL -> Compression Level: %d \n", line_number++, opcode_data_8_bit);
		SDRcore_trans_send_param(CMD_SET_COMPRESSION_LEVEL, opcode_data_8_bit);
		User_Controls.Compression_Level = opcode_data_8_bit;
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_COMPRESSION_LEVEL -> Finished \n", line_number++);
		break;

	case CMD_GET_SET_AUTO_NOTCH:
		print_time(1);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CDM_GET_SET_AUTO_NOTCH -> Auto Notch State: %d \n", line_number++, 
			opcode_data_8_bit);
		SDRcore_recv_send_param(CMD_GET_SET_AUTO_NOTCH, opcode_data_8_bit);
		User_Controls.Auto_Notch = opcode_data_8_bit;
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_AUTO_NOTCH -> Finished \n", line_number++);
		break;

	case CMD_SET_CW_PITCH:
		cw_pitch = opcode_data_8_bit;
		switch (cw_pitch) {
		case 0:
			G_CW_Offset = -400;
			break;
		case 1:
			G_CW_Offset = -500;
			break;
		case 2:
			G_CW_Offset = -600;
			break;
		case 3:
			G_CW_Offset = -700;
			break;
		case 4:
			G_CW_Offset = -800;
			break;
		case 5:
			G_CW_Offset = -1000;
			break;
		}
		
		SetHWLO(G_tune_freq);
		SDRcore_recv_send_param(CMD_SET_CW_PITCH, cw_pitch);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_SET_CW_PITCH -> G_cw_pitch: %d, Index: %d\n", line_number++,
			G_CW_Offset,cw_pitch);
		User_Controls.CW_Pitch = cw_pitch;
		break;

	case CMD_GET_SET_PANADAPTER_SMOOTHING:
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_PANADAPTER_SMOOTHING: %d\n", line_number++, t_opcode_data);
		User_Controls.Panadapter_Average = t_opcode_data;
		SDRcore_recv_send_param(CMD_GET_SET_PANADAPTER_SMOOTHING, (t_opcode_data));
		break;

	case CMD_GET_SET_PANADAPTER_GAIN:
		User_Controls.Panadapter_Gain = s_opcode_data;
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_PANADAPTER_GAIN: %d\n", line_number++, s_opcode_data);
		break;

	case CMD_GET_SET_PANADAPTER_BASE:
		User_Controls.Panadapter_Base = s_opcode_data;
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_PANADAPTER_BASE: %d\n", line_number++, s_opcode_data);
		break;

	case CMD_GET_SET_PANADAPTER_REFRESH:
		SDRcore_recv_send_param(CMD_GET_SET_PANADAPTER_REFRESH, t_opcode_data);
		User_Controls.Panadapter_Refresh = t_opcode_data;
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_PANADAPTER_REFRESH: %d\n", line_number++, t_opcode_data);
		break;

	case CMD_GET_SET_PANADAPTER_FILL:
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_PANADAPTER_FILL: %d\n", line_number++, t_opcode_data);
		User_Controls.Panadapter_Fill = t_opcode_data;
		break;

	case CMD_GET_SET_PANADAPTER_LINE:
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_PANADAPTER_LINE: %d\n", line_number++, t_opcode_data);
		User_Controls.Panadapter_Line = t_opcode_data;
		break;

	case CMD_GET_SET_PANADAPTER_MARKER:
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_PANADAPTER_MARKER: %d\n", line_number++, t_opcode_data);
		User_Controls.Panadapter_Marker = t_opcode_data;
		break;

	case CMD_GET_SET_PANADAPTER_BACKGROUND:
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_PANADAPTER_BACKGROUND: %d\n", line_number++, t_opcode_data);
		User_Controls.Panadapter_Background = t_opcode_data;
		break;

	case CMD_GET_SET_PANADAPTER_CURSOR:
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Process_User_Controls -> CMD_GET_SET_PANADAPTER_CURSOR: %d\n", line_number++, t_opcode_data);
		User_Controls.Panadapter_Cursor = t_opcode_data;
		break;

	}
	if (update_flag) {
		Update_User_Controls_ini();
	}
	return status;
}

int Write_User_Controls() {
	int status = 0;
	FILE *fp_User_ini;
	WCHAR path[PATH_MAX];
	PWSTR lpPath = path;
	char l_path[PATH_MAX] = { 0 };

#if defined(_WIN32)
	HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &lpPath);
	if (SUCCEEDED(hr)) {
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpPath, -1, l_path, sizeof(l_path), NULL, NULL);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Write_User_Controls -> Default Path: %s\n", line_number++, l_path);
		strcat(l_path, "HOMEDIRECTORY/user_controls.ini");
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Write_User_Controls -> user_controls.ini Path: %s\n", line_number++, l_path);
#endif
		fp_User_ini = fopen(l_path, "w");
		if (fp_User_ini != NULL) {
			fprintf(fp_User_ini,
				"VERSION=%d;\n"
				"SPEAKER_VOLUME=%d;\n"
				"SPEAKER_MUTE=%d;\n"
				"MIC_VOLUME=%d;\n"
				"MIC_MUTE=%d;\n"
				"FILTER_LOW_INDEX=%d;\n"
				"FILTER_HIGH_INDEX=%d;\n"
				"AGC_INDEX=%d;\n"
				"ALC_VALUE=%d;\n"
				"NR_VALUE=%d;\n"
				"NB_STATUS=%d;\n"
				"NB_THRESHOLD=%d;\n"
				"NB_PULSE_WIDTH=%d;\n"
				"COMPRESSION_STATUS=%d;\n"
				"COMPRESSION_LEVEL=%d;\n"
				"PANADAPTER_AVERAGE=%d;\n"
				"PANADAPTER_FILL=%d;\n"
				"PANADAPTER_LINE=%d;\n"
				"PANADAPTER_MARKER=%d;\n"
				"PANADAPTER_BACKGROUND=%d;\n"
				"CW_PITCH=%d;\n"
				"PANADAPTER_CURSOR=%d;\n"
				"PANADAPTER_REFRESH=%d;\n"
				"PANADAPTER_BASE=%d;\n"
				"PANADAPTER_GAIN=%d;\n"
				"AUTO_NOTCH=%d;\n"
				"FILTER_CW_INDEX=%d;\n",
				User_Controls.Version = VERSION,
				User_Controls.Speaker_Volume = 0,
				User_Controls.Speaker_Muted = 0,
				User_Controls.Mic_Volume = 0,
				User_Controls.Mic_Muted = 0,
				User_Controls.Filter_Low_index = 0,
				User_Controls.Filter_High_index = 0,
				User_Controls.AGC_Index = 0,
				User_Controls.ALC_Value = 0,
				User_Controls.NR_Value = 0,
				User_Controls.NB = 0,
				User_Controls.NB_Threshold = 10,
				User_Controls.NB_Pulse_Width = 30,
				User_Controls.Compression = 0,
				User_Controls.Compression_Level = 0,
				User_Controls.Panadapter_Average = 0,
				User_Controls.Panadapter_Fill = 0,
				User_Controls.Panadapter_Line = 0,
				User_Controls.Panadapter_Marker = 0,
				User_Controls.Panadapter_Background = 0,
				User_Controls.CW_Pitch = 2,
				User_Controls.Panadapter_Cursor = 0,
				User_Controls.Panadapter_Refresh = 6,
				User_Controls.Panadapter_Base = 0,
				User_Controls.Panadapter_Gain = 0,
				User_Controls.Auto_Notch = 0,
				User_Controls.Filter_CW_Index = 0);
			fclose(fp_User_ini);
		}
		else {
			print_time(0);
			fprintf(G_fp_logfile, "[%d] Write_User_Controls -> Create user_controls.ini -> FAILED: %s \n", line_number++,l_path);
		}
	}
	else {
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Write_User_Controls -> SHGetKnownFolderPath FAILED \n",line_number++);
	}
	print_time(0);
	fprintf(G_fp_logfile, "[%d] Write_User_Controls -> Finished \n", line_number++);
	return status;
}

int Create_User_Controls(int force) {
	int status = 0;
	FILE *fp_User_ini;
	WCHAR path[PATH_MAX];
	PWSTR lpPath = path;
	char l_path[PATH_MAX] = { 0 };

	print_time(0);
	fprintf(G_fp_logfile, "[%d] Create_User_Controls -> Called.\n", line_number++);
#if defined(_WIN32)
	HRESULT hr = SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &lpPath);
	if (SUCCEEDED(hr)) {
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, lpPath, -1, l_path, sizeof(l_path), NULL, NULL);
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Create_User_Controls -> Default Path: %s\n", line_number++, l_path);
		strcat(l_path, "HOMEDIRECTORY/user_controls.ini");
		print_time(0);
		fprintf(G_fp_logfile, "[%d] Create_User_Controls -> user_controls.ini Path: %s\n", line_number++, l_path);
#endif

		if (!force) {
			fp_User_ini = fopen(l_path, "r");
			if (fp_User_ini == NULL) {
				print_time(0);
				fprintf(G_fp_logfile, "[%d] Create_User_Controls -> Does not exist. -> Creating user_controls.ini. Force: %d\n", line_number++, force);
				Write_User_Controls();
			}
			else {
				print_time(0);
				fprintf(G_fp_logfile, "[%d] Create_User_Controls -> user_controls exists. Not creating user_controls.ini. Force: %d\n", line_number++
																																,force);
				fclose(fp_User_ini);
			}
		}
		else {
			print_time(0);
			fprintf(G_fp_logfile, "[%d] Create_User_Controls -> force: %d. Forcing create of user_controls.ini\n", line_number++, force);
			Write_User_Controls();
		}
	}
	return status;
}



