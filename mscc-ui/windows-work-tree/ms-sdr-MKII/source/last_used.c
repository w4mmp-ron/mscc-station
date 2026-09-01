#define _CRT_SECURE_NO_WARNINGS 1
#include <math.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
#include "usbavrcmd.h"
#include "usbavrcmd.h"
//#include "SRDLL.h"
#include "extern.h"
#include "last_used.h"
#include "version.h"

enum last_bands {
    B10, B12, B15, B17, B20, B30, B40, B60, B80, B160, B630, B2200, STARTUP_BAND
} G_last_bands;

struct {
    uint16_t band;
    UINT32 freq;
    char mode;
} G_last_used_stack_VFOA[12];

struct {
    uint16_t band;
    UINT32 freq;
    char mode;
} G_last_used_stack_VFOB[12];

uint8_t G_vfo = VFO_A;
uint8_t G_current_vfo = VFO_A;
//const char *homedir;

int Send_last_used_VFO_A(uint16_t band, uint8_t element) {
    uint8_t selected_band = 100;
    char mode = 'N';
    uint32_t freq = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Send_last_used_VFO_A-> band %d, element %d\n", line_number++, band, element);
    switch (band) {
        case 2200:
            selected_band = 11;
            break;
        case 630:
            selected_band = 10;
            break;
        case 160:
            selected_band = 9;
            break;
        case 80:
            selected_band = 8;
            break;
        case 60:
            selected_band = 7;
            break;
        case 40:
            selected_band = 6;
            break;
        case 30:
            selected_band = 5;
            break;
        case 20:
            selected_band = 4;
            break;
        case 17:
            selected_band = 3;
            break;
        case 15:
            selected_band = 2;
            break;
        case 12:
            selected_band = 1;
            break;
        case 10:
            selected_band = 0;
            break;
    }
    if (G_network_initialized) {
        freq = G_last_used_stack_VFOA[selected_band].freq;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Send_last_used_VFO_A . Calling Gui_send_param . CMD_GET_SET_LAST_USED_FREQ . Frequency: %d \n",
                line_number++, freq);
        Sleep(100);
        Gui_send_param(CMD_GET_SET_LAST_USED_FREQ, freq);
        mode = G_last_used_stack_VFOA[selected_band].mode;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Send_last_used_VFO_A . Calling Gui_send_param . CMD_GET_SET_LAST_USED_MODE . Mode: %c \n",
                line_number++, mode);
        Sleep(100);
        Gui_send_param(CMD_GET_SET_LAST_USED_MODE, mode);
        Sleep(100);

    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Send_last_used_VFO_A-> Finished . band: %d, freq: %ld, mode: %c\n", line_number++,
            selected_band, freq, mode);
    return 1;
}

int Send_last_used_VFO_B(uint16_t band, uint8_t element) {
    uint8_t selected_band = 100;
    char mode = 'N';
    uint32_t freq = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Send_last_used_VFO_B-> band %d, element %d\n", line_number++, band, element);
    switch (band) {
        case 2200:
            selected_band = 11;
            break;
        case 630:
            selected_band = 10;
            break;
        case 160:
            selected_band = 9;
            break;
        case 80:
            selected_band = 8;
            break;
        case 60:
            selected_band = 7;
            break;
        case 40:
            selected_band = 6;
            break;
        case 30:
            selected_band = 5;
            break;
        case 20:
            selected_band = 4;
            break;
        case 17:
            selected_band = 3;
            break;
        case 15:
            selected_band = 2;
            break;
        case 12:
            selected_band = 1;
            break;
        case 10:
            selected_band = 0;
            break;
    }
    if (G_network_initialized) {
        freq = G_last_used_stack_VFOB[selected_band].freq;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Send_last_used_VFO_B . Calling Gui_send_param . CMD_GET_SET_LAST_USED_FREQ . Frequency: %d \n",
                line_number++, freq);
        Sleep(100);
        Gui_send_param(CMD_GET_SET_LAST_USED_FREQ, freq);
        mode = G_last_used_stack_VFOB[selected_band].mode;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Send_last_used_VFO_B . Calling Gui_send_param . CMD_GET_SET_LAST_USED_MODE . Mode: %c \n",
                line_number++, mode);
        Sleep(100);
        Gui_send_param(CMD_GET_SET_LAST_USED_MODE, mode);
        Sleep(100);

    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Send_last_used_VFO_B-> Finished . band: %d, freq: %ld, mode: %c\n", line_number++,
            selected_band, freq, mode);
    return 1;
}

