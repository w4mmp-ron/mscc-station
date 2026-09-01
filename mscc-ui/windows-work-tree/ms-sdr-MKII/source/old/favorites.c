#define _CRT_SECURE_NO_WARNINGS 1
#include "extern.h"
#include <math.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include <stdio.h>
#include "port_defines.h"
#include "usbavrcmd.h"
//#include "SRDLL.h"


#define MAX_BAND 10
#define MAX_RECORD_COUNT 100

struct {

    struct {
        char name[20];
        char freq[20];
        char mode[10];
        uint8_t hi_cut;
        uint8_t cw_bw;
        uint8_t tx_hi_cut;
        uint8_t delete;
    } record[MAX_RECORD_COUNT];
    int next_record;
    int num_records;
} favorites[MAX_BAND];

char G_favorites_name[PATH_MAX] = {0};
//const char *homedir;

int Set_favorites_name(char *fav_name, int size) {
    int status = 0;

    memset(G_favorites_name, 0, sizeof (G_favorites_name));
    strcpy(G_favorites_name, &fav_name[1]);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Get_favorites_name . Name: %s, Size: %d\n", line_number++, G_favorites_name, size);
    return status;
}

/*int Update_favorites_file(char *favorites_file_name, int band, int record_count) {
    FILE *Favs_stack_ini;
    int record_number = 0;
    char favorites_name[PATH_MAX] = {0};

    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_favorites_file . Default Path: %s\n", line_number++, homedir);
        strcpy(favorites_name, homedir);
        //strcat(favorites_name, "/.local/share/mscc/");
        strcat(favorites_name, favorites_file_name);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_favorites_file . Favorites Path: %s\n", line_number++, favorites_name);
        Favs_stack_ini = fopen(favorites_name, "w");
        if (Favs_stack_ini != NULL) {
            for (record_number = 0; record_number < (record_count); record_number++) {
                fprintf(Favs_stack_ini, "NAME=%s,FREQ=%s,MODE=%s,HI_CUT=%d,CW_BW=%d,TX_HI_CUT=%d,D=%d;\n",
                        favorites[band].record[record_number].name,
                        favorites[band].record[record_number].freq,
                        favorites[band].record[record_number].mode,
                        favorites[band].record[record_number].hi_cut,
                        favorites[band].record[record_number].cw_bw,
                        favorites[band].record[record_number].tx_hi_cut,
                        favorites[band].record[record_number].delete);

                print_time(0);
                fprintf(G_fp_logfile, "[%d] Update_favorites_file . Name: %s, Freq: %s, Mode: %s, Hi_Cut:%d, CW_BW:%d, TX_HI_CUT: %d, Delete: %d\n", line_number++,
                        favorites[band].record[record_number].name,
                        favorites[band].record[record_number].freq,
                        favorites[band].record[record_number].mode,
                        favorites[band].record[record_number].hi_cut,
                        favorites[band].record[record_number].cw_bw,
                        favorites[band].record[record_number].tx_hi_cut,
                        favorites[band].record[record_number].delete);
            }
            fclose(Favs_stack_ini);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Update_favorites_file . File Open failed\n", line_number++);
            return 0;
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_favorites_file . getenv failed\n", line_number++);
        return 0;
    }
    return 1;
}
*/
void Get_Favorites_File_name(int band, char *file_name) {

    switch (band) {
        case 0:
            strcpy(file_name, "/b10_favs.ini");
            break;
        case 1:
            strcpy(file_name, "/b12_favs.ini");
            break;
        case 2:
            strcpy(file_name, "/b15_favs.ini");
            break;
        case 3:
            strcpy(file_name, "/b17_favs.ini");
            break;
        case 4:
            strcpy(file_name, "/b20_favs.ini");
            break;
        case 5:
            strcpy(file_name, "/b30_favs.ini");
            break;
        case 6:
            strcpy(file_name, "/b40_favs.ini");
            break;
        case 7:
            strcpy(file_name, "/b60_favs.ini");
            break;
        case 8:
            strcpy(file_name, "/b80_favs.ini");
            break;
        case 9:
            strcpy(file_name, "/b160_favs.ini");
            break;
        default:
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Get_Favorites_File_name . Invalid Band\n", line_number++);
    }
}

