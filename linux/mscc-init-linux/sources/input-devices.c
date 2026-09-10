#include "extern.h"
//#include "commands.h"
//#include "version.h"

extern struct input_devices G_input_devices[MAX_INPUT_DEVICES];
int num_input_devices_found = 0;
int sound_ini_file_exists = 0;
const char *homedir;
char audio_device[MAX_PATH] = {0};

int Get_Sound_Device() {
    FILE *fp_Sound_ini;
    char l_path[MAX_PATH] = {0};
    char init_record[MAX_PATH] = {0};
    int status = 0;
    char *end_of_file;

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/.local/share/mscc");
        print_time();
        fprintf(G_fp_logfile, "[%d] Get_Sound_Device -> Default Path: %s\n", line_number++, l_path);
        strcat(l_path, "/sound-device.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] Get_Sound_Device -> sound-device.ini Path: %s\n", line_number++, l_path);
        fp_Sound_ini = fopen(l_path, "r");
        if (fp_Sound_ini != NULL) {
            end_of_file = fgets(init_record, sizeof (init_record), fp_Sound_ini);
            if (end_of_file != NULL) {
                strncpy(audio_device, init_record, (strlen(init_record) - 1));
                print_time();
                fprintf(G_fp_logfile, "[%d] Get_Sound_Device -> %s\n", line_number++, audio_device);
                print_time();
                fprintf(G_fp_logfile, "[%d] Get_Sound_Device -> input record: %s, audio_device: %s\n",
                        line_number++, init_record, audio_device);
                fclose(fp_Sound_ini);
            } else {
                print_time();
                fprintf(G_fp_logfile, "[%d] Get_Sound_Device -> FAIL: No record found\n", line_number++);
            }
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] Get_Sound_Device -> FAIL: file not found\n", line_number++);
        }
    }
    return status;
}

int delete_ini_file() {
    FILE *fp_Sound_ini;
    char l_path[MAX_PATH] = {0};
    int status = 0;
    int ret;

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/.local/share/mscc");
        print_time();
        fprintf(G_fp_logfile, "[%d] delete_ini_file -> Default Path: %s\n", line_number++, l_path);

        strcat(l_path, "/sdrcore_trans.ini");

        print_time();
        fprintf(G_fp_logfile, "[%d] delete_ini_file -> sdrcore_trans_ini Path: %s\n", line_number++, l_path);
        fp_Sound_ini = fopen(l_path, "r");
        if (fp_Sound_ini != NULL) {
            fclose(fp_Sound_ini);
            print_time();
            fprintf(G_fp_logfile, "[%d] delete_ini_file -> File Exists -> Will now delete \n", line_number++);
            ret = remove(l_path);
            if (ret == 0) {
                print_time();
                fprintf(G_fp_logfile, "[%d] delete_ini_file -> The initialization file has been deleted \n", line_number++);
                status = 1;
            } else {
                print_time();
                fprintf(G_fp_logfile, "[%d] delete_ini_file -> The initialization file Deletion has FAILED \n", line_number++);
                status = 0;
            }
        }
    }
    return status;
}

int check_for_sound_ini_file() {
    FILE *fp_Sound_ini;
    char l_path[MAX_PATH] = {0};
    int status = 0;

    print_time();
    fprintf(G_fp_logfile, "[%d] check_for_sound_ini_file -> Called.\n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/.local/share/mscc");
        print_time();
        fprintf(G_fp_logfile, "[%d] check_for_sound_ini_file -> Default Path: %s\n", line_number++, l_path);
        strcat(l_path, "/sdrcore_trans.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] check_for_sound_ini_file -> sdrcore_trans_ini Path: %s\n", line_number++, l_path);
        fp_Sound_ini = fopen(l_path, "r");
        if (fp_Sound_ini != NULL) {
            sound_ini_file_exists = 1;
            fclose(fp_Sound_ini);
            print_time();
            fprintf(G_fp_logfile, "[%d] check_for_sound_ini_file -> File Exists \n", line_number++);
            status = 1;
        } else {
            sound_ini_file_exists = 0;
            print_time();
            fprintf(G_fp_logfile, "[%d] check_for_sound_ini_file -> File does not Exist.  File will be created \n", line_number++);
            status = 0;
        }
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] check_for_sound_ini_file -> Finished.\n", line_number++);
    return status;
}