int Get_Startup(uint32_t *freq, char *mode) {
    int status = 0;
    FILE *Startup_ini = NULL;
    char file_name[PATH_MAX] = {0};
    char startup_init_record[132];

    struct {
        char *freq_ptr;
        char *mode_ptr;
        uint32_t freq;
        char mode;
    } startup_record;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Get_Startup - Called. \n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(file_name, homedir);
        strcat(file_name, "/startup.ini");

        Startup_ini = fopen(file_name, "r");
        if (Startup_ini != NULL) {
            while (fgets(startup_init_record, sizeof (startup_init_record), Startup_ini) != NULL) {
                startup_record.freq_ptr = strstr(startup_init_record, "FREQUENCY=");
                if (startup_record.freq_ptr != NULL) {
                    startup_record.freq = atoi(((startup_record.freq_ptr + (sizeof ("FREQUENCY=")))) - 1);
                }
                startup_record.mode_ptr = strstr(startup_init_record, "MODE=");
                if (startup_record.mode_ptr != NULL) {
                    startup_record.mode_ptr = (startup_record.mode_ptr + (sizeof ("MODE=")) - 1);
                    startup_record.mode = *startup_record.mode_ptr;
                }
            }
            fclose(Startup_ini);
            *freq = startup_record.freq;
            *mode = startup_record.mode;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Get_Startup.  Freq: %d, mode %c \n", line_number++, *freq, *mode);
        }
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Get_Startup . Finished\n", line_number++);
    return status;
}

int Update_Startup(uint32_t freq, char mode) {
    int status = 0;
    FILE *Startup_ini = NULL;
    char file_name[PATH_MAX] = {0};

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_Startup - Called. freq: %d, mode: %c\n", line_number++, freq, mode);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(file_name, homedir);
        strcat(file_name,"/startup.ini");
        Startup_ini = fopen(file_name, "w");
        if (Startup_ini != NULL) {
            fprintf(Startup_ini, "FREQUENCY=%d;\n", freq);
            fprintf(Startup_ini, "MODE=%c;\n", mode);
            fflush(Startup_ini);
            fclose(Startup_ini);
        }
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_Startup - Finished \n", line_number++);
    return status;
}

int Update_last_used_VFO_A() {
    FILE *Last_used_update = NULL;
    char file_name[PATH_MAX] = {0};
    int band = 0;
    static UINT32 previous_freq = 0;
    static char previous_mode = 'n';

    if (!G_setting_last_used) {
        if (G_band_normal != GENERAL_BAND && G_TX == FALSE) {
            G_updating_last_used = TRUE;
            if (previous_freq != G_tune_freq || previous_mode != G_mode) {
                if (!G_dll_active) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_A . Previous Freq: %ld, G_tune_freq: %ld, Previous Mode %c, G_mode %c \n",
                            line_number++, previous_freq, G_tune_freq, previous_mode, G_mode);
                    if ((homedir = My_getenv("HOME")) != NULL) {
                        strcpy(file_name, homedir);
                        strcat(file_name, "/vfoa/Last_used.ini");
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_A . Open: %s \n", line_number++, file_name);
                        Last_used_update = fopen(file_name, "w");
                        if (Last_used_update != NULL) {
                            for (band = 0; band < 12; band++) {
                                if (band > 9 && G_pcb_version != GEMINUS) {
                                    G_last_used_stack_VFOA[band].freq = 10000000;
                                    G_last_used_stack_VFOA[band].mode = 'N';
                                }
                                fprintf(Last_used_update, "BAND=%d,FREQ=%d,MODE=%c;\n", band,
                                        G_last_used_stack_VFOA[band].freq, G_last_used_stack_VFOA[band].mode);
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_A . Update . Band: %d, Freq: %ld, Mode: %c \n",
                                        line_number++, band, G_last_used_stack_VFOA[band].freq,
                                        G_last_used_stack_VFOA[band].mode);
                            }
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_A . Finished \n", line_number++);
                        } else {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_A . Open file failed\n", line_number++);
                            return 0;
                        }
                        if (Last_used_update != NULL) {
                            fflush(Last_used_update);
                            fclose(Last_used_update);
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_A . Last_used File Closed\n", line_number++);
                        } else {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_A . Last_used File is NOT Open but should be open\n",
                                    line_number++);
                        }
                    }
                    previous_freq = G_tune_freq;
                    previous_mode = G_mode;
                } else {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_A . G_dll_active is true. Will update on next call to update_last_used \n", line_number++);
                }
            }
            G_updating_last_used = FALSE;
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_A . G_setting_last_used is TRUE. Will update on next call to update_last_used \n", line_number++);
    }
    return 1;
}