int Get_Band_To_Index(int band) {
    int index = 100;

    switch (band) {
        case 10:
            index = 0;
            break;
        case 12:
            index = 1;
            break;
        case 15:
            index = 2;
            break;
        case 17:
            index = 3;
            break;
        case 20:
            index = 4;
            break;
        case 30:
            index = 5;
            break;
        case 40:
            index = 6;
            break;
        case 60:
            index = 7;
            break;
        case 80:
            index = 8;
            break;
        case 160:
            index = 9;
            break;
    }
    return index;
}

int Get_band(uint32_t freq, uint8_t apply_offset) {
    int band = FREQ_OUT_OF_RANGE;
    uint32_t offset = 0;

    if (apply_offset) {
        offset = 12000;
    }
    if (freq >= (1800000 - offset) && freq <= (2000000 + offset)) band = 9;
    if (freq >= 3500000 - offset && freq <= 4000000 + offset) band = 8;
    if (freq >= 5330500 - offset && freq <= 5403500 + offset) band = 7;
    if (freq >= 7000000 - offset && freq <= 7300000 + offset) band = 6;
    if (freq >= 10100000 - offset && freq <= 10150000 + offset) band = 5;
    if (freq >= 14000000 - offset && freq <= 14350000 + offset) band = 4;
    if (freq >= 18068000 - offset && freq <= 18168000 + offset) band = 3;
    if (freq >= 21000000 - offset && freq <= 21450000 + offset) band = 2;
    if (freq >= 24890000 - offset && freq <= 24990000 + offset) band = 1;
    if (freq >= 28000000 - offset && freq <= 29700000 + offset) band = 0;
    return band;
}