void init_sound_ini() {
    FILE *fp_Sound_ini = NULL;
    char l_path[MAX_PATH] = {0};
    int lenght = 0;
    int record = 0;
    char init_record[512];
    char *end_of_file;

    int mynumber;

    struct {
        char *record_number;
        char *record_number_sof;

        char *device_name;
        char *device_name_sof;
        char *device_name_eof;

        char *device_index;
        char *device_index_sof;

        char *channel;
        char *channel_sof;

        char *default_device;
        char *default_device_sof;

        char *selected;
        char *selected_sof;
    } device_record;

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/sdrcore_trans.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] init_sound_ini -> sdrcore_trans_ini Path: %s\n", line_number++, l_path);
        fp_Sound_ini = fopen(l_path, "r");
        if (fp_Sound_ini != NULL) {
            end_of_file = fgets(init_record, sizeof (init_record), fp_Sound_ini);
            while (end_of_file != NULL) {
                device_record.record_number = strstr(init_record, "RECORD_NUMBER");
                device_record.record_number_sof = strstr(device_record.record_number, "=");
                mynumber = atoi((device_record.record_number_sof + 1));
                G_input_devices[record].record_number = mynumber;
                print_time();
                fprintf(G_fp_logfile, "[%d] RECORD_NUMBER=% d\n,", line_number++, G_input_devices[record].record_number);

                device_record.device_name = strstr(init_record, "DEVICE_NAME");
                device_record.device_name_sof = strstr(device_record.device_name, "=");
                device_record.device_name_eof = strstr(device_record.device_name_sof, "^");
                lenght = device_record.device_name_eof - device_record.device_name_sof;
                memset(G_input_devices[record].name, 0, sizeof (G_input_devices[record].name));
                strncpy(G_input_devices[record].name, (device_record.device_name_sof + 1), (lenght - 1));
                print_time();
                fprintf(G_fp_logfile, "[%d] DEVICE NAME = %s\n,", line_number++, G_input_devices[record].name);

                device_record.device_index = strstr(init_record, "DEVICE_INDEX");
                device_record.device_index_sof = strstr(device_record.record_number, "=");
                mynumber = atoi((device_record.device_index_sof + 1));
                G_input_devices[record].device_index = mynumber;
                print_time();
                fprintf(G_fp_logfile, "[%d] DEVICE INDEX = %d\n,", line_number++, G_input_devices[record].device_index);

                device_record.channel = strstr(init_record, "CHANNEL_COUNT");
                device_record.channel_sof = strstr(device_record.channel, "=");
                mynumber = atoi((device_record.channel_sof + 1));
                G_input_devices[record].num_channels = mynumber;
                print_time();
                fprintf(G_fp_logfile, "[%d] DEVICE CHANNEL COUNT =% d\n,", line_number++, G_input_devices[record].num_channels);

                device_record.default_device = strstr(init_record, "DEFAULT");
                device_record.default_device_sof = strstr(device_record.default_device, "=");
                mynumber = atoi((device_record.default_device_sof + 1));
                G_input_devices[record].default_input_device = mynumber;
                print_time();
                fprintf(G_fp_logfile, "[%d] DEVICE DEFAULT SET = %d\n,", line_number++, G_input_devices[record].default_input_device);

                device_record.selected = strstr(init_record, "SELECTED");
                device_record.selected_sof = strstr(device_record.selected, "=");
                mynumber = atoi((device_record.selected_sof + 1));
                G_input_devices[record].selected_input_device = mynumber;
                print_time();
                fprintf(G_fp_logfile, "[%d] DEVICE SELECTED SET = %d\n,", line_number++, G_input_devices[record].selected_input_device);

                print_time();
                fprintf(G_fp_logfile, "[%d] RECORD_NUMBER=%d,DEVICE_NAME=%s,DEVICE_INDEX=%d,CHANNEL_COUNT=%d,DEFAULT=%d,SELECTED=%d;\n",
                        line_number, G_input_devices[record].record_number, G_input_devices[record].name,
                        G_input_devices[record].device_index, G_input_devices[record].num_channels,
                        G_input_devices[record].default_input_device, G_input_devices[record].selected_input_device);
                record++;
                num_input_devices_found = record;
                end_of_file = fgets(init_record, sizeof (init_record), fp_Sound_ini);
            }
            fclose(fp_Sound_ini);
        }
    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] init_sound_ini -> Open file failed\n", line_number++);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] init_sound_ini -> Finished\n", line_number++);
}