int Update_last_used_VFO_B() {
    FILE *Last_used_update = NULL;
    char file_name[PATH_MAX] = {0};
    int band = 0;
    static UINT32 previous_freq = 0;
    static char previous_mode = 'n';

    if (!G_setting_last_used) {
        if (G_band_normal != GENERAL_BAND && G_TX == FALSE) {
            G_updating_last_used = TRUE;
            if (previous_freq != G_tune_freq || previous_mode != G_mode) {
                if (!G_dll_active) {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_B . Previous Freq: %ld, G_tune_freq: %ld, Previous Mode %c, G_mode %c \n",
                            line_number++, previous_freq, G_tune_freq, previous_mode, G_mode);
                    if ((homedir = My_getenv("HOME")) != NULL) {
                        strcpy(file_name, homedir);
                        strcat(file_name, "/vfob/Last_used.ini");
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_B . Open: %s \n", line_number++, file_name);
                        Last_used_update = fopen(file_name, "w");
                        if (Last_used_update != NULL) {
                            for (band = 0; band < 12; band++) {
                                if (band > 9 && G_pcb_version != GEMINUS) {
                                    G_last_used_stack_VFOB[band].freq = 10000000;
                                    G_last_used_stack_VFOB[band].mode = 'N';
                                }
                                fprintf(Last_used_update, "BAND=%d,FREQ=%d,MODE=%c;\n", band,
                                        G_last_used_stack_VFOB[band].freq, G_last_used_stack_VFOB[band].mode);
                                print_time(0);
                                fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_B . Update . Band: %d, Freq: %ld, Mode: %c \n",
                                        line_number++, band, G_last_used_stack_VFOB[band].freq,
                                        G_last_used_stack_VFOB[band].mode);
                            }
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_B . Finished \n", line_number++);
                        } else {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_B . Open file failed\n", line_number++);
                            return 0;
                        }
                        if (Last_used_update != NULL) {
                            fflush(Last_used_update);
                            fclose(Last_used_update);
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_B . Last_used File Closed\n", line_number++);
                        } else {
                            print_time(0);
                            fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_B . Last_used File is NOT Open but should be open\n",
                                    line_number++);
                        }
                    }
                    previous_freq = G_tune_freq;
                    previous_mode = G_mode;
                } else {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_B . G_dll_active is true. Will update on next call to update_last_used \n", line_number++);
                }
            }
            G_updating_last_used = FALSE;
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_last_used_VFO_B . G_setting_last_used is TRUE. Will update on next call to update_last_used \n", line_number++);
    }
    return 1;
}

int Get_Last_Band(uint32_t i) {
    int band = 0;
    if (i >= 135500 && i <= 138000) band = B2200;
    if (i >= 471000 && i <= 480000) band = B630;
    if (i >= 1780000 && i <= 2010000) band = B160;
    if (i >= 3480000 && i <= 4010000) band = B80;
    if (i >= 5310500 && i <= 5413500) band = B60;
    if (i >= 6980000 && i <= 7310000) band = B40;
    if (i >= 10080000 && i <= 10160000) band = B30;
    if (i >= 18048000 && i <= 18170000) band = B17;
    if (i >= 13980000 && i <= 14360000) band = B20;
    if (i >= 20980000 && i <= 21460000) band = B15;
    if (i >= 24870000 && i <= 25000000) band = B12;
    if (i >= 27980000 && i <= 29710000) band = B10;
    return band;
}

