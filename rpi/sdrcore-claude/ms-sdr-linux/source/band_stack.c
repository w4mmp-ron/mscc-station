#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
#include "usbavrcmd.h"
#include "usbavrcmd.h"
#include "SRDLL.h"
#include "extern.h"
#include "band_stack.h"

enum bands {
        B10, B12, B15, B17, B20, B30, B40, B60, B80, B160, B_NOBAND
} G_bands;

extern struct band_stack G_band_stack[10];

const char *homedir;

char *strtok_r(char *str, const char *delim, char **saveptr);

int send_band_stack(uint8_t index, uint8_t element)
{
        uint32_t freq;
        char mode;
        uint8_t band = 0;

        print_time(0);
        fprintf(G_fp_logfile, "[%d] send_band_stack-> index %d, element %d\n", line_number++, index, element);
        freq = G_band_stack[index].freq[element];
        mode = G_band_stack[index].mode[element];
        switch (index) {
        case 0:
                band = 10;
                break;
        case 1:
                band = 12;
                break;
        case 2:
                band = 15;
                break;
        case 3:
                band = 17;
                break;
        case 4:
                band = 20;
                break;
        case 5:
                band = 30;
                break;
        case 6:
                band = 40;
                break;
        case 7:
                band = 60;
                break;
        case 8:
                band = 80;
                break;
        case 9:
                band = 160;
                break;
        }
        if (G_network_initialized) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] send_band_stack -> Calling Gui_send_param \n", line_number++);
                Gui_send_param(CMD_GET_STACK_FREQ, freq);
                Gui_send_param(CMD_GET_STACK_MODE, mode);
                Gui_send_param(CMD_GET_BAND_STACK_BAND, band);
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] send_band_stack-> finished\n", line_number++);
        return 1;
}

int gui_send_band_stack()
{
        int band = 0;
        //int index = 0;

        band = G_bands;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] gui_send_band_stack -> band %d\n", line_number++, band);
        Gui_send_param(CMD_GET_BAND_STACK, band);
        //for (index = 0; index < 3; index++) {
        //	Gui_send_param(CMD_GET_BAND_STACK, G_band_stack[band].freq[index]);
        //	Gui_send_param(CMD_GET_BAND_STACK, G_band_stack[band].mode[index]);
        //}
        print_time(0);
        fprintf(G_fp_logfile, "[%d] update_band_stack -> Finished\n", line_number++);
        return 1;
}

int update_band_stack()
{
        FILE *Band_stack_update;
        char file_name[PATH_MAX] = {0};
        //int lenght = 0;
        int band = 0;
        int index = 0;

        if ((homedir = getenv("HOME")) != NULL) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] update_band_stack -> Default Path: %s\n", line_number++, homedir);
                strcpy(file_name, homedir);
                strcat(file_name, "/.local/share/mscc/Band_stack.ini");
                print_time(0);
                fprintf(G_fp_logfile, "[%d] update_band_stack -> Band_stack Path: %s\n", line_number++, homedir);
                Band_stack_update = fopen(file_name, "w");
                if (Band_stack_update != NULL) {
                        for (band = 0; band < 10; band++) {
                                fprintf(Band_stack_update, "%d", band);
                                for (index = 0; index < 3; index++) {
                                        fprintf(Band_stack_update, ",%d:%c", G_band_stack[band].freq[index], G_band_stack[band].mode[index]);
                                }
                                fprintf(Band_stack_update, ";\n");
                        }
                } else {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] update_band_stack -> Open file failed\n", line_number++);
                        return 0;
                }
                print_time(0);
                fprintf(G_fp_logfile, "[%d] update_band_stack -> Finished\n", line_number++);
                fclose(Band_stack_update);
        }
        return 1;
}

int get_band_stack()
{
        FILE *Band_stack_ini;
        char file_name[PATH_MAX] = {0};
        int lenght = 0;
        int band = 0;
        char cw_init_record[132];
        char delim[2];
        char *token = NULL;
        char *next_token = NULL;
        int index = 0;

        delim[1] = 0;
        if ((homedir = getenv("HOME")) != NULL) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] get_band_stack -> Default Path: %s\n", line_number++, homedir);
                strcpy(file_name, homedir);
                strcat(file_name, "/.local/share/mscc/Band_stack.ini");
                print_time(0);
                fprintf(G_fp_logfile, "[%d] get_band_stack -> Band_stack Path: %s\n", line_number++, homedir);
                Band_stack_ini = fopen(file_name, "r");
                if (Band_stack_ini != NULL) {
                        while (fgets(cw_init_record, sizeof(cw_init_record), Band_stack_ini) != NULL) {
                                delim[0] = ',';
                                token = strtok_r(cw_init_record, delim, &next_token);
                                band = atoi(token);
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] get_band_stack -> Init line: %d,", line_number++, band);
                                for (index = 0; index < 3; index++) {
                                        delim[0] = ':';
                                        token = strtok_r(NULL, delim, &next_token);
                                        G_band_stack[band].freq[index] = atoi(token);
                                        delim[0] = ',';
                                        token = strtok_r(NULL, delim, &next_token);
                                        G_band_stack[band].mode[index] = token[0];
                                        fprintf(G_fp_logfile, "%d:%c,", G_band_stack[band].freq[index], G_band_stack[band].mode[index]);
                                }
                                fprintf(G_fp_logfile, "\n");
                        }
                } else {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] get_band_stack -> Open file failed\n", line_number++);
                        return 0;
                }
                print_time(0);
                fprintf(G_fp_logfile, "[%d] get_band_stack -> Finished\n", line_number++);
                fclose(Band_stack_ini);
        }
        //update_band_stack();
        return 1;
}