int update_sound_ini() {
    FILE *fp_Sound_ini;
    char l_path[MAX_PATH] = {0};
    int lenght = 0;
    int record = 0;
    int index = 0;

    print_time();
    fprintf(G_fp_logfile, "[%d] update_sound_ini -> Called.\n", line_number++);
    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/.local/share/mscc");
        print_time();
        fprintf(G_fp_logfile, "[%d] update_sound_ini -> Default Path: %s\n", line_number++, l_path);

        strcat(l_path, "/sdrcore_trans.ini");

        print_time();
        fprintf(G_fp_logfile, "[%d] update_sound_ini -> sdrcore_trans_ini Path: %s\n", line_number++, l_path);
        fp_Sound_ini = fopen(l_path, "w");
        if (fp_Sound_ini != NULL) {
            for (record = 0; record < num_input_devices_found; record++) {
                fprintf(fp_Sound_ini,
                        "RECORD_NUMBER=%d^DEVICE_NAME=%s^DEVICE_INDEX=%d^CHANNEL_COUNT=%d^DEFAULT=%d^SELECTED=%d;\n",
                        G_input_devices[record].record_number, G_input_devices[record].name, G_input_devices[record].device_index,
                        G_input_devices[record].num_channels, G_input_devices[record].default_input_device, 
                                        G_input_devices[record].selected_input_device);
            }
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] update_sound_ini -> Open file failed\n", line_number++);
            return 0;
        }
        print_time();
        fprintf(G_fp_logfile, "[%d] update_sound_ini -> Finished\n", line_number++);
        fclose(fp_Sound_ini);
    }
    return 1;
}

void print_input_devices(void) {
    int record;
    print_time();
    fprintf(G_fp_logfile, "[%d] print_input_devices -> Called.\n", line_number++);
    for (record = 0; record < num_input_devices_found; record++) {
        print_time();
        fprintf(G_fp_logfile, "[%d] print_input_devices -> RECORD_NUMBER=%d,DEVICE_NAME=%s,DEVICE_INDEX=%d,CHANNEL_COUNT=%d,DEFAULT=%d,SELECTED=%d;\n", line_number++,
                G_input_devices[record].record_number, G_input_devices[record].name, G_input_devices[record].device_index,
                G_input_devices[record].num_channels, G_input_devices[record].default_input_device, G_input_devices[record].selected_input_device);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] print_input_devices -> print_input_devices -> Finshed.\n", line_number++);
}

void set_selected_device(int record_index) {
    int index = 0;
    print_time();
    fprintf(G_fp_logfile, "[%d] set_selected_device -> Called with record_index: %d\n", line_number++, record_index);
    for (index = 0; index < num_input_devices_found; index++) {
        G_input_devices[index].selected_input_device = 0;
    }
    G_input_devices[record_index].selected_input_device = 1;
    print_time();
    fprintf(G_fp_logfile, "[%d] set_selected -> Finished\n", line_number++);
}

void build_input_devices(int device_index) {
    static int index = 0;
    char *input_device;

    print_time();
    fprintf(G_fp_logfile, "[%d] build_input_devices -> Called. device_index: %d\n", line_number++, device_index);
    if (index < MAX_INPUT_DEVICES) {
        input_device = strstr(lpInfo->name, audio_device);
        if (input_device != NULL) {
            G_input_device_index = index;
            print_time();
            fprintf(G_fp_logfile, "[%d] build_input_devices -> %s FOUND\n", line_number++, audio_device);
        }
        strcpy(G_input_devices[index].name, lpInfo->name);
        G_input_devices[index].device_index = device_index;
        G_input_devices[index].num_channels = lpInfo->maxInputChannels;
        G_input_devices[index].record_number = index;
        if (device_index == Pa_GetHostApiInfo(lpInfo->hostApi)->defaultInputDevice) {
            G_input_devices[index].default_input_device = 1;
        }
        print_time();
        fprintf(G_fp_logfile, "[%d] build_input_device -> index: %d, name: %s, dev_index: %d, channels: %d, record_num: %d \n",
                line_number++, index, G_input_devices[index].name, G_input_devices[index].device_index, G_input_devices[index].num_channels,
                G_input_devices[index].record_number);
        index++;
        num_input_devices_found = index;
    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] Number of output devices exceed permitted limit\n", line_number++);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] build_input_devices -> Finished.\n", line_number++);
}

