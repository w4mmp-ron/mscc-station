#include "extern.h"

extern struct output_devices G_output_devices[MAX_OUTPUT_DEVICES];

struct pulse_output_devices {
    int record_number;
    int device_index;
    char name[120];
    int num_channels;
    int default_output_device;
    int selected_output_device;
};
struct pulse_output_devices G_pulse_output_devices[MAX_OUTPUT_DEVICES];
int num_output_devices_found = 0;
int G_num_pulse_output_devices_found = 0;
int sound_ini_file_exists = 0;
char audio_device[MAX_PATH] = {0};
const char *homedir;

int Get_Sound_Device() {
    FILE *fp_Sound_ini;
    char l_path[MAX_PATH] = {0};
    char init_record[MAX_PATH] = {0};
    int status = 0;
    char *end_of_file;
    int record_length = 0;
    //int ret;
    //char audio_device_temp[MAX_PATH] = {0};

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
                //strncpy(audio_device, init_record,(strlen(init_record) - 1));
                record_length = strlen(init_record);
                strncpy(audio_device, init_record, (record_length - 1));
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

        strcat(l_path, "/sdrcore_recv.ini");

        print_time();
        fprintf(G_fp_logfile, "[%d] delete_ini_file -> sdrcore_recv.ini Path: %s\n", line_number++, l_path);
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

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/sdrcore_recv.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] check_for_sound_ini_file -> sdrcore_recv_ini Path: %s\n", line_number++, l_path);
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
        char *record_number_eof;
        char record_number_data[2];

        char *device_name;
        char *device_name_sof;
        char *device_name_eof;
        char *device_name_data[80];

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
        strcat(l_path, "/sdrcore_recv.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] init_sound_ini -> sdrcore_recv_ini Path: %s\n", line_number++, l_path);
        fp_Sound_ini = fopen(l_path, "r");
        if (fp_Sound_ini != NULL) {
            end_of_file = fgets(init_record, sizeof (init_record), fp_Sound_ini);
            while (end_of_file != NULL) {
                device_record.record_number = strstr(init_record, "RECORD_NUMBER");
                device_record.record_number_sof = strstr(device_record.record_number, "=");
                mynumber = atoi((device_record.record_number_sof + 1));
                G_output_devices[record].record_number = mynumber;
                print_time();
                fprintf(G_fp_logfile, "[%d] RECORD_NUMBER=% d\n,", line_number++, G_output_devices[record].record_number);

                device_record.device_name = strstr(init_record, "DEVICE_NAME");
                device_record.device_name_sof = strstr(device_record.device_name, "=");
                device_record.device_name_eof = strstr(device_record.device_name_sof, "^");
                lenght = device_record.device_name_eof - device_record.device_name_sof;
                memset(G_output_devices[record].name, 0, sizeof (G_output_devices[record].name));
                strncpy(G_output_devices[record].name, (device_record.device_name_sof + 1), (lenght - 1));
                print_time();
                fprintf(G_fp_logfile, "[%d] DEVICE NAME = %s\n,", line_number++, G_output_devices[record].name);

                device_record.device_index = strstr(init_record, "DEVICE_INDEX");
                device_record.device_index_sof = strstr(device_record.record_number, "=");
                mynumber = atoi((device_record.device_index_sof + 1));
                G_output_devices[record].device_index = mynumber;
                print_time();
                fprintf(G_fp_logfile, "[%d] DEVICE INDEX = %d\n,", line_number++, G_output_devices[record].device_index);

                device_record.channel = strstr(init_record, "CHANNEL_COUNT");
                device_record.channel_sof = strstr(device_record.channel, "=");
                mynumber = atoi((device_record.channel_sof + 1));
                G_output_devices[record].num_channels = mynumber;
                print_time();
                fprintf(G_fp_logfile, "[%d] DEVICE CHANNEL COUNT =% d\n,", line_number++, G_output_devices[record].num_channels);

                device_record.default_device = strstr(init_record, "DEFAULT");
                device_record.default_device_sof = strstr(device_record.default_device, "=");
                mynumber = atoi((device_record.default_device_sof + 1));
                G_output_devices[record].default_output_device = mynumber;
                print_time();
                fprintf(G_fp_logfile, "[%d] DEVICE DEFAULT SET = %d\n,", line_number++, G_output_devices[record].default_output_device);

                device_record.selected = strstr(init_record, "SELECTED");
                device_record.selected_sof = strstr(device_record.selected, "=");
                mynumber = atoi((device_record.selected_sof + 1));
                G_output_devices[record].selected_output_device = mynumber;
                print_time();
                fprintf(G_fp_logfile, "[%d] DEVICE SELECTED SET = %d\n,", line_number++, G_output_devices[record].selected_output_device);

                print_time();
                fprintf(G_fp_logfile, "[%d] RECORD_NUMBER=%d,DEVICE_NAME=%s,DEVICE_INDEX=%d,CHANNEL_COUNT=%d,DEFAULT=%d,SELECTED=%d;\n",
                        line_number, G_output_devices[record].record_number, G_output_devices[record].name,
                        G_output_devices[record].device_index, G_output_devices[record].num_channels,
                        G_output_devices[record].default_output_device, G_output_devices[record].selected_output_device);
                record++;
                num_output_devices_found = record;
                fflush(G_fp_logfile);
                end_of_file = fgets(init_record, sizeof (init_record), fp_Sound_ini);
            }
        }
    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] init_sound_ini -> Open file failed\n", line_number++);
    }
    print_time();
    fprintf(G_fp_logfile, "[%d] init_sound_ini -> Finished\n", line_number++);
    if (fp_Sound_ini != NULL) {
        fclose(fp_Sound_ini);
    }
}