void set_band_stack(uint32_t i, char mode)
{
        static uint8_t b_160_stack_count = 0;
        static uint8_t b_80_stack_count = 0;
        static uint8_t b_60_stack_count = 0;
        static uint8_t b_40_stack_count = 0;
        static uint8_t b_30_stack_count = 0;
        static uint8_t b_20_stack_count = 0;
        static uint8_t b_17_stack_count = 0;
        static uint8_t b_15_stack_count = 0;
        static uint8_t b_12_stack_count = 0;
        static uint8_t b_10_stack_count = 0;

        print_time(0);
        fprintf(G_fp_logfile, "[%d] set_band_stack called -> \n", line_number++);
        G_bands = B_NOBAND;
        if (i >= 1780000 && i <= 2010000) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] set_band_stack -> Band 160, stack count %d\n", line_number++, b_160_stack_count);
                G_bands = B160;
                switch (b_160_stack_count) {
                case 0:
                        G_band_stack[G_bands].freq[b_160_stack_count] = i;
                        G_band_stack[G_bands].mode[b_160_stack_count] = mode;
                        break;
                case 1:
                        G_band_stack[G_bands].freq[b_160_stack_count] = i;
                        G_band_stack[G_bands].mode[b_160_stack_count] = mode;
                        break;
                case 2:
                        G_band_stack[G_bands].freq[b_160_stack_count] = i;
                        G_band_stack[G_bands].mode[b_160_stack_count] = mode;
                        break;
                }
                if (++b_160_stack_count > 2) b_160_stack_count = 0;
        }//160M 
        else if (i >= 3480000 && i <= 4010000) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] set_band_stack -> Band 80\n", line_number++);
                G_bands = B80;
                switch (b_80_stack_count) {
                case 0:
                        G_band_stack[G_bands].freq[b_80_stack_count] = i;
                        G_band_stack[G_bands].mode[b_80_stack_count] = mode;
                        break;
                case 1:
                        G_band_stack[G_bands].freq[b_80_stack_count] = i;
                        G_band_stack[G_bands].mode[b_80_stack_count] = mode;
                        break;
                case 2:
                        G_band_stack[G_bands].freq[b_80_stack_count] = i;
                        G_band_stack[G_bands].mode[b_80_stack_count] = mode;
                        break;
                }
                if (++b_80_stack_count > 2) b_80_stack_count = 0;
        }//80-75M
        else if (i >= 5310500 && i <= 5413500) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] set_band_stack -> Band 60\n", line_number++);
                G_bands = B60;
                switch (b_60_stack_count) {
                case 0:
                        G_band_stack[G_bands].freq[b_60_stack_count] = i;
                        G_band_stack[G_bands].mode[b_60_stack_count] = mode;
                        break;
                case 1:
                        G_band_stack[G_bands].freq[b_60_stack_count] = i;
                        G_band_stack[G_bands].mode[b_60_stack_count] = mode;
                        break;
                case 2:
                        G_band_stack[G_bands].freq[b_60_stack_count] = i;
                        G_band_stack[G_bands].mode[b_60_stack_count] = mode;
                        break;
                }
                if (++b_60_stack_count > 2) b_60_stack_count = 0;
        }//60M
        else if (i >= 6980000 && i <= 7310000) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] set_band_stack -> Band 40\n", line_number++);
                G_bands = B40;
                switch (b_40_stack_count) {
                case 0:
                        G_band_stack[G_bands].freq[b_40_stack_count] = i;
                        G_band_stack[G_bands].mode[b_40_stack_count] = mode;
                        break;
                case 1:
                        G_band_stack[G_bands].freq[b_40_stack_count] = i;
                        G_band_stack[G_bands].mode[b_40_stack_count] = mode;
                        break;
                case 2:
                        G_band_stack[G_bands].freq[b_40_stack_count] = i;
                        G_band_stack[G_bands].mode[b_40_stack_count] = mode;
                        break;
                }
                if (++b_40_stack_count > 2) b_40_stack_count = 0;
        }//40M
        else if (i >= 10080000 && i <= 10160000) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] set_band_stack -> Band 30\n", line_number++);
                G_bands = B30;
                switch (b_30_stack_count) {
                case 0:
                        G_band_stack[G_bands].freq[b_30_stack_count] = i;
                        G_band_stack[G_bands].mode[b_30_stack_count] = mode;
                        break;
                case 1:
                        G_band_stack[G_bands].freq[b_30_stack_count] = i;
                        G_band_stack[G_bands].mode[b_30_stack_count] = mode;
                        break;
                case 2:
                        G_band_stack[G_bands].freq[b_30_stack_count] = i;
                        G_band_stack[G_bands].mode[b_30_stack_count] = mode;
                        break;
                }
                if (++b_30_stack_count > 2) b_30_stack_count = 0;
        }//30M
        else if (i >= 18048000 && i <= 18170000) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] set_band_stack -> Band 17\n", line_number++);
                G_bands = B17;
                switch (b_17_stack_count) {
                case 0:
                        G_band_stack[G_bands].freq[b_17_stack_count] = i;
                        G_band_stack[G_bands].mode[b_17_stack_count] = mode;
                        break;
                case 1:
                        G_band_stack[G_bands].freq[b_17_stack_count] = i;
                        G_band_stack[G_bands].mode[b_17_stack_count] = mode;
                        break;
                case 2:
                        G_band_stack[G_bands].freq[b_17_stack_count] = i;
                        G_band_stack[G_bands].mode[b_17_stack_count] = mode;
                        break;
                }
                if (++b_17_stack_count > 2) b_17_stack_count = 0;
        }//17M
        else if (i >= 13980000 && i <= 14360000) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] set_band_stack -> Band 20\n", line_number++);
                G_bands = B20;
                switch (b_20_stack_count) {
                case 0:
                        G_band_stack[G_bands].freq[b_20_stack_count] = i;
                        G_band_stack[G_bands].mode[b_20_stack_count] = mode;
                        break;
                case 1:
                        G_band_stack[G_bands].freq[b_20_stack_count] = i;
                        G_band_stack[G_bands].mode[b_20_stack_count] = mode;
                        break;
                case 2:
                        G_band_stack[G_bands].freq[b_20_stack_count] = i;
                        G_band_stack[G_bands].mode[b_20_stack_count] = mode;
                        break;
                }
                if (++b_20_stack_count > 2) b_20_stack_count = 0;
        }//20M
        else if (i >= 20980000 && i <= 21460000) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] set_band_stack -> Band 15\n", line_number++);
                G_bands = B15;
                switch (b_15_stack_count) {
                case 0:
                        G_band_stack[G_bands].freq[b_15_stack_count] = i;
                        G_band_stack[G_bands].mode[b_15_stack_count] = mode;
                        break;
                case 1:
                        G_band_stack[G_bands].freq[b_15_stack_count] = i;
                        G_band_stack[G_bands].mode[b_15_stack_count] = mode;
                        break;
                case 2:
                        G_band_stack[G_bands].freq[b_15_stack_count] = i;
                        G_band_stack[G_bands].mode[b_15_stack_count] = mode;
                        break;
                }
                if (++b_15_stack_count > 2) b_15_stack_count = 0;
        }//15M
        else if (i >= 24870000 && i <= 25000000) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] set_band_stack -> Band 12\n", line_number++);
                G_bands = B12;
                switch (b_12_stack_count) {
                case 0:
                        G_band_stack[G_bands].freq[b_12_stack_count] = i;
                        G_band_stack[G_bands].mode[b_12_stack_count] = mode;
                        break;
                case 1:
                        G_band_stack[G_bands].freq[b_12_stack_count] = i;
                        G_band_stack[G_bands].mode[b_12_stack_count] = mode;
                        break;
                case 2:
                        G_band_stack[G_bands].freq[b_12_stack_count] = i;
                        G_band_stack[G_bands].mode[b_12_stack_count] = mode;
                        break;
                }
                if (++b_12_stack_count > 2) b_12_stack_count = 0;
        }//12M
        else if (i >= 27980000 && i <= 29710000) {
                print_time(0);
                fprintf(G_fp_logfile, "[%d] set_band_stack -> Band 10\n", line_number++);
                G_bands = B10;

                switch (b_10_stack_count) {
                case 0:
                        G_band_stack[G_bands].freq[b_10_stack_count] = i;
                        G_band_stack[G_bands].mode[b_10_stack_count] = mode;
                        break;
                case 1:
                        G_band_stack[G_bands].freq[b_10_stack_count] = i;
                        G_band_stack[G_bands].mode[b_10_stack_count] = mode;
                        break;
                case 2:
                        G_band_stack[G_bands].freq[b_10_stack_count] = i;
                        G_band_stack[G_bands].mode[b_10_stack_count] = mode;
                        break;
                }
                if (++b_10_stack_count > 2) b_10_stack_count = 0;
        }//10M


        update_band_stack();
        print_time(0);
        fprintf(G_fp_logfile, "[%d] set_band_stack finished \n", line_number++);
}





