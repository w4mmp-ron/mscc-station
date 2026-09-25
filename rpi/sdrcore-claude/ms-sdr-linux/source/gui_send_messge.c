/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/timeb.h>
#include <sys/time.h>

#include "usbavrcmd.h"
#include <usb.h>
#include "SRDLL.h"
#include "extern.h"
#include "version.h"
#include "band_stack.h"
#include "last_used.h"
#include "iq.h"

/*const char *homedir;

struct {
        uint8_t Master_controller;
        uint8_t MFC;
        uint8_t Solidus_temp_sensor;
        uint8_t Meter;
        uint8_t IQBD;
        uint8_t Current_sensor;
} I2C_Controls;

int Parse_I2C_Controls_record(char *record, char *field)
{
        int status = -1;
        char source_field[80] = {0};
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

int Init_I2C_Controls()
{
        int status = 0;
        //int error_flag = 0;
        FILE *fP_User_ini = NULL;
        char init_record[300];
        char field[80] = {0};
        char file_name[PATH_MAX] = {0};
        char *record_status = NULL;

        print_time(1);
        fprintf(G_fp_logfile, "[%d] Init_I2C_Controls -> Called.\n", line_number++);

        if ((homedir = getenv("HOME")) != NULL) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Init_I2C_Controls -> Default Path: %s\n", line_number++, homedir);
                strcat(file_name, homedir);
                strcat(file_name, "/.local/share/mscc/i2c.ini");
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Init_I2C_Controls -> user_controls.ini Path: %s\n", line_number++, file_name);
                fP_User_ini = fopen(file_name, "r");
                if (fP_User_ini != NULL) {
                        record_status = fgets(init_record, sizeof(init_record), fP_User_ini);
                        while (record_status != NULL) {
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] Init_I2C_Controls -> Source record: %s", line_number++, init_record);
                                memset(field, 0, sizeof(field));
                                strcpy(field, "G_MASTER_CONTROLLER_attached");
                                status = Parse_I2C_Controls_record(init_record, field);
                                if (status != -1) {
                                        I2C_Controls.Master_controller = status;
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] Init_I2C_Controls -> G_MASTER_CONTROLLER_attached: %d\n", line_number++, status);
                                }
                                memset(field, 0, sizeof(field));
                                strcpy(field, "G_MFC_attached");
                                status = Parse_I2C_Controls_record(init_record, field);
                                if (status != -1) {
                                        I2C_Controls.MFC = status;
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] Init_I2C_Controls -> G_MFC_attached: %d\n", line_number++, status);
                                }
                                memset(field, 0, sizeof(field));
                                strcpy(field, "G_SOLIDUS_TEMP_SENSOR_attached");
                                status = Parse_I2C_Controls_record(init_record, field);
                                if (status != -1) {
                                        I2C_Controls.Solidus_temp_sensor = status;
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] Init_I2C_Controls -> G_SOLIDUS_TEMP_SENSOR_attached: %d\n", line_number++, status);
                                }
                                memset(field, 0, sizeof(field));
                                strcpy(field, "G_METER_attached");
                                status = Parse_I2C_Controls_record(init_record, field);
                                if (status != -1) {
                                        I2C_Controls.Meter = status;
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] Init_I2C_Controls -> G_METER_attached: %d\n", line_number++, status);
                                }
                                memset(field, 0, sizeof(field));
                                strcpy(field, "G_IQBD_attached");
                                status = Parse_I2C_Controls_record(init_record, field);
                                if (status != -1) {
                                        I2C_Controls.IQBD = status;
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] Init_I2C_Controls -> G_IQBD_attached: %d\n", line_number++, status);
                                }
                                memset(field, 0, sizeof(field));
                                strcpy(field, "G_CURRENT_SENSOR_attached");
                                status = Parse_I2C_Controls_record(init_record, field);
                                if (status != -1) {
                                        I2C_Controls.Current_sensor = status;
                                        print_time(0);
                                        fprintf(G_fp_logfile, "[%d] Init_I2C_Controls -> G_CURRENT_SENSOR_attached: %d\n", line_number++, status);
                                }
                                record_status = fgets(init_record, sizeof(init_record), fP_User_ini);
                        }
                } else {
                        status = -1;
                }
        } else {
                status = -1;
        }
        return status;
}

void Gui_Send_Message(char * message)
{
        char data[PATH_MAX];

        strcpy(data, message);
        Gui_send_param_extended(CMD_SET_SERVER_ERROR, (byte *) data, strlen(data));
}

int Check_Threads()
{
        int status = 0;
        int sleep_time = 1;

        status = Init_I2C_Controls();
        if (I2C_Controls.Master_controller == 1) {
                if (G_MASTER_CONTROLLER_attached == FALSE) {
                        Gui_Send_Message("MASTER_CONTROLLER THREAD FAILED. \n MSCC WILL NOW STOP ");
                        sleep_time = 3000;
                        status = -1;
                }
        }

        if (I2C_Controls.MFC == 1) {
                if (G_MFC_attached == FALSE) {
                        Gui_Send_Message("MFC THREAD NOT RUNNING. \n MSCC WILL NOW STOP");
                        sleep_time = 3000;
                        status = -1;
                }
        }

        if (I2C_Controls.Solidus_temp_sensor == 1) {
                if (G_SOLIDUS_TEMP_SENSOR_attached == FALSE) {
                        Gui_Send_Message("SOLIDUS TEMPERATURE THREAD NOT RUNNING. \n MSCC WILL NOW STOP");
                        sleep_time = 3000;
                        status = -1;
                }
        }

        if (I2C_Controls.Meter == 1) {
                if (G_METER_attached == FALSE) {
                        Gui_Send_Message("METER THREAD NOT RUNNING. \n MSCC WILL NOW STOP ");
                        sleep_time = 3000;
                        status = -1;
                }
        }

        if (I2C_Controls.IQBD == 1) {
                if (G_IQBD_attached == FALSE) {
                        Gui_Send_Message("IQBD THREAD NOT RUNNING. \n MSCC WILL NOW STOP ");
                        sleep_time = 3000;
                        status = -1;
                }
        }

        if (I2C_Controls.Current_sensor == 1) {
                if (G_CURRENT_SENSOR_attached == FALSE) {
                        Gui_Send_Message("CURRENT SENSOR THREAD NOT RUNNING. \n MSCC WILL NOW STOP ");
                        sleep_time = 3000;
                        status = -1;
                }
        }
        Sleep(sleep_time);
        return status;
}*/