int Update_favorites(uint32_t freq, char mode) {
    FILE* Favs_stack_ini;
    int record_number = 0;
    char favorites_file_name[PATH_MAX] = { 0 };
    int band = 0;
    char frequency[20] = { 0 };
    int next_record = 0;
    char new_mode[5] = { 0 };
    int record_count = 0;
    struct {
        char name[20];
        char freq[20];
        char mode[10];
        uint8_t hi_cut;
        uint8_t cw_bw;
        uint8_t tx_hi_cut;
        uint8_t delete;
    } favorites;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_favorites. Called. Freq: %ld, Mode: %c\n", line_number++, freq, mode);
    band = Get_band(freq, 1);
    if (band == FREQ_OUT_OF_RANGE) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_favorites . Invalid Band\n", line_number++);
        return 0;
    }
    switch (mode) {
    case 'A':
        strcpy(new_mode, "AM");
        break;
    case 'L':
        strcpy(new_mode, "LSB");
        break;
    case 'U':
        strcpy(new_mode, "USB");
        break;
    case 'C':
        strcpy(new_mode, "CW");
        break;
    }
    strcpy(favorites.name, G_favorites_name);
    sprintf(favorites.freq, "%ld", freq);
    strcpy(favorites.mode, new_mode);
    favorites.cw_bw = Current_Filters.CW_BW;
    favorites.hi_cut = Current_Filters.High_Cut;
    favorites.tx_hi_cut = User_Controls.TX_hi_cut;
    favorites.delete = 0;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_favorites. Name: %s, Freq: %s, Mode: %s\n", line_number++,
        favorites.name,favorites.freq, favorites.mode);

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(favorites_file_name, homedir);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_favorites. Default Path: %s\n", line_number++, favorites_file_name);
        switch (band) {
        case 0:
            strcat(favorites_file_name, "/b10_favs.ini");
            break;
        case 1:
            strcat(favorites_file_name, "/b12_favs.ini");
            break;
        case 2:
            strcat(favorites_file_name, "/b15_favs.ini");
            break;
        case 3:
            strcat(favorites_file_name, "/b17_favs.ini");
            break;
        case 4:
            strcat(favorites_file_name, "/b20_favs.ini");
            break;
        case 5:
            strcat(favorites_file_name, "/b30_favs.ini");
            break;
        case 6:
            strcat(favorites_file_name, "/b40_favs.ini");
            break;
        case 7:
            strcat(favorites_file_name, "/b60_favs.ini");
            break;
        case 8:
            strcat(favorites_file_name, "/b80_favs.ini");
            break;
        case 9:
            strcat(favorites_file_name, "/b160_favs.ini");
            break;
        default:
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Update_favorites. Invalid Band\n", line_number++);
        }
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_favorites. Favorites File Name: %s\n", line_number++, favorites_file_name);
        Favs_stack_ini = fopen(favorites_file_name, "a");
        if (Favs_stack_ini != NULL) {
            fprintf(Favs_stack_ini, "NAME=%s,FREQ=%s,MODE=%s,HI_CUT=%d,CW_BW=%d,TX_HI_CUT=%d,D=%d;\n",
                favorites.name,
                favorites.freq,
                favorites.mode,
                favorites.hi_cut,
                favorites.cw_bw,
                favorites.tx_hi_cut,
                favorites.delete);
            fflush(Favs_stack_ini);
            fclose(Favs_stack_ini);
        }
        else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Update_favorites_file . File Open failed\n", line_number++);
            return 0;
        }
    }
    else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_favorites_file . getenv failed\n", line_number++);
        return 0;
    }
    Sleep(1);
    Gui_send_param(CMD_GET_FAVS, band);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_favorites . Finished\n", line_number++);
    return 1;
}
/*int Update_favorites(uint32_t freq, char mode) {
    int band = 0;
    char frequency[20] = {0};
    int next_record = 0;
    char new_mode[5] = {0};
    int record_count = 0;

    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_favorites . Called . Freq: %ld, Mode: %c\n", line_number++, freq, mode);
    band = Get_band(freq, 1);
    if (band == FREQ_OUT_OF_RANGE) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Update_favorites . Invalid Band\n", line_number++);
        return 0;
    }
    next_record = favorites[band].next_record;
    record_count = favorites[band].num_records;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_favorites . next_record: %d, record_count :%d\n",
            line_number++, next_record, record_count);
    sprintf(frequency, "%ld", freq);
    memset(favorites[band].record[next_record].name, 0, sizeof (favorites[band].record[next_record].name));
    memset(favorites[band].record[next_record].freq, 0, sizeof (favorites[band].record[next_record].freq));
    memset(favorites[band].record[next_record].mode, 0, sizeof (favorites[band].record[next_record].mode));
    switch (mode) {
        case 'A':
            strcpy(new_mode, "AM");
            break;
        case 'L':
            strcpy(new_mode, "LSB");
            break;
        case 'U':
            strcpy(new_mode, "USB");
            break;
        case 'C':
            strcpy(new_mode, "CW");
            break;
    }
    strcpy(favorites[band].record[next_record].name, G_favorites_name);
    strcpy(favorites[band].record[next_record].freq, frequency);
    strcpy(favorites[band].record[next_record].mode, new_mode);
    favorites[band].record[next_record].cw_bw = Current_Filters.CW_BW;
    favorites[band].record[next_record].hi_cut = Current_Filters.High_Cut;
    favorites[band].record[next_record].tx_hi_cut = User_Controls.TX_hi_cut;
    favorites[band].record[next_record].delete = 0;
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_favorites . Next Record: %d, Name: %s, Freq: %s, Mode: %s\n", line_number++,
            next_record, favorites[band].record[next_record].name,
            favorites[band].record[next_record].freq, favorites[band].record[next_record].mode);
    next_record++;
    record_count++;
    if (record_count > MAX_RECORD_COUNT) {
        record_count = MAX_RECORD_COUNT;
    }
    favorites[band].num_records = record_count;

    if (next_record >= MAX_RECORD_COUNT) {
        favorites[band].next_record = 0;
    } else {
        favorites[band].next_record = next_record;
    }
    switch (band) {
        case 0:
            Update_favorites_file("/b10_favs.ini", band, record_count);
            break;
        case 1:
            Update_favorites_file("/b12_favs.ini", band, record_count);
            break;
        case 2:
            Update_favorites_file("/b15_favs.ini", band, record_count);
            break;
        case 3:
            Update_favorites_file("/b17_favs.ini", band, record_count);
            break;
        case 4:
            Update_favorites_file("/b20_favs.ini", band, record_count);
            break;
        case 5:
            Update_favorites_file("/b30_favs.ini", band, record_count);
            break;
        case 6:
            Update_favorites_file("/b40_favs.ini", band, record_count);
            break;
        case 7:
            Update_favorites_file("/b60_favs.ini", band, record_count);
            break;
        case 8:
            Update_favorites_file("/b80_favs.ini", band, record_count);
            break;
        case 9:
            Update_favorites_file("/b160_favs.ini", band, record_count);
            break;
        default:
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Update_favorites . Invalid Band\n", line_number++);
    }
    Gui_send_param(CMD_GET_FAVS, band);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Update_favorites . Finished\n", line_number++);
    return 1;
}*/