int update_sound_ini() {
    FILE *fp_Sound_ini;
    char l_path[MAX_PATH] = {0};
    int lenght = 0;
    int record = 0;
    int index = 0;

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/.local/share/mscc");

        strcat(l_path, "/sdrcore_recv.ini");

        print_time();
        fprintf(G_fp_logfile, "[%d] update_sound_ini -> sdrcore_recv_ini Path: %s\n", line_number++, l_path);

        fp_Sound_ini = fopen(l_path, "w");
        if (fp_Sound_ini != NULL) {
            for (record = 0; record < num_output_devices_found; record++) {
                fprintf(fp_Sound_ini, "RECORD_NUMBER=%d^DEVICE_NAME=%s^DEVICE_INDEX=%d^CHANNEL_COUNT=%d^DEFAULT=%d^SELECTED=%d;\n",
                        G_output_devices[record].record_number, G_output_devices[record].name, G_output_devices[record].device_index,
                        G_output_devices[record].num_channels, G_output_devices[record].default_output_device, G_output_devices[record].selected_output_device);
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

int Update_Pulse_sound_ini(int index) {
    FILE *fp_Sound_ini;
    char l_path[MAX_PATH] = {0};
    int lenght = 0;
    int record = 0;

    if ((homedir = My_getenv("HOME")) != NULL) {
        strcpy(l_path, homedir);
        strcat(l_path, "/.local/share/mscc");
        strcat(l_path, "/sdrcore_recv_pulse.ini");
        print_time();
        fprintf(G_fp_logfile, "[%d] Update_Pulse_sound_ini -> sdrcore_recv_pulse.ini Path: %s\n", line_number++, l_path);
        fp_Sound_ini = fopen(l_path, "w");
        if (fp_Sound_ini != NULL) {
            for (record = 0; record < index; record++) {
                fprintf(fp_Sound_ini, "RECORD_NUMBER=%d^DEVICE_NAME=%s^DEVICE_INDEX=%d^CHANNEL_COUNT=%d^DEFAULT=%d^SELECTED=%d;\n",
                        G_pulse_output_devices[record].record_number, G_pulse_output_devices[record].name,
                        G_pulse_output_devices[record].device_index,
                        G_pulse_output_devices[record].num_channels,
                        G_pulse_output_devices[record].default_output_device,
                        G_pulse_output_devices[record].selected_output_device);
            }
        } else {
            print_time();
            fprintf(G_fp_logfile, "[%d] Update_Pulse_sound_ini -> Open file failed\n", line_number++);
            return 0;
        }
        print_time();
        fprintf(G_fp_logfile, "[%d] Update_Pulse_sound_ini -> Finished\n", line_number++);
        fclose(fp_Sound_ini);
    }
    return 1;
}

void print_output_devices(void) {
    int record;

    for (record = 0; record < num_output_devices_found; record++) {
        print_time();
        fprintf(G_fp_logfile, "[%d] RECORD_NUMBER=%d,DEVICE_NAME=%s,DEVICE_INDEX=%d,CHANNEL_COUNT=%d,DEFAULT=%d,SELECTED=%d;\n", line_number++,
                G_output_devices[record].record_number, G_output_devices[record].name, G_output_devices[record].device_index,
                G_output_devices[record].num_channels, G_output_devices[record].default_output_device, G_output_devices[record].selected_output_device);
    }
}

void set_selected_device(int record_index) {
    int index = 0;
    print_time();
    fprintf(G_fp_logfile, "[%d] set_selected called with record_index: %d\n", line_number++, record_index);
    for (index = 0; index < num_output_devices_found; index++) {
        G_output_devices[index].selected_output_device = 0;
    }
    G_output_devices[record_index].selected_output_device = 1;
    print_time();
    fprintf(G_fp_logfile, "[%d] set_selected -> Finished\n", line_number++);
}

void Print_pulse_output_devices(int device) {

    print_time();
    fprintf(G_fp_logfile, "[%d] RECORD_NUMBER=%d,DEVICE_NAME=%s,DEVICE_INDEX=%d,CHANNEL_COUNT=%d,DEFAULT=%d,SELECTED=%d;\n", line_number++,
            G_pulse_output_devices[device].record_number, G_pulse_output_devices[device].name,
            G_pulse_output_devices[device].device_index,
            G_pulse_output_devices[device].num_channels, G_pulse_output_devices[device].default_output_device,
            G_pulse_output_devices[device].selected_output_device);
}

void Build_pulse_output_devices(int device_index) {
    static int index = 0;
    char *output_device;
    int compare = 0;
    char proficio_device[120];

    if (index < MAX_OUTPUT_DEVICES) {
        /*output_device = strstr(lpInfo_Pulse->name, audio_device);
        if (output_device != NULL) {
            G_pulse_output_device_index = index;
            print_time();
            fprintf(G_fp_logfile, "[%d] Build_pulse_devices -> %s FOUND\n", line_number++, audio_device);
        }*/
        //strcpy(proficio_device, lpInfo_Pulse->name);
        print_time();
        fprintf(G_fp_logfile, "[%d] Build_pulse_output_devices -> name: %s\n",
                line_number++, lpInfo_Pulse->name);
        compare = strcmp(lpInfo_Pulse->name, "Monitor of Proficio Transceiver");
        if (compare == 0) {
            print_time();
            fprintf(G_fp_logfile, "[%d] Build_pulse_output_devices -> Proficio Found: %s\n",
                    line_number++, lpInfo_Pulse->name);
            if (lpInfo_Pulse->maxInputChannels == 2) {
                G_Pulse_Proficio_Device = device_index;
                print_time();
                fprintf(G_fp_logfile, "[%d] Build_pulse_output_devices -> Proficio Found: %s, G_Pulse_Proficio_Device: %d\n",
                        line_number++, lpInfo_Pulse->name, G_Pulse_Proficio_Device);
            }
        }
        if (lpInfo_Pulse->maxOutputChannels == 2) {
            strcpy(G_pulse_output_devices[index].name, lpInfo_Pulse->name);
            G_pulse_output_devices[index].device_index = device_index;
            G_pulse_output_devices[index].num_channels = lpInfo_Pulse->maxOutputChannels;
            G_pulse_output_devices[index].record_number = index;
            
            if (device_index == Pa_GetHostApiInfo(lpInfo_Pulse->hostApi)->defaultOutputDevice) {
            //if (device_index == 18) {
                G_pulse_output_devices[index].default_output_device = 1;
                G_Pulse_Output_Device = device_index;
                print_time();
                fprintf(G_fp_logfile, "[%d] Build_pulse_devices -> G_Pulse_Output_Device: %d\n",
                        line_number++, G_Pulse_Output_Device);
            }
            G_num_pulse_output_devices_found = index;
            index++;
        }
    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] Build_pulse_devices Number of output devices exceed permitted limit\n",
                line_number++);
    }
}

void build_output_devices(int device_index) {
    static int index = 0;
    char *output_device;

    if (index < MAX_OUTPUT_DEVICES) {
        output_device = strstr(lpInfo->name, audio_device);
        if (output_device != NULL) {
            G_output_device_index = index;
            print_time();
            fprintf(G_fp_logfile, "[%d] build_output_devices -> %s FOUND\n", line_number++, audio_device);
        }
        strcpy(G_output_devices[index].name, lpInfo->name);
        G_output_devices[index].device_index = device_index;
        G_output_devices[index].num_channels = lpInfo->maxOutputChannels;
        num_output_devices_found++;
        G_output_devices[index].record_number = index;
        if (device_index == Pa_GetHostApiInfo(lpInfo->hostApi)->defaultOutputDevice) {
            G_output_devices[index].default_output_device = 1;
        }
        index++;
        num_output_devices_found = index;

    } else {
        print_time();
        fprintf(G_fp_logfile, "[%d] Number of output devices exceed permitted limit\n", line_number++);
    }
}