int Set_last_used_VFO_A(uint32_t i, char mode) {
    int status = TRUE;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Set_last_used_VFO_A called . Freq: %ld, Mode: %c \n", line_number++, i, mode);
    if (mode == 'A' || mode == 'L' || mode == 'U' || mode == 'C') {
        if (!G_updating_last_used) {
            G_setting_last_used = TRUE;
            G_last_bands = STARTUP_BAND;
            G_last_bands = Get_Last_Band(i);
            G_last_used_stack_VFOA[G_last_bands].freq = i;
            G_last_used_stack_VFOA[G_last_bands].mode = mode;
            G_setting_last_used = FALSE;
        } else {
            status = FALSE;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Set_last_used_VFO_A . G_updating_last_used is TRUE. \n", line_number++);
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Set_last_used_VFO_A . Invalid mode: %c \n", line_number++, mode);
    }
    //update_last_used();
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Set_last_used_VFO_A . finished \n", line_number++);
    return status;
}

int Set_last_used_VFO_B(uint32_t i, char mode) {
    int status = TRUE;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Set_last_used_VFO_B called . Freq: %ld, Mode: %c \n", line_number++, i, mode);
    if (mode == 'A' || mode == 'L' || mode == 'U' || mode == 'C') {
        if (!G_updating_last_used) {
            G_setting_last_used = TRUE;
            G_last_bands = STARTUP_BAND;
            G_last_bands = Get_Last_Band(i);
            G_last_used_stack_VFOB[G_last_bands].freq = i;
            G_last_used_stack_VFOB[G_last_bands].mode = mode;
            G_setting_last_used = FALSE;
        } else {
            status = FALSE;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Set_last_used_VFO_B . G_updating_last_used is TRUE. \n", line_number++);
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Set_last_used_VFO_B . Invalid mode: %c \n", line_number++, mode);
    }
    //update_last_used();
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Set_last_used_VFO_B . finished \n", line_number++);
    return status;
}

int last_used_corrupt() {
    int status = 0;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] last_used_corrupt Called.\n", line_number++);
    Stop_all(0, 0);
    return status;
}

int Create_Last_Used() {
    int status = 1;
    int ret = 0;
    int band_count = 0;
    int element_1;
    FILE *Last_used_ini;
    char file_name[PATH_MAX] = {0};
    char mode = 'N';

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(file_name, homedir);
        strcat(file_name, "/vfoa/Last_used.ini");
        print_time(1);
        fprintf(G_fp_logfile, "[%d] Create_Last_Used . Last_used Path: %s\n", line_number++, homedir);
        Last_used_ini = fopen(file_name, "w");
        if (Last_used_ini != NULL) {
            for (band_count = 0; band_count < 12; band_count++) {
                switch (band_count) {
                    case 11:
                        element_1 = 135700;
                        break;
                    case 10:
                        element_1 = 472000;
                        break;
                    case 9:
                        element_1 = 1800000;
                        break;
                    case 8:
                        element_1 = 3500000;
                        break;
                    case 7:
                        element_1 = 5332000;
                        break;
                    case 6:
                        element_1 = 7000000;
                        break;
                    case 5:
                        element_1 = 10100000;
                        break;
                    case 4:
                        element_1 = 14000000;
                        break;
                    case 3:
                        element_1 = 18068000;
                        break;
                    case 2:
                        element_1 = 21000000;
                        break;
                    case 1:
                        element_1 = 24890000;
                        break;
                    case 0:
                        element_1 = 28000000;
                        break;
                }
                switch (band_count) {
                    case 9:
                        mode = 'L';
                        fprintf(Last_used_ini, "BAND=%d,FREQ=%d,MODE=%c;\n", band_count, element_1, mode);
                        break;
                    case 8:
                        mode = 'L';
                        fprintf(Last_used_ini, "BAND=%d,FREQ=%d,MODE=%c;\n", band_count, element_1, mode);
                        break;
                    case 6:
                        mode = 'L';
                        fprintf(Last_used_ini, "BAND=%d,FREQ=%d,MODE=%c;\n", band_count, element_1, mode);
                        break;
                    default:
                        mode = 'U';
                        fprintf(Last_used_ini, "BAND=%d,FREQ=%d,MODE=%c;\n", band_count, element_1, mode);
                }
                print_time(0);
                fprintf(G_fp_logfile, "[%d] Create_Last_Used . BAND: %d, FREQUENCY: %d, MODE: %c\n", line_number++,
                        band_count, element_1, mode);
            }
            fflush(Last_used_ini);
            fclose(Last_used_ini);
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Create_Last_Used . Finished\n", line_number++);
        } else {
            status = 0;
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Create_Last_Used . Create FAILED \n", line_number++);
        }
    } else {
        status = 0;
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_Last_Used . SHGetKnownFolderPath FAILED \n", line_number++);
    }
    return status;
}