int Create_favorites_files() {
    int status = 0;
    FILE *Favs_stack_ini;
    char temp_path[PATH_MAX] = {0};

    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_favorites_files . Default Path: %s\n", line_number++, homedir);
        strcpy(temp_path, homedir);
        //strcat(temp_path, "/.local/share/mscc/");
        strcat(temp_path, "b160_favs.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_favorites_files . Favorites Path: %s\n", line_number++, temp_path);
        Favs_stack_ini = fopen(temp_path, "w");
        fclose(Favs_stack_ini);

        strcpy(temp_path, homedir);
        //strcat(temp_path, "/.local/share/mscc/");
        strcat(temp_path, "b80_favs.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_favorites_files . Favorites Path: %s\n", line_number++, temp_path);
        Favs_stack_ini = fopen(temp_path, "w");
        fclose(Favs_stack_ini);

        strcpy(temp_path, homedir);
        //strcat(temp_path, "/.local/share/mscc/");
        strcat(temp_path, "b60_favs.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_favorites_files . Favorites Path: %s\n", line_number++, temp_path);
        Favs_stack_ini = fopen(temp_path, "w");
        fclose(Favs_stack_ini);

        strcpy(temp_path, homedir);
        //strcat(temp_path, "/.local/share/mscc/");
        strcat(temp_path, "b40_favs.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_favorites_files . Favorites Path: %s\n", line_number++, temp_path);
        Favs_stack_ini = fopen(temp_path, "w");
        fclose(Favs_stack_ini);

        strcpy(temp_path, homedir);
        //strcat(temp_path, "/.local/share/mscc/");
        strcat(temp_path, "b30_favs.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_favorites_files . Favorites Path: %s\n", line_number++, temp_path);
        Favs_stack_ini = fopen(temp_path, "w");
        fclose(Favs_stack_ini);

        strcpy(temp_path, homedir);
        //strcat(temp_path, "/.local/share/mscc/");
        strcat(temp_path, "b20_favs.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_favorites_files . Favorites Path: %s\n", line_number++, temp_path);
        Favs_stack_ini = fopen(temp_path, "w");
        fclose(Favs_stack_ini);

        strcpy(temp_path, homedir);
        //strcat(temp_path, "/.local/share/mscc/");
        strcat(temp_path, "b17_favs.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_favorites_files . Favorites Path: %s\n", line_number++, temp_path);
        Favs_stack_ini = fopen(temp_path, "w");
        fclose(Favs_stack_ini);

        strcpy(temp_path, homedir);
        //strcat(temp_path, "/.local/share/mscc/");
        strcat(temp_path, "b15_favs.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_favorites_files . Favorites Path: %s\n", line_number++, temp_path);
        Favs_stack_ini = fopen(temp_path, "w");
        fclose(Favs_stack_ini);

        strcpy(temp_path, homedir);
        //strcat(temp_path, "/.local/share/mscc/");
        strcat(temp_path, "b12_favs.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_favorites_files . Favorites Path: %s\n", line_number++, temp_path);
        Favs_stack_ini = fopen(temp_path, "w");
        fclose(Favs_stack_ini);

        strcpy(temp_path, homedir);
        //strcat(temp_path, "/.local/share/mscc/");
        strcat(temp_path, "b10_favs.ini");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Create_favorites_files . Favorites Path: %s\n", line_number++, temp_path);
        Favs_stack_ini = fopen(temp_path, "w");
        fclose(Favs_stack_ini);
    }
    return status;
}

int Init_favorites_structure(char *favorites_file_name, int band) {
    FILE *Favs_stack_ini;
    int lenght = 0;
    char favs_init_record[512];
    char *field_start;
    char *field_end;
    int record_number = 0;
    int record_count = 0;
    char favorites_name[PATH_MAX] = {0};
    char field[3] = {0};

    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Init_favorites_structure . Default Path: %s\n", line_number++, homedir);
        strcpy(favorites_name, homedir);
        //strcat(favorites_name, "/.local/share/mscc/");
        strcat(favorites_name, favorites_file_name);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Init_favorites_structure . Favorites Name: %s\n", line_number++, favorites_name);
        Favs_stack_ini = fopen(favorites_name, "r");
        if (Favs_stack_ini != NULL) {
            while (fgets(favs_init_record, sizeof (favs_init_record), Favs_stack_ini) != NULL &&
                    record_number < MAX_RECORD_COUNT) {
                field_start = strstr(favs_init_record, "NAME=");
                field_end = strstr(field_start, ",");
                strncat(favorites[band].record[record_number].name, &field_start[5], ((field_end - (field_start + 5))));
                
                field_start = strstr(favs_init_record, "FREQ=");
                field_end = strstr(field_start, ",");
                strncat(favorites[band].record[record_number].freq, &field_start[5], ((field_end - (field_start + 5))));
                
                field_start = strstr(favs_init_record, "MODE=");
                field_end = strstr(field_start, ",");
                strncat(favorites[band].record[record_number].mode, &field_start[5], ((field_end - (field_start + 5))));

                field_start = strstr(favs_init_record, "HI_CUT=");
                field_end = strstr(field_start, ",");
                memset(field, 0, sizeof (field));
                strncat(field, &field_start[strlen("HI_CUT=")], ((field_end - (field_start + strlen("HI_CUT=")))));
                favorites[band].record[record_number].hi_cut = atoi(field);

                field_start = strstr(favs_init_record, "CW_BW=");
                field_end = strstr(field_start, ",");
                memset(field, 0, sizeof (field));
                strncat(field, &field_start[strlen("CW_BW=")], ((field_end - (field_start + strlen("CW_BW=")))));
                favorites[band].record[record_number].cw_bw = atoi(field);

                field_start = strstr(favs_init_record, "TX_HI_CUT=");
                field_end = strstr(field_start, ",");
                memset(field, 0, sizeof (field));
                strncat(field, &field_start[strlen("TX_HI_CUT=")], ((field_end - (field_start + strlen("TX_HI_CUT=")))));
                favorites[band].record[record_number].tx_hi_cut = atoi(field);

                field_start = strstr(favs_init_record, "D=");
                field_end = strstr(field_start, ";");
                memset(field, 0, sizeof (field));
                strncat(field, &field_start[strlen("D=")], ((field_end - (field_start + strlen("D=")))));
                favorites[band].record[record_number].delete = atoi(field);

                print_time(0);
                fprintf(G_fp_logfile, "[%d] Init_favorites_structure . Favorites Record: Name: %s, Freq: %s, Mode: %s \n",
                        line_number,
                        favorites[band].record[record_number].name, favorites[band].record[record_number].freq,
                        favorites[band].record[record_number].mode);
                record_number++;
                if (record_number >= MAX_RECORD_COUNT) {
                    record_number = 0;
                }
                favorites[band].next_record = record_number;
                record_count++;
                if (record_count > MAX_RECORD_COUNT) {
                    record_count = MAX_RECORD_COUNT;
                }
                favorites[band].num_records = record_count;
            }
            fclose(Favs_stack_ini);
        } else {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] Init_favorites_structure . File Open failed\n", line_number++);
            return 0;
        }
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Init_favorites_structure . getenv failed\n", line_number++);
        return 0;
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Init_favorites_structure . Finished. Filename: %s, record_number: %d, record_count: %d \n",
            line_number++, favorites_file_name, record_number, record_count);
    
    return 1;
}