int Init_last_used_VFO_A() {
    FILE *Last_used_ini;
    char file_name[PATH_MAX] = {0};
    char cw_init_record[132];
    int status = 1;

    struct {
        char *band;
        char *freq;
        char *mode;
    } last_used_record;
    int my_number;
    uint8_t band;

    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(1);
        fprintf(G_fp_logfile, "[%d] Init_last_used_VFO_A . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/vfoa/Last_used.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Init_last_used_VFO_A . Last_used Path: %s\n", line_number++, file_name);
        Last_used_ini = fopen(file_name, "r");
        if (Last_used_ini != NULL) {
            while (fgets(cw_init_record, sizeof (cw_init_record), Last_used_ini) != NULL) {
                last_used_record.band = strstr(cw_init_record, "BAND=");
                last_used_record.freq = strstr(cw_init_record, "FREQ=");
                last_used_record.mode = strstr(cw_init_record, "MODE=");
                if (last_used_record.band != NULL && last_used_record.freq != NULL && last_used_record.mode != NULL) {
                    my_number = atoi((last_used_record.band + 5));
                    band = my_number;
                    G_last_used_stack_VFOA[band].band = my_number;
                    my_number = atoi((last_used_record.freq + 5));
                    G_last_used_stack_VFOA[band].freq = my_number;
                    G_last_used_stack_VFOA[band].mode = *((last_used_record.mode + 5));
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_last_used_VFO_A . Band: %d, Frequency: %d, MODE: %c\n", line_number++,
                            G_last_used_stack_VFOA[band].band, G_last_used_stack_VFOA[band].freq,
                            G_last_used_stack_VFOA[band].mode);
                } else {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_last_used_VFO_A . INVALID RECORD. last_used.ini will be created\n", line_number++);
                    status = 0;
                }
            }
            if (Last_used_ini != NULL) {
                fclose(Last_used_ini);
            }
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Init_last_used_VFO_A . Open file failed. Last_used.ini will be created\n", line_number++);
            status = 0;
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Init_last_used_VFO_A . Finished\n", line_number++);

    }
    return status;
}

int Init_last_used_VFO_B() {
    FILE *Last_used_ini;
    char file_name[PATH_MAX] = {0};
    char cw_init_record[132];
    int status = 1;

    struct {
        char *band;
        char *freq;
        char *mode;
    } last_used_record;
    int my_number;
    uint8_t band;

    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(1);
        fprintf(G_fp_logfile, "[%d] Init_last_used_VFO_B . Default Path: %s\n", line_number++, homedir);
        strcpy(file_name, homedir);
        strcat(file_name, "/vfob/Last_used.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Init_last_used_VFO_B . Last_used Path: %s\n", line_number++, file_name);
        Last_used_ini = fopen(file_name, "r");
        if (Last_used_ini != NULL) {
            while (fgets(cw_init_record, sizeof (cw_init_record), Last_used_ini) != NULL) {
                last_used_record.band = strstr(cw_init_record, "BAND=");
                last_used_record.freq = strstr(cw_init_record, "FREQ=");
                last_used_record.mode = strstr(cw_init_record, "MODE=");
                if (last_used_record.band != NULL && last_used_record.freq != NULL && last_used_record.mode != NULL) {
                    my_number = atoi((last_used_record.band + 5));
                    band = my_number;
                    G_last_used_stack_VFOB[band].band = my_number;
                    my_number = atoi((last_used_record.freq + 5));
                    G_last_used_stack_VFOB[band].freq = my_number;
                    G_last_used_stack_VFOB[band].mode = *((last_used_record.mode + 5));
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_last_used_VFO_B . Band: %d, Frequency: %d, MODE: %c\n", line_number++,
                            G_last_used_stack_VFOB[band].band, G_last_used_stack_VFOB[band].freq,
                            G_last_used_stack_VFOB[band].mode);
                } else {
                    print_time(0);
                    fprintf(G_fp_logfile, "[%d] Init_last_used_VFO_B . INVALID RECORD. last_used.ini will be created\n", line_number++);
                    status = 0;
                }
            }
            if (Last_used_ini != NULL) {
                fclose(Last_used_ini);
            }
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Init_last_used_VFO_B . Open file failed. Last_used.ini will be created\n", line_number++);
            status = 0;
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Init_last_used_VFO_B . Finished\n", line_number++);

    }
    return status;
}