int Init_favorites() {
    int status = 0;
    int band = 0;
    int record_number = 0;
    print_time(1);
    fprintf(G_fp_logfile, "[%d] Init_favorites . Called\n", line_number++);
    for (band = 0; band < MAX_BAND; band++) {
        for (record_number = 0; record_number < MAX_RECORD_COUNT; record_number++) {
            memset(favorites[band].record[record_number].freq, 0, sizeof (favorites[band].record[record_number].freq));
            memset(favorites[band].record[record_number].name, 0, sizeof (favorites[band].record[record_number].name));
            memset(favorites[band].record[record_number].mode, 0, sizeof (favorites[band].record[record_number].mode));
            favorites[band].record[record_number].cw_bw = 0;
            favorites[band].record[record_number].delete = 0;
            favorites[band].record[record_number].hi_cut = 0;
            favorites[band].record[record_number].tx_hi_cut = 0;
        }
        favorites[band].next_record = 0;
        favorites[band].num_records = 0;
    }
    status = Init_favorites_structure("b160_favs.ini", 9);
    if (status == 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Init_favorites . Favorites Initialization file not found . calling  Create_favorites_files \n", line_number++);
        Create_favorites_files();
        Init_favorites_structure("b160_favs.ini", 9);
    }
    Init_favorites_structure("b80_favs.ini", 8);
    Init_favorites_structure("b60_favs.ini", 7);
    Init_favorites_structure("b40_favs.ini", 6);
    Init_favorites_structure("b30_favs.ini", 5);
    Init_favorites_structure("b20_favs.ini", 4);
    Init_favorites_structure("b17_favs.ini", 3);
    Init_favorites_structure("b15_favs.ini", 2);
    Init_favorites_structure("b12_favs.ini", 1);
    Init_favorites_structure("b10_favs.ini", 0);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Init_favorites . Finished\n", line_number++);
    return status;
}

int Find_Favorites_Name(char *record, char *name) {
    int status = FALSE;
    char *field_start;
    char *field_end;

    //print_time(0);
    //fprintf(G_fp_logfile, "[%d] Find_Favorites_Name . record: %s, name: %s\n", line_number++, record, name);
    field_start = strstr(record, "NAME=");
    field_start = field_start + sizeof ("NAME");
    field_end = strstr(field_start, name);
    if (field_end != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Find_Favorites_Name . name found: %s\n", line_number++, field_end);
        status = TRUE;
    }
    return status;
}

int Favs_Delete_Fav(int band) {
    int status = 0;
    int index = 100;
    FILE *Favs_stack_ini;
    FILE *Favs_file_ini_temp;
    int record_found = FALSE;
    int lenght = 0;
    char favs_init_record[PATH_MAX] = {0};
    //char *field_start;
    //char *field_end;
    int record_number = 0;
    int record_count = 0;
    char favorites_file_name[PATH_MAX] = {0};
    char favorites_file_root[PATH_MAX] = {0};
    char favorites_file_temp[PATH_MAX] = {0};
    char field[3] = {0};

    index = Get_Band_To_Index(band);
    Get_Favorites_File_name(index, favorites_file_name);
    if ((homedir = My_getenv("HOME")) != NULL) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Favs_Delete_Fav . Default Path: %s\n", line_number++, homedir);
        strcpy(favorites_file_root, homedir);
        //strcat(favorites_file_root, "/.local/share/mscc/");
        strcat(favorites_file_root, favorites_file_name);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Favs_Delete_Fav . favorites_file_name: %s\n",
                line_number++, favorites_file_root);
        Favs_stack_ini = fopen(favorites_file_root, "r");

        strcpy(favorites_file_temp, favorites_file_root);
        strcat(favorites_file_temp, "-temp");
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Favs_Delete_Fav . favorites_file_temp %s\n",
                line_number++, favorites_file_temp);
        Favs_file_ini_temp = fopen(favorites_file_temp, "w");
        if (Favs_stack_ini != NULL && Favs_file_ini_temp != NULL) {
            while ((fgets(favs_init_record, sizeof (favs_init_record), Favs_stack_ini)) != NULL) {
                record_found = Find_Favorites_Name(favs_init_record, G_favorites_name);
                if (record_found == FALSE) {
                    fprintf(Favs_file_ini_temp, "%s", favs_init_record);
                }
            }
        }
        if (Favs_stack_ini != NULL) {
            fclose(Favs_stack_ini);
            if (Favs_file_ini_temp != NULL) {
                fclose(Favs_file_ini_temp);
                status = remove(favorites_file_root);
                if (status == 0) {
                    status = rename(favorites_file_temp, favorites_file_root);
                    if (status == 0) {
                        print_time(0);
                        fprintf(G_fp_logfile, "[%d] Favs_Delete_Fav . File Renamed %s\n",
                                line_number++, favorites_file_root);
                        //Init_favorites();
                        Gui_send_param(CMD_GET_FAVS, index);
                    }
                }
            }
        }
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Favs_Delete_Fav . Called. Band: %d, index: %d filename: %s, favorite_name: %s\n",
            line_number++, band, index, favorites_file_name, G_favorites_name);
    return status;
}